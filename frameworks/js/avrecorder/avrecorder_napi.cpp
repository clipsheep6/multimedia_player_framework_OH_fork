/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "avrecorder_napi.h"
#include <climits>
#include "media_log.h"
#include "media_errors.h"
#include "common_napi.h"
#include "surface_utils.h"
#include "string_ex.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVRecorderNapi"};
}

namespace OHOS {
namespace Media {
thread_local napi_ref AVRecorderNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "AVRecorder";

AVRecorderNapi::AVRecorderNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVRecorderNapi::~AVRecorderNapi()
{
    recorder_ = nullptr;
    recorderCb_ = nullptr;
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value AVRecorderNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAVRecorder", JsCreateAVRecorder),
    };

    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("prepare", JsPrepare),
        DECLARE_NAPI_FUNCTION("getInputSurface", JsGetInputSurface),
        DECLARE_NAPI_FUNCTION("start", JsStart),
        DECLARE_NAPI_FUNCTION("pause", JsPause),
        DECLARE_NAPI_FUNCTION("resume", JsResume),
        DECLARE_NAPI_FUNCTION("stop", JsStop),
        DECLARE_NAPI_FUNCTION("reset", JsReset),
        DECLARE_NAPI_FUNCTION("release", JsRelease),
        DECLARE_NAPI_FUNCTION("on", JsSetEventCallback),

        DECLARE_NAPI_GETTER("state", GetState),
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define AVRecorder class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value AVRecorderNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "failed to napi_get_cb_info");

    AVRecorderNapi *jsRecorder = new(std::nothrow) AVRecorderNapi();
    CHECK_AND_RETURN_RET_LOG(jsRecorder != nullptr, result, "failed to new AVRecorderNapi");

    jsRecorder->env_ = env;
    jsRecorder->recorder_ = RecorderFactory::CreateRecorder();
    CHECK_AND_RETURN_RET_LOG(jsRecorder->recorder_ != nullptr, result, "failed to CreateRecorder");

    jsRecorder->recorderCb_ = std::make_shared<VideoCallbackNapi>(env);
    (void)jsRecorder->recorder_->SetRecorderCallback(jsRecorder->recorderCb_);

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(jsRecorder),
        AVRecorderNapi::Destructor, nullptr, &(jsRecorder->wrapper_));
    if (status != napi_ok) {
        delete jsRecorder;
        MEDIA_LOGE("Failed to wrap native instance");
        return result;
    }

    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void AVRecorderNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        delete reinterpret_cast<AVRecorderNapi *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}
} // namespace Media
} // namespace OHOS