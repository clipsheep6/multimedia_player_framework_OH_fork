/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "recorder_server.h"
#include "map"
#include "media_log.h"
#include "media_errors.h"
#include "engine_factory_repo.h"
#include "param_wrapper.h"
#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "media_dfx.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderServer"};
    const std::map<OHOS::Media::RecorderServer::RecStatus, std::string> RECORDER_STATE_MAP = {
        {OHOS::Media::RecorderServer::REC_INITIALIZED, "initialized"},
        {OHOS::Media::RecorderServer::REC_CONFIGURED, "configured"},
        {OHOS::Media::RecorderServer::REC_PREPARED, "prepared"},
        {OHOS::Media::RecorderServer::REC_RECORDING, "recording"},
        {OHOS::Media::RecorderServer::REC_PAUSED, "paused"},
        {OHOS::Media::RecorderServer::REC_ERROR, "error"},
    };
}

namespace OHOS {
namespace Media {
const std::string START_TAG = "RecorderCreate->Start";
const std::string STOP_TAG = "RecorderStop->Destroy";
#define CHECK_STATUS_FAILED_AND_LOGE_RET(statusFailed, ret) \
    do { \
        if (statusFailed) { \
            MEDIA_LOGE("invalid status, current status is %{public}s", GetStatusDescription(status_).c_str()); \
            return ret; \
        }; \
    } while (false)

std::shared_ptr<IRecorderService> RecorderServer::Create()
{
    std::shared_ptr<RecorderServer> server = std::make_shared<RecorderServer>();
    int32_t ret = server->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to init RecorderServer");
    return server;
}

RecorderServer::RecorderServer()
    : taskQue_("RecorderServer")
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    taskQue_.Start();
}

RecorderServer::~RecorderServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto task = std::make_shared<TaskHandler<void>>([&, this] {
            recorderEngine_ = nullptr;
        });
        (void)taskQue_.EnqueueTask(task);
        (void)task->GetResult();
        taskQue_.Stop();
    }
}

int32_t RecorderServer::Init()
{
    MediaTrace trace("RecorderServer::Init");
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    uint64_t fullTokenId = IPCSkeleton::GetCallingFullTokenID();
    int32_t appUid = IPCSkeleton::GetCallingUid();
    int32_t appPid = IPCSkeleton::GetCallingPid();

    auto task = std::make_shared<TaskHandler<MediaServiceErrCode>>([&, this] {
        auto engineFactory = EngineFactoryRepo::Instance().GetEngineFactory(IEngineFactory::Scene::SCENE_RECORDER);
        CHECK_AND_RETURN_RET_LOG(engineFactory != nullptr, MSERR_CREATE_REC_ENGINE_FAILED,
            "failed to get factory");
        recorderEngine_ = engineFactory->CreateRecorderEngine(appUid, appPid, tokenId, fullTokenId);
        CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_CREATE_REC_ENGINE_FAILED,
            "failed to create recorder engine");
        return MSERR_OK;
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.Value() == MSERR_OK, result.Value(), "Result failed");

    status_ = REC_INITIALIZED;
    BehaviorEventWrite(GetStatusDescription(status_), "Recorder");
    return MSERR_OK;
}

const std::string& RecorderServer::GetStatusDescription(OHOS::Media::RecorderServer::RecStatus status)
{
    static const std::string ILLEGAL_STATE = "PLAYER_STATUS_ILLEGAL";
    CHECK_AND_RETURN_RET(status >= OHOS::Media::RecorderServer::REC_INITIALIZED &&
        status <= OHOS::Media::RecorderServer::REC_ERROR, ILLEGAL_STATE);

    return RECORDER_STATE_MAP.find(status)->second;
}

void RecorderServer::OnError(ErrorType errorType, int32_t errorCode)
{
    std::lock_guard<std::mutex> lock(cbMutex_);
    lastErrMsg_ = MSErrorToExtErrorString(static_cast<MediaServiceErrCode>(errorCode));
    FaultEventWrite(lastErrMsg_, "Recorder");
    CHECK_AND_RETURN(recorderCb_ != nullptr);
    recorderCb_->OnError(static_cast<RecorderErrorType>(errorType), errorCode);
}

void RecorderServer::OnInfo(InfoType type, int32_t extra)
{
    std::lock_guard<std::mutex> lock(cbMutex_);
    if (recorderCb_ != nullptr) {
        recorderCb_->OnInfo(type, extra);
    }
}

void RecorderServer::OnAudioCaptureChange(const AudioRecorderChangeInfo &audioRecorderChangeInfo)
{
    std::lock_guard<std::mutex> lock(cbMutex_);
    if (recorderCb_ != nullptr) {
        recorderCb_->OnAudioCaptureChange(audioRecorderChangeInfo);
    }
}

int32_t RecorderServer::SetVideoSource(VideoSourceType source, int32_t &sourceId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_INITIALIZED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.videoSource = source;
    config_.withVideo = true;
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->SetVideoSource(source, sourceId);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t RecorderServer::SetVideoEncoder(int32_t sourceId, VideoCodecFormat encoder)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.videoCodec = encoder;
    VidEnc vidEnc(encoder);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Configure(sourceId, vidEnc);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t RecorderServer::SetVideoSize(int32_t sourceId, int32_t width, int32_t height)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.width = width;
    config_.height = height;
    VidRectangle vidSize(width, height);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Configure(sourceId, vidSize);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t RecorderServer::SetVideoFrameRate(int32_t sourceId, int32_t frameRate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.frameRate = frameRate;
    VidFrameRate vidFrameRate(frameRate);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Configure(sourceId, vidFrameRate);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t RecorderServer::SetVideoEncodingBitRate(int32_t sourceId, int32_t rate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.bitRate = rate;
    VidBitRate vidBitRate(rate);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Configure(sourceId, vidBitRate);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t RecorderServer::SetVideoIsHdr(int32_t sourceId, bool isHdr)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.isHdr = isHdr;
    VidIsHdr vidIsHdr(isHdr);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Configure(sourceId, vidIsHdr);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t RecorderServer::SetCaptureRate(int32_t sourceId, double fps)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.captureRate = fps;
    CaptureRate captureRate(fps);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Configure(sourceId, captureRate);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

sptr<OHOS::Surface> RecorderServer::GetSurface(int32_t sourceId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_PREPARED && status_ != REC_RECORDING && status_ != REC_PAUSED,
        nullptr);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, nullptr, "engine is nullptr");
    auto task = std::make_shared<TaskHandler<sptr<OHOS::Surface>>>([&, this] {
        return recorderEngine_->GetSurface(sourceId);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t RecorderServer::SetAudioSource(AudioSourceType source, int32_t &sourceId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_INITIALIZED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");

    config_.audioSource = source;
    config_.withAudio = true;
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->SetAudioSource(source, sourceId);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t RecorderServer::SetAudioEncoder(int32_t sourceId, AudioCodecFormat encoder)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.audioCodec = encoder;
    AudEnc audEnc(encoder);
    MEDIA_LOGD("set audio encoder sourceId:%{public}d, encoder:%{public}d", sourceId, encoder);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Configure(sourceId, audEnc);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t RecorderServer::SetAudioSampleRate(int32_t sourceId, int32_t rate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.audioSampleRate = rate;
    AudSampleRate audSampleRate(rate);
    MEDIA_LOGD("set audio sampleRate sourceId:%{public}d, rate:%{public}d", sourceId, rate);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Configure(sourceId, audSampleRate);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t RecorderServer::SetAudioChannels(int32_t sourceId, int32_t num)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.audioChannel = num;
    AudChannel audChannel(num);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Configure(sourceId, audChannel);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t RecorderServer::SetAudioEncodingBitRate(int32_t sourceId, int32_t bitRate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.audioBitRate = bitRate;
    AudBitRate audBitRate(bitRate);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Configure(sourceId, audBitRate);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t RecorderServer::SetDataSource(DataSourceType dataType, int32_t &sourceId)
{
    (void)dataType;
    (void)sourceId;
    return MSERR_INVALID_OPERATION;
}

int32_t RecorderServer::SetMaxDuration(int32_t duration)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.maxDuration = duration;
    MaxDuration maxDuration(duration);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Configure(DUMMY_SOURCE_ID, maxDuration);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t RecorderServer::SetOutputFormat(OutputFormatType format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_INITIALIZED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.format = format;
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->SetOutputFormat(format);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    ret = result.Value();
    status_ = (ret == MSERR_OK ? REC_CONFIGURED : REC_INITIALIZED);
    BehaviorEventWrite(GetStatusDescription(status_), "Recorder");
    return ret;
}

int32_t RecorderServer::SetOutputFile(int32_t fd)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.url = fd;
    OutFd outFileFd(fd);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Configure(DUMMY_SOURCE_ID, outFileFd);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t RecorderServer::SetNextOutputFile(int32_t fd)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    NextOutFd nextFileFd(fd);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Configure(DUMMY_SOURCE_ID, nextFileFd);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t RecorderServer::SetMaxFileSize(int64_t size)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.maxFileSize = size;
    MaxFileSize maxFileSize(size);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Configure(DUMMY_SOURCE_ID, maxFileSize);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

void RecorderServer::SetLocation(float latitude, float longitude)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_ != REC_CONFIGURED) {
        return;
    }
    CHECK_AND_RETURN_LOG(recorderEngine_ != nullptr, "engine is nullptr");
    config_.latitude = latitude;
    config_.longitude = longitude;
    config_.withLocation = true;
    GeoLocation geoLocation(latitude, longitude);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Configure(DUMMY_SOURCE_ID, geoLocation);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_LOG(ret == MSERR_OK, "EnqueueTask failed");

    (void)task->GetResult();
    return;
}

void RecorderServer::SetOrientationHint(int32_t rotation)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(status_ == REC_CONFIGURED, "status_ error");
    CHECK_AND_RETURN_LOG(recorderEngine_ != nullptr, "engine is nullptr");
    config_.rotation = rotation;
    RotationAngle rotationAngle(rotation);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Configure(DUMMY_SOURCE_ID, rotationAngle);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_LOG(ret == MSERR_OK, "EnqueueTask failed");

    (void)task->GetResult();
    return;
}

int32_t RecorderServer::SetRecorderCallback(const std::shared_ptr<RecorderCallback> &callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_INITIALIZED && status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);

    {
        std::lock_guard<std::mutex> cbLock(cbMutex_);
        recorderCb_ = callback;
    }

    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    std::shared_ptr<IRecorderEngineObs> obs = shared_from_this();
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->SetObs(obs);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t RecorderServer::Prepare()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("RecorderServer::Prepare");
    if (status_ == REC_PREPARED) {
        MEDIA_LOGE("Can not repeat Prepare");
        return MSERR_INVALID_OPERATION;
    }
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_CONFIGURED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Prepare();
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    ret = result.Value();
    status_ = (ret == MSERR_OK ? REC_PREPARED : REC_ERROR);
    BehaviorEventWrite(GetStatusDescription(status_), "Recorder");
    return ret;
}

int32_t RecorderServer::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("RecorderServer::Start");
    if (status_ == REC_RECORDING) {
        MEDIA_LOGE("Can not repeat Start");
        return MSERR_INVALID_OPERATION;
    }
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_PREPARED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Start();
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    ret = result.Value();
    status_ = (ret == MSERR_OK ? REC_RECORDING : REC_ERROR);
    BehaviorEventWrite(GetStatusDescription(status_), "Recorder");
    return ret;
}

int32_t RecorderServer::Pause()
{
    MediaTrace trace("RecorderServer::Pause");
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_ == REC_PAUSED) {
        MEDIA_LOGE("Can not repeat Pause");
        return MSERR_INVALID_OPERATION;
    }
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_RECORDING, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Pause();
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    ret = result.Value();
    status_ = (ret == MSERR_OK ? REC_PAUSED : REC_ERROR);
    BehaviorEventWrite(GetStatusDescription(status_), "Recorder");
    return ret;
}

int32_t RecorderServer::Resume()
{
    MediaTrace trace("RecorderServer::Resume");
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_ == REC_RECORDING) {
        MEDIA_LOGE("Can not repeat Resume");
        return MSERR_INVALID_OPERATION;
    }
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_RECORDING && status_ != REC_PAUSED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Resume();
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    ret = result.Value();
    status_ = (ret == MSERR_OK ? REC_RECORDING : REC_ERROR);
    BehaviorEventWrite(GetStatusDescription(status_), "Recorder");
    return ret;
}

int32_t RecorderServer::Stop(bool block)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("RecorderServer::Stop");
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_RECORDING && status_ != REC_PAUSED, MSERR_INVALID_OPERATION);

    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Stop(block);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    ret = result.Value();
    status_ = (ret == MSERR_OK ? REC_INITIALIZED : REC_ERROR);
    BehaviorEventWrite(GetStatusDescription(status_), "Recorder");
    return ret;
}

int32_t RecorderServer::Reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("RecorderServer::Reset");
    CHECK_AND_RETURN_RET_LOG(recorderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return recorderEngine_->Reset();
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    ret = result.Value();
    status_ = (ret == MSERR_OK ? REC_INITIALIZED : REC_ERROR);
    BehaviorEventWrite(GetStatusDescription(status_), "Recorder");
    return ret;
}

int32_t RecorderServer::Release()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto task = std::make_shared<TaskHandler<void>>([&, this] {
            recorderEngine_ = nullptr;
        });
        (void)taskQue_.EnqueueTask(task);
        (void)task->GetResult();
    }
    return MSERR_OK;
}

int32_t RecorderServer::SetFileSplitDuration(FileSplitType type, int64_t timestamp, uint32_t duration)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_STATUS_FAILED_AND_LOGE_RET(status_ != REC_RECORDING && status_ != REC_PAUSED, MSERR_INVALID_OPERATION);

    (void)type;
    (void)timestamp;
    (void)duration;
    return MSERR_OK;
}

int32_t RecorderServer::SetParameter(int32_t sourceId, const Format &format)
{
    (void)sourceId;
    (void)format;
    return MSERR_OK;
}

int32_t RecorderServer::DumpInfo(int32_t fd)
{
    std::string dumpString;
    dumpString += "In RecorderServer::DumpInfo\n";
    dumpString += "RecorderServer current state is: " + std::to_string(status_) + "\n";
    if (lastErrMsg_.size() != 0) {
        dumpString += "RecorderServer last error is: " + lastErrMsg_ + "\n";
    }
    dumpString += "RecorderServer videoSource is: " + std::to_string(config_.videoSource) + "\n";
    dumpString += "RecorderServer audioSource is: " + std::to_string(config_.audioSource) + "\n";
    dumpString += "RecorderServer videoCodec is: " + std::to_string(config_.videoCodec) + "\n";
    dumpString += "RecorderServer audioCodec is: " + std::to_string(config_.audioCodec) + "\n";
    dumpString += "RecorderServer width is: " + std::to_string(config_.width) + "\n";
    dumpString += "RecorderServer height is: " + std::to_string(config_.height) + "\n";
    dumpString += "RecorderServer frameRate is: " + std::to_string(config_.frameRate) + "\n";
    dumpString += "RecorderServer bitRate is: " + std::to_string(config_.bitRate) + "\n";
    dumpString += "RecorderServer captureRate is: " + std::to_string(config_.captureRate) + "\n";
    dumpString += "RecorderServer audioSampleRate is: " + std::to_string(config_.audioSampleRate) + "\n";
    dumpString += "RecorderServer audioChannel is: " + std::to_string(config_.audioChannel) + "\n";
    dumpString += "RecorderServer audioBitRate is: " + std::to_string(config_.audioBitRate) + "\n";
    dumpString += "RecorderServer isHdr is: " + std::to_string(config_.isHdr) + "\n";
    dumpString += "RecorderServer maxDuration is: " + std::to_string(config_.maxDuration) + "\n";
    dumpString += "RecorderServer format is: " + std::to_string(config_.format) + "\n";
    dumpString += "RecorderServer maxFileSize is: " + std::to_string(config_.maxFileSize) + "\n";
    write(fd, dumpString.c_str(), dumpString.size());

    return MSERR_OK;
}

int32_t RecorderServer::GetAVRecorderConfig(ConfigMap &configMap)
{
    std::lock_guard<std::mutex> lock(mutex_);
    configMap["audioBitrate"] = config_.audioBitRate;
    configMap["audioChannels"] = config_.audioChannel;
    configMap["audioCodec"] = static_cast<int32_t>(config_.audioCodec);
    configMap["audioSampleRate"] = config_.audioSampleRate;
    configMap["fileFormat"] = static_cast<int32_t>(config_.format);
    configMap["videoBitrate"] = config_.bitRate;
    configMap["videoCodec"] = static_cast<int32_t>(config_.videoCodec);
    configMap["videoFrameHeight"] = config_.height;
    configMap["videoFrameWidth"] = config_.width;
    configMap["videoFrameRate"] = config_.frameRate;
    configMap["audioSourceType"] = static_cast<int32_t>(config_.audioSource);
    configMap["videoSourceType"] = static_cast<int32_t>(config_.videoSource);
    configMap["url"] = config_.url;
    configMap["rotation"] = config_.rotation;
    configMap["withVideo"] = config_.withVideo;
    configMap["withAudio"] = config_.withAudio;
    configMap["withLocation"] = config_.withLocation;
    return MSERR_OK;
}

int32_t RecorderServer::GetLocation(Location &location)
{
    std::lock_guard<std::mutex> lock(mutex_);
    location.latitude = config_.latitude;
    location.longitude = config_.longitude;
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
