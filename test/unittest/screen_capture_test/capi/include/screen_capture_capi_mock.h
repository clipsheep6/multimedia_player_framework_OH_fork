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
#ifndef SCREEN_CAPTURE_CAPI_MOCK_H
#define SCREEN_CAPTURE_CAPI_MOCK_H

#include "native_avscreen_capture.h"
#include "screen_capture_mock.h"

namespace OHOS {
namespace Media {
class ScreenCaptureCapiMock : public ScreenCaptureMock {
public:
    explicit ScreenCaptureCapiMock(OH_AVScreenCapture* screencapture) : screenCapture_(screencapture) {}
    ~ScreenCaptureCapiMock() = default;
    int32_t SetScreenCaptureCallback(const std::shared_ptr<ScreenCaptureCallBackMock>& callback,
        const bool isErrorCallBackEnabled, const bool isDataCallBackEnabled,
        const bool isStateChangeCallBackEnabled) override;
    int32_t Init(AVScreenCaptureConfig config) override;
    int32_t StartScreenCapture() override;
    int32_t StartScreenCaptureWithSurface(const std::any& value) override;
    int32_t StopScreenCapture() override;
    int32_t StartScreenRecording() override;
    int32_t StopScreenRecording() override;
    int32_t Release() override;
    int32_t SetMicrophoneEnabled(bool isMicrophone) override;
    int32_t SetCanvasRotation(bool canvasRotation) override;
    int32_t AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audioBuffer,
        OHOS::Media::AudioCaptureSourceType type) override;
    sptr<OHOS::SurfaceBuffer> AcquireVideoBuffer(int32_t &fence, int64_t &timestamp, OHOS::Rect &damage) override;
    int32_t ReleaseAudioBuffer(OHOS::Media::AudioCaptureSourceType type) override;
    int32_t ReleaseVideoBuffer() override;
    bool IsErrorCallBackEnabled() override
    {
        return isErrorCallBackEnabled_;
    }
    bool IsDataCallBackEnabled() override
    {
        return isDataCallBackEnabled_;
    }
    bool IsStateChangeCallBackEnabled() override
    {
        return isStateChangeCallBackEnabled_;
    }
private:
    static void SetScreenCaptureCallback(OH_AVScreenCapture *screencapture,
        std::shared_ptr<ScreenCaptureCallBackMock> cb);
    static std::shared_ptr<ScreenCaptureCallBackMock> GetCallback(OH_AVScreenCapture *screenCapture);
    static void DelCallback(OH_AVScreenCapture *screenCapture);
    static void OnError(OH_AVScreenCapture *screenCapture, int32_t errorCode);
    static void OnAudioBufferAvailable(OH_AVScreenCapture *screenCapture, bool isReady, OH_AudioCaptureSourceType type);
    static void OnVideoBufferAvailable(OH_AVScreenCapture *screenCapture, bool isReady);
    static void OnErrorNew(OH_AVScreenCapture *screenCapture, int32_t errorCode, void *userData);
    static void OnBufferAvailable(OH_AVScreenCapture *screenCapture, OH_AVBuffer *buffer,
        OH_AVScreenCaptureBufferType bufferType, int64_t timestamp, void *userData);
    static void OnStateChange(struct OH_AVScreenCapture *capture,
        OH_AVScreenCaptureStateCode stateCode, void *userData);
    OH_AVScreenCaptureConfig Convert(AVScreenCaptureConfig config);

    static std::mutex mutex_;
    static std::map<OH_AVScreenCapture *, std::shared_ptr<ScreenCaptureCallBackMock>> mockCbMap_;

    OH_AVScreenCapture* screenCapture_ = nullptr;
    bool isErrorCallBackEnabled_ = false;
    bool isDataCallBackEnabled_ = false;
    bool isStateChangeCallBackEnabled_ = false;
};
} // namespace Media
} // namespace OHOS
#endif