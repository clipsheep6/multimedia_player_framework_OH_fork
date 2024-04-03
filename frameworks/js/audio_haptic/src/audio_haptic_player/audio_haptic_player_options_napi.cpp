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

#include "audio_haptic_player_options_napi.h"
#include "media_log.h"

using namespace std;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioHapticPlayerOptionsNapi"};
}

namespace OHOS {
namespace Media {
thread_local napi_ref AudioHapticPlayerOptionsNapi::sConstructor_ = nullptr;

bool AudioHapticPlayerOptionsNapi::sMuteAudio_ = false;
bool AudioHapticPlayerOptionsNapi::sMuteHaptics_ = false;

AudioHapticPlayerOptionsNapi::AudioHapticPlayerOptionsNapi()
    : env_(nullptr) {
}

AudioHapticPlayerOptionsNapi::~AudioHapticPlayerOptionsNapi() = default;

void AudioHapticPlayerOptionsNapi::Destructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    if (nativeObject != nullptr) {
        auto obj = static_cast<AudioHapticPlayerOptionsNapi *>(nativeObject);
        delete obj;
        obj = nullptr;
    }
}

napi_value AudioHapticPlayerOptionsNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value constructor;
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    napi_property_descriptor audio_haptic_player_options_properties[] = {
        DECLARE_NAPI_GETTER_SETTER("muteAudio", IsAudioMute, SetAudioMute),
        DECLARE_NAPI_GETTER_SETTER("muteHaptics", IsHapticsMute, SetHapticsMute)
    };

    status = napi_define_class(env, AUDIO_HAPTIC_PLAYER_OPTIONS_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Construct,
        nullptr, sizeof(audio_haptic_player_options_properties) / sizeof(audio_haptic_player_options_properties[0]),
        audio_haptic_player_options_properties, &constructor);
    if (status != napi_ok) {
        return result;
    }

    status = napi_create_reference(env, constructor, 1, &sConstructor_);
    if (status == napi_ok) {
        status = napi_set_named_property(env, exports,
            AUDIO_HAPTIC_PLAYER_OPTIONS_NAPI_CLASS_NAME.c_str(), constructor);
        if (status == napi_ok) {
            return exports;
        }
    }
    MEDIA_LOGE("Failure in AudioHapticPlayerOptionsNapi::Init()");

    return result;
}

napi_value AudioHapticPlayerOptionsNapi::Construct(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsThis = nullptr;
    size_t argCount = 0;

    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status == napi_ok) {
        unique_ptr<AudioHapticPlayerOptionsNapi> obj = make_unique<AudioHapticPlayerOptionsNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            obj->muteAudio_ = sMuteAudio_;
            obj->muteHaptics_ = sMuteHaptics_;
            status = napi_wrap(env, jsThis, static_cast<void*>(obj.get()),
                AudioHapticPlayerOptionsNapi::Destructor, nullptr, nullptr);
            if (status == napi_ok) {
                obj.release();
                return jsThis;
            }
        }
    }

    MEDIA_LOGE("Failed in AudioHapticPlayerOptionsNapi::Construct()!");
    napi_get_undefined(env, &jsThis);

    return jsThis;
}

napi_value AudioHapticPlayerOptionsNapi::CreateAudioHapticPlayerOptionsWrapper(napi_env env,
    bool muteAudio, bool muteHaptics)
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
    MEDIA_LOGE("Failed in CreateAudioHapticPlayerOptionsWrapper, %{public}d", status);

    napi_get_undefined(env, &result);

    return result;
}

napi_value AudioHapticPlayerOptionsNapi::IsAudioMute(napi_env env, napi_callback_info info)
{
    napi_status status;
    AudioHapticPlayerOptionsNapi *audioHapticPlayerOptionsNapi = nullptr;
    size_t argc = 0;
    napi_value jsThis = nullptr;
    napi_value jsResult = nullptr;
    napi_get_undefined(env, &jsResult);

    status = napi_get_cb_info(env, info, &argc, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("IsAudioMute: failed for napi_get_cb_info");
        return jsResult;
    }

    status = napi_unwrap(env, jsThis, (void **)&audioHapticPlayerOptionsNapi);
    if (status == napi_ok) {
        status = napi_get_boolean(env, audioHapticPlayerOptionsNapi->muteAudio_, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }

    return jsResult;
}

napi_value AudioHapticPlayerOptionsNapi::SetAudioMute(napi_env env, napi_callback_info info)
{
    napi_status status;
    AudioHapticPlayerOptionsNapi *audioHapticPlayerOptionsNapi = nullptr;
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    napi_value jsThis = nullptr;
    bool muteAudio = false;
    napi_value jsResult = nullptr;
    napi_get_undefined(env, &jsResult);

    status = napi_get_cb_info(env, info, &argc, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr) {
        MEDIA_LOGE("SetAudioMute: failed for napi_get_cb_info");
        return jsResult;
    }

    status = napi_unwrap(env, jsThis, (void **)&audioHapticPlayerOptionsNapi);
    if (status == napi_ok) {
        napi_valuetype valueType = napi_undefined;
        if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_boolean) {
            MEDIA_LOGE("SetAudioMute: failed for wrong data type");
            return jsResult;
        }
    }

    status = napi_get_value_bool(env, args[0], &muteAudio);
    if (status == napi_ok) {
        audioHapticPlayerOptionsNapi->muteAudio_ = muteAudio;
    }

    return jsResult;
}

napi_value AudioHapticPlayerOptionsNapi::IsHapticsMute(napi_env env, napi_callback_info info)
{
    napi_status status;
    AudioHapticPlayerOptionsNapi *audioHapticPlayerOptionsNapi = nullptr;
    size_t argc = 0;
    napi_value jsThis = nullptr;
    napi_value jsResult = nullptr;
    napi_get_undefined(env, &jsResult);

    status = napi_get_cb_info(env, info, &argc, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("IsHapticsMute: failed for napi_get_cb_info");
        return jsResult;
    }

    status = napi_unwrap(env, jsThis, (void **)&audioHapticPlayerOptionsNapi);
    if (status == napi_ok) {
        status = napi_get_boolean(env, audioHapticPlayerOptionsNapi->muteHaptics_, &jsResult);
        if (status == napi_ok) {
            return jsResult;
        }
    }

    return jsResult;
}

napi_value AudioHapticPlayerOptionsNapi::SetHapticsMute(napi_env env, napi_callback_info info)
{
    napi_status status;
    AudioHapticPlayerOptionsNapi *audioHapticPlayerOptionsNapi = nullptr;
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    napi_value jsThis = nullptr;
    bool muteHaptics = false;
    napi_value jsResult = nullptr;
    napi_get_undefined(env, &jsResult);

    status = napi_get_cb_info(env, info, &argc, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr) {
        MEDIA_LOGE("SetHapticsMute: failed for napi_get_cb_info");
        return jsResult;
    }

    status = napi_unwrap(env, jsThis, (void **)&audioHapticPlayerOptionsNapi);
    if (status == napi_ok) {
        napi_valuetype valueType = napi_undefined;
        if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_boolean) {
            MEDIA_LOGE("SetHapticsMute: failed for wrong data type");
            return jsResult;
        }
    }

    status = napi_get_value_bool(env, args[0], &muteHaptics);
    if (status == napi_ok) {
        audioHapticPlayerOptionsNapi->muteHaptics_ = muteHaptics;
    }

    return jsResult;
}
}  // namespace Media
}  // namespace OHOS
