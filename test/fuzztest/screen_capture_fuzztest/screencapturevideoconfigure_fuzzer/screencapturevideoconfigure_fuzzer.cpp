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
#include "screencapturevideoconfigure_fuzzer.h"

using namespace std;
using namespace OHOS;
using namespace Media;

namespace OHOS {
namespace Media {
ScreenCaptureVideoConfigureFuzzer::ScreenCaptureVideoConfigureFuzzer()
{
}

ScreenCaptureVideoConfigureFuzzer::~ScreenCaptureVideoConfigureFuzzer()
{
}

template<class T>
size_t GetObject(T &object, const uint8_t *data, size_t size)
{
    size_t objectSize = sizeof(object);
    if (objectSize > size) {
        return 0;
    }
    return memcpy_s(&object, objectSize, data, objectSize) == EOK ? objectSize : 0;
}

bool ScreenCaptureVideoConfigureFuzzer::FuzzScreenCaptureVideoConfigure(uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(VideoCaptureInfo)) {
        return false;
    }
    size_t startPos = 0;
    bool retFlags = TestScreenCapture::CreateScreenCapture();
    RETURN_IF(retFlags, false);

    AVScreenCaptureConfig config;
    constexpr int32_t videoSourceTypeList = 4;
    constexpr uint32_t recorderTime = 3;
    startPos += GetObject<int32_t>(config.videoInfo.videoCapInfo.videoFrameWidth, data + startPos, size - startPos);
    startPos += GetObject<int32_t>(config.videoInfo.videoCapInfo.videoFrameHeight, data + startPos, size - startPos);
    VideoSourceType videoSourceType[videoSourceTypeList] {
        VIDEO_SOURCE_SURFACE_YUV,
        VIDEO_SOURCE_SURFACE_ES,
        VIDEO_SOURCE_SURFACE_RGBA,
        VIDEO_SOURCE_BUTT
    };
    int32_t vsourcesubscript = *reinterpret_cast<int32_t *>(data) % (videoSourceTypeList);
    config.videoInfo.videoCapInfo.videoSource = videoSourceType[vsourcesubscript];

    std::shared_ptr<TestScreenCaptureCallbackTest> callbackobj
        = std::make_shared<TestScreenCaptureCallbackTest>(screenCapture);
    TestScreenCapture::SetMicrophoneEnabled(true);
    TestScreenCapture::SetScreenCaptureCallback(callbackobj);
    TestScreenCapture::Init(config);
    TestScreenCapture::StartScreenCapture();
    sleep(recorderTime);
    TestScreenCapture::StopScreenCapture();
    TestScreenCapture::Release();
    return true;
}
} // namespace Media

bool FuzzTestScreenCaptureVideoConfigure(uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return true;
    }

    if (size < sizeof(VideoCaptureInfo)) {
        return true;
    }
    ScreenCaptureVideoConfigureFuzzer testScreenCapture;
    return testScreenCapture.FuzzScreenCaptureVideoConfigure(data, size);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzTestScreenCaptureVideoConfigure(data, size);
    return 0;
}