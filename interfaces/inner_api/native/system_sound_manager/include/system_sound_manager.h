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

#ifndef SYSTEM_SOUND_MANAGER_H
#define SYSTEM_SOUND_MANAGER_H

#include <string>

#include "foundation/ability/ability_runtime/interfaces/kits/native/appkit/ability_runtime/context/context.h"

#include "ringtone_player.h"
#include "system_tone_player.h"

namespace OHOS {
namespace Media {
enum RingtoneType {
    RINGTONE_TYPE_SIM_CARD_0 = 0,
    RINGTONE_TYPE_SIM_CARD_1 = 1,
};

enum SystemToneType {
    SYSTEM_TONE_TYPE_SIM_CARD_0 = 0,
    SYSTEM_TONE_TYPE_SIM_CARD_1 = 1,
    SYSTEM_TONE_TYPE_NOTIFICATION = 32,
};

class SystemSoundManager {
public:
    virtual ~SystemSoundManager() = default;

    /**
     * @brief Returns the ringtone player instance.
     *
     * @param context Indicates the Context object on OHOS.
     * @param ringtoneType Indicates the ringtone type for which player instance has to be returned.
     * @return Returns RingtonePlayer.
     * @since 10
     */
    virtual std::shared_ptr<RingtonePlayer> GetRingtonePlayer(const std::shared_ptr<AbilityRuntime::Context> &context,
        RingtoneType ringtoneType) = 0;

    /**
     * @brief API used for setting the ringtone uri.
     *
     * @param context Indicates the Context object on OHOS.
     * @param uri Indicates which uri to be set for the tone type.
     * @param ringtoneType Indicates the ringtone type.
     * @return Returns {@link MSERR_OK} if set the ringtone uri successfully;
     * returns an error code defined in {@link media_errors.h} otherwise.
     * @since 10
     */
    virtual int32_t SetRingtoneUri(const std::shared_ptr<AbilityRuntime::Context> &context, const std::string &uri,
        RingtoneType ringtoneType) = 0;

    /**
     * @brief Returns the current ringtone uri.
     *
     * @param context Indicates the Context object on OHOS.
     * @param ringtoneType Indicates the ringtone type.
     * @return Returns the current ringtone uri.
     * @since 10
     */
    virtual std::string GetRingtoneUri(const std::shared_ptr<AbilityRuntime::Context> &context,
        RingtoneType ringtoneType) = 0;

    /**
     * @brief Returns the system tone player instance
     *
     * @param context Indicates the Context object on OHOS.
     * @param systemToneType Indicates the system tone type for which player instance has to be returned.
     * @return Returns SystemTonePlayer.
     * @since 11
     */
    virtual std::shared_ptr<SystemTonePlayer> GetSystemTonePlayer(
        const std::shared_ptr<AbilityRuntime::Context> &context, SystemToneType systemToneType) = 0;

    /**
     * @brief API used for setting the system tone uri
     *
     * @param context Indicates the Context object on OHOS.
     * @param uri indicates which uri to be set for system tone.
     * @param systemToneType Indicates the system tone type.
     * @return Returns {@link MSERR_OK} if set the system tone uri successfully;
     * returns an error code defined in {@link media_errors.h} otherwise.
     * @since 11
     */
    virtual int32_t SetSystemToneUri(const std::shared_ptr<AbilityRuntime::Context> &context, const std::string &uri,
        SystemToneType systemToneType) = 0;

    /**
     * @brief Returns the current system tone uri
     *
     * @param context Indicates the Context object on OHOS.
     * @return Returns the system tone uri
     * @since 11
     */
    virtual std::string GetSystemToneUri(const std::shared_ptr<AbilityRuntime::Context> &context,
        SystemToneType systemToneType)= 0;
};

class __attribute__((visibility("default"))) SystemSoundManagerFactory {
public:
    static std::shared_ptr<SystemSoundManager> CreateSystemSoundManager();

private:
    static std::shared_ptr<SystemSoundManager> systemSoundManager_;
    static std::mutex systemSoundManagerMutex_;
    SystemSoundManagerFactory() = default;
    ~SystemSoundManagerFactory() = default;
};
} // Media
} // OHOS
#endif // SYSTEM_SOUND_MANAGER_H
