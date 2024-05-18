/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "screen_capture_service_proxy.h"
#include "media_log.h"
#include "media_errors.h"
#include "avsharedmemory_ipc.h"

namespace {
constexpr int MAX_WINDOWS_LEN = 1000;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ScreenCaptureServiceProxy"};
}

namespace OHOS {
namespace Media {
ScreenCaptureServiceProxy::ScreenCaptureServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardScreenCaptureService>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

ScreenCaptureServiceProxy::~ScreenCaptureServiceProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t ScreenCaptureServiceProxy::DestroyStub()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int error = Remote()->SendRequest(DESTROY, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "DestroyStub failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t ScreenCaptureServiceProxy::SetListenerObject(const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    (void)data.WriteRemoteObject(object);
    int error = Remote()->SendRequest(SET_LISTENER_OBJ, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetListenerObject failed, error: %{public}d", error);

    return reply.ReadInt32();
}

void ScreenCaptureServiceProxy::Release()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "Failed to write descriptor!");

    int error = Remote()->SendRequest(RELEASE, data, reply, option);
    CHECK_AND_RETURN_LOG(error == MSERR_OK, "Release failed, error: %{public}d", error);
}

int32_t ScreenCaptureServiceProxy::SetCaptureMode(CaptureMode captureMode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    token = data.WriteInt32(captureMode);
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write SetCaptureMode!");

    int error = Remote()->SendRequest(SET_CAPTURE_MODE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetCaptureMode failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t ScreenCaptureServiceProxy::SetDataType(DataType dataType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    token = data.WriteInt32(static_cast<int32_t>(dataType));
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write SetDataType!");

    int error = Remote()->SendRequest(SET_DATA_TYPE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetDataType failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t ScreenCaptureServiceProxy::SetRecorderInfo(RecorderInfo recorderInfo)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    token = data.WriteString(recorderInfo.url) && data.WriteString(recorderInfo.fileFormat);
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write recorderInfo!");

    int error = Remote()->SendRequest(SET_RECORDER_INFO, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetRecorderInfo failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t ScreenCaptureServiceProxy::SetOutputFile(int32_t fd)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    (void)data.WriteFileDescriptor(fd);

    int error = Remote()->SendRequest(SET_OUTPUT_FILE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetOutputFile failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t ScreenCaptureServiceProxy::InitAudioEncInfo(AudioEncInfo audioEncInfo)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    token = data.WriteInt32(audioEncInfo.audioBitrate) && data.WriteInt32(audioEncInfo.audioCodecformat);
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write audioEncInfo!");

    int error = Remote()->SendRequest(INIT_AUDIO_ENC_INFO, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "InitAudioEncInfo failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t ScreenCaptureServiceProxy::InitAudioCap(AudioCaptureInfo audioInfo)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    token = data.WriteInt32(audioInfo.audioSampleRate) && data.WriteInt32(audioInfo.audioChannels)
            && data.WriteInt32(audioInfo.audioSource);
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write audioinfo!");

    int error = Remote()->SendRequest(INIT_AUDIO_CAP, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "InitAudioCap failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t ScreenCaptureServiceProxy::InitVideoEncInfo(VideoEncInfo videoEncInfo)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    token = data.WriteInt32(videoEncInfo.videoCodec) && data.WriteInt32(videoEncInfo.videoBitrate) &&
            data.WriteInt32(videoEncInfo.videoFrameRate);
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write videoEncInfo!");

    int error = Remote()->SendRequest(INIT_VIDEO_ENC_INFO, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "InitVideoEncInfo failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t ScreenCaptureServiceProxy::InitVideoCap(VideoCaptureInfo videoInfo)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    token = data.WriteUint64(videoInfo.displayId) && data.WriteInt32(videoInfo.taskIDs.size());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write displayid and taskId size!");
    // write list data
    int count = 0;
    for (int32_t taskID : videoInfo.taskIDs) {
        token = data.WriteInt32(taskID);
        CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write video taskIDs!");
        count++;
        if (count >= MAX_WINDOWS_LEN) {
            break;
        }
    }
    token = data.WriteInt32(videoInfo.videoFrameWidth) && data.WriteInt32(videoInfo.videoFrameHeight) &&
            data.WriteInt32(videoInfo.videoSource);
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write videoinfo!");

    int error = Remote()->SendRequest(INIT_VIDEO_CAP, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION, "InitVideoCap, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t ScreenCaptureServiceProxy::StartScreenCapture(bool isPrivacyAuthorityEnabled)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    token = data.WriteBool(isPrivacyAuthorityEnabled);
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write isPrivacyAuthorityEnabled!");

    int error = Remote()->SendRequest(START_SCREEN_CAPTURE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "StartScreenCapture failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t ScreenCaptureServiceProxy::StartScreenCaptureWithSurface(sptr<Surface> surface, bool isPrivacyAuthorityEnabled)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    CHECK_AND_RETURN_RET_LOG(surface != nullptr, MSERR_NO_MEMORY, "surface is nullptr");
    sptr<IBufferProducer> producer = surface->GetProducer();
    CHECK_AND_RETURN_RET_LOG(producer != nullptr, MSERR_NO_MEMORY, "producer is nullptr");

    sptr<IRemoteObject> object = producer->AsObject();
    if (data.WriteRemoteObject(object)) {
        MEDIA_LOGI("ScreenCaptureServiceProxy StartScreenCaptureWithSurface WriteRemoteObject successfully");
    } else {
        MEDIA_LOGI("ScreenCaptureServiceProxy StartScreenCaptureWithSurface WriteRemoteObject failed");
        return MSERR_INVALID_OPERATION;
    }

    token = data.WriteBool(isPrivacyAuthorityEnabled);
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write isPrivacyAuthorityEnabled!");

    int error = Remote()->SendRequest(START_SCREEN_CAPTURE_WITH_SURFACE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "StartScreenCapture failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t ScreenCaptureServiceProxy::StopScreenCapture()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int error = Remote()->SendRequest(STOP_SCREEN_CAPTURE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "StopScreenCapture failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t ScreenCaptureServiceProxy::AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audioBuffer,
                                                      AudioCaptureSourceType type)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    token = data.WriteInt32(type);
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write type!");

    int error = Remote()->SendRequest(ACQUIRE_AUDIO_BUF, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
                             "AcquireAudioBuffer failed, error: %{public}d", error);
    int ret = reply.ReadInt32();
    if (ret == MSERR_OK) {
        int32_t audioBufferLen = reply.ReadInt32();
        if (audioBufferLen < 0) {
            return MSERR_INVALID_VAL;
        }
        auto buffer = reply.ReadBuffer(audioBufferLen);
        uint8_t* audiobuffer = static_cast<uint8_t *>(malloc(audioBufferLen));
        CHECK_AND_RETURN_RET_LOG(audiobuffer != nullptr, MSERR_NO_MEMORY, "audio buffer malloc failed");
        memset_s(audiobuffer, audioBufferLen, 0, audioBufferLen);
        if (memcpy_s(audiobuffer, audioBufferLen, buffer, audioBufferLen) != EOK) {
            MEDIA_LOGE("audioBuffer memcpy_s fail");
        }
        AudioCaptureSourceType sourceType = static_cast<AudioCaptureSourceType>(reply.ReadInt32());
        if ((sourceType > APP_PLAYBACK) || (sourceType < SOURCE_INVALID)) {
            sourceType = type;
        }
        int64_t audioTime = reply.ReadInt64();
        audioBuffer = std::make_shared<AudioBuffer>(audiobuffer, audioBufferLen, audioTime, sourceType);
    }
    return ret;
}

int32_t ScreenCaptureServiceProxy::AcquireVideoBuffer(sptr<OHOS::SurfaceBuffer> &surfaceBuffer, int32_t &fence,
                                                      int64_t &timestamp, OHOS::Rect &damage)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int error = Remote()->SendRequest(ACQUIRE_VIDEO_BUF, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
                             "AcquireVideoBuffer failed, error: %{public}d", error);
    int ret = reply.ReadInt32();
    if (ret == MSERR_OK) {
        if (surfaceBuffer != nullptr) {
            surfaceBuffer->ReadFromMessageParcel(reply);
        }
        fence = reply.ReadInt32();
        timestamp = reply.ReadInt64();
        damage.x = reply.ReadInt32();
        damage.y = reply.ReadInt32();
        damage.w = reply.ReadInt32();
        damage.h = reply.ReadInt32();
    }
    return ret;
}

int32_t ScreenCaptureServiceProxy::ReleaseAudioBuffer(AudioCaptureSourceType type)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    token = data.WriteInt32(type);
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write type!");

    int error = Remote()->SendRequest(RELEASE_AUDIO_BUF, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
                             "ReleaseAudioBuffer failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t ScreenCaptureServiceProxy::ReleaseVideoBuffer()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int error = Remote()->SendRequest(RELEASE_VIDEO_BUF, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
                             "ReleaseVideoBuffer failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t ScreenCaptureServiceProxy::ExcludeContent(ScreenCaptureContentFilter &contentFilter)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    token = data.WriteInt32(static_cast<int32_t>(contentFilter.filteredAudioContents.size()));
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write filteredAudioContents size!");

    // The filteredAudioContents size is limited, no big data risk.
    for (const auto &element : contentFilter.filteredAudioContents) {
        token = data.WriteInt32(static_cast<int32_t>(element));
        CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write filteredAudioContents");
    }
    token = data.WriteInt32(static_cast<int32_t>(contentFilter.windowIDsVec.size()));
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write windowCount size!");
    for (size_t i = 0; i < contentFilter.windowIDsVec.size(); i++) {
        token = data.WriteInt32(static_cast<int32_t>(contentFilter.windowIDsVec[i]));
        CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write windowIDs");
    }
    int error = Remote()->SendRequest(EXCLUDE_CONTENT, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "ExcludeContent failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t ScreenCaptureServiceProxy::SetMicrophoneEnabled(bool isMicrophone)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    token = data.WriteBool(isMicrophone);
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write Microphone state!");

    int error = Remote()->SendRequest(SET_MIC_ENABLE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
                             "SetMicrophoneEnabled failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t ScreenCaptureServiceProxy::SetCanvasRotation(bool canvasRotation)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    token = data.WriteBool(canvasRotation);
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write Microphone state!");

    int error = Remote()->SendRequest(SET_SCREEN_ROTATION, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
                             "SetCanvasRotation failed, error: %{public}d", error);
    return reply.ReadInt32();
}
} // namespace Media
} // namespace OHOS
