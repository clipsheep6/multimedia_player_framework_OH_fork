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

#include "audio_haptic_player_napi.h"

#include "media_log.h"

namespace {
/* Constants for array index */
const int32_t PARAM0 = 0;
const int32_t PARAM1 = 1;

/* Constants for array size */
const int32_t ARGS_ONE = 1;
const int32_t ARGS_TWO = 2;

const std::string AUDIO_INTERRUPT_CALLBACK_NAME = "audioInterrupt";
const std::string END_OF_STREAM_CALLBACK_NAME = "endOfStream";

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioHapticPlayerNapi"};
}

namespace OHOS {
namespace Media {
thread_local napi_ref AudioHapticPlayerNapi::sConstructor_ = nullptr;
std::shared_ptr<AudioHapticPlayer> AudioHapticPlayerNapi::sAudioHapticPlayer_ = nullptr;

AudioHapticPlayerNapi::AudioHapticPlayerNapi() : env_(nullptr) {}

AudioHapticPlayerNapi::~AudioHapticPlayerNapi() = default;

napi_value AudioHapticPlayerNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;

    napi_property_descriptor audioHapticPlayerProp[] = {
        DECLARE_NAPI_FUNCTION("isMuted", IsMuted),
        DECLARE_NAPI_FUNCTION("start", Start),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_FUNCTION("on", On),
        DECLARE_NAPI_FUNCTION("off", Off),
    };

    status = napi_define_class(env, AUDIO_HAPTIC_PLAYER_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor,
        nullptr, sizeof(audioHapticPlayerProp) / sizeof(audioHapticPlayerProp[0]), audioHapticPlayerProp, &ctorObj);
    if (status == napi_ok) {
        if (napi_create_reference(env, ctorObj, refCount, &sConstructor_) == napi_ok) {
            status = napi_set_named_property(env, exports, AUDIO_HAPTIC_PLAYER_NAPI_CLASS_NAME.c_str(), ctorObj);
            if (status == napi_ok) {
                return exports;
            }
        }
    }

    return nullptr;
}

napi_value AudioHapticPlayerNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    status = napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr);
    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<AudioHapticPlayerNapi> obj = std::make_unique<AudioHapticPlayerNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            if (obj->sAudioHapticPlayer_ != nullptr) {
                obj->audioHapticPlayer_ = move(obj->sAudioHapticPlayer_);
            } else {
                MEDIA_LOGE("Failed to create sAudioHapticPlayer_ instance.");
                return result;
            }

            if (obj->audioHapticPlayer_ != nullptr && obj->callbackNapi_ == nullptr) {
                obj->callbackNapi_ = std::make_shared<AudioHapticPlayerCallbackNapi>(env);
                CHECK_AND_RETURN_RET_LOG(obj->callbackNapi_ != nullptr, result, "No memory");
                int32_t ret = obj->audioHapticPlayer_->SetAudioHapticPlayerCallback(obj->callbackNapi_);
                MEDIA_LOGI("Constructor: SetAudioHapticPlayerCallback %{public}s",
                    ret == 0 ? "succeess" : "failed");
            }

            status = napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()),
                AudioHapticPlayerNapi::Destructor, nullptr, nullptr);
            if (status == napi_ok) {
                obj.release();
                return thisVar;
            } else {
                MEDIA_LOGE("Failed to wrap the native audioHapticPlayer object with JS.");
            }
        }
    }

    return result;
}

void AudioHapticPlayerNapi::Destructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    AudioHapticPlayerNapi *audioHapticPlayerHelper = reinterpret_cast<AudioHapticPlayerNapi*>(nativeObject);
    if (audioHapticPlayerHelper != nullptr) {
        audioHapticPlayerHelper->~AudioHapticPlayerNapi();
    }
}

napi_value AudioHapticPlayerNapi::CreatePlayerInstance(napi_env env,
    std::shared_ptr<AudioHapticPlayer> &audioHapticPlayer)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value ctor;

    status = napi_get_reference_value(env, sConstructor_, &ctor);
    if (status == napi_ok) {
        sAudioHapticPlayer_ = audioHapticPlayer;
        status = napi_new_instance(env, ctor, 0, nullptr, &result);
        if (status == napi_ok) {
            return result;
        } else {
            MEDIA_LOGE("CreatePlayerInstance: New instance could not be obtained.");
        }
    }

    napi_get_undefined(env, &result);
    return result;
}

napi_value AudioHapticPlayerNapi::IsMuted(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_boolean(env, false, &result);

    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    napi_status status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && thisVar != nullptr, result, "IsMuted: napi_get_cb_info fail");
    if (argc != ARGS_ONE) {
        MEDIA_LOGE("IsMuted: requires 1 parameters");
        AudioHapticCommonNapi::ThrowError(env, NAPI_ERR_INPUT_INVALID, "mandatory parameters are left unspecified");
        return result;
    }

    void *native = nullptr;
    status = napi_unwrap(env, thisVar, &native);
    auto *audioHapticPlayerNapi = reinterpret_cast<AudioHapticPlayerNapi *>(native);
    if (status != napi_ok || audioHapticPlayerNapi == nullptr) {
        MEDIA_LOGE("IsMuted: unwrap failure!");
        return result;
    }

    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, argv[PARAM0], &valueType);
    int32_t jsAudioHapticType = -1;
    if (valueType == napi_number) {
        napi_get_value_int32(env, argv[PARAM0], &jsAudioHapticType);
    }
    if (!IsLegalAudioHapticType(jsAudioHapticType)) {
        MEDIA_LOGE("IsMuted: the param type mismatch");
        AudioHapticCommonNapi::ThrowError(env, NAPI_ERR_INPUT_INVALID,
            "parameter verification failed: The param of type must be enum AudioHapticType");
        return result;
    }

    AudioHapticType audioHapticType = static_cast<AudioHapticType>(jsAudioHapticType);
    bool isMuted = audioHapticPlayerNapi->audioHapticPlayer_->IsMuted(audioHapticType);
    napi_get_boolean(env, isMuted, &result);

    return result;
}

bool AudioHapticPlayerNapi::IsLegalAudioHapticType(int32_t audioHapticType)
{
    switch (audioHapticType) {
        case AUDIO_HAPTIC_TYPE_AUDIO:
        case AUDIO_HAPTIC_TYPE_HAPTIC:
            return true;
        default:
            break;
    }
    MEDIA_LOGE("IsLegalAudioHapticType: audioHapticType %{public}d is invalid", audioHapticType);
    return false;
}

void AudioHapticPlayerNapi::CommonAsyncCallbackComp(napi_env env, napi_status status, void *data)
{
    auto context = static_cast<AudioHapticPlayerAsyncContext *>(data);
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
    }
    napi_delete_async_work(env, context->work);

    delete context;
    context = nullptr;
}

napi_value AudioHapticPlayerNapi::Start(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    napi_status status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && thisVar != nullptr, result, "Start: napi_get_cb_info fail");
    if (argc != 0) {
        MEDIA_LOGE("Start: requires 0 parameters");
        AudioHapticCommonNapi::ThrowError(env, NAPI_ERR_INPUT_INVALID);
        return result;
    }

    std::unique_ptr<AudioHapticPlayerAsyncContext> asyncContext = std::make_unique<AudioHapticPlayerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &result);
        napi_create_string_utf8(env, "Start", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource,
            [](napi_env env, void *data) {
                AudioHapticPlayerAsyncContext *context = static_cast<AudioHapticPlayerAsyncContext*>(data);
                context->status = context->objectInfo->audioHapticPlayer_->Start();
            },
            CommonAsyncCallbackComp, static_cast<void*>(asyncContext.get()), &asyncContext->work);
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

napi_value AudioHapticPlayerNapi::Stop(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    napi_status status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && thisVar != nullptr, result, "Stop: napi_get_cb_info fail");
    if (argc != 0) {
        MEDIA_LOGE("Stop: requires 0 parameters");
        AudioHapticCommonNapi::ThrowError(env, NAPI_ERR_INPUT_INVALID);
        return result;
    }

    std::unique_ptr<AudioHapticPlayerAsyncContext> asyncContext = std::make_unique<AudioHapticPlayerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &result);
        napi_create_string_utf8(env, "Stop", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource,
            [](napi_env env, void *data) {
                AudioHapticPlayerAsyncContext *context = static_cast<AudioHapticPlayerAsyncContext*>(data);
                context->status = context->objectInfo->audioHapticPlayer_->Stop();
            },
            CommonAsyncCallbackComp, static_cast<void*>(asyncContext.get()), &asyncContext->work);
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

napi_value AudioHapticPlayerNapi::Release(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value resource = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    napi_status status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    napi_get_undefined(env, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && thisVar != nullptr, result, "Release: napi_get_cb_info fail");
    if (argc != 0) {
        MEDIA_LOGE("Release: requires 0 parameters");
        AudioHapticCommonNapi::ThrowError(env, NAPI_ERR_INPUT_INVALID);
        return result;
    }

    std::unique_ptr<AudioHapticPlayerAsyncContext> asyncContext = std::make_unique<AudioHapticPlayerAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &result);
        napi_create_string_utf8(env, "Release", NAPI_AUTO_LENGTH, &resource);
        status = napi_create_async_work(env, nullptr, resource,
            [](napi_env env, void *data) {
                AudioHapticPlayerAsyncContext *context = static_cast<AudioHapticPlayerAsyncContext*>(data);
                context->status = context->objectInfo->audioHapticPlayer_->Release();
            },
            CommonAsyncCallbackComp, static_cast<void*>(asyncContext.get()), &asyncContext->work);
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

napi_value AudioHapticPlayerNapi::On(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {nullptr};
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, argv, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, result, "On: napi_get_cb_info fail");
    if (argc != ARGS_TWO) {
        MEDIA_LOGE("On: requires 2 parameters");
        AudioHapticCommonNapi::ThrowError(env, NAPI_ERR_INPUT_INVALID);
        return result;
    }

    napi_valuetype argvType = napi_undefined;
    napi_typeof(env, argv[PARAM0], &argvType);
    if (argvType != napi_string) {
        MEDIA_LOGE("On: the first parameter must be napi_string");
        AudioHapticCommonNapi::ThrowError(env, NAPI_ERR_INPUT_INVALID);
        return result;
    }
    std::string callbackName = AudioHapticCommonNapi::GetStringArgument(env, argv[0]);
    MEDIA_LOGI("On: callbackName: %{public}s", callbackName.c_str());

    napi_typeof(env, argv[PARAM1], &argvType);
    if (argvType != napi_function) {
        MEDIA_LOGE("On: the second parameter must be napi_function");
        AudioHapticCommonNapi::ThrowError(env, NAPI_ERR_INPUT_INVALID);
        return result;
    }

    return RegisterCallback(env, jsThis, argv, callbackName);
}

napi_value AudioHapticPlayerNapi::RegisterCallback(napi_env env, napi_value jsThis, napi_value* argv,
    const std::string& cbName)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    AudioHapticPlayerNapi *audioHapticPlayerNapi = nullptr;
    napi_status status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&audioHapticPlayerNapi));
    if (status != napi_ok || audioHapticPlayerNapi == nullptr || audioHapticPlayerNapi->audioHapticPlayer_ == nullptr) {
        MEDIA_LOGE("RegisterCallback: Failed to get audioHapticPlayer_");
        AudioHapticCommonNapi::ThrowError(env, NAPI_ERR_OPERATE_NOT_ALLOWED);
        return result;
    }

    if (!cbName.compare(AUDIO_INTERRUPT_CALLBACK_NAME) ||
        !cbName.compare(END_OF_STREAM_CALLBACK_NAME)) {
        result = RegisterAudioHapticPlayerCallback(env, argv, cbName, audioHapticPlayerNapi);
    } else {
        MEDIA_LOGE("RegisterCallback: the callback name: %{public}s is not supported", cbName.c_str());
        AudioHapticCommonNapi::ThrowError(env, NAPI_ERR_INPUT_INVALID);
        return result;
    }

    return result;
}

napi_value AudioHapticPlayerNapi::RegisterAudioHapticPlayerCallback(napi_env env, napi_value* argv,
    const std::string& cbName, AudioHapticPlayerNapi *audioHapticPlayerNapi)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    if (audioHapticPlayerNapi->callbackNapi_ == nullptr) {
        MEDIA_LOGE("RegisterAudioHapticPlayerCallback: callbackNapi_ is null");
        AudioHapticCommonNapi::ThrowError(env, NAPI_ERR_OPERATE_NOT_ALLOWED);
        return result;
    }
    audioHapticPlayerNapi->callbackNapi_->SaveCallbackReference(cbName, argv[PARAM1]);

    return result;
}

napi_value AudioHapticPlayerNapi::Off(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {nullptr};
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, argv, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, result, "Off: napi_get_cb_info fail");
    if (argc != ARGS_ONE && argc != ARGS_TWO) {
        MEDIA_LOGE("Off: requires 1 or 2 parameters");
        AudioHapticCommonNapi::ThrowError(env, NAPI_ERR_INPUT_INVALID);
        return result;
    }

    napi_valuetype argvType = napi_undefined;
    napi_typeof(env, argv[PARAM0], &argvType);
    if (argvType != napi_string) {
        MEDIA_LOGE("Off: the parameter must be napi_string");
        AudioHapticCommonNapi::ThrowError(env, NAPI_ERR_INPUT_INVALID);
        return result;
    }
    std::string callbackName = AudioHapticCommonNapi::GetStringArgument(env, argv[0]);
    MEDIA_LOGI("Off: callbackName: %{public}s", callbackName.c_str());

    return UnregisterCallback(env, jsThis, callbackName);
}

napi_value AudioHapticPlayerNapi::UnregisterCallback(napi_env env, napi_value jsThis, const std::string &cbName)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    AudioHapticPlayerNapi *audioHapticPlayerNapi = nullptr;
    napi_status status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&audioHapticPlayerNapi));
    if (status != napi_ok || audioHapticPlayerNapi == nullptr || audioHapticPlayerNapi->audioHapticPlayer_ == nullptr) {
        MEDIA_LOGE("UnregisterCallback: Failed to get audioHapticPlayer_");
        AudioHapticCommonNapi::ThrowError(env, NAPI_ERR_OPERATE_NOT_ALLOWED);
        return result;
    }

    if (!cbName.compare(AUDIO_INTERRUPT_CALLBACK_NAME) ||
        !cbName.compare(END_OF_STREAM_CALLBACK_NAME)) {
        result = UnregisterAudioHapticPlayerCallback(env, cbName, audioHapticPlayerNapi);
    } else {
        MEDIA_LOGE("UnregisterCallback: the callback name: %{public}s is not supported", cbName.c_str());
        AudioHapticCommonNapi::ThrowError(env, NAPI_ERR_INPUT_INVALID);
        return result;
    }

    return result;
}

napi_value AudioHapticPlayerNapi::UnregisterAudioHapticPlayerCallback(napi_env env, const std::string& cbName,
    AudioHapticPlayerNapi *audioHapticPlayerNapi)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    if (audioHapticPlayerNapi->callbackNapi_ == nullptr) {
        MEDIA_LOGE("RegisterAudioHapticPlayerCallback: callbackNapi_ is null");
        AudioHapticCommonNapi::ThrowError(env, NAPI_ERR_OPERATE_NOT_ALLOWED);
        return result;
    }
    audioHapticPlayerNapi->callbackNapi_->RemoveCallbackReference(cbName);

    return result;
}
} // namespace Media
} // namespace OHOS