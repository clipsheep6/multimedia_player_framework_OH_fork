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

#ifndef SCREENCAPTUREAUDIOSAMPLERATENDK_FUZZER
#define SCREENCAPTUREAUDIOSAMPLERATENDK_FUZZER

#include <fcntl.h>
#include <securec.h>
#include <unistd.h>
#include <cstdint>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include "test_ndk_screen_capture.h"

#define FUZZ_PROJECT_NAME "screencaptureaudiosamplerate_ndk_fuzzer"

namespace OHOS {
namespace Media {
class ScreenCaptureAudioSampleRateNdkFuzzer : public TestNdkScreenCapture {
public:
    ScreenCaptureAudioSampleRateNdkFuzzer();
    ~ScreenCaptureAudioSampleRateNdkFuzzer();
    bool FuzzScreenCaptureAudioSampleRateNdk(uint8_t *data, size_t size);
    OH_AVScreenCapture* screenCapture = nullptr;
    std::shared_ptr<TestScreenCaptureNdkCallback> screenCaptureCb = nullptr;
};
} // namespace Media
bool FuzzTestScreenCaptureAudioSampleRateNdk(uint8_t *data, size_t size);
} // namespace OHOS
#endif