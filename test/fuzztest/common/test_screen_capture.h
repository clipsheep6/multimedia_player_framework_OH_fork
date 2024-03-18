/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef TEST_SCREEN_CAPTURE_H
#define TEST_SCREEN_CAPTURE_H

#include <fcntl.h>
#include <thread>
#include <cstdio>
#include <iostream>
#include "nativetoken_kit.h"
#include "accesstoken_kit.h"
#include "token_setproc.h"
#include "screen_capture.h"
#include "aw_common.h"
#include "media_errors.h"

namespace OHOS {
namespace Media {
#define RETURN_IF(cond, ret, ...)        \
do {                                     \
    if (!(cond)) {                       \
        return ret;                      \
    }                                    \
} while (0)

class TestScreenCapture : public NoCopyable {
public:
    TestScreenCapture();
    ~TestScreenCapture();
    std::shared_ptr<ScreenCapture> screenCapture = nullptr;
    bool CreateScreenCapture();
    int32_t SetScreenCaptureCallback(const std::shared_ptr<ScreenCaptureCallBack>& callback);
    int32_t Init(AVScreenCaptureConfig config);
    int32_t StartScreenCapture();
    int32_t StopScreenCapture();
    int32_t Release();
    int32_t SetMicrophoneEnabled(bool isMicrophone);
    int32_t SetCanvasRotation(bool canvasRotation);
    int32_t AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audioBuffer, AudioCaptureSourceType type);
    sptr<OHOS::SurfaceBuffer> AcquireVideoBuffer(int32_t &fence, int64_t &timestamp, Rect &damage);
    int32_t ReleaseAudioBuffer(AudioCaptureSourceType type);
    int32_t ReleaseVideoBuffer();
};

class TestScreenCaptureCallbackTest : public ScreenCaptureCallBack, public NoCopyable {
public:
    TestScreenCaptureCallbackTest() {}
    ~TestScreenCaptureCallbackTest() = default;
    std::shared_ptr<TestScreenCapture> screenCapture;
    void OnError(ScreenCaptureErrorType errorType, int32_t errorCode) override;
    void OnAudioBufferAvailable(bool isReady, AudioCaptureSourceType type) override;
    void OnVideoBufferAvailable(bool isReady) override;
};
} // namespace Media
} // namespace OHOS
#endif
