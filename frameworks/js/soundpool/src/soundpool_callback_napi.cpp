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
#include "soundpool_callback_napi.h"
#include <uv.h>
#include "media_errors.h"
#include "media_log.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SoundPoolCallBackNapi"};
}

namespace OHOS {
namespace Media {
SoundPoolCallBackNapi::SoundPoolCallBackNapi(napi_env env)
{
    env_ = env;
    MEDIA_LOGI("0x%{public}06" PRIXPTR "Instances create", FAKE_POINTER(this));
}

void SoundPoolCallBackNapi::OnLoadCompleted(int32_t soundId)
{
    MEDIA_LOGI("OnLoadCompleted recived soundId:%{public}d", soundId);
    SendLoadCompletedCallback(soundId);
}

void SoundPoolCallBackNapi::OnPlayFinished()
{
    MEDIA_LOGI("OnPlayFinished recived");
    SendPlayCompletedCallback();
}

void SoundPoolCallBackNapi::OnError(int32_t errorCode)
{
    MEDIA_LOGI("OnError recived:error:%{public}d", errorCode);
    if (errorCode == MSERR_INVALID_OPERATION) {
        SendErrorCallback(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
            "The soundpool timed out. Please confirm that the input stream is normal.");
    } else if (errorCode == MSERR_NO_MEMORY) {
        SendErrorCallback(MSERR_EXT_API9_NO_MEMORY, "soundpool memery error.");
    } else if (errorCode == MSERR_SERVICE_DIED) {
        SendErrorCallback(MSERR_EXT_API9_SERVICE_DIED, "releated server died");
    } else {
        SendErrorCallback(MSERR_EXT_API9_IO, "IO error happened.");
    }
}

void SoundPoolCallBackNapi::SaveCallbackReference(const std::string &name, std::weak_ptr<AutoRef> ref)
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_[name] = ref;
    MEDIA_LOGI("Set callback type: %{public}s", name.c_str());
}

void SoundPoolCallBackNapi::CancelCallbackReference(const std::string &name)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = refMap_.find(name);
    if (iter != refMap_.end()) {
        refMap_.erase(iter);
    }
    MEDIA_LOGI("Cancel callback type: %{public}s", name.c_str());
}

void SoundPoolCallBackNapi::ClearCallbackReference()
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_.clear();
    MEDIA_LOGI("ClearCallback!");
}

void SoundPoolCallBackNapi::SendErrorCallback(int32_t errCode, const std::string &msg)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(SoundPoolEvent::EVENT_ERROR) == refMap_.end()) {
        MEDIA_LOGW("can not find error callback!");
        return;
    }

    SoundPoolJsCallBack *cb = new(std::nothrow) SoundPoolJsCallBack();
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");
    cb->autoRef = refMap_.at(SoundPoolEvent::EVENT_ERROR);
    cb->callbackName = SoundPoolEvent::EVENT_ERROR;
    cb->errorCode = errCode;
    cb->errorMsg = msg;
    return OnJsErrorCallBack(cb);
}

void SoundPoolCallBackNapi::SendLoadCompletedCallback(int32_t soundId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(SoundPoolEvent::EVENT_LOAD_COMPLETED) == refMap_.end()) {
        MEDIA_LOGW("can not find loadcompleted callback!");
        return;
    }

    SoundPoolJsCallBack *cb = new(std::nothrow) SoundPoolJsCallBack();
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");
    cb->autoRef = refMap_.at(SoundPoolEvent::EVENT_LOAD_COMPLETED);
    cb->callbackName = SoundPoolEvent::EVENT_LOAD_COMPLETED;
    cb->loadSoundId = soundId;
    return OnJsloadCompletedCallBack(cb);
}

void SoundPoolCallBackNapi::SendPlayCompletedCallback()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(SoundPoolEvent::EVENT_PLAY_FINISHED) == refMap_.end()) {
        MEDIA_LOGW("can not find playfinished callback!");
        return;
    }

    SoundPoolJsCallBack *cb = new(std::nothrow) SoundPoolJsCallBack();
    CHECK_AND_RETURN_LOG(cb != nullptr, "cb is nullptr");
    cb->autoRef = refMap_.at(SoundPoolEvent::EVENT_PLAY_FINISHED);
    cb->callbackName = SoundPoolEvent::EVENT_PLAY_FINISHED;
    return OnJsplayCompletedCallBack(cb);
}

void SoundPoolCallBackNapi::OnJsErrorCallBack(SoundPoolJsCallBack *jsCb) const
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
    }, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "work is nullptr");
        SoundPoolJsCallBack *event = reinterpret_cast<SoundPoolJsCallBack *>(work->data);
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
    CHECK_AND_RETURN_LOG(ret == 0, "fail to uv_queue_work_with_qos task");
    CANCEL_SCOPE_EXIT_GUARD(0);
    CANCEL_SCOPE_EXIT_GUARD(1);
}

void SoundPoolCallBackNapi::OnJsloadCompletedCallBack(SoundPoolJsCallBack *jsCb) const
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
        MEDIA_LOGD("OnJsloadCompletedCallBack uv_queue_work_with_qos");
    }, [] (uv_work_t *work, int status) {
        CHECK_AND_RETURN_LOG(work != nullptr, "work is nullptr");
        if (work->data != nullptr) {
            MEDIA_LOGD("work data not nullptr");
            SoundPoolJsCallBack *event = reinterpret_cast<SoundPoolJsCallBack *>(work->data);
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
                nstatus = napi_create_int32(ref->env_, event->loadSoundId, &args[0]);
                CHECK_AND_BREAK_LOG(nstatus == napi_ok && args[0] != nullptr,
                    "%{public}s fail to create callback", request.c_str());
                const size_t argCount = 1;
                napi_value result = nullptr;
                nstatus = napi_call_function(ref->env_, nullptr, jsCallback, argCount, args, &result);
                CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s fail to napi call function", request.c_str());
            } while (0);
            delete event;
        }
        delete work;
    }, uv_qos_user_initiated);
    CHECK_AND_RETURN_LOG(ret == 0, "fail to uv_queue_work_with_qos task");
    CANCEL_SCOPE_EXIT_GUARD(0);
    CANCEL_SCOPE_EXIT_GUARD(1);
}

void SoundPoolCallBackNapi::OnJsplayCompletedCallBack(SoundPoolJsCallBack *jsCb) const
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
        MEDIA_LOGD("OnJsplayCompletedCallBack uv_queue_work_with_qos");
    }, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "work is nullptr");
        if (work->data != nullptr) {
            MEDIA_LOGI("work data not nullptr");
            SoundPoolJsCallBack *event = reinterpret_cast<SoundPoolJsCallBack *>(work->data);
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

                napi_value result = nullptr;
                nstatus = napi_call_function(ref->env_, nullptr, jsCallback, 0, nullptr, &result);
                CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s fail to napi call function", request.c_str());
            } while (0);
            delete event;
        }
        delete work;
    }, uv_qos_user_initiated);
    CHECK_AND_RETURN_LOG(ret == 0, "fail to uv_queue_work_with_qos task");

    CANCEL_SCOPE_EXIT_GUARD(0);
    CANCEL_SCOPE_EXIT_GUARD(1);
}
} // namespace Media
} // namespace OHOS