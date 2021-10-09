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

#include "video_decoder_callback_napi.h"
#include <uv.h>
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoDecoderCallbackNapi"};
}

namespace OHOS {
namespace Media {
VideoDecoderCallbackNapi::VideoDecoderCallbackNapi(napi_env env)
    : env_(env)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

VideoDecoderCallbackNapi::~VideoDecoderCallbackNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void VideoDecoderCallbackNapi::SaveCallbackReference(const std::string &callbackName, napi_value args)
{
    std::lock_guard<std::mutex> lock(mutex_);

    napi_ref callback = nullptr;
    const int32_t refCount = 1;

    napi_status status = napi_create_reference(env_, args, refCount, &callback);
    CHECK_AND_RETURN_LOG(status == napi_ok && callback != nullptr, "creating reference for callback fail");

    std::shared_ptr<AutoRef> cb = std::make_shared<AutoRef>(env_, callback);
    if (callbackName == ERROR_CALLBACK_NAME) {
        errorCallback_ = cb;
    } else if (callbackName == NEED_DATA_CALLBACK_NAME) {
        needDataCallback_ = cb;
    } else if (callbackName == OUTPUT_AVAILABLE_CALLBACK_NAME) {
        outputAvailableCallback_ = cb;
    } else if (callbackName == CONFIGURE_CALLBACK_NAME) {
        configureCallback_ = cb;
    } else if (callbackName == START_CALLBACK_NAME) {
        startCallback_ = cb;
    } else if (callbackName == FLUSH_CALLBACK_NAME) {
        flushCallback_ = cb;
    } else if (callbackName == STOP_CALLBACK_NAME) {
        stopCallback_ = cb;
    } else if (callbackName == RESET_CALLBACK_NAME) {
        resetCallback_ = cb;
    } else {
        MEDIA_LOGW("Unknown callback type: %{public}s", callbackName.c_str());
        return;
    }
}

void VideoDecoderCallbackNapi::OnOutputBufferAvailable(uint32_t index, VideoDecoderBufferInfo info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("OnOutputBufferAvailable");
    CHECK_AND_RETURN_LOG(outputAvailableCallback_ != nullptr, "No callback reference");
    napi_value jsCallback = nullptr;
    napi_status status = napi_get_reference_value(env_, outputAvailableCallback_->cb_, &jsCallback);
    CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr, "get reference value fail");

    napi_value result = nullptr;
    status = napi_call_function(env_, nullptr, jsCallback, 0, nullptr, &result);
    CHECK_AND_RETURN_LOG(status == napi_ok, "send callback fail");

    MEDIA_LOGD("send callback success");
}

void VideoDecoderCallbackNapi::OnInputBufferAvailable(uint32_t index)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("OnInputBufferAvailable");
    CHECK_AND_RETURN_LOG(needDataCallback_ != nullptr, "No callback reference");
    napi_value jsCallback = nullptr;
    napi_status status = napi_get_reference_value(env_, needDataCallback_->cb_, &jsCallback);
    CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr, "get reference value fail");

    napi_value result = nullptr;
    status = napi_call_function(env_, nullptr, jsCallback, 0, nullptr, &result);
    CHECK_AND_RETURN_LOG(status == napi_ok, "send callback fail");

    MEDIA_LOGD("send callback success");
}

void VideoDecoderCallbackNapi::SendErrorCallback(napi_env env, MediaServiceExtErrCode errCode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(env != nullptr, "Cannot find context");
    CHECK_AND_RETURN_LOG(errorCallback_ != nullptr, "Cannot find the reference of error callback");

    napi_value jsCallback = nullptr;
    napi_status status = napi_get_reference_value(env, errorCallback_->cb_, &jsCallback);
    CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr, "Failed to get the representation of callback");

    napi_value msgValStr = nullptr;
    status = napi_create_string_utf8(env, MSExtErrorToString(errCode).c_str(), NAPI_AUTO_LENGTH, &msgValStr);
    CHECK_AND_RETURN_LOG(status == napi_ok && msgValStr != nullptr, "Failed to create error message string");

    napi_value args[1] = { nullptr };
    status = napi_create_error(env, nullptr, msgValStr, &args[0]);
    CHECK_AND_RETURN_LOG(status == napi_ok && args[0] != nullptr, "Failed to create JavaScript Error object");

    status = CommonNapi::FillErrorArgs(env, static_cast<int32_t>(errCode), args[0]);
    CHECK_AND_RETURN_LOG(status == napi_ok, "Failed to create BusinessError object");

    const size_t argCount = 1;
    napi_value result = nullptr;
    status = napi_call_function(env, nullptr, jsCallback, argCount, args, &result);
    CHECK_AND_RETURN_LOG(status == napi_ok, "Failed to call JavaScript error function");
}

void VideoDecoderCallbackNapi::SendCallback(napi_env env, const std::string &callbackName)
{
    std::shared_ptr<AutoRef> callbackRef = nullptr;
    if (callbackName == CONFIGURE_CALLBACK_NAME) {
        callbackRef = configureCallback_;
    } else if (callbackName == START_CALLBACK_NAME) {
        callbackRef = startCallback_;
    } else if (callbackName == FLUSH_CALLBACK_NAME) {
        callbackRef = flushCallback_;
    } else if (callbackName == STOP_CALLBACK_NAME) {
        callbackRef = stopCallback_;
    } else if (callbackName == RESET_CALLBACK_NAME) {
        callbackRef = resetCallback_;
    } else {
        MEDIA_LOGW("Unknown callback type: %{public}s", callbackName.c_str());
        return;
    }

    CHECK_AND_RETURN_LOG(callbackRef != nullptr, "no callback reference");
    napi_value jsCallback = nullptr;
    napi_status status = napi_get_reference_value(env, callbackRef->cb_, &jsCallback);
    CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr, "get reference value fail");

    napi_value result = nullptr;
    status = napi_call_function(env, nullptr, jsCallback, 0, nullptr, &result);
    CHECK_AND_RETURN_LOG(status == napi_ok, "send callback fail");

    MEDIA_LOGD("send callback success");
}
}  // namespace Media
}  // namespace OHOS
