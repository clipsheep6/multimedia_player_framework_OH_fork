/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "transcoder_server.h"
#include "map"
#include "media_log.h"
#include "media_errors.h"
#include "engine_factory_repo.h"
#include "param_wrapper.h"
#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "media_dfx.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "TransCoderServer"};
    const std::map<OHOS::Media::TransCoderServer::RecStatus, std::string> TRANSCODER_STATE_MAP = {
        {OHOS::Media::TransCoderServer::REC_INITIALIZED, "initialized"},
        {OHOS::Media::TransCoderServer::REC_CONFIGURED, "configured"},
        {OHOS::Media::TransCoderServer::REC_PREPARED, "prepared"},
        {OHOS::Media::TransCoderServer::REC_TRANSCODERING, "transcordring"},
        {OHOS::Media::TransCoderServer::REC_PAUSED, "paused"},
        {OHOS::Media::TransCoderServer::REC_ERROR, "error"},
    };
}

namespace OHOS {
namespace Media {
const std::string START_TAG = "TransCoderCreate->Start";
const std::string STOP_TAG = "TransCoderStop->Destroy";

std::shared_ptr<ITransCoderService> TransCoderServer::Create()
{
    std::shared_ptr<TransCoderServer> server = std::make_shared<TransCoderServer>();
    int32_t ret = server->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to init TransCoderServer");
    return server;
}

TransCoderServer::TransCoderServer()
    : taskQue_("TransCoderServer")
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " create", FAKE_POINTER(this));
    taskQue_.Start();
}

TransCoderServer::~TransCoderServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto task = std::make_shared<TaskHandler<void>>([&, this] {
            transCoderEngine_ = nullptr;
        });
        (void)taskQue_.EnqueueTask(task);
        (void)task->GetResult();
        taskQue_.Stop();
    }
}

int32_t TransCoderServer::Init()
{
    MediaTrace trace("TransCoderServer::Init");
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    uint64_t fullTokenId = IPCSkeleton::GetCallingFullTokenID();
    int32_t appUid = IPCSkeleton::GetCallingUid();
    int32_t appPid = IPCSkeleton::GetCallingPid();

    auto task = std::make_shared<TaskHandler<MediaServiceErrCode>>([&, this] {
        auto engineFactory = EngineFactoryRepo::Instance().GetEngineFactory(
            IEngineFactory::Scene::SCENE_TRANSCODER, appUid);
        CHECK_AND_RETURN_RET_LOG(engineFactory != nullptr, MSERR_CREATE_REC_ENGINE_FAILED,
            "failed to get factory");
        transCoderEngine_ = engineFactory->CreateTransCoderEngine(appUid, appPid, tokenId, fullTokenId);
        CHECK_AND_RETURN_RET_LOG(transCoderEngine_ != nullptr, MSERR_CREATE_REC_ENGINE_FAILED,
            "failed to create transCoder engine");
        return MSERR_OK;
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.Value() == MSERR_OK, result.Value(), "Result failed");

    status_ = REC_INITIALIZED;
    BehaviorEventWrite(GetStatusDescription(status_), "TransCoder");
    return MSERR_OK;
}

const std::string& TransCoderServer::GetStatusDescription(OHOS::Media::TransCoderServer::RecStatus status)
{
    static const std::string ILLEGAL_STATE = "PLAYER_STATUS_ILLEGAL";
    CHECK_AND_RETURN_RET(status >= OHOS::Media::TransCoderServer::REC_INITIALIZED &&
        status <= OHOS::Media::TransCoderServer::REC_ERROR, ILLEGAL_STATE);

    return TRANSCODER_STATE_MAP.find(status)->second;
}

void TransCoderServer::OnError(TransCoderErrorType errorType, int32_t errorCode)
{
    std::lock_guard<std::mutex> lock(cbMutex_);
    lastErrMsg_ = MSErrorToExtErrorString(static_cast<MediaServiceErrCode>(errorCode));
    FaultEventWrite(lastErrMsg_, "TransCoder");
    CHECK_AND_RETURN(transCoderCb_ != nullptr);
    transCoderCb_->OnError(static_cast<TransCoderErrorType>(errorType), errorCode);
}

void TransCoderServer::OnInfo(TransCoderOnInfoType type, int32_t extra)
{
    std::lock_guard<std::mutex> lock(cbMutex_);
    if (transCoderCb_ != nullptr) {
        transCoderCb_->OnInfo(type, extra);
    }
}

int32_t TransCoderServer::SetVideoEncoder(VideoCodecFormat encoder)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == REC_CONFIGURED, MSERR_INVALID_OPERATION,
        "invalid status, current status is %{public}s", GetStatusDescription(status_).c_str());
    CHECK_AND_RETURN_RET_LOG(transCoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.videoCodec = encoder;
    VideoEnc vidEnc(encoder);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return transCoderEngine_->Configure(vidEnc);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t TransCoderServer::SetVideoSize(int32_t width, int32_t height)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == REC_CONFIGURED, MSERR_INVALID_OPERATION,
        "invalid status, current status is %{public}s", GetStatusDescription(status_).c_str());
    CHECK_AND_RETURN_RET_LOG(transCoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.width = width;
    config_.height = height;
    VideoRectangle vidSize(width, height);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return transCoderEngine_->Configure(vidSize);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t TransCoderServer::SetVideoEncodingBitRate(int32_t rate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == REC_CONFIGURED, MSERR_INVALID_OPERATION,
        "invalid status, current status is %{public}s", GetStatusDescription(status_).c_str());
    CHECK_AND_RETURN_RET_LOG(transCoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.videoBitRate = rate;
    VideoBitRate vidBitRate(rate);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return transCoderEngine_->Configure(vidBitRate);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t TransCoderServer::SetAudioEncoder(AudioCodecFormat encoder)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == REC_CONFIGURED, MSERR_INVALID_OPERATION,
        "invalid status, current status is %{public}s", GetStatusDescription(status_).c_str());
    CHECK_AND_RETURN_RET_LOG(transCoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.audioCodec = encoder;
    AudioEnc audEnc(encoder);
    MEDIA_LOGD("set audio encoder encoder:%{public}d", encoder);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return transCoderEngine_->Configure(audEnc);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t TransCoderServer::SetAudioEncodingBitRate(int32_t bitRate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == REC_CONFIGURED, MSERR_INVALID_OPERATION,
        "invalid status, current status is %{public}s", GetStatusDescription(status_).c_str());
    CHECK_AND_RETURN_RET_LOG(transCoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.audioBitRate = bitRate;
    AudioBitRate audBitRate(bitRate);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return transCoderEngine_->Configure(audBitRate);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t TransCoderServer::SetOutputFormat(OutputFormatType format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == REC_INITIALIZED, MSERR_INVALID_OPERATION,
        "invalid status, current status is %{public}s", GetStatusDescription(status_).c_str());
    CHECK_AND_RETURN_RET_LOG(transCoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.format = format;
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return transCoderEngine_->SetOutputFormat(format);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    ret = result.Value();
    status_ = (ret == MSERR_OK ? REC_CONFIGURED : REC_INITIALIZED);
    BehaviorEventWrite(GetStatusDescription(status_), "TransCoder");
    return ret;
}

int32_t TransCoderServer::SetInputFile(std::string url)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == REC_INITIALIZED, MSERR_INVALID_OPERATION,
        "invalid status, current status is %{public}s", GetStatusDescription(status_).c_str());
    CHECK_AND_RETURN_RET_LOG(transCoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.srcUrl = url;
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return transCoderEngine_->SetInputFile(url);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t TransCoderServer::SetInputFile(int32_t fd, int64_t offset, int64_t size)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == REC_INITIALIZED, MSERR_INVALID_OPERATION,
        "invalid status, current status is %{public}s", GetStatusDescription(status_).c_str());
    CHECK_AND_RETURN_RET_LOG(transCoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.srcFd = fd;
    config_.srcFdOffset = offset;
    config_.srcFdSize = size;
    uriHelper_ = std::make_unique<UriHelper>(fd, offset, size);
    CHECK_AND_RETURN_RET_LOG(uriHelper_->AccessCheck(UriHelper::URI_READ),
        MSERR_INVALID_VAL, "Failed to read the fd");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return transCoderEngine_->SetInputFile(uriHelper_->FormattedUri());
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t TransCoderServer::SetOutputFile(int32_t fd)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == REC_INITIALIZED, MSERR_INVALID_OPERATION,
        "invalid status, current status is %{public}s", GetStatusDescription(status_).c_str());
    CHECK_AND_RETURN_RET_LOG(transCoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    config_.dstUrl = fd;
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return transCoderEngine_->SetOutputFile(fd);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t TransCoderServer::SetTransCoderCallback(const std::shared_ptr<TransCoderCallback> &callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == REC_INITIALIZED || status_ == REC_CONFIGURED, MSERR_INVALID_OPERATION,
        "invalid status, current status is %{public}s", GetStatusDescription(status_).c_str());
    {
        std::lock_guard<std::mutex> cbLock(cbMutex_);
        transCoderCb_ = callback;
    }

    CHECK_AND_RETURN_RET_LOG(transCoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    std::shared_ptr<ITransCoderEngineObs> obs = shared_from_this();
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return transCoderEngine_->SetObs(obs);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    return result.Value();
}

int32_t TransCoderServer::Prepare()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("TransCoderServer::Prepare");
    if (status_ == REC_PREPARED) {
        MEDIA_LOGE("Can not repeat Prepare");
        return MSERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(status_ == REC_CONFIGURED, MSERR_INVALID_OPERATION,
        "invalid status, current status is %{public}s", GetStatusDescription(status_).c_str());
    CHECK_AND_RETURN_RET_LOG(transCoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return transCoderEngine_->Prepare();
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    ret = result.Value();
    status_ = (ret == MSERR_OK ? REC_PREPARED : REC_ERROR);
    BehaviorEventWrite(GetStatusDescription(status_), "TransCoder");
    return ret;
}

int32_t TransCoderServer::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("TransCoderServer::Start");
    if (status_ == REC_TRANSCODERING) {
        MEDIA_LOGE("Can not repeat Start");
        return MSERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(status_ == REC_PREPARED, MSERR_INVALID_OPERATION,
        "invalid status, current status is %{public}s", GetStatusDescription(status_).c_str());
    CHECK_AND_RETURN_RET_LOG(transCoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return transCoderEngine_->Start();
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    ret = result.Value();
    status_ = (ret == MSERR_OK ? REC_TRANSCODERING : REC_ERROR);
    BehaviorEventWrite(GetStatusDescription(status_), "TransCoder");
    return ret;
}

int32_t TransCoderServer::Pause()
{
    MediaTrace trace("TransCoderServer::Pause");
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_ == REC_PAUSED) {
        MEDIA_LOGE("Can not repeat Pause");
        return MSERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(status_ == REC_TRANSCODERING, MSERR_INVALID_OPERATION,
        "invalid status, current status is %{public}s", GetStatusDescription(status_).c_str());
    CHECK_AND_RETURN_RET_LOG(transCoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return transCoderEngine_->Pause();
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    ret = result.Value();
    status_ = (ret == MSERR_OK ? REC_PAUSED : REC_ERROR);
    BehaviorEventWrite(GetStatusDescription(status_), "TransCoder");
    return ret;
}

int32_t TransCoderServer::Resume()
{
    MediaTrace trace("TransCoderServer::Resume");
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_ == REC_TRANSCODERING) {
        MEDIA_LOGE("Can not repeat Resume");
        return MSERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(status_ == REC_TRANSCODERING || status_ == REC_PAUSED, MSERR_INVALID_OPERATION,
        "invalid status, current status is %{public}s", GetStatusDescription(status_).c_str());
    CHECK_AND_RETURN_RET_LOG(transCoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return transCoderEngine_->Resume();
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    ret = result.Value();
    status_ = (ret == MSERR_OK ? REC_TRANSCODERING : REC_ERROR);
    BehaviorEventWrite(GetStatusDescription(status_), "TransCoder");
    return ret;
}

int32_t TransCoderServer::Cancel()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("TransCoderServer::Cancel");
    CHECK_AND_RETURN_RET_LOG(transCoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        return transCoderEngine_->Cancel();
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    ret = result.Value();
    status_ = (ret == MSERR_OK ? REC_INITIALIZED : REC_ERROR);
    BehaviorEventWrite(GetStatusDescription(status_), "TransCoder");
    return ret;
}

int32_t TransCoderServer::Release()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto task = std::make_shared<TaskHandler<void>>([&, this] {
            transCoderEngine_ = nullptr;
        });
        (void)taskQue_.EnqueueTask(task);
        (void)task->GetResult();
    }
    return MSERR_OK;
}

int32_t TransCoderServer::DumpInfo(int32_t fd)
{
    std::string dumpString;
    dumpString += "In TransCoderServer::DumpInfo\n";
    dumpString += "TransCoderServer current state is: " + std::to_string(status_) + "\n";
    if (lastErrMsg_.size() != 0) {
        dumpString += "TransCoderServer last error is: " + lastErrMsg_ + "\n";
    }
    dumpString += "TransCoderServer videoCodec is: " + std::to_string(config_.videoCodec) + "\n";
    dumpString += "TransCoderServer audioCodec is: " + std::to_string(config_.audioCodec) + "\n";
    dumpString += "TransCoderServer width is: " + std::to_string(config_.width) + "\n";
    dumpString += "TransCoderServer height is: " + std::to_string(config_.height) + "\n";
    dumpString += "TransCoderServer bitRate is: " + std::to_string(config_.videoBitRate) + "\n";
    dumpString += "TransCoderServer audioBitRate is: " + std::to_string(config_.audioBitRate) + "\n";
    dumpString += "TransCoderServer format is: " + std::to_string(config_.format) + "\n";
    write(fd, dumpString.c_str(), dumpString.size());

    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
