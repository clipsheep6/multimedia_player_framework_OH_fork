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

#include "system_sound_manager.h"

namespace OHOS {
namespace Media {
class SystemSoundManagerImpl : public SystemSoundManager {
public:
    SystemSoundManagerImpl();
    ~SystemSoundManagerImpl();

    // SystemSoundManager override
    int32_t SetRingtoneUri(const std::shared_ptr<AbilityRuntime::Context> &context, const std::string &uri,
        RingtoneType ringtoneType) override;
    std::string GetRingtoneUri(const std::shared_ptr<AbilityRuntime::Context> &context,
        RingtoneType ringtoneType) override;
    std::shared_ptr<RingtonePlayer> GetRingtonePlayer(const std::shared_ptr<AbilityRuntime::Context> &context,
        RingtoneType ringtoneType) override;

    int32_t SetSystemToneUri(const std::shared_ptr<AbilityRuntime::Context> &context, const std::string &uri,
        SystemToneType systemToneType) override;
    std::string GetSystemToneUri(const std::shared_ptr<AbilityRuntime::Context> &context,
        SystemToneType systemToneType) override;
    std::shared_ptr<SystemTonePlayer> GetSystemTonePlayer(const std::shared_ptr<AbilityRuntime::Context> &context,
        SystemToneType systemToneType) override;

private:
    void LoadSystemSoundUriMap(void);
    void WriteUriToDatabase(const std::string &key, const std::string &uri);
    std::string GetUriFromDatabase(const std::string &key);
    std::string GetKeyForDatabase(const std::string &systemSoundType, int32_t type);

    bool isRingtoneTypeValid(RingtoneType ringtongType);
    bool isSystemToneTypeValid(SystemToneType systemToneType);

    std::unordered_map<RingtoneType, std::string> ringtoneUriMap_;
    std::unordered_map<RingtoneType, std::shared_ptr<RingtonePlayer>> ringtonePlayerMap_;
    std::unordered_map<SystemToneType, std::string> systemToneUriMap_;
    std::unordered_map<SystemToneType, std::shared_ptr<SystemTonePlayer>> systemTonePlayerMap_;
};
} // namespace Media
} // namespace OHOS
#endif // SYSTEM_SOUND_MANAGER_IMPL_H
