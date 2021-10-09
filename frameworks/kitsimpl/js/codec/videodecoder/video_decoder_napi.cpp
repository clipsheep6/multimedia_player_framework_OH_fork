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

#include "video_decoder_napi.h"
#include "video_decoder_callback_napi.h"
#include <vector>
#include "media_log.h"
#include "media_errors.h"
#include "securec.h"
#include "common_napi.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoDecoderNapi"};
}

namespace OHOS {
namespace Media {
napi_ref VideoDecoderNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "VideoDecoder";

VideoDecoderNapi::VideoDecoderNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

VideoDecoderNapi::~VideoDecoderNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
    callbackNapi_ = nullptr;
    nativeDecoder_ = nullptr;
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value VideoDecoderNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("configure", Configure),
        DECLARE_NAPI_FUNCTION("start", Start),
        DECLARE_NAPI_FUNCTION("decode", Decode),
        DECLARE_NAPI_FUNCTION("render", Render),
        DECLARE_NAPI_FUNCTION("flush", Flush),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("reset", Reset),
        DECLARE_NAPI_FUNCTION("on", On)
    };
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createVideoDecoder", CreateVideoDecoder),
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define VideoDecoder class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value VideoDecoderNapi::Constructor(napi_env env, napi_callback_info info)
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

    VideoDecoderNapi *decoderNapi = new(std::nothrow) VideoDecoderNapi();
    CHECK_AND_RETURN_RET_LOG(decoderNapi != nullptr, nullptr, "No memory");

    decoderNapi->env_ = env;
    decoderNapi->nativeDecoder_ = VideoDecoderFactory::CreateVideoDecoder("h264");
    CHECK_AND_RETURN_RET_LOG(decoderNapi->nativeDecoder_ != nullptr, nullptr, "No memory");

    if (decoderNapi->callbackNapi_ == nullptr) {
        decoderNapi->callbackNapi_ = std::make_shared<VideoDecoderCallbackNapi>(env);
        (void)decoderNapi->nativeDecoder_->SetCallback(decoderNapi->callbackNapi_);
    }

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(decoderNapi),
        VideoDecoderNapi::Destructor, nullptr, &(decoderNapi->wrapper_));
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
        delete decoderNapi;
        MEDIA_LOGE("Failed to wrap native instance");
        return result;
    }

    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void VideoDecoderNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        delete reinterpret_cast<VideoDecoderNapi *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}

napi_value VideoDecoderNapi::CreateVideoDecoder(napi_env env, napi_callback_info info)
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

    MEDIA_LOGD("CreateVideoDecoder success");
    return result;
}

napi_value VideoDecoderNapi::Configure(napi_env env, napi_callback_info info)
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

    VideoDecoderNapi *decoder = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&decoder));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && decoder != nullptr, undefinedResult, "Failed to retrieve instance");
    CHECK_AND_RETURN_RET_LOG(decoder->nativeDecoder_ != nullptr, undefinedResult, "No memory");
    std::vector<Format> config;
    if (decoder->nativeDecoder_->Configure(config, nullptr) != MSERR_OK) {
        decoder->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    decoder->StateCallback(env, CONFIGURE_CALLBACK_NAME);
    MEDIA_LOGD("Configure success");
    return undefinedResult;
}

napi_value VideoDecoderNapi::Start(napi_env env, napi_callback_info info)
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

    VideoDecoderNapi *decoder = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&decoder));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && decoder != nullptr, undefinedResult, "Failed to retrieve instance");
    CHECK_AND_RETURN_RET_LOG(decoder->nativeDecoder_ != nullptr, undefinedResult, "No memory");
    if (decoder->nativeDecoder_->Start() != MSERR_OK) {
        decoder->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    decoder->StateCallback(env, START_CALLBACK_NAME);
    MEDIA_LOGD("Start success");
    return undefinedResult;
}

napi_value VideoDecoderNapi::Decode(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 1;
    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    VideoDecoderNapi *decoder = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&decoder));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && decoder != nullptr, undefinedResult, "Failed to retrieve instance");
    CHECK_AND_RETURN_RET_LOG(decoder->nativeDecoder_ != nullptr, undefinedResult, "No memory");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_object) {
        decoder->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    int32_t offset = -1;
    CommonNapi::GetPropertyInt32(env, args[0], "offset", offset);
    int32_t size = -1;
    CommonNapi::GetPropertyInt32(env, args[0], "size", size);
    int32_t ptsMs = -1;
    CommonNapi::GetPropertyInt32(env, args[0], "ptsMs", ptsMs);
    size_t length = -1;
    int32_t *data = nullptr;
    CommonNapi::GetPropertyArraybuffer(env, args[0], "data", data, length);
    auto buffer = decoder->nativeDecoder_->GetInputBuffer(0);
    if (offset < 0 || size <= 0 || ptsMs < 0 || data == nullptr || length < 0 || buffer == nullptr) {
        decoder->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }
    if (memcpy_s(buffer->GetBase(), static_cast<size_t>(buffer->GetSize()), data, length) != EOK) {
        decoder->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    if (decoder->nativeDecoder_->PushInputBuffer(0, offset, size, ptsMs, BUFFER_FLAG_NONE) != MSERR_OK) {
        decoder->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }
    MEDIA_LOGD("Decode success");
    return undefinedResult;
}

napi_value VideoDecoderNapi::Render(napi_env env, napi_callback_info info)
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

    VideoDecoderNapi *decoder = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&decoder));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && decoder != nullptr, undefinedResult, "Failed to retrieve instance");
    CHECK_AND_RETURN_RET_LOG(decoder->nativeDecoder_ != nullptr, undefinedResult, "No memory");
    if (decoder->nativeDecoder_->ReleaseOutputBuffer(0, true) != MSERR_OK) {
        decoder->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }
    MEDIA_LOGD("Render success");
    return undefinedResult;
}

napi_value VideoDecoderNapi::Flush(napi_env env, napi_callback_info info)
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

    VideoDecoderNapi *decoder = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&decoder));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && decoder != nullptr, undefinedResult, "Failed to retrieve instance");
    CHECK_AND_RETURN_RET_LOG(decoder->nativeDecoder_ != nullptr, undefinedResult, "No memory");
    if (decoder->nativeDecoder_->Flush() != MSERR_OK) {
        decoder->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    decoder->StateCallback(env, FLUSH_CALLBACK_NAME);
    MEDIA_LOGD("Flush success");
    return undefinedResult;
}

napi_value VideoDecoderNapi::Stop(napi_env env, napi_callback_info info)
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

    VideoDecoderNapi *decoder = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&decoder));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && decoder != nullptr, undefinedResult, "Failed to retrieve instance");
    CHECK_AND_RETURN_RET_LOG(decoder->nativeDecoder_ != nullptr, undefinedResult, "No memory");
    if (decoder->nativeDecoder_->Stop() != MSERR_OK) {
        decoder->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    decoder->StateCallback(env, STOP_CALLBACK_NAME);
    MEDIA_LOGD("Stop success");
    return undefinedResult;
}

napi_value VideoDecoderNapi::Reset(napi_env env, napi_callback_info info)
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

    VideoDecoderNapi *decoder = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&decoder));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && decoder != nullptr, undefinedResult, "Failed to retrieve instance");
    CHECK_AND_RETURN_RET_LOG(decoder->nativeDecoder_ != nullptr, undefinedResult, "No memory");
    if (decoder->nativeDecoder_->Reset() != MSERR_OK) {
        decoder->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    decoder->StateCallback(env, RESET_CALLBACK_NAME);
    MEDIA_LOGD("Reset success");
    return undefinedResult;
}

napi_value VideoDecoderNapi::On(napi_env env, napi_callback_info info)
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

    VideoDecoderNapi *decoder = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&decoder));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && decoder != nullptr, undefinedResult, "Failed to retrieve instance");

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        decoder->ErrorCallback(env, MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGD("callbackName: %{public}s", callbackName.c_str());

    CHECK_AND_RETURN_RET_LOG(decoder->callbackNapi_ != nullptr, undefinedResult, "callbackNapi_ is nullptr");
    std::shared_ptr<VideoDecoderCallbackNapi> cb =
        std::static_pointer_cast<VideoDecoderCallbackNapi>(decoder->callbackNapi_);
    cb->SaveCallbackReference(callbackName, args[1]);
    return undefinedResult;
}

void VideoDecoderNapi::ErrorCallback(napi_env env, MediaServiceExtErrCode errCode)
{
    if (callbackNapi_ != nullptr) {
        std::shared_ptr<VideoDecoderCallbackNapi> napiCb =
            std::static_pointer_cast<VideoDecoderCallbackNapi>(callbackNapi_);
        napiCb->SendErrorCallback(env, errCode);
    }
}

void VideoDecoderNapi::StateCallback(napi_env env, const std::string &callbackName)
{
    if (callbackNapi_ != nullptr) {
        std::shared_ptr<VideoDecoderCallbackNapi> napiCb =
            std::static_pointer_cast<VideoDecoderCallbackNapi>(callbackNapi_);
        napiCb->SendCallback(env, callbackName);
    }
}
}  // namespace Media
}  // namespace OHOS
