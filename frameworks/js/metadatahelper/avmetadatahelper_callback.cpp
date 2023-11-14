/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "avmetadatahelper_callback.h"
#include <uv.h>
#include "media_errors.h"
#include "media_log.h"
#include "scope_guard.h"
#include "event_queue.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetadataHelperCallback"};
}

namespace OHOS {
namespace Media {
class NapiCallback {
public:
    struct Base {
        std::weak_ptr<AutoRef> callback;
        std::string callbackName = "unknown";
        Base() = default;
        virtual ~Base() = default;
        virtual void UvWork()
        {
            std::shared_ptr<AutoRef> ref = callback.lock();
            CHECK_AND_RETURN_LOG(ref != nullptr,
                "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(ref->env_, &scope);
            CHECK_AND_RETURN_LOG(scope != nullptr,
                "%{public}s scope is nullptr", callbackName.c_str());
            ON_SCOPE_EXIT(0) {
                napi_close_handle_scope(ref->env_, scope);
            };

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            // Call back function
            napi_value result = nullptr;
            status = napi_call_function(ref->env_, nullptr, jsCallback, 0, nullptr, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok,
                "%{public}s failed to napi_call_function", callbackName.c_str());
        }
        virtual void JsCallback()
        {
            UvWork();
            delete this;
        }
    };

    struct Error : public Base {
        std::string errorMsg = "unknown";
        MediaServiceExtErrCodeAPI9 errorCode = MSERR_EXT_API9_UNSUPPORT_FORMAT;
        void UvWork() override
        {
            std::shared_ptr<AutoRef> ref = callback.lock();
            CHECK_AND_RETURN_LOG(ref != nullptr,
                "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(ref->env_, &scope);
            CHECK_AND_RETURN_LOG(scope != nullptr,
                "%{public}s scope is nullptr", callbackName.c_str());
            ON_SCOPE_EXIT(0) {
                napi_close_handle_scope(ref->env_, scope);
            };

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            napi_value args[1] = {nullptr};
            (void)CommonNapi::CreateError(ref->env_, errorCode, errorMsg, args[0]);

            // Call back function
            napi_value result = nullptr;
            status = napi_call_function(ref->env_, nullptr, jsCallback, 1, args, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok,
                "%{public}s failed to napi_call_function", callbackName.c_str());
        }
    };

    static void CompleteCallback(napi_env env, NapiCallback::Base *jsCb)
    {
        ON_SCOPE_EXIT(0) {
            delete jsCb;
        };

        uv_loop_s *loop = nullptr;
        napi_get_uv_event_loop(env, &loop);
        CHECK_AND_RETURN_LOG(loop != nullptr, "Fail to napi_get_uv_event_loop");

        uv_work_t *work = new(std::nothrow) uv_work_t;
        CHECK_AND_RETURN_LOG(work != nullptr, "Fail to new uv_work_t");

        work->data = reinterpret_cast<void *>(jsCb);
        // async callback, jsWork and jsWork->data should be heap object.
        int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
            CHECK_AND_RETURN_LOG(work != nullptr, "Work thread is nullptr");
            (void)status;
            NapiCallback::Base *cb = reinterpret_cast<NapiCallback::Base *>(work->data);
            if (cb != nullptr) {
                MEDIA_LOGI("JsCallBack %{public}s, uv_queue_work start", cb->callbackName.c_str());
                cb->UvWork();
                delete cb;
            }
            delete work;
        });
        if (ret != 0) {
            MEDIA_LOGE("Failed to execute libuv work queue");
            delete jsCb;
            delete work;
        }
        CANCEL_SCOPE_EXIT_GUARD(0);
    }
};

AVMetadataHelperCallback::AVMetadataHelperCallback(napi_env env, AVMetadataHelperNotify *listener)
    : env_(env), listener_(listener)
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    onInfoFuncs_[HELPER_INFO_TYPE_STATE_CHANGE] = &AVMetadataHelperCallback::OnStateChangeCb;
}

AVMetadataHelperCallback::~AVMetadataHelperCallback()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void AVMetadataHelperCallback::OnError(int32_t errorCode, const std::string &errorMsg)
{
    if (errorCode == HelperErrorType::INVALID_RESULT) {
        MEDIA_LOGE("OnError: errorCode %{public}d, errorMsg %{public}s", errorCode, errorMsg.c_str());
        AVMetadataHelperCallback::OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, errorMsg);
        return;
    }
    MediaServiceExtErrCodeAPI9 errorCodeApi9 = MSErrorToExtErrorAPI9(static_cast<MediaServiceErrCode>(errorCode));
    AVMetadataHelperCallback::OnErrorCb(errorCodeApi9, errorMsg);
}

void AVMetadataHelperCallback::OnErrorCb(MediaServiceExtErrCodeAPI9 errorCode, const std::string &errorMsg)
{
    std::string message = MSExtAVErrorToString(errorCode) + errorMsg;
    MEDIA_LOGE("OnErrorCb:errorCode %{public}d, errorMsg %{public}s", errorCode, message.c_str());
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(AVMetadataHelperEvent::EVENT_ERROR) == refMap_.end()) {
        MEDIA_LOGW("can not find error callback!");
        return;
    }

    NapiCallback::Error *cb = new(std::nothrow) NapiCallback::Error();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new Error");

    cb->callback = refMap_.at(AVMetadataHelperEvent::EVENT_ERROR);
    cb->callbackName = AVMetadataHelperEvent::EVENT_ERROR;
    cb->errorCode = errorCode;
    cb->errorMsg = message;
    NapiCallback::CompleteCallback(env_, cb);
}

void AVMetadataHelperCallback::OnInfo(HelperOnInfoType type, int32_t extra, const Format &infoBody)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGI("OnInfo is called, OnInfoType: %{public}d", type);
    if (onInfoFuncs_.count(type) > 0) {
        (this->*onInfoFuncs_[type])(extra, infoBody);
    } else {
        MEDIA_LOGI("OnInfo: no member func supporting, %{public}d", type);
    }
}

void AVMetadataHelperCallback::OnStateChangeCb(const int32_t extra, const Format &infoBody)
{
    HelperStates state = static_cast<HelperStates>(extra);
    MEDIA_LOGI("OnStateChanged is called, current state: %{public}d", state);

    if (listener_ != nullptr) {
        listener_->NotifyState(state);
    }
}

void AVMetadataHelperCallback::SaveCallbackReference(const std::string &name, std::weak_ptr<AutoRef> ref)
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_[name] = ref;
}

void AVMetadataHelperCallback::ClearCallbackReference()
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_.clear();
}

void AVMetadataHelperCallback::ClearCallbackReference(const std::string &name)
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_.erase(name);
}

void AVMetadataHelperCallback::Start()
{
    isloaded_ = true;
}

void AVMetadataHelperCallback::Pause()
{
    isloaded_ = false;
}

void AVMetadataHelperCallback::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);

    Format infoBody;
    AVMetadataHelperCallback::OnStateChangeCb(HelperStates::HELPER_RELEASED, infoBody);
    listener_ = nullptr;
}
} // namespace Media
} // namespace OHOS