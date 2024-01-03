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

#ifndef AUDIO_HAPTIC_MANAGER_NAPI_H
#define AUDIO_HAPTIC_MANAGER_NAPI_H

#include "audio_haptic_manager.h"

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "audio_info.h"

#include "audio_haptic_common_napi.h"
#include "audio_haptic_player_napi.h"
#include "audio_haptic_player_options_napi.h"

namespace OHOS {
namespace Media {
static const std::string AUDIO_HAPTIC_MANAGER_NAPI_CLASS_NAME = "AudioHapticManager";

static const std::map<std::string, AudioLatencyMode> latencyModeMap = {
    {"AUDIO_LATENCY_MODE_NORMAL", AUDIO_LATENCY_MODE_NORMAL},
    {"AUDIO_LATENCY_MODE_FAST", AUDIO_LATENCY_MODE_FAST}
};

class AudioHapticManagerNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);

    AudioHapticManagerNapi();
    ~AudioHapticManagerNapi();

private:
    static void Destructor(napi_env env, void *nativeObject, void *finalizeHint);
    static napi_value Construct(napi_env env, napi_callback_info info);
    static napi_value GetAudioHapticManager(napi_env env, napi_callback_info info);
    static napi_status AddNamedProperty(napi_env env, napi_value object, const std::string name, int32_t enumValue);
    static napi_value CreateLatencyModeObject(napi_env env);

    static napi_value RegisterSource(napi_env env, napi_callback_info info);
    static void AsyncRegisterSource(napi_env env, void *data);
    static napi_value UnregisterSource(napi_env env, napi_callback_info info);
    static napi_value SetAudioLatencyMode(napi_env env, napi_callback_info info);
    static napi_value SetStreamUsage(napi_env env, napi_callback_info info);
    static napi_value CreatePlayer(napi_env env, napi_callback_info info);
    static void AsyncCreatePlayer(napi_env env, void *data);

    static void RegisterSourceAsyncCallbackComp(napi_env env, napi_status status, void *data);
    static void UnregisterSourceAsyncCallbackComp(napi_env env, napi_status status, void *data);
    static void CreatePlayerAsyncCallbackComp(napi_env env, napi_status status, void *data);

    static bool IsLegalAudioLatencyMode(int32_t latencyMode);
    static bool IsLegalAudioStreamUsage(int32_t streamUsage);

    static napi_ref sConstructor_;
    static napi_ref sLatencyMode_;

    napi_env env_;

    std::shared_ptr<AudioHapticManager> audioHapticMgrClient_ = nullptr;
};

struct AudioHapticManagerAsyncContext {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    bool status;
    AudioHapticManagerNapi *objectInfo;
    std::string audioUri;
    std::string hapticUri;
    std::shared_ptr<AudioHapticPlayer> audioHapticPlayer;
    int32_t sourceID;
    AudioHapticPlayerOptions playerOptions {false, false};
};
} // namespace Media
} // namespace OHOS
#endif // AUDIO_HAPTIC_MANAGER_NAPI_H