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

#include "recordersetvideoencoder_fuzzer.h"
#include <iostream>
#include <cmath>
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
RecorderSetVideoEncoderFuzzer::RecorderSetVideoEncoderFuzzer()
{
}
RecorderSetVideoEncoderFuzzer::~RecorderSetVideoEncoderFuzzer()
{
}
bool RecorderSetVideoEncoderFuzzer::RecorderSetVideoEncoderFuzz(uint8_t *data, size_t size)
{
    constexpr int32_t videoCodecFormatList = 4;
    constexpr uint32_t recorderTime = 5;
    RETURN_IF(TestRecorder::CreateRecorder(), false);

    static VideoRecorderConfig_ g_videoRecorderConfig;
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = MPEG4;
    g_videoRecorderConfig.outputFd = open("/data/test/media/recorder_video_yuv_mpeg4.mp4", O_RDWR);
    
    if (g_videoRecorderConfig.outputFd >= 0) {
        RETURN_IF(TestRecorder::SetVideoSource(g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::SetOutputFormat(g_videoRecorderConfig), false);

        VideoCodecFormat videoCodecFormats[videoCodecFormatList] {
            VIDEO_DEFAULT,
            H264,
            MPEG4,
            VIDEO_CODEC_FORMAT_BUTT,
        };
        g_videoRecorderConfig.videoSourceId = *reinterpret_cast<int32_t *>(data);
        int32_t videoFormat = abs((ProduceRandomNumberCrypt()) % (videoCodecFormatList));
        g_videoRecorderConfig.videoFormat = videoCodecFormats[videoFormat];

        RETURN_IF(TestRecorder::SetVideoEncoder(g_videoRecorderConfig), true);
        RETURN_IF(TestRecorder::SetVideoSize(g_videoRecorderConfig), true);
        RETURN_IF(TestRecorder::SetVideoFrameRate(g_videoRecorderConfig), true);
        RETURN_IF(TestRecorder::SetVideoEncodingBitRate(g_videoRecorderConfig), true);
        RETURN_IF(TestRecorder::Prepare(g_videoRecorderConfig), true);
        RETURN_IF(TestRecorder::RequesetBuffer(PURE_VIDEO, g_videoRecorderConfig), true);
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
bool FuzzTestRecorderSetVideoEncoder(uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }

    if (size < sizeof(int32_t)) {
        return 0;
    }
    RecorderSetVideoEncoderFuzzer testRecorder;
    return testRecorder.RecorderSetVideoEncoderFuzz(data, size);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzTestRecorderSetVideoEncoder(data, size);
    return 0;
}

