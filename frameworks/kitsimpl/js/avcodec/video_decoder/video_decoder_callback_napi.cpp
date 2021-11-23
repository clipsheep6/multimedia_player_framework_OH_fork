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
VideoDecoderCallbackNapi::VideoDecoderCallbackNapi(napi_env env, std::shared_ptr<VideoDecoder> videoDecoderImpl)
    : env_(env),
      videoDecoderImpl_(videoDecoderImpl)
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
    CHECK_AND_RETURN_LOG(status == napi_ok && callback != nullptr, "Failed to create callback reference");

    std::shared_ptr<AutoRef> cb = std::make_shared<AutoRef>(env_, callback);
    if (callbackName == CONFIGURE_CALLBACK_NAME) {
        configureCallback_ = cb;
    } else if (callbackName == PREPARE_CALLBACK_NAME) {
        prepareCallback_ = cb;
    } else if (callbackName == START_CALLBACK_NAME) {
        startCallback_ = cb;
    } else if (callbackName == STOP_CALLBACK_NAME) {
        stopCallback_ = cb;
    } else if (callbackName == FLUSH_CALLBACK_NAME) {
        flushCallback_ = cb;
    } else if (callbackName == RESET_CALLBACK_NAME) {
        resetCallback_ = cb;
    } else if (callbackName == ERROR_CALLBACK_NAME) {
        errorCallback_ = cb;
    } else if (callbackName == INPUT_CALLBACK_NAME) {
        inputCallback_ = cb;
    } else if (callbackName == OUTPUT_CALLBACK_NAME) {
        outputCallback_ = cb;
    } else {
        MEDIA_LOGW("Unknown callback type: %{public}s", callbackName.c_str());
        return;
    }
}

void VideoDecoderCallbackNapi::SendErrorCallback(MediaServiceExtErrCode errCode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(errorCallback_ != nullptr, "Cannot find the reference of error callback");

    VideoDecoderJsCallback *cb = new(std::nothrow) VideoDecoderJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = errorCallback_;
    cb->callbackName = ERROR_CALLBACK_NAME;
    cb->errorMsg = MSExtErrorToString(errCode);
    cb->errorCode = errCode;
    return OnJsErrorCallBack(cb);
}

std::shared_ptr<AutoRef> VideoDecoderCallbackNapi::StateCallbackSelect(const std::string &callbackName) const
{
    CHECK_AND_RETURN_RET_LOG(!callbackName.empty(), nullptr, "Illegal callbackname");
    if (callbackName == CONFIGURE_CALLBACK_NAME) {
        return configureCallback_;
    } else if (callbackName == PREPARE_CALLBACK_NAME) {
        return prepareCallback_;
    } else if (callbackName == START_CALLBACK_NAME) {
        return startCallback_;
    } else if (callbackName == STOP_CALLBACK_NAME) {
        return stopCallback_;
    } else if (callbackName == FLUSH_CALLBACK_NAME) {
        return flushCallback_;
    } else if (callbackName == RESET_CALLBACK_NAME) {
        return resetCallback_;
    } else if (callbackName == ERROR_CALLBACK_NAME) {
        return errorCallback_;
    } else if (callbackName == INPUT_CALLBACK_NAME) {
        return inputCallback_;
    } else if (callbackName == OUTPUT_CALLBACK_NAME) {
        return outputCallback_;
    } else {
        MEDIA_LOGW("Unknown callback type: %{public}s", callbackName.c_str());
        return nullptr;
    }
}

void VideoDecoderCallbackNapi::SendStateCallback(const std::string &callbackName)
{
    std::shared_ptr<AutoRef> callbackRef = nullptr;
    callbackRef = StateCallbackSelect(callbackName);
    CHECK_AND_RETURN_LOG(callbackRef != nullptr, "No callback reference");
    VideoDecoderJsCallback *cb = new(std::nothrow) VideoDecoderJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = callbackRef;
    cb->callbackName = callbackName;
    return OnJsStateCallBack(cb);
}

void VideoDecoderCallbackNapi::OnError(AVCodecErrorType errorType, int32_t errCode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("OnError is called, name: %{public}d, error message: %{public}d", errorType, errCode);
    CHECK_AND_RETURN_LOG(errorCallback_ != nullptr, "Cannot find the reference of error callback");

    VideoDecoderJsCallback *cb = new(std::nothrow) VideoDecoderJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = errorCallback_;
    cb->callbackName = ERROR_CALLBACK_NAME;
    cb->errorMsg = MSErrorToString(static_cast<MediaServiceErrCode>(errCode));
    cb->errorCode = MSErrorToExtError(static_cast<MediaServiceErrCode>(errCode));
    return OnJsErrorCallBack(cb);
}

void VideoDecoderCallbackNapi::OnOutputFormatChanged(const Format &format)
{
    MEDIA_LOGD("OnOutputFormatChanged() is called");
}

void VideoDecoderCallbackNapi::OnInputBufferAvailable(uint32_t index)
{
    MEDIA_LOGD("OnInputBufferAvailable() is called, index: %{public}u", index);
    CHECK_AND_RETURN_LOG(inputCallback_ != nullptr, "Cannot find callback reference");

    CHECK_AND_RETURN_LOG(videoDecoderImpl_ != nullptr, "Failed to retrieve instance");
    auto buffer = videoDecoderImpl_->GetInputBuffer(index);
    CHECK_AND_RETURN_LOG(buffer != nullptr, "Failed to GetInputBuffer");

    VideoDecoderJsCallback *cb = new(std::nothrow) VideoDecoderJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = inputCallback_;
    cb->callbackName = INPUT_CALLBACK_NAME;
    cb->index = index;
    cb->memory = buffer;
    AVCodecBufferInfo info;
    cb->info = info;
    return OnJsInputCallBack(cb);
}

void VideoDecoderCallbackNapi::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    MEDIA_LOGD("OnOutputBufferAvailable() is called, index: %{public}u", index);
    CHECK_AND_RETURN_LOG(outputCallback_ != nullptr, "Cannot find callback reference");

    CHECK_AND_RETURN_LOG(videoDecoderImpl_ != nullptr, "Failed to retrieve instance");
    auto buffer = videoDecoderImpl_->GetOutputBuffer(index);
    CHECK_AND_RETURN_LOG(buffer != nullptr, "Failed to GetOutputBuffer");

    VideoDecoderJsCallback *cb = new(std::nothrow) VideoDecoderJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = outputCallback_;
    cb->callbackName = OUTPUT_CALLBACK_NAME;
    cb->index = index;
    cb->info = info;
    cb->flag = flag;
    cb->memory = buffer;
    return OnJsOutputCallBack(cb);
}

void VideoDecoderCallbackNapi::OnJsStateCallBack(VideoDecoderJsCallback *jsCb) const
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        MEDIA_LOGE("Fail to get uv event loop");
        delete jsCb;
        return;
    }

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        MEDIA_LOGE("Fail to new uv_work_t");
        delete jsCb;
        return;
    }

    work->data = reinterpret_cast<void *>(jsCb);
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "work is nullptr");
        VideoDecoderJsCallback *event = reinterpret_cast<VideoDecoderJsCallback *>(work->data);
        std::string request = event->callbackName;
        napi_env env = event->callback->env_;
        napi_ref callback = event->callback->cb_;
        MEDIA_LOGD("OnJsStateCallBack %{public}s, uv_queue_work start", request.c_str());
        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());
            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(env, callback, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());
            // Call back function
            napi_value result = nullptr;
            nstatus = napi_call_function(env, nullptr, jsCallback, 0, nullptr, &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s failed to napi call function", request.c_str());
        } while (0);
        delete event;
        delete work;
    });
    if (ret != 0) {
        MEDIA_LOGE("Failed to uv_queue_work task");
        delete jsCb;
        delete work;
    }
}

void VideoDecoderCallbackNapi::OnJsErrorCallBack(VideoDecoderJsCallback *jsCb) const
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        MEDIA_LOGE("Fail to get uv event loop");
        delete jsCb;
        return;
    }

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        MEDIA_LOGE("No memory");
        delete jsCb;
        return;
    }

    work->data = reinterpret_cast<void *>(jsCb);
    // async callback, jsWork and jsWork->data should be heap object.
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "Work thread is nullptr");
        VideoDecoderJsCallback *event = reinterpret_cast<VideoDecoderJsCallback *>(work->data);
        std::string request = event->callbackName;
        napi_env env = event->callback->env_;
        napi_ref callback = event->callback->cb_;
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", request.c_str());
        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());
            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(env, callback, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            napi_value msgValStr = nullptr;
            nstatus = napi_create_string_utf8(env, event->errorMsg.c_str(), NAPI_AUTO_LENGTH, &msgValStr);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && msgValStr != nullptr, "%{public}s fail to get error code value",
                request.c_str());

            napi_value args[1] = { nullptr };
            nstatus = napi_create_error(env, nullptr, msgValStr, &args[0]);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && args[0] != nullptr, "%{public}s fail to create error callback",
                request.c_str());

            nstatus = CommonNapi::FillCodecErrorArgs(env, static_cast<int32_t>(event->errorCode), args[0]);
            CHECK_AND_RETURN_LOG(nstatus == napi_ok, "create error callback fail");

            // Call back function
            const size_t argCount = 1;
            napi_value result = nullptr;
            nstatus = napi_call_function(env, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s fail to napi call function", request.c_str());
        } while (0);
        delete event;
        delete work;
    });
    if (ret != 0) {
        MEDIA_LOGE("Failed to execute libuv work queue");
        delete jsCb;
        delete work;
    }
}

void VideoDecoderCallbackNapi::OnJsInputCallBack(VideoDecoderJsCallback *jsCb) const
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        MEDIA_LOGE("Fail to get uv event loop");
        delete jsCb;
        return;
    }

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        MEDIA_LOGE("Fail to new uv_work_t");
        delete jsCb;
        return;
    }

    work->data = reinterpret_cast<void *>(jsCb);
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "work is nullptr");
        VideoDecoderJsCallback *event = reinterpret_cast<VideoDecoderJsCallback *>(work->data);
        std::string request = event->callbackName;
        napi_env env = event->callback->env_;
        napi_ref callback = event->callback->cb_;
        MEDIA_LOGD("OnJsStateCallBack %{public}s, uv_queue_work start", request.c_str());
        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());
            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(env, callback, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            napi_value args[1] = { nullptr };
            nstatus = napi_create_object(env, &args[0]);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s create fail", request.c_str());

            napi_value timeStampMsStr = nullptr;
            nstatus = napi_create_string_utf8(env, "timeStampMs", NAPI_AUTO_LENGTH, &timeStampMsStr);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s create fail", request.c_str());
            napi_value timeStampMs = nullptr;
            nstatus = napi_create_int32(env, -1, &timeStampMs);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s create fail", request.c_str());
            status = napi_set_property(env, args[0], timeStampMsStr, timeStampMs);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s set property fail", request.c_str());

            napi_value flagStr = nullptr;
            nstatus = napi_create_string_utf8(env, "flag", NAPI_AUTO_LENGTH, &flagStr);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s create fail", request.c_str());
            napi_value flag = nullptr;
            nstatus = napi_create_int32(env, static_cast<int32_t>(AVCODEC_BUFFER_FLAG_NONE), &flag);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s create fail", request.c_str());
            status = napi_set_property(env, args[0], flagStr, flag);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s set property fail", request.c_str());

            napi_value bufferStr = nullptr;
            nstatus = napi_create_string_utf8(env, "buffer", NAPI_AUTO_LENGTH, &bufferStr);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s create fail", request.c_str());
            napi_value codecBuffer = CommonNapi::CreateCodecBuffer(env, event->index, event->memory, event->info);
            CHECK_AND_BREAK_LOG(codecBuffer != nullptr, "%{public}s create codecBuffer fail", request.c_str());
            status = napi_set_property(env, args[0], bufferStr, codecBuffer);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s set property fail", request.c_str());

            // Call back function
            const size_t argCount = 1;
            napi_value result = nullptr;
            nstatus = napi_call_function(env, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s failed to napi call function", request.c_str());
        } while (0);
        delete event;
        delete work;
    });
    if (ret != 0) {
        MEDIA_LOGE("Failed to uv_queue_work task");
        delete jsCb;
        delete work;
    }
}

void VideoDecoderCallbackNapi::OnJsOutputCallBack(VideoDecoderJsCallback *jsCb) const
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        MEDIA_LOGE("Fail to get uv event loop");
        delete jsCb;
        return;
    }

    uv_work_t *work = new(std::nothrow) uv_work_t;
    if (work == nullptr) {
        MEDIA_LOGE("Fail to new uv_work_t");
        delete jsCb;
        return;
    }

    work->data = reinterpret_cast<void *>(jsCb);
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "work is nullptr");
        VideoDecoderJsCallback *event = reinterpret_cast<VideoDecoderJsCallback *>(work->data);
        std::string request = event->callbackName;
        napi_env env = event->callback->env_;
        napi_ref callback = event->callback->cb_;
        MEDIA_LOGD("OnJsStateCallBack %{public}s, uv_queue_work start", request.c_str());
        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());
            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(env, callback, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            napi_value args[1] = { nullptr };
            nstatus = napi_create_object(env, &args[0]);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s create fail", request.c_str());

            napi_value timeStampMsStr = nullptr;
            nstatus = napi_create_string_utf8(env, "timeStampMs", NAPI_AUTO_LENGTH, &timeStampMsStr);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s create fail", request.c_str());
            napi_value timeStampMs = nullptr;
            const int32_t UsToMs = 1000;
            nstatus = napi_create_int32(env, static_cast<int32_t>(event->info.presentationTimeUs/UsToMs), &timeStampMs);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s create fail", request.c_str());
            status = napi_set_property(env, args[0], timeStampMsStr, timeStampMs);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s set property fail", request.c_str());

            napi_value flagStr = nullptr;
            nstatus = napi_create_string_utf8(env, "flag", NAPI_AUTO_LENGTH, &flagStr);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s create fail", request.c_str());
            napi_value flag = nullptr;
            nstatus = napi_create_int32(env, static_cast<int32_t>(event->flag), &flag);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s create fail", request.c_str());
            status = napi_set_property(env, args[0], flagStr, flag);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s set property fail", request.c_str());

            napi_value bufferStr = nullptr;
            nstatus = napi_create_string_utf8(env, "buffer", NAPI_AUTO_LENGTH, &bufferStr);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s create fail", request.c_str());
            napi_value codecBuffer = CommonNapi::CreateCodecBuffer(env, event->index, event->memory, event->info);
            CHECK_AND_BREAK_LOG(codecBuffer != nullptr, "%{public}s create codecBuffer fail", request.c_str());
            status = napi_set_property(env, args[0], bufferStr, codecBuffer);
            CHECK_AND_BREAK_LOG(status == napi_ok, "%{public}s set property fail", request.c_str());

            // Call back function
            const size_t argCount = 1;
            napi_value result = nullptr;
            nstatus = napi_call_function(env, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s failed to napi call function", request.c_str());
        } while (0);
        delete event;
        delete work;
    });
    if (ret != 0) {
        MEDIA_LOGE("Failed to uv_queue_work task");
        delete jsCb;
        delete work;
    }
}
}  // namespace Media
}  // namespace OHOS
