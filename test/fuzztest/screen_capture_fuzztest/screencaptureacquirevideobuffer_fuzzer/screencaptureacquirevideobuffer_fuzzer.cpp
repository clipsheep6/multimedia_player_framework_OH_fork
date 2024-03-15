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
#include "screen_capture.h"
#include "screencaptureacquirevideobuffer_fuzzer.h"

using namespace std;
using namespace OHOS;
using namespace Media;

namespace OHOS {
namespace Media {
ScreenCaptureAcquireVideoBufferFuzzer::ScreenCaptureAcquireVideoBufferFuzzer()
{
}

ScreenCaptureAcquireVideoBufferFuzzer::~ScreenCaptureAcquireVideoBufferFuzzer()
{
}

namespace {
    const uint8_t* g_data = nullptr;
    size_t g_size = 0;
    size_t g_pos;
}

void SetConfig(AVScreenCaptureConfig &config)
{
    AudioCaptureInfo miccapinfo = {
        .audioSampleRate = 48000,
        .audioChannels = 2,
        .audioSource = SOURCE_DEFAULT
    };

    VideoCaptureInfo videocapinfo = {
        .videoFrameWidth = 720,
        .videoFrameHeight = 1280,
        .videoSource = VIDEO_SOURCE_SURFACE_RGBA
    };

    AudioInfo audioinfo = {
        .micCapInfo = miccapinfo,
    };

    VideoInfo videoinfo = {
        .videoCapInfo = videocapinfo
    };

    config = {
        .captureMode = CAPTURE_HOME_SCREEN,
        .dataType = ORIGINAL_STREAM,
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

bool ScreenCaptureAcquireVideoBufferFuzzer::FuzzScreenCaptureAcquireVideoBuffer(uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    bool retFlags = TestScreenCapture::CreateScreenCapture();
    RETURN_IF(retFlags, false);

    AVScreenCaptureConfig config;
    SetConfig(config);
    g_data = data;
    g_size = size;
    g_pos = 0;

    int32_t fence = GetData<int32_t>();
    int64_t timestamp = GetData<int64_t>();
    OHOS::Rect damage = GetData<OHOS::Rect>();

    TestScreenCapture::SetMicrophoneEnabled(true);
    TestScreenCapture::SetCanvasRotation(true);
    TestScreenCapture::Init(config);
    TestScreenCapture::StartScreenCapture();
    TestScreenCapture::AcquireVideoBuffer(fence, timestamp, damage);
    TestScreenCapture::ReleaseVideoBuffer();
    TestScreenCapture::StopScreenCapture();
    TestScreenCapture::Release();
    return true;
}
} // namespace Media

bool FuzzTestScreenCaptureAcquireVideoBuffer(uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return true;
    }

    ScreenCaptureAcquireVideoBufferFuzzer testScreenCapture;
    return testScreenCapture.FuzzScreenCaptureAcquireVideoBuffer(data, size);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzTestScreenCaptureAcquireVideoBuffer(data, size);
    return 0;
}