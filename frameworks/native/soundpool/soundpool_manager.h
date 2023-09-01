/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#ifndef SOUNDPOOL_MANAGER_H
#define SOUNDPOOL_MANAGER_H

#include <unistd.h>
#include <unordered_map>
#include "media_log.h"
#include "soundpool.h"

namespace OHOS {
namespace Media {
class SoundPool;

class SoundPoolManager {
public:
    SoundPoolManager(const SoundPoolManager&) = delete;
    SoundPoolManager& operator = (const SoundPoolManager&) = delete;
    ~SoundPoolManager();

    int32_t GetSoundPool(const pid_t pid, std::shared_ptr<SoundPool>& soundPool);
    int32_t SetSoundPool(const pid_t pid, std::shared_ptr<SoundPool> soundPool);
    int32_t Release(const pid_t pid);

    static SoundPoolManager& GetInstance()
    {
        static SoundPoolManager instance;
        return instance;
    };

private:
    SoundPoolManager() {}
    std::mutex mutex_;
    std::unordered_map<pid_t, std::shared_ptr<SoundPool>> soundPools_;
};
} // namespace Media
} // namespace OHOS
#endif // SOUNDPOOL_MANAGER_H
