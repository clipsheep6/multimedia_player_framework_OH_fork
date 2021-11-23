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

#include "audio_encoder_napi.h"
#include <unistd.h>
#include <fcntl.h>
#include <climits>
#include "audio_encoder_callback_napi.h"
#include "media_log.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "string_ex.h"
#include "common_napi.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioEncoderNapi"};
}

namespace OHOS {
namespace Media {
napi_ref AudioEncoderNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "AudioEncoder";

AudioEncoderNapi::AudioEncoderNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AudioEncoderNapi::~AudioEncoderNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
    if (taskQue_ != nullptr) {
        (void)taskQue_->Stop();
    }
    callbackNapi_ = nullptr;
    audioEncoderImpl_ = nullptr;
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value AudioEncoderNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("configure", Configure),
        DECLARE_NAPI_FUNCTION("prepare", Prepare),
        DECLARE_NAPI_FUNCTION("start", Start),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("flush", Flush),
        DECLARE_NAPI_FUNCTION("reset", Reset),
        DECLARE_NAPI_FUNCTION("on", On),
        DECLARE_NAPI_FUNCTION("queueInputBuffer", QueueInputBuffer),
        DECLARE_NAPI_FUNCTION("releaseOutputBuffer", ReleaseOutputBuffer),
        DECLARE_NAPI_FUNCTION("setParameter", SetParameter)
    };
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAudioEncoderByName", CreateAudioEncoderByName),
        DECLARE_NAPI_STATIC_FUNCTION("createAudioEncoderByCodec", CreateAudioEncoderByCodec),
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define AudioEncoder class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value AudioEncoderNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    napi_value jsThis = nullptr;
    size_t argCount = 2;
    napi_value args[2] = {0};
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && args[0] != nullptr && args[1] != nullptr, undefined, "Unknown");

    AudioEncoderNapi *audioEncoderNapi = new(std::nothrow) AudioEncoderNapi();
    CHECK_AND_RETURN_RET_LOG(audioEncoderNapi != nullptr, undefined, "No memory");

    audioEncoderNapi->env_ = env;
    std::string str = CommonNapi::GetStringArgument(env, args[0]);
    int32_t creator = -1;
    (void)napi_get_value_int32(env, args[1], &creator);
    switch (static_cast<AudioEncoderCreator>(creator)) {
        case AUDIO_ENCODER_CREATOR_BY_NAME:
            audioEncoderNapi->audioEncoderImpl_ = AudioEncoderFactory::CreateByEncoderName(str);
            break;
        case AUDIO_ENCODER_CREATOR_BY_CODEC:
            audioEncoderNapi->audioEncoderImpl_ = AudioEncoderFactory::CreateByMimeType(str);
            break;
        default:
            break;
    }

    if (audioEncoderNapi->audioEncoderImpl_ == nullptr) {
        delete audioEncoderNapi;
        MEDIA_LOGE("Failed to create native instance");
        return undefined;
    }

    audioEncoderNapi->taskQue_ = std::make_unique<TaskQueue>("AudioEncoderNapi");
    (void)audioEncoderNapi->taskQue_->Start();

    audioEncoderNapi->callbackNapi_ = std::make_shared<AudioEncoderCallbackNapi>(env, audioEncoderNapi->audioEncoderImpl_);
    (void)audioEncoderNapi->audioEncoderImpl_->SetCallback(audioEncoderNapi->callbackNapi_);

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(audioEncoderNapi),
        AudioEncoderNapi::Destructor, nullptr, &(audioEncoderNapi->wrapper_));
    if (status != napi_ok) {
        delete audioEncoderNapi;
        MEDIA_LOGE("Failed to wrap native instance");
        return undefined;
    }

    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void AudioEncoderNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        delete reinterpret_cast<AudioEncoderNapi *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}

napi_value AudioEncoderNapi::CreateAudioEncoderByName(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value constructor = nullptr;
    napi_status status = napi_get_reference_value(env, constructor_, &constructor);
    if (status != napi_ok) {
        MEDIA_LOGE("Failed to get the representation of constructor object");
        napi_get_undefined(env, &result);
        return result;
    }

    size_t argCount = 1;
    napi_value args[2] = { nullptr };
    status = napi_get_cb_info(env, info, &argCount, args, nullptr, nullptr);
    if (status != napi_ok || args[0] == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        napi_get_undefined(env, &result);
        return result;
    }

    status = napi_create_int32(env, static_cast<int32_t>(AUDIO_ENCODER_CREATOR_BY_NAME), &args[1]);
    if (status != napi_ok || args[1] == nullptr) {
        MEDIA_LOGE("Failed to create int32");
        napi_get_undefined(env, &result);
        return result;
    }

    status = napi_new_instance(env, constructor, 0, args, &result);
    if (status != napi_ok) {
        MEDIA_LOGE("Fail to new instance");
        napi_get_undefined(env, &result);
        return result;
    }

    MEDIA_LOGD("CreateAudioEncoderByName success");
    return result;
}

napi_value AudioEncoderNapi::CreateAudioEncoderByCodec(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value constructor = nullptr;
    napi_status status = napi_get_reference_value(env, constructor_, &constructor);
    if (status != napi_ok) {
        MEDIA_LOGE("Failed to get the representation of constructor object");
        napi_get_undefined(env, &result);
        return result;
    }

    size_t argCount = 1;
    napi_value args[2] = { nullptr };
    status = napi_get_cb_info(env, info, &argCount, args, nullptr, nullptr);
    if (status != napi_ok || args[0] == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        napi_get_undefined(env, &result);
        return result;
    }

    status = napi_create_int32(env, static_cast<int32_t>(AUDIO_ENCODER_CREATOR_BY_CODEC), &args[1]);
    if (status != napi_ok || args[1] == nullptr) {
        MEDIA_LOGE("Failed to create int32");
        napi_get_undefined(env, &result);
        return result;
    }

    status = napi_new_instance(env, constructor, 0, args, &result);
    if (status != napi_ok) {
        MEDIA_LOGE("Fail to new instance");
        napi_get_undefined(env, &result);
        return result;
    }

    MEDIA_LOGD("CreateAudioEncoderByCodec success");
    return result;
}

napi_value AudioEncoderNapi::Configure(napi_env env, napi_callback_info info)
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

    AudioEncoderNapi *encoderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&encoderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && encoderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_object) {
        encoderNapi->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    CHECK_AND_RETURN_RET_LOG(encoderNapi->audioEncoderImpl_ != nullptr, undefinedResult, "No memory");
    CHECK_AND_RETURN_RET_LOG(encoderNapi->taskQue_ != nullptr, undefinedResult, "No TaskQue");
    auto task = std::make_shared<TaskHandler<void>>([napi = encoderNapi]() {
        // todo
        Format format;
        int32_t ret = napi->audioEncoderImpl_->Configure(format);
        if (ret == MSERR_OK) {
            napi->StateCallback(START_CALLBACK_NAME);
        } else {
            napi->ErrorCallback(MSERR_EXT_UNKNOWN);
        }
        MEDIA_LOGD("Configure success");
    });
    (void)encoderNapi->taskQue_->EnqueueTask(task);
    return undefinedResult;
}

napi_value AudioEncoderNapi::Prepare(napi_env env, napi_callback_info info)
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

    AudioEncoderNapi *audioEncoderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&audioEncoderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && audioEncoderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(audioEncoderNapi->audioEncoderImpl_ != nullptr, undefinedResult, "No memory");
    CHECK_AND_RETURN_RET_LOG(audioEncoderNapi->taskQue_ != nullptr, undefinedResult, "No TaskQue");
    auto task = std::make_shared<TaskHandler<void>>([napi = audioEncoderNapi]() {
        int32_t ret = napi->audioEncoderImpl_->Prepare();
        if (ret == MSERR_OK) {
            napi->StateCallback(START_CALLBACK_NAME);
        } else {
            napi->ErrorCallback(MSERR_EXT_UNKNOWN);
        }
        MEDIA_LOGD("Prepare success");
    });
    (void)audioEncoderNapi->taskQue_->EnqueueTask(task);
    return undefinedResult;
}

napi_value AudioEncoderNapi::Start(napi_env env, napi_callback_info info)
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

    AudioEncoderNapi *audioEncoderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&audioEncoderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && audioEncoderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(audioEncoderNapi->audioEncoderImpl_ != nullptr, undefinedResult, "No memory");
    CHECK_AND_RETURN_RET_LOG(audioEncoderNapi->taskQue_ != nullptr, undefinedResult, "No TaskQue");
    auto task = std::make_shared<TaskHandler<void>>([napi = audioEncoderNapi]() {
        int32_t ret = napi->audioEncoderImpl_->Start();
        if (ret == MSERR_OK) {
            napi->StateCallback(START_CALLBACK_NAME);
        } else {
            napi->ErrorCallback(MSERR_EXT_UNKNOWN);
        }
        MEDIA_LOGD("Start success");
    });
    (void)audioEncoderNapi->taskQue_->EnqueueTask(task);
    return undefinedResult;
}

napi_value AudioEncoderNapi::Stop(napi_env env, napi_callback_info info)
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

    AudioEncoderNapi *audioEncoderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&audioEncoderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && audioEncoderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(audioEncoderNapi->audioEncoderImpl_ != nullptr, undefinedResult, "No memory");
    CHECK_AND_RETURN_RET_LOG(audioEncoderNapi->taskQue_ != nullptr, undefinedResult, "No TaskQue");
    auto task = std::make_shared<TaskHandler<void>>([napi = audioEncoderNapi]() {
        int32_t ret = napi->audioEncoderImpl_->Stop();
        if (ret == MSERR_OK) {
            napi->StateCallback(STOP_CALLBACK_NAME);
        } else {
            napi->ErrorCallback(MSERR_EXT_UNKNOWN);
        }
        MEDIA_LOGD("Stop success");
    });
    (void)audioEncoderNapi->taskQue_->EnqueueTask(task);
    return undefinedResult;
}

napi_value AudioEncoderNapi::Flush(napi_env env, napi_callback_info info)
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

    AudioEncoderNapi *audioEncoderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&audioEncoderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && audioEncoderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(audioEncoderNapi->audioEncoderImpl_ != nullptr, undefinedResult, "No memory");
    CHECK_AND_RETURN_RET_LOG(audioEncoderNapi->taskQue_ != nullptr, undefinedResult, "No TaskQue");
    auto task = std::make_shared<TaskHandler<void>>([napi = audioEncoderNapi]() {
        int32_t ret = napi->audioEncoderImpl_->Flush();
        if (ret == MSERR_OK) {
            napi->StateCallback(RESET_CALLBACK_NAME);
        } else {
            napi->ErrorCallback(MSERR_EXT_UNKNOWN);
        }
        MEDIA_LOGD("Flush success");
    });
    (void)audioEncoderNapi->taskQue_->EnqueueTask(task);
    return undefinedResult;
}

napi_value AudioEncoderNapi::Reset(napi_env env, napi_callback_info info)
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

    AudioEncoderNapi *audioEncoderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&audioEncoderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && audioEncoderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(audioEncoderNapi->audioEncoderImpl_ != nullptr, undefinedResult, "No memory");
    CHECK_AND_RETURN_RET_LOG(audioEncoderNapi->taskQue_ != nullptr, undefinedResult, "No TaskQue");
    auto task = std::make_shared<TaskHandler<void>>([napi = audioEncoderNapi]() {
        int32_t ret = napi->audioEncoderImpl_->Reset();
        if (ret == MSERR_OK) {
            napi->StateCallback(RESET_CALLBACK_NAME);
        } else {
            napi->ErrorCallback(MSERR_EXT_UNKNOWN);
        }
        MEDIA_LOGD("Reset success");
    });
    (void)audioEncoderNapi->taskQue_->EnqueueTask(task);
    return undefinedResult;
}

napi_value AudioEncoderNapi::On(napi_env env, napi_callback_info info)
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

    AudioEncoderNapi *audioEncoderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&audioEncoderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && audioEncoderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        audioEncoderNapi->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGD("callbackName: %{public}s", callbackName.c_str());

    CHECK_AND_RETURN_RET_LOG(audioEncoderNapi->callbackNapi_ != nullptr, undefinedResult, "callbackNapi_ is nullptr");
    auto cb = std::static_pointer_cast<AudioEncoderCallbackNapi>(audioEncoderNapi->callbackNapi_);
    cb->SaveCallbackReference(callbackName, args[1]);
    return undefinedResult;
}

napi_value AudioEncoderNapi::QueueInputBuffer(napi_env env, napi_callback_info info)
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

    std::unique_ptr<AudioEncoderAsyncContext> asyncContext = std::make_unique<AudioEncoderAsyncContext>();
    CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, undefinedResult, "No memory");
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->audioEncoderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncContext->audioEncoderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    const int32_t refCount = 1;
    for (size_t i = 0; i < argCount; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, args[i], &valueType);
        if (i == 0 && valueType == napi_object) {
            int32_t timeStampMs = -1;
            CommonNapi::GetPropertyInt32(env, args[0], "timeStampMs", timeStampMs);
            int32_t flag = 0;
            CommonNapi::GetPropertyInt32(env, args[0], "flag", flag);
            napi_value buffer = nullptr;
            status = napi_get_named_property(env, args[0], "buffer", &buffer);
            CHECK_AND_RETURN_RET_LOG(status == napi_ok && buffer != nullptr, undefinedResult, "Unknown");
            int32_t index = 0;
            CommonNapi::GetPropertyInt32(env, buffer, "index", index);
            int32_t size = 0;
            CommonNapi::GetPropertyInt32(env, buffer, "size", size);
            int32_t offset = 0;
            CommonNapi::GetPropertyInt32(env, buffer, "offset", offset);
            asyncContext->index = index;
            asyncContext->flag = static_cast<AVCodecBufferFlag>(flag);
            AVCodecBufferInfo bufferInfo;
            const int32_t MsToUs = 1000;
            bufferInfo.presentationTimeUs = timeStampMs*MsToUs;
            bufferInfo.size = size;
            bufferInfo.offset = offset;
            asyncContext->info = bufferInfo;
        } else if (i == 1 && valueType == napi_function) {
            napi_create_reference(env, args[i], refCount, &asyncContext->callbackRef);
        } else {
            MEDIA_LOGE("Mismatch");
        }
    }
    napi_value result = nullptr;
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &result);
    } else {
        napi_get_undefined(env, &result);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "QueueInputBuffer", NAPI_AUTO_LENGTH, &resource);

    status = napi_create_async_work(
        env, nullptr, resource,
        [](napi_env env, void *data) {
            auto context = static_cast<AudioEncoderAsyncContext *>(data);
            context->status = context->audioEncoderNapi->audioEncoderImpl_->
                QueueInputBuffer(context->index, context->info, context->flag);
        },
        SetFunctionAsyncCallbackComplete, static_cast<void *>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        result = nullptr;
    } else {
        status = napi_queue_async_work(env, asyncContext->work);
        if (status == napi_ok) {
            asyncContext.release();
        } else {
            result = nullptr;
        }
    }

    return result;
}

napi_value AudioEncoderNapi::ReleaseOutputBuffer(napi_env env, napi_callback_info info)
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

    std::unique_ptr<AudioEncoderAsyncContext> asyncContext = std::make_unique<AudioEncoderAsyncContext>();
    CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, undefinedResult, "No memory");
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->audioEncoderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncContext->audioEncoderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    const int32_t refCount = 1;
    for (size_t i = 0; i < argCount; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, args[i], &valueType);
        if (i == 0 && valueType == napi_object) {
            napi_value buffer = nullptr;
            status = napi_get_named_property(env, args[0], "buffer", &buffer);
            CHECK_AND_RETURN_RET_LOG(status == napi_ok && buffer != nullptr, undefinedResult, "Unknown");
            int32_t index = 0;
            CommonNapi::GetPropertyInt32(env, buffer, "index", index);
            asyncContext->index = index;
        } else if (i == 1 && valueType == napi_function) {
            napi_create_reference(env, args[i], refCount, &asyncContext->callbackRef);
        } else {
            MEDIA_LOGE("Mismatch");
        }
    }
    napi_value result = nullptr;
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &result);
    } else {
        napi_get_undefined(env, &result);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "ReleaseOutputBuffer", NAPI_AUTO_LENGTH, &resource);

    status = napi_create_async_work(
        env, nullptr, resource,
        [](napi_env env, void *data) {
            auto context = static_cast<AudioEncoderAsyncContext *>(data);
            context->status = context->audioEncoderNapi->audioEncoderImpl_->
                ReleaseOutputBuffer(context->index);
        },
        SetFunctionAsyncCallbackComplete, static_cast<void *>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        result = nullptr;
    } else {
        status = napi_queue_async_work(env, asyncContext->work);
        if (status == napi_ok) {
            asyncContext.release();
        } else {
            result = nullptr;
        }
    }

    return result;
}

napi_value AudioEncoderNapi::SetParameter(napi_env env, napi_callback_info info)
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

    std::unique_ptr<AudioEncoderAsyncContext> asyncContext = std::make_unique<AudioEncoderAsyncContext>();
    CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, undefinedResult, "No memory");
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->audioEncoderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncContext->audioEncoderNapi != nullptr, undefinedResult,
        "Failed to retrieve instance");

    const int32_t refCount = 1;
    for (size_t i = 0; i < argCount; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, args[i], &valueType);
        if (i == 0 && valueType == napi_object) {
            // todo
        } else if (i == 1 && valueType == napi_function) {
            napi_create_reference(env, args[i], refCount, &asyncContext->callbackRef);
        } else {
            MEDIA_LOGE("Mismatch");
        }
    }
    napi_value result = nullptr;
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &result);
    } else {
        napi_get_undefined(env, &result);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "SetParameter", NAPI_AUTO_LENGTH, &resource);

    status = napi_create_async_work(
        env, nullptr, resource,
        [](napi_env env, void *data) {
            //auto context = static_cast<AudioEncoderAsyncContext *>(data);
            // todo
            //context->status = context->audioEncoderNapi->audioEncoderImpl_->
            //    SetParameter(context->index);
        },
        SetFunctionAsyncCallbackComplete, static_cast<void *>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        result = nullptr;
    } else {
        status = napi_queue_async_work(env, asyncContext->work);
        if (status == napi_ok) {
            asyncContext.release();
        } else {
            result = nullptr;
        }
    }

    return result;
}

void AudioEncoderNapi::CommonCallbackRoutine(napi_env env, AudioEncoderAsyncContext* &asyncContext,
                                                    const napi_value &valueParam)
{
    static const size_t MIN_REQUIRED_ARG_COUNT = 2;
    napi_value result[MIN_REQUIRED_ARG_COUNT] = {0};
    napi_value retVal;

    if (!asyncContext->status) {
        napi_get_undefined(env, &result[0]);
        result[1] = valueParam;
    } else {
        napi_value message = nullptr;
        napi_create_string_utf8(env, "Error, Operation not supported or Failed", NAPI_AUTO_LENGTH, &message);
        napi_create_error(env, nullptr, message, &result[0]);
        napi_get_undefined(env, &result[1]);
    }

    if (asyncContext->deferred) {
        if (!asyncContext->status) {
            napi_resolve_deferred(env, asyncContext->deferred, result[1]);
        } else {
            napi_reject_deferred(env, asyncContext->deferred, result[0]);
        }
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, MIN_REQUIRED_ARG_COUNT, result, &retVal);
        napi_delete_reference(env, asyncContext->callbackRef);
    }
    napi_delete_async_work(env, asyncContext->work);

    delete asyncContext;
    asyncContext = nullptr;
}

void AudioEncoderNapi::SetFunctionAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto asyncContext = static_cast<AudioEncoderAsyncContext*>(data);
    napi_value valueParam = nullptr;

    if (asyncContext != nullptr) {
        if (!asyncContext->status) {
            napi_get_undefined(env, &valueParam);
        }
        CommonCallbackRoutine(env, asyncContext, valueParam);
    } else {
        MEDIA_LOGE("AudioEncoderAsyncContext* is nulltr");
    }
}

void AudioEncoderNapi::ErrorCallback(MediaServiceExtErrCode errCode)
{
    if (callbackNapi_ != nullptr) {
        auto napiCb = std::static_pointer_cast<AudioEncoderCallbackNapi>(callbackNapi_);
        napiCb->SendErrorCallback(errCode);
    }
}

void AudioEncoderNapi::StateCallback(const std::string &callbackName)
{
    if (callbackNapi_ != nullptr) {
        auto napiCb = std::static_pointer_cast<AudioEncoderCallbackNapi>(callbackNapi_);
        napiCb->SendStateCallback(callbackName);
    }
}
}  // namespace Media
}  // namespace OHOS
