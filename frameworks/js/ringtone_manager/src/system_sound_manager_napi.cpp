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

#include "system_sound_manager_napi.h"

#include "access_token.h"
#include "accesstoken_kit.h"
#include "audio_renderer_info_napi.h"
#include "ipc_skeleton.h"
#include "media_log.h"
#include "tokenid_kit.h"

using namespace std;
using OHOS::Security::AccessToken::AccessTokenKit;

namespace {
    /* Constants for array index */
    const int32_t PARAM0 = 0;
    const int32_t PARAM1 = 1;
    const int32_t PARAM2 = 2;
    const int32_t PARAM3 = 3;

    /* Constants for array size */
    const int32_t ARGS_ONE = 1;
    const int32_t ARGS_TWO = 2;
    const int32_t ARGS_THREE = 3;
    const int32_t ARGS_FOUR = 4;
    const int32_t SIZE = 100;

    const int ERROR = -1;
    const int SUCCESS = 0;

    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SystemSoundManagerNapi"};
}

namespace OHOS {
namespace Media {
napi_ref SystemSoundManagerNapi::sConstructor_ = nullptr;
napi_ref SystemSoundManagerNapi::ringtoneType_ = nullptr;

SystemSoundManagerNapi::SystemSoundManagerNapi()
    : env_(nullptr), sysSoundMgrClient_(nullptr) {}

SystemSoundManagerNapi::~SystemSoundManagerNapi() = default;

napi_status SystemSoundManagerNapi::AddNamedProperty(napi_env env, napi_value object, const std::string name,
    int32_t enumValue)
{
    napi_status status;
    napi_value napiValue;

    status = napi_create_int32(env, enumValue, &napiValue);
    if (status == napi_ok) {
        status = napi_set_named_property(env, object, name.c_str(), napiValue);
    }

    return status;
}

napi_value SystemSoundManagerNapi::CreateRingtoneTypeObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;
    int32_t refCount = 1;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (auto &iter: ringtoneTypeMap) {
            propName = iter.first;
            status = AddNamedProperty(env, result, propName, iter.second);
            if (status != napi_ok) {
                MEDIA_LOGE("Failed to add named prop!");
                break;
            }
            propName.clear();
        }
        if (status == napi_ok) {
            status = napi_create_reference(env, result, refCount, &ringtoneType_);
            if (status == napi_ok) {
                return result;
            }
        }
    }
    napi_get_undefined(env, &result);

    return result;
}

napi_value SystemSoundManagerNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;

    napi_property_descriptor syssndmgr_prop[] = {
        DECLARE_NAPI_FUNCTION("setSystemRingtoneUri", SetSystemRingtoneUri),
        DECLARE_NAPI_FUNCTION("getSystemRingtoneUri", GetSystemRingtoneUri),
        DECLARE_NAPI_FUNCTION("getSystemRingtonePlayer", GetSystemRingtonePlayer),
        DECLARE_NAPI_FUNCTION("setSystemNotificationUri", SetSystemNotificationUri),
        DECLARE_NAPI_FUNCTION("getSystemNotificationUri", GetSystemNotificationUri),
        DECLARE_NAPI_FUNCTION("setSystemAlarmUri", SetSystemAlarmUri),
        DECLARE_NAPI_FUNCTION("getSystemAlarmUri", GetSystemAlarmUri)
    };

    napi_property_descriptor static_prop[] = {
        DECLARE_NAPI_STATIC_FUNCTION("getSystemSoundManager", GetSystemSoundManager),
        DECLARE_NAPI_PROPERTY("RingtoneType", CreateRingtoneTypeObject(env))
    };

    status = napi_define_class(env, SYSTEM_SND_MNGR_NAPI_CLASS_NAME.c_str(),
        NAPI_AUTO_LENGTH, Construct, nullptr, sizeof(syssndmgr_prop) / sizeof(syssndmgr_prop[0]),
        syssndmgr_prop, &ctorObj);
    if (status == napi_ok) {
        if (napi_create_reference(env, ctorObj, refCount, &sConstructor_) == napi_ok) {
            if (napi_set_named_property(env, exports,
                SYSTEM_SND_MNGR_NAPI_CLASS_NAME.c_str(), ctorObj) == napi_ok &&
                napi_define_properties(env, exports, sizeof(static_prop) / sizeof
                (static_prop[0]), static_prop) == napi_ok) {
                    return exports;
            }
        }
    }

    return nullptr;
}

shared_ptr<AbilityRuntime::Context> SystemSoundManagerNapi::GetAbilityContext(napi_env env, napi_value contextArg)
{
    MEDIA_LOGI("SystemSoundManagerNapi::GetAbilityContext");
    bool stageMode = false;

    napi_status status = AbilityRuntime::IsStageContext(env, contextArg, stageMode);
    if (status == napi_ok && stageMode) {
        MEDIA_LOGI("GetAbilityContext: Getting context with stage model");
        auto context = AbilityRuntime::GetStageModeContext(env, contextArg);
        CHECK_AND_RETURN_RET_LOG(context != nullptr, nullptr, "Failed to obtain ability in STAGE mode");
        return context;
    } else {
        MEDIA_LOGI("GetAbilityContext: Getting context with FA model");
        auto ability = AbilityRuntime::GetCurrentAbility(env);
        CHECK_AND_RETURN_RET_LOG(ability != nullptr, nullptr, "Failed to obtain ability in FA mode");
        auto faContext = ability->GetAbilityContext();
        CHECK_AND_RETURN_RET_LOG(faContext != nullptr, nullptr, "GetAbilityContext returned null in FA model");
        return faContext;
    }
}

napi_value SystemSoundManagerNapi::Construct(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    status = napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr);
    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<SystemSoundManagerNapi> obj = std::make_unique<SystemSoundManagerNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            obj->sysSoundMgrClient_ = RingtoneFactory::CreateRingtoneManager();
            if (obj->sysSoundMgrClient_ == nullptr) {
                MEDIA_LOGE("Failed to create sysSoundMgrClient_ instance.");
                return result;
            }

            status = napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()),
                               SystemSoundManagerNapi::Destructor, nullptr, nullptr);
            if (status == napi_ok) {
                obj.release();
                return thisVar;
            } else {
                MEDIA_LOGE("Failed to wrap the native syssndmngr object with JS.");
            }
        }
    }

    return result;
}

void SystemSoundManagerNapi::Destructor(napi_env env, void* nativeObject, void* finalize_hint)
{
    SystemSoundManagerNapi *systemSoundManagerhelper = reinterpret_cast<SystemSoundManagerNapi*>(nativeObject);
    if (systemSoundManagerhelper != nullptr) {
        systemSoundManagerhelper->~SystemSoundManagerNapi();
    }
}

bool SystemSoundManagerNapi::VerifySelfSystemPermission()
{
    Security::AccessToken::FullTokenID selfTokenID = IPCSkeleton::GetSelfTokenID();
    auto tokenTypeFlag = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(static_cast<uint32_t>(selfTokenID));
    if (tokenTypeFlag == Security::AccessToken::TOKEN_NATIVE ||
        tokenTypeFlag == Security::AccessToken::TOKEN_SHELL ||
        Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(selfTokenID)) {
        return true;
    }
    return false;
}

void SystemSoundManagerNapi::SetSystemSoundUriAsyncCallbackComp(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<SystemSoundManagerAsyncContext *>(data);
    napi_value setUriCallback = nullptr;
    napi_value retVal = nullptr;
    napi_value result[2] = {};

    napi_get_undefined(env, &result[PARAM1]);
    if (!context->status) {
        napi_get_undefined(env, &result[PARAM0]);
    } else {
        napi_value message = nullptr;
        napi_create_string_utf8(env, "SetUri Error: Operation is not supported or failed", NAPI_AUTO_LENGTH, &message);
        napi_create_error(env, nullptr, message, &result[PARAM0]);
    }

    if (context->deferred) {
        if (!context->status) {
            napi_resolve_deferred(env, context->deferred, result[PARAM1]);
        } else {
            napi_reject_deferred(env, context->deferred, result[PARAM0]);
        }
    } else {
        napi_get_reference_value(env, context->callbackRef, &setUriCallback);
        napi_call_function(env, nullptr, setUriCallback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, context->callbackRef);
    }
    napi_delete_async_work(env, context->work);

    delete context;
    context = nullptr;
}

void SystemSoundManagerNapi::GetSystemSoundUriAsyncCallbackComp(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<SystemSoundManagerAsyncContext *>(data);
    napi_value getUriCallback = nullptr;
    napi_value retVal = nullptr;
    napi_value result[2] = {};

    if (!context->status) {
        napi_get_undefined(env, &result[PARAM0]);
        napi_create_string_utf8(env, context->uri.c_str(), NAPI_AUTO_LENGTH, &result[PARAM1]);
    } else {
        napi_value message = nullptr;
        napi_create_string_utf8(env, "GetUri Error: Operation is not supported or failed", NAPI_AUTO_LENGTH, &message);
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
        napi_get_reference_value(env, context->callbackRef, &getUriCallback);
        napi_call_function(env, nullptr, getUriCallback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, context->callbackRef);
    }
    napi_delete_async_work(env, context->work);

    delete context;
    context = nullptr;
}

napi_value SystemSoundManagerNapi::GetSystemSoundManager(napi_env env, napi_callback_info info)
{
    if (!VerifySelfSystemPermission()) {
        MEDIA_LOGE("GetSystemSoundManager: System permission validation failed.");
        return nullptr;
    }
    napi_status status;
    napi_value result = nullptr;
    napi_value ctor;

    status = napi_get_reference_value(env, sConstructor_, &ctor);
    if (status == napi_ok) {
        status = napi_new_instance(env, ctor, 0, nullptr, &result);
        if (status == napi_ok) {
            return result;
        } else {
            MEDIA_LOGE("GetSystemSoundManager: new instance can not be obtained.");
        }
    }

    napi_get_undefined(env, &result);
    return result;
}

napi_value SystemSoundManagerNapi::SetSystemRingtoneUri(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_FOUR;
    napi_value argv[ARGS_FOUR] = {0};
    char buffer[SIZE];
    napi_value thisVar = nullptr;
    const int32_t refCount = 1;
    size_t res = 0;

    status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    if (status != napi_ok || thisVar == nullptr) {
        MEDIA_LOGE("SetSystemRingtoneUri: Failed to retrieve details about the callback");
        return result;
    }

    NAPI_ASSERT(env, (argc == ARGS_THREE || argc == ARGS_FOUR),
        "SetSystemRingtoneUri: requires 4 parameters maximum");
    std::unique_ptr<SystemSoundManagerAsyncContext> asyncContext = std::make_unique<SystemSoundManagerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status != napi_ok || asyncContext->objectInfo == nullptr) {
        MEDIA_LOGE("SetSystemRingtoneUri: Failed to unwrap object");
        return result;
    }

    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == PARAM0) {
            asyncContext->abilityContext_ = GetAbilityContext(env, argv[i]);
        } else if (i == PARAM1 && valueType == napi_string) {
            napi_get_value_string_utf8(env, argv[i], buffer, SIZE, &res);
            asyncContext->uri = std::string(buffer);
        } else if (i == PARAM2 && valueType == napi_number) {
            napi_get_value_int32(env, argv[i], &asyncContext->ringtoneType);
        } else if (i == PARAM3 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
            break;
        } else {
            if ((i == PARAM1) && (valueType == napi_null)) {
                MEDIA_LOGE("SetSystemRingtoneUri: string value type is null");
                continue;
            }
            NAPI_ASSERT(env, false, "SetSystemRingtoneUri: type mismatch");
        }
    }

    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &result);
    }

    napi_create_string_utf8(env, "SetSystemRingtoneUri", NAPI_AUTO_LENGTH, &resource);
    status = napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            SystemSoundManagerAsyncContext *context = static_cast<SystemSoundManagerAsyncContext *>(data);
            if (context->uri.empty()) {
                context->status = ERROR;
            } else {
                context->status = context->objectInfo->sysSoundMgrClient_->SetSystemRingtoneUri(
                    context->abilityContext_, context->uri, static_cast<RingtoneType>(context->ringtoneType));
            }
        }, SetSystemSoundUriAsyncCallbackComp, static_cast<void*>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        MEDIA_LOGE("Failed to get create async work");
        napi_get_undefined(env, &result);
    } else {
        napi_queue_async_work(env, asyncContext->work);
        asyncContext.release();
    }

    return result;
}

napi_value SystemSoundManagerNapi::GetSystemRingtoneUri(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_THREE;
    napi_value argv[ARGS_THREE] = {0};
    napi_value thisVar = nullptr;
    const int32_t refCount = 1;

    status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    if (status != napi_ok || thisVar == nullptr) {
        MEDIA_LOGE("GetSystemRingtoneUri: Failed to retrieve details about the callback");
        return result;
    }

    NAPI_ASSERT(env, (argc == ARGS_TWO || argc == ARGS_THREE), "GetSystemRingtoneUri: requires 3 parameters maximum");
    std::unique_ptr<SystemSoundManagerAsyncContext> asyncContext
        = std::make_unique<SystemSoundManagerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == PARAM0) {
                asyncContext->abilityContext_ = GetAbilityContext(env, argv[i]);
            } else if (i == PARAM1 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->ringtoneType);
            } else if (i == PARAM2 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "GetSystemRingtoneUri: type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        }

        napi_create_string_utf8(env, "GetSystemRingtoneUri", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource,
            [](napi_env env, void* data) {
                SystemSoundManagerAsyncContext *context
                    = static_cast<SystemSoundManagerAsyncContext *>(data);
                context->uri = context->objectInfo->sysSoundMgrClient_->GetSystemRingtoneUri(
                    context->abilityContext_, static_cast<RingtoneType>(context->ringtoneType));
                context->status = SUCCESS;
            },
            GetSystemSoundUriAsyncCallbackComp, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_LOGE("GetSystemRingtoneUri: Failed to get create async work");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void SystemSoundManagerNapi::GetRingtonePlayerAsyncCallbackComp(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<SystemSoundManagerAsyncContext *>(data);
    napi_value getPlayerCallback = nullptr;
    napi_value retVal = nullptr;
    napi_value result[2] = {};
    napi_value rngplrResult = nullptr;

    if (context->ringtonePlayer != nullptr) {
        rngplrResult = RingtonePlayerNapi::GetRingtonePlayerInstance(env, context->ringtonePlayer);
        if (rngplrResult == nullptr) {
            napi_value message = nullptr;
            napi_create_string_utf8(env, "GetPlayer Error: Operation is not supported or failed",
                NAPI_AUTO_LENGTH, &message);
            napi_create_error(env, nullptr, message, &result[PARAM0]);
            napi_get_undefined(env, &result[PARAM1]);
        } else {
            napi_get_undefined(env, &result[0]);
            result[PARAM1] = rngplrResult;
        }
    } else {
        MEDIA_LOGE("No get ringtoneplayer found!");
        napi_value message = nullptr;
        napi_create_string_utf8(env, "GetPlayer Error: Operation is not supported or failed",
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
        napi_get_reference_value(env, context->callbackRef, &getPlayerCallback);
        napi_call_function(env, nullptr, getPlayerCallback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, context->callbackRef);
    }
    napi_delete_async_work(env, context->work);

    delete context;
    context = nullptr;
}

napi_value SystemSoundManagerNapi::GetSystemRingtonePlayer(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_THREE;
    napi_value argv[ARGS_THREE] = {0};
    napi_value thisVar = nullptr;
    const int32_t refCount = 1;

    status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    if (status != napi_ok || thisVar == nullptr) {
        MEDIA_LOGE("GetSystemRingtonePlayer: Failed to retrieve details about the callback");
        return result;
    }

    NAPI_ASSERT(env, (argc == ARGS_TWO || argc == ARGS_THREE),
        "GetSystemRingtonePlayer: requires 3 parameters maximum");
    std::unique_ptr<SystemSoundManagerAsyncContext> asyncContext
        = std::make_unique<SystemSoundManagerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == PARAM0) {
                asyncContext->abilityContext_ = GetAbilityContext(env, argv[i]);
            } else if (i == PARAM1 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->ringtoneType);
            } else if (i == PARAM2 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "GetSystemRingtonePlayer: type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        }

        napi_create_string_utf8(env, "GetSystemRingtonePlayer", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource,
            [](napi_env env, void* data) {
                SystemSoundManagerAsyncContext *context
                    = static_cast<SystemSoundManagerAsyncContext *>(data);
                std::shared_ptr<IRingtonePlayer> ringtonePlayer = context->objectInfo->sysSoundMgrClient_->
                    GetRingtonePlayer(context->abilityContext_, static_cast<RingtoneType>(context->ringtoneType));
                if (ringtonePlayer != nullptr) {
                    context->ringtonePlayer = ringtonePlayer;
                    context->status = SUCCESS;
                } else {
                    context->status = ERROR;
                }
            },
            GetRingtonePlayerAsyncCallbackComp, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_LOGE("GetSystemRingtonePlayer: Failed to get create async work");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value SystemSoundManagerNapi::SetSystemNotificationUri(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_THREE;
    napi_value argv[ARGS_THREE] = {0};
    char buffer[SIZE];
    napi_value thisVar = nullptr;
    const int32_t refCount = 1;
    size_t res = 0;

    status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    if (status != napi_ok || thisVar == nullptr) {
        MEDIA_LOGE("SetSystemNotificationUri: Failed to retrieve details about the callback");
        return result;
    }

    NAPI_ASSERT(env, (argc == ARGS_TWO || argc == ARGS_THREE),
        "SetSystemNotificationUri: requires 3 parameters maximum");
    std::unique_ptr<SystemSoundManagerAsyncContext> asyncContext
        = std::make_unique<SystemSoundManagerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status != napi_ok || asyncContext->objectInfo == nullptr) {
        MEDIA_LOGE("SetSystemNotificationUri: Failed to unwrap object");
        return result;
    }

    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == PARAM0) {
            asyncContext->abilityContext_ = GetAbilityContext(env, argv[i]);
        } else if (i == PARAM1 && valueType == napi_string) {
            napi_get_value_string_utf8(env, argv[i], buffer, SIZE, &res);
            asyncContext->uri = std::string(buffer);
        } else if (i == PARAM2 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
            break;
        } else {
            if ((i == PARAM1) && (valueType == napi_null)) {
                MEDIA_LOGE("SetSystemNotificationUri: string value type is null");
                continue;
            }
            NAPI_ASSERT(env, false, "SetSystemNotificationUri: type mismatch");
        }
    }

    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &result);
    }

    napi_create_string_utf8(env, "SetSystemNotificationUri", NAPI_AUTO_LENGTH, &resource);
    status = napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            SystemSoundManagerAsyncContext *context = static_cast<SystemSoundManagerAsyncContext *>(data);
            if (context->uri.empty()) {
                context->status = ERROR;
            } else {
                context->status = context->objectInfo->sysSoundMgrClient_->SetSystemNotificationUri(
                    context->abilityContext_, context->uri);
            }
        },
        SetSystemSoundUriAsyncCallbackComp, static_cast<void*>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        MEDIA_LOGE("SetSystemNotificationUri: Failed to get create async work");
        napi_get_undefined(env, &result);
    } else {
        napi_queue_async_work(env, asyncContext->work);
        asyncContext.release();
    }

    return result;
}

napi_value SystemSoundManagerNapi::GetSystemNotificationUri(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
    const int32_t refCount = 1;

    status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    if (status != napi_ok || thisVar == nullptr) {
        MEDIA_LOGE("GetSystemNotificationUri: Failed to retrieve details about the callback");
        return result;
    }

    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO),
        "GetSystemNotificationUri: requires 2 parameters maximum");
    std::unique_ptr<SystemSoundManagerAsyncContext> asyncContext
        = std::make_unique<SystemSoundManagerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_TWO) {
            asyncContext->abilityContext_ = GetAbilityContext(env, argv[PARAM0]);

            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[PARAM1], &valueType);
            if (valueType == napi_function) {
                napi_create_reference(env, argv[PARAM1], refCount, &asyncContext->callbackRef);
            } else {
                NAPI_ASSERT(env, false, "GetSystemNotificationUri: type mismatch");
            }
        } else {
            napi_create_promise(env, &asyncContext->deferred, &result);
        }

        napi_create_string_utf8(env, "GetSystemNotificationUri", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource,
            [](napi_env env, void* data) {
                SystemSoundManagerAsyncContext *context
                    = static_cast<SystemSoundManagerAsyncContext *>(data);
                context->uri = context->objectInfo->sysSoundMgrClient_->GetSystemNotificationUri(
                    context->abilityContext_);
                context->status = SUCCESS;
            },
            GetSystemSoundUriAsyncCallbackComp, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_LOGE("GetSystemNotificationUri: Failed to get create async work");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value SystemSoundManagerNapi::SetSystemAlarmUri(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_THREE;
    napi_value argv[ARGS_THREE] = {0};
    char buffer[SIZE];
    napi_value thisVar = nullptr;
    const int32_t refCount = 1;
    size_t res = 0;

    status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    if (status != napi_ok || thisVar == nullptr) {
        MEDIA_LOGE("SetSystemAlarmUri: Failed to retrieve details about the callback");
        return result;
    }

    NAPI_ASSERT(env, (argc == ARGS_TWO || argc == ARGS_THREE),
        "SetSystemAlarmUri: requires 3 parameters maximum");
    std::unique_ptr<SystemSoundManagerAsyncContext> asyncContext
        = std::make_unique<SystemSoundManagerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status != napi_ok || asyncContext->objectInfo == nullptr) {
        MEDIA_LOGE("SetSystemAlarmUri: Failed to unwrap object");
        return result;
    }

    for (size_t i = PARAM1; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == PARAM1 && valueType == napi_string) {
            napi_get_value_string_utf8(env, argv[i], buffer, SIZE, &res);
            asyncContext->uri = std::string(buffer);
        } else if (i == PARAM2 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
            break;
        } else {
            if ((i == PARAM1) && (valueType == napi_null)) {
                MEDIA_LOGE("SetSystemAlarmUri: string value type is null");
                continue;
            }
            NAPI_ASSERT(env, false, "SetSystemAlarmUri: type mismatch");
        }
    }

    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &result);
    }

    napi_create_string_utf8(env, "SetSystemAlarmUri", NAPI_AUTO_LENGTH, &resource);
    status = napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            SystemSoundManagerAsyncContext *context = static_cast<SystemSoundManagerAsyncContext *>(data);
            if (context->uri.empty()) {
                context->status = ERROR;
            } else {
                context->status = context->objectInfo->sysSoundMgrClient_->SetSystemAlarmUri(
                    context->abilityContext_, context->uri);
            }
        },
        SetSystemSoundUriAsyncCallbackComp, static_cast<void*>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        MEDIA_LOGE("SetSystemAlarmUri: Failed to get create async work");
        napi_get_undefined(env, &result);
    } else {
        napi_queue_async_work(env, asyncContext->work);
        asyncContext.release();
    }

    return result;
}

napi_value SystemSoundManagerNapi::GetSystemAlarmUri(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
    const int32_t refCount = 1;

    status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    if (status != napi_ok || thisVar == nullptr) {
        MEDIA_LOGE("GetSystemAlarmUri: Failed to retrieve details about the callback");
        return result;
    }

    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO),
        "GetSystemAlarmUri: requires 2 parameters maximum");
    std::unique_ptr<SystemSoundManagerAsyncContext> asyncContext
        = std::make_unique<SystemSoundManagerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_TWO) {
            asyncContext->abilityContext_ = GetAbilityContext(env, argv[PARAM0]);

            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[PARAM1], &valueType);
            if (valueType == napi_function) {
                napi_create_reference(env, argv[PARAM1], refCount, &asyncContext->callbackRef);
            } else {
                NAPI_ASSERT(env, false, "GetSystemAlarmUri: type mismatch");
            }
        } else {
            napi_create_promise(env, &asyncContext->deferred, &result);
        }

        napi_create_string_utf8(env, "GetSystemAlarmUri", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource,
            [](napi_env env, void* data) {
                SystemSoundManagerAsyncContext *context
                    = static_cast<SystemSoundManagerAsyncContext *>(data);
                context->uri = context->objectInfo->sysSoundMgrClient_->GetSystemAlarmUri(
                    context->abilityContext_);
                context->status = SUCCESS;
            },
            GetSystemSoundUriAsyncCallbackComp, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_LOGE("GetSystemAlarmUri: Failed to get create async work");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

static napi_value Init(napi_env env, napi_value exports)
{
    SystemSoundManagerNapi::Init(env, exports);
    AudioRendererInfoNapi::Init(env, exports);
    RingtoneOptionsNapi::Init(env, exports);
    RingtonePlayerNapi::Init(env, exports);

    return exports;
}

/*
 * module define
 */
static napi_module g_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "multimedia.systemSoundManager",
    .nm_priv = (reinterpret_cast<void*>(0)),
    .reserved = {0}
};

/*
 * module register
 */
extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&g_module);
}
} // namespace Media
} // namespace OHOS