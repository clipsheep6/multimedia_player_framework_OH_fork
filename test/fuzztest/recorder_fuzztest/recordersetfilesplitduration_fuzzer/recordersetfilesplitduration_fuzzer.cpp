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

#include "recordersetfilesplitduration_fuzzer.h"
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
RecorderSetFileSplitDurationFuzzer::RecorderSetFileSplitDurationFuzzer()
{
}

RecorderSetFileSplitDurationFuzzer::~RecorderSetFileSplitDurationFuzzer()
{
}

bool RecorderSetFileSplitDurationFuzzer::FuzzRecorderSetFileSplitDuration(uint8_t *data, size_t size)
{
    constexpr int32_t fileSplitTypeList = 4;
    constexpr int32_t audioFps = 30;
    constexpr int32_t audioMaxFileSize = 5000;
    constexpr int32_t durationValue = 1000;
    constexpr int32_t timestampValue = -1;
    recorder = RecorderFactory::CreateRecorder();
    RETURN_IF(TestRecorder::CreateRecorder(), false);

    static VideoRecorderConfig_ g_videoRecorderConfig;
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = MPEG4;
    g_videoRecorderConfig.outputFd = open("/data/test/media/recorder_SetFileSplitDuration.mp4", O_RDWR);

    if (g_videoRecorderConfig.outputFd > 0) {
        TestRecorder::SetVideoSource(g_videoRecorderConfig);
        TestRecorder::SetOutputFormat(g_videoRecorderConfig);
        TestRecorder::CameraServicesForVideo(g_videoRecorderConfig);
        TestRecorder::SetMaxDuration(g_videoRecorderConfig);
        TestRecorder::SetOutputFile(g_videoRecorderConfig);
        TestRecorder::SetRecorderCallback(g_videoRecorderConfig);
        recorder->SetCaptureRate(0, audioFps);
        TestRecorder::SetMaxFileSize(audioMaxFileSize, g_videoRecorderConfig);
        TestRecorder::SetNextOutputFile(g_videoRecorderConfig);

        FileSplitType fileSplitType[fileSplitTypeList] {
            FileSplitType::FILE_SPLIT_POST,
            FileSplitType::FILE_SPLIT_PRE,
            FileSplitType::FILE_SPLIT_NORMAL,
            FileSplitType::FILE_SPLIT_BUTT,
        };
        int32_t  indexValue = *reinterpret_cast<int32_t *>(data) % (fileSplitTypeList);

        TestRecorder::SetFileSplitDuration(fileSplitType[indexValue],
            timestampValue, durationValue, g_videoRecorderConfig);
    }
    close(g_videoRecorderConfig.outputFd);
    return true;
}
}

bool FuzzTestRecorderSetFileSplitDuration(uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return true;
    }

    if (size < sizeof(int32_t)) {
        return true;
    }
    RecorderSetFileSplitDurationFuzzer testRecorder;
    return testRecorder.FuzzRecorderSetFileSplitDuration(data, size);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzTestRecorderSetFileSplitDuration(data, size);
    return 0;
}