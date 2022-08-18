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

#include "recordersetaudiosource_fuzzer.h"
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
RecorderSetAudioSourceFuzzer::RecorderSetAudioSourceFuzzer()
{
}
RecorderSetAudioSourceFuzzer::~RecorderSetAudioSourceFuzzer()
{
}
bool RecorderSetAudioSourceFuzzer::FuzzRecorderSetAudioSource(uint8_t *data, size_t size)
{
    constexpr int32_t AUDIO_SOURCE_TYPES_LIST = 3;
    recorder = RecorderFactory::CreateRecorder();
    if (recorder == nullptr) {
        cout << "recorder is null" << endl;
        recorder->Release();
        return false;
    }

    static VideoRecorderConfig g_videoRecorderConfig;
    g_videoRecorderConfig.outputFd = open("/data/test/media/recorder_audio_es.m4a", O_RDWR);

    AudioSourceType AudioSourceType[AUDIO_SOURCE_TYPES_LIST] {
        AUDIO_SOURCE_INVALID,
        AUDIO_SOURCE_DEFAULT,
        AUDIO_MIC,
    };

    int32_t sourcesubscript = abs((ProduceRandomNumberCrypt()) % (AUDIO_SOURCE_TYPES_LIST));
    int32_t sourceId = *reinterpret_cast<int32_t *>(data);

    g_videoRecorderConfig.aSource = AudioSourceType[sourcesubscript];
    g_videoRecorderConfig.audioSourceId = sourceId;
    
    if (g_videoRecorderConfig.outputFd > 0) {
        int32_t retValue = SetConfig(PURE_AUDIO, g_videoRecorderConfig);
        FUZZTEST_CHECK(retValue != 0, "expect SetConfig fail!!!", true, 
            recorder->Release(), close(g_videoRecorderConfig.outputFd))
        
        std::shared_ptr<TestRecorderCallbackTest> cb = std::make_shared<TestRecorderCallbackTest>();
        
        retValue += recorder->SetRecorderCallback(cb);
        FUZZTEST_CHECK(retValue != 0, "SetRecorderCallback fail!!!", true, 
            recorder->Release(), close(g_videoRecorderConfig.outputFd))

        retValue = recorder->Prepare();
        FUZZTEST_CHECK(retValue != 0, "Prepare fail!!!", true, 
            recorder->Release(), close(g_videoRecorderConfig.outputFd))

        retValue = recorder->Start();
        FUZZTEST_CHECK(retValue != 0, "Start fail!!!", true, 
            recorder->Release(), close(g_videoRecorderConfig.outputFd))

        sleep(RECORDER_TIME);

        retValue = recorder->Stop(false);
        FUZZTEST_CHECK(retValue != 0, "Stop fail!!!", true, 
            recorder->Release(), close(g_videoRecorderConfig.outputFd))

        retValue = recorder->Release();
        FUZZTEST_CHECK(retValue != 0, "Release fail!!!", true, 
            cout << "over!" << endl, close(g_videoRecorderConfig.outputFd))
    }
    cout << "success!" << endl;
    close(g_videoRecorderConfig.outputFd);
    return true;
}
}
bool FuzzTestRecorderSetAudioSource(uint8_t *data, size_t size)
{
    RecorderSetAudioSourceFuzzer testRecorder;
    return testRecorder.FuzzRecorderSetAudioSource(data, size);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzTestRecorderSetAudioSource(data, size);
    return 0;
}

