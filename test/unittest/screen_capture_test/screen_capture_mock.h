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
#ifndef SCREEN_CAPTURE_MOCK_H
#define SCREEN_CAPTURE_MOCK_H

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <securec.h>
#include <string>
#include <thread>
#include <cstdio>
#include "avbuffer.h"
#include "gtest/gtest.h"
#include "screen_capture.h"
#include "screen_capture_controller.h"
#include "unittest_log.h"
#include "media_errors.h"
#include "display_manager.h"
#include "screen_manager.h"
#include "external_window.h"

namespace OHOS {
namespace Media {
namespace ScreenCaptureTestParam {
    constexpr uint32_t RECORDER_TIME = 2;
} // namespace ScreenCaptureTestParam

class ScreenCaptureCallBackMock : public NoCopyable {
public:
    virtual void OnError(int32_t errorCode) = 0;
    virtual void OnAudioBufferAvailable(bool isReady, AudioCaptureSourceType type) = 0;
    virtual void OnVideoBufferAvailable(bool isReady) = 0;
    virtual void OnStateChange(AVScreenCaptureStateCode stateCode) = 0;
    virtual void OnError(int32_t errorCode, void *userData)
    {
        (void)errorCode;
        (void)userData;
    }
    virtual void OnBufferAvailable(const std::shared_ptr<AVBuffer> buffer,
        AVScreenCaptureBufferType bufferType, int64_t timestamp) = 0;
};

class ScreenCaptureMock {
public:
    virtual ~ScreenCaptureMock() = default;
    virtual int32_t SetScreenCaptureCallback(const std::shared_ptr<ScreenCaptureCallBackMock>& callback,
        const bool isErrorCallBackEnabled = false, const bool isDataCallBackEnabled = false,
        const bool isStateChangeCallBackEnabled = false) = 0;
    virtual int32_t Init(AVScreenCaptureConfig config) = 0;
    virtual int32_t StartScreenCapture() = 0;
    virtual int32_t StartScreenCaptureWithSurface(const std::any& value) = 0;
    virtual int32_t StopScreenCapture() = 0;
    virtual int32_t StartScreenRecording() = 0;
    virtual int32_t StopScreenRecording() = 0;
    virtual int32_t Release() = 0;
    virtual int32_t SetMicrophoneEnabled(bool isMicrophone) = 0;
    virtual int32_t SetCanvasRotation(bool canvasRotation) = 0;
    virtual int32_t AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audioBuffer, AudioCaptureSourceType type) = 0;
    virtual sptr<OHOS::SurfaceBuffer> AcquireVideoBuffer(int32_t &fence, int64_t &timestamp, OHOS::Rect &damage) = 0;
    virtual int32_t ReleaseAudioBuffer(AudioCaptureSourceType type) = 0;
    virtual int32_t ReleaseVideoBuffer() = 0;
    virtual bool IsErrorCallBackEnabled()
    {
        return false;
    }
    virtual bool IsDataCallBackEnabled()
    {
        return false;
    }
    virtual bool IsStateChangeCallBackEnabled()
    {
        return false;
    }
};

class __attribute__((visibility("default"))) ScreenCaptureMockFactory {
public:
    static std::shared_ptr<ScreenCaptureMock> CreateScreenCapture();
private:
    ScreenCaptureMockFactory() = delete;
    ~ScreenCaptureMockFactory() = delete;
};
} // namespace Media
} // namespace OHOS
#endif