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
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaDataSourceImpl"};
}

namespace OHOS {
namespace Media {
const std::string READ_AT_CALLBACK_NAME = "readAt";

struct AvMemNapiWarp {
    explicit AvMemNapiWarp(const std::shared_ptr<AVSharedMemory> &mem) : mem_(mem)
    {
        MEDIA_LOGD("0x%{public}06" PRIXPTR " AvMemNapiWarp Instances create", FAKE_POINTER(this));
    };
    ~AvMemNapiWarp()
    {
        MEDIA_LOGD("0x%{public}06" PRIXPTR " AvMemNapiWarp Instances destroy", FAKE_POINTER(this));
    };
    std::shared_ptr<AVSharedMemory> mem_;
};

std::shared_ptr<MediaDataSourceCallback> MediaDataSourceCallback::Create(napi_env env, napi_value dataSrcNapi)
{
    CHECK_AND_RETURN_RET_LOG(env != nullptr && dataSrcNapi != nullptr, nullptr, "env or src is nullptr");
    MediaDataSourceNapi *data = nullptr;
    napi_status status = napi_unwrap(env, dataSrcNapi, reinterpret_cast<void **>(&data));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && data != nullptr, nullptr, "unwarp failed");
    CHECK_AND_RETURN_RET_LOG(data->CallbackCheckAndSetNoChange() == MSERR_OK, nullptr, "check callback failed");
    uint32_t refCount = 1;
    napi_ref ref = nullptr;
    status = napi_create_reference(env, dataSrcNapi, refCount, &(ref));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && ref != nullptr, nullptr, "create ref failed");
    std::shared_ptr<MediaDataSourceCallback> mediaDataSource =
        std::make_shared<MediaDataSourceCallback>(env, ref, *data);
    if (mediaDataSource == nullptr) {
        status = napi_reference_unref(env, ref, &refCount);
        CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "unref failed");
    }
    return mediaDataSource;
}

MediaDataSourceCallback::MediaDataSourceCallback(napi_env env, napi_ref ref, MediaDataSourceNapi &src)
    : env_(env),
      napiSrcRef_(ref),
      dataSrc_(src)
{
    src.GetReadAtRef(readAt_);
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaDataSourceCallback::~MediaDataSourceCallback()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    if (napiSrcRef_ != nullptr) {
        uint32_t refCount = 0;
        napi_reference_unref(env_, napiSrcRef_, &refCount);
        napiSrcRef_ = nullptr;
    }
    env_ = nullptr;
}

void MediaDataSourceCallback::Release() const
{
    if(works_ != nullptr){
        int status = uv_cancel(reinterpret_cast<uv_req_t*>(works_.get()));
        if (status != 0) {
            MEDIA_LOGE("work cancel failed");
        }
    }
}

napi_value MediaDataSourceCallback::GetDataSrc() const
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env_, &undefinedResult);
    napi_value jsResult = nullptr;
    napi_status status = napi_get_reference_value(env_, napiSrcRef_, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsResult != nullptr, undefinedResult, "get reference value fail");
    return jsResult;
}

MediaDataSourceCallback::CallbackWarp::CallbackWarp(std::shared_ptr<JsCallbackContext> &jsCb) : jsCb_(jsCb) 
{
    argv_ = static_cast<napi_value *>(malloc(sizeof(napi_value) * jsCb->argsCount));
    MEDIA_LOGD("CallbackWarp instances create");
}

MediaDataSourceCallback::CallbackWarp::~CallbackWarp()
{
    MEDIA_LOGD("CallbackWarp instances destroy");
}

napi_value *MediaDataSourceCallback::CallbackWarp::GetArgs()
{
    napi_status status = napi_create_uint32(jsCb_->callback->env_, jsCb_->length, &argv_[0]);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "create napi val failed");

    AvMemNapiWarp *memWarp = new(std::nothrow) AvMemNapiWarp(jsCb_->mem);
    CHECK_AND_RETURN_RET_LOG(memWarp != nullptr, nullptr, "AvMemNapiWarp is null");
    status = napi_create_external_arraybuffer(jsCb_->callback->env_, jsCb_->mem->GetBase(),
        static_cast<size_t>(jsCb_->mem->GetSize()), [] (napi_env env, void *data, void *hint) {
            (void)env;
            (void)data;
            AvMemNapiWarp *memWarp = reinterpret_cast<AvMemNapiWarp *>(hint);
            delete memWarp;
        }, reinterpret_cast<void *>(memWarp), &argv_[1]);
    if (status != napi_ok) {
        delete memWarp;
        MEDIA_LOGE("create napi val failed");
        return nullptr;
    }
    if (jsCb_->argsCount == 3) {
        status = napi_create_int64(jsCb_->callback->env_, jsCb_->pos, &argv_[2]);
        CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "create napi val failed");
    }
    return argv_;
}

void MediaDataSourceCallback::CallbackWarp::SetResult(int32_t size)
{
    std::unique_lock<std::mutex> lock(mutex_);
    isSetResult_ = true;
    resultValue_ = size;
    condVarResult_.notify_all();
}

void MediaDataSourceCallback::CallbackWarp::GetResult(int32_t &size)
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("get result");
    if (!isSetResult_) {
        condVarResult_.wait(lock);
    }
    size = resultValue_;
}

std::shared_ptr<AutoRef> &MediaDataSourceCallback::CallbackWarp::GetCallback()
{
    return jsCb_->callback;
}

std::string MediaDataSourceCallback::CallbackWarp::GetCallbackName()
{
    return jsCb_->callbackName;
}

size_t MediaDataSourceCallback::CallbackWarp::GetArgsCount()
{
    return jsCb_->argsCount;
}

int32_t MediaDataSourceCallback::ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
{
    CHECK_AND_RETURN_RET_LOG(env_ != nullptr, 0, "env is nullptr");
    CHECK_AND_RETURN_RET_LOG(readAt_ != nullptr, 0, "readAt_ is nullptr");

    std::shared_ptr<JsCallbackContext> cb = std::make_shared<JsCallbackContext>(3);
    CHECK_AND_RETURN_RET_LOG(cb != nullptr, 0, "cb is nullptr");

    cb->callback = readAt_;
    cb->callbackName = READ_AT_CALLBACK_NAME;
    cb->length = length;
    cb->mem = mem;
    cb->pos = pos;

    CallbackWarp *callbackWarp = new(std::nothrow) CallbackWarp(cb);

    return JsCallback(callbackWarp);
}

int32_t MediaDataSourceCallback::ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
{
    CHECK_AND_RETURN_RET_LOG(env_ != nullptr, 0, "env is nullptr");
    CHECK_AND_RETURN_RET_LOG(readAt_ != nullptr, 0, "readAt_ is nullptr");
   
    std::shared_ptr<JsCallbackContext> cb = std::make_shared<JsCallbackContext>(2);
    CHECK_AND_RETURN_RET_LOG(cb != nullptr, 0, "cb is nullptr");

    cb->callback = readAt_;
    cb->callbackName = READ_AT_CALLBACK_NAME;
    cb->length = length;
    cb->mem = mem;

    CallbackWarp *callbackWarp = new(std::nothrow) CallbackWarp(cb);

    return JsCallback(callbackWarp);
}

int32_t MediaDataSourceCallback::GetSize(int64_t &size)
{
    return dataSrc_.GetSize(size);
}

int32_t MediaDataSourceCallback::JsCallback(CallbackWarp *callbackWarp)
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        delete callbackWarp;
        return 0;
    }

    uv_work_t *work = new(std::nothrow) uv_work_t;
    works_ = std::make_shared<uv_work_t*>(work);
    if (work == nullptr) {
        MEDIA_LOGE("No memory");
        delete callbackWarp;
        return 0;
    }
    work->data = reinterpret_cast<void *>(callbackWarp);

    int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
        CHECK_AND_RETURN_LOG(work != nullptr, "work is nullptr");
        CallbackWarp *event = reinterpret_cast<CallbackWarp *>(work->data);
        std::string request = event->GetCallbackName();
        MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work start", request.c_str());
        do {
            CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());
            std::shared_ptr<AutoRef> ref = event->GetCallback();
            CHECK_AND_BREAK_LOG(ref != nullptr, "%{public}s AutoRef is nullptr", request.c_str());

            napi_value jsCallback = nullptr;
            napi_status nstatus = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "%{public}s get reference value fail",
                request.c_str());

            int32_t size = 0;
            napi_value result = nullptr;
            nstatus = napi_call_function(ref->env_, nullptr, jsCallback, event->GetArgsCount(), event->GetArgs(), &result);
            CHECK_AND_BREAK_LOG(nstatus == napi_ok, "%{public}s fail to napi call function", request.c_str());
            status = napi_get_value_int32(ref->env_, result, &size);
            CHECK_AND_BREAK_LOG(status == napi_ok, "get value for ref failed");
            event->SetResult(size);
        } while (0);
        delete work;
    });
    if (ret != 0) {
        MEDIA_LOGE("Failed to execute libuv work queue");
        delete callbackWarp;
        delete work;
    }
    
    int32_t size = 0;
    callbackWarp->GetResult(size);
    delete callbackWarp;
    works_ = nullptr;
    return size; 
}

} // namespace Media
} // namespace OHOS
