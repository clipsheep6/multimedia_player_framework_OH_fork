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

#include "recordersetaudioencoder_fuzzer.h"
#include <cmath>
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
RecorderSetAudioEncoderFuzzer::RecorderSetAudioEncoderFuzzer()
{
}
RecorderSetAudioEncoderFuzzer::~RecorderSetAudioEncoderFuzzer()
{
}
bool RecorderSetAudioEncoderFuzzer::FuzzRecorderSetAudioEncoder(uint8_t *data, size_t size)
{
    constexpr int32_t audioCodecFormatList = 3;
    constexpr uint32_t recorderTime = 5;
    RETURN_IF(TestRecorder::CreateRecorder(), false);

    static VideoRecorderConfig_ g_videoRecorderConfig;
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = MPEG4;
    g_videoRecorderConfig.outputFd = open("/data/test/media/recorder_video_yuv_mpeg4.mp4", O_RDWR);
    
    if (g_videoRecorderConfig.outputFd >= 0) {
        RETURN_IF(TestRecorder::SetVideoSource(g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::SetAudioSource(g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::SetOutputFormat(g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::SetVideoEncoder(g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::SetVideoSize(g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::SetVideoFrameRate(g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::SetVideoEncodingBitRate(g_videoRecorderConfig), false);

        AudioCodecFormat audioCodecFormat[audioCodecFormatList] {
            AUDIO_DEFAULT,
            AAC_LC,
            AUDIO_CODEC_FORMAT_BUTT,
        };
        int32_t audioFormat = abs((ProduceRandomNumberCrypt()) % (AUDIO_CODEC_FORMAT_LIST));
        g_videoRecorderConfig.audioSourceId = *reinterpret_cast<int32_t *>(data);
        g_videoRecorderConfig.audioFormat = audioCodecFormat[audioFormat];

        RETURN_IF(TestRecorder::SetAudioEncoder(g_videoRecorderConfig), true);
        RETURN_IF(TestRecorder::SetAudioSampleRate(g_videoRecorderConfig), true);
        RETURN_IF(TestRecorder::SetAudioChannels(g_videoRecorderConfig), true);
        RETURN_IF(TestRecorder::SetAudioEncodingBitRate(g_videoRecorderConfig), true);
        RETURN_IF(TestRecorder::SetMaxDuration(g_videoRecorderConfig), true);
        RETURN_IF(TestRecorder::SetOutputFile(g_videoRecorderConfig), true);
        RETURN_IF(TestRecorder::SetRecorderCallback(g_videoRecorderConfig), true);
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
bool FuzzTestRecorderSetAudioEncoder(uint8_t *data, size_t size)
{
    RecorderSetAudioEncoderFuzzer testRecorder;
    return testRecorder.FuzzRecorderSetAudioEncoder(data, size);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzTestRecorderSetAudioEncoder(data, size);
    return 0;
}