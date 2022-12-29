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

#include "media_data_source_callback.h"
#include <uv.h>
#include "media_log.h"
#include "media_errors.h"
#include "scope_guard.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaDataSourceImpl"};
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
    MediaDataSourceJsCallback *cb = new(std::nothrow) MediaDataSourceJsCallback();
    CHECK_AND_RETURN_RET_LOG(cb != nullptr, 0, "Failed to Create MediaDataSourceJsCallback");

    cb->callback = refMap_.at(READAT_CALLBACK_NAME);
    cb->callbackName = READAT_CALLBACK_NAME;
    cb->memory = mem;
    cb->length = length;
    cb->pos = pos;

    ON_SCOPE_EXIT(0) { delete cb; };
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    CHECK_AND_RETURN_RET_LOG(loop != nullptr, 0, "Failed to get uv event loop");

    uv_work_t *work = new(std::nothrow) uv_work_t;
    CHECK_AND_RETURN_RET_LOG(work != nullptr, 0, "Failed to new uv_work_t");
    work->data = reinterpret_cast<void *>(cb);
    // async callback, jsWork and jsWork->data should be heap object.
    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        // Js Thread
        CHECK_AND_RETURN_LOG(work != nullptr, "Work thread is nullptr");
        MediaDataSourceJsCallback *event = reinterpret_cast<MediaDataSourceJsCallback *>(work->data);
        CHECK_AND_RETURN_LOG(event != nullptr, "work data is null");
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", event->callbackName.c_str());
        do {
            CHECK_AND_BREAK(status != UV_ECANCELED);
            std::shared_ptr<AutoRef> ref = event->callback.lock();
            CHECK_AND_BREAK_LOG(ref != nullptr, "%{public}s AutoRef is nullptr", event->callbackName.c_str());

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_BREAK(nstatus == napi_ok && jsCallback != nullptr);

            napi_value args[3] = { nullptr };
            nstatus = napi_create_external_arraybuffer(ref->env_, event->memory->GetBase(), static_cast<size_t>(event->memory->GetSize()),
                [](napi_env env, void *data, void *hint) {}, nullptr, &args[0]);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "create napi val failed");
            nstatus = napi_create_uint32(ref->env_, event->length, &args[1]);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "create napi val failed");
            nstatus = napi_create_int64(ref->env_, event->pos, &args[2]);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "create napi val failed");
            
            napi_value result = nullptr;
            nstatus = napi_call_function(ref->env_, nullptr, jsCallback, 3, args, &result);
            CHECK_AND_BREAK(nstatus == napi_ok);
            nstatus = napi_get_value_int32(ref->env_, result, &event->size);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "get value for ref failed");
        } while (0);
        delete event;
        delete work;
    });
    if (ret != 0) {
        MEDIA_LOGE("Failed to execute libuv work queue");
        delete work;
        return -2;
    }
    return cb->size;
}

int32_t MediaDataSourceCallback::GetSize(int64_t &size)
{
    size = size_;
    return MSERR_OK;
}

void MediaDataSourceCallback::SaveCallbackReference(const std::string &name, std::weak_ptr<AutoRef> ref)
{
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
