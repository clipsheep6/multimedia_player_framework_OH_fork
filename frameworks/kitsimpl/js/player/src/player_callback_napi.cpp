/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "player_callback_napi.h"
#include <uv.h>
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerCallbackNapi"};
}

namespace OHOS {
namespace Media {
PlayerCallbackNapi::PlayerCallbackNapi(napi_env env, AudioPlayerNapi &player)
    : env_(env),
      playerNapi_(player)
{
}

PlayerCallbackNapi::~PlayerCallbackNapi()
{
}

void PlayerCallbackNapi::OnError(int32_t errorType, int32_t errorCode)
{
    MEDIA_LOGD("OnError is called, type: %{public}d, error code: %{public}d", errorType, errorCode);
    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback {
        .env = env_,
        .callback = playerNapi_.errorCallback_,
        .callbackName = "error",
        .errorType = std::to_string(errorType),
        .errorCode = std::to_string(errorCode),
    };
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");
    CHECK_AND_RETURN_LOG(cb->env != nullptr, "env is nullptr");
    CHECK_AND_RETURN_LOG(cb->callback != nullptr, "callback is nullptr");
    return OnJsCallBackError(cb);
}

void PlayerCallbackNapi::OnSeekDone(uint64_t currentPositon)
{
    MEDIA_LOGD("OnSeekDone is called, currentPositon: %{public}llu", currentPositon);
    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback {
        .env = env_,
        .callback = playerNapi_.timeUpdateCallback_,
        .callbackName = "position",
        .position = static_cast<int32_t>(currentPositon),
    };
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");
    CHECK_AND_RETURN_LOG(cb->env != nullptr, "env is nullptr");
    CHECK_AND_RETURN_LOG(cb->callback != nullptr, "callback is nullptr");
    return OnJsCallBackPosition(cb);
}

void PlayerCallbackNapi::OnEndOfStream(bool isLooping)
{
    MEDIA_LOGD("OnEndOfStream is called, isloop: %{public}d", isLooping);
    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback {
        .env = env_,
        .callback = playerNapi_.finishCallback_,
        .callbackName = "finish",
    };
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");
    CHECK_AND_RETURN_LOG(cb->env != nullptr, "env is nullptr");
    CHECK_AND_RETURN_LOG(cb->callback != nullptr, "callback is nullptr");
    return OnJsCallBack(cb);
}

void PlayerCallbackNapi::OnStateChanged(PlayerStates state)
{
    MEDIA_LOGD("OnStateChanged is called, current state: %{public}d", state);
    playerNapi_.SetCurrentState(state);
    PlayerJsCallback *cb = new(std::nothrow) PlayerJsCallback {
        .env = env_,
        .callback = GetStateCallBackRef(state),
        .callbackName = GetStateCallBackName(state),
    };
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");
    CHECK_AND_RETURN_LOG(cb->env != nullptr, "env is nullptr");
    CHECK_AND_RETURN_LOG(cb->callback != nullptr, "callback is nullptr");
    return OnJsCallBack(cb);
}

void PlayerCallbackNapi::OnMessage(int32_t type, int32_t extra)
{
    MEDIA_LOGD("OnMessage() is called, type: %{public}d, extra: %{public}d", type, extra);
}

void PlayerCallbackNapi::OnPositionUpdated(uint64_t postion)
{
    MEDIA_LOGD("OnPositionUpdated() is called, postion: %{public}llu", postion);
}

napi_ref PlayerCallbackNapi::GetStateCallBackRef(PlayerStates state)
{
    switch (state) {
        case PLAYER_PREPARED:
            return playerNapi_.dataLoadCallback_;
        case PLAYER_STARTED:
            return playerNapi_.playCallback_;
        case PLAYER_PAUSED:
            return playerNapi_.pauseCallback_;
        case PLAYER_STOPPED:
            return playerNapi_.stopCallback_;
        case PLAYER_IDLE:
            return playerNapi_.resetCallback_;
        default:
            return nullptr;
    }
}

std::string PlayerCallbackNapi::GetStateCallBackName(PlayerStates state)
{
    switch (state) {
        case PLAYER_PREPARED:
            return "dataload";
        case PLAYER_STARTED:
            return "play";
        case PLAYER_PAUSED:
            return "pause";
        case PLAYER_STOPPED:
            return "stop";
        case PLAYER_IDLE:
            return "reset";
        default:
            return "unknown";
    }
}

void PlayerCallbackNapi::OnJsCallBack(PlayerJsCallback *jsCb)
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        MEDIA_LOGE("fail to new uv_work_t");
        delete jsCb;
        return;
    }
    work->data = reinterpret_cast<void *>(jsCb);

    uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        MEDIA_LOGD("JsCallBack Default, uv_queue_work start");
        // Js Thread
        PlayerJsCallback *event = reinterpret_cast<PlayerJsCallback *>(work->data);
        std::string request = event->callbackName;
        do {
            napi_value jsCallback = nullptr;
            status = napi_get_reference_value(event->env, event->callback, &jsCallback);
            CHECK_AND_BREAK_LOG(status == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            // Call back function
            napi_value result = nullptr;
            status = napi_call_function(event->env, nullptr, jsCallback, 0, nullptr, &result);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s fail to napi call function", request.c_str());

            if (event->callback != nullptr) {
                napi_delete_reference(event->env, event->callback);
            }
        } while (0);
        delete event;
        delete work;
    });
}

void PlayerCallbackNapi::OnJsCallBackError(PlayerJsCallback *jsCb)
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        MEDIA_LOGE("fail to new uv_work_t");
        delete jsCb;
        return;
    }
    work->data = reinterpret_cast<void *>(jsCb);

    uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        MEDIA_LOGD("JsCallBack Error, uv_queue_work start");
        // Js Thread
        PlayerJsCallback *event = reinterpret_cast<PlayerJsCallback *>(work->data);
        std::string request = event->callbackName;
        do {
            napi_value jsCallback = nullptr;
            status = napi_get_reference_value(event->env, event->callback, &jsCallback);
            CHECK_AND_BREAK_LOG(status == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            napi_value errorTypeVal = nullptr;
            status = napi_create_string_utf8(event->env, event->errorType.c_str(), NAPI_AUTO_LENGTH, &errorTypeVal);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s fail to get error type value", request.c_str());

            napi_value errorCodeVal = nullptr;
            status = napi_create_string_utf8(event->env, event->errorCode.c_str(), NAPI_AUTO_LENGTH, &errorCodeVal);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s fail to get error code value", request.c_str());

            napi_value args[1] = { nullptr };
            status = napi_create_error(event->env, errorTypeVal, errorCodeVal, &args[0]);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s fail to create error callback", request.c_str());

            // Call back function
            const size_t argCount = 1;
            napi_value result = nullptr;
            status = napi_call_function(event->env, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s fail to napi call function", request.c_str());

            if (event->callback != nullptr) {
                napi_delete_reference(event->env, event->callback);
            }
        } while (0);
        delete event;
        delete work;
    });
}

void PlayerCallbackNapi::OnJsCallBackPosition(PlayerJsCallback *jsCb)
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        MEDIA_LOGE("fail to new uv_work_t");
        delete jsCb;
        return;
    }
    work->data = reinterpret_cast<void *>(jsCb);

    uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        MEDIA_LOGD("JsCallBack Position, uv_queue_work start");
        // Js Thread
        PlayerJsCallback *event = reinterpret_cast<PlayerJsCallback *>(work->data);
        std::string request = event->callbackName;
        do {
            napi_value jsCallback = nullptr;
            status = napi_get_reference_value(event->env, event->callback, &jsCallback);
            CHECK_AND_BREAK_LOG(status == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            // Call back function
            napi_value args[1] = { nullptr };
            status = napi_create_int32(event->env, event->position, &args[0]);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s fail to create position callback", request.c_str());

            const size_t argCount = 1;
            napi_value result = nullptr;
            status = napi_call_function(event->env, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s fail to call seekDone callback", request.c_str());

            if (event->callback != nullptr) {
                napi_delete_reference(event->env, event->callback);
            }
        } while (0);
        delete event;
        delete work;
    });
}
}  // namespace Media
}  // namespace OHOS