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
#include <iostream>
#include "aw_common.h"
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "screencaptureacquireaudiobuffer_ndk_fuzzer.h"

using namespace std;
using namespace OHOS;
using namespace Media;

namespace OHOS {
namespace Media {
ScreenCaptureAcquireAudioBufferNdkFuzzer::ScreenCaptureAcquireAudioBufferNdkFuzzer()
{
}

ScreenCaptureAcquireAudioBufferNdkFuzzer::~ScreenCaptureAcquireAudioBufferNdkFuzzer()
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

bool ScreenCaptureAcquireAudioBufferNdkFuzzer::FuzzScreenCaptureAcquireAudioBufferNdk(uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    screenCapture = OH_AVScreenCapture_Create();

    OH_AVScreenCaptureConfig config;
    SetConfig(config);
    constexpr int32_t audioSourceTypesList = 5;
    OH_AudioBuffer *audioBuffer = static_cast<OH_AudioBuffer*>(malloc(sizeof(OH_AudioBuffer)));
    if (audioBuffer == nullptr) {
        cout << "audio buffer is nullptr" << endl;
        return false;
    }
    const OH_AudioCaptureSourceType audioSourceType[audioSourceTypesList] {
        OH_SOURCE_INVALID,
        OH_SOURCE_DEFAULT,
        OH_MIC,
        OH_ALL_PLAYBACK,
        OH_APP_PLAYBACK,
    };
    int32_t asourcesubscript = (static_cast<int32_t>(*data)) % (audioSourceTypesList);
    OH_AudioCaptureSourceType type = audioSourceType[asourcesubscript];

    OH_AVScreenCapture_SetMicrophoneEnabled(screenCapture, true);
    OH_AVScreenCapture_Init(screenCapture, config);
    OH_AVScreenCapture_StartScreenCapture(screenCapture);
    OH_AVScreenCapture_AcquireAudioBuffer(screenCapture, &audioBuffer, type);
    OH_AVScreenCapture_ReleaseAudioBuffer(screenCapture, type);
    OH_AVScreenCapture_StopScreenCapture(screenCapture);
    OH_AVScreenCapture_Release(screenCapture);
    return true;
}
} // namespace Media

bool FuzzTestScreenCaptureAcquireAudioBufferNdk(uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return true;
    }

    ScreenCaptureAcquireAudioBufferNdkFuzzer testScreenCapture;
    return testScreenCapture.FuzzScreenCaptureAcquireAudioBufferNdk(data, size);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzTestScreenCaptureAcquireAudioBufferNdk(data, size);
    return 0;
}