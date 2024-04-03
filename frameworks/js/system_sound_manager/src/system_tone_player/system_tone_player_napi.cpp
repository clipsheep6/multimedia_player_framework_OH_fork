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

#include "system_tone_player_napi.h"

#include "media_log.h"
#include "common_napi.h"

namespace {
/* Constants for array index */
const int32_t PARAM0 = 0;
const int32_t PARAM1 = 1;

/* Constants for array size */
const int32_t ARGS_ONE = 1;
const int32_t ARGS_TWO = 2;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SystemTonePlayerNapi"};
}

namespace OHOS {
namespace Media {
thread_local napi_ref SystemTonePlayerNapi::sConstructor_ = nullptr;
std::shared_ptr<SystemTonePlayer> SystemTonePlayerNapi::sSystemTonePlayer_ = nullptr;

SystemTonePlayerNapi::SystemTonePlayerNapi() : env_(nullptr) {}

SystemTonePlayerNapi::~SystemTonePlayerNapi() = default;

napi_value SystemTonePlayerNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;

    napi_property_descriptor system_tone_player_prop[] = {
        DECLARE_NAPI_FUNCTION("getTitle", GetTitle),
        DECLARE_NAPI_FUNCTION("prepare", Prepare),
        DECLARE_NAPI_FUNCTION("start", Start),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("release", Release),
    };

    status = napi_define_class(env, SYSTEM_TONE_PLAYER_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH,
        SystemTonePlayerNapiConstructor, nullptr, sizeof(system_tone_player_prop) / sizeof(system_tone_player_prop[0]),
        system_tone_player_prop, &ctorObj);
    if (status == napi_ok) {
        if (napi_create_reference(env, ctorObj, refCount, &sConstructor_) == napi_ok) {
            status = napi_set_named_property(env, exports, SYSTEM_TONE_PLAYER_NAPI_CLASS_NAME.c_str(), ctorObj);
            if (status == napi_ok) {
                return exports;
            }
        }
    }

    return nullptr;
}

napi_value SystemTonePlayerNapi::SystemTonePlayerNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    status = napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr);
    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<SystemTonePlayerNapi> obj = std::make_unique<SystemTonePlayerNapi>();
        if (obj != nullptr) {
            ObjectRefMap<SystemTonePlayerNapi>::Insert(obj.get());
            obj->env_ = env;
            if (obj->sSystemTonePlayer_ != nullptr) {
                obj->systemTonePlayer_ = move(obj->sSystemTonePlayer_);
            } else {
                MEDIA_LOGE("Failed to create sSystemTonePlayer_ instance.");
                return result;
            }

            status = napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()),
                SystemTonePlayerNapi::SystemTonePlayerNapiDestructor, nullptr, nullptr);
            if (status == napi_ok) {
                obj.release();
                return thisVar;
            } else {
                ObjectRefMap<SystemTonePlayerNapi>::Erase(obj.get());
                MEDIA_LOGE("Failed to wrap the native system tone player object with JS.");
            }
        }
    }

    return result;
}

void SystemTonePlayerNapi::SystemTonePlayerNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint)
{
    SystemTonePlayerNapi *systemTonePlayerHelper = reinterpret_cast<SystemTonePlayerNapi*>(nativeObject);
    if (systemTonePlayerHelper != nullptr) {
        ObjectRefMap<SystemTonePlayerNapi>::DecreaseRef(systemTonePlayerHelper);
    }
}

napi_value SystemTonePlayerNapi::GetSystemTonePlayerInstance(napi_env env,
    std::shared_ptr<SystemTonePlayer> &systemTonePlayer)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value ctor;

    status = napi_get_reference_value(env, sConstructor_, &ctor);
    if (status == napi_ok) {
        sSystemTonePlayer_ = systemTonePlayer;
        status = napi_new_instance(env, ctor, 0, nullptr, &result);
        if (status == napi_ok) {
            return result;
        } else {
            MEDIA_LOGE("GetSystemTonePlayerInstance: New instance could not be obtained.");
        }
    }

    napi_get_undefined(env, &result);
    return result;
}

void SystemTonePlayerNapi::CommonAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<SystemTonePlayerAsyncContext *>(data);
    napi_value callback = nullptr;
    napi_value retVal = nullptr;
    napi_value result[2] = {};

    napi_get_undefined(env, &result[PARAM1]);
    if (!context->status) {
        napi_get_undefined(env, &result[PARAM0]);
    } else {
        napi_value message = nullptr;
        napi_create_string_utf8(env, "Error: Operation is not supported or failed", NAPI_AUTO_LENGTH, &message);
        napi_create_error(env, nullptr, message, &result[PARAM0]);
    }

    if (context->deferred) {
        if (!context->status) {
            napi_resolve_deferred(env, context->deferred, result[PARAM1]);
        } else {
            napi_reject_deferred(env, context->deferred, result[PARAM0]);
        }
    } else {
        napi_get_reference_value(env, context->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, context->callbackRef);
    }
    napi_delete_async_work(env, context->work);

    delete context;
    context = nullptr;
}

void SystemTonePlayerNapi::GetTitleAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto context = static_cast<SystemTonePlayerAsyncContext *>(data);
    napi_value getTitleCallback = nullptr;
    napi_value retVal = nullptr;
    napi_value result[2] = {};

    if (!context->status) {
        napi_get_undefined(env, &result[PARAM0]);
        napi_create_string_utf8(env, context->title.c_str(), NAPI_AUTO_LENGTH, &result[PARAM1]);
    } else {
        napi_value message = nullptr;
        napi_create_string_utf8(env, "GetTitle Error: Operation is not supported or failed",
            NAPI_AUTO_LENGTH, &message);
        napi_create_error(env, nullptr, message, &result[PARAM0]);
        napi_get_undefined(env, &result[PARAM1]);
    }

    if (context->deferred) {
        if (!context->status) {
            napi_resolve_deferred(env, context->deferred, result[PARAM1]);
        } else {
            napi_reject_deferred(env, context->deferred, result[PARAM0]);
        }
    } else {
        napi_get_reference_value(env, context->callbackRef, &getTitleCallback);
        napi_call_function(env, nullptr, getTitleCallback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, context->callbackRef);
    }
    napi_delete_async_work(env, context->work);

    delete context;
    context = nullptr;
}

napi_value SystemTonePlayerNapi::GetTitle(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    const int32_t refCount = 1;

    napi_status status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && thisVar != nullptr, result, "GetTitle: napi_get_cb_info failed");

    NAPI_ASSERT(env, argc <= ARGS_ONE, "GetTitle: requires 1 parameter maximum");
    std::unique_ptr<SystemTonePlayerAsyncContext> asyncContext = std::make_unique<SystemTonePlayerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[PARAM0], &valueType);
            CHECK_AND_RETURN_RET_LOG(valueType == napi_function, result,
                "GetTitle: the param type is not napi_function");
            napi_create_reference(env, argv[PARAM0], refCount, &asyncContext->callbackRef);
        } else {
            napi_create_promise(env, &asyncContext->deferred, &result);
        }

        napi_create_string_utf8(env, "GetTitle", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource,
            [](napi_env env, void *data) {
                SystemTonePlayerAsyncContext *context = static_cast<SystemTonePlayerAsyncContext *>(data);
                context->title = context->objectInfo->systemTonePlayer_->GetTitle();
                context->status = MSERR_OK;
            },
            GetTitleAsyncCallbackComplete, static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_LOGE("GetTitle: Failed to get create async work");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value SystemTonePlayerNapi::Prepare(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    const int32_t refCount = 1;

    status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && thisVar != nullptr, result,
        "Prepare: Failed to retrieve details about the callback");

    NAPI_ASSERT(env, argc <= ARGS_ONE, "Prepare: requires 1 parameter maximum");
    std::unique_ptr<SystemTonePlayerAsyncContext> asyncContext = std::make_unique<SystemTonePlayerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[PARAM0], &valueType);
            CHECK_AND_RETURN_RET_LOG(valueType == napi_function, result,
                "Prepare: the param type is not napi_function");
            napi_create_reference(env, argv[PARAM0], refCount, &asyncContext->callbackRef);
        } else {
            napi_create_promise(env, &asyncContext->deferred, &result);
        }

        napi_create_string_utf8(env, "Prepare", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource,
            [](napi_env env, void* data) {
                SystemTonePlayerAsyncContext* context = static_cast<SystemTonePlayerAsyncContext*>(data);
                context->status = context->objectInfo->systemTonePlayer_->Prepare();
            },
            CommonAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_LOGE("Prepare: Failed to get create async work");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void SystemTonePlayerNapi::StartAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto context = static_cast<SystemTonePlayerAsyncContext *>(data);
    napi_value getTitleCallback = nullptr;
    napi_value retVal = nullptr;
    napi_value result[2] = {};

    if (!context->status) {
        napi_get_undefined(env, &result[PARAM0]);
        napi_create_int32(env, context->streamID, &result[PARAM1]);
    } else {
        napi_value message = nullptr;
        napi_create_string_utf8(env, "GetTitle Error: Operation is not supported or failed",
            NAPI_AUTO_LENGTH, &message);
        napi_create_error(env, nullptr, message, &result[PARAM0]);
        napi_get_undefined(env, &result[PARAM1]);
    }

    if (context->deferred) {
        if (!context->status) {
            napi_resolve_deferred(env, context->deferred, result[PARAM1]);
        } else {
            napi_reject_deferred(env, context->deferred, result[PARAM0]);
        }
    } else {
        napi_get_reference_value(env, context->callbackRef, &getTitleCallback);
        napi_call_function(env, nullptr, getTitleCallback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, context->callbackRef);
    }
    napi_delete_async_work(env, context->work);

    delete context;
    context = nullptr;
}

napi_value SystemTonePlayerNapi::Start(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value resource = nullptr;
    napi_value property = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && thisVar != nullptr, result, "Start: napi_get_cb_info failed");
    NAPI_ASSERT(env, argc <= ARGS_TWO, "Start: requires 2 parameter maximum");

    std::unique_ptr<SystemTonePlayerAsyncContext> asyncContext = std::make_unique<SystemTonePlayerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncContext->objectInfo != nullptr, result,
        "Start: napi_unwrap failed or objectInfo is nullptr.");

    if (argc == 0) {
        napi_create_promise(env, &asyncContext->deferred, &result);
    }
    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == PARAM0 && valueType == napi_object) {
            if (napi_get_named_property(env, argv[PARAM0], "muteAudio", &property) == napi_ok) {
                napi_get_value_bool(env, property, &(asyncContext->systemToneOptions.muteAudio));
            }
            if (napi_get_named_property(env, argv[PARAM0], "muteHaptics", &property) == napi_ok) {
                napi_get_value_bool(env, property, &(asyncContext->systemToneOptions.muteHaptics));
            }
            if (argc == ARGS_ONE) {
                napi_create_promise(env, &asyncContext->deferred, &result);
            }
        } else if ((i == PARAM0 || i == PARAM1) && valueType == napi_function) {
            napi_create_reference(env, argv[PARAM0], 1, &asyncContext->callbackRef);
        } else {
            NAPI_ASSERT(env, false, "Start: type mismatch");
        }
    }

    napi_create_string_utf8(env, "Start", NAPI_AUTO_LENGTH, &resource);
    status = napi_create_async_work(env, nullptr, resource, AsyncStart,
        StartAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
    } else {
        napi_queue_async_work(env, asyncContext->work);
        asyncContext.release();
    }
    return result;
}

void SystemTonePlayerNapi::AsyncStart(napi_env env, void *data)
{
    SystemTonePlayerAsyncContext* context = static_cast<SystemTonePlayerAsyncContext*>(data);
    auto obj = reinterpret_cast<SystemTonePlayerNapi*>(context->objectInfo);
    ObjectRefMap objectGuard(obj);
    auto *napiSystemTonePlayer = objectGuard.GetPtr();
    context->streamID = napiSystemTonePlayer->systemTonePlayer_->Start(context->systemToneOptions);
    context->status = MSERR_OK;
}

napi_value SystemTonePlayerNapi::Stop(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
    const int32_t refCount = 1;

    status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    if (status != napi_ok || thisVar == nullptr) {
        MEDIA_LOGE("Stop: Failed to retrieve details about the callback");
        return result;
    }

    NAPI_ASSERT(env, argc == ARGS_ONE || argc == ARGS_TWO, "Stop: requires 1 or 2 parameter");
    std::unique_ptr<SystemTonePlayerAsyncContext> asyncContext = std::make_unique<SystemTonePlayerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[PARAM0], &valueType);
        if (valueType == napi_number) {
            napi_get_value_int32(env, argv[PARAM0], &(asyncContext->streamID));
            napi_create_promise(env, &asyncContext->deferred, &result);
        } else {
            NAPI_ASSERT(env, false, "Stop: type mismatch");
        }
        if (argc == ARGS_TWO) {
            napi_typeof(env, argv[PARAM1], &valueType);
            CHECK_AND_RETURN_RET_LOG(valueType == napi_function, result, "Stop: the param type is not napi_function");
            napi_create_reference(env, argv[PARAM0], refCount, &asyncContext->callbackRef);
        } else {
            napi_create_promise(env, &asyncContext->deferred, &result);
        }

        napi_create_string_utf8(env, "Stop", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource,
            [](napi_env env, void* data) {
                SystemTonePlayerAsyncContext* context = static_cast<SystemTonePlayerAsyncContext*>(data);
                context->status = context->objectInfo->systemTonePlayer_->Stop(context->streamID);
            },
            CommonAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_LOGE("Stop: Failed to get create async work");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value SystemTonePlayerNapi::Release(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    const int32_t refCount = 1;

    status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    if (status != napi_ok || thisVar == nullptr) {
        MEDIA_LOGE("Release: Failed to retrieve details about the callback");
        return result;
    }

    NAPI_ASSERT(env, argc <= ARGS_ONE, "Release: requires 1 parameter maximum");
    std::unique_ptr<SystemTonePlayerAsyncContext> asyncContext = std::make_unique<SystemTonePlayerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[PARAM0], &valueType);
            CHECK_AND_RETURN_RET_LOG(valueType == napi_function, result,
                "Release: the param type is not napi_function");
            napi_create_reference(env, argv[PARAM0], refCount, &asyncContext->callbackRef);
        } else {
            napi_create_promise(env, &asyncContext->deferred, &result);
        }

        napi_create_string_utf8(env, "Release", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource,
            [](napi_env env, void* data) {
                SystemTonePlayerAsyncContext* context = static_cast<SystemTonePlayerAsyncContext*>(data);
                context->status = context->objectInfo->systemTonePlayer_->Release();
            },
            CommonAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_LOGE("Release: Failed to get create async work");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}
} // namespace Media
} // namespace OHOS