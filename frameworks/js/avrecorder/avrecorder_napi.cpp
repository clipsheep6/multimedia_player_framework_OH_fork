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
#include "avrecorder_callback.h"
#include "recorder_napi_utils.h"
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
    CancelCallback();
    recorder_ = nullptr;
    recorderCb_ = nullptr;

    if (taskQue_ != nullptr) {
        (void)taskQue_->Stop();
    }

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

    jsRecorder->taskQue_ = std::make_unique<TaskQueue>("AVRecorderNapi");
    (void)jsRecorder->taskQue_->Start();

    jsRecorder->recorderCb_ = std::make_shared<AVRecorderCallback>(env);
    CHECK_AND_RETURN_RET_LOG(jsRecorder->recorderCb_ != nullptr, result, "failed to CreateRecorderCb");
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
        AVRecorderNapi *napi = reinterpret_cast<AVRecorderNapi *>(nativeObject);
        napi->RemoveSurface();
        delete napi;
    }
    MEDIA_LOGD("Destructor success");
}

napi_value AVRecorderNapi::JsCreateAVRecorder(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "failed to napi_get_cb_info");

    std::unique_ptr<AVRecorderAsyncContext> asyncCtx = std::make_unique<AVRecorderAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    asyncCtx->JsResult = std::make_unique<MediaJsResultInstance>(constructor_);
    asyncCtx->ctorFlag = true;

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsCreateAVRecorder", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
              MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    MEDIA_LOGD("JsCreateAVRecorder success");
    return result;
}

napi_value AVRecorderNapi::JsPrepare(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("JsPrepare In");

    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    napi_value jsThis = nullptr;
    napi_value args[2] = { nullptr };

    auto asyncCtx = std::make_unique<AVRecorderAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");

    // get args
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, result, "failed to napi_get_cb_info");

    // get recordernapi
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncCtx->napi != nullptr, result, "failed to napi_unwrap");

    // set param
    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_object) {
        asyncCtx->AVRecorderSignError(MSERR_EXT_API9_INVALID_PARAMETER, "", "");
    } else {
        (void)asyncCtx->napi->GetConfig(env, args[0]);
        (void)asyncCtx->napi->GetProfile(env, args[0]);
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[1]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Prepare", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
        AVRecorderAsyncContext * threadCtx = reinterpret_cast<AVRecorderAsyncContext *>(data);
        CHECK_AND_RETURN_LOG(threadCtx != nullptr, "threadCtx is nullptr!");
        if (threadCtx->napi == nullptr || threadCtx->napi->recorder_ == nullptr) {
            threadCtx->AVRecorderSignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "", "");
            return;
        }

        int32_t ret = threadCtx->napi->SetProfile();
        CHECK_AND_RETURN_SIGNERROR(ret == MSERR_OK, threadCtx, ret, "prepare");

        ret = threadCtx->napi->SetConfiguration();
        CHECK_AND_RETURN_SIGNERROR(ret == MSERR_OK, threadCtx, ret, "prepare");

        ret = threadCtx->napi->recorder_->Prepare();
        CHECK_AND_RETURN_SIGNERROR(ret == MSERR_OK, threadCtx, ret, "prepare");

        threadCtx->napi->StateCallback(AVRecorderState::STATE_PREPARED);
        MEDIA_LOGD("JsPrepare success");
    }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();
    MEDIA_LOGD("JsPrepare End");
    return result;
}

napi_value AVRecorderNapi::JsGetInputSurface(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("JsGetInputSurface In");

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVRecorderAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, result, "failed to napi_get_cb_info");

    // get recordernapi
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncCtx->napi != nullptr, result, "failed to napi_unwrap");

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "GetInputSurface", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
        AVRecorderAsyncContext * threadCtx = reinterpret_cast<AVRecorderAsyncContext *>(data);
        CHECK_AND_RETURN_LOG(threadCtx != nullptr, "threadCtx is nullptr!");
        if (threadCtx->napi == nullptr || threadCtx->napi->recorder_ == nullptr) {
            threadCtx->AVRecorderSignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "", "");
            return;
        }

        AVRecorderNapi *napi = threadCtx->napi;
        napi->surface_ = napi->recorder_->GetSurface(napi->videoSourceID);
        if (napi->surface_ == nullptr) {
            threadCtx->AVRecorderSignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "GetInputSurface", "");
            return;
        }

        SurfaceError error = SurfaceUtils::GetInstance()->Add(napi->surface_->GetUniqueId(), napi->surface_);
        if (error != SURFACE_ERROR_OK) {
            threadCtx->AVRecorderSignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "GetInputSurface", "");
            return;
        }

        auto surfaceId = std::to_string(napi->surface_->GetUniqueId());
        threadCtx->JsResult = std::make_unique<MediaJsResultString>(surfaceId);
    }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();
    MEDIA_LOGD("JsGetInputSurface End");
    return result;
}

AVRecorderNapi* AVRecorderNapi::GetAVRecorderNapi(napi_env env, napi_callback_info info)
{
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, nullptr, "failed to napi_get_cb_info");

    AVRecorderNapi *recorderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorderNapi != nullptr, nullptr, "failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(recorderNapi->recorder_ != nullptr, nullptr, "No recorder");
    CHECK_AND_RETURN_RET_LOG(recorderNapi->taskQue_ != nullptr, nullptr, "No TaskQue");

    return recorderNapi;
}

napi_value AVRecorderNapi::JsStart(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("JsStart In");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    AVRecorderNapi *recorderNapi = GetAVRecorderNapi(env, info);
    CHECK_AND_RETURN_RET_LOG(recorderNapi != nullptr, result, "failed to get AVRecorderNapi");

    auto task = std::make_shared<TaskHandler<void>>([napi = recorderNapi]() {
        int32_t ret = napi->recorder_->Start();
        if (ret == MSERR_OK) {
            napi->StateCallback(AVRecorderState::STATE_STARTED);
            MEDIA_LOGD("Start success");
        } else {
            napi->ErrorCallback(ret, "Start");
        }
    });
    (void)recorderNapi->taskQue_->EnqueueTask(task);

    MEDIA_LOGD("JsStart End");
    return result;
}

napi_value AVRecorderNapi::JsPause(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("JsPause In");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    AVRecorderNapi *recorderNapi = GetAVRecorderNapi(env, info);
    CHECK_AND_RETURN_RET_LOG(recorderNapi != nullptr, result, "failed to get AVRecorderNapi");

    auto task = std::make_shared<TaskHandler<void>>([napi = recorderNapi]() {
        int32_t ret = napi->recorder_->Pause();
        if (ret == MSERR_OK) {
            napi->StateCallback(AVRecorderState::STATE_PAUSED);
            MEDIA_LOGD("Pause success");
        } else {
            napi->ErrorCallback(ret, "Pause");
        }
    });
    (void)recorderNapi->taskQue_->EnqueueTask(task);

    MEDIA_LOGD("JsPause End");
    return result;
}

napi_value AVRecorderNapi::JsResume(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("JsResume In");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    AVRecorderNapi *recorderNapi = GetAVRecorderNapi(env, info);
    CHECK_AND_RETURN_RET_LOG(recorderNapi != nullptr, result, "failed to get AVRecorderNapi");

    auto task = std::make_shared<TaskHandler<void>>([napi = recorderNapi]() {
        int32_t ret = napi->recorder_->Resume();
        if (ret == MSERR_OK) {
            napi->StateCallback(AVRecorderState::STATE_STARTED);
            MEDIA_LOGD("Resume success");
        } else {
            napi->ErrorCallback(ret, "Resume");
        }
    });
    (void)recorderNapi->taskQue_->EnqueueTask(task);

    MEDIA_LOGD("JsResume End");
    return result;
}

napi_value AVRecorderNapi::JsStop(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("JsStop In");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    AVRecorderNapi *recorderNapi = GetAVRecorderNapi(env, info);
    CHECK_AND_RETURN_RET_LOG(recorderNapi != nullptr, result, "failed to get AVRecorderNapi");

    auto task = std::make_shared<TaskHandler<void>>([napi = recorderNapi]() {
        int32_t ret = napi->recorder_->Stop(false);
        if (ret == MSERR_OK) {
            napi->StateCallback(AVRecorderState::STATE_STOPPED);
            MEDIA_LOGD("Stop success");
        } else {
            napi->ErrorCallback(ret, "Stop");
        }
    });
    (void)recorderNapi->taskQue_->EnqueueTask(task);

    MEDIA_LOGD("JsStop End");
    return result;
}

napi_value AVRecorderNapi::JsReset(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("JsReset In");

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVRecorderAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, result, "failed to napi_get_cb_info");

    // get recordernapi
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncCtx->napi != nullptr, result, "failed to napi_unwrap");

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Reset", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
        AVRecorderAsyncContext * threadCtx = reinterpret_cast<AVRecorderAsyncContext *>(data);
        CHECK_AND_RETURN_LOG(threadCtx != nullptr, "threadCtx is nullptr!");
        if (threadCtx->napi == nullptr || threadCtx->napi->recorder_ == nullptr) {
            threadCtx->AVRecorderSignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "", "");
            return;
        }

        threadCtx->napi->RemoveSurface();
        int32_t ret = threadCtx->napi->recorder_->Reset();
        CHECK_AND_RETURN_SIGNERROR(ret == MSERR_OK, threadCtx, ret, "Reset");

        threadCtx->napi->StateCallback(AVRecorderState::STATE_IDLE);
        MEDIA_LOGD("Reset success");
    }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();
    MEDIA_LOGD("JsReset End");
    return result;
}

napi_value AVRecorderNapi::JsRelease(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("JsRelease In");

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVRecorderAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, result, "failed to napi_get_cb_info");

    // get recordernapi
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncCtx->napi != nullptr, result, "failed to napi_unwrap");

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Release", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
        AVRecorderAsyncContext * threadCtx = reinterpret_cast<AVRecorderAsyncContext *>(data);
        CHECK_AND_RETURN_LOG(threadCtx != nullptr, "threadCtx is nullptr!");
        if (threadCtx->napi == nullptr || threadCtx->napi->recorder_ == nullptr) {
            threadCtx->AVRecorderSignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "", "");
            return;
        }

        threadCtx->napi->RemoveSurface();
        int32_t ret = threadCtx->napi->recorder_->Release();
        CHECK_AND_RETURN_SIGNERROR(ret == MSERR_OK, threadCtx, ret, "Release");

        threadCtx->napi->StateCallback(AVRecorderState::STATE_RELEASED);
        threadCtx->napi->CancelCallback();
        MEDIA_LOGD("Release success");
    }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();
    MEDIA_LOGD("JsRelease End");
    return result;
}

napi_value AVRecorderNapi::JsSetEventCallback(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("JsSetEventCallback In");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    static constexpr size_t minArgCount = 2;
    size_t argCount = minArgCount;
    napi_value args[minArgCount] = { nullptr, nullptr };
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr || args[1] == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return result;
    }

    AVRecorderNapi *recorderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorderNapi != nullptr, result, "Failed to retrieve instance");

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        recorderNapi->ErrorCallback(MSERR_EXT_API9_INVALID_PARAMETER, "SetEventCallback");
        return result;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGD("callbackName: %{public}s", callbackName.c_str());

    napi_ref ref = nullptr;
    status = napi_create_reference(env, args[1], 1, &ref);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && ref != nullptr, result, "failed to create reference!");

    std::shared_ptr<AutoRef> autoRef = std::make_shared<AutoRef>(env, ref);
    recorderNapi->SetCallbackReference(callbackName, autoRef);

    MEDIA_LOGD("JsSetEventCallback End");
    return result;
}

napi_value AVRecorderNapi::GetState(napi_env env, napi_callback_info info)
{
    napi_value jsThis = nullptr;
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "Failed to retrieve details about the callback");

    AVRecorderNapi *recorderNapi = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&recorderNapi);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorderNapi != nullptr, result, "Failed to retrieve instance");

    auto napiCb = std::static_pointer_cast<AVRecorderCallback>(recorderNapi->recorderCb_);
    CHECK_AND_RETURN_RET_LOG(napiCb != nullptr, result, "napiCb is nullptr!");
    std::string curState = napiCb->GetState();
    MEDIA_LOGD("GetState success, State: %{public}s", curState.c_str());

    napi_value jsResult = nullptr;
    status = napi_create_string_utf8(env, curState.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "napi_create_string_utf8 error");
    return jsResult;
}

int32_t AVRecorderNapi::GetConfig(napi_env env, napi_value args)
{
    int32_t audioSource = AUDIO_SOURCE_INVALID;
    int32_t videoSource = VIDEO_SOURCE_BUTT;

    bool ret = CommonNapi::GetPropertyInt32(env, args, "audioSourceType", audioSource);
    if (ret) {
        config.audioSourceType = static_cast<AudioSourceType>(audioSource);
        withAudio_ = true;
        MEDIA_LOGI("audioSource Type %d!", audioSource);
    }
    
    ret = CommonNapi::GetPropertyInt32(env, args, "videoSourceType", videoSource);
    if (ret) {
        config.videoSourceType = static_cast<VideoSourceType>(videoSource);
        withVideo_ = true;
        MEDIA_LOGI("videoSource Type %d!", videoSource);
    }

    config.url = CommonNapi::GetPropertyString(env, args, "url");
    (void)CommonNapi::GetPropertyInt32(env, args, "rotation", config.rotation);

    napi_value geoLocation = nullptr;
    napi_get_named_property(env, args, "location", &geoLocation);
    double tempLatitude = 0;
    double tempLongitude = 0;
    (void)CommonNapi::GetPropertyDouble(env, geoLocation, "latitude", tempLatitude);
    (void)CommonNapi::GetPropertyDouble(env, geoLocation, "longitude", tempLongitude);
    config.location.latitude = static_cast<float>(tempLatitude);
    config.location.longitude = static_cast<float>(tempLongitude);

    return MSERR_OK;
}

int32_t AVRecorderNapi::GetProfile(napi_env env, napi_value args)
{
    napi_value item = nullptr;
    napi_get_named_property(env, args, "profile", &item);

    AVRecorderProfile &profile = config.profile;

    if (withAudio_) {
        (void)CommonNapi::GetPropertyInt32(env, item, "audioBitrate", profile.audioBitrate);
        (void)CommonNapi::GetPropertyInt32(env, item, "audioChannels", profile.audioChannels);
        std::string audioCodec = CommonNapi::GetPropertyString(env, item, "audioCodec");
        (void)MapMimeToAudioCodecFormat(audioCodec, profile.audioCodecFormat);
        (void)CommonNapi::GetPropertyInt32(env, item, "audioSampleRate", profile.auidoSampleRate);
        MEDIA_LOGI("audioBitrate %d, audioChannels %d, audioCodecFormat %d, audioSampleRate %d!",
                   profile.audioBitrate, profile.audioChannels, profile.audioCodecFormat, profile.auidoSampleRate);
    }

    if (withVideo_) {
        (void)CommonNapi::GetPropertyInt32(env, item, "videoBitrate", profile.videoBitrate);
        std::string videoCodec = CommonNapi::GetPropertyString(env, item, "videoCodec");
        (void)MapMimeToVideoCodecFormat(videoCodec, profile.videoCodecFormat);
        (void)CommonNapi::GetPropertyInt32(env, item, "videoFrameWidth", profile.videoFrameWidth);
        (void)CommonNapi::GetPropertyInt32(env, item, "videoFrameHeight", profile.videoFrameHeight);
        (void)CommonNapi::GetPropertyInt32(env, item, "videoFrameRate", profile.videoFrameRate);
        MEDIA_LOGI("videoBitrate %d, videoCodecFormat %d, videoFrameWidth %d, videoFrameHeight %d, videoFrameRate %d",
                   profile.videoBitrate, profile.videoCodecFormat, profile.videoFrameWidth,
                   profile.videoFrameHeight, profile.videoFrameRate);
    }

    std::string outputFile = CommonNapi::GetPropertyString(env, item, "fileFormat");
    (void)MapExtensionNameToOutputFormat(outputFile, profile.fileFormat);
    MEDIA_LOGI("fileFormat %d", profile.fileFormat);

    return MSERR_OK;
}

int32_t AVRecorderNapi::SetProfile()
{
    int32_t ret;
    AVRecorderProfile &profile = config.profile;
    CHECK_AND_RETURN_RET(recorder_ != nullptr, MSERR_INVALID_OPERATION);

    if (withAudio_) {
        ret = recorder_->SetAudioSource(config.audioSourceType, audioSourceID);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Fail to set AudioSource");
    }
    
    if (withVideo_) {
        ret = recorder_->SetVideoSource(config.videoSourceType, videoSourceID);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Fail to set VideoSource");
    }

    ret = recorder_->SetOutputFormat(profile.fileFormat);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Fail to set OutputFormat");

    if (withAudio_) {
        ret = recorder_->SetAudioEncoder(audioSourceID, profile.audioCodecFormat);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Fail to set audioCodec");

        ret = recorder_->SetAudioSampleRate(audioSourceID, profile.auidoSampleRate);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Fail to set auidoSampleRate");

        ret = recorder_->SetAudioChannels(audioSourceID, profile.audioChannels);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Fail to set audioChannels");

        ret = recorder_->SetAudioEncodingBitRate(audioSourceID, profile.audioBitrate);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Fail to set audioBitrate");
    }
    
    if (withVideo_) {
        ret = recorder_->SetVideoEncoder(videoSourceID, profile.videoCodecFormat);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Fail to set videoCodec");

        ret = recorder_->SetVideoSize(videoSourceID, profile.videoFrameWidth, profile.videoFrameHeight);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Fail to set videoSize");

        ret = recorder_->SetVideoFrameRate(videoSourceID, profile.videoFrameRate);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Fail to set videoFrameRate");

        ret = recorder_->SetVideoEncodingBitRate(videoSourceID, profile.videoBitrate);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Fail to set videoBitrate");
    }

    return MSERR_OK;
}

int32_t AVRecorderNapi::SetConfiguration()
{
    CHECK_AND_RETURN_RET(recorder_ != nullptr, MSERR_INVALID_OPERATION);

    recorder_->SetLocation(config.location.latitude, config.location.longitude);
    recorder_->SetOrientationHint(config.rotation);

    const std::string fdHead = "fd://";
    if (config.url.find(fdHead) != std::string::npos) {
        int32_t fd = -1;
        std::string inputFd = config.url.substr(fdHead.size());
        CHECK_AND_RETURN_RET(StrToInt(inputFd, fd) == true, MSERR_INVALID_VAL);
        CHECK_AND_RETURN_RET(fd >= 0, MSERR_INVALID_VAL);
        int32_t ret = recorder_->SetOutputFile(fd);
        CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
    } else {
        MEDIA_LOGE("invalid input uri, not a fd!");
        return MSERR_INVALID_VAL;
    }

    return MSERR_OK;
}

void AVRecorderNapi::ErrorCallback(int32_t errCode, const std::string& param1)
{
    CHECK_AND_RETURN_LOG(recorderCb_ != nullptr, "recorderCb_ is nullptr!");
    auto napiCb = std::static_pointer_cast<AVRecorderCallback>(recorderCb_);
    napiCb->SendErrorCallback(errCode, param1, "");
}

void AVRecorderNapi::StateCallback(const std::string &state)
{
    CHECK_AND_RETURN_LOG(recorderCb_ != nullptr, "recorderCb_ is nullptr!");
    auto napiCb = std::static_pointer_cast<AVRecorderCallback>(recorderCb_);
    napiCb->SendStateCallback(state, StateChangeReason::USER);
}

void AVRecorderNapi::SetCallbackReference(const std::string &callbackName, std::shared_ptr<AutoRef> ref)
{
    eventCbMap_[callbackName] = ref;
    CHECK_AND_RETURN_LOG(recorderCb_ != nullptr, "recorderCb_ is nullptr!");
    auto napiCb = std::static_pointer_cast<AVRecorderCallback>(recorderCb_);
    napiCb->SaveCallbackReference(callbackName, ref);
}

void AVRecorderNapi::CancelCallback()
{
    CHECK_AND_RETURN_LOG(recorderCb_ != nullptr, "recorderCb_ is nullptr!");
    auto napiCb = std::static_pointer_cast<AVRecorderCallback>(recorderCb_);
    napiCb->ClearCallbackReference();
}

void AVRecorderNapi::RemoveSurface()
{
    if (surface_ != nullptr) {
        auto id = surface_->GetUniqueId();
        auto surface = SurfaceUtils::GetInstance()->GetSurface(id);
        if (surface) {
            (void)SurfaceUtils::GetInstance()->Remove(id);
        }
        surface_ = nullptr;
    }
}

void AVRecorderAsyncContext::AVRecorderSignError(int32_t code, const std::string &param1, const std::string &param2)
{
    std::string message = MSExtErrorAPI9ToString(static_cast<MediaServiceExtErrCodeAPI9>(code), param1, param2);
    SignError(code, message);
}
} // namespace Media
} // namespace OHOS