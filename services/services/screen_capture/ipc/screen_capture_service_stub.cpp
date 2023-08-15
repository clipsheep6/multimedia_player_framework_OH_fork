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

#include "screen_capture_service_stub.h"
#include "media_server_manager.h"
#include "media_log.h"
#include "media_errors.h"
#include "avsharedmemory_ipc.h"
#include "screen_capture_listener_proxy.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ScreenCaptureServiceStub"};
}

namespace OHOS {
namespace Media {
sptr<ScreenCaptureServiceStub> ScreenCaptureServiceStub::Create()
{
    sptr<ScreenCaptureServiceStub> screenCaptureStub = new(std::nothrow) ScreenCaptureServiceStub();
    CHECK_AND_RETURN_RET_LOG(screenCaptureStub != nullptr, nullptr, "failed to new ScreenCaptureServiceStub");

    int32_t ret = screenCaptureStub->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to screenCapture stub init");
    return screenCaptureStub;
}

ScreenCaptureServiceStub::ScreenCaptureServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

ScreenCaptureServiceStub::~ScreenCaptureServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t ScreenCaptureServiceStub::Init()
{
    screenCaptureServer_ = ScreenCaptureServer::Create();
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, MSERR_NO_MEMORY,
        "failed to create ScreenCaptureServer Service");
    screenCaptureStubFuncs_[SET_LISTENER_OBJ] = &ScreenCaptureServiceStub::SetListenerObject;
    screenCaptureStubFuncs_[RELEASE] = &ScreenCaptureServiceStub::Release;
    screenCaptureStubFuncs_[SET_MIC_ENABLE] = &ScreenCaptureServiceStub::SetMicrophoneEnabled;
    screenCaptureStubFuncs_[SET_CAPTURE_MODE] = &ScreenCaptureServiceStub::SetCaptureMode;
    screenCaptureStubFuncs_[INIT_AUDIO_CAP] = &ScreenCaptureServiceStub::InitAudioCap;
    screenCaptureStubFuncs_[INIT_VIDEO_CAP] = &ScreenCaptureServiceStub::InitVideoCap;
    screenCaptureStubFuncs_[START_SCREEN_CAPTURE] = &ScreenCaptureServiceStub::StartScreenCapture;
    screenCaptureStubFuncs_[STOP_SCREEN_CAPTURE] = &ScreenCaptureServiceStub::StopScreenCapture;
    screenCaptureStubFuncs_[ACQUIRE_AUDIO_BUF] = &ScreenCaptureServiceStub::AcquireAudioBuffer;
    screenCaptureStubFuncs_[ACQUIRE_VIDEO_BUF] = &ScreenCaptureServiceStub::AcquireVideoBuffer;
    screenCaptureStubFuncs_[RELEASE_AUDIO_BUF] = &ScreenCaptureServiceStub::ReleaseAudioBuffer;
    screenCaptureStubFuncs_[RELEASE_VIDEO_BUF] = &ScreenCaptureServiceStub::ReleaseVideoBuffer;
    screenCaptureStubFuncs_[DESTROY] = &ScreenCaptureServiceStub::DestroyStub;

    return MSERR_OK;
}

int32_t ScreenCaptureServiceStub::DestroyStub()
{
    screenCaptureServer_ = nullptr;
    MediaServerManager::GetInstance().DestroyStubObject(MediaServerManager::SCREEN_CAPTURE, AsObject());
    return MSERR_OK;
}

int ScreenCaptureServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    MEDIA_LOGD("Stub: OnRemoteRequest of code: %{public}u is received", code);

    auto remoteDescriptor = data.ReadInterfaceToken();
    if (ScreenCaptureServiceStub::GetDescriptor() != remoteDescriptor) {
        MEDIA_LOGE("Invalid descriptor");
        return MSERR_INVALID_OPERATION;
    }

    auto itFunc = screenCaptureStubFuncs_.find(code);
    if (itFunc != screenCaptureStubFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            int32_t ret = (this->*memberFunc)(data, reply);
            if (ret != MSERR_OK) {
                MEDIA_LOGE("Calling memberFunc is failed.");
            }
            return MSERR_OK;
        }
    }
    MEDIA_LOGW("ScreenCaptureServiceStub: no member func supporting, applying default process");

    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t ScreenCaptureServiceStub::SetCaptureMode(CaptureMode captureMode)
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, false,
        "screen capture server is nullptr");
    return screenCaptureServer_->SetCaptureMode(captureMode);
}

int32_t ScreenCaptureServiceStub::InitAudioCap(AudioCaptureInfo audioInfo)
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, false,
        "screen capture server is nullptr");
    return screenCaptureServer_->InitAudioCap(audioInfo);
}

int32_t ScreenCaptureServiceStub::InitVideoCap(VideoCaptureInfo videoInfo)
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, false,
        "screen capture server is nullptr");
    return screenCaptureServer_->InitVideoCap(videoInfo);
}

int32_t ScreenCaptureServiceStub::StartScreenCapture()
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, false,
        "screen capture server is nullptr");
    return screenCaptureServer_->StartScreenCapture();
}

int32_t ScreenCaptureServiceStub::StopScreenCapture()
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, false,
        "screen capture server is nullptr");
    return screenCaptureServer_->StopScreenCapture();
}

int32_t ScreenCaptureServiceStub::SetListenerObject(const sptr<IRemoteObject> &object)
{
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_NO_MEMORY, "set listener object is nullptr");

    sptr<IStandardScreenCaptureListener> listener = iface_cast<IStandardScreenCaptureListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, MSERR_NO_MEMORY, "failed to convert IStandardScreenCaptureListener");

    std::shared_ptr<ScreenCaptureCallBack> callback = std::make_shared<ScreenCaptureListenerCallback>(listener);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_NO_MEMORY, "failed to new ScreenCaptureCallBack");

    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, MSERR_NO_MEMORY, "recorder server is nullptr");
    (void)screenCaptureServer_->SetScreenCaptureCallback(callback);
    return MSERR_OK;
}

int32_t ScreenCaptureServiceStub::SetMicrophoneEnabled(bool isMicrophone)
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, false,
        "screen capture server is nullptr");
    return screenCaptureServer_->SetMicrophoneEnabled(isMicrophone);
}

int32_t ScreenCaptureServiceStub::AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audioBuffer,
                                                     AudioCaptureSourceType type)
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, false,
        "screen capture server is nullptr");
    return screenCaptureServer_->AcquireAudioBuffer(audioBuffer, type);
}

int32_t ScreenCaptureServiceStub::AcquireVideoBuffer(sptr<OHOS::SurfaceBuffer> &surfaceBuffer, int32_t &fence,
                                                     int64_t &timestamp, OHOS::Rect &damage)
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, false,
        "screen capture server is nullptr");
    return screenCaptureServer_->AcquireVideoBuffer(surfaceBuffer, fence, timestamp, damage);
}

int32_t ScreenCaptureServiceStub::ReleaseAudioBuffer(AudioCaptureSourceType type)
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, false,
        "screen capture server is nullptr");
    return screenCaptureServer_->ReleaseAudioBuffer(type);
}

int32_t ScreenCaptureServiceStub::ReleaseVideoBuffer()
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, false,
        "screen capture server is nullptr");
    return screenCaptureServer_->ReleaseVideoBuffer();
}

int32_t ScreenCaptureServiceStub::SetMicrophoneEnabled(MessageParcel &data, MessageParcel &reply)
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, MSERR_INVALID_STATE,
        "screen capture server is nullptr");
    (void)data;
    bool setMicEnable = data.ReadBool();
    int32_t ret = SetMicrophoneEnabled(setMicEnable);
    reply.WriteInt32(ret);
    return MSERR_OK;
}

int32_t ScreenCaptureServiceStub::SetCaptureMode(MessageParcel &data, MessageParcel &reply)
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, MSERR_INVALID_STATE,
        "screen capture server is nullptr");
    (void)data;
    CaptureMode mode = static_cast<CaptureMode>(data.ReadInt32());
    int32_t ret = SetCaptureMode(mode);
    reply.WriteInt32(ret);
    return MSERR_OK;
}

int32_t ScreenCaptureServiceStub::InitAudioCap(MessageParcel &data, MessageParcel &reply)
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, MSERR_INVALID_STATE,
        "screen capture server is nullptr");
    (void)data;
    AudioCaptureInfo audioInfo;
    audioInfo.audioSampleRate = data.ReadInt32();
    audioInfo.audioChannels = data.ReadInt32();
    audioInfo.audioSource = static_cast<AudioCaptureSourceType>(data.ReadInt32());
    int32_t ret = InitAudioCap(audioInfo);
    reply.WriteInt32(ret);
    return MSERR_OK;
}

int32_t ScreenCaptureServiceStub::InitVideoCap(MessageParcel &data, MessageParcel &reply)
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, MSERR_INVALID_STATE,
        "screen capture server is nullptr");
    (void)data;
    VideoCaptureInfo videoInfo;
    videoInfo.displayId = data.ReadUint64();
    int32_t size = data.ReadInt32();
    if (size > 0) {
        for (auto i = 0; i < size; i++) {
            videoInfo.taskIDs.push_back(data.ReadInt32());
        }
    }
    videoInfo.videoFrameWidth = data.ReadInt32();
    videoInfo.videoFrameHeight = data.ReadInt32();
    videoInfo.videoSource = static_cast<VideoSourceType>(data.ReadInt32());
    int32_t ret = InitVideoCap(videoInfo);
    reply.WriteInt32(ret);
    return MSERR_OK;
}

int32_t ScreenCaptureServiceStub::StartScreenCapture(MessageParcel &data, MessageParcel &reply)
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, MSERR_INVALID_STATE,
        "screen capture server is nullptr");
    (void)data;
    int32_t ret = StartScreenCapture();
    reply.WriteInt32(ret);
    return MSERR_OK;
}

int32_t ScreenCaptureServiceStub::StopScreenCapture(MessageParcel &data, MessageParcel &reply)
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, MSERR_INVALID_STATE,
        "screen capture server is nullptr");
    (void)data;
    int32_t ret = StopScreenCapture();
    reply.WriteInt32(ret);
    return MSERR_OK;
}

int32_t ScreenCaptureServiceStub::AcquireAudioBuffer(MessageParcel &data, MessageParcel &reply)
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, MSERR_INVALID_STATE,
        "screen capture server is nullptr");
    (void)data;
    std::shared_ptr<AudioBuffer> audioBuffer;
    AudioCaptureSourceType type = static_cast<AudioCaptureSourceType>(data.ReadInt32());
    int32_t ret = AcquireAudioBuffer(audioBuffer, type);
    reply.WriteInt32(ret);
    if (ret == MSERR_OK) {
        reply.WriteInt32(audioBuffer->length);
        if ((audioBuffer->buffer != nullptr)&&(audioBuffer->length > 0)) {
            reply.WriteBuffer(audioBuffer->buffer, audioBuffer->length);
        }
        reply.WriteInt32(audioBuffer->sourcetype);
        reply.WriteInt64(audioBuffer->timestamp);
    }
    return MSERR_OK;
}

int32_t ScreenCaptureServiceStub::AcquireVideoBuffer(MessageParcel &data, MessageParcel &reply)
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, MSERR_INVALID_STATE,
        "screen capture server is nullptr");
    (void)data;
    int32_t fence = 0;
    int64_t timestamp = 0;
    OHOS::Rect damage;
    sptr<OHOS::SurfaceBuffer> videoBuffer = nullptr;
    int32_t ret = AcquireVideoBuffer(videoBuffer, fence, timestamp, damage);
    reply.WriteInt32(ret);
    if (ret == MSERR_OK) {
        if (videoBuffer != nullptr) {
            videoBuffer->WriteToMessageParcel(reply);
        }
        reply.WriteInt32(fence);
        reply.WriteInt64(timestamp);
        reply.WriteInt32(damage.x);
        reply.WriteInt32(damage.y);
        reply.WriteInt32(damage.w);
        reply.WriteInt32(damage.h);
    }
    return MSERR_OK;
}

int32_t ScreenCaptureServiceStub::ReleaseAudioBuffer(MessageParcel &data, MessageParcel &reply)
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, MSERR_INVALID_STATE,
        "screen capture server is nullptr");
    (void)data;
    AudioCaptureSourceType type = static_cast<AudioCaptureSourceType>(data.ReadInt32());
    int32_t ret = ReleaseAudioBuffer(type);
    reply.WriteInt32(ret);
    return MSERR_OK;
}

int32_t ScreenCaptureServiceStub::ReleaseVideoBuffer(MessageParcel &data, MessageParcel &reply)
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureServer_ != nullptr, MSERR_INVALID_STATE,
        "screen capture server is nullptr");
    (void)data;
    int32_t ret = ReleaseVideoBuffer();
    reply.WriteInt32(ret);
    return MSERR_OK;
}
int32_t ScreenCaptureServiceStub::SetListenerObject(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    reply.WriteInt32(SetListenerObject(object));
    return MSERR_OK;
}

void ScreenCaptureServiceStub::Release()
{
    CHECK_AND_RETURN_LOG(screenCaptureServer_ != nullptr, "screen capture server is nullptr");
    return screenCaptureServer_->Release();
}

int32_t ScreenCaptureServiceStub::Release(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    (void)reply;
    Release();
    return MSERR_OK;
}

int32_t ScreenCaptureServiceStub::DestroyStub(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(DestroyStub());
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS