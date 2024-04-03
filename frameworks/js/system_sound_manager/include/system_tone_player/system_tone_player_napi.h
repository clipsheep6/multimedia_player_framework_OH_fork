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

#ifndef SYSTEM_TONE_NAPI_H
#define SYSTEM_TONE_NAPI_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "media_errors.h"
#include "media_log.h"
#include "system_tone_options_napi.h"
#include "system_tone_player.h"

namespace OHOS {
namespace Media {
static const std::string SYSTEM_TONE_PLAYER_NAPI_CLASS_NAME = "SystemTonePlayer";

class SystemTonePlayerNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);
    static napi_value GetSystemTonePlayerInstance(napi_env env, std::shared_ptr<SystemTonePlayer> &systemTonePlayer);

    SystemTonePlayerNapi();
    ~SystemTonePlayerNapi();

private:
    static void SystemTonePlayerNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value SystemTonePlayerNapiConstructor(napi_env env, napi_callback_info info);

    static napi_value GetTitle(napi_env env, napi_callback_info info);
    static napi_value Prepare(napi_env env, napi_callback_info info);
    static napi_value Start(napi_env env, napi_callback_info info);
    static void AsyncStart(napi_env env, void *data);
    static napi_value Stop(napi_env env, napi_callback_info info);
    static napi_value Release(napi_env env, napi_callback_info info);

    static void CommonAsyncCallbackComplete(napi_env env, napi_status status, void* data);
    static void GetTitleAsyncCallbackComplete(napi_env env, napi_status status, void *data);
    static void StartAsyncCallbackComplete(napi_env env, napi_status status, void *data);

    napi_env env_;
    std::shared_ptr<SystemTonePlayer> systemTonePlayer_;

    static thread_local napi_ref sConstructor_;
    static std::shared_ptr<SystemTonePlayer> sSystemTonePlayer_;
};

struct SystemTonePlayerAsyncContext {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    bool status;
    SystemTonePlayerNapi *objectInfo;
    std::string title;
    int32_t streamID;
    SystemToneOptions systemToneOptions {false, false};
};
} // namespace Media
} // namespace OHOS
#endif // SYSTEM_TONE_NAPI_H