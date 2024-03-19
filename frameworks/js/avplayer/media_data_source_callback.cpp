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

#include "media_data_source_callback.h"
#include "buffer/avsharedmemory.h"
#include "media_dfx.h"
#include "media_log.h"
#include "media_errors.h"
#include "scope_guard.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaDataSourceCallback"};
}

namespace OHOS {
namespace Media {
MediaDataSourceJsCallback::~MediaDataSourceJsCallback()
{
    isExit_ = true;
    cond_.notify_all();
    memory_ = nullptr;
}

void MediaDataSourceJsCallback::WaitResult()
{
    std::unique_lock<std::mutex> lock(mutexCond_);
    if (!setResult_) {
        static constexpr int32_t timeout = 100;
        cond_.wait_for(lock, std::chrono::milliseconds(timeout), [this]() { return setResult_ || isExit_; });
        if (!setResult_) {
            readSize_ = 0;
            if (isExit_) {
                MEDIA_LOGW("Reset, ReadAt has been cancel!");
            } else {
                MEDIA_LOGW("timeout 100ms!");
            }
        }
    }
    setResult_ = false;
}

MediaDataSourceCallback::MediaDataSourceCallback(napi_env env, int64_t fileSize)
    : env_(env),
      size_(fileSize)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaDataSourceCallback::~MediaDataSourceCallback()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    env_ = nullptr;
}

int32_t MediaDataSourceCallback::ReadAt(const std::shared_ptr<AVSharedMemory> &mem, uint32_t length, int64_t pos)
{
    MEDIA_LOGD("ReadAt in");
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (refMap_.find(READAT_CALLBACK_NAME) == refMap_.end()) {
            return SOURCE_ERROR_IO;
        }
        cb_ = std::make_shared<MediaDataSourceJsCallback>(READAT_CALLBACK_NAME, mem, length, pos);
        CHECK_AND_RETURN_RET_LOG(cb_ != nullptr, 0, "Failed to Create MediaDataSourceJsCallback");
        cb_->callback_ = refMap_.at(READAT_CALLBACK_NAME);
    }
    ON_SCOPE_EXIT(0) {
        cb_ = nullptr;
    };

    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    CHECK_AND_RETURN_RET_LOG(loop != nullptr, 0, "Failed to get uv event loop");
    uv_work_t *work = new(std::nothrow) uv_work_t;
    CHECK_AND_RETURN_RET_LOG(work != nullptr, 0, "Failed to new uv_work_t");
    ON_SCOPE_EXIT(1) {
        delete work;
    };

    MediaDataSourceJsCallbackWraper *cbWrap = new(std::nothrow) MediaDataSourceJsCallbackWraper();
    CHECK_AND_RETURN_RET_LOG(cbWrap != nullptr, 0, "Failed to new MediaDataSourceJsCallbackWraper");
    cbWrap->cb_ = cb_;
    work->data = reinterpret_cast<void *>(cbWrap);
    // async callback, jsWork and jsWork->data should be heap object.
    int ret = UvWork(loop, work);
    CHECK_AND_RETURN_RET_LOG(ret == 0, SOURCE_ERROR_IO, "Failed to execute uv queue work");
    CANCEL_SCOPE_EXIT_GUARD(1);
    cb_->WaitResult();
    MEDIA_LOGD("ReadAt out");
    return cb_->readSize_;
}

int32_t MediaDataSourceCallback::UvWork(uv_loop_s *loop, uv_work_t *work)
{
    MEDIA_LOGD("begin UvWork");
    return uv_queue_work_with_qos(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr && work->data != nullptr, "work is nullptr");
        MediaDataSourceJsCallbackWraper *wrap = reinterpret_cast<MediaDataSourceJsCallbackWraper *>(work->data);
        std::shared_ptr<MediaDataSourceJsCallback> event = wrap->cb_.lock();
        CHECK_AND_RETURN_LOG(event != nullptr, "MediaDataSourceJsCallback is nullptr");
        MEDIA_LOGD("length is %{public}u", event->length_);
        do {
            CHECK_AND_BREAK(status != UV_ECANCELED);
            std::shared_ptr<AutoRef> ref = event->callback_.lock();
            CHECK_AND_BREAK_LOG(ref != nullptr, "%{public}s AutoRef is nullptr", event->callbackName_.c_str());

            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(ref->env_, &scope);
            CHECK_AND_BREAK_LOG(scope != nullptr, "%{public}s scope is nullptr", event->callbackName_.c_str());
            ON_SCOPE_EXIT(0) {
                napi_close_handle_scope(ref->env_, scope);
            };

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_BREAK(nstatus == napi_ok && jsCallback != nullptr);

            // noseek mode don't need pos, so noseek mode need 2 parameters and seekable mode need 3 parameters
            int32_t paramNum;
            napi_value args[3] = { nullptr };
            CHECK_AND_BREAK_LOG(event->memory_ != nullptr, "failed to checkout memory");
            nstatus = napi_create_external_arraybuffer(ref->env_, event->memory_->GetBase(),
                static_cast<size_t>(event->length_), [](napi_env env, void *data, void *hint) {}, nullptr, &args[0]);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "create napi arraybuffer failed");
            CHECK_AND_BREAK_LOG(napi_create_uint32(ref->env_, event->length_, &args[1]) == napi_ok,
                "set length failed");
            if (event->pos_ != -1) {
                paramNum = 3;  // 3 parameters
                CHECK_AND_BREAK_LOG(napi_create_int64(ref->env_, event->pos_, &args[2]) == napi_ok,  // 2 parameters
                    "set pos failed");
            } else {
                paramNum = 2;  // 2 parameters
            }

            napi_value size;
            MEDIA_LOGD("call JS function");
            nstatus = napi_call_function(ref->env_, nullptr, jsCallback, paramNum, args, &size);
            CHECK_AND_BREAK(nstatus == napi_ok);
            nstatus = napi_get_value_int32(ref->env_, size, &event->readSize_);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "get size failed");
            std::unique_lock<std::mutex> lock(event->mutexCond_);
            event->setResult_ = true;
            event->cond_.notify_all();
        } while (0);
        delete work;
    }, uv_qos_user_initiated);
}

int32_t MediaDataSourceCallback::ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
{
    (void)pos;
    (void)length;
    (void)mem;
    return MSERR_OK;
}

int32_t MediaDataSourceCallback::ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
{
    (void)length;
    (void)mem;
    return MSERR_OK;
}

int32_t MediaDataSourceCallback::GetSize(int64_t &size)
{
    size = size_;
    return MSERR_OK;
}

void MediaDataSourceCallback::SaveCallbackReference(const std::string &name, std::shared_ptr<AutoRef> ref)
{
    MEDIA_LOGD("Add Callback: %{public}s", name.c_str());
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_[name] = ref;
}

int32_t MediaDataSourceCallback::GetCallback(const std::string &name, napi_value *callback)
{
    (void)name;
    if (refMap_.find(READAT_CALLBACK_NAME) == refMap_.end()) {
        return MSERR_INVALID_VAL;
    }
    auto ref = refMap_.at(READAT_CALLBACK_NAME);
    napi_status nstatus = napi_get_reference_value(ref->env_, ref->cb_, callback);
    CHECK_AND_RETURN_RET(nstatus == napi_ok && callback != nullptr, MSERR_INVALID_OPERATION);
    return MSERR_OK;
}

void MediaDataSourceCallback::ClearCallbackReference()
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::map<std::string, std::shared_ptr<AutoRef>> temp;
    temp.swap(refMap_);
    MEDIA_LOGD("callback has been clear");
    if (cb_) {
        cb_->isExit_ = true;
        cb_->cond_.notify_all();
    }
}

bool MediaDataSourceCallback::AddNapiValueProp(napi_env env, napi_value obj, const std::string &key, napi_value value)
{
    CHECK_AND_RETURN_RET(obj != nullptr, false);

    napi_value keyNapi = nullptr;
    napi_status status = napi_create_string_utf8(env, key.c_str(), NAPI_AUTO_LENGTH, &keyNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    status = napi_set_property(env, obj, keyNapi, value);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, false, "Failed to set property");

    return true;
}
} // namespace Media
} // namespace OHOS
