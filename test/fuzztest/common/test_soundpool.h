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

#ifndef TEST_SOUNDPOOL_H
#define TEST_SOUNDPOOL_H

#include <fcntl.h>
#include <thread>
#include <cstdio>
#include <iostream>
#include "isoundpool.h"
#include "aw_common.h"
#include "media_errors.h"

namespace OHOS {
namespace Media {
#define RETURN_IF(cond, ret, ...)        \
do {                                     \
    if (!(cond)) {                       \
        return ret;                      \
    }                                    \
} while (0)

class TestSoundPool : public NoCopyable {
public:
    TestSoundPool();
    ~TestSoundPool();
    bool CreateSoundPool(int maxStreams, AudioStandard::AudioRendererInfo audioRenderInfo);
    int32_t Load(std::string url);
    int32_t Load(int32_t fd, int64_t offset, int64_t length);
    int32_t Play(int32_t soundID, PlayParams playParameters);
    int32_t Stop(int32_t streamID);
    int32_t SetLoop(int32_t streamID, int32_t loop);
    int32_t SetPriority(int32_t streamID, int32_t priority);
    int32_t SetRate(int32_t streamID, AudioStandard::AudioRendererRate renderRate);
    int32_t SetVolume(int32_t streamID, float leftVolume, float rigthVolume);
    int32_t Unload(int32_t soundID);
    int32_t Release();
    int32_t SetSoundPoolCallback(const std::shared_ptr<ISoundPoolCallback> &soundPoolCallback);
    size_t GetFileSize(const std::string& fileName);
    std::shared_ptr<ISoundPool> soundPool = nullptr;
};

class TestSoundPoolCallback : public ISoundPoolCallback, public NoCopyable {
public:
    TestSoundPoolCallback()
    {
    }
    ~TestSoundPoolCallback()
    {
    }
    std::shared_ptr<TestSoundPool> soundPool = nullptr;
    void OnError(int32_t errorCode) override;
    void OnLoadCompleted(int32_t soundId) override;
    void OnPlayFinished() override;
};
} // namespace Media
} // namespace OHOS
#endif