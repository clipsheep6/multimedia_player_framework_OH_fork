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

#ifndef SYSTEM_SOUND_MANAGER_IMPL_H
#define SYSTEM_SOUND_MANAGER_IMPL_H

#include <array>

#include "data_ability_helper.h"
#include "foundation/ability/ability_runtime/interfaces/kits/native/appkit/ability_runtime/context/context.h"
#include "uri.h"
#include "want.h"

#include "audio_system_manager.h"
#include "media_log.h"
#include "player.h"

#include "system_sound_manager.h"

namespace OHOS {
namespace Media {
class SystemSoundManagerImpl : public SystemSoundManager {
public:
    SystemSoundManagerImpl();
    ~SystemSoundManagerImpl();

    // SystemSoundManager override
    int32_t SetRingtoneUri(const std::shared_ptr<AbilityRuntime::Context> &ctx, const std::string &uri,
        RingtoneType type) override;
    std::string GetRingtoneUri(const std::shared_ptr<AbilityRuntime::Context> &ctx, RingtoneType type) override;
    std::shared_ptr<RingtonePlayer> GetRingtonePlayer(const std::shared_ptr<AbilityRuntime::Context> &ctx,
        RingtoneType type) override;

    int32_t SetSystemToneUri(const std::shared_ptr<AbilityRuntime::Context> &ctx, const std::string &uri) override;
    std::string GetSystemToneUri(const std::shared_ptr<AbilityRuntime::Context> &ctx) override;

private:
    void LoadSystemSoundUriMap(void);
    void WriteUriToKvStore(RingtoneType ringtoneType, const std::string &systemSoundType, const std::string &uri);
    bool LoadUriFromKvStore(RingtoneType ringtoneType, const std::string &systemSoundType);
    std::string GetKeyForRingtoneKvStore(RingtoneType ringtoneType, const std::string &systemSoundType);

    std::array<std::string, 2> ringtoneUri_ = {}; // 2 is the max number of sim cards.
    std::unordered_map<RingtoneType, std::unordered_map<std::string, std::string>> ringtoneUriMap_;
    std::array<std::shared_ptr<RingtonePlayer>, 2> ringtonePlayer_ = {nullptr}; // 2 is the max number of sim cards.
    std::shared_ptr<AppExecFwk::DataAbilityHelper> abilityHelper_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif // SYSTEM_SOUND_MANAGER_IMPL_H
