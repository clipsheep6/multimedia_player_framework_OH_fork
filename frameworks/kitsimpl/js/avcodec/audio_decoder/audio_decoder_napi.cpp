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
#include <climits>
#include "audio_decoder_callback_napi.h"
#include "avcodec_napi_utils.h"
#include "media_log.h"
#include "media_errors.h"
#include "common_napi.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioDecoderNapi"};
}

namespace OHOS {
namespace Media {
napi_ref AudioDecoderNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "AudioDecodeProcessor";

AudioDecoderNapi::AudioDecoderNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AudioDecoderNapi::~AudioDecoderNapi()
{
    callback_ = nullptr;
    adec_ = nullptr;
    if (wrap_ != nullptr) {
        napi_delete_reference(env_, wrap_);
    }
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
        DECLARE_NAPI_FUNCTION("queueInput", QueueInput),
        DECLARE_NAPI_FUNCTION("releaseOutput", ReleaseOutput),
        DECLARE_NAPI_FUNCTION("setParameter", SetParameter),
        DECLARE_NAPI_FUNCTION("getOutputMediaDescription", GetOutputMediaDescription),
        DECLARE_NAPI_FUNCTION("getAudioDecoderCaps", GetAudioDecoderCaps),
        DECLARE_NAPI_FUNCTION("on", On),
    };
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAudioDecoderByMime", CreateAudioDecoderByMime),
        DECLARE_NAPI_STATIC_FUNCTION("createAudioDecoderByName", CreateAudioDecoderByName),
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
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    napi_value jsThis = nullptr;
    size_t argCount = 2;
    napi_value args[2] = {0};
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && args[0] != nullptr && args[1] != nullptr, undefined);

    AudioDecoderNapi *adecNapi = new(std::nothrow) AudioDecoderNapi();
    CHECK_AND_RETURN_RET(adecNapi != nullptr, undefined);

    adecNapi->env_ = env;
    std::string name = CommonNapi::GetStringArgument(env, args[0]);

    int32_t useMime = 0;
    status = napi_get_value_int32(env, args[1], &useMime);
    CHECK_AND_RETURN_RET(status == napi_ok, undefined);
    if (useMime == 1) {
        adecNapi->adec_ = AudioDecoderFactory::CreateByMime(name);
    } else {
        adecNapi->adec_ = AudioDecoderFactory::CreateByName(name);
    }
    CHECK_AND_RETURN_RET(adecNapi->adec_ != nullptr, undefined);

    if (adecNapi->callback_ == nullptr) {
        adecNapi->callback_ = std::make_shared<AudioDecoderCallbackNapi>(env, adecNapi->adec_);
        (void)adecNapi->adec_->SetCallback(adecNapi->callback_);
    }

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(adecNapi),
        AudioDecoderNapi::Destructor, nullptr, &(adecNapi->wrap_));
    if (status != napi_ok) {
        delete adecNapi;
        MEDIA_LOGE("Failed to wrap native instance");
        return undefined;
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

napi_value AudioDecoderNapi::CreateAudioDecoderByMime(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter CreateAudioDecoderByMime");
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && jsThis != nullptr, undefined);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>();
    CHECK_AND_RETURN_RET(asyncCtx != nullptr, undefined);
    napi_get_undefined(env, &asyncCtx->asyncRet);

    napi_valuetype valueType = napi_undefined;
    CHECK_AND_RETURN_RET(args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok &&
        valueType == napi_string, undefined);
    asyncCtx->pluginName = CommonNapi::GetStringArgument(env, args[0]);
    asyncCtx->createByMime = 1;

    valueType = napi_undefined;
    if (args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok && valueType == napi_function) {
        napi_create_reference(env, args[1], 1, &asyncCtx->callbackRef);
    }

    if (asyncCtx->callbackRef == nullptr) {
        napi_create_promise(env, &asyncCtx->deferred, &undefined);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "CreateAudioDecoderByMime", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, AudioDecoderNapi::AsyncCreator,
        AudioDecoderNapi::CompleteAsyncFunc, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return undefined;
}

napi_value AudioDecoderNapi::CreateAudioDecoderByName(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter CreateAudioDecoderByName");
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && jsThis != nullptr, undefined);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>();
    CHECK_AND_RETURN_RET(asyncCtx != nullptr, undefined);
    napi_get_undefined(env, &asyncCtx->asyncRet);

    napi_valuetype valueType = napi_undefined;
    CHECK_AND_RETURN_RET(args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok &&
        valueType == napi_string, undefined);
    asyncCtx->pluginName = CommonNapi::GetStringArgument(env, args[0]);
    asyncCtx->createByMime = 0;

    valueType = napi_undefined;
    if (args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok && valueType == napi_function) {
        napi_create_reference(env, args[1], 1, &asyncCtx->callbackRef);
    }

    if (asyncCtx->callbackRef == nullptr) {
        napi_create_promise(env, &asyncCtx->deferred, &undefined);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "CreateAudioDecoderByName", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, AudioDecoderNapi::AsyncCreator,
        AudioDecoderNapi::CompleteAsyncFunc, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return undefined;
}

napi_value AudioDecoderNapi::Configure(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter Configure");
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && jsThis != nullptr, undefined);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>();
    CHECK_AND_RETURN_RET(asyncCtx != nullptr, undefined);
    napi_get_undefined(env, &asyncCtx->asyncRet);

    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));
    CHECK_AND_RETURN_RET(status == napi_ok && asyncCtx->napi != nullptr, undefined);
    CHECK_AND_RETURN_RET(asyncCtx->napi->adec_ != nullptr, undefined);

    napi_valuetype valueType = napi_undefined;
    CHECK_AND_RETURN_RET(args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok &&
        valueType == napi_object, undefined);
    CHECK_AND_RETURN_RET(AVCodecNapiUtil::ExtractMediaFormat(env, args[0], asyncCtx->format) == true, undefined);

    valueType = napi_undefined;
    if (args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok && valueType == napi_function) {
        napi_create_reference(env, args[1], 1, &asyncCtx->callbackRef);
    }

    if (asyncCtx->callbackRef == nullptr) {
        napi_create_promise(env, &asyncCtx->deferred, &undefined);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Configure", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AudioDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr || asyncCtx->napi == nullptr || asyncCtx->napi->adec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->adec_->Configure(asyncCtx->format) != MSERR_OK) {
                asyncCtx->SignError(MSERR_UNKNOWN, "Failed to Configure");
            } else {
                asyncCtx->success = true;
            }
        },
        AudioDecoderNapi::CompleteAsyncFunc, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return undefined;
}

napi_value AudioDecoderNapi::Prepare(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter Prepare");
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && jsThis != nullptr, undefined);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>();
    CHECK_AND_RETURN_RET(asyncCtx != nullptr, undefined);
    napi_get_undefined(env, &asyncCtx->asyncRet);

    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));
    CHECK_AND_RETURN_RET(status == napi_ok && asyncCtx->napi != nullptr, undefined);
    CHECK_AND_RETURN_RET(asyncCtx->napi->adec_ != nullptr, undefined);

    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        napi_create_reference(env, args[0], 1, &asyncCtx->callbackRef);
    }

    if (asyncCtx->callbackRef == nullptr) {
        napi_create_promise(env, &asyncCtx->deferred, &undefined);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Prepare", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AudioDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr || asyncCtx->napi == nullptr || asyncCtx->napi->adec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->adec_->Prepare() != MSERR_OK) {
                asyncCtx->SignError(MSERR_UNKNOWN, "Failed to Prepare");
            } else {
                asyncCtx->success = true;
            }
        },
        AudioDecoderNapi::CompleteAsyncFunc, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return undefined;
}

napi_value AudioDecoderNapi::Start(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter Start");
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && jsThis != nullptr, undefined);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>();
    CHECK_AND_RETURN_RET(asyncCtx != nullptr, undefined);
    napi_get_undefined(env, &asyncCtx->asyncRet);

    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));
    CHECK_AND_RETURN_RET(status == napi_ok && asyncCtx->napi != nullptr, undefined);
    CHECK_AND_RETURN_RET(asyncCtx->napi->adec_ != nullptr, undefined);

    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        napi_create_reference(env, args[0], 1, &asyncCtx->callbackRef);
    }

    if (asyncCtx->callbackRef == nullptr) {
        napi_create_promise(env, &asyncCtx->deferred, &undefined);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Start", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AudioDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr || asyncCtx->napi == nullptr || asyncCtx->napi->adec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->adec_->Start() != MSERR_OK) {
                asyncCtx->SignError(MSERR_UNKNOWN, "Failed to Start");
            } else {
                asyncCtx->success = true;
            }
        },
        AudioDecoderNapi::CompleteAsyncFunc, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return undefined;
}

napi_value AudioDecoderNapi::Stop(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter Stop");
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && jsThis != nullptr, undefined);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>();
    CHECK_AND_RETURN_RET(asyncCtx != nullptr, undefined);
    napi_get_undefined(env, &asyncCtx->asyncRet);

    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));
    CHECK_AND_RETURN_RET(status == napi_ok && asyncCtx->napi != nullptr, undefined);
    CHECK_AND_RETURN_RET(asyncCtx->napi->adec_ != nullptr, undefined);

    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        napi_create_reference(env, args[0], 1, &asyncCtx->callbackRef);
    }

    if (asyncCtx->callbackRef == nullptr) {
        napi_create_promise(env, &asyncCtx->deferred, &undefined);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Stop", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AudioDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr || asyncCtx->napi == nullptr || asyncCtx->napi->adec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->adec_->Stop() != MSERR_OK) {
                asyncCtx->SignError(MSERR_UNKNOWN, "Failed to Stop");
            } else {
                asyncCtx->success = true;
            }
        },
        AudioDecoderNapi::CompleteAsyncFunc, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return undefined;
}

napi_value AudioDecoderNapi::Flush(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter Flush");
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && jsThis != nullptr, undefined);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>();
    CHECK_AND_RETURN_RET(asyncCtx != nullptr, undefined);
    napi_get_undefined(env, &asyncCtx->asyncRet);

    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));
    CHECK_AND_RETURN_RET(status == napi_ok && asyncCtx->napi != nullptr, undefined);
    CHECK_AND_RETURN_RET(asyncCtx->napi->adec_ != nullptr, undefined);

    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        napi_create_reference(env, args[0], 1, &asyncCtx->callbackRef);
    }

    if (asyncCtx->callbackRef == nullptr) {
        napi_create_promise(env, &asyncCtx->deferred, &undefined);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Flush", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AudioDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr || asyncCtx->napi == nullptr || asyncCtx->napi->adec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->adec_->Flush() != MSERR_OK) {
                asyncCtx->SignError(MSERR_UNKNOWN, "Failed to Flush");
            } else {
                asyncCtx->success = true;
            }
        },
        AudioDecoderNapi::CompleteAsyncFunc, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return undefined;
}

napi_value AudioDecoderNapi::Reset(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("Enter Reset");
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && jsThis != nullptr, undefined);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>();
    CHECK_AND_RETURN_RET(asyncCtx != nullptr, undefined);
    napi_get_undefined(env, &asyncCtx->asyncRet);

    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));
    CHECK_AND_RETURN_RET(status == napi_ok && asyncCtx->napi != nullptr, undefined);
    CHECK_AND_RETURN_RET(asyncCtx->napi->adec_ != nullptr, undefined);

    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        napi_create_reference(env, args[0], 1, &asyncCtx->callbackRef);
    }

    if (asyncCtx->callbackRef == nullptr) {
        napi_create_promise(env, &asyncCtx->deferred, &undefined);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Reset", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AudioDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr || asyncCtx->napi == nullptr || asyncCtx->napi->adec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->adec_->Reset() != MSERR_OK) {
                asyncCtx->SignError(MSERR_UNKNOWN, "Failed to Reset");
            } else {
                asyncCtx->success = true;
            }
        },
        AudioDecoderNapi::CompleteAsyncFunc, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return undefined;
}

napi_value AudioDecoderNapi::QueueInput(napi_env env, napi_callback_info info)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && jsThis != nullptr, undefined);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>();
    CHECK_AND_RETURN_RET(asyncCtx != nullptr, undefined);
    napi_get_undefined(env, &asyncCtx->asyncRet);

    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));
    CHECK_AND_RETURN_RET(status == napi_ok && asyncCtx->napi != nullptr, undefined);
    CHECK_AND_RETURN_RET(asyncCtx->napi->adec_ != nullptr, undefined);

    napi_valuetype valueType = napi_undefined;
    CHECK_AND_RETURN_RET(args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok &&
        valueType == napi_object, undefined);
    CHECK_AND_RETURN_RET(AVCodecNapiUtil::ExtractCodecBuffer(env, args[0], asyncCtx->index,
        asyncCtx->info, asyncCtx->flag) == true, undefined);

    valueType = napi_undefined;
    if (args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok && valueType == napi_function) {
        napi_create_reference(env, args[1], 1, &asyncCtx->callbackRef);
    }

    if (asyncCtx->callbackRef == nullptr) {
        napi_create_promise(env, &asyncCtx->deferred, &undefined);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "QueueInput", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AudioDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr || asyncCtx->napi == nullptr || asyncCtx->napi->adec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            CHECK_AND_RETURN(asyncCtx->index > 0);
            if (asyncCtx->napi->adec_->QueueInputBuffer(asyncCtx->index, asyncCtx->info, asyncCtx->flag) != MSERR_OK) {
                asyncCtx->SignError(MSERR_UNKNOWN, "Failed to QueueInput");
            } else {
                asyncCtx->success = true;
            }
        },
        AudioDecoderNapi::CompleteAsyncFunc, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return undefined;
}

napi_value AudioDecoderNapi::ReleaseOutput(napi_env env, napi_callback_info info)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && jsThis != nullptr, undefined);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>();
    CHECK_AND_RETURN_RET(asyncCtx != nullptr, undefined);
    napi_get_undefined(env, &asyncCtx->asyncRet);

    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));
    CHECK_AND_RETURN_RET(status == napi_ok && asyncCtx->napi != nullptr, undefined);
    CHECK_AND_RETURN_RET(asyncCtx->napi->adec_ != nullptr, undefined);

    napi_valuetype valueType = napi_undefined;
    CHECK_AND_RETURN_RET(args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok &&
        valueType == napi_object, undefined);
    CHECK_AND_RETURN_RET(CommonNapi::GetPropertyInt32(env, args[0], "index", asyncCtx->index) == true, undefined);

    valueType = napi_undefined;
    if (args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok && valueType == napi_function) {
        napi_create_reference(env, args[1], 1, &asyncCtx->callbackRef);
    }

    if (asyncCtx->callbackRef == nullptr) {
        napi_create_promise(env, &asyncCtx->deferred, &undefined);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "ReleaseOutput", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AudioDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr || asyncCtx->napi == nullptr || asyncCtx->napi->adec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            CHECK_AND_RETURN(asyncCtx->index > 0);
            if (asyncCtx->napi->adec_->ReleaseOutputBuffer(asyncCtx->index) != MSERR_OK) {
                asyncCtx->SignError(MSERR_UNKNOWN, "Failed to ReleaseOutput");
            } else {
                asyncCtx->success = true;
            }
        },
        AudioDecoderNapi::CompleteAsyncFunc, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return undefined;
}

napi_value AudioDecoderNapi::SetParameter(napi_env env, napi_callback_info info)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && jsThis != nullptr, undefined);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>();
    CHECK_AND_RETURN_RET(asyncCtx != nullptr, undefined);
    napi_get_undefined(env, &asyncCtx->asyncRet);

    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));
    CHECK_AND_RETURN_RET(status == napi_ok && asyncCtx->napi != nullptr, undefined);
    CHECK_AND_RETURN_RET(asyncCtx->napi->adec_ != nullptr, undefined);

    napi_valuetype valueType = napi_undefined;
    CHECK_AND_RETURN_RET(args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok &&
        valueType == napi_object, undefined);
    CHECK_AND_RETURN_RET(AVCodecNapiUtil::ExtractMediaFormat(env, args[0], asyncCtx->format) == true, undefined);

    valueType = napi_undefined;
    if (args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok && valueType == napi_function) {
        napi_create_reference(env, args[1], 1, &asyncCtx->callbackRef);
    }

    if (asyncCtx->callbackRef == nullptr) {
        napi_create_promise(env, &asyncCtx->deferred, &undefined);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "SetParameter", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AudioDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr || asyncCtx->napi == nullptr || asyncCtx->napi->adec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            if (asyncCtx->napi->adec_->SetParameter(asyncCtx->format) != MSERR_OK) {
                asyncCtx->SignError(MSERR_UNKNOWN, "Failed to SetParameter");
            } else {
                asyncCtx->success = true;
            }
        },
        AudioDecoderNapi::CompleteAsyncFunc, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return undefined;
}

napi_value AudioDecoderNapi::GetOutputMediaDescription(napi_env env, napi_callback_info info)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && jsThis != nullptr, undefined);

    auto asyncCtx = std::make_unique<AudioDecoderAsyncContext>();
    CHECK_AND_RETURN_RET(asyncCtx != nullptr, undefined);
    napi_get_undefined(env, &asyncCtx->asyncRet);

    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));
    CHECK_AND_RETURN_RET(status == napi_ok && asyncCtx->napi != nullptr, undefined);
    CHECK_AND_RETURN_RET(asyncCtx->napi->adec_ != nullptr, undefined);

    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        napi_create_reference(env, args[0], 1, &asyncCtx->callbackRef);
    }

    if (asyncCtx->callbackRef == nullptr) {
        napi_create_promise(env, &asyncCtx->deferred, &undefined);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "GetOutputMediaDescription", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AudioDecoderAsyncContext *>(data);
            if (asyncCtx == nullptr || asyncCtx->napi == nullptr || asyncCtx->napi->adec_ == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            Format format;
            if (asyncCtx->napi->adec_->GetOutputFormat(format) != MSERR_OK) {
                asyncCtx->SignError(MSERR_UNKNOWN, "Failed to GetOutputMediaDescription");
            } else {
                asyncCtx->success = true;
                asyncCtx->asyncRet = AVCodecNapiUtil::CompressMediaFormat(env, format);
            }
        },
        AudioDecoderNapi::CompleteAsyncFunc, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return undefined;
}

napi_value AudioDecoderNapi::GetAudioDecoderCaps(napi_env env, napi_callback_info info)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);
    return undefined;
}

napi_value AudioDecoderNapi::On(napi_env env, napi_callback_info info)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    static const size_t MIN_REQUIRED_ARG_COUNT = 2;
    size_t argCount = MIN_REQUIRED_ARG_COUNT;
    napi_value args[MIN_REQUIRED_ARG_COUNT] = { nullptr };
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr || args[1] == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefined;
    }

    AudioDecoderNapi *audioDecoderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&audioDecoderNapi));
    CHECK_AND_RETURN_RET(status == napi_ok && audioDecoderNapi != nullptr, undefined);

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        audioDecoderNapi->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefined;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGD("callbackName: %{public}s", callbackName.c_str());

    CHECK_AND_RETURN_RET(audioDecoderNapi->callback_ != nullptr, undefined);
    auto cb = std::static_pointer_cast<AudioDecoderCallbackNapi>(audioDecoderNapi->callback_);
    cb->SaveCallbackReference(callbackName, args[1]);
    return undefined;
}

void AudioDecoderNapi::AsyncCallback(napi_env env, AudioDecoderAsyncContext *asyncCtx)
{
    napi_value args[2] = {nullptr};

    if (!asyncCtx->success) {
        args[0] = asyncCtx->asyncRet;
    } else {
        args[1] = asyncCtx->asyncRet;
    }

    if (asyncCtx->deferred) {
        if (!asyncCtx->success) {
            napi_reject_deferred(env, asyncCtx->deferred, args[0]);
        } else {
            napi_resolve_deferred(env, asyncCtx->deferred, args[1]);
        }
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(env, asyncCtx->callbackRef, &callback);
        CHECK_AND_RETURN(callback != nullptr);
        const size_t argCount = 2;
        napi_value retVal;
        napi_get_undefined(env, &retVal);
        napi_call_function(env, nullptr, callback, argCount, args, &retVal);
        napi_delete_reference(env, asyncCtx->callbackRef);
    }
    napi_delete_async_work(env, asyncCtx->work);
    delete asyncCtx;
    asyncCtx = nullptr;
}

void AudioDecoderNapi::CompleteAsyncFunc(napi_env env, napi_status status, void *data)
{
    auto asyncCtx = reinterpret_cast<AudioDecoderAsyncContext *>(data);
    CHECK_AND_RETURN(asyncCtx != nullptr);
    if (status == napi_ok && asyncCtx->success == false) {
        (void)CommonNapi::CreateError(env, asyncCtx->errCode, asyncCtx->errMessage, asyncCtx->asyncRet);
    } else {
        (void)CommonNapi::CreateError(env, -1, "status != napi_ok", asyncCtx->asyncRet);
    }
    AudioDecoderNapi::AsyncCallback(env, asyncCtx);
}

void AudioDecoderNapi::AsyncCreator(napi_env env, void *data)
{
    auto asyncCtx = reinterpret_cast<AudioDecoderAsyncContext *>(data);
    CHECK_AND_RETURN(asyncCtx != nullptr);
    napi_value constructor = nullptr;
    napi_status status = napi_get_reference_value(env, constructor_, &constructor);
    if (status != napi_ok || constructor == nullptr) {
        asyncCtx->SignError(MSERR_UNKNOWN, "Failed to new instance");
        return;
    }
    napi_value args[2] = { nullptr };
    status = napi_create_string_utf8(env, asyncCtx->pluginName.c_str(), NAPI_AUTO_LENGTH, &args[0]);
    if (status != napi_ok) {
        asyncCtx->SignError(MSERR_UNKNOWN, "No memory");
        return;
    }

    status = napi_create_int32(env, asyncCtx->createByMime, &args[1]);
    if (status != napi_ok) {
        asyncCtx->SignError(MSERR_UNKNOWN, "No memory");
        return;
    }

    status = napi_new_instance(env, constructor, 0, args, &asyncCtx->asyncRet);
    if (status != napi_ok || asyncCtx->asyncRet == nullptr) {
        asyncCtx->SignError(MSERR_UNKNOWN, "Failed to new instance");
        return;
    }
    asyncCtx->success = true;
}

void AudioDecoderNapi::ErrorCallback(MediaServiceExtErrCode errCode)
{
    if (callback_ != nullptr) {
        auto napiCb = std::static_pointer_cast<AudioDecoderCallbackNapi>(callback_);
        napiCb->SendErrorCallback(errCode);
    }
}
}  // namespace Media
}  // namespace OHOS
