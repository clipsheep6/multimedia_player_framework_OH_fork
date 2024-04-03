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

#ifndef SYSTEM_SOUND_MNGR_NAPI_H
#define SYSTEM_SOUND_MNGR_NAPI_H

#include "system_sound_manager.h"

#include "ringtone_player_napi.h"
#include "system_tone_player_napi.h"

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_base_context.h"
#include "ability.h"

namespace OHOS {
namespace Media {
static const std::string SYSTEM_SND_MNGR_NAPI_CLASS_NAME = "SystemSoundManager";

static const std::map<std::string, RingtoneType> ringtoneTypeMap = {
    {"RINGTONE_TYPE_DEFAULT", RINGTONE_TYPE_SIM_CARD_0}, // deprecated
    {"RINGTONE_TYPE_MULTISIM", RINGTONE_TYPE_SIM_CARD_1}, // deprecated
    {"RINGTONE_TYPE_SIM_CARD_0", RINGTONE_TYPE_SIM_CARD_0},
    {"RINGTONE_TYPE_SIM_CARD_1", RINGTONE_TYPE_SIM_CARD_1}
};

static const std::map<std::string, SystemToneType> systemToneTypeMap = {
    {"SYSTEM_TONE_TYPE_SIM_CARD_0", SYSTEM_TONE_TYPE_SIM_CARD_0},
    {"SYSTEM_TONE_TYPE_SIM_CARD_1", SYSTEM_TONE_TYPE_SIM_CARD_1},
    {"SYSTEM_TONE_TYPE_NOTIFICATION", SYSTEM_TONE_TYPE_NOTIFICATION}
};

class SystemSoundManagerNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);

    SystemSoundManagerNapi();
    ~SystemSoundManagerNapi();

private:
    static void Destructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value Construct(napi_env env, napi_callback_info info);
    static napi_value GetSystemSoundManager(napi_env env, napi_callback_info info);
    static napi_status AddNamedProperty(napi_env env, napi_value object, const std::string name, int32_t enumValue);
    static napi_value CreateRingtoneTypeObject(napi_env env);
    static napi_value CreateSystemToneTypeObject(napi_env env);
    static std::shared_ptr<AbilityRuntime::Context> GetAbilityContext(napi_env env, napi_value contextArg);
    static bool VerifySelfSystemPermission();

    static napi_value SetRingtoneUri(napi_env env, napi_callback_info info);
    static void AsyncSetRingtoneUri(napi_env env, void *data);
    static napi_value GetRingtoneUri(napi_env env, napi_callback_info info);
    static void AsyncGetRingtoneUri(napi_env env, void *data);
    static napi_value GetRingtonePlayer(napi_env env, napi_callback_info info);
    static void AsyncGetRingtonePlayer(napi_env env, void *data);

    static napi_value SetSystemToneUri(napi_env env, napi_callback_info info);
    static void AsyncSetSystemToneUri(napi_env env, void *data);
    static napi_value GetSystemToneUri(napi_env env, napi_callback_info info);
    static void AsyncGetSystemToneUri(napi_env env, void *data);
    static napi_value GetSystemTonePlayer(napi_env env, napi_callback_info info);
    static void AsyncGetSystemTonePlayer(napi_env env, void *data);

    static void SetSystemSoundUriAsyncCallbackComp(napi_env env, napi_status status, void* data);
    static void GetSystemSoundUriAsyncCallbackComp(napi_env env, napi_status status, void* data);
    static void GetRingtonePlayerAsyncCallbackComp(napi_env env, napi_status status, void* data);
    static void GetSystemTonePlayerAsyncCallbackComp(napi_env env, napi_status status, void* data);
    static thread_local napi_ref sConstructor_;
    static thread_local napi_ref ringtoneType_;
    static thread_local napi_ref systemToneType_;

    napi_env env_;

    std::shared_ptr<SystemSoundManager> sysSoundMgrClient_ = nullptr;
};

struct SystemSoundManagerAsyncContext {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    bool status;
    SystemSoundManagerNapi *objectInfo;
    std::shared_ptr<AbilityRuntime::Context> abilityContext_;
    std::string uri;
    std::shared_ptr<RingtonePlayer> ringtonePlayer;
    int32_t ringtoneType;
    std::shared_ptr<SystemTonePlayer> systemTonePlayer;
    int32_t systemToneType;
};
} // namespace Media
} // namespace OHOS
#endif // SYSTEM_SOUND_MNGR_NAPI_H