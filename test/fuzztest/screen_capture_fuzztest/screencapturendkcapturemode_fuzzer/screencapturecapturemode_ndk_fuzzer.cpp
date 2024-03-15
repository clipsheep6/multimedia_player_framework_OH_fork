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

#include <cmath>
#include <cstdlib>
#include <iostream>
#include "aw_common.h"
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "screencapturecapturemode_ndk_fuzzer.h"

using namespace std;
using namespace OHOS;
using namespace Media;

namespace OHOS {
namespace Media {
ScreenCaptureCaptureModeNdkFuzzer::ScreenCaptureCaptureModeNdkFuzzer()
{
}

ScreenCaptureCaptureModeNdkFuzzer::~ScreenCaptureCaptureModeNdkFuzzer()
{
}

void SetConfig(OH_AVScreenCaptureConfig &config)
{
    OH_AudioCaptureInfo miccapinfo = {
        .audioSampleRate = 48000,
        .audioChannels = 2,
        .audioSource = OH_SOURCE_DEFAULT
    };

    OH_VideoCaptureInfo videocapinfo = {
        .videoFrameWidth = 720,
        .videoFrameHeight = 1280,
        .videoSource = OH_VIDEO_SOURCE_SURFACE_RGBA
    };

    OH_AudioInfo audioinfo = {
        .micCapInfo = miccapinfo
    };

    OH_VideoInfo videoinfo = {
        .videoCapInfo = videocapinfo
    };

    config = {
        .captureMode = OH_CAPTURE_HOME_SCREEN,
        .dataType = OH_ORIGINAL_STREAM,
        .audioInfo = audioinfo,
        .videoInfo = videoinfo,
    };
}

bool ScreenCaptureCaptureModeNdkFuzzer::FuzzScreenCaptureCaptureModeNdk(uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(OH_CaptureMode)) {
        return false;
    }
    screenCapture = OH_AVScreenCapture_Create();

    OH_AVScreenCaptureConfig config;
    SetConfig(config);
    constexpr int32_t captureModeList = 4;
    constexpr uint32_t recorderTime = 3;
    const OH_CaptureMode captureMode_[captureModeList] {
        OH_CAPTURE_HOME_SCREEN,
        OH_CAPTURE_SPECIFIED_SCREEN,
        OH_CAPTURE_SPECIFIED_WINDOW,
        OH_CAPTURE_INVAILD
    };
    int32_t capturemodesubscript = abs(*reinterpret_cast<int32_t *>(data) % (captureModeList));
    config.captureMode = captureMode_[capturemodesubscript];

    OH_AVScreenCapture_SetMicrophoneEnabled(screenCapture, true);
    OH_AVScreenCapture_SetCanvasRotation(screenCapture, true);
    OH_AVScreenCaptureCallback callback;
    callback.onError = TestScreenCaptureNdkCallback::OnError;
    callback.onAudioBufferAvailable = TestScreenCaptureNdkCallback::OnAudioBufferAvailable;
    callback.onVideoBufferAvailable = TestScreenCaptureNdkCallback::OnVideoBufferAvailable;
    OH_AVScreenCapture_SetCallback(screenCapture, callback);
    OH_AVScreenCapture_Init(screenCapture, config);
    OH_AVScreenCapture_StartScreenCapture(screenCapture);
    sleep(recorderTime);
    OH_AVScreenCapture_StopScreenCapture(screenCapture);
    OH_AVScreenCapture_Release(screenCapture);
    return true;
}
} // namespace Media

bool FuzzTestScreenCaptureCaptureModeNdk(uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return true;
    }

    if (size < sizeof(OH_CaptureMode)) {
        return true;
    }
    ScreenCaptureCaptureModeNdkFuzzer testScreenCapture;
    return testScreenCapture.FuzzScreenCaptureCaptureModeNdk(data, size);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzTestScreenCaptureCaptureModeNdk(data, size);
    return 0;
}