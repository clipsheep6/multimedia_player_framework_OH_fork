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

#include "recorderstop_fuzzer.h"
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
RecorderStopFuzzer::RecorderStopFuzzer()
{
}
RecorderStopFuzzer::~RecorderStopFuzzer()
{
}
bool RecorderStopFuzzer::FuzzRecorderStop(uint8_t *data, size_t size)
{
    constexpr uint32_t recorderTime = 5;
    constexpr int32_t blockValueList = 2;
    RETURN_IF(TestRecorder::CreateRecorder(), false);

    static VideoRecorderConfig_ g_videoRecorderConfig;
    g_videoRecorderConfig.outputFd = open("/data/test/media/recorder_Stop.m4a", O_RDWR);

    if (g_videoRecorderConfig.outputFd > 0) {
        RETURN_IF(TestRecorder::SetAudioSource(g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::SetOutputFormat(g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::CameraServicesForAudio(g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::SetMaxDuration(g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::SetOutputFile(g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::SetRecorderCallback(g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::Prepare(g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::Start(g_videoRecorderConfig), false);
        sleep(recorderTime);

        bool BlockValue[blockValueList] {
            true,
            false,
        };
        int32_t sourceId = (*reinterpret_cast<int32_t *>(data)) % blockValueList;

        RETURN_IF(TestRecorder::Stop(BlockValue[sourceId], g_videoRecorderConfig), false);
        RETURN_IF(TestRecorder::Release(g_videoRecorderConfig), false);
    }
    close(g_videoRecorderConfig.outputFd);
    return true;
}
}
bool FuzzTestRecorderStop(uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return true;
    }

    if (size < sizeof(int32_t)) {
        return true;
    }
    RecorderStopFuzzer testRecorder;
    return testRecorder.FuzzRecorderStop(data, size);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzTestRecorderStop(data, size);
    return 0;
}

