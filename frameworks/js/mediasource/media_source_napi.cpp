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

#include "media_source_napi.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaSourceNapi"};
}

namespace OHOS {
namespace Media {
const std::string CLASS_NAME = "MediaSource";
thread_local napi_ref MediaSourceNapi::constructor_ = nullptr;

napi_value MediaSourceNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createMediaSourceWithUrl", JsCreateMediaSourceWithUrl),
    };
    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), 0, Constructor, nullptr, 0, nullptr, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define AVPlayer class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");
    return exports;
}

std::shared_ptr<AVMediaSourceTmp> MediaSourceNapi::GetMediaSource(napi_env env, napi_value jsMediaSource)
{
    MediaSourceNapi *mediaSource = nullptr;
    napi_status status = napi_unwrap(env, jsMediaSource, reinterpret_cast<void **>(&mediaSource));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && mediaSource != nullptr, nullptr, "failed to napi_unwrap");

    return mediaSource->mediaSource_;
}

napi_value MediaSourceNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "failed to napi_get_cb_info");

    MediaSourceNapi *jsMediaSource = new(std::nothrow) MediaSourceNapi();
    CHECK_AND_RETURN_RET_LOG(jsMediaSource != nullptr, result, "failed to new MediaSourceNapi");

    jsMediaSource->env_ = env;
    jsMediaSource->mediaSource_ = std::make_shared<AVMediaSourceTmp>();
    CHECK_AND_RETURN_RET_LOG(jsMediaSource->mediaSource_ != nullptr, result, "failed to Create MediaSource");

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(jsMediaSource),
        MediaSourceNapi::Destructor, nullptr, nullptr);
    if (status != napi_ok) {
        delete jsMediaSource;
        MEDIA_LOGE("Failed to wrap native instance");
        return result;
    }

    MEDIA_LOGI("0x%{public}06" PRIXPTR " Constructor success", FAKE_POINTER(jsMediaSource));
    return jsThis;
}

void MediaSourceNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        MediaSourceNapi *jsMediaSource = reinterpret_cast<MediaSourceNapi *>(nativeObject);
        jsMediaSource->mediaSource_ = nullptr;
        delete jsMediaSource;
    }
    MEDIA_LOGI("Destructor success");
}

napi_value MediaSourceNapi::JsCreateMediaSourceWithUrl(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("JsCreateMediaSourceWithUrl In");
    size_t argCount = 2;
    napi_value args[2] = { nullptr };
    napi_value jsMediaSource = nullptr;
    napi_get_undefined(env, &jsMediaSource);
    napi_status status = napi_get_cb_info(env, info, &argCount, args, nullptr, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "failed to napi_get_cb_info");

    napi_valuetype valueType = napi_undefined;
    if (argCount < 1 || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_string) {
        return nullptr;
    }

    napi_value constructor = nullptr;
    napi_status ret = napi_get_reference_value(env, constructor_, &constructor);
    if (ret != napi_ok || constructor == nullptr) {
        return nullptr;
    }
    napi_new_instance(env, constructor, 0, nullptr, &jsMediaSource);

    std::shared_ptr<AVMediaSourceTmp> mediaSource = GetMediaSource(env, jsMediaSource);
    mediaSource->url = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGE("JsCreateMediaSourceWithUrl get map");
    (void)CommonNapi::GetPropertyMap(env, args[1], mediaSource->header);
    return jsMediaSource;
}
} // Media
} // OHOS