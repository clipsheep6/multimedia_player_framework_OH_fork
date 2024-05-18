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

#include "ringtone_player_napi.h"

#include "audio_renderer_info_napi.h"
#include "avplayer_napi.h"
#include "media_log.h"

using namespace std;

namespace {
/* Constants for array index */
const int32_t PARAM0 = 0;
const int32_t PARAM1 = 1;

/* Constants for array size */
const int32_t ARGS_ONE = 1;
const int32_t ARGS_TWO = 2;

const std::string AUDIO_INTERRUPT_CALLBACK_NAME = "audioInterrupt";

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RingtonePlayerNapi"};

const int SUCCESS = 0;
}

namespace OHOS {
namespace Media {
static const std::map<RingtoneState, std::string> STATEMAP = {
    {STATE_INVALID, AVPlayerState::STATE_ERROR},
    {STATE_NEW, AVPlayerState::STATE_INITIALIZED},
    {STATE_PREPARED, AVPlayerState::STATE_PREPARED},
    {STATE_RUNNING, AVPlayerState::STATE_PLAYING},
    {STATE_STOPPED, AVPlayerState::STATE_STOPPED},
    {STATE_RELEASED, AVPlayerState::STATE_RELEASED},
    {STATE_PAUSED, AVPlayerState::STATE_PAUSED},
};

thread_local napi_ref RingtonePlayerNapi::sConstructor_ = nullptr;
shared_ptr<RingtonePlayer> RingtonePlayerNapi::sRingtonePlayer_ = nullptr;

RingtonePlayerNapi::RingtonePlayerNapi() : env_(nullptr) {}

RingtonePlayerNapi::~RingtonePlayerNapi() = default;

static napi_value ThrowErrorAndReturn(napi_env env, int32_t errCode, const std::string &errMessage)
{
    RingtoneCommonNapi::ThrowError(env, errCode, errMessage);
    return nullptr;
}

napi_value RingtonePlayerNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;

    napi_property_descriptor ringtone_player_prop[] = {
        DECLARE_NAPI_FUNCTION("getTitle", GetTitle),
        DECLARE_NAPI_FUNCTION("getAudioRendererInfo", GetAudioRendererInfo),
        DECLARE_NAPI_FUNCTION("configure", Configure),
        DECLARE_NAPI_FUNCTION("start", Start),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_FUNCTION("on", On),
        DECLARE_NAPI_FUNCTION("off", Off),
        DECLARE_NAPI_GETTER("state", GetAudioState)
    };

    status = napi_define_class(env, RINGTONE_PLAYER_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH,
        RingtonePlayerNapiConstructor, nullptr, sizeof(ringtone_player_prop) / sizeof(ringtone_player_prop[0]),
        ringtone_player_prop, &ctorObj);
    if (status == napi_ok) {
        if (napi_create_reference(env, ctorObj, refCount, &sConstructor_) == napi_ok) {
            status = napi_set_named_property(env, exports, RINGTONE_PLAYER_NAPI_CLASS_NAME.c_str(), ctorObj);
            if (status == napi_ok) {
                return exports;
            }
        }
    }

    return nullptr;
}

napi_value RingtonePlayerNapi::RingtonePlayerNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    status = napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr);
    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<RingtonePlayerNapi> obj = std::make_unique<RingtonePlayerNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            if (obj->sRingtonePlayer_ != nullptr) {
                obj->ringtonePlayer_ = move(obj->sRingtonePlayer_);
            } else {
                MEDIA_LOGE("Failed to create sRingtonePlayer_ instance.");
                return result;
            }

            if (obj->ringtonePlayer_ != nullptr && obj->callbackNapi_ == nullptr) {
                obj->callbackNapi_ = std::make_shared<RingtonePlayerCallbackNapi>(env);
                CHECK_AND_RETURN_RET_LOG(obj->callbackNapi_ != nullptr, result, "No memory");
                int32_t ret = obj->ringtonePlayer_->SetRingtonePlayerInterruptCallback(obj->callbackNapi_);
                MEDIA_LOGI("AudioRendererNapi::Construct SetRendererCallback %{public}s",
                    ret == 0 ? "succeess" : "failed");
            }

            status = napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()),
                RingtonePlayerNapi::RingtonePlayerNapiDestructor, nullptr, nullptr);
            if (status == napi_ok) {
                obj.release();
                return thisVar;
            } else {
                MEDIA_LOGE("Failed to wrap the native rngplyrmngr object with JS.");
            }
        }
    }

    return result;
}

void RingtonePlayerNapi::RingtonePlayerNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint)
{
    RingtonePlayerNapi *ringtonePlayerHelper = reinterpret_cast<RingtonePlayerNapi*>(nativeObject);
    if (ringtonePlayerHelper != nullptr) {
        ringtonePlayerHelper->~RingtonePlayerNapi();
    }
}

napi_value RingtonePlayerNapi::GetRingtonePlayerInstance(napi_env env, shared_ptr<RingtonePlayer> &ringtonePlayer)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value ctor;

    status = napi_get_reference_value(env, sConstructor_, &ctor);
    if (status == napi_ok) {
        sRingtonePlayer_ = ringtonePlayer;
        status = napi_new_instance(env, ctor, 0, nullptr, &result);
        if (status == napi_ok) {
            return result;
        } else {
            MEDIA_LOGE("GetRingtonePlayerInstance: New instance could not be obtained.");
        }
    }

    napi_get_undefined(env, &result);
    return result;
}

void RingtonePlayerNapi::CommonAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<RingtonePlayerAsyncContext *>(data);
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

void RingtonePlayerNapi::GetTitleAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto context = static_cast<RingtonePlayerAsyncContext *>(data);
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

napi_value RingtonePlayerNapi::GetTitle(napi_env env, napi_callback_info info)
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
        MEDIA_LOGE("GetTitle: Failed to retrieve details about the callback");
        return result;
    }

    NAPI_ASSERT(env, argc <= ARGS_ONE, "GetTitle: requires 1 parameter maximum");
    std::unique_ptr<RingtonePlayerAsyncContext> asyncContext = std::make_unique<RingtonePlayerAsyncContext>();
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
                RingtonePlayerAsyncContext *context = static_cast<RingtonePlayerAsyncContext *>(data);
                context->title = context->objectInfo->ringtonePlayer_->GetTitle();
                context->status = SUCCESS;
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

void RingtonePlayerNapi::GetAudioRendererInfoAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto context = static_cast<RingtonePlayerAsyncContext *>(data);
    napi_value getRendererInfoCallback = nullptr;
    napi_value retVal = nullptr;
    napi_value valueParam = nullptr;
    napi_value result[2] = {};

    if (!context->status) {
        unique_ptr<AudioStandard::AudioRendererInfo> audioRendererInfo =
            make_unique<AudioStandard::AudioRendererInfo>();
        audioRendererInfo->contentType = context->contentType;
        audioRendererInfo->streamUsage = context->streamUsage;
        audioRendererInfo->rendererFlags = context->rendererFlags;

        valueParam = AudioRendererInfoNapi::CreateAudioRendererInfoWrapper(env, audioRendererInfo);
        napi_get_undefined(env, &result[PARAM0]);
        result[PARAM1] = valueParam;
    } else {
        napi_value message = nullptr;
        napi_create_string_utf8(env, "GetRendererInfo Error: Operation is not supported or failed",
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
        napi_get_reference_value(env, context->callbackRef, &getRendererInfoCallback);
        napi_call_function(env, nullptr, getRendererInfoCallback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, context->callbackRef);
    }
    napi_delete_async_work(env, context->work);

    delete context;
    context = nullptr;
}

napi_value RingtonePlayerNapi::GetAudioRendererInfo(napi_env env, napi_callback_info info)
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
        "GetAudioRendererInfo: Failed to retrieve details about the callback");

    NAPI_ASSERT(env, argc <= ARGS_ONE, "GetAudioRendererInfo: requires 1 parameter maximum");
    std::unique_ptr<RingtonePlayerAsyncContext> asyncContext = std::make_unique<RingtonePlayerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[PARAM0], &valueType);
            CHECK_AND_RETURN_RET_LOG(valueType == napi_function, result,
                "GetAudioRendererInfo: the param type is not napi_function");
            napi_create_reference(env, argv[PARAM0], refCount, &asyncContext->callbackRef);
        } else {
            napi_create_promise(env, &asyncContext->deferred, &result);
        }

        napi_create_string_utf8(env, "GetAudioRendererInfo", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource, AsyncGetAudioRendererInfo,
            GetAudioRendererInfoAsyncCallbackComplete, static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_LOGE("GetAudioRendererInfo: Failed to get create async work");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void RingtonePlayerNapi::AsyncGetAudioRendererInfo(napi_env env, void *data)
{
    RingtonePlayerAsyncContext *context = static_cast<RingtonePlayerAsyncContext *>(data);
    AudioStandard::AudioRendererInfo rendererInfo;
    context->status = context->objectInfo->ringtonePlayer_->GetAudioRendererInfo(rendererInfo);
    if (context->status == SUCCESS) {
        context->contentType = rendererInfo.contentType;
        context->streamUsage = rendererInfo.streamUsage;
        context->rendererFlags  = rendererInfo.rendererFlags;
    }
}

napi_value RingtonePlayerNapi::Configure(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value resource = nullptr;
    napi_value property = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
    const int32_t refCount = 1;
    double volume = 1.0;

    napi_status status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && thisVar != nullptr, result, "Configure: napi_get_cb_info failed");

    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameters maximum");
    std::unique_ptr<RingtonePlayerAsyncContext> asyncContext = std::make_unique<RingtonePlayerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[PARAM0], &valueType);
        if (valueType == napi_object) {
            if ((napi_get_named_property(env, argv[PARAM0], "volume", &property) != napi_ok)
                || napi_get_value_double(env, property, &volume) != napi_ok) {
                NAPI_ASSERT(env, false, "missing volume properties");
            }
            asyncContext->volume = (float)volume;

            if ((napi_get_named_property(env, argv[PARAM0], "loop", &property) != napi_ok)
                || napi_get_value_bool(env, property, &asyncContext->loop) != napi_ok) {
                NAPI_ASSERT(env, false, "missing loop properties");
            }
        }

        if (argc == ARGS_TWO) {
            napi_typeof(env, argv[PARAM1], &valueType);
            if (valueType == napi_function) {
                napi_create_reference(env, argv[PARAM1], refCount, &asyncContext->callbackRef);
            }
        } else {
            napi_create_promise(env, &asyncContext->deferred, &result);
        }

        napi_create_string_utf8(env, "Configure", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource, AsyncConfigure,
            CommonAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_LOGE("Configure: Failed to create async work");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

void RingtonePlayerNapi::AsyncConfigure(napi_env env, void *data)
{
    RingtonePlayerAsyncContext* context = static_cast<RingtonePlayerAsyncContext*>(data);
    context->status = context->objectInfo->ringtonePlayer_->Configure(context->volume, context->loop);
}

napi_value RingtonePlayerNapi::Start(napi_env env, napi_callback_info info)
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
        MEDIA_LOGE("Start: Failed to retrieve details about the callback");
        return result;
    }

    NAPI_ASSERT(env, argc <= ARGS_ONE, "Start: requires 1 parameter maximum");
    std::unique_ptr<RingtonePlayerAsyncContext> asyncContext = std::make_unique<RingtonePlayerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[PARAM0], &valueType);
            CHECK_AND_RETURN_RET_LOG(valueType == napi_function, result,
                "Start: the param type is not napi_function");
            napi_create_reference(env, argv[PARAM0], refCount, &asyncContext->callbackRef);
        } else {
            napi_create_promise(env, &asyncContext->deferred, &result);
        }

        napi_create_string_utf8(env, "Start", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource,
            [](napi_env env, void* data) {
                RingtonePlayerAsyncContext* context = static_cast<RingtonePlayerAsyncContext*>(data);
                context->status = context->objectInfo->ringtonePlayer_->Start();
            },
            CommonAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            MEDIA_LOGE("Start: Failed to get create async work");
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work(env, asyncContext->work);
            asyncContext.release();
        }
    }

    return result;
}

napi_value RingtonePlayerNapi::Stop(napi_env env, napi_callback_info info)
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
        MEDIA_LOGE("Stop: Failed to retrieve details about the callback");
        return result;
    }

    NAPI_ASSERT(env, argc <= ARGS_ONE, "Stop: requires 1 parameter maximum");
    std::unique_ptr<RingtonePlayerAsyncContext> asyncContext = std::make_unique<RingtonePlayerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[PARAM0], &valueType);
            CHECK_AND_RETURN_RET_LOG(valueType == napi_function, result,
                "Stop: the param type is not napi_function");
            napi_create_reference(env, argv[PARAM0], refCount, &asyncContext->callbackRef);
        } else {
            napi_create_promise(env, &asyncContext->deferred, &result);
        }

        napi_create_string_utf8(env, "Stop", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource,
            [](napi_env env, void* data) {
                RingtonePlayerAsyncContext* context = static_cast<RingtonePlayerAsyncContext*>(data);
                context->status = context->objectInfo->ringtonePlayer_->Stop();
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

napi_value RingtonePlayerNapi::Release(napi_env env, napi_callback_info info)
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
    std::unique_ptr<RingtonePlayerAsyncContext> asyncContext = std::make_unique<RingtonePlayerAsyncContext>();
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
                RingtonePlayerAsyncContext* context = static_cast<RingtonePlayerAsyncContext*>(data);
                context->status = context->objectInfo->ringtonePlayer_->Release();
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

napi_value RingtonePlayerNapi::GetAudioState(napi_env env, napi_callback_info info)
{
    napi_status status;
    size_t argc = 0;
    napi_value thisVar = nullptr;
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    status = napi_get_cb_info(env, info, &argc, nullptr, &thisVar, nullptr);
    if (status != napi_ok || thisVar == nullptr) {
        MEDIA_LOGE("GetAudioState: fail to napi_get_cb_info");
        return result;
    }

    std::string curState = AVPlayerState::STATE_ERROR;
    std::unique_ptr<RingtonePlayerAsyncContext> asyncContext
        = std::make_unique<RingtonePlayerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo && asyncContext->objectInfo->ringtonePlayer_ != nullptr) {
        RingtoneState ringtoneState_ = asyncContext->objectInfo->ringtonePlayer_->GetRingtoneState();
        if (STATEMAP.find(ringtoneState_) != STATEMAP.end()) {
            curState = STATEMAP.at(ringtoneState_);
        }
    }
    napi_create_string_utf8(env, curState.c_str(), NAPI_AUTO_LENGTH, &result);

    return result;
}

napi_value RingtonePlayerNapi::On(napi_env env, napi_callback_info info)
{
    const size_t requireArgc = 2;
    size_t argc = 3;

    napi_value argv[requireArgc + 1] = {nullptr, nullptr, nullptr};
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, argv, &jsThis, nullptr);
    if (status != napi_ok) {
        ThrowErrorAndReturn(env, NAPI_ERR_SYSTEM, "system err");
    }
    if (argc < requireArgc) {
        ThrowErrorAndReturn(env, NAPI_ERR_INPUT_INVALID, "mandatory parameters are left unspecified");
    }

    napi_valuetype argvType = napi_undefined;
    napi_typeof(env, argv[0], &argvType);
    if (argvType != napi_string) {
        ThrowErrorAndReturn(env, NAPI_ERR_INPUT_INVALID,
            "incorrect parameter types: The type of eventType must be string");
    }

    std::string callbackName = RingtoneCommonNapi::GetStringArgument(env, argv[0]);
    MEDIA_LOGI("RingtonePlayerNapi: On callbackName: %{public}s", callbackName.c_str());

    napi_valuetype callbackFunction = napi_undefined;
    if (argc == requireArgc) {
        napi_typeof(env, argv[1], &callbackFunction);
        if (callbackFunction != napi_function) {
            ThrowErrorAndReturn(env, NAPI_ERR_INPUT_INVALID,
                "incorrect parameter types: The type of callback must be function");
        }
    } else {
        napi_valuetype paramArg1 = napi_undefined;
        napi_typeof(env, argv[1], &paramArg1);
        napi_valuetype expectedValType = napi_number;  // Default. Reset it with 'callbackName' if check, if required.
        if (paramArg1 != expectedValType) {
            ThrowErrorAndReturn(env, NAPI_ERR_INPUT_INVALID,
                "incorrect parameter types: The parameter must be number");
        }

        const int32_t arg2 = 2;
        napi_typeof(env, argv[arg2], &callbackFunction);
        if (callbackFunction != napi_function) {
            ThrowErrorAndReturn(env, NAPI_ERR_INPUT_INVALID,
                "incorrect parameter types: The type of callback must be function");
        }
    }

    return RegisterCallback(env, jsThis, argv, callbackName);
}

napi_value RingtonePlayerNapi::RegisterCallback(napi_env env, napi_value jsThis, napi_value* argv,
    const std::string& cbName)
{
    RingtonePlayerNapi *ringtonePlayerNapi = nullptr;
    napi_status status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&ringtonePlayerNapi));
    if (status != napi_ok) {
        ThrowErrorAndReturn(env, NAPI_ERR_SYSTEM, "system err");
    }
    if (ringtonePlayerNapi == nullptr) {
        ThrowErrorAndReturn(env, NAPI_ERR_NO_MEMORY, "no memory");
    }
    if (ringtonePlayerNapi->ringtonePlayer_ == nullptr) {
        ThrowErrorAndReturn(env, NAPI_ERR_NO_MEMORY, "no memory");
    }

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    if (!cbName.compare(AUDIO_INTERRUPT_CALLBACK_NAME)) {
        result = RegisterRingtonePlayerCallback(env, argv, cbName, ringtonePlayerNapi);
    } else {
        bool unknownCallback = true;
        if (unknownCallback) {
            ThrowErrorAndReturn(env, NAPI_ERR_INVALID_PARAM,
                "parameter verification failed: The param of type is not supported");
        }
    }

    return result;
}

napi_value RingtonePlayerNapi::RegisterRingtonePlayerCallback(napi_env env, napi_value* argv,
    const std::string& cbName, RingtonePlayerNapi *ringtonePlayerNapi)
{
    if (ringtonePlayerNapi->callbackNapi_ == nullptr) {
        ThrowErrorAndReturn(env, NAPI_ERR_NO_MEMORY, "no memory");
    }

    std::shared_ptr<RingtonePlayerCallbackNapi> cb =
        std::static_pointer_cast<RingtonePlayerCallbackNapi>(ringtonePlayerNapi->callbackNapi_);
    cb->SaveCallbackReference(cbName, argv[PARAM1]);

    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    return result;
}

napi_value RingtonePlayerNapi::Off(napi_env env, napi_callback_info info)
{
    const size_t requireArgc = 2;
    size_t argc = 3;

    napi_value argv[requireArgc + 1] = {nullptr, nullptr, nullptr};
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, argv, &jsThis, nullptr);
    if (status != napi_ok) {
        ThrowErrorAndReturn(env, NAPI_ERR_SYSTEM, "system err");
    }
    if (argc > requireArgc) {
        ThrowErrorAndReturn(env, NAPI_ERR_INPUT_INVALID, "mandatory parameters are left unspecified");
    }

    napi_valuetype callbackNameType = napi_undefined;
    napi_typeof(env, argv[0], &callbackNameType);
    if (callbackNameType != napi_string) {
        ThrowErrorAndReturn(env, NAPI_ERR_INPUT_INVALID,
            "incorrect parameter types: The type of eventType must be string");
    }

    string callbackName = RingtoneCommonNapi::GetStringArgument(env, argv[0]);
    MEDIA_LOGI("Off callbackName: %{public}s", callbackName.c_str());

    return UnregisterCallback(env, jsThis, callbackName);
}

napi_value RingtonePlayerNapi::UnregisterCallback(napi_env env, napi_value jsThis, const string& cbName)
{
    RingtonePlayerNapi *ringtonePlayerNapi = nullptr;
    napi_status status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&ringtonePlayerNapi));
    if (status != napi_ok) {
        ThrowErrorAndReturn(env, NAPI_ERR_SYSTEM, "system err");
    }
    if (ringtonePlayerNapi == nullptr) {
        ThrowErrorAndReturn(env, NAPI_ERR_NO_MEMORY, "no memory");
    }
    if (ringtonePlayerNapi->ringtonePlayer_ == nullptr) {
        ThrowErrorAndReturn(env, NAPI_ERR_NO_MEMORY, "no memory");
    }

    if (!cbName.compare(AUDIO_INTERRUPT_CALLBACK_NAME)) {
        UnregisterRingtonePlayerCallback(ringtonePlayerNapi, cbName);
    } else {
        bool unknownCallback = true;
        if (unknownCallback) {
            ThrowErrorAndReturn(env, NAPI_ERR_INVALID_PARAM,
                "parameter verification failed: The param of type is not supported");
        }
    }

    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    return result;
}

void RingtonePlayerNapi::UnregisterRingtonePlayerCallback(RingtonePlayerNapi *ringtonePlayerNapi, const string& cbName)
{
    CHECK_AND_RETURN_LOG(ringtonePlayerNapi->callbackNapi_ != nullptr, "ringtonePlayerCallbackNapi is nullptr");

    shared_ptr<RingtonePlayerCallbackNapi> cb =
        static_pointer_cast<RingtonePlayerCallbackNapi>(ringtonePlayerNapi->callbackNapi_);
    cb->RemoveCallbackReference(cbName);
}
} // namespace Media
} // namespace OHOS