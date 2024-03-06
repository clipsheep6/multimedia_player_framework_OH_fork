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

#include "avimagegenerator_napi.h"
#include "media_log.h"
#include "media_errors.h"
#include "common_napi.h"
#include "pixel_map_napi.h"
#include "string_ex.h"
#include "player_xcollie.h"
#include "media_dfx.h"
#ifdef SUPPORT_JSSTACK
#include "xpower_event_js.h"
#endif
#include "av_common.h"
#include "ipc_skeleton.h"
#include "tokenid_kit.h"

using namespace OHOS::AudioStandard;

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVImageGeneratorNapi"};
}

namespace OHOS {
namespace Media {
static const std::map<HelperStates, std::string> stateMap = {
    {HELPER_IDLE, AVMetadataHelperState::STATE_IDLE},
    {HELPER_PREPARED, AVMetadataHelperState::STATE_PREPARED},
    {HELPER_RELEASED, AVMetadataHelperState::STATE_RELEASED},
    {HELPER_CALL_DONE, AVMetadataHelperState::STATE_CALL_DONE},
};

thread_local napi_ref AVImageGeneratorNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "AVImageGenerator";

AVImageGeneratorNapi::AVImageGeneratorNapi()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVImageGeneratorNapi::~AVImageGeneratorNapi()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value AVImageGeneratorNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAVImageGenerator", JsCreateAVImageGenerator),
    };

    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("fetchFrameByTime", JsFetchFrameAtTime),
        DECLARE_NAPI_FUNCTION("release", JsRelease),

        DECLARE_NAPI_GETTER_SETTER("url", JsGetUrl, JsSetUrl),
        DECLARE_NAPI_GETTER_SETTER("fdSrc", JsGetAVFileDescriptor, JsSetAVFileDescriptor),
    };

    napi_value constructor = nullptr;
    CHECK_AND_RETURN_RET_LOG(sizeof(properties[0]) != 0, nullptr, "Failed to define calss");

    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define AVImageGenerator class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value AVImageGeneratorNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "failed to napi_get_cb_info");

    AVImageGeneratorNapi *generator = new(std::nothrow) AVImageGeneratorNapi();
    CHECK_AND_RETURN_RET_LOG(generator != nullptr, result, "failed to new AVImageGeneratorNapi");

    generator->env_ = env;
    generator->helper_ = AVMetadataHelperFactory::CreateAVMetadataHelper();
    CHECK_AND_RETURN_RET_LOG(generator->helper_ != nullptr, result, "failed to CreateMetadataHelper");

    generator->taskQue_ = std::make_unique<TaskQueue>("AVImageGeneratorNapi");
    (void)generator->taskQue_->Start();

    generator->generatorCb_ = std::make_shared<AVMetadataHelperCallback>(env, generator);
    (void)generator->helper_->SetHelperCallback(generator->generatorCb_);

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(generator),
        AVImageGeneratorNapi::Destructor, nullptr, nullptr);
    if (status != napi_ok) {
        delete generator;
        MEDIA_LOGE("Failed to wrap native instance");
        return result;
    }

    MEDIA_LOGI("Constructor success");
    return jsThis;
}

void AVImageGeneratorNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)finalize;
    if (nativeObject != nullptr) {
        AVImageGeneratorNapi *generator = reinterpret_cast<AVImageGeneratorNapi *>(nativeObject);
        auto task = generator->ReleaseTask();
        if (task != nullptr) {
            MEDIA_LOGI("Destructor Wait Release Task Start");
            task->GetResult(); // sync release
            MEDIA_LOGI("Destructor Wait Release Task End");
        }
        generator->WaitTaskQueStop();
        delete generator;
    }
    MEDIA_LOGI("Destructor success");
}

napi_value AVImageGeneratorNapi::JsCreateAVImageGenerator(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVImageGeneratorNapi::JsCreateAVImageGenerator");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsCreateAVImageGenerator In");

    std::unique_ptr<MediaAsyncContext> asyncContext = std::make_unique<MediaAsyncContext>(env);

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, nullptr, "failed to napi_get_cb_info");

    asyncContext->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncContext->deferred = CommonNapi::CreatePromise(env, asyncContext->callbackRef, result);
    asyncContext->JsResult = std::make_unique<MediaJsResultInstance>(constructor_);
    asyncContext->ctorFlag = true;

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsCreateAVImageGenerator", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void *data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    MEDIA_LOGI("JsCreateAVImageGenerator Out");
    return result;
}

std::shared_ptr<TaskHandler<TaskRet>> AVImageGeneratorNapi::FetchFrameAtTimeTask(
    std::unique_ptr<AVImageGeneratorAsyncContext> &promiseCtx)
{
    auto task = std::make_shared<TaskHandler<TaskRet>>(
        [this, &napi = promiseCtx->napi, &pixelMap = promiseCtx->pixel_]() {
        MEDIA_LOGI("FetchFrameAtTime Task In");
        std::unique_lock<std::mutex> lock(taskMutex_);
        auto state = GetCurrentState();
        if (state == AVMetadataHelperState::STATE_PREPARED || state == AVMetadataHelperState::STATE_CALL_DONE) {
            auto map = helper_->FetchFrameAtTime(napi->timeUs_, napi->option_, napi->param_);
            if (map == nullptr) {
                MEDIA_LOGE("FetchFrameAtTime Task pixelMap is nullptr");
                return TaskRet(MSERR_EXT_API9_UNSUPPORT_FORMAT,
                    "failed to FetchFrameAtTime, pixelMap is nullptr!");
            }
            MEDIA_LOGI("FetchFrameAtTime Task end size: %{public}d", map->GetByteCount());
            pixelMap = map;

            stopWait_ = false;
            LISTENER(stateChangeCond_.wait(lock, [this]() { return stopWait_.load(); }),
                "FetchFrameAtTimeTask", false)

            if (GetCurrentState() == AVMetadataHelperState::STATE_ERROR) {
                return TaskRet(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
                    "failed to FetchFrameAtTime, metadata helper enter error status!");
            }
        } else {
            return TaskRet(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
                "current state is not initialized, unsupport FetchFrameAtTime operation");
        }

        MEDIA_LOGI("FetchFrameAtTime Task Out");
        return TaskRet(MSERR_EXT_API9_OK, "Success");
    });

    (void)taskQue_->EnqueueTask(task);
    return task;
}

int32_t AVImageGeneratorNapi::GetFetchFrameArgs(std::unique_ptr<AVImageGeneratorAsyncContext> &asyncCtx,
    napi_env env, napi_value timeUs, napi_value option, napi_value params)
{
    napi_status ret = napi_get_value_int64(env, timeUs, &asyncCtx->napi->timeUs_);
    if (ret != napi_ok) {
        asyncCtx->SignError(MSERR_INVALID_VAL, "failed to get timeUs");
        return MSERR_INVALID_VAL;
    }

    ret = napi_get_value_int32(env, option, &asyncCtx->napi->option_);
    if (ret != napi_ok) {
        asyncCtx->SignError(MSERR_INVALID_VAL, "failed to get option");
        return MSERR_INVALID_VAL;
    }

    int32_t width = -1;
    if (!CommonNapi::GetPropertyInt32(env, params, "width", width)) {
        MEDIA_LOGW("failed to get width");
    }

    int32_t height = -1;
    if (!CommonNapi::GetPropertyInt32(env, params, "height", height)) {
        MEDIA_LOGW("failed to get height");
    }

    PixelFormat colorFormat = PixelFormat::RGBA_8888;
    int32_t formatVal = 3;
    if (!CommonNapi::GetPropertyInt32(env, params, "colorFormat", formatVal)) {
        MEDIA_LOGW("failed to get colorFormat");
    }
    if (formatVal != static_cast<int32_t>(PixelFormat::RGB_565)
        && formatVal != static_cast<int32_t>(PixelFormat::RGBA_8888)
        && formatVal != static_cast<int32_t>(PixelFormat::RGB_888)) {
        asyncCtx->SignError(MSERR_INVALID_VAL, "formatVal is invalid");
        return MSERR_INVALID_VAL;
    }
    colorFormat = static_cast<PixelFormat>(formatVal);

    asyncCtx->napi->param_.dstWidth = width;
    asyncCtx->napi->param_.dstHeight = height;
    asyncCtx->napi->param_.colorFormat = colorFormat;
    return MSERR_OK;
}

bool AVImageGeneratorNapi::CheckSystemApp(napi_env env)
{
    uint64_t tokenId = IPCSkeleton::GetSelfTokenID();
    int32_t errorCode = 202; // General errorCode for system api permission denied.
    if (!Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(tokenId)) {
        std::string errorMessage = "System api can be invoked only by system applications";
        if (napi_throw_error(env, std::to_string(errorCode).c_str(), errorMessage.c_str()) != napi_ok) {
            MEDIA_LOGE("failed to throw err, code=%{public}d, msg=%{public}s.", errorCode, errorMessage.c_str());
        }
        return false;
    }
    return true;
}

napi_value AVImageGeneratorNapi::JsFetchFrameAtTime(napi_env env, napi_callback_info info)
{
    MEDIA_LOGI("JsFetchFrameAtTime  in");
    const int32_t maxParam = 4; // config + callbackRef
    const int32_t callbackParam = 3; // callback
    size_t argCount = maxParam;
    napi_value args[maxParam] = { nullptr };
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    auto asyncCtx = std::make_unique<AVImageGeneratorAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get AsyncContext");
    asyncCtx->napi = AVImageGeneratorNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceWithParameter");
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi->taskQue_ != nullptr, result, "taskQue is nullptr!");
    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[callbackParam]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    napi_valuetype valueType = napi_undefined;
    if (argCount < callbackParam || napi_typeof(env, args[2], &valueType) != napi_ok // 2 is the second arg.
        || valueType != napi_object) {
        asyncCtx->SignError(MSERR_EXT_API9_INVALID_PARAMETER, "JsFetchFrameAtTime");
    } else {
        if (asyncCtx->napi->GetFetchFrameArgs(
            asyncCtx, env, args[0], args[1], args[2]) == MSERR_OK) { // 2 is the second arg.
            asyncCtx->task_ = asyncCtx->napi->FetchFrameAtTimeTask(asyncCtx);
        } else {
            asyncCtx->SignError(MSERR_INVALID_VAL, "failed to get args");
        }
    }
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsFetchFrameAtTime", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
        AVImageGeneratorAsyncContext* asyncCtx = reinterpret_cast<AVImageGeneratorAsyncContext *>(data);
        CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "asyncCtx is nullptr!");
        if (asyncCtx->task_) {
            auto result = asyncCtx->task_->GetResult();
            MEDIA_LOGI("FetchFrameAtTime get result end");
            if (result.Value().first != MSERR_EXT_API9_OK) {
                asyncCtx->SignError(result.Value().first, result.Value().second);
            }
        }
        MEDIA_LOGI("The js thread of fetch frame at time finishes execution and returns");
    }, CreatePixelMapComplete, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();
    MEDIA_LOGI("JsFetchFrameAtTime Out");
    return result;
}

void AVImageGeneratorNapi::CreatePixelMapComplete(napi_env env, napi_status status, void *data)
{
    napi_value result = nullptr;

    MEDIA_LOGI("CreatePixelMapComplete In");
    auto context = static_cast<AVImageGeneratorAsyncContext*>(data);

    if (status == napi_ok && context->errCode == napi_ok) {
        MEDIA_LOGI("set pixel map success");
        context->status = MSERR_OK;
        result = Media::PixelMapNapi::CreatePixelMap(env, context->pixel_);
    } else {
        context->status = context->errCode == napi_ok ? MSERR_INVALID_VAL : context->errCode;
        MEDIA_LOGW("set pixel map failed");
        napi_get_undefined(env, &result);
    }

    CommonCallbackRoutine(env, context, result);
}

void AVImageGeneratorNapi::CommonCallbackRoutine(napi_env env, AVImageGeneratorAsyncContext* &asyncContext,
    const napi_value &valueParam)
{
    napi_value result[2] = {0};
    napi_value retVal;
    napi_value callback = nullptr;

    napi_get_undefined(env, &result[0]);
    napi_get_undefined(env, &result[1]);

    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(env, &scope);
    if (scope == nullptr) {
        MEDIA_LOGW("scope is null");
        return;
    }

    if (asyncContext == nullptr) {
        MEDIA_LOGW("asyncContext is null");
        return;
    }

    if (asyncContext->status == ERR_OK) {
        result[1] = valueParam;
        napi_create_uint32(env, asyncContext->status, &result[0]);
    } else {
        napi_create_uint32(env, asyncContext->status, &result[0]);
    }

    if (asyncContext->deferred) {
        MEDIA_LOGI("deferred in");
        if (asyncContext->status == ERR_OK) {
            napi_resolve_deferred(env, asyncContext->deferred, result[1]);
        } else {
            napi_reject_deferred(env, asyncContext->deferred, result[0]);
        }
    } else {
        MEDIA_LOGI("callback in");
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, 2, result, &retVal); // 2 is the second arg.
        napi_delete_reference(env, asyncContext->callbackRef);
    }

    napi_delete_async_work(env, asyncContext->work);
    napi_close_handle_scope(env, scope);

    delete asyncContext;
    asyncContext = nullptr;
}

void AVImageGeneratorNapi::WaitTaskQueStop()
{
    MEDIA_LOGI("WaitTaskQueStop In");
    std::unique_lock<std::mutex> lock(taskMutex_);
    LISTENER(stopTaskQueCond_.wait(lock, [this]() { return taskQueStoped_; }), "StopTaskQue", false)
    MEDIA_LOGI("WaitTaskQueStop Out");
}

void AVImageGeneratorNapi::StopTaskQue()
{
    MEDIA_LOGI("StopTaskQue In");
    taskQue_->Stop();
    taskQueStoped_ = true;
    stopTaskQueCond_.notify_all();
    MEDIA_LOGI("StopTaskQue Out");
}

std::shared_ptr<TaskHandler<TaskRet>> AVImageGeneratorNapi::ReleaseTask()
{
    std::shared_ptr<TaskHandler<TaskRet>> task = nullptr;
    if (isReleased_.load()) {
        MEDIA_LOGE("Instance is released.");
        return task;
    }

    task = std::make_shared<TaskHandler<TaskRet>>([this]() {
        MEDIA_LOGI("Release Task In");
        PauseListenCurrentResource(); // Pause event listening for the current resource
        ResetUserParameters();

        if (helper_ != nullptr) {
            (void)helper_->Release();
            helper_ = nullptr;
        }

        if (generatorCb_ != nullptr) {
            generatorCb_->Release();
        }

        std::thread(&AVImageGeneratorNapi::StopTaskQue, this).detach();

        MEDIA_LOGI("Release Task Out");
        return TaskRet(MSERR_EXT_API9_OK, "Success");
    });

    std::unique_lock<std::mutex> lock(taskMutex_);
    isReleased_.store(true);
    (void)taskQue_->EnqueueTask(task, true); // CancelNotExecutedTask
    stopWait_ = true;
    stateChangeCond_.notify_all();
    return task;
}

napi_value AVImageGeneratorNapi::JsRelease(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVImageGeneratorNapi::release");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsRelease In");

    auto promiseCtx = std::make_unique<AVImageGeneratorAsyncContext>(env);
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    AVImageGeneratorNapi *generator = AVImageGeneratorNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(generator != nullptr, result, "failed to GetJsInstance");
    promiseCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    promiseCtx->deferred = CommonNapi::CreatePromise(env, promiseCtx->callbackRef, result);

    promiseCtx->task_ = generator->ReleaseTask();

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsRelease", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void *data) {
            auto promiseCtx = reinterpret_cast<AVImageGeneratorAsyncContext *>(data);
            CHECK_AND_RETURN_LOG(promiseCtx != nullptr, "promiseCtx is nullptr!");
            if (promiseCtx->task_ == nullptr) {
                promiseCtx->SignError(MSERR_INVALID_VAL, "Async release task is invalid.");
                return;
            }

            auto result = promiseCtx->task_->GetResult();
            MEDIA_LOGD("Release task GetResult end.");
            if (result.HasResult() && result.Value().first != MSERR_EXT_API9_OK) {
                promiseCtx->SignError(result.Value().first, result.Value().second);
                return;
            }
            MEDIA_LOGI("Release task completed.");
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(promiseCtx.get()), &promiseCtx->work));
    napi_queue_async_work_with_qos(env, promiseCtx->work, napi_qos_user_initiated);
    promiseCtx.release();
    MEDIA_LOGI("JsRelease Out");
    return result;
}

void AVImageGeneratorNapi::SetSource(std::string url)
{
    MEDIA_LOGI("input url is %{public}s!", url.c_str());
    bool isFd = (url.find("fd://") != std::string::npos) ? true : false;
    bool isNetwork = (url.find("http") != std::string::npos) ? true : false;
    if (isNetwork) {
        auto task = std::make_shared<TaskHandler<void>>([this, url]() {
            std::unique_lock<std::mutex> lock(taskMutex_);
            auto state = GetCurrentState();
            if (state != AVMetadataHelperState::STATE_IDLE) {
                OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not idle, unsupport set url");
                return;
            }
            if (helper_ != nullptr) {
                if (helper_->SetSource(url, AVMetadataUsage::AV_META_USAGE_PIXEL_MAP) != MSERR_OK) {
                    OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "failed to SetSourceNetWork");
                }
                stopWait_ = false;
                LISTENER(stateChangeCond_.wait(lock, [this]() { return stopWait_.load(); }),
                    "SetSourceNetWork", false)
            }
        });
        (void)taskQue_->EnqueueTask(task);
    } else if (isFd) {
        std::string inputFd = url.substr(sizeof("fd://") - 1);
        int32_t fd = -1;
        if (!StrToInt(inputFd, fd) || fd < 0) {
            OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "The input parameter is not a fd://+numeric string");
            return;
        }
        auto task = std::make_shared<TaskHandler<void>>([this, fd]() {
            std::unique_lock<std::mutex> lock(taskMutex_);
            auto state = GetCurrentState();
            if (state != AVMetadataHelperState::STATE_IDLE) {
                OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not idle, unsupport set source fd");
                return;
            }
            if (helper_ != nullptr) {
                if (helper_->SetSource(fd, 0, -1, AVMetadataUsage::AV_META_USAGE_PIXEL_MAP) != MSERR_OK) {
                    OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "failed to SetSourceFd");
                }
                stopWait_ = false;
                LISTENER(stateChangeCond_.wait(lock, [this]() { return stopWait_.load(); }), "SetSourceFd", false)
            }
        });
        (void)taskQue_->EnqueueTask(task);
    } else {
        OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "The input parameter is not fd:// or network address");
    }
}

napi_value AVImageGeneratorNapi::JsSetUrl(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVImageGeneratorNapi::set url");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsSetUrl In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1; // url: string
    AVImageGeneratorNapi *generator = AVImageGeneratorNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(generator != nullptr, result, "failed to GetJsInstanceWithParameter");

    if (generator->GetCurrentState() != AVMetadataHelperState::STATE_IDLE) {
        generator->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not idle, unsupport set url");
        return result;
    }

    napi_valuetype valueType = napi_undefined;
    if (argCount < 1 || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_string) {
        generator->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "url is not string");
        return result;
    }

    // get url from js
    generator->url_ = CommonNapi::GetStringArgument(env, args[0]);
    generator->SetSource(generator->url_);

    MEDIA_LOGI("JsSetUrl Out");
    return result;
}

napi_value AVImageGeneratorNapi::JsGetUrl(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVImageGeneratorNapi::get url");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsGetUrl In");

    AVImageGeneratorNapi *generator = AVImageGeneratorNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(generator != nullptr, result, "failed to GetJsInstance");

    napi_value value = nullptr;
    (void)napi_create_string_utf8(env, generator->url_.c_str(), NAPI_AUTO_LENGTH, &value);

    MEDIA_LOGI("JsGetUrl Out Current Url: %{public}s", generator->url_.c_str());
    return value;
}

void AVImageGeneratorNapi::SetAVFileDescriptorTask(std::shared_ptr<AVMetadataHelper>& avHelper,
    AVFileDescriptor& fileDescriptor)
{
    auto task = std::make_shared<TaskHandler<void>>([this, &helper_ = avHelper, &fileDescriptor_ = fileDescriptor]() {
        MEDIA_LOGI("SetAVFileDescriptor Task");
        std::unique_lock<std::mutex> lock(taskMutex_);
        auto state = GetCurrentState();
        if (state != AVMetadataHelperState::STATE_IDLE) {
            OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not idle, unsupport set source fd");
            return;
        }

        if (helper_ != nullptr) {
            if (helper_->SetSource(fileDescriptor_.fd, fileDescriptor_.offset, fileDescriptor_.length,
                AVMetadataUsage::AV_META_USAGE_PIXEL_MAP) != MSERR_OK) {
                MEDIA_LOGE("Helper SetSource FileDescriptor failed");
            }
            stopWait_ = false;
            LISTENER(stateChangeCond_.wait(lock, [this]() { return stopWait_.load(); }),
                "SetSource FileDescriptor", false)
        }
        MEDIA_LOGI("SetSource FileDescriptor end");
    });
    (void)taskQue_->EnqueueTask(task);
}

napi_value AVImageGeneratorNapi::JsSetAVFileDescriptor(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVImageGeneratorNapi::set fd");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsSetAVFileDescriptor In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1; // url: string
    AVImageGeneratorNapi *generator = AVImageGeneratorNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(generator != nullptr, result, "failed to GetJsInstanceWithParameter");

    if (generator->GetCurrentState() != AVMetadataHelperState::STATE_IDLE) {
        generator->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not idle, unsupport set fd");
        return result;
    }

    generator->StartListenCurrentResource(); // Listen to the events of the current resource
    napi_valuetype valueType = napi_undefined;
    if (argCount < 1 || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_object) {
        generator->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "SetAVFileDescriptor is not napi_object");
        return result;
    }

    if (!CommonNapi::GetFdArgument(env, args[0], generator->fileDescriptor_)) {
        MEDIA_LOGE("get fileDescriptor argument failed!");
        generator->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER,
            "invalid parameters, please check the input parameters(fileDescriptor)");
        return result;
    }
    generator->SetAVFileDescriptorTask(generator->helper_, generator->fileDescriptor_);
    MEDIA_LOGI("JsSetAVFileDescriptor Out");
    return result;
}

napi_value AVImageGeneratorNapi::JsGetAVFileDescriptor(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVImageGeneratorNapi::get fd");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsGetAVFileDescriptor In");

    AVImageGeneratorNapi *generator = AVImageGeneratorNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(generator != nullptr, result, "failed to GetJsInstance");

    napi_value value = nullptr;
    (void)napi_create_object(env, &value);
    (void)CommonNapi::AddNumberPropInt32(env, value, "fd", generator->fileDescriptor_.fd);
    (void)CommonNapi::AddNumberPropInt64(env, value, "offset", generator->fileDescriptor_.offset);
    (void)CommonNapi::AddNumberPropInt64(env, value, "length", generator->fileDescriptor_.length);

    MEDIA_LOGI("JsGetAVFileDescriptor Out");
    return value;
}

std::string AVImageGeneratorNapi::GetCurrentState()
{
    if (isReleased_.load()) {
        return AVMetadataHelperState::STATE_RELEASED;
    } else {
        std::string curState = AVMetadataHelperState::STATE_ERROR;

        if (stateMap.find(state_) != stateMap.end()) {
            curState = stateMap.at(state_);
        }
        return curState;
    }
}

void AVImageGeneratorNapi::SaveCallbackReference(const std::string &callbackName, std::shared_ptr<AutoRef> ref)
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_[callbackName] = ref;
    if (generatorCb_ != nullptr) {
        generatorCb_->SaveCallbackReference(callbackName, ref);
    }
}

void AVImageGeneratorNapi::ClearCallbackReference()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (generatorCb_ != nullptr) {
        generatorCb_->ClearCallbackReference();
    }
    refMap_.clear();
}

void AVImageGeneratorNapi::ClearCallbackReference(const std::string &callbackName)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (generatorCb_ != nullptr) {
        generatorCb_->ClearCallbackReference(callbackName);
    }
    refMap_.erase(callbackName);
}

void AVImageGeneratorNapi::NotifyState(HelperStates state)
{
    std::lock_guard<std::mutex> lock(taskMutex_);
    state_ = state;
    MEDIA_LOGI("notify completed, current state: %{public}s", GetCurrentState().c_str());
    stopWait_ = true;
    stateChangeCond_.notify_all();
}

void AVImageGeneratorNapi::ResetUserParameters()
{
    url_.clear();
    fileDescriptor_.fd = 0;
    fileDescriptor_.offset = 0;
    fileDescriptor_.length = -1;
    fileDescriptor_.length = -1;
}

void AVImageGeneratorNapi::StartListenCurrentResource()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (generatorCb_ != nullptr) {
        generatorCb_->Start();
    }
}

void AVImageGeneratorNapi::PauseListenCurrentResource()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (generatorCb_ != nullptr) {
        generatorCb_->Pause();
    }
}

void AVImageGeneratorNapi::OnErrorCb(MediaServiceExtErrCodeAPI9 errorCode, const std::string &errorMsg)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (generatorCb_ != nullptr) {
        generatorCb_->OnErrorCb(errorCode, errorMsg);
    }
}

AVImageGeneratorNapi* AVImageGeneratorNapi::GetJsInstance(napi_env env, napi_callback_info info)
{
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, nullptr, "failed to napi_get_cb_info");

    AVImageGeneratorNapi *generator = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&generator));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && generator != nullptr, nullptr, "failed to napi_unwrap");

    return generator;
}

AVImageGeneratorNapi* AVImageGeneratorNapi::GetJsInstanceWithParameter(napi_env env, napi_callback_info info,
    size_t &argc, napi_value *argv)
{
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, argv, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, nullptr, "failed to napi_get_cb_info");

    AVImageGeneratorNapi *generator = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&generator));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && generator != nullptr, nullptr, "failed to napi_unwrap");

    return generator;
}
} // namespace Media
} // namespace OHOS