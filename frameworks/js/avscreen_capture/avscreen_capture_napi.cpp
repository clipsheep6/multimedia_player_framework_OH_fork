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

#include "avscreen_capture_napi.h"
#include "avscreen_capture_callback.h"
#include "common_napi.h"
#include "media_dfx.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVScreenCaptureNapi"};
}

namespace OHOS {
namespace Media {
thread_local napi_ref AVScreenCaptureNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "AVScreenCapture";
std::map<std::string, AVScreenCaptureNapi::AvScreenCaptureTaskqFunc> AVScreenCaptureNapi::taskQFuncs_ = {
    {AVScreenCapturegOpt::START_RECORDING, &AVScreenCaptureNapi::StartRecording},
    {AVScreenCapturegOpt::STOP_RECORDING, &AVScreenCaptureNapi::StopRecording},
    {AVScreenCapturegOpt::RELEASE, &AVScreenCaptureNapi::Release},
};

AVScreenCaptureNapi::AVScreenCaptureNapi()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR "Instances create", FAKE_POINTER(this));
}

AVScreenCaptureNapi::~AVScreenCaptureNapi()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR "Instances destroy", FAKE_POINTER(this));
}

napi_value AVScreenCaptureNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAVScreenCaptureRecorder", JsCreateAVScreenRecorder),
        DECLARE_NAPI_STATIC_FUNCTION("reportAVScreenCaptureUserChoice", JsReportAVScreenCaptureUserChoice)
    };

    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("init", JsInit),
        DECLARE_NAPI_FUNCTION("startRecording", JsStartRecording),
        DECLARE_NAPI_FUNCTION("stopRecording", JsStopRecording),
        DECLARE_NAPI_FUNCTION("setMicEnabled", JsSetMicrophoneEnabled),
        DECLARE_NAPI_FUNCTION("release", JsRelease),
        DECLARE_NAPI_FUNCTION("on", JsSetEventCallback),
        DECLARE_NAPI_FUNCTION("off", JsCancelEventCallback),
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
                                           sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define AVScreenCapture class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value AVScreenCaptureNapi::Constructor(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVScreenCaptureNapi::Constructor");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "failed to napi_get_cb_info");

    AVScreenCaptureNapi *jsScreenCapture = new(std::nothrow) AVScreenCaptureNapi();
    CHECK_AND_RETURN_RET_LOG(jsScreenCapture != nullptr, result, "failed to new AVScreenCaptureNapi");

    jsScreenCapture->env_ = env;
    jsScreenCapture->screenCapture_ = ScreenCaptureFactory::CreateScreenCapture();
    CHECK_AND_RETURN_RET_LOG(jsScreenCapture->screenCapture_ != nullptr, result, "failed to CreateScreenCapture");

    jsScreenCapture->taskQue_ = std::make_unique<TaskQueue>("OS_AVScreenCaptureNapi");
    (void)jsScreenCapture->taskQue_->Start();

    jsScreenCapture->screenCaptureCb_ = std::make_shared<AVScreenCaptureCallback>(env);
    CHECK_AND_RETURN_RET_LOG(jsScreenCapture->screenCaptureCb_ != nullptr, result, "failed to CreateScreenCaptureCb");
    (void)jsScreenCapture->screenCapture_->SetScreenCaptureCallback(jsScreenCapture->screenCaptureCb_);

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(jsScreenCapture),
                       AVScreenCaptureNapi::Destructor, nullptr, nullptr);
    if (status != napi_ok) {
        delete jsScreenCapture;
        MEDIA_LOGE("Failed to wrap native instance");
        return result;
    }

    MEDIA_LOGI("Constructor success");
    return jsThis;
}

void AVScreenCaptureNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    MediaTrace trace("AVScreenCaptureNapi::Destructor");
    (void)finalize;
    if (nativeObject != nullptr) {
        AVScreenCaptureNapi *napi = reinterpret_cast<AVScreenCaptureNapi *>(nativeObject);
        if (napi->taskQue_ != nullptr) {
            (void)napi->taskQue_->Stop();
        }

        napi->screenCaptureCb_ = nullptr;

        if (napi->screenCapture_) {
            napi->screenCapture_->Release();
            napi->screenCapture_ = nullptr;
        }

        delete napi;
    }
    MEDIA_LOGI("Destructor success");
}

napi_value AVScreenCaptureNapi::JsCreateAVScreenRecorder(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVScreenCapture::JsCreateAVScreenRecorder");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "failed to napi_get_cb_info");

    std::unique_ptr<AVScreenCaptureAsyncContext> asyncCtx = std::make_unique<AVScreenCaptureAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    asyncCtx->JsResult = std::make_unique<MediaJsResultInstance>(constructor_);
    asyncCtx->ctorFlag = true;

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsCreateAVScreenRecorder", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
        MEDIA_LOGD("JsCreateAVScreenRecorder napi_create_async_work");
    }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    asyncCtx.release();

    MEDIA_LOGI("JsCreateAVScreenRecorder success");
    return result;
}

RetInfo GetReturnInfo(int32_t errCode, const std::string &operate, const std::string &param,
    const std::string &add = "")
{
    MEDIA_LOGE("failed to %{public}s, param %{public}s, errCode = %{public}d",
        operate.c_str(), param.c_str(), errCode);
    MediaServiceExtErrCodeAPI9 err = MSErrorToExtErrorAPI9(static_cast<MediaServiceErrCode>(errCode));
    
    std::string message;
    if (err == MSERR_EXT_API9_INVALID_PARAMETER) {
        message = MSExtErrorAPI9ToString(err, param, "") + add;
    } else {
        message = MSExtErrorAPI9ToString(err, operate, "") + add;
    }

    MEDIA_LOGE("errCode: %{public}d, errMsg: %{public}s", err, message.c_str());
    return RetInfo(err, message);
}

napi_value AVScreenCaptureNapi::JsReportAVScreenCaptureUserChoice(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVScreenCapture::JsReportAVScreenCaptureUserChoice");
    const std::string &opt = AVScreenCapturegOpt::REPORT_USER_CHOICE;
    MEDIA_LOGI("Js %{public}s Start", opt.c_str());

    const int32_t maxParam = 2; // config + callbackRef
    size_t argCount = maxParam;
    napi_value args[maxParam] = { nullptr };

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVScreenCaptureAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");

    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, nullptr, "failed to napi_get_cb_info");
    MEDIA_LOGI("argCountL %{public}zu", argCount);

    if (argCount < maxParam) {
        asyncCtx->AVScreenCaptureSignError(MSERR_MANDATORY_PARAMETER_UNSPECIFIED, "ReportUserChoice", "");
        return result;
    }

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        asyncCtx->AVScreenCaptureSignError(MSERR_INCORRECT_PARAMETER_TYPE, "ReportUserChoice", "sessionId",
            "sessionId is not number");
        return result;
    }
    int32_t sessionId;
    status = napi_get_value_int32(env, args[0], &sessionId);
    if (status != napi_ok) {
        asyncCtx->AVScreenCaptureSignError(MSERR_INCORRECT_PARAMETER_TYPE, "ReportUserChoice", "sessionId",
            "UserChoice get sessionId failed");
        return result;
    }

    valueType = napi_undefined;
    if (napi_typeof(env, args[1], &valueType) != napi_ok || valueType != napi_string) {
        asyncCtx->AVScreenCaptureSignError(MSERR_INCORRECT_PARAMETER_TYPE, "ReportUserChoice", "choice",
            "choice is not string");
        return result;
    }
    std::string choice = CommonNapi::GetStringArgument(env, args[1]);
    MEDIA_LOGI("JsReportAVScreenCaptureUserChoice sessionId: %{public}d, choice: %{public}s",
        sessionId, choice.c_str());

    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    asyncCtx->controller_ = ScreenCaptureControllerFactory::CreateScreenCaptureController();
    asyncCtx->controller_->ReportAVScreenCaptureUserChoice(sessionId, choice);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, opt.c_str(), NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, AsyncJsReportAVScreenCaptureUserChoice,
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    asyncCtx.release();

    MEDIA_LOGI("Js %{public}s End", opt.c_str());
    return result;
}

void AVScreenCaptureNapi::AsyncJsReportAVScreenCaptureUserChoice(napi_env env, void* data)
{
    AVScreenCaptureAsyncContext* asyncCtx = reinterpret_cast<AVScreenCaptureAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "asyncCtx is nullptr!");

    if (asyncCtx->task_) {
        auto result = asyncCtx->task_->GetResult();
        if (result.Value().first != MSERR_EXT_API9_OK) {
            asyncCtx->SignError(result.Value().first, result.Value().second);
        }
    }
    MEDIA_LOGI("The js thread of ReportAVScreenCaptureUserChoice finishes execution and returns");
}

napi_value AVScreenCaptureNapi::JsInit(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVScreenCapture::JsInit");
    const std::string &opt = AVScreenCapturegOpt::INIT;
    MEDIA_LOGI("Js %{public}s Start", opt.c_str());

    const int32_t maxParam = 2; // config + callbackRef
    size_t argCount = maxParam;
    napi_value args[maxParam] = { nullptr };

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVScreenCaptureAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");
    asyncCtx->napi = AVScreenCaptureNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceAndArgs");
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi->taskQue_ != nullptr, result, "taskQue is nullptr!");

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[1]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    if (asyncCtx->napi->GetConfig(asyncCtx, env, args[0]) == MSERR_OK) {
        asyncCtx->task_ = AVScreenCaptureNapi::GetInitTask(asyncCtx);
        (void)asyncCtx->napi->taskQue_->EnqueueTask(asyncCtx->task_);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, opt.c_str(), NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
        AVScreenCaptureAsyncContext* asyncCtx = reinterpret_cast<AVScreenCaptureAsyncContext *>(data);
        CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "asyncCtx is nullptr!");

        if (asyncCtx->task_) {
            auto result = asyncCtx->task_->GetResult();
            if (result.Value().first != MSERR_EXT_API9_OK) {
                asyncCtx->SignError(result.Value().first, result.Value().second);
            }
        }
        MEDIA_LOGI("The js thread of init finishes execution and returns");
    }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    asyncCtx.release();

    MEDIA_LOGI("Js %{public}s End", opt.c_str());
    return result;
}

napi_value AVScreenCaptureNapi::JsStartRecording(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVScreenCapture::JsStartRecording");
    return ExecuteByPromise(env, info,  AVScreenCapturegOpt::START_RECORDING);
}

napi_value AVScreenCaptureNapi::JsStopRecording(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVScreenCapture::JsStopRecording");
    return ExecuteByPromise(env, info,  AVScreenCapturegOpt::STOP_RECORDING);
}

napi_value AVScreenCaptureNapi::JsSetMicrophoneEnabled(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVScreenCapture::JsSetMicrophoneEnabled");
    const std::string &opt = AVScreenCapturegOpt::SET_MIC_ENABLE;
    MEDIA_LOGI("Js %{public}s Start", opt.c_str());

    const int32_t maxParam = 1; // config + callbackRef
    size_t argCount = maxParam;
    napi_value args[maxParam] = { nullptr };

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVScreenCaptureAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");
    asyncCtx->napi = AVScreenCaptureNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceAndArgs");
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi->taskQue_ != nullptr, result, "taskQue is nullptr!");

    napi_valuetype valueType = napi_undefined;
    if (argCount < 1 || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_boolean) {
        asyncCtx->AVScreenCaptureSignError(MSERR_EXT_API9_INVALID_PARAMETER, "SetMivcrophoneEnable",
            "setMicrophone input is not boolean");
        return result;
    }
    bool enable;
    napi_status status = napi_get_value_bool(env, args[0], &enable);
    if (status != napi_ok) {
        asyncCtx->AVScreenCaptureSignError(MSERR_EXT_API9_INVALID_PARAMETER, "SetMivcrophoneEnable",
            "setMicrophone get value failed");
        return result;
    }

    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    asyncCtx->task_ = AVScreenCaptureNapi::GetSetMicrophoneEnableTask(asyncCtx, enable);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, opt.c_str(), NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
        AVScreenCaptureAsyncContext* asyncCtx = reinterpret_cast<AVScreenCaptureAsyncContext *>(data);
        CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "asyncCtx is nullptr!");

        if (asyncCtx->task_) {
            auto result = asyncCtx->task_->GetResult();
            if (result.Value().first != MSERR_EXT_API9_OK) {
                asyncCtx->SignError(result.Value().first, result.Value().second);
            }
        }
        MEDIA_LOGI("The js thread of init finishes execution and returns");
    }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    asyncCtx.release();

    MEDIA_LOGI("Js %{public}s End", opt.c_str());
    return result;
}

napi_value AVScreenCaptureNapi::JsRelease(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVScreenCapture::JsRelease");
    return ExecuteByPromise(env, info,  AVScreenCapturegOpt::RELEASE);
}

napi_value AVScreenCaptureNapi::JsSetEventCallback(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVScreenCapture::JsSetEventCallback");
    MEDIA_LOGI("JsSetEventCallback Start");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 2;
    napi_value args[2] = { nullptr, nullptr };
    AVScreenCaptureNapi *avScreenCaptureNapi = AVScreenCaptureNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(avScreenCaptureNapi != nullptr, result, "Failed to retrieve instance");

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        avScreenCaptureNapi->ErrorCallback(MSERR_INVALID_VAL, "SetEventCallback");
        return result;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    if (callbackName != AVScreenCaptureEvent::EVENT_ERROR &&
        callbackName != AVScreenCaptureEvent::EVENT_STATE_CHANGE) {
        avScreenCaptureNapi->ErrorCallback(MSERR_INVALID_VAL, "SetEventCallback");
        return result;
    }

    napi_ref ref = nullptr;
    napi_status status = napi_create_reference(env, args[1], 1, &ref);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && ref != nullptr, result, "failed to create reference!");

    std::shared_ptr<AutoRef> autoRef = std::make_shared<AutoRef>(env, ref);
    avScreenCaptureNapi->SetCallbackReference(callbackName, autoRef);

    MEDIA_LOGI("JsSetEventCallback set callback %{public}s End", callbackName.c_str());
    return result;
}

napi_value AVScreenCaptureNapi::JsCancelEventCallback(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVScreenCapture::JsCancelEventCallback");
    MEDIA_LOGI("JsCancelEventCallback Start");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 1;
    napi_value args[1] = { nullptr };
    AVScreenCaptureNapi *avScreenCaptureNapi = AVScreenCaptureNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(avScreenCaptureNapi != nullptr, result, "Failed to retrieve instance");

    napi_valuetype valueType0 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string) {
        avScreenCaptureNapi->ErrorCallback(MSERR_INVALID_VAL, "CancelEventCallback");
        return result;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    if (callbackName != AVScreenCaptureEvent::EVENT_ERROR &&
        callbackName != AVScreenCaptureEvent::EVENT_STATE_CHANGE) {
        avScreenCaptureNapi->ErrorCallback(MSERR_INVALID_VAL, "CancelEventCallback");
        return result;
    }

    avScreenCaptureNapi->CancelCallbackReference(callbackName);

    MEDIA_LOGI("JsCancelEventCallback cancel callback %{public}s End", callbackName.c_str());
    return result;
}

std::shared_ptr<TaskHandler<RetInfo>> AVScreenCaptureNapi::GetInitTask(
    const std::unique_ptr<AVScreenCaptureAsyncContext> &asyncCtx)
{
    return std::make_shared<TaskHandler<RetInfo>>([napi = asyncCtx->napi, config = asyncCtx->config_]() {
        const std::string &option = AVScreenCapturegOpt::INIT;
        MEDIA_LOGI("%{public}s Start", option.c_str());

        CHECK_AND_RETURN_RET(napi != nullptr && napi->screenCapture_ != nullptr,
            GetReturnInfo(MSERR_INVALID_OPERATION, option, ""));

        int32_t ret = napi->screenCapture_->Init(config);
        CHECK_AND_RETURN_RET(ret == MSERR_OK, ((void)napi->screenCapture_->Release(), GetReturnInfo(ret, "Init", "")));

        MEDIA_LOGI("%{public}s End", option.c_str());
        return RetInfo(MSERR_EXT_API9_OK, "");
    });
}

std::shared_ptr<TaskHandler<RetInfo>> AVScreenCaptureNapi::GetSetMicrophoneEnableTask(
    const std::unique_ptr<AVScreenCaptureAsyncContext> &asyncCtx, const bool enable)
{
    return std::make_shared<TaskHandler<RetInfo>>([napi = asyncCtx->napi, enable]() {
        const std::string &option = AVScreenCapturegOpt::SET_MIC_ENABLE;
        MEDIA_LOGI("%{public}s Start", option.c_str());

        CHECK_AND_RETURN_RET(napi != nullptr && napi->screenCapture_ != nullptr,
            GetReturnInfo(MSERR_INVALID_OPERATION, option, ""));

        int32_t ret = napi->screenCapture_->SetMicrophoneEnabled(enable);
        CHECK_AND_RETURN_RET(ret == MSERR_OK, ((void)napi->screenCapture_->Release(),
            GetReturnInfo(ret, "SetMicrophoneEnable", "")));

        MEDIA_LOGI("%{public}s End", option.c_str());
        return RetInfo(MSERR_EXT_API9_OK, "");
    });
}

AVScreenCaptureNapi *AVScreenCaptureNapi::GetJsInstanceAndArgs(
    napi_env env, napi_callback_info info, size_t &argCount, napi_value *args)
{
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, nullptr, "failed to napi_get_cb_info");
    MEDIA_LOGI("argCount:%{public}zu", argCount);

    AVScreenCaptureNapi *screenCaptureNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&screenCaptureNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && screenCaptureNapi != nullptr, nullptr,
        "failed to retrieve instance");

    return screenCaptureNapi;
}

RetInfo AVScreenCaptureNapi::StartRecording()
{
    int32_t ret = screenCapture_->SetPrivacyAuthorityEnabled();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, GetReturnInfo(ret, "SetPrivacyAuthorityEnabled", ""));
    ret = screenCapture_->StartScreenRecording();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, GetReturnInfo(ret, "StartRecording", ""));
    return RetInfo(MSERR_EXT_API9_OK, "");
}

RetInfo AVScreenCaptureNapi::StopRecording()
{
    int32_t ret = screenCapture_->StopScreenRecording();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, GetReturnInfo(ret, "StopRecording", ""));
    return RetInfo(MSERR_EXT_API9_OK, "");
}

RetInfo AVScreenCaptureNapi::Release()
{
    int32_t ret = screenCapture_->Release();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, GetReturnInfo(ret, "Release", ""));
    return RetInfo(MSERR_EXT_API9_OK, "");
}

int32_t AVScreenCaptureNapi::GetConfig(std::unique_ptr<AVScreenCaptureAsyncContext> &asyncCtx,
    napi_env env, napi_value args)
{
    napi_valuetype valueType = napi_undefined;
    if (args == nullptr || napi_typeof(env, args, &valueType) != napi_ok || valueType != napi_object) {
        asyncCtx->AVScreenCaptureSignError(MSERR_INCORRECT_PARAMETER_TYPE, "GetConfig", "AVScreenCaptureRecordConfig",
            "config type should be AVScreenCaptureRecordConfig.");
        return MSERR_INCORRECT_PARAMETER_TYPE;
    }

    asyncCtx->config_.captureMode = CaptureMode::CAPTURE_HOME_SCREEN;
    asyncCtx->config_.dataType = DataType::CAPTURE_FILE;

    int32_t ret =  GetAudioInfo(asyncCtx, env, args);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "failed to GetAudioInfo");
    ret =  GetVideoInfo(asyncCtx, env, args);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "failed to GetVideoInfo");
    ret =  GetRecorderInfo(asyncCtx, env, args);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "failed to GetRecorderInfo");
    return MSERR_OK;
}

int32_t AVScreenCaptureNapi::GetAudioInfo(std::unique_ptr<AVScreenCaptureAsyncContext> &asyncCtx,
    napi_env env, napi_value args)
{
    int32_t audioSampleRate = AVSCREENCAPTURE_DEFAULT_AUDIO_SAMPLE_RATE;
    int32_t audioChannels = AVSCREENCAPTURE_DEFAULT_AUDIO_CHANNELS;
    int32_t audioBitrate = AVSCREENCAPTURE_DEFAULT_AUDIO_BIT_RATE;

    AudioCaptureInfo &micConfig = asyncCtx->config_.audioInfo.micCapInfo;
    AudioCaptureInfo &innerConfig = asyncCtx->config_.audioInfo.innerCapInfo;
    AudioEncInfo &encoderConfig = asyncCtx->config_.audioInfo.audioEncInfo;

    int32_t ret = AVScreenCaptureNapi::GetPropertyInt32(env, args, "audioSampleRate", audioSampleRate);
    CHECK_AND_RETURN_RET(ret == MSERR_OK,
        (asyncCtx->AVScreenCaptureSignError(ret, "getAudioSampleRate", "audioSampleRate"), ret));
    micConfig.audioSampleRate = audioSampleRate;
    innerConfig.audioSampleRate = audioSampleRate;
    MEDIA_LOGI("input audioSampleRate %{public}d", micConfig.audioSampleRate);

    ret = AVScreenCaptureNapi::GetPropertyInt32(env, args, "audioChannelCount", audioChannels);
    CHECK_AND_RETURN_RET(ret == MSERR_OK,
        (asyncCtx->AVScreenCaptureSignError(ret, "getAudioChannelCount", "audioChannelCount"), ret));
    micConfig.audioChannels = audioChannels;
    innerConfig.audioChannels = audioChannels;
    MEDIA_LOGI("input audioChannelCount %{public}d", micConfig.audioChannels);
    micConfig.audioSource = AudioCaptureSourceType::MIC;
    innerConfig.audioSource = AudioCaptureSourceType::ALL_PLAYBACK;

    ret = AVScreenCaptureNapi::GetPropertyInt32(env, args, "audioBitrate", audioBitrate);
    CHECK_AND_RETURN_RET(ret == MSERR_OK,
        (asyncCtx->AVScreenCaptureSignError(ret, "getAudioBitrate", "audioBitrate"), ret));
    encoderConfig.audioBitrate = audioBitrate;
    encoderConfig.audioCodecformat = AudioCodecFormat::AAC_LC;
    MEDIA_LOGI("input audioBitrate %{public}d", encoderConfig.audioBitrate);
    return MSERR_OK;
}

int32_t AVScreenCaptureNapi::GetVideoInfo(std::unique_ptr<AVScreenCaptureAsyncContext> &asyncCtx,
    napi_env env, napi_value args)
{
    int32_t videoBitrate = AVSCREENCAPTURE_DEFAULT_VIDEO_BIT_RATE;
    int32_t preset = VideoCodecFormat::H264;
    int32_t frameWidth = AVSCREENCAPTURE_DEFAULT_FRAME_WIDTH;
    int32_t frameHeight = AVSCREENCAPTURE_DEFAULT_FRAME_HEIGHT;

    VideoEncInfo &encoderConfig = asyncCtx->config_.videoInfo.videoEncInfo;
    VideoCaptureInfo &videoConfig = asyncCtx->config_.videoInfo.videoCapInfo;

    int32_t ret = AVScreenCaptureNapi::GetPropertyInt32(env, args, "videoBitrate", videoBitrate);
    CHECK_AND_RETURN_RET(ret == MSERR_OK,
        (asyncCtx->AVScreenCaptureSignError(ret, "getVideoBitrate", "videoBitrate"), ret));
    encoderConfig.videoBitrate = videoBitrate;
    encoderConfig.videoFrameRate = AVSCREENCAPTURE_DEFAULT_VIDEO_FRAME_RATE;
    MEDIA_LOGI("input videoBitrate %{public}d", encoderConfig.videoBitrate);

    ret = AVScreenCaptureNapi::GetPropertyInt32(env, args, "preset", preset);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, (asyncCtx->AVScreenCaptureSignError(ret, "getPreset", "preset"), ret));
    encoderConfig.videoCodec = AVScreenCaptureNapi::GetVideoCodecFormat(preset);
    MEDIA_LOGI("input videoCodec %{public}d", encoderConfig.videoCodec);

    ret = AVScreenCaptureNapi::GetPropertyInt32(env, args, "frameWidth", frameWidth);
    CHECK_AND_RETURN_RET(ret == MSERR_OK,
        (asyncCtx->AVScreenCaptureSignError(ret, "getFrameWidth", "frameWidth"), ret));
    videoConfig.videoFrameWidth = frameWidth;
    ret = AVScreenCaptureNapi::GetPropertyInt32(env, args, "frameHeight", frameHeight);
    CHECK_AND_RETURN_RET(ret == MSERR_OK,
        (asyncCtx->AVScreenCaptureSignError(ret, "getFrameHeight", "frameHeight"), ret));
    videoConfig.videoFrameHeight = frameHeight;
    MEDIA_LOGI("input frameWidth %{public}d, frameHeight %{public}d",
        videoConfig.videoFrameWidth, videoConfig.videoFrameHeight);
    videoConfig.videoSource = VideoSourceType::VIDEO_SOURCE_SURFACE_RGBA;
    return MSERR_OK;
}

int32_t AVScreenCaptureNapi::GetRecorderInfo(std::unique_ptr<AVScreenCaptureAsyncContext> &asyncCtx,
    napi_env env, napi_value args)
{
    RecorderInfo &recorderConfig = asyncCtx->config_.recorderInfo;
    recorderConfig.fileFormat = AVSCREENCAPTURE_DEFAULT_FILE_FORMAT;
    int32_t fd = -1;
    (void)CommonNapi::GetPropertyInt32(env, args, "fd", fd);
    recorderConfig.url = "fd://" + std::to_string(fd);
    CHECK_AND_RETURN_RET(recorderConfig.url != "",
        (asyncCtx->AVScreenCaptureSignError(MSERR_INVALID_VAL, "GetRecorderInfo", "url"), MSERR_INVALID_VAL));
    MEDIA_LOGI("input url %{public}s", recorderConfig.url.c_str());
    return MSERR_OK;
}

VideoCodecFormat AVScreenCaptureNapi::GetVideoCodecFormat(const int32_t &preset)
{
    const std::map<AVScreenCaptureRecorderPreset, VideoCodecFormat> presetToCodecFormat = {
        { AVScreenCaptureRecorderPreset::SCREEN_RECORD_PRESET_H264_AAC_MP4, VideoCodecFormat::H264 },
        { AVScreenCaptureRecorderPreset::SCREEN_RECORD_PRESET_H265_AAC_MP4, VideoCodecFormat::H265 }
    };
    VideoCodecFormat codecFormat = VideoCodecFormat::H264;
    auto iter = presetToCodecFormat.find(static_cast<AVScreenCaptureRecorderPreset>(preset));
    if (iter != presetToCodecFormat.end()) {
        codecFormat = iter->second;
    }
    return codecFormat;
}

void AVScreenCaptureNapi::ErrorCallback(int32_t errCode, const std::string &operate, const std::string &add)
{
    MEDIA_LOGE("failed to %{public}s, errCode = %{public}d", operate.c_str(), errCode);
    CHECK_AND_RETURN_LOG(screenCaptureCb_ != nullptr, "screenCaptureCb_ is nullptr!");
    auto napiCb = std::static_pointer_cast<AVScreenCaptureCallback>(screenCaptureCb_);

    MediaServiceExtErrCodeAPI9 err = MSErrorToExtErrorAPI9(static_cast<MediaServiceErrCode>(errCode));
    std::string msg = MSExtErrorAPI9ToString(err, operate, "") + add;
    napiCb->SendErrorCallback(errCode, msg);
}

void AVScreenCaptureNapi::StateCallback(const AVScreenCaptureStateCode &stateCode)
{
    MEDIA_LOGI("Change state to %{public}d", stateCode);
    CHECK_AND_RETURN_LOG(screenCaptureCb_ != nullptr, "screenCaptureCb_ is nullptr!");
    auto napiCb = std::static_pointer_cast<AVScreenCaptureCallback>(screenCaptureCb_);
    napiCb->SendStateCallback(stateCode);
}

void AVScreenCaptureNapi::SetCallbackReference(const std::string &callbackName, std::shared_ptr<AutoRef> ref)
{
    eventCbMap_[callbackName] = ref;
    CHECK_AND_RETURN_LOG(screenCaptureCb_ != nullptr, "screenCaptureCb_ is nullptr!");
    auto napiCb = std::static_pointer_cast<AVScreenCaptureCallback>(screenCaptureCb_);
    napiCb->SaveCallbackReference(callbackName, ref);
}

void AVScreenCaptureNapi::CancelCallbackReference(const std::string &callbackName)
{
    CHECK_AND_RETURN_LOG(screenCaptureCb_ != nullptr, "screenCaptureCb_ is nullptr!");
    auto napiCb = std::static_pointer_cast<AVScreenCaptureCallback>(screenCaptureCb_);
    napiCb->CancelCallbackReference(callbackName);
    eventCbMap_[callbackName] = nullptr;
}

void AVScreenCaptureNapi::CancelCallback()
{
    CHECK_AND_RETURN_LOG(screenCaptureCb_ != nullptr, "screenCaptureCb_ is nullptr!");
    auto napiCb = std::static_pointer_cast<AVScreenCaptureCallback>(screenCaptureCb_);
    napiCb->ClearCallbackReference();
}

std::shared_ptr<TaskHandler<RetInfo>> AVScreenCaptureNapi::GetPromiseTask(
    AVScreenCaptureNapi *avnapi, const std::string &opt)
{
    return std::make_shared<TaskHandler<RetInfo>>([napi = avnapi, option = opt]() {
        MEDIA_LOGI("%{public}s Start", option.c_str());
        CHECK_AND_RETURN_RET(napi != nullptr && napi->screenCapture_ != nullptr,
            GetReturnInfo(MSERR_INVALID_OPERATION, option, ""));

        RetInfo ret(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "");
        auto itFunc = taskQFuncs_.find(option);
        CHECK_AND_RETURN_RET_LOG(itFunc != taskQFuncs_.end(), ret, "%{public}s not found in map!", option.c_str());
        auto memberFunc = itFunc->second;
        CHECK_AND_RETURN_RET_LOG(memberFunc != nullptr, ret, "memberFunc is nullptr!");
        ret = (napi->*memberFunc)();
        
        MEDIA_LOGI("%{public}s End", option.c_str());
        return ret;
    });
}

napi_value AVScreenCaptureNapi::ExecuteByPromise(napi_env env, napi_callback_info info, const std::string &opt)
{
    MEDIA_LOGI("Js %{public}s Start", opt.c_str());
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    size_t argCount = 1; // Only callbackRef parameter
    napi_value args[1] = { nullptr }; // Only callbackRef parameter

    auto asyncCtx = std::make_unique<AVScreenCaptureAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");
    asyncCtx->napi = AVScreenCaptureNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceAndArgs");
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi->taskQue_ != nullptr, result, "taskQue is nullptr!");

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    asyncCtx->task_ = AVScreenCaptureNapi::GetPromiseTask(asyncCtx->napi, opt);
    (void)asyncCtx->napi->taskQue_->EnqueueTask(asyncCtx->task_);
    asyncCtx->opt_ = opt;

    napi_value resource = nullptr;
    napi_create_string_utf8(env, opt.c_str(), NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
        AVScreenCaptureAsyncContext* asyncCtx = reinterpret_cast<AVScreenCaptureAsyncContext *>(data);
        CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "asyncCtx is nullptr!");

        if (asyncCtx->task_) {
            auto result = asyncCtx->task_->GetResult();
            if (result.Value().first != MSERR_EXT_API9_OK) {
                asyncCtx->SignError(result.Value().first, result.Value().second);
            }

            if (result.Value().first == MSERR_EXT_API9_OK) {
                asyncCtx->JsResult = std::make_unique<MediaJsResultString>(result.Value().second);
            }
        }
        MEDIA_LOGI("The js thread of %{public}s finishes execution and returns", asyncCtx->opt_.c_str());
    }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    asyncCtx.release();

    MEDIA_LOGI("Js %{public}s End", opt.c_str());
    return result;
}

int32_t AVScreenCaptureNapi::GetPropertyInt32(napi_env env, napi_value configObj, const std::string &type,
    int32_t &result)
{
    napi_value item = nullptr;
    bool exist = false;
    napi_status status = napi_has_named_property(env, configObj, type.c_str(), &exist);
    if (status != napi_ok || !exist) {
        MEDIA_LOGI("can not find %{public}s property", type.c_str());
        return MSERR_OK;
    }

    if (napi_get_named_property(env, configObj, type.c_str(), &item) != napi_ok) {
        MEDIA_LOGI("get %{public}s property fail", type.c_str());
        return MSERR_UNKNOWN;
    }

    if (napi_get_value_int32(env, item, &result) != napi_ok) {
        std::string string = CommonNapi::GetStringArgument(env, item);
        if (string == "") {
            // This attribute has not been assigned
            return MSERR_OK;
        } else {
            MEDIA_LOGE("get %{public}s property value fail", type.c_str());
            return MSERR_INVALID_VAL;
        }
    }

    MEDIA_LOGI("get %{public}s : %{public}d!", type.c_str(), result);
    return MSERR_OK;
}

void AVScreenCaptureAsyncContext::AVScreenCaptureSignError(int32_t errCode, const std::string &operate,
    const std::string &param, const std::string &add)
{
    RetInfo retInfo = GetReturnInfo(errCode, operate, param, add);
    SignError(retInfo.first, retInfo.second);
}

} // namespace Media
} // namespace OHOS