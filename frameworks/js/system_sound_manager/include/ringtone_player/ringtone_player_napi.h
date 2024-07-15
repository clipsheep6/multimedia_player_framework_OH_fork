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

#ifndef RINGTONE_PLAYER_NAPI_H
#define RINGTONE_PLAYER_NAPI_H

#include <map>

#include "audio_info.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "media_errors.h"
#include "media_log.h"
#include "ringtone_common_napi.h"
#include "ringtone_options_napi.h"
#include "ringtone_player.h"
#include "ringtone_player_callback_napi.h"

namespace OHOS {
namespace Media {
static const std::string RINGTONE_PLAYER_NAPI_CLASS_NAME = "RingtonePlayer";

class RingtonePlayerNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);
    static napi_value GetRingtonePlayerInstance(napi_env env, std::shared_ptr<RingtonePlayer> &ringtonePlayer);

    RingtonePlayerNapi();
    ~RingtonePlayerNapi();

private:
    static void RingtonePlayerNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value RingtonePlayerNapiConstructor(napi_env env, napi_callback_info info);
    static napi_value GetTitle(napi_env env, napi_callback_info info);
    static napi_value GetAudioRendererInfo(napi_env env, napi_callback_info info);
    static void AsyncGetAudioRendererInfo(napi_env env, void *data);
    static napi_value Configure(napi_env env, napi_callback_info info);
    static void AsyncConfigure(napi_env env, void *data);
    static napi_value Start(napi_env env, napi_callback_info info);
    static napi_value Stop(napi_env env, napi_callback_info info);
    static napi_value Release(napi_env env, napi_callback_info info);
    static napi_value GetAudioState(napi_env env, napi_callback_info info);
    static napi_value On(napi_env env, napi_callback_info info);
    static napi_value Off(napi_env env, napi_callback_info info);

    static napi_value RegisterCallback(napi_env env, napi_value jsThis, napi_value* argv, const std::string& cbName);
    static napi_value RegisterRingtonePlayerCallback(napi_env env, napi_value* argv, const std::string& cbName,
        RingtonePlayerNapi *ringtonePlayerNapi);
    static napi_value UnregisterCallback(napi_env env, napi_value jsThis, const std::string& cbName);
    static void UnregisterRingtonePlayerCallback(RingtonePlayerNapi *ringtonePlayerNapi, const std::string& cbName);

    static void CommonAsyncCallbackComplete(napi_env env, napi_status status, void* data);
    static void GetTitleAsyncCallbackComplete(napi_env env, napi_status status, void *data);
    static void GetAudioRendererInfoAsyncCallbackComplete(napi_env env, napi_status status, void *data);

    napi_env env_;
    std::shared_ptr<RingtonePlayer> ringtonePlayer_;
    std::shared_ptr<RingtonePlayerInterruptCallback> callbackNapi_ = nullptr;

    static thread_local napi_ref sConstructor_;
    static std::shared_ptr<RingtonePlayer> sRingtonePlayer_;
};

struct RingtonePlayerAsyncContext {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    bool status;
    RingtonePlayerNapi *objectInfo;
    AudioStandard::ContentType contentType;
    AudioStandard::StreamUsage streamUsage;
    int32_t rendererFlags;
    float volume;
    bool loop;
    std::string title;
    RingtoneState *ringtoneState;
};
} // namespace Media
} // namespace OHOS
#endif // RINGTONE_PLAYER_NAPI_H