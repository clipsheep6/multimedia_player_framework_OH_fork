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

#include "recordergetsurface_fuzzer.h"
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
RecorderGetSurfaceFuzzer::RecorderGetSurfaceFuzzer()
{
}
RecorderGetSurfaceFuzzer::~RecorderGetSurfaceFuzzer()
{
}
bool RecorderGetSurfaceFuzzer::FuzzRecorderGetSurface(uint8_t *data, size_t size)
{
    constexpr uint32_t recorderTime = 5;
    RETURN_IF(TestRecorder::CreateRecorder(), false);

    static VideoRecorderConfig_ g_videoRecorderConfig;
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = MPEG4;
    g_videoRecorderConfig.outputFd = open("/data/test/media/recorder_video_yuv_mpeg4.mp4", O_RDWR);

    if (g_videoRecorderConfig.outputFd >= 0) {
        RETURN_IF(TestRecorder::SetConfig(PURE_VIDEO, g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::Prepare(g_videoRecorderConfig), false);

        g_videoRecorderConfig.videoSourceId = *reinterpret_cast<int32_t *>(data);

        RETURN_IF(TestRecorder::GetSurface(g_videoRecorderConfig), true);

        if (g_videoRecorderConfig.vSource == VIDEO_SOURCE_SURFACE_ES) {
            int32_t retValue = GetStubFile();
            if (retValue != 0) {
                return true;
            }
            camereHDIThread.reset(new(std::nothrow) std::thread(&TestRecorder::HDICreateESBuffer, this));
        } else {
            camereHDIThread.reset(new(std::nothrow) std::thread(&TestRecorder::HDICreateYUVBuffer, this));
        }

        RETURN_IF(TestRecorder::Start(g_videoRecorderConfig), true);
        sleep(recorderTime);
        RETURN_IF(TestRecorder::Stop(false, g_videoRecorderConfig), true);
        StopBuffer(PURE_VIDEO);
        RETURN_IF(TestRecorder::Reset(g_videoRecorderConfig), true);
        RETURN_IF(TestRecorder::Release(g_videoRecorderConfig), true);
    }
    close(g_videoRecorderConfig.outputFd);
    return false;
}
}
bool FuzzTestRecorderGetSurface(uint8_t *data, size_t size)
{
    RecorderGetSurfaceFuzzer testRecorder;
    return testRecorder.FuzzRecorderGetSurface(data, size);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzTestRecorderGetSurface(data, size);
    return 0;
}

