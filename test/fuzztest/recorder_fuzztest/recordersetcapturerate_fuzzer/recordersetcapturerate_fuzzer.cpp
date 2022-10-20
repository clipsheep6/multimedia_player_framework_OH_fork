/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "recordersetcapturerate_fuzzer.h"
#include <iostream>
#include "aw_common.h"
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "recorder.h"

using namespace std;
using namespace OHOS;
using namespace Media;
using namespace PlayerTestParam;
using namespace RecorderTestParam;

namespace OHOS {
namespace Media {
RecorderSetCaptureRateFuzzer::RecorderSetCaptureRateFuzzer()
{
}

RecorderSetCaptureRateFuzzer::~RecorderSetCaptureRateFuzzer()
{
}

bool RecorderSetCaptureRateFuzzer::FuzzRecorderSetCaptureRate(uint8_t *data, size_t size)
{
    constexpr int32_t audioMaxFileSize = 5000;
    RETURN_IF(TestRecorder::CreateRecorder(), false);

    static VideoRecorderConfig_ g_videoRecorderConfig;
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = MPEG4;
    g_videoRecorderConfig.outputFd = open("/data/test/media/recorder_SetCaptureRate.mp4", O_RDWR);

    if (g_videoRecorderConfig.outputFd > 0) {
        TestRecorder::SetVideoSource(g_videoRecorderConfig);
        TestRecorder::SetOutputFormat(g_videoRecorderConfig);
        TestRecorder::CameraServicesForVideo(g_videoRecorderConfig);
        TestRecorder::SetCaptureRate(g_videoRecorderConfig, *reinterpret_cast<double *>(data));
        TestRecorder::SetMaxFileSize(audioMaxFileSize, g_videoRecorderConfig);
        TestRecorder::SetNextOutputFile(g_videoRecorderConfig);
    }
    close(g_videoRecorderConfig.outputFd);
    return true;
}
}

bool FuzzTestRecorderSetCaptureRate(uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return true;
    }

    if (size < sizeof(double)) {
        return true;
    }
    RecorderSetCaptureRateFuzzer testRecorder;
    return testRecorder.FuzzRecorderSetCaptureRate(data, size);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzTestRecorderSetCaptureRate(data, size);
    return 0;
}

