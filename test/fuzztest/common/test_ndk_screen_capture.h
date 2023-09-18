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

#ifndef TEST_NDK_SCREEN_CAPTURE_H
#define TEST_NDK_SCREEN_CAPTURE_H

#include <fcntl.h>
#include <thread>
#include <cstdio>
#include <iostream>
#include "nativetoken_kit.h"
#include "accesstoken_kit.h"
#include "token_setproc.h"
#include "native_avscreen_capture.h"
#include "native_avscreen_capture_base.h"
#include "native_avscreen_capture_errors.h"
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

class TestNdkScreenCapture : public NoCopyable {
public:
    TestNdkScreenCapture();
    ~TestNdkScreenCapture();
};

class TestScreenCaptureNdkCallback : public OH_AVScreenCaptureCallback, public NoCopyable {
public:
    TestScreenCaptureNdkCallback() {}
    ~TestScreenCaptureNdkCallback() = default;
    static void OnError(OH_AVScreenCapture *screenCapture, int32_t errorCode);
    static void OnAudioBufferAvailable(OH_AVScreenCapture *screenCapture, bool isReady, OH_AudioCaptureSourceType type);
    static void OnVideoBufferAvailable(OH_AVScreenCapture *screenCapture, bool isReady);
};
} // namespace Media
} // namespace OHOS
#endif