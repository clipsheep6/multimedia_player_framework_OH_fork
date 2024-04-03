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

#include "system_tone_options_napi.h"
#include "media_log.h"

using namespace std;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SystemToneOptionsNapi"};
}

namespace OHOS {
namespace Media {
thread_local napi_ref SystemToneOptionsNapi::sConstructor_ = nullptr;

bool SystemToneOptionsNapi::sMuteAudio_ = false;
bool SystemToneOptionsNapi::sMuteHaptics_ = false;

SystemToneOptionsNapi::SystemToneOptionsNapi()
    : env_(nullptr) {
}

SystemToneOptionsNapi::~SystemToneOptionsNapi() = default;

void SystemToneOptionsNapi::Destructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    if (nativeObject != nullptr) {
        auto obj = static_cast<SystemToneOptionsNapi *>(nativeObject);
        delete obj;
        obj = nullptr;
    }
}

napi_value SystemToneOptionsNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value constructor;
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    napi_property_descriptor system_tone_options_properties[] = {
        DECLARE_NAPI_GETTER_SETTER("muteAudio", IsAudioMute, SetAudioMute),
        DECLARE_NAPI_GETTER_SETTER("muteHaptics", IsHapticsMute, SetHapticsMute)
    };

    status = napi_define_class(env, SYSTEM_TONE_OPTIONS_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Construct,
        nullptr, sizeof(system_tone_options_properties) / sizeof(system_tone_options_properties[0]),
        system_tone_options_properties, &constructor);
    if (status != napi_ok) {
        return result;
    }

    status = napi_create_reference(env, constructor, 1, &sConstructor_);
    if (status == napi_ok) {
        status = napi_set_named_property(env, exports, SYSTEM_TONE_OPTIONS_NAPI_CLASS_NAME.c_str(), constructor);
        if (status == napi_ok) {
            return exports;
        }
    }
    MEDIA_LOGE("Failure in SystemToneOptionsNapi::Init()");

    return result;
}

napi_value SystemToneOptionsNapi::Construct(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsThis = nullptr;
    size_t argCount = 0;

    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status == napi_ok) {
        unique_ptr<SystemToneOptionsNapi> obj = make_unique<SystemToneOptionsNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            obj->muteAudio_ = sMuteAudio_;
            obj->muteHaptics_ = sMuteHaptics_;
            status = napi_wrap(env, jsThis, static_cast<void*>(obj.get()),
                SystemToneOptionsNapi::Destructor, nullptr, nullptr);
            if (status == napi_ok) {
                obj.release();
                return jsThis;
            }
        }
    }

    MEDIA_LOGE("Failed in SystemToneOptionsNapi::Construct()!");
    napi_get_undefined(env, &jsThis);

    return jsThis;
}

napi_value SystemToneOptionsNapi::CreateSystemToneOptionsWrapper(napi_env env, bool muteAudio, bool muteHaptics)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;

    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        sMuteAudio_ = muteAudio;
        sMuteHaptics_ = muteHaptics;
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        if (status == napi_ok) {
            return result;
        }
    }
    MEDIA_LOGE("Failed in CreateSystemToneOptionsWrapper, %{public}d", status);

    napi_get_undefined(env, &result);

    return result;
}

napi_value SystemToneOptionsNapi::IsAudioMute(napi_env env, napi_callback_info info)
{
    napi_status status;
    SystemToneOptionsNapi *systemToneOptionsNapi = nullptr;
    size_t argc = 0;
    napi_value jsThis = nullptr;
    napi_value jsResult = nullptr;
    napi_get_undefined(env, &jsResult);

    status = napi_get_cb_info(env, info, &argc, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("IsAudioMute: failed for napi_get_cb_info");
        return jsResult;
    }

    status = napi_unwrap(env, jsThis, (void **)&systemToneOptionsNapi);
    if (status == napi_ok) {
        status = napi_get_boolean(env, systemToneOptionsNapi->muteAudio_, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }

    return jsResult;
}

napi_value SystemToneOptionsNapi::SetAudioMute(napi_env env, napi_callback_info info)
{
    napi_status status;
    SystemToneOptionsNapi *systemToneOptionsNapi = nullptr;
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    napi_value jsThis = nullptr;
    bool muteAudio = false;
    napi_value jsResult = nullptr;
    napi_get_undefined(env, &jsResult);

    status = napi_get_cb_info(env, info, &argc, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || argc < 1) {
        MEDIA_LOGE("SetAudioMute: failed for napi_get_cb_info");
        return jsResult;
    }

    status = napi_unwrap(env, jsThis, (void **)&systemToneOptionsNapi);
    if (status == napi_ok) {
        napi_valuetype valueType = napi_undefined;
        if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_boolean) {
            MEDIA_LOGE("SetAudioMute: failed for wrong data type");
            return jsResult;
        }
    }

    status = napi_get_value_bool(env, args[0], &muteAudio);
    if (status == napi_ok) {
        systemToneOptionsNapi->muteAudio_ = muteAudio;
    }

    return jsResult;
}

napi_value SystemToneOptionsNapi::IsHapticsMute(napi_env env, napi_callback_info info)
{
    napi_status status;
    SystemToneOptionsNapi *systemToneOptionsNapi = nullptr;
    size_t argc = 0;
    napi_value jsThis = nullptr;
    napi_value jsResult = nullptr;
    napi_get_undefined(env, &jsResult);

    status = napi_get_cb_info(env, info, &argc, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("IsHapticsMute: failed for napi_get_cb_info");
        return jsResult;
    }

    status = napi_unwrap(env, jsThis, (void **)&systemToneOptionsNapi);
    if (status == napi_ok) {
        status = napi_get_boolean(env, systemToneOptionsNapi->muteHaptics_, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }

    return jsResult;
}

napi_value SystemToneOptionsNapi::SetHapticsMute(napi_env env, napi_callback_info info)
{
    napi_status status;
    SystemToneOptionsNapi *ringtoneOptionsNapi = nullptr;
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    napi_value jsThis = nullptr;
    bool muteHaptics = false;
    napi_value jsResult = nullptr;
    napi_get_undefined(env, &jsResult);

    status = napi_get_cb_info(env, info, &argc, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || argc < 1) {
        MEDIA_LOGE("SetHapticsMute: failed for napi_get_cb_info");
        return jsResult;
    }

    status = napi_unwrap(env, jsThis, (void **)&ringtoneOptionsNapi);
    if (status == napi_ok) {
        napi_valuetype valueType = napi_undefined;
        if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_boolean) {
            MEDIA_LOGE("SetHapticsMute: failed for wrong data type");
            return jsResult;
        }
    }

    status = napi_get_value_bool(env, args[0], &muteHaptics);
    if (status == napi_ok) {
        ringtoneOptionsNapi->muteHaptics_ = muteHaptics;
    }

    return jsResult;
}
}  // namespace Media
}  // namespace OHOS
