/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "screen_capture_native_mock.h"
#include "media_log.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::Media::ScreenCaptureTestParam;

namespace OHOS {
namespace Media {
void ScreenCaptureNativeCallbackMock::OnError(ScreenCaptureErrorType errorType, int32_t errorCode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    (void)errorType;
    if (mockCb_ != nullptr) {
        mockCb_->OnError(errorCode);
    }
}

void ScreenCaptureNativeCallbackMock::OnAudioBufferAvailable(bool isReady, AudioCaptureSourceType type)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (mockCb_ != nullptr) {
        mockCb_->OnAudioBufferAvailable(isReady, type);
    }
}

void ScreenCaptureNativeCallbackMock::OnVideoBufferAvailable(bool isReady)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (mockCb_ != nullptr) {
        mockCb_->OnVideoBufferAvailable(isReady);
    }
}

void ScreenCaptureNativeCallbackMock::OnStateChange(AVScreenCaptureStateCode stateCode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (mockCb_ != nullptr) {
        mockCb_->OnStateChange(stateCode);
    }
}

void ScreenCaptureNativeCallbackMock::OnRelease()
{
    std::unique_lock<std::mutex> lock(mutex_);
    mockCb_ = nullptr;
}

ScreenCaptureNativeMock::~ScreenCaptureNativeMock()
{
    MEDIA_LOGI("ScreenCaptureNativeMock::~ScreenCaptureNativeMock");
}

int32_t ScreenCaptureNativeMock::SetScreenCaptureCallback(const std::shared_ptr<ScreenCaptureCallBackMock>& callback,
    const bool isErrorCallBackEnabled, const bool isDataCallBackEnabled, const bool isStateChangeCallBackEnabled)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    (void)isErrorCallBackEnabled; // Inner is not support to set New ErrorCallBack
    (void)isDataCallBackEnabled; // Inner is not support to set New DataCallBack
    isStateChangeCallBackEnabled_ = isStateChangeCallBackEnabled;
    if (callback != nullptr) {
        cb_ = std::make_shared<ScreenCaptureNativeCallbackMock>(callback, screenCapture_);
        return screenCapture_->SetScreenCaptureCallback(cb_);
    }
    return MSERR_INVALID_OPERATION;
}

int32_t ScreenCaptureNativeMock::StartScreenCapture()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    if (isStateChangeCallBackEnabled_) {
        int32_t ret = screenCapture_->SetPrivacyAuthorityEnabled();
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "SetPrivacyAuthorityEnabled failed");
    }
    return screenCapture_->StartScreenCapture();
}

int32_t ScreenCaptureNativeMock::StartScreenCaptureWithSurface(const std::any& value)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    if (isStateChangeCallBackEnabled_) {
        int32_t ret = screenCapture_->SetPrivacyAuthorityEnabled();
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "SetPrivacyAuthorityEnabled failed");
    }
    sptr<Surface> surface = std::any_cast<sptr<Surface>>(value);
    return screenCapture_->StartScreenCaptureWithSurface(surface);
}

int32_t ScreenCaptureNativeMock::Init(AVScreenCaptureConfig config)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    return screenCapture_->Init(config);
}

int32_t ScreenCaptureNativeMock::StopScreenCapture()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    return screenCapture_->StopScreenCapture();
}

int32_t ScreenCaptureNativeMock::StartScreenRecording()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    if (isStateChangeCallBackEnabled_) {
        int32_t ret = screenCapture_->SetPrivacyAuthorityEnabled();
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "SetPrivacyAuthorityEnabled failed");
    }
    return screenCapture_->StartScreenRecording();
}

int32_t ScreenCaptureNativeMock::StopScreenRecording()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    return screenCapture_->StopScreenRecording();
}

int32_t ScreenCaptureNativeMock::Release()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    std::shared_ptr<ScreenCaptureCallBack> cb = cb_;
    ScreenCaptureNativeCallbackMock *callback = static_cast<ScreenCaptureNativeCallbackMock *>(cb.get());
    if (callback != nullptr) {
        callback->OnRelease();
        cb_ = nullptr;
    }
    return screenCapture_->Release();
}

int32_t ScreenCaptureNativeMock::SetMicrophoneEnabled(bool isMicrophone)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    return screenCapture_->SetMicrophoneEnabled(isMicrophone);
}

int32_t ScreenCaptureNativeMock::SetCanvasRotation(bool canvasRotation)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    return screenCapture_->SetCanvasRotation(canvasRotation);
}

int32_t ScreenCaptureNativeMock::AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audioBuffer,
    AudioCaptureSourceType type)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    return screenCapture_->AcquireAudioBuffer(audioBuffer, type);
}

sptr<OHOS::SurfaceBuffer> ScreenCaptureNativeMock::AcquireVideoBuffer(int32_t &fence, int64_t &timestamp,
    OHOS::Rect &damage)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, nullptr, "screenCapture_ == nullptr");
    return screenCapture_->AcquireVideoBuffer(fence, timestamp, damage);
}

int32_t ScreenCaptureNativeMock::ReleaseAudioBuffer(AudioCaptureSourceType type)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    return screenCapture_->ReleaseAudioBuffer(type);
}

int32_t ScreenCaptureNativeMock::ReleaseVideoBuffer()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    return screenCapture_->ReleaseVideoBuffer();
}
} // namespace Media
} // namespace OHOS