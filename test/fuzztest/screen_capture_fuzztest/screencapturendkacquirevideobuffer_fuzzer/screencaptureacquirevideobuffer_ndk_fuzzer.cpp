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
#include "screencaptureacquirevideobuffer_ndk_fuzzer.h"

using namespace std;
using namespace OHOS;
using namespace Media;

namespace OHOS {
namespace Media {
ScreenCaptureAcquireVideoBufferNdkFuzzer::ScreenCaptureAcquireVideoBufferNdkFuzzer()
{
}

ScreenCaptureAcquireVideoBufferNdkFuzzer::~ScreenCaptureAcquireVideoBufferNdkFuzzer()
{
}

namespace {
    const uint8_t* g_data = nullptr;
    size_t g_size = 0;
    size_t g_pos;
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

template<class T>
T GetData()
{
    T object {};
    size_t objectSize = sizeof(object);
    if (g_data == nullptr || objectSize > g_size - g_pos) {
        return object;
    }
    errno_t ret = memcpy_s(&object, objectSize, g_data + g_pos, objectSize);
    if (ret != EOK) {
        return {};
    }
    g_pos += objectSize;
    return object;
}

bool ScreenCaptureAcquireVideoBufferNdkFuzzer::FuzzScreenCaptureAcquireVideoBufferNdk(uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    screenCapture = OH_AVScreenCapture_Create();

    OH_AVScreenCaptureConfig config;
    SetConfig(config);
    g_data = data;
    g_size = size;
    g_pos = 0;

    int32_t fence = GetData<int32_t>();
    int64_t timestamp = GetData<int64_t>();
    OH_Rect damage = GetData<OH_Rect>();

    OH_AVScreenCapture_SetMicrophoneEnabled(screenCapture, true);
    OH_AVScreenCapture_SetCanvasRotation(screenCapture, true);
    OH_AVScreenCapture_Init(screenCapture, config);
    OH_AVScreenCapture_StartScreenCapture(screenCapture);
    OH_NativeBuffer *nativeBuffer = OH_AVScreenCapture_AcquireVideoBuffer(screenCapture, &fence, &timestamp, &damage);
    if (nativeBuffer != nullptr) {
        OH_AVScreenCapture_ReleaseVideoBuffer(screenCapture);
    }
    OH_AVScreenCapture_StopScreenCapture(screenCapture);
    OH_AVScreenCapture_Release(screenCapture);
    return true;
}
} // namespace Media

bool FuzzTestScreenCaptureAcquireVideoBufferNdk(uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return true;
    }

    ScreenCaptureAcquireVideoBufferNdkFuzzer testScreenCapture;
    return testScreenCapture.FuzzScreenCaptureAcquireVideoBufferNdk(data, size);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzTestScreenCaptureAcquireVideoBufferNdk(data, size);
    return 0;
}