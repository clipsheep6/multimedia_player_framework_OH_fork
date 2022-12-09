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

#include <climits>
#include "avrecorder_callback.h"
#include "media_log.h"
#include "media_errors.h"
#include "scope_guard.h"
#include "common_napi.h"
#include "surface_utils.h"
#include "string_ex.h"
#include "avrecorder_napi.h"
#include "avcodec_info.h"
#include "avcontainer_common.h"

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

        DECLARE_NAPI_GETTER("state", JsGetState),
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
    (void)finalize;
    if (nativeObject != nullptr) {
        AVRecorderNapi *napi = reinterpret_cast<AVRecorderNapi *>(nativeObject);
        if (napi->taskQue_ != nullptr) {
            (void)napi->taskQue_->Stop();
        }

        napi->RemoveSurface();
        napi->recorderCb_ = nullptr;

        if (napi->recorder_) {
            napi->recorder_->Release();
            napi->recorder_ = nullptr;
        }

        if (napi->wrapper_ != nullptr) {
            napi_delete_reference(env, napi->wrapper_);
        }

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

AVRecorderNapi* AVRecorderNapi::GetJsInstanceAndArgs(napi_env env, napi_callback_info info,
    size_t &argCount, napi_value *args)
{
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, nullptr, "failed to napi_get_cb_info");

    AVRecorderNapi *recorderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorderNapi != nullptr, nullptr, "failed to retrieve instance");

    return recorderNapi;
}

std::unique_ptr<AVRecorderAsyncContext> AVRecorderNapi::GetContextWithPromise(napi_env env, napi_callback_info info,
    size_t &argCount, napi_value *args, napi_value &result)
{
    auto asyncCtx = std::make_unique<AVRecorderAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, nullptr, "failed to get AsyncContext");

    asyncCtx->napi = AVRecorderNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, nullptr, "failed to GetJsInstanceAndArgs");

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[argCount - 1]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    return asyncCtx;
}

AVRecorderNapi* AVRecorderNapi::GetJsInstance(napi_env env, napi_callback_info info)
{
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, nullptr, "failed to napi_get_cb_info");

    AVRecorderNapi *recorderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorderNapi != nullptr, nullptr, "failed to retrieve instance");

    return recorderNapi;
}

napi_value AVRecorderNapi::JsPrepare(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("JsPrepare In");

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 2;
    napi_value args[2] = { nullptr };
    auto asyncCtx = AVRecorderNapi::GetContextWithPromise(env, info, argCount, args, result);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");

    // get param
    (void)asyncCtx->napi->GetConfig(asyncCtx, env, args[0]);

    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Prepare", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
        MEDIA_LOGD("Prepare In");
        AVRecorderAsyncContext* asyncCtx = reinterpret_cast<AVRecorderAsyncContext *>(data);
        CHECK_AND_RETURN_RET(asyncCtx != nullptr && asyncCtx->napi != nullptr && asyncCtx->napi->recorder_ != nullptr,
            asyncCtx->AVRecorderSignError(MSERR_INVALID_OPERATION, "Prepare", ""));

        int32_t ret = asyncCtx->napi->Configure(asyncCtx);
        CHECK_AND_RETURN_LOG(ret == MSERR_OK, "failed to Configure");

        ret = asyncCtx->napi->recorder_->Prepare();
        CHECK_AND_RETURN_RET(ret == MSERR_OK, asyncCtx->AVRecorderSignError(ret, "Prepare", ""));

        asyncCtx->napi->StateCallback(AVRecorderState::STATE_PREPARED);
        MEDIA_LOGD("Prepare success");
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

    size_t argCount = 1;
    napi_value args[1] = { nullptr };
    auto asyncCtx = AVRecorderNapi::GetContextWithPromise(env, info, argCount, args, result);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "GetInputSurface", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
        MEDIA_LOGD("GetInputSurface In");
        AVRecorderAsyncContext* asyncCtx = reinterpret_cast<AVRecorderAsyncContext *>(data);
        CHECK_AND_RETURN_RET(asyncCtx != nullptr && asyncCtx->napi != nullptr && asyncCtx->napi->recorder_ != nullptr,
            asyncCtx->AVRecorderSignError(MSERR_INVALID_OPERATION, "GetInputSurface", ""));

        AVRecorderNapi *napi = asyncCtx->napi;
        napi->surface_ = napi->recorder_->GetSurface(napi->videoSourceID);
        CHECK_AND_RETURN_RET_LOG(napi->surface_ != nullptr,
            asyncCtx->AVRecorderSignError(MSERR_INVALID_OPERATION, "GetInputSurface", ""), "failed to GetSurface");

        SurfaceError error = SurfaceUtils::GetInstance()->Add(napi->surface_->GetUniqueId(), napi->surface_);
        CHECK_AND_RETURN_RET_LOG(error == SURFACE_ERROR_OK,
            asyncCtx->AVRecorderSignError(MSERR_INVALID_OPERATION, "GetInputSurface", ""), "failed to Add Surface");

        auto surfaceId = std::to_string(napi->surface_->GetUniqueId());
        asyncCtx->JsResult = std::make_unique<MediaJsResultString>(surfaceId);
    }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();
    MEDIA_LOGD("JsGetInputSurface End");
    return result;
}

napi_value AVRecorderNapi::JsStart(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("JsStart In");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    AVRecorderNapi *recorderNapi = AVRecorderNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(recorderNapi != nullptr, result, "failed to GetJsInstance");
    CHECK_AND_RETURN_RET_LOG(recorderNapi->recorder_ != nullptr, result, "recorder is nullptr!");
    CHECK_AND_RETURN_RET_LOG(recorderNapi->taskQue_ != nullptr, result, "taskQue is nullptr!");

    auto task = std::make_shared<TaskHandler<void>>([napi = recorderNapi]() {
        MEDIA_LOGD("Start In");
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

    AVRecorderNapi *recorderNapi = AVRecorderNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(recorderNapi != nullptr, result, "failed to GetJsInstance");
    CHECK_AND_RETURN_RET_LOG(recorderNapi->recorder_ != nullptr, result, "recorder is nullptr!");
    CHECK_AND_RETURN_RET_LOG(recorderNapi->taskQue_ != nullptr, result, "taskQue is nullptr!");

    auto task = std::make_shared<TaskHandler<void>>([napi = recorderNapi]() {
        MEDIA_LOGD("Pause In");
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

    AVRecorderNapi *recorderNapi = AVRecorderNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(recorderNapi != nullptr, result, "failed to GetJsInstance");
    CHECK_AND_RETURN_RET_LOG(recorderNapi->recorder_ != nullptr, result, "recorder is nullptr!");
    CHECK_AND_RETURN_RET_LOG(recorderNapi->taskQue_ != nullptr, result, "taskQue is nullptr!");

    auto task = std::make_shared<TaskHandler<void>>([napi = recorderNapi]() {
        MEDIA_LOGD("Resume In");
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

    AVRecorderNapi *recorderNapi = AVRecorderNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(recorderNapi != nullptr, result, "failed to GetJsInstance");
    CHECK_AND_RETURN_RET_LOG(recorderNapi->recorder_ != nullptr, result, "recorder is nullptr!");
    CHECK_AND_RETURN_RET_LOG(recorderNapi->taskQue_ != nullptr, result, "taskQue is nullptr!");

    auto task = std::make_shared<TaskHandler<void>>([napi = recorderNapi]() {
        MEDIA_LOGD("Stop In");
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

    size_t argCount = 1;
    napi_value args[1] = { nullptr };
    auto asyncCtx = AVRecorderNapi::GetContextWithPromise(env, info, argCount, args, result);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Reset", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
        MEDIA_LOGD("Reset In");
        AVRecorderAsyncContext* asyncCtx = reinterpret_cast<AVRecorderAsyncContext *>(data);
        CHECK_AND_RETURN_RET(asyncCtx != nullptr && asyncCtx->napi != nullptr && asyncCtx->napi->recorder_ != nullptr,
            asyncCtx->AVRecorderSignError(MSERR_INVALID_OPERATION, "Reset", ""));

        asyncCtx->napi->RemoveSurface();
        int32_t ret = asyncCtx->napi->recorder_->Reset();
        CHECK_AND_RETURN_RET(ret == MSERR_OK, asyncCtx->AVRecorderSignError(ret, "Reset", ""));

        asyncCtx->napi->StateCallback(AVRecorderState::STATE_IDLE);
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

    size_t argCount = 1;
    napi_value args[1] = { nullptr };
    auto asyncCtx = AVRecorderNapi::GetContextWithPromise(env, info, argCount, args, result);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Release", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
        MEDIA_LOGD("Release In");
        AVRecorderAsyncContext* asyncCtx = reinterpret_cast<AVRecorderAsyncContext *>(data);
        CHECK_AND_RETURN_RET(asyncCtx != nullptr && asyncCtx->napi != nullptr && asyncCtx->napi->recorder_ != nullptr,
            asyncCtx->AVRecorderSignError(MSERR_INVALID_OPERATION, "Reset", ""));

        asyncCtx->napi->RemoveSurface();
        int32_t ret = asyncCtx->napi->recorder_->Release();
        CHECK_AND_RETURN_RET(ret == MSERR_OK, asyncCtx->AVRecorderSignError(ret, "Release", ""));

        asyncCtx->napi->StateCallback(AVRecorderState::STATE_RELEASED);
        asyncCtx->napi->CancelCallback();
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

    size_t argCount = 2;
    napi_value args[2] = { nullptr, nullptr };
    AVRecorderNapi *recorderNapi = AVRecorderNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(recorderNapi != nullptr, result, "Failed to retrieve instance");

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        recorderNapi->ErrorCallback(MSERR_INVALID_VAL, "SetEventCallback");
        return result;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGD("callbackName: %{public}s", callbackName.c_str());

    napi_ref ref = nullptr;
    napi_status status = napi_create_reference(env, args[1], 1, &ref);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && ref != nullptr, result, "failed to create reference!");

    std::shared_ptr<AutoRef> autoRef = std::make_shared<AutoRef>(env, ref);
    recorderNapi->SetCallbackReference(callbackName, autoRef);

    MEDIA_LOGD("JsSetEventCallback End");
    return result;
}

napi_value AVRecorderNapi::JsGetState(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    AVRecorderNapi *recorderNapi = AVRecorderNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(recorderNapi != nullptr, result, "failed to GetJsInstance");

    auto napiCb = std::static_pointer_cast<AVRecorderCallback>(recorderNapi->recorderCb_);
    CHECK_AND_RETURN_RET_LOG(napiCb != nullptr, result, "napiCb is nullptr!");
    std::string curState = napiCb->GetState();
    MEDIA_LOGD("GetState success, State: %{public}s", curState.c_str());

    napi_value jsResult = nullptr;
    napi_status status = napi_create_string_utf8(env, curState.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "napi_create_string_utf8 error");
    return jsResult;
}

int32_t AVRecorderNapi::GetAudioCodecFormat(const std::string &mime, AudioCodecFormat &codecFormat)
{
    const std::map<std::string_view, AudioCodecFormat> mimeStrToCodecFormat = {
        { CodecMimeType::AUDIO_AAC, AudioCodecFormat::AAC_LC },
    };

    auto iter = mimeStrToCodecFormat.find(mime);
    if (iter != mimeStrToCodecFormat.end()) {
        codecFormat = iter->second;
        return MSERR_OK;
    }
    return MSERR_INVALID_VAL;
}

int32_t AVRecorderNapi::GetVideoCodecFormat(const std::string &mime, VideoCodecFormat &codecFormat)
{
    const std::map<std::string_view, VideoCodecFormat> mimeStrToCodecFormat = {
        { CodecMimeType::VIDEO_AVC, VideoCodecFormat::H264 },
        { CodecMimeType::VIDEO_MPEG4, VideoCodecFormat::MPEG4 },
    };

    auto iter = mimeStrToCodecFormat.find(mime);
    if (iter != mimeStrToCodecFormat.end()) {
        codecFormat = iter->second;
        return MSERR_OK;
    }
    return MSERR_INVALID_VAL;
}

int32_t AVRecorderNapi::GetOutputFormat(const std::string &extension, OutputFormatType &type)
{
    const std::map<std::string, OutputFormatType> extensionToOutputFormat = {
        { "mp4", OutputFormatType::FORMAT_MPEG_4 },
        { "m4a", OutputFormatType::FORMAT_M4A },
    };

    auto iter = extensionToOutputFormat.find(extension);
    if (iter != extensionToOutputFormat.end()) {
        type = iter->second;
        return MSERR_OK;
    }
    return MSERR_INVALID_VAL;
}

int32_t AVRecorderNapi::GetProfile(std::unique_ptr<AVRecorderAsyncContext> &asyncCtx, napi_env env, napi_value args)
{
    (void)asyncCtx;
    napi_value item = nullptr;
    napi_get_named_property(env, args, "profile", &item);
    CHECK_AND_RETURN_RET_LOG(item != nullptr, MSERR_INVALID_VAL, "get profile error");

    AVRecorderProfile &profile = config.profile;

    if (withAudio_) {
        (void)CommonNapi::GetPropertyInt32(env, item, "audioBitrate", profile.audioBitrate);
        (void)CommonNapi::GetPropertyInt32(env, item, "audioChannels", profile.audioChannels);
        std::string audioCodec = CommonNapi::GetPropertyString(env, item, "audioCodec");
        (void)GetAudioCodecFormat(audioCodec, profile.audioCodecFormat);
        (void)CommonNapi::GetPropertyInt32(env, item, "audioSampleRate", profile.auidoSampleRate);
        MEDIA_LOGI("audioBitrate %{public}d, audioChannels %{public}d, audioCodecFormat %{public}d,"
                   " audioSampleRate %{public}d!", profile.audioBitrate, profile.audioChannels,
                   profile.audioCodecFormat, profile.auidoSampleRate);
    }

    if (withVideo_) {
        (void)CommonNapi::GetPropertyInt32(env, item, "videoBitrate", profile.videoBitrate);
        std::string videoCodec = CommonNapi::GetPropertyString(env, item, "videoCodec");
        (void)GetVideoCodecFormat(videoCodec, profile.videoCodecFormat);
        (void)CommonNapi::GetPropertyInt32(env, item, "videoFrameWidth", profile.videoFrameWidth);
        (void)CommonNapi::GetPropertyInt32(env, item, "videoFrameHeight", profile.videoFrameHeight);
        (void)CommonNapi::GetPropertyInt32(env, item, "videoFrameRate", profile.videoFrameRate);
        MEDIA_LOGI("videoBitrate %{public}d, videoCodecFormat %{public}d, videoFrameWidth %{public}d,"
                   " videoFrameHeight %{public}d, videoFrameRate %{public}d",
                   profile.videoBitrate, profile.videoCodecFormat, profile.videoFrameWidth,
                   profile.videoFrameHeight, profile.videoFrameRate);
    }

    std::string outputFile = CommonNapi::GetPropertyString(env, item, "fileFormat");
    (void)GetOutputFormat(outputFile, profile.fileFormat);
    MEDIA_LOGI("fileFormat %{public}d", profile.fileFormat);

    return MSERR_OK;
}

int32_t AVRecorderNapi::GetConfig(std::unique_ptr<AVRecorderAsyncContext> &asyncCtx, napi_env env, napi_value args)
{
    napi_valuetype valueType = napi_undefined;
    if (args == nullptr || napi_typeof(env, args, &valueType) != napi_ok || valueType != napi_object) {
        asyncCtx->AVRecorderSignError(MSERR_INVALID_VAL, "GetConfig", "AVRecorderConfig");
        return MSERR_INVALID_VAL;
    }

    int32_t audioSource = AUDIO_SOURCE_INVALID;
    int32_t videoSource = VIDEO_SOURCE_BUTT;

    bool ret = CommonNapi::GetPropertyInt32(env, args, "audioSourceType", audioSource);
    if (ret) {
        config.audioSourceType = static_cast<AudioSourceType>(audioSource);
        withAudio_ = true;
        MEDIA_LOGI("audioSource Type %{public}d!", audioSource);
    }
    
    ret = CommonNapi::GetPropertyInt32(env, args, "videoSourceType", videoSource);
    if (ret) {
        config.videoSourceType = static_cast<VideoSourceType>(videoSource);
        withVideo_ = true;
        MEDIA_LOGI("videoSource Type %{public}d!", videoSource);
    }

    (void)GetProfile(asyncCtx, env, args);

    config.url = CommonNapi::GetPropertyString(env, args, "url");
    (void)CommonNapi::GetPropertyInt32(env, args, "rotation", config.rotation);
    MEDIA_LOGI("rotation %{public}d!", config.rotation);

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

int32_t AVRecorderNapi::SetProfile(AVRecorderAsyncContext *asyncCtx)
{
    CHECK_AND_RETURN_RET_LOG(recorder_ != nullptr, MSERR_INVALID_OPERATION, "recorder is nullptr");
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, MSERR_INVALID_OPERATION, "recorder is asyncCtx");

    int32_t ret;
    AVRecorderProfile &profile = config.profile;

    ret = recorder_->SetOutputFormat(profile.fileFormat);
    CHECK_AND_RETURN_RET(ret == MSERR_OK,
        (asyncCtx->AVRecorderSignError(ret, "SetOutputFormat", "fileFormat"), ret));

    if (withAudio_) {
        ret = recorder_->SetAudioEncoder(audioSourceID, profile.audioCodecFormat);
        CHECK_AND_RETURN_RET(ret == MSERR_OK,
            (asyncCtx->AVRecorderSignError(ret, "SetAudioEncoder", "audioCodecFormat"), ret));

        ret = recorder_->SetAudioSampleRate(audioSourceID, profile.auidoSampleRate);
        CHECK_AND_RETURN_RET(ret == MSERR_OK,
            (asyncCtx->AVRecorderSignError(ret, "SetAudioSampleRate", "auidoSampleRate"), ret));

        ret = recorder_->SetAudioChannels(audioSourceID, profile.audioChannels);
        CHECK_AND_RETURN_RET(ret == MSERR_OK,
            (asyncCtx->AVRecorderSignError(ret, "SetAudioChannels", "audioChannels"), ret));

        ret = recorder_->SetAudioEncodingBitRate(audioSourceID, profile.audioBitrate);
        CHECK_AND_RETURN_RET(ret == MSERR_OK,
            (asyncCtx->AVRecorderSignError(ret, "SetAudioEncodingBitRate", "audioBitrate"), ret));
    }
    
    if (withVideo_) {
        ret = recorder_->SetVideoEncoder(videoSourceID, profile.videoCodecFormat);
        CHECK_AND_RETURN_RET(ret == MSERR_OK,
            (asyncCtx->AVRecorderSignError(ret, "SetVideoEncoder", "videoCodecFormat"), ret));

        ret = recorder_->SetVideoSize(videoSourceID, profile.videoFrameWidth, profile.videoFrameHeight);
        CHECK_AND_RETURN_RET(ret == MSERR_OK,
            (asyncCtx->AVRecorderSignError(ret, "SetVideoSize", "VideoSize"), ret));

        ret = recorder_->SetVideoFrameRate(videoSourceID, profile.videoFrameRate);
        CHECK_AND_RETURN_RET(ret == MSERR_OK,
            (asyncCtx->AVRecorderSignError(ret, "SetVideoFrameRate", "videoFrameRate"), ret));

        ret = recorder_->SetVideoEncodingBitRate(videoSourceID, profile.videoBitrate);
        CHECK_AND_RETURN_RET(ret == MSERR_OK,
            (asyncCtx->AVRecorderSignError(ret, "SetVideoEncodingBitRate", "videoBitrate"), ret));
    }

    return MSERR_OK;
}

int32_t AVRecorderNapi::Configure(AVRecorderAsyncContext *asyncCtx)
{
    CHECK_AND_RETURN_RET_LOG(recorder_ != nullptr, MSERR_INVALID_OPERATION, "recorder is nullptr");
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, MSERR_INVALID_OPERATION, "recorder is asyncCtx");
    
    int32_t ret;

    if (withAudio_) {
        ret = recorder_->SetAudioSource(config.audioSourceType, audioSourceID);
        CHECK_AND_RETURN_RET(ret == MSERR_OK,
            (asyncCtx->AVRecorderSignError(ret, "SetAudioSource", "audioSourceType"), ret));
    }
    
    if (withVideo_) {
        ret = recorder_->SetVideoSource(config.videoSourceType, videoSourceID);
        CHECK_AND_RETURN_RET(ret == MSERR_OK,
            (asyncCtx->AVRecorderSignError(ret, "SetVideoSource", "videoSourceType"), ret));
    }

    ret = SetProfile(asyncCtx);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Fail to set videoBitrate");

    recorder_->SetLocation(config.location.latitude, config.location.longitude);
    recorder_->SetOrientationHint(config.rotation);

    ret = MSERR_INVALID_VAL;
    const std::string fdHead = "fd://";
    CHECK_AND_RETURN_RET(config.url.find(fdHead) != std::string::npos,
        (asyncCtx->AVRecorderSignError(ret, "Getfd", "uri"), ret));
    int32_t fd = -1;
    std::string inputFd = config.url.substr(fdHead.size());
    CHECK_AND_RETURN_RET(StrToInt(inputFd, fd) == true && fd >= 0,
        (asyncCtx->AVRecorderSignError(ret, "Getfd", "uri"), ret));

    ret = recorder_->SetOutputFile(fd);
    CHECK_AND_RETURN_RET(ret == MSERR_OK,
        (asyncCtx->AVRecorderSignError(ret, "SetOutputFile", "uri"), ret));

    return MSERR_OK;
}

void AVRecorderNapi::ErrorCallback(int32_t errCode, const std::string &operate)
{
    CHECK_AND_RETURN_LOG(recorderCb_ != nullptr, "recorderCb_ is nullptr!");
    auto napiCb = std::static_pointer_cast<AVRecorderCallback>(recorderCb_);

    MediaServiceExtErrCodeAPI9 err = MSErrorToExtErrorAPI9(static_cast<MediaServiceErrCode>(errCode));
    std::string msg = MSExtErrorAPI9ToString(err, operate, "");
    napiCb->SendErrorCallback(errCode, msg);
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

void AVRecorderAsyncContext::AVRecorderSignError(int32_t errCode, const std::string &operate, const std::string &param)
{
    MEDIA_LOGE("failed to %{public}s, param %{public}s, errCode = %{public}d", operate.c_str(), param.c_str(), errCode);
    MediaServiceExtErrCodeAPI9 err = MSErrorToExtErrorAPI9(static_cast<MediaServiceErrCode>(errCode));
    std::string message;
    if (err == MSERR_EXT_API9_INVALID_PARAMETER) {
        message = MSExtErrorAPI9ToString(err, param, "");
    } else {
        message = MSExtErrorAPI9ToString(err, operate, "");
    }

    SignError(err, message);
}
} // namespace Media
} // namespace OHOS