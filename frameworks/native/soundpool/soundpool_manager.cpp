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
#include "media_errors.h"
#include "media_log.h"
#include "soundpool_manager.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SoundPoolManager"};
}

namespace OHOS {
namespace Media {
SoundPoolManager::~SoundPoolManager()
{
    MEDIA_LOGI("Destruction SoundPoolManager.");
    std::lock_guard<std::mutex> lock(mutex_);
    soundPools_.clear();
};


int32_t SoundPoolManager::GetSoundPool(const pid_t pid, std::shared_ptr<SoundPool>& soundPool)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = soundPools_.find(pid);
    it != soundPools_.end() ? soundPool = it->second : soundPool = nullptr;
    return MSERR_OK;
}

int32_t SoundPoolManager::SetSoundPool(const pid_t pid, std::shared_ptr<SoundPool> soundPool)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = soundPools_.find(pid);
    if (it != soundPools_.end()) {
        MEDIA_LOGI("SoundPool have setted, use old object.");
        return MSERR_OK;
    }
    soundPool = std::make_shared<SoundPool>();
    soundPools_.emplace(pid, soundPool);

    return MSERR_OK;
}

int32_t SoundPoolManager::Release(const pid_t pid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = soundPools_.find(pid);
    if (it != soundPools_.end()) {
        MEDIA_LOGI("Release soundpool, pid:%{pulibc}d.", pid);
        soundPools_.erase(it);
        return MSERR_OK;
    }
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
