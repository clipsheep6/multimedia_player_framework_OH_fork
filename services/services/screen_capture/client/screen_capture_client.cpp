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

#include "screen_capture_client.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ScreenCaptureClient"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<ScreenCaptureClient> ScreenCaptureClient::Create(
    const sptr<IStandardScreenCaptureService> &ipcProxy)
{
    std::shared_ptr<ScreenCaptureClient> screencapture = std::make_shared<ScreenCaptureClient>(ipcProxy);

    CHECK_AND_RETURN_RET_LOG(screencapture != nullptr, nullptr, "failed to new Screen Capture Client..");

    int32_t ret = screencapture->CreateListenerObject();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to create listener object..");

    return screencapture;
}

ScreenCaptureClient::ScreenCaptureClient(const sptr<IStandardScreenCaptureService> &ipcProxy)
    : screenCaptureProxy_(ipcProxy)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

int32_t ScreenCaptureClient::CreateListenerObject()
{
    std::lock_guard<std::mutex> lock(mutex_);
    listenerStub_ = new(std::nothrow) ScreenCaptureListenerStub();
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, MSERR_NO_MEMORY, "failed to new RecorderListenerStub object");
    CHECK_AND_RETURN_RET_LOG(screenCaptureProxy_ != nullptr, MSERR_NO_MEMORY, "recorder service does not exist.");

    sptr<IRemoteObject> object = listenerStub_->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_NO_MEMORY, "listener object is nullptr..");

    MEDIA_LOGD("SetListenerObject");
    return screenCaptureProxy_->SetListenerObject(object);
}

ScreenCaptureClient::~ScreenCaptureClient()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (screenCaptureProxy_ != nullptr) {
        (void)screenCaptureProxy_->DestroyStub();
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t ScreenCaptureClient::SetScreenCaptureCallback(const std::shared_ptr<ScreenCaptureCallBack> &callback)
{
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_NO_MEMORY, "input param callback is nullptr.");
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, MSERR_NO_MEMORY, "listenerStub_ is nullptr.");

    callback_ = callback;
    MEDIA_LOGD("SetScreenCaptureCallback");
    listenerStub_->SetScreenCaptureCallback(callback);
    return MSERR_OK;
}

void ScreenCaptureClient::MediaServerDied()
{
    std::lock_guard<std::mutex> lock(mutex_);
    screenCaptureProxy_ = nullptr;
    listenerStub_ = nullptr;
    if (callback_ != nullptr) {
        callback_->OnError(SCREEN_CAPTURE_ERROR_INTERNAL, MSERR_SERVICE_DIED);
    }
}

void ScreenCaptureClient::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(screenCaptureProxy_ != nullptr, "screenCapture service does not exist.");
    screenCaptureProxy_->Release();
}

int32_t ScreenCaptureClient::InitAudioCap(AudioCapInfo audioInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(screenCaptureProxy_ != nullptr, MSERR_NO_MEMORY, "screenCapture service does not exist.");
    return screenCaptureProxy_->InitAudioCap(audioInfo);
}

int32_t ScreenCaptureClient::InitVideoCap(VideoCapInfo videoInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(screenCaptureProxy_ != nullptr, MSERR_NO_MEMORY, "screenCapture service does not exist.");
    return screenCaptureProxy_->InitVideoCap(videoInfo);
}

int32_t ScreenCaptureClient::SetMicrophoneEnable(bool isMicrophone)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(screenCaptureProxy_ != nullptr, MSERR_NO_MEMORY, "screenCapture service does not exist.");
    return screenCaptureProxy_->SetMicrophoneEnable(isMicrophone);
}

int32_t ScreenCaptureClient::StartScreenCapture()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(screenCaptureProxy_ != nullptr, MSERR_NO_MEMORY, "screenCapture service does not exist.");
    return screenCaptureProxy_->StartScreenCapture();
}

int32_t ScreenCaptureClient::StopScreenCapture()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(screenCaptureProxy_ != nullptr, MSERR_NO_MEMORY, "screenCapture service does not exist.");
    return screenCaptureProxy_->StopScreenCapture();
}

int32_t ScreenCaptureClient::AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audioBuffer, AudioCapSourceType type)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(screenCaptureProxy_ != nullptr, MSERR_NO_MEMORY, "screenCapture service does not exist.");
    return screenCaptureProxy_->AcquireAudioBuffer(audioBuffer, type);
}

int32_t ScreenCaptureClient::AcquireVideoBuffer(sptr<OHOS::SurfaceBuffer> &surfacebuffer, int32_t &fence,
                                               int64_t &timestamp, OHOS::Rect &damage)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(screenCaptureProxy_ != nullptr, MSERR_NO_MEMORY, "screenCapture service does not exist.");
    return screenCaptureProxy_->AcquireVideoBuffer(surfacebuffer, fence, timestamp, damage);
}

int32_t ScreenCaptureClient::ReleaseAudioBuffer(AudioCapSourceType type)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(screenCaptureProxy_ != nullptr, MSERR_NO_MEMORY, "screenCapture service does not exist.");
    return screenCaptureProxy_->ReleaseAudioBuffer(type);
}

int32_t ScreenCaptureClient::ReleaseVideoBuffer()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(screenCaptureProxy_ != nullptr, MSERR_NO_MEMORY, "screenCapture service does not exist.");
    return screenCaptureProxy_->ReleaseVideoBuffer();
}
} // namespace Media
} // namespace OHOS