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

#include "media_data_source_napi.h"
#include "media_log.h"
#include "media_errors.h"
#include "securec.h"
#include "common_napi.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaDataSourceNapi"};
}

namespace OHOS {
namespace Media {
thread_local napi_ref MediaDataSourceNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "AVDataSource";
const std::string READ_AT_CALLBACK_NAME = "readAt";

MediaDataSourceNapi::MediaDataSourceNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaDataSourceNapi::~MediaDataSourceNapi()
{
    readAt_ = nullptr;
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value MediaDataSourceNapi::Init(napi_env env, napi_value exports)
{
    MEDIA_LOGD("MediaDataSourceNapi Init start");
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_GETTER_SETTER("size", GetSize, SetSize),
        DECLARE_NAPI_FUNCTION("on", On),
    };
    napi_property_descriptor static_prop[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAVDataSource", CreateAVDataSource),
    };
    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && constructor != nullptr, nullptr, "define class fail");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && constructor_ != nullptr, nullptr, "create reference fail");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "set named property fail");

    status = napi_define_properties(env, exports, sizeof(static_prop) / sizeof(static_prop[0]), static_prop);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "define properties fail");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value MediaDataSourceNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    napi_value jsThis = nullptr;
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "constructor fail");

    MediaDataSourceNapi *sourceNapi = new(std::nothrow) MediaDataSourceNapi();
    CHECK_AND_RETURN_RET_LOG(sourceNapi != nullptr, result, "no memory");

    ON_SCOPE_EXIT(0) { delete sourceNapi; };

    sourceNapi->env_ = env;

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(sourceNapi),
        MediaDataSourceNapi::Destructor, nullptr, &(sourceNapi->wrapper_));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "native wrap fail");

    CANCEL_SCOPE_EXIT_GUARD(0);

    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void MediaDataSourceNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        delete reinterpret_cast<MediaDataSourceNapi *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}

napi_value MediaDataSourceNapi::CreateAVDataSource(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("CreateAVDataSource In");

    auto asyncContext = std::make_unique<AVDataSourceAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    asyncContext->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncContext->deferred = CommonNapi::CreatePromise(env, asyncContext->callbackRef, result);
    asyncContext->JsResult = std::make_unique<MediaJsResultInstance>(constructor_);
    
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "CreateAVDataSource", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();

    MEDIA_LOGD("CreateAVDataSource success");
    return result;
}

napi_value MediaDataSourceNapi::On(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    static constexpr size_t minArgCount = 2;
    size_t argCount = minArgCount;
    napi_value args[minArgCount] = { nullptr };
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr && args[0] != nullptr && args[1] != nullptr,
        undefinedResult, "invalid argument");

    MediaDataSourceNapi *data = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&data);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && data != nullptr, undefinedResult, "set callback fail");
    CHECK_AND_RETURN_RET_LOG(data->noChange_ == false, undefinedResult, "no change");

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        MEDIA_LOGE("invalid arguments type");
        return undefinedResult;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGD("callbackName: %{public}s", callbackName.c_str());

    napi_ref ref = nullptr;
    status = napi_create_reference(env, args[1], 1, &ref);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && ref != nullptr, undefinedResult, "failed to create reference!");
    
    std::shared_ptr<AutoRef> autoRef =  std::make_shared<AutoRef>(env, ref);
    data->SetCallbackReference(callbackName, autoRef);
    return undefinedResult;
}

int32_t MediaDataSourceNapi::CallbackCheckAndSetNoChange()
{
    CHECK_AND_RETURN_RET_LOG(readAt_ != nullptr, MSERR_NO_MEMORY, "readAt is null");
    noChange_ = true;
    return MSERR_OK;
}

napi_value MediaDataSourceNapi::GetSize(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    static constexpr size_t minArgCount = 0;
    size_t argCount = minArgCount;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, undefinedResult, "get args error");

    MediaDataSourceNapi *data = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&data);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && data != nullptr, undefinedResult, "napi_unwrap error");
    napi_value result = nullptr;
    status = napi_create_int64(env, data->size_, &result);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && result != nullptr, undefinedResult, "get napi value error");

    MEDIA_LOGD("GetSize success");
    return result;
}

napi_value MediaDataSourceNapi::SetSize(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    static constexpr size_t minArgCount = 1;
    size_t argCount = minArgCount;
    napi_value args[minArgCount] = { nullptr };
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr && args[0] != nullptr,
        undefinedResult, "get args error");

    MediaDataSourceNapi *data = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&data);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && data != nullptr, undefinedResult, "napi_unwrap error");
    CHECK_AND_RETURN_RET_LOG(data->noChange_ == false, undefinedResult, "no change");
    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        MEDIA_LOGE("invalid arguments type");
        return undefinedResult;
    }

    status = napi_get_value_int64(env, args[0], &data->size_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "get napi value error");

    MEDIA_LOGD("SetSize success");
    return undefinedResult;
}

void MediaDataSourceNapi::SetCallbackReference(const std::string &callbackName, std::shared_ptr<AutoRef> ref)
{
    if (callbackName == READ_AT_CALLBACK_NAME) {
        readAt_ = ref;
        CHECK_AND_RETURN_LOG(readAt_ != nullptr, "creating reference for readAt_ fail")
    } else {
        MEDIA_LOGE("unknown callback: %{public}s", callbackName.c_str());
        return;
    }
}

int32_t MediaDataSourceNapi::GetSize(int64_t &size) const
{
    size = size_;
    return MSERR_OK;
}

int32_t MediaDataSourceNapi::GetReadAtRef(std::shared_ptr<AutoRef> &readAtRef) const
{
    readAtRef = readAt_;
    CHECK_AND_RETURN_RET_LOG(readAtRef != nullptr, MSERR_UNKNOWN, "get reference fail");
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
