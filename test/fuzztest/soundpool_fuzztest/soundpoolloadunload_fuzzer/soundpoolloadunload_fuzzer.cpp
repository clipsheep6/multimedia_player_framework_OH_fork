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
#include "soundpoolloadunload_fuzzer.h"

using namespace std;
using namespace OHOS;
using namespace Media;
using namespace AudioStandard;

namespace OHOS {
namespace Media {
SoundPoolLoadUnloadFuzzer::SoundPoolLoadUnloadFuzzer()
{
}

SoundPoolLoadUnloadFuzzer::~SoundPoolLoadUnloadFuzzer()
{
}

bool SoundPoolLoadUnloadFuzzer::FuzzSoundPoolLoad(uint8_t *data, size_t size)
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
        cout << "FuzzSoundPoolLoad set callback failed" << endl;
    }

    const string path = "/data/test/fuzztest_02.mp3";
    int32_t fd = 0;
    fd = open(path.c_str(), O_RDWR);
    if (fd > 0) {
        std::string url = "fd://" + std::to_string(fd);
        TestSoundPool::Load(url);
    } else {
        cout << "FuzzSoundPoolLoad Url open " << path.c_str() << " fail" << endl;
    }
    fd = open(path.c_str(), O_RDONLY);
    if (fd > 0) {
        size_t filesize = TestSoundPool::GetFileSize(path);
        TestSoundPool::Load(fd, 0, filesize);
    } else {
        cout << "FuzzSoundPoolLoad Fd open " << path.c_str() << " fail" << endl;
    }
    sleep(waitTime3);

    if (size >= sizeof(int32_t)) {
        fd = *reinterpret_cast<int32_t *>(data);
        std::string url = "fd://" + std::to_string(fd);
        TestSoundPool::Load(url);
        size_t filesize = TestSoundPool::GetFileSize(path);
        TestSoundPool::Load(fd, 0, filesize);
    }
    TestSoundPool::Release();
    return true;
}

bool SoundPoolLoadUnloadFuzzer::FuzzSoundPoolUnload(uint8_t *data, size_t size)
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
        cout << "FuzzSoundPoolUnload set callback failed" << endl;
    }

    const string path = "/data/test/fuzztest_02.mp3";
    int32_t fd = 0;
    int32_t urlSoundid = 0;
    int32_t fdSoundid = 0;
    fd = open(path.c_str(), O_RDWR);
    if (fd > 0) {
        std::string url = "fd://" + std::to_string(fd);
        urlSoundid = TestSoundPool::Load(url);
    } else {
        cout << "FuzzSoundPoolUnload Url open" << path.c_str() << " fail" << endl;
    }
    fd = open(path.c_str(), O_RDONLY);
    if (fd > 0) {
        size_t filesize = TestSoundPool::GetFileSize(path);
        fdSoundid = TestSoundPool::Load(fd, 0, filesize);
    } else {
        cout << "FuzzSoundPoolUnload Fd open " << path.c_str() << " fail" << endl;
    }
    sleep(waitTime3);

    TestSoundPool::Unload(urlSoundid);
    TestSoundPool::Unload(fdSoundid);
    if (size >= sizeof(int32_t)) {
        int32_t soundID = *reinterpret_cast<int32_t *>(data);
        TestSoundPool::Unload(soundID);
    }
    TestSoundPool::Release();
    return true;
}
} // namespace Media

bool FuzzTestSoundPoolLoad(uint8_t *data, size_t size)
{
    auto soundPool_ = std::make_unique<SoundPoolLoadUnloadFuzzer>();
    return soundPool_->FuzzSoundPoolLoad(data, size);
}

bool FuzzTestSoundPoolUnload(uint8_t *data, size_t size)
{
    auto soundPool_ = std::make_unique<SoundPoolLoadUnloadFuzzer>();
    return soundPool_->FuzzSoundPoolUnload(data, size);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzTestSoundPoolLoad(data, size);
    FuzzTestSoundPoolUnload(data, size);
    return 0;
}