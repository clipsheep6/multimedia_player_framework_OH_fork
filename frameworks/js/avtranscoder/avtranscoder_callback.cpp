/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "avtranscoder_callback.h"
#include <uv.h>
#include "media_errors.h"
#include "scope_guard.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVTransCoderCallback"};
}

namespace OHOS {
namespace Media {
AVTransCoderCallback::AVTransCoderCallback(napi_env env) : env_(env)
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR "Instances create", FAKE_POINTER(this));
}

AVTransCoderCallback::~AVTransCoderCallback()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR "Instances destroy", FAKE_POINTER(this));
}

void AVTransCoderCallback::SaveCallbackReference(const std::string &name, std::weak_ptr<AutoRef> ref)
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_[name] = ref;
    MEDIA_LOGI("Set callback type: %{public}s", name.c_str());
}

void AVTransCoderCallback::CancelCallbackReference(const std::string &name)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = refMap_.find(name);
    if (iter != refMap_.end()) {
        refMap_.erase(iter);
    }
    MEDIA_LOGI("Cancel callback type: %{public}s", name.c_str());
}

void AVTransCoderCallback::ClearCallbackReference()
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_.clear();
    MEDIA_LOGI("ClearCallback!");
}

void AVTransCoderCallback::SendErrorCallback(MediaServiceExtErrCodeAPI9 errCode, const std::string &msg)
{
    std::string message = MSExtAVErrorToString(errCode) + msg;
    MEDIA_LOGE("SendErrorCallback:errorCode %{public}d, errorMsg %{public}s", errCode, message.c_str());
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(AVTransCoderEvent::EVENT_ERROR) == refMap_.end()) {
        MEDIA_LOGW("can not find error callback!");
        return;
    }

    AVTransCoderJsCallback *cb = new(std::nothrow) AVTransCoderJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");
    cb->autoRef = refMap_.at(AVTransCoderEvent::EVENT_ERROR);
    cb->callbackName = AVTransCoderEvent::EVENT_ERROR;
    cb->errorCode = errCode;
    cb->errorMsg = message;
    return OnJsErrorCallBack(cb);
}

void AVTransCoderCallback::SendStateCallback(const std::string &state, const StateChangeReason &reason)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGI("StateChange, currentState: %{public}s to state: %{public}s", currentState_.c_str(), state.c_str());
    currentState_ = state;
}

void AVTransCoderCallback::SendCompleteCallback()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(AVTransCoderEvent::EVENT_COMPLETE) == refMap_.end()) {
        MEDIA_LOGW("can not find statechange callback!");
        return;
    }

    AVTransCoderJsCallback *cb = new(std::nothrow) AVTransCoderJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");
    cb->autoRef = refMap_.at(AVTransCoderEvent::EVENT_COMPLETE);
    cb->callbackName = AVTransCoderEvent::EVENT_COMPLETE;
    return OnJsCompleteCallBack(cb);
}

void AVTransCoderCallback::SendProgressUpdateCallback(int32_t progress)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(AVTransCoderEvent::EVENT_PROGRESS_UPDATE) == refMap_.end()) {
        MEDIA_LOGW("can not find progressupdate callback!");
        return;
    }
    AVTransCoderJsCallback *cb = new(std::nothrow) AVTransCoderJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");
    cb->autoRef = refMap_.at(AVTransCoderEvent::EVENT_PROGRESS_UPDATE);
    cb->callbackName = AVTransCoderEvent::EVENT_PROGRESS_UPDATE;
    cb->progress = progress;
    return OnJsProgressUpdateCallback(cb);
}

void AVTransCoderCallback::OnError(int32_t errCode, const std::string &errorMsg)
{
    MediaServiceExtErrCodeAPI9 errorCodeApi9 = MSErrorToExtErrorAPI9(static_cast<MediaServiceErrCode>(errCode));
    SendErrorCallback(errorCodeApi9, errorMsg);
    SendStateCallback(AVTransCoderState::STATE_ERROR, StateChangeReason::BACKGROUND);
}

void AVTransCoderCallback::OnInfo(int32_t type, int32_t extra)
{
    if (type == TransCoderOnInfoType::INFO_TYPE_TRANSCODER_COMPLETED) {
        SendCompleteCallback();
    } else if (type == TransCoderOnInfoType::INFO_TYPE_PROGRESS_UPDATE) {
        SendProgressUpdateCallback(extra);
    }
}

void AVTransCoderCallback::OnJsCompleteCallBack(AVTransCoderJsCallback *jsCb) const
{
    ON_SCOPE_EXIT(0) {
        delete jsCb;
    };

    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    CHECK_AND_RETURN_LOG(loop != nullptr, "Fail to get uv event loop");

    uv_work_t *work = new(std::nothrow) uv_work_t;
    CHECK_AND_RETURN_LOG(work != nullptr, "fail to new uv_work_t");
    ON_SCOPE_EXIT(1) {
        delete work;
    };

    work->data = reinterpret_cast<void *>(jsCb);
    int ret = QueueCompleteWork(loop, work);
    CHECK_AND_RETURN_LOG(ret == 0, "fail to uv_queue_work_with_qos task");

    CANCEL_SCOPE_EXIT_GUARD(0);
    CANCEL_SCOPE_EXIT_GUARD(1);
}

int32_t AVTransCoderCallback::QueueCompleteWork(uv_loop_s *loop, uv_work_t *work) const
{
    int ret = uv_queue_work_with_qos(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "work is nullptr");
        AVTransCoderJsCallback *event = reinterpret_cast<AVTransCoderJsCallback *>(work->data);
        std::string request = event->callbackName;
        MEDIA_LOGI("uv_queue_work_with_qos start, state changes to %{public}s", event->state.c_str());
        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());
            std::shared_ptr<AutoRef> ref = event->autoRef.lock();
            CHECK_AND_BREAK_LOG(ref != nullptr, "%{public}s AutoRef is nullptr", request.c_str());

            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(ref->env_, &scope);
            CHECK_AND_BREAK_LOG(scope != nullptr, "%{public}s scope is nullptr", request.c_str());
            ON_SCOPE_EXIT(0) {
                napi_close_handle_scope(ref->env_, scope);
            };

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            napi_value args[2] = { nullptr };

            const size_t argCount = 0;
            napi_value result = nullptr;
            nstatus = napi_call_function(ref->env_, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s fail to napi call function", request.c_str());
        } while (0);
        delete event;
        delete work;
    }, uv_qos_user_initiated);
    return ret;
}

void AVTransCoderCallback::OnJsProgressUpdateCallback(AVTransCoderJsCallback *jsCb) const
{
    ON_SCOPE_EXIT(0) {
        delete jsCb;
    };

    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    CHECK_AND_RETURN_LOG(loop != nullptr, "Fail to get uv event loop");

    uv_work_t *work = new(std::nothrow) uv_work_t;
    CHECK_AND_RETURN_LOG(work != nullptr, "fail to new uv_work_t");
    ON_SCOPE_EXIT(1) {
        delete work;
    };

    work->data = reinterpret_cast<void *>(jsCb);
    int ret = QueueProgressUpdateWork(loop, work);
    CHECK_AND_RETURN_LOG(ret == 0, "fail to uv_queue_work_with_qos task");

    CANCEL_SCOPE_EXIT_GUARD(0);
    CANCEL_SCOPE_EXIT_GUARD(1);
}

int32_t AVTransCoderCallback::QueueProgressUpdateWork(uv_loop_s *loop, uv_work_t *work) const
{
    int ret = uv_queue_work_with_qos(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "work is nullptr");
        if (work->data == nullptr) {
            delete work;
            MEDIA_LOGE("workdata is nullptr");
            return;
        }
        AVTransCoderJsCallback *event = reinterpret_cast<AVTransCoderJsCallback *>(work->data);
        std::string request = event->callbackName;
        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());
            std::shared_ptr<AutoRef> ref = event->autoRef.lock();
            CHECK_AND_BREAK_LOG(ref != nullptr, "%{public}s AutoRef is nullptr", request.c_str());

            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(ref->env_, &scope);
            CHECK_AND_BREAK_LOG(scope != nullptr, "%{public}s scope is nullptr", request.c_str());
            ON_SCOPE_EXIT(0) {
                napi_close_handle_scope(ref->env_, scope);
            };

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            napi_value args[1] = { nullptr };
            nstatus = napi_create_int32(ref->env_, event->progress, &args[0]);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && args[0] != nullptr,
                "%{public}s fail to create callback", request.c_str());

            const size_t argCount = 1;
            napi_value result = nullptr;
            nstatus = napi_call_function(ref->env_, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s fail to napi call function", request.c_str());
        } while (0);
        delete event;
        delete work;
    }, uv_qos_user_initiated);
    return ret;
}

std::string AVTransCoderCallback::GetState()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return currentState_;
}

void AVTransCoderCallback::OnJsErrorCallBack(AVTransCoderJsCallback *jsCb) const
{
    ON_SCOPE_EXIT(0) {
        delete jsCb;
    };

    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    CHECK_AND_RETURN_LOG(loop != nullptr, "Fail to get uv event loop");

    uv_work_t *work = new(std::nothrow) uv_work_t;
    CHECK_AND_RETURN_LOG(work != nullptr, "fail to new uv_work_t");
    ON_SCOPE_EXIT(1) {
        delete work;
    };

    work->data = reinterpret_cast<void *>(jsCb);
    // async callback, jsWork and jsWork->data should be heap object.
    int ret = QueueErrorWork(loop, work);
    CHECK_AND_RETURN_LOG(ret == 0, "fail to uv_queue_work_with_qos task");

    CANCEL_SCOPE_EXIT_GUARD(0);
    CANCEL_SCOPE_EXIT_GUARD(1);
}

int32_t AVTransCoderCallback::QueueErrorWork(uv_loop_s *loop, uv_work_t *work) const
{
    int ret = uv_queue_work_with_qos(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "work is nullptr");
        if (work->data == nullptr) {
            delete work;
            MEDIA_LOGE("workdata is nullptr");
            return;
        }
        AVTransCoderJsCallback *event = reinterpret_cast<AVTransCoderJsCallback *>(work->data);
        std::string request = event->callbackName;
        MEDIA_LOGI("uv_queue_work_with_qos start, errorcode:%{public}d , errormessage:%{public}s:",
            event->errorCode, event->errorMsg.c_str());
        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());
            std::shared_ptr<AutoRef> ref = event->autoRef.lock();
            CHECK_AND_BREAK_LOG(ref != nullptr, "%{public}s AutoRef is nullptr", request.c_str());

            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(ref->env_, &scope);
            CHECK_AND_BREAK_LOG(scope != nullptr, "%{public}s scope is nullptr", request.c_str());
            ON_SCOPE_EXIT(0) {
                napi_close_handle_scope(ref->env_, scope);
            };

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            napi_value msgValStr = nullptr;
            nstatus = napi_create_string_utf8(ref->env_, event->errorMsg.c_str(), NAPI_AUTO_LENGTH, &msgValStr);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && msgValStr != nullptr, "create error message str fail");

            napi_value args[1] = { nullptr };
            nstatus = napi_create_error(ref->env_, nullptr, msgValStr, &args[0]);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && args[0] != nullptr, "create error callback fail");

            nstatus = CommonNapi::FillErrorArgs(ref->env_, event->errorCode, args[0]);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "create error callback fail");

            // Call back function
            napi_value result = nullptr;
            nstatus = napi_call_function(ref->env_, nullptr, jsCallback, 1, args, &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s fail to napi call function", request.c_str());
        } while (0);
        delete event;
        delete work;
    }, uv_qos_user_initiated);
    return ret;
}
} // namespace Media
} // namespace OHOS