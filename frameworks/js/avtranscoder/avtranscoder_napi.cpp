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

#include <climits>
#include "avtranscoder_callback.h"
#include "media_dfx.h"
#include "media_log.h"
#include "media_errors.h"
#include "scope_guard.h"
#include "common_napi.h"
#include "surface_utils.h"
#include "string_ex.h"
#include "avcodec_info.h"
#include "av_common.h"
#ifdef SUPPORT_JSSTACK
#include "xpower_event_js.h"
#endif
#include "avtranscoder_napi.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVTransCoderNapi"};
}

namespace OHOS {
namespace Media {
using namespace MediaAVCodec;
thread_local napi_ref AVTransCoderNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "AVTransCoder";
const std::map<std::string, std::vector<std::string>> STATE_CTRL = {
    {AVTransCoderState::STATE_IDLE, {
        AVTransCoderOpt::SET_AV_TRANSCODER_CONFIG,
    }},
    {AVTransCoderState::STATE_PREPARED, {}},
    {AVTransCoderState::STATE_STARTED, {
        AVTransCoderOpt::START,
        AVTransCoderOpt::RESUME
    }},
    {AVTransCoderState::STATE_PAUSED, {
        AVTransCoderOpt::PAUSE
    }},
    {AVTransCoderState::STATE_CANCELLED, {
        AVTransCoderOpt::CANCEL
    }},
    {AVTransCoderState::STATE_RELEASED, {
        AVTransCoderOpt::RELEASE
    }},
    {AVTransCoderState::STATE_COMPLETED, {}},
    {AVTransCoderState::STATE_ERROR, {}},
};
std::map<std::string, AVTransCoderNapi::AvTransCoderTaskqFunc> AVTransCoderNapi::taskQFuncs_ = {
    {AVTransCoderOpt::START, &AVTransCoderNapi::Start},
    {AVTransCoderOpt::PAUSE, &AVTransCoderNapi::Pause},
    {AVTransCoderOpt::RESUME, &AVTransCoderNapi::Resume},
    {AVTransCoderOpt::CANCEL, &AVTransCoderNapi::Cancel},
    {AVTransCoderOpt::RELEASE, &AVTransCoderNapi::Release},
};

AVTransCoderNapi::AVTransCoderNapi()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR "Instances create", FAKE_POINTER(this));
}

AVTransCoderNapi::~AVTransCoderNapi()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR "Instances destroy", FAKE_POINTER(this));
}

napi_value AVTransCoderNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAVTransCoder", JsCreateAVTransCoder),
    };

    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("prepare", JsPrepare),
        DECLARE_NAPI_FUNCTION("start", JsStart),
        DECLARE_NAPI_FUNCTION("pause", JsPause),
        DECLARE_NAPI_FUNCTION("resume", JsResume),
        DECLARE_NAPI_FUNCTION("cancel", JsCancel),
        DECLARE_NAPI_FUNCTION("release", JsRelease),
        DECLARE_NAPI_FUNCTION("on", JsSetEventCallback),
        DECLARE_NAPI_FUNCTION("off", JsCancelEventCallback),

        DECLARE_NAPI_GETTER_SETTER("srcUrl", JsGetSrcUrl, JsSetSrcUrl),
        DECLARE_NAPI_GETTER_SETTER("srcFd", JsGetSrcFd, JsSetSrcFd),
        DECLARE_NAPI_GETTER_SETTER("dstFd", JsGetDstFd, JsSetDstFd),
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
                                           sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define AVTransCoder class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value AVTransCoderNapi::Constructor(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVTransCoder::Constructor");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "failed to napi_get_cb_info");

    AVTransCoderNapi *jsTransCoder = new(std::nothrow) AVTransCoderNapi();
    CHECK_AND_RETURN_RET_LOG(jsTransCoder != nullptr, result, "failed to new AVTransCoderNapi");

    jsTransCoder->env_ = env;
    jsTransCoder->transCoder_ = TransCoderFactory::CreateTransCoder();
    CHECK_AND_RETURN_RET_LOG(jsTransCoder->transCoder_ != nullptr, result, "failed to CreateTransCoder");

    jsTransCoder->taskQue_ = std::make_unique<TaskQueue>("OS_AVTransCoderNapi");
    (void)jsTransCoder->taskQue_->Start();

    jsTransCoder->transCoderCb_ = std::make_shared<AVTransCoderCallback>(env);
    CHECK_AND_RETURN_RET_LOG(jsTransCoder->transCoderCb_ != nullptr, result, "failed to CreateTransCoderCb");
    (void)jsTransCoder->transCoder_->SetTransCoderCallback(jsTransCoder->transCoderCb_);

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(jsTransCoder),
                       AVTransCoderNapi::Destructor, nullptr, nullptr);
    if (status != napi_ok) {
        delete jsTransCoder;
        MEDIA_LOGE("Failed to wrap native instance");
        return result;
    }

    MEDIA_LOGI("Constructor success");
    return jsThis;
}

void AVTransCoderNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    MediaTrace trace("AVTransCoder::Destructor");
    (void)finalize;
    if (nativeObject != nullptr) {
        AVTransCoderNapi *napi = reinterpret_cast<AVTransCoderNapi *>(nativeObject);
        if (napi->taskQue_ != nullptr) {
            (void)napi->taskQue_->Stop();
        }

        napi->transCoderCb_ = nullptr;

        if (napi->transCoder_) {
            napi->transCoder_->Release();
            napi->transCoder_ = nullptr;
        }

        delete napi;
    }
    MEDIA_LOGI("Destructor success");
}

napi_value AVTransCoderNapi::JsCreateAVTransCoder(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVTransCoder::JsCreateAVTransCoder");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    MEDIA_LOGI("JsCreateAVTransCoder Start");

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "failed to napi_get_cb_info");

    std::unique_ptr<AVTransCoderAsyncContext> asyncCtx = std::make_unique<AVTransCoderAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    asyncCtx->JsResult = std::make_unique<MediaJsResultInstance>(constructor_);
    asyncCtx->ctorFlag = true;

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsCreateAVTransCoder", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
              MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    asyncCtx.release();

    MEDIA_LOGI("JsCreateAVTransCoder success");
    return result;
}

RetInfo GetReturnRet(int32_t errCode, const std::string &operate, const std::string &param, const std::string &add = "")
{
    MEDIA_LOGE("failed to %{public}s, param %{public}s, errCode = %{public}d", operate.c_str(), param.c_str(), errCode);
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

napi_value AVTransCoderNapi::JsPrepare(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVTransCoder::JsPrepare");
    const std::string &opt = AVTransCoderOpt::PREPARE;
    MEDIA_LOGI("Js %{public}s Start", opt.c_str());

    const int32_t maxParam = 2; // config + callbackRef
    size_t argCount = maxParam;
    napi_value args[maxParam] = { nullptr };

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVTransCoderAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");
    asyncCtx->napi = AVTransCoderNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceAndArgs");
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi->taskQue_ != nullptr, result, "taskQue is nullptr!");

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[1]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    if (asyncCtx->napi->CheckStateMachine(opt) == MSERR_OK) {
        if (asyncCtx->napi->GetConfig(asyncCtx, env, args[0]) == MSERR_OK) {
            asyncCtx->task_ = AVTransCoderNapi::GetPrepareTask(asyncCtx);
            (void)asyncCtx->napi->taskQue_->EnqueueTask(asyncCtx->task_);
        }
    } else {
        asyncCtx->AVTransCoderSignError(MSERR_INVALID_OPERATION, opt, "");
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, opt.c_str(), NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
        AVTransCoderAsyncContext* asyncCtx = reinterpret_cast<AVTransCoderAsyncContext *>(data);
        CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "asyncCtx is nullptr!");

        if (asyncCtx->task_) {
            auto result = asyncCtx->task_->GetResult();
            if (result.Value().first != MSERR_EXT_API9_OK) {
                asyncCtx->SignError(result.Value().first, result.Value().second);
            }
        }
        MEDIA_LOGI("The js thread of prepare finishes execution and returns");
    }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    asyncCtx.release();

    MEDIA_LOGI("Js %{public}s End", opt.c_str());
    return result;
}

std::shared_ptr<TaskHandler<RetInfo>> AVTransCoderNapi::GetPrepareTask(
    std::unique_ptr<AVTransCoderAsyncContext> &asyncCtx)
{
    return std::make_shared<TaskHandler<RetInfo>>([napi = asyncCtx->napi, config = asyncCtx->config_]() {
        const std::string &option = AVTransCoderOpt::PREPARE;
        MEDIA_LOGI("%{public}s Start", option.c_str());
        CHECK_AND_RETURN_RET(napi != nullptr && napi->transCoder_ != nullptr && config != nullptr,
            GetReturnRet(MSERR_INVALID_OPERATION, option, ""));

        CHECK_AND_RETURN_RET(napi->CheckStateMachine(option) == MSERR_OK,
            GetReturnRet(MSERR_INVALID_OPERATION, option, ""));

        RetInfo retinfo = napi->Configure(config);
        CHECK_AND_RETURN_RET(retinfo.first == MSERR_OK, ((void)napi->transCoder_->Cancel(), retinfo));

        int32_t ret = napi->transCoder_->Prepare();
        CHECK_AND_RETURN_RET(ret == MSERR_OK, ((void)napi->transCoder_->Cancel(), GetReturnRet(ret, "Prepare", "")));

        napi->StateCallback(AVTransCoderState::STATE_PREPARED);
        MEDIA_LOGI("%{public}s End", option.c_str());
        return RetInfo(MSERR_EXT_API9_OK, "");
    });
}

napi_value AVTransCoderNapi::JsStart(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVTransCoder::JsStart");
#ifdef SUPPORT_JSSTACK
    HiviewDFX::ReportXPowerJsStackSysEvent(env, "STREAM_CHANGE", "SRC=Media");
#endif
    return ExecuteByPromise(env, info, AVTransCoderOpt::START);
}

napi_value AVTransCoderNapi::JsPause(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVTransCoder::JsPause");
    return ExecuteByPromise(env, info, AVTransCoderOpt::PAUSE);
}

napi_value AVTransCoderNapi::JsResume(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVTransCoder::JsResume");
#ifdef SUPPORT_JSSTACK
    HiviewDFX::ReportXPowerJsStackSysEvent(env, "STREAM_CHANGE", "SRC=Media");
#endif
    return ExecuteByPromise(env, info, AVTransCoderOpt::RESUME);
}

napi_value AVTransCoderNapi::JsCancel(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVTransCoder::JsCancle");
    return ExecuteByPromise(env, info, AVTransCoderOpt::CANCEL);
}

napi_value AVTransCoderNapi::JsRelease(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVTransCoder::JsRelease");
    return ExecuteByPromise(env, info, AVTransCoderOpt::RELEASE);
}

napi_value AVTransCoderNapi::JsSetEventCallback(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVTransCoder::JsSetEventCallback");
    MEDIA_LOGI("JsSetEventCallback Start");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 2;
    napi_value args[2] = { nullptr, nullptr };
    AVTransCoderNapi *transCoderNapi = AVTransCoderNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(transCoderNapi != nullptr, result, "Failed to retrieve instance");

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        transCoderNapi->ErrorCallback(MSERR_INVALID_VAL, "SetEventCallback");
        return result;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);

    napi_ref ref = nullptr;
    napi_status status = napi_create_reference(env, args[1], 1, &ref);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && ref != nullptr, result, "failed to create reference!");

    std::shared_ptr<AutoRef> autoRef = std::make_shared<AutoRef>(env, ref);
    transCoderNapi->SetCallbackReference(callbackName, autoRef);

    MEDIA_LOGI("JsSetEventCallback End");
    return result;
}

napi_value AVTransCoderNapi::JsCancelEventCallback(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVTransCoder::JsCancelEventCallback");
    MEDIA_LOGI("JsCancelEventCallback Start");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 1;
    napi_value args[1] = { nullptr };
    AVTransCoderNapi *transCoderNapi = AVTransCoderNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(transCoderNapi != nullptr, result, "Failed to retrieve instance");

    napi_valuetype valueType0 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string) {
        transCoderNapi->ErrorCallback(MSERR_INVALID_VAL, "CancelEventCallback");
        return result;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);

    transCoderNapi->CancelCallbackReference(callbackName);

    MEDIA_LOGI("JsCancelEventCallback End");
    return result;
}

AVTransCoderNapi *AVTransCoderNapi::GetJsInstanceAndArgs(
    napi_env env, napi_callback_info info, size_t &argCount, napi_value *args)
{
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, nullptr, "failed to napi_get_cb_info");
    MEDIA_LOGI("argCount:%{public}zu", argCount);

    AVTransCoderNapi *transCoderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&transCoderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && transCoderNapi != nullptr, nullptr, "failed to retrieve instance");

    return transCoderNapi;
}

std::shared_ptr<TaskHandler<RetInfo>> AVTransCoderNapi::GetPromiseTask(AVTransCoderNapi *avnapi, const std::string &opt)
{
    return std::make_shared<TaskHandler<RetInfo>>([napi = avnapi, option = opt]() {
        MEDIA_LOGI("%{public}s Start", option.c_str());
        CHECK_AND_RETURN_RET(napi != nullptr && napi->transCoder_ != nullptr,
            GetReturnRet(MSERR_INVALID_OPERATION, option, ""));

        CHECK_AND_RETURN_RET(napi->CheckStateMachine(option) == MSERR_OK,
            GetReturnRet(MSERR_INVALID_OPERATION, option, ""));
        
        CHECK_AND_RETURN_RET(napi->CheckRepeatOperation(option) == MSERR_OK,
            RetInfo(MSERR_EXT_API9_OK, ""));

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

napi_value AVTransCoderNapi::ExecuteByPromise(napi_env env, napi_callback_info info, const std::string &opt)
{
    MEDIA_LOGI("Js %{public}s Start", opt.c_str());
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    size_t argCount = 1; // Only callbackRef parameter
    napi_value args[1] = { nullptr }; // Only callbackRef parameter

    auto asyncCtx = std::make_unique<AVTransCoderAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");
    asyncCtx->napi = AVTransCoderNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceAndArgs");
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi->taskQue_ != nullptr, result, "taskQue is nullptr!");

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    if (asyncCtx->napi->CheckStateMachine(opt) == MSERR_OK) {
        asyncCtx->task_ = AVTransCoderNapi::GetPromiseTask(asyncCtx->napi, opt);
        (void)asyncCtx->napi->taskQue_->EnqueueTask(asyncCtx->task_);
        asyncCtx->opt_ = opt;
    } else {
        asyncCtx->AVTransCoderSignError(MSERR_INVALID_OPERATION, opt, "");
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, opt.c_str(), NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
        AVTransCoderAsyncContext* asyncCtx = reinterpret_cast<AVTransCoderAsyncContext *>(data);
        CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "asyncCtx is nullptr!");

        if (asyncCtx->task_) {
            auto result = asyncCtx->task_->GetResult();
            if (result.Value().first != MSERR_EXT_API9_OK) {
                asyncCtx->SignError(result.Value().first, result.Value().second);
            }
        }
        MEDIA_LOGI("The js thread of %{public}s finishes execution and returns", asyncCtx->opt_.c_str());
    }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    asyncCtx.release();

    MEDIA_LOGI("Js %{public}s End", opt.c_str());
    return result;
}

napi_value AVTransCoderNapi::JsSetSrcUrl(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVTransCoderNapi::set url");
    const std::string &opt = AVTransCoderOpt::PREPARE;
    size_t argCount = 1;
    napi_value args[1] = { nullptr };

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVTransCoderAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");
    asyncCtx->napi = AVTransCoderNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceAndArgs");

    // get url from js
    asyncCtx->napi->srcUrl_ = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGI("JsSetUrl url: %{public}s", asyncCtx->napi->srcUrl_.c_str());

    auto task = std::make_shared<TaskHandler<void>>([napi = asyncCtx->napi]() {
        MEDIA_LOGI("JsSetSrcUrl Task");
        napi->SetInputFile(napi->srcUrl_);
    });
    (void)asyncCtx->napi->taskQue_->EnqueueTask(task);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, opt.c_str(), NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
        AVTransCoderAsyncContext* asyncCtx = reinterpret_cast<AVTransCoderAsyncContext *>(data);
        CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "asyncCtx is nullptr!");

        if (asyncCtx->task_) {
            auto result = asyncCtx->task_->GetResult();
            if (result.Value().first != MSERR_EXT_API9_OK) {
                asyncCtx->SignError(result.Value().first, result.Value().second);
            }
        }
        MEDIA_LOGI("The js thread of %{public}s finishes execution and returns", asyncCtx->opt_.c_str());
    }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    asyncCtx.release();

    MEDIA_LOGI("JsSetSrcUrl Out");
    return result;
}

napi_value AVTransCoderNapi::JsGetSrcUrl(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVTransCoderNapi::get url");
    size_t argCount = 1;
    napi_value args[1] = { nullptr };

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVTransCoderAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");
    asyncCtx->napi = AVTransCoderNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceAndArgs");

    napi_value value = nullptr;
    (void)napi_create_string_utf8(env, asyncCtx->napi->srcUrl_.c_str(), NAPI_AUTO_LENGTH, &value);

    MEDIA_LOGD("JsGetUrl Out Current Url: %{public}s", asyncCtx->napi->srcUrl_.c_str());
    return value;
}

napi_value AVTransCoderNapi::JsSetSrcFd(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVTransCoderNapi::set fd");
    const std::string &opt = AVTransCoderOpt::PREPARE;
    size_t argCount = 1;
    napi_value args[1] = { nullptr };

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVTransCoderAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");
    asyncCtx->napi = AVTransCoderNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceAndArgs");

    // get url from js
    if (!CommonNapi::GetFdArgument(env, args[0], asyncCtx->napi->srcFd_)) {
        MEDIA_LOGE("get fileDescriptor argument failed!");
        return result;
    }

    auto task = std::make_shared<TaskHandler<void>>([napi = asyncCtx->napi]() {
        MEDIA_LOGI("JsSetSrcFd Task");
        napi->SetInputFile(napi->srcFd_.fd, napi->srcFd_.offset, napi->srcFd_.length);
    });
    (void)asyncCtx->napi->taskQue_->EnqueueTask(task);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, opt.c_str(), NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
        AVTransCoderAsyncContext* asyncCtx = reinterpret_cast<AVTransCoderAsyncContext *>(data);
        CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "asyncCtx is nullptr!");

        if (asyncCtx->task_) {
            auto result = asyncCtx->task_->GetResult();
            if (result.Value().first != MSERR_EXT_API9_OK) {
                asyncCtx->SignError(result.Value().first, result.Value().second);
            }
        }
        MEDIA_LOGI("The js thread of %{public}s finishes execution and returns", asyncCtx->opt_.c_str());
    }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    asyncCtx.release();

    MEDIA_LOGI("JsSetSrcFd Out");
    return result;
}

napi_value AVTransCoderNapi::JsGetSrcFd(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVTransCoderNapi::get url");
    size_t argCount = 1;
    napi_value args[1] = { nullptr };

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVTransCoderAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");
    asyncCtx->napi = AVTransCoderNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceAndArgs");

    napi_value value = nullptr;
    (void)napi_create_string_utf8(env, asyncCtx->napi->srcUrl_.c_str(), NAPI_AUTO_LENGTH, &value);

    MEDIA_LOGD("JsGetUrl Out Current Url: %{public}s", asyncCtx->napi->srcUrl_.c_str());
    return value;
}

napi_value AVTransCoderNapi::JsSetDstFd(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVTransCoderNapi::set fd");
    const std::string &opt = AVTransCoderOpt::PREPARE;
    size_t argCount = 1;
    napi_value args[1] = { nullptr };

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVTransCoderAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");
    asyncCtx->napi = AVTransCoderNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceAndArgs");

    // get url from js
    napi_get_value_int32(env, args[0], &asyncCtx->napi->dstFd_);

    auto task = std::make_shared<TaskHandler<void>>([napi = asyncCtx->napi]() {
        MEDIA_LOGI("JsSetSrcFd Task");
        napi->SetOutputFile(napi->dstFd_);
    });
    (void)asyncCtx->napi->taskQue_->EnqueueTask(task);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, opt.c_str(), NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
        AVTransCoderAsyncContext* asyncCtx = reinterpret_cast<AVTransCoderAsyncContext *>(data);
        CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "asyncCtx is nullptr!");

        if (asyncCtx->task_) {
            auto result = asyncCtx->task_->GetResult();
            if (result.Value().first != MSERR_EXT_API9_OK) {
                asyncCtx->SignError(result.Value().first, result.Value().second);
            }
        }
        MEDIA_LOGI("The js thread of %{public}s finishes execution and returns", asyncCtx->opt_.c_str());
    }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    asyncCtx.release();

    MEDIA_LOGI("JsSetDstFd Out");
    return result;
}

napi_value AVTransCoderNapi::JsGetDstFd(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVTransCoderNapi::get url");
    size_t argCount = 1;
    napi_value args[1] = { nullptr };

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVTransCoderAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");
    asyncCtx->napi = AVTransCoderNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceAndArgs");

    napi_value value = nullptr;
    (void)napi_create_int32(env, asyncCtx->napi->dstFd_, &value);

    MEDIA_LOGD("JsGetUrl Out Current Url: %{public}s", asyncCtx->napi->srcUrl_.c_str());
    return value;
}

RetInfo AVTransCoderNapi::Start()
{
    int32_t ret = transCoder_->Start();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, GetReturnRet(ret, "Start", ""));
    StateCallback(AVTransCoderState::STATE_STARTED);
    return RetInfo(MSERR_EXT_API9_OK, "");
}

RetInfo AVTransCoderNapi::Pause()
{
    int32_t ret = transCoder_->Pause();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, GetReturnRet(ret, "Pause", ""));
    StateCallback(AVTransCoderState::STATE_PAUSED);
    return RetInfo(MSERR_EXT_API9_OK, "");
}

RetInfo AVTransCoderNapi::Resume()
{
    int32_t ret = transCoder_->Resume();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, GetReturnRet(ret, "Resume", ""));
    StateCallback(AVTransCoderState::STATE_STARTED);
    return RetInfo(MSERR_EXT_API9_OK, "");
}

RetInfo AVTransCoderNapi::Cancel()
{
    int32_t ret = transCoder_->Cancel();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, GetReturnRet(ret, "Stop", ""));
    StateCallback(AVTransCoderState::STATE_CANCELLED);
    hasConfiged_ = false;
    return RetInfo(MSERR_EXT_API9_OK, "");
}

RetInfo AVTransCoderNapi::Release()
{
    int32_t ret = transCoder_->Release();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, GetReturnRet(ret, "Release", ""));

    StateCallback(AVTransCoderState::STATE_RELEASED);
    CancelCallback();
    hasConfiged_ = false;
    return RetInfo(MSERR_EXT_API9_OK, "");
}

RetInfo AVTransCoderNapi::SetInputFile(std::string url)
{
    int32_t ret = transCoder_->SetInputFile(url);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, GetReturnRet(ret, "SetInputFile", ""));
    return RetInfo(MSERR_EXT_API9_OK, "");
}

RetInfo AVTransCoderNapi::SetInputFile(int32_t fd, int64_t offset, int64_t size)
{
    int32_t ret = transCoder_->SetInputFile(fd, offset, size);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, GetReturnRet(ret, "SetInputFile", ""));
    return RetInfo(MSERR_EXT_API9_OK, "");
}

RetInfo AVTransCoderNapi::SetOutputFile(int32_t fd)
{
    int32_t ret = transCoder_->SetOutputFile(fd);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, GetReturnRet(ret, "SetOutputFile", ""));
    return RetInfo(MSERR_EXT_API9_OK, "");
}

int32_t AVTransCoderNapi::CheckStateMachine(const std::string &opt)
{
    auto napiCb = std::static_pointer_cast<AVTransCoderCallback>(transCoderCb_);
    CHECK_AND_RETURN_RET_LOG(napiCb != nullptr, MSERR_INVALID_OPERATION, "napiCb is nullptr!");

    std::string curState = napiCb->GetState();
    std::vector<std::string> allowedOpt = STATE_LIST.at(curState);
    if (find(allowedOpt.begin(), allowedOpt.end(), opt) == allowedOpt.end()) {
        MEDIA_LOGE("The %{public}s operation is not allowed in the %{public}s state!", opt.c_str(), curState.c_str());
        return MSERR_INVALID_OPERATION;
    }

    return MSERR_OK;
}

int32_t AVTransCoderNapi::CheckRepeatOperation(const std::string &opt)
{
    auto napiCb = std::static_pointer_cast<AVTransCoderCallback>(transCoderCb_);
    CHECK_AND_RETURN_RET_LOG(napiCb != nullptr, MSERR_INVALID_OPERATION, "napiCb is nullptr!");

    std::string curState = napiCb->GetState();
    std::vector<std::string> repeatOpt = STATE_CTRL.at(curState);
    if (find(repeatOpt.begin(), repeatOpt.end(), opt) != repeatOpt.end()) {
        MEDIA_LOGI("Current state is %{public}s. Please do not call %{public}s again!",
            curState.c_str(), opt.c_str());
        return MSERR_INVALID_OPERATION;
    }

    return MSERR_OK;
}

RetInfo AVTransCoderNapi::Configure(std::shared_ptr<AVTransCoderConfig> config)
{
    CHECK_AND_RETURN_RET(transCoder_ != nullptr, GetReturnRet(MSERR_INVALID_OPERATION, "Configure", ""));
    CHECK_AND_RETURN_RET(config != nullptr, GetReturnRet(MSERR_INVALID_VAL, "Configure", "config"));

    if (hasConfiged_) {
        MEDIA_LOGE("AVTransCoderConfig has been configured and will not be configured again");
        return RetInfo(MSERR_EXT_API9_OK, "");
    }

    int32_t ret = transCoder_->SetOutputFormat(config->fileFormat);
        CHECK_AND_RETURN_RET(ret == MSERR_OK, GetReturnRet(ret, "SetOutputFormat", "fileFormat"));

    ret = transCoder_->SetAudioEncoder(config->audioCodecFormat);
        CHECK_AND_RETURN_RET(ret == MSERR_OK, GetReturnRet(ret, "SetAudioEncoder", "audioCodecFormat"));
    
    ret = transCoder_->SetAudioEncodingBitRate(config->audioBitrate);
        CHECK_AND_RETURN_RET(ret == MSERR_OK, GetReturnRet(ret, "SetAudioEncoderBitRate", "audioBitrate"));
    
    ret = transCoder_->SetVideoEncoder(config->videoCodecFormat);
        CHECK_AND_RETURN_RET(ret == MSERR_OK, GetReturnRet(ret, "SetVideoEncoder", "videoCodecFormat"));
    
    ret = transCoder_->SetVideoEncodingBitRate(config->videoBitrate);
        CHECK_AND_RETURN_RET(ret == MSERR_OK, GetReturnRet(ret, "SetVideoEncoderBitRate", "videoBitrate"));

    ret = transCoder_->SetVideoSize(config->videoFrameWidth, config->videoFrameHeight);
        CHECK_AND_RETURN_RET(ret == MSERR_OK, GetReturnRet(ret, "SetVideoSize", "videoSize"));

    hasConfiged_ = true;
    return RetInfo(MSERR_EXT_API9_OK, "");
}

void AVTransCoderNapi::ErrorCallback(int32_t errCode, const std::string &operate, const std::string &add)
{
    MEDIA_LOGE("failed to %{public}s, errCode = %{public}d", operate.c_str(), errCode);
    CHECK_AND_RETURN_LOG(transCoderCb_ != nullptr, "transCoderCb_ is nullptr!");
    auto napiCb = std::static_pointer_cast<AVTransCoderCallback>(transCoderCb_);

    MediaServiceExtErrCodeAPI9 err = MSErrorToExtErrorAPI9(static_cast<MediaServiceErrCode>(errCode));
    std::string msg = MSExtErrorAPI9ToString(err, operate, "") + add;
    napiCb->SendErrorCallback(errCode, msg);
}

void AVTransCoderNapi::StateCallback(const std::string &state)
{
    MEDIA_LOGI("Change state to %{public}s", state.c_str());
    CHECK_AND_RETURN_LOG(transCoderCb_ != nullptr, "transCoderCb_ is nullptr!");
    auto napiCb = std::static_pointer_cast<AVTransCoderCallback>(transCoderCb_);
    napiCb->SendStateCallback(state, StateChangeReason::USER);
}

void AVTransCoderNapi::SetCallbackReference(const std::string &callbackName, std::shared_ptr<AutoRef> ref)
{
    eventCbMap_[callbackName] = ref;
    CHECK_AND_RETURN_LOG(transCoderCb_ != nullptr, "transCoderCb_ is nullptr!");
    auto napiCb = std::static_pointer_cast<AVTransCoderCallback>(transCoderCb_);
    napiCb->SaveCallbackReference(callbackName, ref);
}

void AVTransCoderNapi::CancelCallbackReference(const std::string &callbackName)
{
    CHECK_AND_RETURN_LOG(transCoderCb_ != nullptr, "transCoderCb_ is nullptr!");
    auto napiCb = std::static_pointer_cast<AVTransCoderCallback>(transCoderCb_);
    napiCb->CancelCallbackReference(callbackName);
    eventCbMap_[callbackName] = nullptr;
}

void AVTransCoderNapi::CancelCallback()
{
    CHECK_AND_RETURN_LOG(transCoderCb_ != nullptr, "transCoderCb_ is nullptr!");
    auto napiCb = std::static_pointer_cast<AVTransCoderCallback>(transCoderCb_);
    napiCb->ClearCallbackReference();
}

int32_t AVTransCoderNapi::GetAudioCodecFormat(const std::string &mime, AudioCodecFormat &codecFormat)
{
    MEDIA_LOGI("mime %{public}s", mime.c_str());
    const std::map<std::string_view, AudioCodecFormat> mimeStrToCodecFormat = {
        { CodecMimeType::AUDIO_AAC, AudioCodecFormat::AAC_LC },
        { "", AudioCodecFormat::AUDIO_DEFAULT },
    };

    auto iter = mimeStrToCodecFormat.find(mime);
    if (iter != mimeStrToCodecFormat.end()) {
        codecFormat = iter->second;
        return MSERR_OK;
    }
    return MSERR_INVALID_VAL;
}

int32_t AVTransCoderNapi::GetAudioConfig(std::unique_ptr<AVTransCoderAsyncContext> &asyncCtx,
    napi_env env, napi_value args)
{
    std::shared_ptr<AVTransCoderConfig> config = asyncCtx->config_;
    int32_t ret = MSERR_OK;
    std::string audioCodec = CommonNapi::GetPropertyString(env, args, "audioCodec");
    ret = AVTransCoderNapi::GetAudioCodecFormat(audioCodec, config->audioCodecFormat);
    CHECK_AND_RETURN_RET(ret == MSERR_OK,
        (asyncCtx->AVTransCoderSignError(ret, "GetAudioCodecFormat", "audioCodecFormat"), ret));

    CHECK_AND_RETURN_RET(CommonNapi::GetPropertyInt32(env, args, "audioBitrate", config->audioBitrate),
        (asyncCtx->AVTransCoderSignError(ret, "GetaudioBitrate", "audioBitrate"), ret));

    return MSERR_OK;
}

int32_t AVTransCoderNapi::GetVideoCodecFormat(const std::string &mime, VideoCodecFormat &codecFormat)
{
    MEDIA_LOGI("mime %{public}s", mime.c_str());
    const std::map<std::string_view, VideoCodecFormat> mimeStrToCodecFormat = {
        { CodecMimeType::VIDEO_AVC, VideoCodecFormat::H264 },
        { CodecMimeType::VIDEO_HEVC, VideoCodecFormat::H265 },
        { "", VideoCodecFormat::VIDEO_DEFAULT },
    };

    auto iter = mimeStrToCodecFormat.find(mime);
    if (iter != mimeStrToCodecFormat.end()) {
        codecFormat = iter->second;
        return MSERR_OK;
    }
    return MSERR_INVALID_VAL;
}

int32_t AVTransCoderNapi::GetVideoConfig(std::unique_ptr<AVTransCoderAsyncContext> &asyncCtx,
    napi_env env, napi_value args)
{
    std::shared_ptr<AVTransCoderConfig> config = asyncCtx->config_;
    int32_t ret = MSERR_OK;
    std::string videoCodec = CommonNapi::GetPropertyString(env, args, "videoCodec");
    ret = AVTransCoderNapi::GetVideoCodecFormat(videoCodec, config->videoCodecFormat);
    CHECK_AND_RETURN_RET(ret == MSERR_OK,
        (asyncCtx->AVTransCoderSignError(ret, "GetVideoCodecFormat", "videoCodecFormat"), ret));

    CHECK_AND_RETURN_RET(CommonNapi::GetPropertyInt32(env, args, "videoBitrate", config->videoBitrate),
        (asyncCtx->AVTransCoderSignError(ret, "GetVideoBitrate", "videoBitrate"), ret));
    
    CHECK_AND_RETURN_RET(CommonNapi::GetPropertyInt32(env, args, "videoFrameWidth", config->videoFrameWidth),
        (asyncCtx->AVTransCoderSignError(ret, "GetVideoFrameWidth", "videoFrameWidth"), ret));
    
    CHECK_AND_RETURN_RET(CommonNapi::GetPropertyInt32(env, args, "videoFrameHeight", config->videoFrameHeight),
        (asyncCtx->AVTransCoderSignError(ret, "GetVideoFrameHeight", "videoFrameHeight"), ret));

    return MSERR_OK;
}

int32_t AVTransCoderNapi::GetOutputFormat(const std::string &extension, OutputFormatType &type)
{
    MEDIA_LOGI("mime %{public}s", extension.c_str());
    const std::map<std::string, OutputFormatType> extensionToOutputFormat = {
        { "mp4", OutputFormatType::FORMAT_MPEG_4 },
        { "m4a", OutputFormatType::FORMAT_M4A },
        { "", OutputFormatType::FORMAT_DEFAULT },
    };

    auto iter = extensionToOutputFormat.find(extension);
    if (iter != extensionToOutputFormat.end()) {
        type = iter->second;
        return MSERR_OK;
    }
    return MSERR_INVALID_VAL;
}

int32_t AVTransCoderNapi::GetConfig(std::unique_ptr<AVTransCoderAsyncContext> &asyncCtx,
    napi_env env, napi_value args)
{
    napi_valuetype valueType = napi_undefined;
    napi_value item = nullptr;
    if (args == nullptr || napi_typeof(env, args, &valueType) != napi_ok || valueType != napi_object) {
        asyncCtx->AVTransCoderSignError(MSERR_INVALID_VAL, "GetConfig", "AVTransCoderConfig");
        return MSERR_INVALID_VAL;
    }

    asyncCtx->config_ = std::make_shared<AVTransCoderConfig>();
    CHECK_AND_RETURN_RET(asyncCtx->config_,
        (asyncCtx->AVTransCoderSignError(MSERR_NO_MEMORY, "AVTransCoderConfig", "AVTransCoderConfig"),
            MSERR_NO_MEMORY));

    std::shared_ptr<AVTransCoderConfig> config = asyncCtx->config_;

    int32_t ret = GetAudioConfig(asyncCtx, env, args);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "failed to GetAudioConfig");

    ret = GetVideoConfig(asyncCtx, env, args);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "failed to GetVideoConfig");

    std::string fileFormat = CommonNapi::GetPropertyString(env, item, "fileFormat");
    ret = AVTransCoderNapi::GetOutputFormat(fileFormat, config->fileFormat);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, (asyncCtx->AVTransCoderSignError(ret, "GetOutputFormat", "fileFormat"), ret));
    
    return MSERR_OK;
}

void AVTransCoderAsyncContext::AVTransCoderSignError(int32_t errCode, const std::string &operate,
    const std::string &param, const std::string &add)
{
    RetInfo retInfo = GetReturnRet(errCode, operate, param, add);
    SignError(retInfo.first, retInfo.second);
}


} // namespace Media
} // namespace OHOS