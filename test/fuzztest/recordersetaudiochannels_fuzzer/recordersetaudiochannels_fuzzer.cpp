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

#include "recordersetaudiochannels_fuzzer.h"
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
RecorderSetAudioChannelsFuzzer::RecorderSetAudioChannelsFuzzer()
{
}
RecorderSetAudioChannelsFuzzer::~RecorderSetAudioChannelsFuzzer()
{
}
bool RecorderSetAudioChannelsFuzzer::FuzzRecorderSetAudioChannels(uint8_t *data, size_t size)
{
    bool retFlags = TestRecorder::CreateRecorder();
    FUZZTEST_CHECK(retFlags, false)

    static VideoRecorderConfig g_videoRecorderConfig;
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = MPEG4;
    g_videoRecorderConfig.outputFd = open("/data/test/media/recorder_video_yuv_mpeg4.mp4", O_RDWR);
    
    if (g_videoRecorderConfig.outputFd >= 0) {
        FUZZTEST_CHECK(TestRecorder::SetVideoSource(g_videoRecorderConfig), false)
        FUZZTEST_CHECK(TestRecorder::SetAudioSource(g_videoRecorderConfig), false)
        FUZZTEST_CHECK(TestRecorder::SetOutputFormat(g_videoRecorderConfig), false)
        FUZZTEST_CHECK(TestRecorder::CameraServicesForVideo(g_videoRecorderConfig), false)
        FUZZTEST_CHECK(TestRecorder::SetAudioEncoder(g_videoRecorderConfig), false)
        FUZZTEST_CHECK(TestRecorder::SetAudioSampleRate(g_videoRecorderConfig), false)

        g_videoRecorderConfig.audioSourceId = *reinterpret_cast<int32_t *>(data);
        g_videoRecorderConfig.channelCount =  ProduceRandomNumberCrypt();

        FUZZTEST_CHECK(TestRecorder::SetAudioChannels(g_videoRecorderConfig), true)
        FUZZTEST_CHECK(TestRecorder::SetAudioEncodingBitRate(g_videoRecorderConfig), true)
        FUZZTEST_CHECK(TestRecorder::SetMaxDuration(g_videoRecorderConfig), true)
        FUZZTEST_CHECK(TestRecorder::SetOutputFile(g_videoRecorderConfig), true)
        FUZZTEST_CHECK(TestRecorder::SetRecorderCallback(g_videoRecorderConfig), true)
        FUZZTEST_CHECK(TestRecorder::Prepare(g_videoRecorderConfig), true)
        FUZZTEST_CHECK(TestRecorder::RequesetBuffer(PURE_VIDEO, g_videoRecorderConfig), true)
        FUZZTEST_CHECK(TestRecorder::Start(g_videoRecorderConfig), true)
        sleep(RECORDER_TIME);
        FUZZTEST_CHECK(TestRecorder::Stop(false, g_videoRecorderConfig), true)
        StopBuffer(PURE_VIDEO);
        FUZZTEST_CHECK(TestRecorder::Reset(g_videoRecorderConfig), true)
        FUZZTEST_CHECK(TestRecorder::Release(g_videoRecorderConfig), true)
    }
    close(g_videoRecorderConfig.outputFd);
    return true;
}
}
bool FuzzTestRecorderSetAudioChannels(uint8_t *data, size_t size)
{
    RecorderSetAudioChannelsFuzzer testRecorder;
    return testRecorder.FuzzRecorderSetAudioChannels(data, size);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzTestRecorderSetAudioChannels(data, size);
    return 0;
}