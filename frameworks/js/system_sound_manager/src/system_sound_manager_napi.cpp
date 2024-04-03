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
thread_local napi_ref SystemSoundManagerNapi::sConstructor_ = nullptr;
thread_local napi_ref SystemSoundManagerNapi::ringtoneType_ = nullptr;
thread_local napi_ref SystemSoundManagerNapi::systemToneType_ = nullptr;

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
                MEDIA_LOGE("CreateRingtoneTypeObject: Failed to add named prop!");
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

napi_value SystemSoundManagerNapi::CreateSystemToneTypeObject(napi_env env)
{
    napi_value result = nullptr;
    napi_status status;
    std::string propName;
    int32_t refCount = 1;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        for (auto &iter: systemToneTypeMap) {
            propName = iter.first;
            status = AddNamedProperty(env, result, propName, iter.second);
            if (status != napi_ok) {
                MEDIA_LOGE("CreateSystemToneTypeObject: Failed to add named prop!");
                break;
            }
            propName.clear();
        }
        if (status == napi_ok) {
            status = napi_create_reference(env, result, refCount, &systemToneType_);
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
        DECLARE_NAPI_FUNCTION("setSystemRingtoneUri", SetRingtoneUri), // deprecated
        DECLARE_NAPI_FUNCTION("setRingtoneUri", SetRingtoneUri),
        DECLARE_NAPI_FUNCTION("getSystemRingtoneUri", GetRingtoneUri), // deprecated
        DECLARE_NAPI_FUNCTION("getRingtoneUri", GetRingtoneUri),
        DECLARE_NAPI_FUNCTION("getSystemRingtonePlayer", GetRingtonePlayer), // deprecated
        DECLARE_NAPI_FUNCTION("getRingtonePlayer", GetRingtonePlayer),
        DECLARE_NAPI_FUNCTION("setSystemToneUri", SetSystemToneUri),
        DECLARE_NAPI_FUNCTION("getSystemToneUri", GetSystemToneUri),
        DECLARE_NAPI_FUNCTION("getSystemTonePlayer", GetSystemTonePlayer)
    };

    napi_property_descriptor static_prop[] = {
        DECLARE_NAPI_STATIC_FUNCTION("getSystemSoundManager", GetSystemSoundManager),
        DECLARE_NAPI_PROPERTY("RingtoneType", CreateRingtoneTypeObject(env)),
        DECLARE_NAPI_PROPERTY("SystemToneType", CreateSystemToneTypeObject(env)),
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
            obj->sysSoundMgrClient_ = SystemSoundManagerFactory::CreateSystemSoundManager();
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
                MEDIA_LOGE("Failed to wrap the native sysSoundMgr object with JS.");
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

napi_value SystemSoundManagerNapi::SetRingtoneUri(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_FOUR;
    napi_value argv[ARGS_FOUR] = {0};
    char buffer[SIZE];
    napi_value thisVar = nullptr;
    const int32_t refCount = 1;
    size_t res = 0;

    napi_status status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && thisVar != nullptr, result, "SetRingtoneUri: napi_get_cb_info fail");

    NAPI_ASSERT(env, (argc == ARGS_THREE || argc == ARGS_FOUR), "SetRingtoneUri: requires 4 parameters maximum");
    std::unique_ptr<SystemSoundManagerAsyncContext> asyncContext = std::make_unique<SystemSoundManagerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncContext->objectInfo != nullptr, result,
        "SetRingtoneUri: Failed to unwrap object");

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
            CHECK_AND_CONTINUE(!(i == PARAM1) && (valueType == napi_null));
            NAPI_ASSERT(env, false, "SetRingtoneUri: type mismatch");
        }
    }

    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &result);
    }

    napi_create_string_utf8(env, "SetRingtoneUri", NAPI_AUTO_LENGTH, &resource);
    status = napi_create_async_work(env, nullptr, resource, AsyncSetRingtoneUri,
        SetSystemSoundUriAsyncCallbackComp, static_cast<void*>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        MEDIA_LOGE("Failed to get create async work");
        napi_get_undefined(env, &result);
    } else {
        napi_queue_async_work(env, asyncContext->work);
        asyncContext.release();
    }

    return result;
}

void SystemSoundManagerNapi::AsyncSetRingtoneUri(napi_env env, void *data)
{
    SystemSoundManagerAsyncContext *context = static_cast<SystemSoundManagerAsyncContext *>(data);
    if (context->uri.empty()) {
        context->status = ERROR;
    } else {
        context->status = context->objectInfo->sysSoundMgrClient_->SetRingtoneUri(
            context->abilityContext_, context->uri, static_cast<RingtoneType>(context->ringtoneType));
    }
}

napi_value SystemSoundManagerNapi::GetRingtoneUri(napi_env env, napi_callback_info info)
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
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && thisVar != nullptr, result,
        "GetRingtoneUri: Failed to retrieve details about the callback");

    NAPI_ASSERT(env, (argc == ARGS_TWO || argc == ARGS_THREE), "GetRingtoneUri: requires 3 parameters maximum");
    std::unique_ptr<SystemSoundManagerAsyncContext> asyncContext = std::make_unique<SystemSoundManagerAsyncContext>();
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
                NAPI_ASSERT(env, false, "GetRingtoneUri: type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        }

        napi_create_string_utf8(env, "GetRingtoneUri", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource, AsyncGetRingtoneUri,
            GetSystemSoundUriAsyncCallbackComp, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_LOGE("GetRingtoneUri: Failed to get create async work");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void SystemSoundManagerNapi::AsyncGetRingtoneUri(napi_env env, void *data)
{
    SystemSoundManagerAsyncContext *context
        = static_cast<SystemSoundManagerAsyncContext *>(data);
    context->uri = context->objectInfo->sysSoundMgrClient_->GetRingtoneUri(
        context->abilityContext_, static_cast<RingtoneType>(context->ringtoneType));
    context->status = SUCCESS;
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
            napi_create_string_utf8(env, "GetRingtonePlayer Error: Operation is not supported or failed",
                NAPI_AUTO_LENGTH, &message);
            napi_create_error(env, nullptr, message, &result[PARAM0]);
            napi_get_undefined(env, &result[PARAM1]);
        } else {
            napi_get_undefined(env, &result[0]);
            result[PARAM1] = rngplrResult;
        }
    } else {
        MEDIA_LOGE("Failed to get system tone player!");
        napi_value message = nullptr;
        napi_create_string_utf8(env, "GetRingtonePlayer Error: Operation is not supported or failed",
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

napi_value SystemSoundManagerNapi::GetRingtonePlayer(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_THREE;
    napi_value argv[ARGS_THREE] = {0};
    napi_value thisVar = nullptr;
    const int32_t refCount = 1;

    napi_status status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    CHECK_AND_RETURN_RET_LOG((status == napi_ok && thisVar != nullptr), result,
        "GetRingtonePlayer: Failed to retrieve details about the callback");

    NAPI_ASSERT(env, (argc == ARGS_TWO || argc == ARGS_THREE), "GetRingtonePlayer: require 3 parameters max");
    std::unique_ptr<SystemSoundManagerAsyncContext> asyncContext = std::make_unique<SystemSoundManagerAsyncContext>();
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
                NAPI_ASSERT(env, false, "GetRingtonePlayer: type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        }

        napi_create_string_utf8(env, "GetRingtonePlayer", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource, AsyncGetRingtonePlayer,
            GetRingtonePlayerAsyncCallbackComp, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_LOGE("GetRingtonePlayer: Failed to get create async work");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void SystemSoundManagerNapi::AsyncGetRingtonePlayer(napi_env env, void *data)
{
    SystemSoundManagerAsyncContext *context = static_cast<SystemSoundManagerAsyncContext *>(data);
    std::shared_ptr<RingtonePlayer> ringtonePlayer =context->objectInfo->sysSoundMgrClient_->
        GetRingtonePlayer(context->abilityContext_, static_cast<RingtoneType>(context->ringtoneType));
    if (ringtonePlayer != nullptr) {
        context->ringtonePlayer = ringtonePlayer;
        context->status = SUCCESS;
    } else {
        context->status = ERROR;
    }
}

napi_value SystemSoundManagerNapi::SetSystemToneUri(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_FOUR;
    napi_value argv[ARGS_FOUR] = {0};
    char buffer[SIZE];
    napi_value thisVar = nullptr;
    const int32_t refCount = 1;
    size_t res = 0;

    napi_status status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && thisVar != nullptr, result, "SetSystemToneUri: get_cb_info failed");

    NAPI_ASSERT(env, (argc == ARGS_THREE || argc == ARGS_FOUR), "SetSystemToneUri: requires 4 parameters maximum");
    std::unique_ptr<SystemSoundManagerAsyncContext> asyncContext = std::make_unique<SystemSoundManagerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncContext->objectInfo != nullptr, result,
        "SetSystemToneUri: Failed to unwrap object");

    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == PARAM0) {
            asyncContext->abilityContext_ = GetAbilityContext(env, argv[i]);
        } else if (i == PARAM1 && valueType == napi_string) {
            napi_get_value_string_utf8(env, argv[i], buffer, SIZE, &res);
            asyncContext->uri = std::string(buffer);
        } else if (i == PARAM2 && valueType == napi_number) {
            napi_get_value_int32(env, argv[i], &asyncContext->systemToneType);
        } else if (i == PARAM3 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
            break;
        } else {
            CHECK_AND_CONTINUE(!(i == PARAM1) && (valueType == napi_null));
            NAPI_ASSERT(env, false, "SetSystemToneUri: type mismatch");
        }
    }

    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &result);
    }

    napi_create_string_utf8(env, "SetSystemToneUri", NAPI_AUTO_LENGTH, &resource);
    status = napi_create_async_work(env, nullptr, resource, AsyncSetSystemToneUri,
        SetSystemSoundUriAsyncCallbackComp, static_cast<void*>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        MEDIA_LOGE("Failed to get create async work");
        napi_get_undefined(env, &result);
    } else {
        napi_queue_async_work(env, asyncContext->work);
        asyncContext.release();
    }

    return result;
}

void SystemSoundManagerNapi::AsyncSetSystemToneUri(napi_env env, void *data)
{
    SystemSoundManagerAsyncContext *context = static_cast<SystemSoundManagerAsyncContext *>(data);
    if (context->uri.empty()) {
        context->status = ERROR;
    } else {
        context->status = context->objectInfo->sysSoundMgrClient_->SetSystemToneUri(
            context->abilityContext_, context->uri, static_cast<SystemToneType>(context->systemToneType));
    }
}

napi_value SystemSoundManagerNapi::GetSystemToneUri(napi_env env, napi_callback_info info)
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
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && thisVar != nullptr, result,
        "GetSystemToneUri: Failed to retrieve details about the callback");

    NAPI_ASSERT(env, (argc == ARGS_TWO || argc == ARGS_THREE), "GetSystemToneUri: requires 3 parameters maximum");
    std::unique_ptr<SystemSoundManagerAsyncContext> asyncContext = std::make_unique<SystemSoundManagerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == PARAM0) {
                asyncContext->abilityContext_ = GetAbilityContext(env, argv[i]);
            } else if (i == PARAM1 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->systemToneType);
            } else if (i == PARAM2 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "GetSystemToneUri: type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        }

        napi_create_string_utf8(env, "GetSystemToneUri", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource, AsyncGetSystemToneUri,
            GetSystemSoundUriAsyncCallbackComp, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_LOGE("GetSystemToneUri: Failed to get create async work");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void SystemSoundManagerNapi::AsyncGetSystemToneUri(napi_env env, void *data)
{
    SystemSoundManagerAsyncContext *context
        = static_cast<SystemSoundManagerAsyncContext *>(data);
    context->uri = context->objectInfo->sysSoundMgrClient_->GetSystemToneUri(
        context->abilityContext_, static_cast<SystemToneType>(context->systemToneType));
    context->status = SUCCESS;
}

napi_value SystemSoundManagerNapi::GetSystemTonePlayer(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_THREE;
    napi_value argv[ARGS_THREE] = {0};
    napi_value thisVar = nullptr;
    const int32_t refCount = 1;

    napi_status status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    CHECK_AND_RETURN_RET_LOG((status == napi_ok && thisVar != nullptr), result,
        "GetSystemTonePlayer: Failed to retrieve details about the callback");

    NAPI_ASSERT(env, (argc == ARGS_TWO || argc == ARGS_THREE), "GetSystemTonePlayer: require 3 parameters max");
    std::unique_ptr<SystemSoundManagerAsyncContext> asyncContext = std::make_unique<SystemSoundManagerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == PARAM0) {
                asyncContext->abilityContext_ = GetAbilityContext(env, argv[i]);
            } else if (i == PARAM1 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->systemToneType);
            } else if (i == PARAM2 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "GetSystemTonePlayer: type mismatch");
            }
        }

        if (asyncContext->callbackRef == nullptr) {
            napi_create_promise(env, &asyncContext->deferred, &result);
        }

        napi_create_string_utf8(env, "GetSystemTonePlayer", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource, AsyncGetSystemTonePlayer,
            GetSystemTonePlayerAsyncCallbackComp, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_LOGE("GetRingtonePlayer: Failed to get create async work");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void SystemSoundManagerNapi::AsyncGetSystemTonePlayer(napi_env env, void *data)
{
    SystemSoundManagerAsyncContext *context = static_cast<SystemSoundManagerAsyncContext *>(data);
    std::shared_ptr<SystemTonePlayer> systemTonePlayer =context->objectInfo->sysSoundMgrClient_->
        GetSystemTonePlayer(context->abilityContext_, static_cast<SystemToneType>(context->systemToneType));
    if (systemTonePlayer != nullptr) {
        context->systemTonePlayer = systemTonePlayer;
        context->status = SUCCESS;
    } else {
        context->status = ERROR;
    }
}

void SystemSoundManagerNapi::GetSystemTonePlayerAsyncCallbackComp(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<SystemSoundManagerAsyncContext *>(data);
    napi_value getPlayerCallback = nullptr;
    napi_value retVal = nullptr;
    napi_value result[2] = {};
    napi_value rngplrResult = nullptr;

    if (context->systemTonePlayer != nullptr) {
        rngplrResult = SystemTonePlayerNapi::GetSystemTonePlayerInstance(env, context->systemTonePlayer);
        if (rngplrResult == nullptr) {
            napi_value message = nullptr;
            napi_create_string_utf8(env, "GetSystemTonePlayer Error: Operation is not supported or failed",
                NAPI_AUTO_LENGTH, &message);
            napi_create_error(env, nullptr, message, &result[PARAM0]);
            napi_get_undefined(env, &result[PARAM1]);
        } else {
            napi_get_undefined(env, &result[0]);
            result[PARAM1] = rngplrResult;
        }
    } else {
        MEDIA_LOGE("Failed to get system tone player!");
        napi_value message = nullptr;
        napi_create_string_utf8(env, "GetSystemTonePlayer Error: Operation is not supported or failed",
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

static napi_value Init(napi_env env, napi_value exports)
{
    SystemSoundManagerNapi::Init(env, exports);
    AudioRendererInfoNapi::Init(env, exports);
    RingtoneOptionsNapi::Init(env, exports);
    RingtonePlayerNapi::Init(env, exports);
    SystemToneOptionsNapi::Init(env, exports);
    SystemTonePlayerNapi::Init(env, exports);

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