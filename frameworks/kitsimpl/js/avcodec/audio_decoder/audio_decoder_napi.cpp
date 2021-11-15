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

#include "audio_decoder_napi.h"
#include <unistd.h>
#include <fcntl.h>
#include <climits>
#include "audio_decoder_callback_napi.h"
#include "media_log.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "string_ex.h"
#include "common_napi.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioDecoderNapi"};
}

namespace OHOS {
namespace Media {
napi_ref AudioDecoderNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "AudioDecoder";

AudioDecoderNapi::AudioDecoderNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AudioDecoderNapi::~AudioDecoderNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
    if (taskQue_ != nullptr) {
        (void)taskQue_->Stop();
    }
    callbackNapi_ = nullptr;
    audioDecoderImpl_ = nullptr;
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value AudioDecoderNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("configure", Configure),
        DECLARE_NAPI_FUNCTION("prepare", Prepare),
        DECLARE_NAPI_FUNCTION("start", Start),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("flush", Flush),
        DECLARE_NAPI_FUNCTION("reset", Reset),
        DECLARE_NAPI_FUNCTION("on", On)
    };
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAudioDecoder", CreateAudioDecoder),
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define AudioDecoder class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value AudioDecoderNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value jsThis = nullptr;
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return result;
    }

    AudioDecoderNapi *audioDecoderNapi = new(std::nothrow) AudioDecoderNapi();
    CHECK_AND_RETURN_RET_LOG(audioDecoderNapi != nullptr, nullptr, "No memory");

    audioDecoderNapi->env_ = env;
    audioDecoderNapi->audioDecoderImpl_ = AudioDecoderFactory::CreateByCodecName("todo");
    CHECK_AND_RETURN_RET_LOG(audioDecoderNapi->audioDecoderImpl_ != nullptr, nullptr, "No memory");

    audioDecoderNapi->taskQue_ = std::make_unique<TaskQueue>("AudioDecoderNapi");
    (void)audioDecoderNapi->taskQue_->Start();

    audioDecoderNapi->callbackNapi_ = std::make_shared<AudioDecoderCallbackNapi>(env);
    (void)audioDecoderNapi->audioDecoderImpl_->SetCallback(audioDecoderNapi->callbackNapi_);

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(audioDecoderNapi),
        AudioDecoderNapi::Destructor, nullptr, &(audioDecoderNapi->wrapper_));
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
        delete audioDecoderNapi;
        MEDIA_LOGE("Failed to wrap native instance");
        return result;
    }

    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void AudioDecoderNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        delete reinterpret_cast<AudioDecoderNapi *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}

napi_value AudioDecoderNapi::CreateAudioDecoder(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value constructor = nullptr;
    napi_status status = napi_get_reference_value(env, constructor_, &constructor);
    if (status != napi_ok) {
        MEDIA_LOGE("Failed to get the representation of constructor object");
        napi_get_undefined(env, &result);
        return result;
    }

    status = napi_new_instance(env, constructor, 0, nullptr, &result);
    if (status != napi_ok) {
        MEDIA_LOGE("new instance fail");
        napi_get_undefined(env, &result);
        return result;
    }

    MEDIA_LOGD("CreateAudioDecoder success");
    return result;
}

napi_value AudioDecoderNapi::Configure(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};

    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioDecoderNapi *decoderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&decoderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && decoderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_object) {
        decoderNapi->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    CHECK_AND_RETURN_RET_LOG(decoderNapi->audioDecoderImpl_ != nullptr, undefinedResult, "No memory");
    CHECK_AND_RETURN_RET_LOG(decoderNapi->taskQue_ != nullptr, undefinedResult, "No TaskQue");
    auto task = std::make_shared<TaskHandler<void>>([napi = decoderNapi]() {
        // todo
        Format format;
        int32_t ret = napi->audioDecoderImpl_->Configure(format);
        if (ret == MSERR_OK) {
            napi->StateCallback(START_CALLBACK_NAME);
        } else {
            napi->ErrorCallback(MSERR_EXT_UNKNOWN);
        }
        MEDIA_LOGD("Configure success");
    });
    (void)decoderNapi->taskQue_->EnqueueTask(task);
    return undefinedResult;
}

napi_value AudioDecoderNapi::Prepare(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioDecoderNapi *audioDecoderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&audioDecoderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && audioDecoderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(audioDecoderNapi->audioDecoderImpl_ != nullptr, undefinedResult, "No memory");
    CHECK_AND_RETURN_RET_LOG(audioDecoderNapi->taskQue_ != nullptr, undefinedResult, "No TaskQue");
    auto task = std::make_shared<TaskHandler<void>>([napi = audioDecoderNapi]() {
        int32_t ret = napi->audioDecoderImpl_->Prepare();
        if (ret == MSERR_OK) {
            napi->StateCallback(START_CALLBACK_NAME);
        } else {
            napi->ErrorCallback(MSERR_EXT_UNKNOWN);
        }
        MEDIA_LOGD("Prepare success");
    });
    (void)audioDecoderNapi->taskQue_->EnqueueTask(task);
    return undefinedResult;
}

napi_value AudioDecoderNapi::Start(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioDecoderNapi *audioDecoderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&audioDecoderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && audioDecoderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(audioDecoderNapi->audioDecoderImpl_ != nullptr, undefinedResult, "No memory");
    CHECK_AND_RETURN_RET_LOG(audioDecoderNapi->taskQue_ != nullptr, undefinedResult, "No TaskQue");
    auto task = std::make_shared<TaskHandler<void>>([napi = audioDecoderNapi]() {
        int32_t ret = napi->audioDecoderImpl_->Start();
        if (ret == MSERR_OK) {
            napi->StateCallback(START_CALLBACK_NAME);
        } else {
            napi->ErrorCallback(MSERR_EXT_UNKNOWN);
        }
        MEDIA_LOGD("Start success");
    });
    (void)audioDecoderNapi->taskQue_->EnqueueTask(task);
    return undefinedResult;
}

napi_value AudioDecoderNapi::Stop(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioDecoderNapi *audioDecoderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&audioDecoderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && audioDecoderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(audioDecoderNapi->audioDecoderImpl_ != nullptr, undefinedResult, "No memory");
    CHECK_AND_RETURN_RET_LOG(audioDecoderNapi->taskQue_ != nullptr, undefinedResult, "No TaskQue");
    auto task = std::make_shared<TaskHandler<void>>([napi = audioDecoderNapi]() {
        int32_t ret = napi->audioDecoderImpl_->Stop();
        if (ret == MSERR_OK) {
            napi->StateCallback(STOP_CALLBACK_NAME);
        } else {
            napi->ErrorCallback(MSERR_EXT_UNKNOWN);
        }
        MEDIA_LOGD("Stop success");
    });
    (void)audioDecoderNapi->taskQue_->EnqueueTask(task);
    return undefinedResult;
}

napi_value AudioDecoderNapi::Flush(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioDecoderNapi *audioDecoderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&audioDecoderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && audioDecoderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(audioDecoderNapi->audioDecoderImpl_ != nullptr, undefinedResult, "No memory");
    CHECK_AND_RETURN_RET_LOG(audioDecoderNapi->taskQue_ != nullptr, undefinedResult, "No TaskQue");
    auto task = std::make_shared<TaskHandler<void>>([napi = audioDecoderNapi]() {
        int32_t ret = napi->audioDecoderImpl_->Flush();
        if (ret == MSERR_OK) {
            napi->StateCallback(RESET_CALLBACK_NAME);
        } else {
            napi->ErrorCallback(MSERR_EXT_UNKNOWN);
        }
        MEDIA_LOGD("Flush success");
    });
    (void)audioDecoderNapi->taskQue_->EnqueueTask(task);
    return undefinedResult;
}

napi_value AudioDecoderNapi::Reset(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioDecoderNapi *audioDecoderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&audioDecoderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && audioDecoderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(audioDecoderNapi->audioDecoderImpl_ != nullptr, undefinedResult, "No memory");
    CHECK_AND_RETURN_RET_LOG(audioDecoderNapi->taskQue_ != nullptr, undefinedResult, "No TaskQue");
    auto task = std::make_shared<TaskHandler<void>>([napi = audioDecoderNapi]() {
        int32_t ret = napi->audioDecoderImpl_->Reset();
        if (ret == MSERR_OK) {
            napi->StateCallback(RESET_CALLBACK_NAME);
        } else {
            napi->ErrorCallback(MSERR_EXT_UNKNOWN);
        }
        MEDIA_LOGD("Reset success");
    });
    (void)audioDecoderNapi->taskQue_->EnqueueTask(task);
    return undefinedResult;
}

napi_value AudioDecoderNapi::On(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    static const size_t MIN_REQUIRED_ARG_COUNT = 2;
    size_t argCount = MIN_REQUIRED_ARG_COUNT;
    napi_value args[MIN_REQUIRED_ARG_COUNT] = { nullptr };
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr || args[1] == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    AudioDecoderNapi *audioDecoderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&audioDecoderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && audioDecoderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        audioDecoderNapi->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGD("callbackName: %{public}s", callbackName.c_str());

    CHECK_AND_RETURN_RET_LOG(audioDecoderNapi->callbackNapi_ != nullptr, undefinedResult, "callbackNapi_ is nullptr");
    auto cb = std::static_pointer_cast<AudioDecoderCallbackNapi>(audioDecoderNapi->callbackNapi_);
    cb->SaveCallbackReference(callbackName, args[1]);
    return undefinedResult;
}

void AudioDecoderNapi::ErrorCallback(MediaServiceExtErrCode errCode)
{
    if (callbackNapi_ != nullptr) {
        auto napiCb = std::static_pointer_cast<AudioDecoderCallbackNapi>(callbackNapi_);
        napiCb->SendErrorCallback(errCode);
    }
}

void AudioDecoderNapi::StateCallback(const std::string &callbackName)
{
    if (callbackNapi_ != nullptr) {
        auto napiCb = std::static_pointer_cast<AudioDecoderCallbackNapi>(callbackNapi_);
        napiCb->SendStateCallback(callbackName);
    }
}
}  // namespace Media
}  // namespace OHOS
