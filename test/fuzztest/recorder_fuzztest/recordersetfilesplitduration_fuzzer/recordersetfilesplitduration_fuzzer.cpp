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
    recorder = RecorderFactory::CreateRecorder();
    RETURN_IF(TestRecorder::CreateRecorder(), false);

    static VideoRecorderConfig_ g_videoRecorderConfig;
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = MPEG4;
    g_videoRecorderConfig.outputFd = open("/data/test/media/recorder_video_SetMaxFileSize_001.mp4", O_RDWR);

    if (g_videoRecorderConfig.outputFd > 0) {
        RETURN_IF(TestRecorder::SetVideoSource(g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::SetOutputFormat(g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::CameraServicesForVideo(g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::SetMaxDuration(g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::SetOutputFile(g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::SetRecorderCallback(g_videoRecorderConfig), false);
        recorder->SetCaptureRate(0, audioFps);
        RETURN_IF(TestRecorder::SetMaxFileSize(audioMaxFileSize, g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::SetNextOutputFile(g_videoRecorderConfig), false);

        FileSplitType fileSplitType[fileSplitTypeList] {
            FileSplitType::FILE_SPLIT_POST,
            FileSplitType::FILE_SPLIT_PRE,
            FileSplitType::FILE_SPLIT_NORMAL,
            FileSplitType::FILE_SPLIT_BUTT,
        };
        int32_t  indexValue = ProduceRandomNumberCrypt() % (fileSplitTypeList);
        int64_t timestampValue = *reinterpret_cast<int32_t *>(data);
        uint32_t durationValue = static_cast<uint32_t>(ProduceRandomNumberCrypt());

        RETURN_IF(TestRecorder::SetFileSplitDuration(fileSplitType[indexValue],
            timestampValue, durationValue, g_videoRecorderConfig), true);
    }
    close(g_videoRecorderConfig.outputFd);
    return false;
}
}

bool FuzzTestRecorderSetFileSplitDuration(uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }

    if (size < sizeof(int32_t)) {
        return 0;
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