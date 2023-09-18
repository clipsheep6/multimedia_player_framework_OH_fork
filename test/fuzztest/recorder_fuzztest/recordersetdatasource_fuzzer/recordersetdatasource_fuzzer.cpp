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

#include "recordersetdatasource_fuzzer.h"
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
RecorderSetDataSourceFuzzer::RecorderSetDataSourceFuzzer()
{
}

RecorderSetDataSourceFuzzer::~RecorderSetDataSourceFuzzer()
{
}

bool RecorderSetDataSourceFuzzer::FuzzRecorderSetDataSource(uint8_t *data, size_t size)
{
    constexpr uint32_t recorderTime = 5;
    constexpr int32_t videoSourceTypeList = 3;
    RETURN_IF(TestRecorder::CreateRecorder(), false);

    static VideoRecorderConfig_ g_videoRecorderConfig;
    const VideoSourceType VideoSourceTypes[videoSourceTypeList] {
        VideoSourceType::VIDEO_SOURCE_SURFACE_YUV,
        VideoSourceType::VIDEO_SOURCE_SURFACE_ES,
        VideoSourceType::VIDEO_SOURCE_BUTT
    };
    int32_t sourcesubscript = abs(ProduceRandomNumberCrypt()) % (videoSourceTypeList);
    g_videoRecorderConfig.vSource = VideoSourceTypes[sourcesubscript];
    g_videoRecorderConfig.videoSourceId = *reinterpret_cast<int32_t *>(data);
    g_videoRecorderConfig.videoFormat = MPEG4;
    g_videoRecorderConfig.outputFd = open("/data/test/media/recorder_SetDataSource.mp4", O_RDWR);

    if (g_videoRecorderConfig.outputFd >= 0) {
        TestRecorder::SetDataSource(g_videoRecorderConfig);
        TestRecorder::SetVideoSource(g_videoRecorderConfig);
        TestRecorder::SetOutputFormat(g_videoRecorderConfig);
        TestRecorder::CameraServicesForVideo(g_videoRecorderConfig);
        TestRecorder::SetMaxDuration(g_videoRecorderConfig);
        TestRecorder::SetOutputFile(g_videoRecorderConfig) ;
        TestRecorder::SetRecorderCallback(g_videoRecorderConfig);
        TestRecorder::Prepare(g_videoRecorderConfig);
        RETURN_IF(TestRecorder::RequesetBuffer(PURE_VIDEO, g_videoRecorderConfig), false);
        TestRecorder::Start(g_videoRecorderConfig);
        sleep(recorderTime);
        TestRecorder::Pause(g_videoRecorderConfig);
        sleep(recorderTime);
        TestRecorder::Resume(g_videoRecorderConfig);
        sleep(recorderTime);
        TestRecorder::Stop(false, g_videoRecorderConfig);
        StopBuffer(PURE_VIDEO);
        TestRecorder::Reset(g_videoRecorderConfig);
        TestRecorder::Release(g_videoRecorderConfig);
    }
    close(g_videoRecorderConfig.outputFd);
    return true;
}
}

bool FuzzTestRecorderSetSource(uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return true;
    }

    if (size < sizeof(int32_t)) {
        return true;
    }
    RecorderSetDataSourceFuzzer testrecorder;
    return testrecorder.FuzzRecorderSetDataSource(data, size);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzTestRecorderSetSource(data, size);
    return 0;
}

