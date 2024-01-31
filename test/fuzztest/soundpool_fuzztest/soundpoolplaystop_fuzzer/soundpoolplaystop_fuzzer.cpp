/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <cmath>
#include <iostream>
#include "aw_common.h"
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "isoundpool.h"
#include "soundpoolplaystop_fuzzer.h"

using namespace std;
using namespace OHOS;
using namespace Media;
using namespace AudioStandard;

namespace OHOS {
namespace Media {
SoundPoolPlayStopFuzzer::SoundPoolPlayStopFuzzer()
{
}

SoundPoolPlayStopFuzzer::~SoundPoolPlayStopFuzzer()
{
}

bool SoundPoolPlayStopFuzzer::FuzzSoundPoolPlay(uint8_t *data, size_t size)
{
    int maxStreams = 3;
    AudioStandard::AudioRendererInfo audioRenderInfo;
    audioRenderInfo.contentType = CONTENT_TYPE_MUSIC;
    audioRenderInfo.streamUsage = STREAM_USAGE_MUSIC;
    audioRenderInfo.rendererFlags = 0;
    bool retFlags = TestSoundPool::CreateSoundPool(maxStreams, audioRenderInfo);
    RETURN_IF(retFlags, false);
    std::shared_ptr<TestSoundPoolCallback> cb = std::make_shared<TestSoundPoolCallback>();
    int32_t ret = TestSoundPool::SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }

    const string path = "/data/test/fuzztest_02.mp3";
    int32_t fd = 0;
    int32_t soundID = 0;
    fd = open(path.c_str(), O_RDWR);
    if (fd > 0) {
        std::string url = "fd://" + std::to_string(fd);
        soundID = TestSoundPool::Load(url);
    } else {
        cout << fd << " open failed, fileName " << path.c_str() << endl;
    }
    sleep(waitTime3);

    struct PlayParams playParameters;
    if (soundID > 0) {
        TestSoundPool::Play(soundID, playParameters);
        sleep(waitTime1);
    } else {
        cout << "Get soundId failed, please try to get soundId." << endl;
    }
    int32_t soundId = *reinterpret_cast<int32_t *>(data);
    TestSoundPool::Play(soundId, playParameters);
    sleep(waitTime1);
    playParameters.loop = *reinterpret_cast<int32_t *>(data);
    playParameters.rate = *reinterpret_cast<int32_t *>(data);
    playParameters.leftVolume = *reinterpret_cast<float *>(data);
    playParameters.rightVolume = *reinterpret_cast<float *>(data);
    playParameters.priority = *reinterpret_cast<int32_t *>(data);
    playParameters.parallelPlayFlag = *reinterpret_cast<bool *>(data);
    int32_t streamId =  TestSoundPool::Play(soundID, playParameters);
    sleep(waitTime1);
    TestSoundPool::Stop(streamId);
    TestSoundPool::Release();
    return true;
}

bool SoundPoolPlayStopFuzzer::FuzzSoundPoolStop(uint8_t *data, size_t size)
{
    int maxStreams = 3;
    AudioStandard::AudioRendererInfo audioRenderInfo;
    audioRenderInfo.contentType = CONTENT_TYPE_MUSIC;
    audioRenderInfo.streamUsage = STREAM_USAGE_MUSIC;
    audioRenderInfo.rendererFlags = 0;
    bool retFlags = TestSoundPool::CreateSoundPool(maxStreams, audioRenderInfo);
    RETURN_IF(retFlags, false);
    std::shared_ptr<TestSoundPoolCallback> cb = std::make_shared<TestSoundPoolCallback>();
    int32_t ret = TestSoundPool::SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }

    const string path = "/data/test/fuzztest_02.mp3";
    int32_t fd = 0;
    int32_t soundID = 0;
    fd = open(path.c_str(), O_RDWR);
    if (fd > 0) {
        std::string url = "fd://" + std::to_string(fd);
        soundID = TestSoundPool::Load(url);
    } else {
        cout << fd << " open failed, fileName " << path.c_str() << endl;
    }
    sleep(waitTime3);

    struct PlayParams playParameters;
    if (soundID > 0) {
        int32_t streamID = TestSoundPool::Play(soundID, playParameters);
        sleep(waitTime1);
        if (streamID > 0) {
            TestSoundPool::Stop(streamID);
        }
        streamID = *reinterpret_cast<int32_t *>(data);
        TestSoundPool::Stop(streamID);
    } else {
        cout << "Get soundId failed, please try to get soundId." << endl;
    }

    TestSoundPool::Release();
    return true;
}
} // namespace Media

bool FuzzTestSoundPoolPlay(uint8_t *data, size_t size)
{
    auto soundPool_ = std::make_unique<SoundPoolPlayStopFuzzer>();
    return soundPool_->FuzzSoundPoolPlay(data, size);
}
bool FuzzTestSoundPoolStop(uint8_t *data, size_t size)
{
    auto soundPool_ = std::make_unique<SoundPoolPlayStopFuzzer>();
    return soundPool_->FuzzSoundPoolStop(data, size);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzTestSoundPoolPlay(data, size);
    FuzzTestSoundPoolStop(data, size);
    return 0;
}