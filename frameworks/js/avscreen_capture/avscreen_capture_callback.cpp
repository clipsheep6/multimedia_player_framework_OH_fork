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

#include "avscreen_capture_callback.h"
#include "scope_guard.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVScreenCaptureCallback"};
}

namespace OHOS {
namespace Media {
AVScreenCaptureCallback::AVScreenCaptureCallback(napi_env env) : env_(env)
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR "Instances create", FAKE_POINTER(this));
}

AVScreenCaptureCallback::~AVScreenCaptureCallback()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR "Instances destroy", FAKE_POINTER(this));
}

void AVScreenCaptureCallback::SendErrorCallback(int32_t errCode, const std::string &msg)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(AVScreenCaptureEvent::EVENT_ERROR) == refMap_.end()) {
        MEDIA_LOGW("can not find error callback!");
        return;
    }

    AVScreenCaptureJsCallback *cb = new(std::nothrow) AVScreenCaptureJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");
    cb->autoRef = refMap_.at(AVScreenCaptureEvent::EVENT_ERROR);
    cb->callbackName = AVScreenCaptureEvent::EVENT_ERROR;
    cb->errorCode = errCode;
    cb->errorMsg = msg;
    return OnJsErrorCallBack(cb);
}

void AVScreenCaptureCallback::SendStateCallback(AVScreenCaptureStateCode stateCode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(AVScreenCaptureEvent::EVENT_STATE_CHANGE) == refMap_.end()) {
        MEDIA_LOGW("can not find stateChange callback!");
        return;
    }

    AVScreenCaptureJsCallback *cb = new(std::nothrow) AVScreenCaptureJsCallback();
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");
    cb->autoRef = refMap_.at(AVScreenCaptureEvent::EVENT_STATE_CHANGE);
    cb->callbackName = AVScreenCaptureEvent::EVENT_STATE_CHANGE;
    cb->stateCode = stateCode;
    return OnJsStateChangeCallBack(cb);
}

void AVScreenCaptureCallback::SaveCallbackReference(const std::string &name, std::weak_ptr<AutoRef> ref)
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_[name] = ref;
    MEDIA_LOGI("Set callback type: %{public}s", name.c_str());
}

void AVScreenCaptureCallback::CancelCallbackReference(const std::string &name)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = refMap_.find(name);
    if (iter != refMap_.end()) {
        refMap_.erase(iter);
    }
    MEDIA_LOGI("Cancel callback type: %{public}s", name.c_str());
}

void AVScreenCaptureCallback::ClearCallbackReference()
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_.clear();
    MEDIA_LOGI("ClearCallback!");
}

void AVScreenCaptureCallback::OnError(ScreenCaptureErrorType errorType, int32_t errorCode)
{
    MEDIA_LOGI("OnError is called, name: %{public}d, error message: %{public}d", errorType, errorCode);
    SendErrorCallback(errorCode, "Screen capture failed.");
}

void AVScreenCaptureCallback::OnStateChange(AVScreenCaptureStateCode stateCode)
{
    MEDIA_LOGI("OnStateChange() is called, stateCode: %{public}d", stateCode);
    SendStateCallback(stateCode);
}

void AVScreenCaptureCallback::OnJsErrorCallBack(AVScreenCaptureJsCallback *jsCb) const
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
    int ret = uv_queue_work_with_qos(loop, work, [] (uv_work_t *work) {
        MEDIA_LOGD("OnJsErrorCallBack uv_queue_work_with_qos");
    }, OnJsErrorCallBackWork, uv_qos_user_initiated);
    CHECK_AND_RETURN_LOG(ret == 0, "fail to uv_queue_work_with_qos task");

    CANCEL_SCOPE_EXIT_GUARD(0);
    CANCEL_SCOPE_EXIT_GUARD(1);
}

void AVScreenCaptureCallback::OnJsErrorCallBackWork(uv_work_t *work, int status)
{
    // Js Thread
    CHECK_AND_RETURN_LOG(work != nullptr, "work is nullptr");
    if (work->data == nullptr) {
        delete work;
        MEDIA_LOGE("workdata is nullptr");
        return;
    }
    AVScreenCaptureJsCallback *event = reinterpret_cast<AVScreenCaptureJsCallback *>(work->data);
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
}

void AVScreenCaptureCallback::OnJsStateChangeCallBack(AVScreenCaptureJsCallback *jsCb) const
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
    int ret = uv_queue_work_with_qos(loop, work, [] (uv_work_t *work) {
        MEDIA_LOGD("OnJsStateChangeCallBack uv_queue_work_with_qos");
    }, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "work is nullptr");
        AVScreenCaptureJsCallback *event = reinterpret_cast<AVScreenCaptureJsCallback *>(work->data);
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
            nstatus = napi_create_int32(ref->env_, event->stateCode, &args[0]);
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
    CHECK_AND_RETURN_LOG(ret == 0, "fail to uv_queue_work_with_qos task");

    CANCEL_SCOPE_EXIT_GUARD(0);
    CANCEL_SCOPE_EXIT_GUARD(1);
}

} // namespace Media
} // namespace OHOS