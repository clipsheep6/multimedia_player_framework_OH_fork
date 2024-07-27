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

#include "recorder_service_stub.h"
#include <unistd.h>
#include "recorder_listener_proxy.h"
#include "media_server_manager.h"
#include "media_log.h"
#include "media_errors.h"
#include "ipc_skeleton.h"
#include "media_permission.h"
#include "accesstoken_kit.h"
#include "media_dfx.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_RECORDER, "RecorderServiceStub"};
}

namespace OHOS {
namespace Media {
sptr<RecorderServiceStub> RecorderServiceStub::Create()
{
    sptr<RecorderServiceStub> recorderStub = new(std::nothrow) RecorderServiceStub();
    CHECK_AND_RETURN_RET_LOG(recorderStub != nullptr, nullptr, "failed to new RecorderServiceStub");

    int32_t ret = recorderStub->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to recorder stub init");
    StatisticEventWriteBundleName("create", "RecorderServiceStub");
    return recorderStub;
}

RecorderServiceStub::RecorderServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

RecorderServiceStub::~RecorderServiceStub()
{
    (void)CancellationMonitor(pid_);
    needAudioPermissionCheck = false;
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t RecorderServiceStub::Init()
{
    recorderServer_ = RecorderServer::Create();
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "failed to create RecorderServer");
    FillRecFuncPart1();
    FillRecFuncPart2();
    pid_ = IPCSkeleton::GetCallingPid();
    (void)RegisterMonitor(pid_);
    return MSERR_OK;
}

void RecorderServiceStub::FillRecFuncPart1()
{
    recFuncs_[SET_LISTENER_OBJ] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetListenerObject(data, reply); };
    recFuncs_[SET_VIDEO_SOURCE] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetVideoSource(data, reply); };
    recFuncs_[SET_VIDEO_ENCODER] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetVideoEncoder(data, reply); };
    recFuncs_[SET_VIDEO_SIZE] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetVideoSize(data, reply); };
    recFuncs_[SET_VIDEO_FARAME_RATE] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetVideoFrameRate(data, reply); };
    recFuncs_[SET_VIDEO_ENCODING_BIT_RATE] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetVideoEncodingBitRate(data, reply); };
    recFuncs_[GET_SURFACE] =
        [this](MessageParcel &data, MessageParcel &reply) { return GetSurface(data, reply); };
    recFuncs_[SET_AUDIO_SOURCE] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetAudioSource(data, reply); };
    recFuncs_[SET_AUDIO_ENCODER] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetAudioEncoder(data, reply); };
    recFuncs_[SET_AUDIO_SAMPLE_RATE] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetAudioSampleRate(data, reply); };
    recFuncs_[SET_AUDIO_CHANNELS] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetAudioChannels(data, reply); };
    recFuncs_[SET_AUDIO_ENCODING_BIT_RATE] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetAudioEncodingBitRate(data, reply); };
    recFuncs_[SET_DATA_SOURCE] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetDataSource(data, reply); };
    recFuncs_[SET_MAX_DURATION] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetMaxDuration(data, reply); };
    recFuncs_[SET_OUTPUT_FORMAT] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetOutputFormat(data, reply); };
    recFuncs_[SET_OUTPUT_FILE] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetOutputFile(data, reply); };
    recFuncs_[SET_LOCATION] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetLocation(data, reply); };
    recFuncs_[SET_ORIENTATION_HINT] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetOrientationHint(data, reply); };
    recFuncs_[SET_USER_CUSTOM_INFO] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetUserCustomInfo(data, reply); };
    recFuncs_[SET_GENRE] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetGenre(data, reply); };
    recFuncs_[PREPARE] =
        [this](MessageParcel &data, MessageParcel &reply) { return Prepare(data, reply); };
}

void RecorderServiceStub::FillRecFuncPart2()
{
    recFuncs_[START] =
        [this](MessageParcel &data, MessageParcel &reply) { return Start(data, reply); };
    recFuncs_[PAUSE] =
        [this](MessageParcel &data, MessageParcel &reply) { return Pause(data, reply); };
    recFuncs_[RESUME] =
        [this](MessageParcel &data, MessageParcel &reply) { return Resume(data, reply); };
    recFuncs_[STOP] =
        [this](MessageParcel &data, MessageParcel &reply) { return Stop(data, reply); };
    recFuncs_[RESET] =
        [this](MessageParcel &data, MessageParcel &reply) { return Reset(data, reply); };
    recFuncs_[RELEASE] =
        [this](MessageParcel &data, MessageParcel &reply) { return Release(data, reply); };
    recFuncs_[DESTROY] =
        [this](MessageParcel &data, MessageParcel &reply) { return DestroyStub(data, reply); };
    recFuncs_[GET_AV_RECORDER_CONFIG] =
        [this](MessageParcel &data, MessageParcel &reply) { return GetAVRecorderConfig(data, reply); };
    recFuncs_[GET_LOCATION] =
        [this](MessageParcel &data, MessageParcel &reply) { return GetLocation(data, reply); };
    recFuncs_[SET_VIDEO_IS_HDR] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetVideoIsHdr(data, reply); };
    recFuncs_[SET_VIDEO_ENABLE_TEMPORAL_SCALE] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetVideoEnableTemporalScale(data, reply); };
    recFuncs_[GET_AUDIO_CAPTURER_CHANGE_INFO] =
        [this](MessageParcel &data, MessageParcel &reply) { return GetCurrentCapturerChangeInfo(data, reply); };
    recFuncs_[GET_AVAILABLE_ENCODER] =
        [this](MessageParcel &data, MessageParcel &reply) { return GetAvailableEncoder(data, reply); };
    recFuncs_[GET_MAX_AMPLITUDE] =
        [this](MessageParcel &data, MessageParcel &reply) { return GetMaxAmplitude(data, reply); };
    recFuncs_[IS_WATERMARK_SUPPORTED] =
        [this](MessageParcel &data, MessageParcel &reply) { return IsWatermarkSupported(data, reply); };
    recFuncs_[SET_WATERMARK] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetWatermark(data, reply); };
    recFuncs_[SET_META_CONFIGS] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetMetaConfigs(data, reply); };
    recFuncs_[SET_META_SOURCE] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetMetaSource(data, reply); };
    recFuncs_[SET_META_MIME_TYPE] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetMetaMimeType(data, reply); };
    recFuncs_[SET_META_TIMED_KEY] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetMetaTimedKey(data, reply); };
    recFuncs_[SET_META_TRACK_SRC_MIME_TYPE] =
        [this](MessageParcel &data, MessageParcel &reply) { return SetMetaSourceTrackMime(data, reply); };
    recFuncs_[GET_META_SURFACE] =
        [this](MessageParcel &data, MessageParcel &reply) { return GetMetaSurface(data, reply); };
}

int32_t RecorderServiceStub::DestroyStub()
{
    recorderServer_ = nullptr;
    MediaServerManager::GetInstance().DestroyStubObject(MediaServerManager::RECORDER, AsObject());
    return MSERR_OK;
}

int RecorderServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    MEDIA_LOGI("Stub: OnRemoteRequest of code: %{public}d is received", code);
    int32_t permissionResult;

    auto remoteDescriptor = data.ReadInterfaceToken();
    CHECK_AND_RETURN_RET_LOG(RecorderServiceStub::GetDescriptor() == remoteDescriptor,
        MSERR_INVALID_OPERATION, "Invalid descriptor");

    {
        if (code == SET_AUDIO_SOURCE) {
            std::lock_guard<std::mutex> lock(stmutex_);
            int32_t type = data.ReadInt32();
            CHECK_AND_RETURN_RET_LOG(audioSourceType_ == AUDIO_SOURCE_INVALID,
                MSERR_EXT_API9_OPERATE_NOT_PERMIT, "unsupport parameter or repeated operation");
            audioSourceType_ = static_cast<AudioSourceType>(type);
        }
    }

    if (AUDIO_REQUEST.count(code) != 0) {
        permissionResult = CheckPermission();
        needAudioPermissionCheck = true;
    } else if (COMMON_REQUEST.count(code) != 0) {
        if (needAudioPermissionCheck) {
            permissionResult = CheckPermission();
        } else {
            // none audio request no need to check permission in recorder server
            permissionResult = Security::AccessToken::PERMISSION_GRANTED;
        }
    } else {
        // none audio request no need to check permission in recorder server
        permissionResult = Security::AccessToken::PERMISSION_GRANTED;
    }
    CHECK_AND_RETURN_RET_LOG(permissionResult == Security::AccessToken::PERMISSION_GRANTED,
        MSERR_EXT_API9_PERMISSION_DENIED, "user do not have the correct permission");

    auto itFunc = recFuncs_.find(code);
    if (itFunc != recFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            std::lock_guard<std::mutex> lock(mutex_);
            (void)IpcRecovery(false);
            int32_t ret = memberFunc(data, reply);
            if (ret != MSERR_OK) {
                MEDIA_LOGE("calling memberFunc is failed.");
            }
            if (AUDIO_REQUEST.count(code) != 0 && reply.ReadInt32() != MSERR_OK) {
                MEDIA_LOGE("audio memberFunc failed, reset permission check.");
            }
            return MSERR_OK;
        }
    }
    MEDIA_LOGW("RecorderServiceStub: no member func supporting, applying default process");

    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t RecorderServiceStub::SetListenerObject(const sptr<IRemoteObject> &object)
{
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_NO_MEMORY, "set listener object is nullptr");

    sptr<IStandardRecorderListener> listener = iface_cast<IStandardRecorderListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, MSERR_NO_MEMORY, "failed to convert IStandardRecorderListener");

    std::shared_ptr<RecorderCallback> callback = std::make_shared<RecorderListenerCallback>(listener);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_NO_MEMORY, "failed to new RecorderListenerCallback");

    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    (void)recorderServer_->SetRecorderCallback(callback);
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetVideoSource(VideoSourceType source, int32_t &sourceId)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetVideoSource(source, sourceId);
}

int32_t RecorderServiceStub::SetVideoEncoder(int32_t sourceId, VideoCodecFormat encoder)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetVideoEncoder(sourceId, encoder);
}

int32_t RecorderServiceStub::SetVideoSize(int32_t sourceId, int32_t width, int32_t height)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetVideoSize(sourceId, width, height);
}

int32_t RecorderServiceStub::SetVideoFrameRate(int32_t sourceId, int32_t frameRate)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetVideoFrameRate(sourceId, frameRate);
}

int32_t RecorderServiceStub::SetVideoEncodingBitRate(int32_t sourceId, int32_t rate)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetVideoEncodingBitRate(sourceId, rate);
}

int32_t RecorderServiceStub::SetVideoIsHdr(int32_t sourceId, bool isHdr)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetVideoIsHdr(sourceId, isHdr);
}

int32_t RecorderServiceStub::SetVideoEnableTemporalScale(int32_t sourceId, bool enableTemporalScale)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetVideoEnableTemporalScale(sourceId, enableTemporalScale);
}

int32_t RecorderServiceStub::SetMetaConfigs(int32_t sourceId)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetMetaConfigs(sourceId);
}

int32_t RecorderServiceStub::SetMetaSource(MetaSourceType source, int32_t &sourceId)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetMetaSource(source, sourceId);
}

int32_t RecorderServiceStub::SetMetaMimeType(int32_t sourceId, const std::string_view &type)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetMetaMimeType(sourceId, type);
}

int32_t RecorderServiceStub::SetMetaTimedKey(int32_t sourceId, const std::string_view &timedKey)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetMetaTimedKey(sourceId, timedKey);
}

int32_t RecorderServiceStub::SetMetaSourceTrackMime(int32_t sourceId, const std::string_view &srcTrackMime)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetMetaSourceTrackMime(sourceId, srcTrackMime);
}

sptr<OHOS::Surface> RecorderServiceStub::GetSurface(int32_t sourceId)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, nullptr, "recorder server is nullptr");
    return recorderServer_->GetSurface(sourceId);
}

sptr<OHOS::Surface> RecorderServiceStub::GetMetaSurface(int32_t sourceId)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, nullptr, "recorder server is nullptr");
    return recorderServer_->GetMetaSurface(sourceId);
}

int32_t RecorderServiceStub::SetAudioSource(AudioSourceType source, int32_t &sourceId)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetAudioSource(source, sourceId);
}

int32_t RecorderServiceStub::SetAudioEncoder(int32_t sourceId, AudioCodecFormat encoder)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetAudioEncoder(sourceId, encoder);
}

int32_t RecorderServiceStub::SetAudioSampleRate(int32_t sourceId, int32_t rate)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetAudioSampleRate(sourceId, rate);
}

int32_t RecorderServiceStub::SetAudioChannels(int32_t sourceId, int32_t num)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetAudioChannels(sourceId, num);
}

int32_t RecorderServiceStub::SetAudioEncodingBitRate(int32_t sourceId, int32_t bitRate)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetAudioEncodingBitRate(sourceId, bitRate);
}

int32_t RecorderServiceStub::SetDataSource(DataSourceType dataType, int32_t &sourceId)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetDataSource(dataType, sourceId);
}

int32_t RecorderServiceStub::SetUserCustomInfo(Meta &userCustomInfo)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetUserCustomInfo(userCustomInfo);
}

int32_t RecorderServiceStub::SetGenre(std::string &genre)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetGenre(genre);
}

int32_t RecorderServiceStub::SetMaxDuration(int32_t duration)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetMaxDuration(duration);
}

int32_t RecorderServiceStub::SetOutputFormat(OutputFormatType format)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetOutputFormat(format);
}

int32_t RecorderServiceStub::SetOutputFile(int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetOutputFile(fd);
}

int32_t RecorderServiceStub::SetLocation(float latitude, float longitude)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    recorderServer_->SetLocation(latitude, longitude);
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetOrientationHint(int32_t rotation)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    recorderServer_->SetOrientationHint(rotation);
    return MSERR_OK;
}

int32_t RecorderServiceStub::Prepare()
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->Prepare();
}

int32_t RecorderServiceStub::Start()
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->Start();
}

int32_t RecorderServiceStub::Pause()
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->Pause();
}

int32_t RecorderServiceStub::Resume()
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->Resume();
}

int32_t RecorderServiceStub::Stop(bool block)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->Stop(block);
}

int32_t RecorderServiceStub::Reset()
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->Reset();
}

int32_t RecorderServiceStub::Release()
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->Release();
}

int32_t RecorderServiceStub::DumpInfo(int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return std::static_pointer_cast<RecorderServer>(recorderServer_)->DumpInfo(fd);
}

int32_t RecorderServiceStub::GetAVRecorderConfig(ConfigMap &configMap)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->GetAVRecorderConfig(configMap);
}

int32_t RecorderServiceStub::GetLocation(Location &location)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->GetLocation(location);
}

int32_t RecorderServiceStub::GetCurrentCapturerChangeInfo(AudioRecorderChangeInfo &changeInfo)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->GetCurrentCapturerChangeInfo(changeInfo);
}

int32_t RecorderServiceStub::GetAvailableEncoder(std::vector<EncoderCapabilityData> &encoderInfo)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->GetAvailableEncoder(encoderInfo);
}

int32_t RecorderServiceStub::GetMaxAmplitude()
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->GetMaxAmplitude();
}

int32_t RecorderServiceStub::IsWatermarkSupported(bool &isWatermarkSupported)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->IsWatermarkSupported(isWatermarkSupported);
}

int32_t RecorderServiceStub::SetWatermark(std::shared_ptr<AVBuffer> &waterMarkBuffer)
{
    CHECK_AND_RETURN_RET_LOG(recorderServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    return recorderServer_->SetWatermark(waterMarkBuffer);
}

int32_t RecorderServiceStub::DoIpcAbnormality()
{
    MEDIA_LOGI("Enter DoIpcAbnormality.");
    SetIpcAlarmedFlag();
    return MSERR_OK;
}

int32_t RecorderServiceStub::DoIpcRecovery(bool fromMonitor)
{
    MEDIA_LOGI("Enter DoIpcRecovery %{public}d.", fromMonitor);
    UnSetIpcAlarmedFlag();
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetListenerObject(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    reply.WriteInt32(SetListenerObject(object));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetVideoSource(MessageParcel &data, MessageParcel &reply)
{
    int32_t source = data.ReadInt32();
    VideoSourceType sourceType = static_cast<VideoSourceType>(source);
    int32_t sourceId = 0;
    int32_t ret = SetVideoSource(sourceType, sourceId);
    reply.WriteInt32(sourceId);
    reply.WriteInt32(ret);
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetVideoEncoder(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t encoder = data.ReadInt32();
    VideoCodecFormat codecFormat = static_cast<VideoCodecFormat>(encoder);
    reply.WriteInt32(SetVideoEncoder(sourceId, codecFormat));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetVideoSize(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t width = data.ReadInt32();
    int32_t height = data.ReadInt32();
    reply.WriteInt32(SetVideoSize(sourceId, width, height));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetVideoFrameRate(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t frameRate = data.ReadInt32();
    reply.WriteInt32(SetVideoFrameRate(sourceId, frameRate));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetVideoEncodingBitRate(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t rate = data.ReadInt32();
    reply.WriteInt32(SetVideoEncodingBitRate(sourceId, rate));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetVideoIsHdr(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    bool isHdr = data.ReadBool();
    reply.WriteInt32(SetVideoIsHdr(sourceId, isHdr));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetVideoEnableTemporalScale(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    bool enableTemporalScale = data.ReadBool();
    reply.WriteInt32(SetVideoEnableTemporalScale(sourceId, enableTemporalScale));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetMetaConfigs(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t ret = SetMetaConfigs(sourceId);
    reply.WriteInt32(ret);
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetMetaSource(MessageParcel &data, MessageParcel &reply)
{
    int32_t source = data.ReadInt32();
    MetaSourceType sourceType = static_cast<MetaSourceType>(source);
    int32_t sourceId = 0;
    int32_t ret = SetMetaSource(sourceType, sourceId);
    reply.WriteInt32(sourceId);
    reply.WriteInt32(ret);
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetMetaMimeType(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    std::string_view mimetype(data.ReadCString());
    reply.WriteInt32(SetMetaMimeType(sourceId, mimetype));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetMetaTimedKey(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    std::string_view timedKey(data.ReadCString());
    reply.WriteInt32(SetMetaTimedKey(sourceId, timedKey));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetMetaSourceTrackMime(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    std::string_view srcTrackMime(data.ReadCString());
    reply.WriteInt32(SetMetaSourceTrackMime(sourceId, srcTrackMime));
    return MSERR_OK;
}

int32_t RecorderServiceStub::GetSurface(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    sptr<OHOS::Surface> surface = GetSurface(sourceId);
    if (surface != nullptr && surface->GetProducer() != nullptr) {
        sptr<IRemoteObject> object = surface->GetProducer()->AsObject();
        (void)reply.WriteRemoteObject(object);
    }
    return MSERR_OK;
}

int32_t RecorderServiceStub::GetMetaSurface(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    sptr<OHOS::Surface> surface = GetMetaSurface(sourceId);
    if (surface != nullptr && surface->GetProducer() != nullptr) {
        sptr<IRemoteObject> object = surface->GetProducer()->AsObject();
        (void)reply.WriteRemoteObject(object);
    }
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetAudioSource(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = 0;
    int32_t ret = SetAudioSource(audioSourceType_, sourceId);
    reply.WriteInt32(sourceId);
    reply.WriteInt32(ret);
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetAudioEncoder(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t format = data.ReadInt32();
    AudioCodecFormat encoderFormat = static_cast<AudioCodecFormat>(format);
    reply.WriteInt32(SetAudioEncoder(sourceId, encoderFormat));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetAudioSampleRate(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t rate = data.ReadInt32();
    reply.WriteInt32(SetAudioSampleRate(sourceId, rate));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetAudioChannels(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t num = data.ReadInt32();
    reply.WriteInt32(SetAudioChannels(sourceId, num));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetAudioEncodingBitRate(MessageParcel &data, MessageParcel &reply)
{
    int32_t sourceId = data.ReadInt32();
    int32_t bitRate = data.ReadInt32();
    reply.WriteInt32(SetAudioEncodingBitRate(sourceId, bitRate));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetDataSource(MessageParcel &data, MessageParcel &reply)
{
    int32_t type = data.ReadInt32();
    int32_t sourceId = 0;
    DataSourceType dataType = static_cast<DataSourceType>(type);
    int32_t ret = SetDataSource(dataType, sourceId);
    reply.WriteInt32(sourceId);
    reply.WriteInt32(ret);
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetMaxDuration(MessageParcel &data, MessageParcel &reply)
{
    int32_t duration = data.ReadInt32();
    reply.WriteInt32(SetMaxDuration(duration));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetOutputFormat(MessageParcel &data, MessageParcel &reply)
{
    int32_t type = data.ReadInt32();
    OutputFormatType formatType = static_cast<OutputFormatType>(type);
    reply.WriteInt32(SetOutputFormat(formatType));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetOutputFile(MessageParcel &data, MessageParcel &reply)
{
    int32_t fd = data.ReadFileDescriptor();
    reply.WriteInt32(SetOutputFile(fd));
    (void)::close(fd);
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetLocation(MessageParcel &data, MessageParcel &reply)
{
    (void)reply;
    float latitude = data.ReadFloat();
    float longitude = data.ReadFloat();
    SetLocation(latitude, longitude);
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetOrientationHint(MessageParcel &data, MessageParcel &reply)
{
    (void)reply;
    int32_t rotation = data.ReadInt32();
    return SetOrientationHint(rotation);
}

int32_t RecorderServiceStub::SetUserCustomInfo(MessageParcel &data, MessageParcel &reply)
{
    (void)reply;
    Meta userCustomInfo;
    bool ret = userCustomInfo.FromParcel(data);
    if (!ret) {
        MEDIA_LOGE("userCustomInfo FromParcel failed");
    }
    reply.WriteInt32(SetUserCustomInfo(userCustomInfo));
    return MSERR_OK;
}

int32_t RecorderServiceStub::SetGenre(MessageParcel &data, MessageParcel &reply)
{
    (void)reply;
    std::string genre = data.ReadString();
    reply.WriteInt32(SetGenre(genre));
    return MSERR_OK;
}

int32_t RecorderServiceStub::Prepare(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Prepare());
    return MSERR_OK;
}

int32_t RecorderServiceStub::Start(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Start());
    return MSERR_OK;
}

int32_t RecorderServiceStub::Pause(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Pause());
    return MSERR_OK;
}

int32_t RecorderServiceStub::Resume(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Resume());
    return MSERR_OK;
}

int32_t RecorderServiceStub::Stop(MessageParcel &data, MessageParcel &reply)
{
    bool block = data.ReadBool();
    reply.WriteInt32(Stop(block));
    audioSourceType_ = AUDIO_SOURCE_INVALID;
    return MSERR_OK;
}

int32_t RecorderServiceStub::Reset(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Reset());
    needAudioPermissionCheck = false;
    audioSourceType_ = AUDIO_SOURCE_INVALID;
    return MSERR_OK;
}

int32_t RecorderServiceStub::Release(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Release());
    needAudioPermissionCheck = false;
    audioSourceType_ = AUDIO_SOURCE_INVALID;
    return MSERR_OK;
}

int32_t RecorderServiceStub::DestroyStub(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(DestroyStub());
    needAudioPermissionCheck = false;
    audioSourceType_ = AUDIO_SOURCE_INVALID;
    return MSERR_OK;
}

int32_t RecorderServiceStub::GetAVRecorderConfig(MessageParcel &data, MessageParcel &reply)
{
    ConfigMap configMap;
    GetAVRecorderConfig(configMap);

    (void)reply.WriteInt32(configMap["audioBitrate"]);
    (void)reply.WriteInt32(configMap["audioChannels"]);
    (void)reply.WriteInt32(configMap["audioCodec"]);
    (void)reply.WriteInt32(configMap["audioSampleRate"]);
    (void)reply.WriteInt32(configMap["fileFormat"]);
    (void)reply.WriteInt32(configMap["videoBitrate"]);
    (void)reply.WriteInt32(configMap["videoCodec"]);
    (void)reply.WriteInt32(configMap["videoFrameHeight"]);
    (void)reply.WriteInt32(configMap["videoFrameWidth"]);
    (void)reply.WriteInt32(configMap["videoFrameRate"]);
    (void)reply.WriteInt32(configMap["audioSourceType"]);
    (void)reply.WriteInt32(configMap["videoSourceType"]);
    (void)reply.WriteInt32(configMap["url"]);
    (void)reply.WriteInt32(configMap["rotation"]);
    (void)reply.WriteInt32(configMap["withVideo"]);
    (void)reply.WriteInt32(configMap["withAudio"]);
    (void)reply.WriteInt32(configMap["withLocation"]);

    return MSERR_OK;
}

int32_t RecorderServiceStub::GetLocation(MessageParcel &data, MessageParcel &reply)
{
    Location location;
    GetLocation(location);
    (void)reply.WriteFloat(location.latitude);
    (void)reply.WriteFloat(location.longitude);
    return MSERR_OK;
}

int32_t RecorderServiceStub::CheckPermission()
{
    Security::AccessToken::AccessTokenID tokenCaller = IPCSkeleton::GetCallingTokenID();

    switch (audioSourceType_) {
        case AUDIO_SOURCE_VOICE_CALL:
            return Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenCaller,
                "ohos.permission.RECORD_VOICE_CALL");
        case AUDIO_MIC:
        case AUDIO_SOURCE_DEFAULT:
            return Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenCaller,
                "ohos.permission.MICROPHONE");
        case AUDIO_INNER:
            MEDIA_LOGE("not supported audio source. Permission denied");
            return Security::AccessToken::PERMISSION_DENIED;
        default:
            return Security::AccessToken::PERMISSION_GRANTED;
    }
}

int32_t RecorderServiceStub::GetCurrentCapturerChangeInfo(MessageParcel &data, MessageParcel &reply)
{
    AudioRecorderChangeInfo changeInfo;
    int32_t ret = GetCurrentCapturerChangeInfo(changeInfo);
    changeInfo.Marshalling(reply);

    reply.WriteInt32(ret);
    return MSERR_OK;
}

int32_t RecorderServiceStub::GetAvailableEncoder(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    std::vector<EncoderCapabilityData> encoderInfo;
    int32_t ret = GetAvailableEncoder(encoderInfo);
    reply.WriteInt32(static_cast<int32_t>(encoderInfo.size()));
    for (auto iter = encoderInfo.begin(); iter != encoderInfo.end(); iter++) {
        iter->Marshalling(reply);
    }
    reply.WriteInt32(ret);

    return MSERR_OK;
}

int32_t RecorderServiceStub::GetMaxAmplitude(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(GetMaxAmplitude());

    return MSERR_OK;
}

int32_t RecorderServiceStub::IsWatermarkSupported(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    bool isWatermarkSupported = false;
    int32_t ret = IsWatermarkSupported(isWatermarkSupported);
    CHECK_AND_RETURN_RET_LOG(reply.WriteBool(isWatermarkSupported), MSERR_INVALID_OPERATION, "reply write failed");
    CHECK_AND_RETURN_RET_LOG(reply.WriteInt32(ret), MSERR_INVALID_OPERATION, "reply write failed");
    return MSERR_OK;
}
int32_t RecorderServiceStub::SetWatermark(MessageParcel &data, MessageParcel &reply)
{
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    CHECK_AND_RETURN_RET_LOG(buffer->ReadFromMessageParcel(data), MSERR_INVALID_OPERATION, "read buffer failed");
    CHECK_AND_RETURN_RET_LOG(reply.WriteInt32(SetWatermark(buffer)), MSERR_INVALID_OPERATION, "reply write failed");
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
