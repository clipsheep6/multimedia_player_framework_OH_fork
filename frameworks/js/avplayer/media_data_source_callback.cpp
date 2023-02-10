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
#include <uv.h>
#include "media_dfx.h"
#include "media_log.h"
#include "media_errors.h"
#include "scope_guard.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaDataSourceCallback"};
}

namespace OHOS {
namespace Media {
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
    MediaDataSourceJsCallback *cb = new(std::nothrow) MediaDataSourceJsCallback(READAT_CALLBACK_NAME, mem, length, pos);
    CHECK_AND_RETURN_RET_LOG(cb != nullptr, 0, "Failed to Create MediaDataSourceJsCallback");
    cb->callback_ = refMap_.at(READAT_CALLBACK_NAME);
    ON_SCOPE_EXIT(0) { delete cb; };

    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    CHECK_AND_RETURN_RET_LOG(loop != nullptr, 0, "Failed to get uv event loop");
    uv_work_t *work = new(std::nothrow) uv_work_t;
    CHECK_AND_RETURN_RET_LOG(work != nullptr, 0, "Failed to new uv_work_t");
    ON_SCOPE_EXIT(1) { delete work; };
    
    work->data = reinterpret_cast<void *>(cb);
    // async callback, jsWork and jsWork->data should be heap object.
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        MediaDataSourceJsCallback *event = reinterpret_cast<MediaDataSourceJsCallback *>(work->data);
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", event->callbackName_.c_str());
        do {
            CHECK_AND_BREAK(status != UV_ECANCELED);
            std::shared_ptr<AutoRef> ref = event->callback_;
            CHECK_AND_BREAK_LOG(ref != nullptr, "%{public}s AutoRef is nullptr", event->callbackName_.c_str());

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_BREAK(nstatus == napi_ok && jsCallback != nullptr);

            napi_value args[3] = { nullptr };
            MEDIA_LOGD("offset is %{public}u, length is %{public}u", event->memory_->GetOffset(), event->length_);
            nstatus = napi_create_external_arraybuffer(ref->env_, event->memory_->GetBaseWithOffset(),
                static_cast<size_t>(event->length_), [](napi_env env, void *data, void *hint) {}, nullptr, &args[0]);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "create napi arraybuffer failed");
            nstatus = napi_create_uint32(event->callback_->env_, event->length_, &args[1]);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "create napi length failed");
            nstatus = napi_create_int64(event->callback_->env_, event->pos_, &args[2]);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "create napi pos failed");
            
            napi_value size;
            {
                MediaTrace trace("MediaDataSourceCallback::ReadAt");
                nstatus = napi_call_function(ref->env_, nullptr, jsCallback, 3, args, &size);
            }
            CHECK_AND_BREAK(nstatus == napi_ok);
            event->SetResult(size);
        } while (0);
    });
    CHECK_AND_RETURN_RET_LOG(ret == 0, SOURCE_ERROR_IO, "Failed to execute uv queue work");

    cb->WaitResult();
    int32_t size;
    napi_status status = napi_get_value_int32(cb->callback_->env_, cb->readSize_, &size);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, 0, "get size failed");
    return size;
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

void MediaDataSourceCallback::ClearCallbackReference()
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_.clear();
}
} // namespace Media
} // namespace OHOS
