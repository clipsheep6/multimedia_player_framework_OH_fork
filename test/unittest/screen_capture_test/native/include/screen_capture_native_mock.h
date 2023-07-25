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
#ifndef SCREEN_CAPTURE_NATIVE_MOCK_H
#define SCREEN_CAPTURE_NATIVE_MOCK_H

#include "screen_capture_mock.h"

namespace OHOS {
namespace Media {
class ScreenCaptureNativeMock : public ScreenCaptureMock {
public:
    explicit ScreenCaptureNativeMock(std::shared_ptr<ScreenCapture> screencapture) : screenCapture_(screencapture) {}
    ~ScreenCaptureNativeMock() = default;
    int32_t SetScreenCaptureCallback(const std::shared_ptr<ScreenCaptureCallBackMock>& callback) override;
    int32_t Init(AVScreenCaptureConfig config) override;
    int32_t StartScreenCapture() override;
    int32_t StopScreenCapture() override;
    int32_t Release() override;
    int32_t SetMicrophoneEnabled(bool isMicrophone) override;
    int32_t AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audioBuffer, AudioCaptureSourceType type) override;
    sptr<OHOS::SurfaceBuffer> AcquireVideoBuffer(int32_t &fence, int64_t &timestamp, OHOS::Rect &damage) override;
    int32_t ReleaseAudioBuffer(AudioCaptureSourceType type) override;
    int32_t ReleaseVideoBuffer() override;

private:
    std::shared_ptr<ScreenCapture> screenCapture_ = nullptr;
    AVScreenCaptureConfig config_;
};

class ScreenCaptureNativeCallbackMock : public ScreenCaptureCallBack, public NoCopyable {
public:
    ScreenCaptureNativeCallbackMock(std::shared_ptr<ScreenCaptureCallBackMock> cb,
        std::weak_ptr<ScreenCapture> vd) : mockCb_(cb), screenCapture_(vd) {}
    ~ScreenCaptureNativeCallbackMock()
    {
        mockCb_ = nullptr;
    }
    void OnError(ScreenCaptureErrorType errorType, int32_t errorCode) override;
    void OnAudioBufferAvailable(bool isReady, AudioCaptureSourceType type) override;
    void OnVideoBufferAvailable(bool isReady) override;

private:
    std::shared_ptr<ScreenCaptureCallBackMock> mockCb_ = nullptr;
    std::weak_ptr<ScreenCapture> screenCapture_;
};
} // namespace Media
} // namespace OHOS
#endif