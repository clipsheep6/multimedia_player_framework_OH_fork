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

#include "avmetadataextractor_napi.h"
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
#if !defined(ANDROID_PLATFORM) && !defined(IOS_PLATFORM)
#include "ipc_skeleton.h"
#include "tokenid_kit.h"
#endif

using namespace OHOS::AudioStandard;

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetadataExtractorNapi"};
}

namespace OHOS {
namespace Media {
static const std::map<HelperStates, std::string> stateMap = {
    {HELPER_IDLE, AVMetadataHelperState::STATE_IDLE},
    {HELPER_PREPARED, AVMetadataHelperState::STATE_PREPARED},
    {HELPER_RELEASED, AVMetadataHelperState::STATE_RELEASED},
    {HELPER_CALL_DONE, AVMetadataHelperState::STATE_CALL_DONE},
};

thread_local napi_ref AVMetadataExtractorNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "AVMetadataExtractor";

AVMetadataExtractorNapi::AVMetadataExtractorNapi()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVMetadataExtractorNapi::~AVMetadataExtractorNapi()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value AVMetadataExtractorNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAVMetadataExtractor", JsCreateAVMetadataExtractor),
    };

    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("fetchMetadata", JsResolveMetadata),
        DECLARE_NAPI_FUNCTION("fetchAlbumCover", JsFetchArtPicture),
        DECLARE_NAPI_FUNCTION("release", JsRelease),

        DECLARE_NAPI_GETTER_SETTER("url", JsGetUrl, JsSetUrl),
        DECLARE_NAPI_GETTER_SETTER("fdSrc", JsGetAVFileDescriptor, JsSetAVFileDescriptor),
        DECLARE_NAPI_GETTER_SETTER("dataSrc", JsGetDataSrc, JsSetDataSrc),
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define AVMetadataHelper class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value AVMetadataExtractorNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "failed to napi_get_cb_info");

    AVMetadataExtractorNapi *extractor = new(std::nothrow) AVMetadataExtractorNapi();
    CHECK_AND_RETURN_RET_LOG(extractor != nullptr, result, "failed to new AVMetadataExtractorNapi");

    extractor->env_ = env;
    extractor->helper_ = AVMetadataHelperFactory::CreateAVMetadataHelper();
    CHECK_AND_RETURN_RET_LOG(extractor->helper_ != nullptr, result, "failed to CreateMetadataHelper");

    extractor->taskQue_ = std::make_unique<TaskQueue>("AVMetadataExtractorNapi");
    (void)extractor->taskQue_->Start();

    extractor->extractorCb_ = std::make_shared<AVMetadataHelperCallback>(env, extractor);
    (void)extractor->helper_->SetHelperCallback(extractor->extractorCb_);

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(extractor),
        AVMetadataExtractorNapi::Destructor, nullptr, nullptr);
    if (status != napi_ok) {
        delete extractor;
        MEDIA_LOGE("Failed to wrap native instance");
        return result;
    }

    MEDIA_LOGI("Constructor success");
    return jsThis;
}

void AVMetadataExtractorNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)finalize;
    if (nativeObject != nullptr) {
        AVMetadataExtractorNapi *extractor = reinterpret_cast<AVMetadataExtractorNapi *>(nativeObject);
        auto task = extractor->ReleaseTask();
        if (task != nullptr) {
            MEDIA_LOGI("Destructor Wait Release Task Start");
            task->GetResult(); // sync release
            MEDIA_LOGI("Destructor Wait Release Task End");
        }
        extractor->WaitTaskQueStop();
        delete extractor;
    }
    MEDIA_LOGI("Destructor success");
}

napi_value AVMetadataExtractorNapi::JsCreateAVMetadataExtractor(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVMetadataExtractorNapi::JsCreateAVMetadataExtractor");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsCreateAVMetadataExtractor In");

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
    napi_create_string_utf8(env, "JsCreateAVMetadataExtractor", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void *data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    MEDIA_LOGI("JsCreateAVMetadataExtractor Out");
    return result;
}

std::shared_ptr<TaskHandler<TaskRet>> AVMetadataExtractorNapi::ResolveMetadataTask(
    std::unique_ptr<AVMetadataExtractorAsyncContext> &promiseCtx)
{
    auto task = std::make_shared<TaskHandler<TaskRet>>([this, &metadata = promiseCtx->metadata_]() {
        MEDIA_LOGI("ResolveMetadata Task In");
        std::unique_lock<std::mutex> lock(taskMutex_);
        auto state = GetCurrentState();
        if (state == AVMetadataHelperState::STATE_PREPARED || state == AVMetadataHelperState::STATE_CALL_DONE) {
            std::shared_ptr<Meta> res = helper_->GetAVMetadata();
            if (res == nullptr) {
                MEDIA_LOGE("ResolveMetadata Task AVMetadata is nullptr");
                return TaskRet(MSERR_EXT_API9_UNSUPPORT_FORMAT,
                    "failed to ResolveMetadata Task, AVMetadata is nullptr!");
            }
            MEDIA_LOGD("ResolveMetadata Task end");
            metadata = res;

            stopWait_ = false;
            LISTENER(stateChangeCond_.wait(lock, [this]() { return stopWait_.load(); }), "ResolveMetadataTask", false)

            if (GetCurrentState() == AVMetadataHelperState::STATE_ERROR) {
                return TaskRet(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
                    "failed to resolve metadata, metadata helper enter error status!");
            }
        } else {
            return TaskRet(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
                "current state is not initialized, unsupport resolve metadata operation");
        }

        MEDIA_LOGI("ResolveMetadata Task Out");
        return TaskRet(MSERR_EXT_API9_OK, "Success");
    });

    (void)taskQue_->EnqueueTask(task);
    return task;
}

napi_value AVMetadataExtractorNapi::JsResolveMetadata(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVMetadataExtractorNapi::resolveMetadata");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsResolveMetadata In");

    auto promiseCtx = std::make_unique<AVMetadataExtractorAsyncContext>(env);
    napi_value args[1] = { nullptr };
    size_t argCount = 1;

    AVMetadataExtractorNapi* extractor
        = AVMetadataExtractorNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(extractor != nullptr, result, "failed to GetJsInstance");
    promiseCtx->napi = extractor;
    promiseCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    promiseCtx->deferred = CommonNapi::CreatePromise(env, promiseCtx->callbackRef, result);

    promiseCtx->task_ = extractor->ResolveMetadataTask(promiseCtx);

    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsResolveMetadata", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void *data) {
            auto promiseCtx = reinterpret_cast<AVMetadataExtractorAsyncContext *>(data);
            CHECK_AND_RETURN_LOG(promiseCtx != nullptr, "promiseCtx is nullptr!");

            if (promiseCtx->task_) {
                auto result = promiseCtx->task_->GetResult();
                MEDIA_LOGD("JsResolveMetadata get result end");
                if (result.Value().first != MSERR_EXT_API9_OK) {
                    MEDIA_LOGE("JsResolveMetadata get result SignError");
                    promiseCtx->SignError(result.Value().first, result.Value().second);
                }
            }
            MEDIA_LOGI("The js thread of resolving meta data finishes execution and returns");
        },
        ResolveMetadataComplete, static_cast<void *>(promiseCtx.get()), &promiseCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, promiseCtx->work));
    promiseCtx.release();
    MEDIA_LOGI("JsResolveMetadata Out");
    return result;
}

void AVMetadataExtractorNapi::ResolveMetadataComplete(napi_env env, napi_status status, void *data)
{
    MEDIA_LOGI("ResolveMetadataComplete In");
    auto promiseCtx = static_cast<AVMetadataExtractorAsyncContext*>(data);
    CHECK_AND_RETURN_LOG(promiseCtx != nullptr, "promiseCtx is nullptr!");

    bool ret = true;
    napi_value result = nullptr;
    napi_value location = nullptr;
    napi_value customInfo = nullptr;
    napi_create_object(env, &result);
    napi_create_object(env, &location);
    napi_create_object(env, &customInfo);
    std::shared_ptr<Meta> metadata = promiseCtx->metadata_;
    if (status != napi_ok || promiseCtx->errCode != napi_ok) {
        promiseCtx->status = promiseCtx->errCode == napi_ok ? MSERR_INVALID_VAL : promiseCtx->errCode;
        MEDIA_LOGI("Resolve meta data failed");
        napi_get_undefined(env, &result);
        CommonCallbackRoutine(env, promiseCtx, result);
        return;
    }
    for (const auto &key : g_Metadata) {
        if (metadata->Find(key) == metadata->end()) {
            MEDIA_LOGE("failed to find key: %{public}s", key.c_str());
            continue;
        }
        MEDIA_LOGE("success to find key: %{public}s", key.c_str());
        if (key == "latitude" || key == "longitude") {
            CHECK_AND_CONTINUE_LOG(CommonNapi::SetPropertyByValueType(env, location, metadata, key),
                "SetProperty failed, key: %{public}s", key.c_str());
            continue;
        }
        if (key == "customInfo") {
            std::shared_ptr<Meta> customData = std::make_shared<Meta>();
            ret = metadata->GetData(key, customData);
            CHECK_AND_CONTINUE_LOG(ret, "GetData failed, key %{public}s", key.c_str());
            for (auto iter = customData->begin(); iter != customData->end(); ++iter) {
                AnyValueType type = customData->GetValueType(iter->first);
                CHECK_AND_CONTINUE_LOG(type == AnyValueType::STRING, "key not string");
                CHECK_AND_CONTINUE_LOG(CommonNapi::SetPropertyByValueType(env, customInfo, customData, iter->first),
                    "SetProperty failed, key: %{public}s", key.c_str());
            }
            continue;
        }
        CHECK_AND_CONTINUE_LOG(CommonNapi::SetPropertyByValueType(env, result, metadata, key),
            "SetProperty failed, key: %{public}s", key.c_str());
    }
    napi_set_named_property(env, result, "location", location);
    napi_set_named_property(env, result, "customInfo", customInfo);
    promiseCtx->status = ERR_OK;

    CommonCallbackRoutine(env, promiseCtx, result);
}

std::shared_ptr<TaskHandler<TaskRet>> AVMetadataExtractorNapi::FetchArtPictureTask(
    std::unique_ptr<AVMetadataExtractorAsyncContext> &promiseCtx)
{
    auto task = std::make_shared<TaskHandler<TaskRet>>(
        [this, &napi = promiseCtx->napi, &picture = promiseCtx->artPicture_]() {
        MEDIA_LOGI("FetchArtPicture Task In");
        std::unique_lock<std::mutex> lock(taskMutex_);
        auto state = GetCurrentState();
        if (state == AVMetadataHelperState::STATE_PREPARED || state == AVMetadataHelperState::STATE_CALL_DONE) {
            auto mem = helper_->FetchArtPicture();
            if (mem == nullptr) {
                return TaskRet(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
                    "FetchArtPicture result is nullptr.");
            }
            MEDIA_LOGD("FetchArtPicture Task end size: %{public}d", mem->GetSize());
            SourceOptions options;
            uint32_t errCode;
            std::unique_ptr<ImageSource> imageSource
                = ImageSource::CreateImageSource(mem->GetBase(), mem->GetSize(), options, errCode);
            if (imageSource == nullptr) {
                return TaskRet(MSERR_EXT_API9_INVALID_PARAMETER,
                    "Create image source failed.");
            }

            DecodeOptions decodeParam;
            std::unique_ptr<PixelMap> pixelMap = imageSource->CreatePixelMap(0, decodeParam, errCode);
            if (pixelMap == nullptr) {
                return TaskRet(MSERR_EXT_API9_INVALID_PARAMETER,
                    "Create pixel map failed.");
            }
            picture = std::move(pixelMap);

            stopWait_ = false;
            LISTENER(stateChangeCond_.wait(lock, [this]() { return stopWait_.load(); }), "FetchArtPictureTask", false)

            if (GetCurrentState() == AVMetadataHelperState::STATE_ERROR) {
                return TaskRet(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
                    "failed to FetchArtPicture, metadata helper enter error status!");
            }
        } else {
            return TaskRet(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
                "current state is not initialized, unsupport FetchArtPicture operation");
        }

        MEDIA_LOGI("FetchArtPicture Task Out");
        return TaskRet(MSERR_EXT_API9_OK, "Success");
    });

    (void)taskQue_->EnqueueTask(task);
    return task;
}

napi_value AVMetadataExtractorNapi::JsFetchArtPicture(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVMetadataExtractorNapi::fetchArtPicture");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsFetchArtPicture In");

    auto promiseCtx = std::make_unique<AVMetadataExtractorAsyncContext>(env);
    napi_value args[1] = { nullptr };
    size_t argCount = 1;

    AVMetadataExtractorNapi* extractor
        = AVMetadataExtractorNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(extractor != nullptr, result, "failed to GetJsInstance");
    promiseCtx->napi = extractor;
    promiseCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    promiseCtx->deferred = CommonNapi::CreatePromise(env, promiseCtx->callbackRef, result);

    promiseCtx->task_ = extractor->FetchArtPictureTask(promiseCtx);

    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsFetchArtPicture", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void *data) {
            MEDIA_LOGI("JsFetchArtPicture task start");
            auto promiseCtx = reinterpret_cast<AVMetadataExtractorAsyncContext *>(data);
            CHECK_AND_RETURN_LOG(promiseCtx != nullptr, "promiseCtx is nullptr!");

            if (promiseCtx->task_) {
                auto result = promiseCtx->task_->GetResult();
                MEDIA_LOGD("JsFetchArtPicture get result end");
                if (result.Value().first != MSERR_EXT_API9_OK) {
                    MEDIA_LOGI("JsFetchArtPicture get result SignError");
                    promiseCtx->SignError(result.Value().first, result.Value().second);
                }
            }
            MEDIA_LOGI("The js thread of resolving meta data finishes execution and returns");
        },
        FetchArtPictureComplete, static_cast<void *>(promiseCtx.get()), &promiseCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, promiseCtx->work));
    promiseCtx.release();
    MEDIA_LOGI("JsFetchArtPicture Out");
    return result;
}

void AVMetadataExtractorNapi::FetchArtPictureComplete(napi_env env, napi_status status, void *data)
{
    napi_value result = nullptr;

    MEDIA_LOGI("FetchArtPictureComplete In");
    auto context = static_cast<AVMetadataExtractorAsyncContext*>(data);

    if (status == napi_ok && context->errCode == napi_ok) {
        result = Media::PixelMapNapi::CreatePixelMap(env, context->artPicture_);
        context->status = ERR_OK;
    } else {
        context->status = context->errCode == napi_ok ? MSERR_INVALID_VAL : context->errCode;
        napi_get_undefined(env, &result);
    }

    CommonCallbackRoutine(env, context, result);
}

void AVMetadataExtractorNapi::CommonCallbackRoutine(napi_env env, AVMetadataExtractorAsyncContext* &asyncContext,
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
        MEDIA_LOGD("deferred in");
        if (asyncContext->status == ERR_OK) {
            napi_resolve_deferred(env, asyncContext->deferred, result[1]);
        } else {
            napi_reject_deferred(env, asyncContext->deferred, result[0]);
        }
    } else {
        MEDIA_LOGD("callback in");
        napi_get_reference_value(env, asyncContext->callbackRef, &callback);
        napi_call_function(env, nullptr, callback, 2, result, &retVal); // 2
        napi_delete_reference(env, asyncContext->callbackRef);
    }

    napi_delete_async_work(env, asyncContext->work);
    napi_close_handle_scope(env, scope);

    delete asyncContext;
    asyncContext = nullptr;
}

void AVMetadataExtractorNapi::WaitTaskQueStop()
{
    MEDIA_LOGI("WaitTaskQueStop In");
    std::unique_lock<std::mutex> lock(taskMutex_);
    LISTENER(stopTaskQueCond_.wait(lock, [this]() { return taskQueStoped_; }), "StopTaskQue", false)
    MEDIA_LOGI("WaitTaskQueStop Out");
}

void AVMetadataExtractorNapi::StopTaskQue()
{
    MEDIA_LOGI("StopTaskQue In");
    taskQue_->Stop();
    taskQueStoped_ = true;
    stopTaskQueCond_.notify_all();
    MEDIA_LOGI("StopTaskQue Out");
}

std::shared_ptr<TaskHandler<TaskRet>> AVMetadataExtractorNapi::ReleaseTask()
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

        if (extractorCb_ != nullptr) {
            extractorCb_->Release();
        }

        std::thread(&AVMetadataExtractorNapi::StopTaskQue, this).detach();

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

napi_value AVMetadataExtractorNapi::JsRelease(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVMetadataExtractorNapi::release");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsRelease In");

    auto promiseCtx = std::make_unique<AVMetadataExtractorAsyncContext>(env);
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    AVMetadataExtractorNapi *extractor
        = AVMetadataExtractorNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(extractor != nullptr, result, "failed to GetJsInstance");
    promiseCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    promiseCtx->deferred = CommonNapi::CreatePromise(env, promiseCtx->callbackRef, result);

    promiseCtx->task_ = extractor->ReleaseTask();
    if (extractor->dataSrcCb_ != nullptr) {
        extractor->dataSrcCb_->ClearCallbackReference();
        extractor->dataSrcCb_ = nullptr;
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsRelease", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void *data) {
            auto promiseCtx = reinterpret_cast<AVMetadataExtractorAsyncContext *>(data);
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

void AVMetadataExtractorNapi::SetSource(std::string url)
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
                helper_->SetSource(url);
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
                helper_->SetSource(fd, 0, -1);
                stopWait_ = false;
                LISTENER(stateChangeCond_.wait(lock, [this]() { return stopWait_.load(); }), "SetSourceFd", false)
            }
        });
        (void)taskQue_->EnqueueTask(task);
    } else {
        OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "The input parameter is not fd:// or network address");
    }
}

napi_value AVMetadataExtractorNapi::JsSetUrl(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVMetadataExtractorNapi::set url");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsSetUrl In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1; // url: string
    AVMetadataExtractorNapi *extractor
        = AVMetadataExtractorNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(extractor != nullptr, result, "failed to GetJsInstanceWithParameter");

    if (extractor->GetCurrentState() != AVMetadataHelperState::STATE_IDLE) {
        extractor->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not idle, unsupport set url");
        return result;
    }

    napi_valuetype valueType = napi_undefined;
    if (argCount < 1 || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_string) {
        extractor->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "url is not string");
        return result;
    }

    // get url from js
    extractor->url_ = CommonNapi::GetStringArgument(env, args[0]);
    extractor->SetSource(extractor->url_);

    MEDIA_LOGI("JsSetUrl Out");
    return result;
}

napi_value AVMetadataExtractorNapi::JsGetUrl(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVMetadataExtractorNapi::get url");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsGetUrl In");

    AVMetadataExtractorNapi *extractor = AVMetadataExtractorNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(extractor != nullptr, result, "failed to GetJsInstance");

    napi_value value = nullptr;
    (void)napi_create_string_utf8(env, extractor->url_.c_str(), NAPI_AUTO_LENGTH, &value);

    MEDIA_LOGI("JsGetUrl Out Current Url: %{public}s", extractor->url_.c_str());
    return value;
}

void AVMetadataExtractorNapi::SetAVFileDescriptorTask(std::shared_ptr<AVMetadataHelper>& avHelper,
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
            helper_->SetSource(fileDescriptor_.fd, fileDescriptor_.offset, fileDescriptor_.length);
            stopWait_ = false;
            LISTENER(stateChangeCond_.wait(lock, [this]() { return stopWait_.load(); }),
                "SetSource FileDescriptor", false)
        }
        MEDIA_LOGI("SetSource FileDescriptor end");
    });
    (void)taskQue_->EnqueueTask(task);
}

napi_value AVMetadataExtractorNapi::JsSetAVFileDescriptor(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVMetadataExtractorNapi::set fd");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsSetAVFileDescriptor In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1; // url: string
    AVMetadataExtractorNapi *extractor
        = AVMetadataExtractorNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(extractor != nullptr, result, "failed to GetJsInstanceWithParameter");

    if (extractor->GetCurrentState() != AVMetadataHelperState::STATE_IDLE) {
        extractor->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not idle, unsupport set fd");
        return result;
    }

    extractor->StartListenCurrentResource(); // Listen to the events of the current resource
    napi_valuetype valueType = napi_undefined;
    if (argCount < 1 || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_object) {
        extractor->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "SetAVFileDescriptor is not napi_object");
        return result;
    }

    if (!CommonNapi::GetFdArgument(env, args[0], extractor->fileDescriptor_)) {
        MEDIA_LOGE("get fileDescriptor argument failed!");
        extractor->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER,
            "invalid parameters, please check the input parameters(fileDescriptor)");
        return result;
    }
    extractor->SetAVFileDescriptorTask(extractor->helper_, extractor->fileDescriptor_);
    MEDIA_LOGI("JsSetAVFileDescriptor Out");
    return result;
}

napi_value AVMetadataExtractorNapi::JsGetAVFileDescriptor(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVMetadataExtractorNapi::get fd");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsGetAVFileDescriptor In");

    AVMetadataExtractorNapi *extractor = AVMetadataExtractorNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(extractor != nullptr, result, "failed to GetJsInstance");

    napi_value value = nullptr;
    (void)napi_create_object(env, &value);
    (void)CommonNapi::AddNumberPropInt32(env, value, "fd", extractor->fileDescriptor_.fd);
    (void)CommonNapi::AddNumberPropInt64(env, value, "offset", extractor->fileDescriptor_.offset);
    (void)CommonNapi::AddNumberPropInt64(env, value, "length", extractor->fileDescriptor_.length);

    MEDIA_LOGI("JsGetAVFileDescriptor Out");
    return value;
}

void AVMetadataExtractorNapi::SetDataSrcTask(std::shared_ptr<AVMetadataHelper>& avHelper,
    std::shared_ptr<HelperDataSourceCallback>& dataSrcCb)
{
    auto task = std::make_shared<TaskHandler<void>>([this, &helper_ = avHelper, &dataSrcCb_ = dataSrcCb]() {
        MEDIA_LOGI("SetDataSrc Task");
        std::unique_lock<std::mutex> lock(taskMutex_);
        auto state = GetCurrentState();
        if (state != AVMetadataHelperState::STATE_IDLE) {
            OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not idle, unsupport set data source");
            return;
        }

        if (helper_ != nullptr) {
            MEDIA_LOGI("SetDataSrc Task SetSource");
            helper_->SetSource(dataSrcCb_);

            stopWait_ = false;
            LISTENER(stateChangeCond_.wait(lock, [this]() { return stopWait_.load(); }), "Set data source", false)
        }
        MEDIA_LOGI("SetDataSrc Task SetSource end");
    });
    (void)taskQue_->EnqueueTask(task);
}

napi_value AVMetadataExtractorNapi::JsSetDataSrc(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVMetadataExtractorNapi::set dataSrc");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsSetDataSrc In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    AVMetadataExtractorNapi *jsMetaHelper
        = AVMetadataExtractorNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsMetaHelper != nullptr, result, "failed to GetJsInstanceWithParameter");

    if (jsMetaHelper->GetCurrentState() != AVMetadataHelperState::STATE_IDLE) {
        jsMetaHelper->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not idle, unsupport set dataSrc");
        return result;
    }
    jsMetaHelper->StartListenCurrentResource(); // Listen to the events of the current resource

    napi_valuetype valueType = napi_undefined;
    if (argCount < 1 || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_object) {
        jsMetaHelper->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "args[0] is not napi_object");
        return result;
    }
    (void)CommonNapi::GetPropertyInt64(env, args[0], "fileSize", jsMetaHelper->dataSrcDescriptor_.fileSize);
    if (jsMetaHelper->dataSrcDescriptor_.fileSize < -1 || jsMetaHelper->dataSrcDescriptor_.fileSize == 0) {
        jsMetaHelper->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER,
            "invalid parameters, please check parameter fileSize");
        return result;
    }
    MEDIA_LOGI("Recvive filesize is %{public}" PRId64 "", jsMetaHelper->dataSrcDescriptor_.fileSize);
    jsMetaHelper->dataSrcCb_
        = std::make_shared<HelperDataSourceCallback>(env, jsMetaHelper->dataSrcDescriptor_.fileSize);

    napi_value callback = nullptr;
    napi_ref ref = nullptr;
    napi_get_named_property(env, args[0], "callback", &callback);
    jsMetaHelper->dataSrcDescriptor_.callback = callback;
    napi_status status = napi_create_reference(env, callback, 1, &ref);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && ref != nullptr, result, "failed to create reference!");
    std::shared_ptr<AutoRef> autoRef = std::make_shared<AutoRef>(env, ref);
    jsMetaHelper->dataSrcCb_->SaveCallbackReference(HELPER_READAT_CALLBACK_NAME, autoRef);

    jsMetaHelper->SetDataSrcTask(jsMetaHelper->helper_, jsMetaHelper->dataSrcCb_);

    MEDIA_LOGI("JsSetDataSrc Out");
    return result;
}

napi_value AVMetadataExtractorNapi::JsGetDataSrc(napi_env env, napi_callback_info info)
{
    MediaTrace trace("AVMetadataExtractorNapi::get dataSrc");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsGetDataSrc In");

    AVMetadataExtractorNapi *jsMetaHelper = AVMetadataExtractorNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsMetaHelper != nullptr, result, "failed to GetJsInstance");
    CHECK_AND_RETURN_RET_LOG(jsMetaHelper->dataSrcCb_ != nullptr, result, "failed to check dataSrcCb_");

    napi_value value = nullptr;
    int64_t fileSize;
    napi_value callback = nullptr;
    (void)napi_create_object(env, &value);
    (void)jsMetaHelper->dataSrcCb_->GetSize(fileSize);
    (void)CommonNapi::AddNumberPropInt64(env, value, "fileSize", fileSize);
    int32_t ret = jsMetaHelper->dataSrcCb_->GetCallback(HELPER_READAT_CALLBACK_NAME, &callback);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, result, "failed to GetCallback");
    (void)HelperDataSourceCallback::AddNapiValueProp(env, value, "callback", callback);

    MEDIA_LOGI("JsGetDataSrc Out");
    return value;
}

std::string AVMetadataExtractorNapi::GetCurrentState()
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

void AVMetadataExtractorNapi::SaveCallbackReference(const std::string &callbackName, std::shared_ptr<AutoRef> ref)
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_[callbackName] = ref;
    if (extractorCb_ != nullptr) {
        extractorCb_->SaveCallbackReference(callbackName, ref);
    }
}

void AVMetadataExtractorNapi::ClearCallbackReference()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (extractorCb_ != nullptr) {
        extractorCb_->ClearCallbackReference();
    }
    refMap_.clear();
}

void AVMetadataExtractorNapi::ClearCallbackReference(const std::string &callbackName)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (extractorCb_ != nullptr) {
        extractorCb_->ClearCallbackReference(callbackName);
    }
    refMap_.erase(callbackName);
}

void AVMetadataExtractorNapi::NotifyState(HelperStates state)
{
    std::lock_guard<std::mutex> lock(taskMutex_);
    state_ = state;
    MEDIA_LOGI("notify completed, current state: %{public}s", GetCurrentState().c_str());
    stopWait_ = true;
    stateChangeCond_.notify_all();
}

void AVMetadataExtractorNapi::ResetUserParameters()
{
    url_.clear();
    fileDescriptor_.fd = 0;
    fileDescriptor_.offset = 0;
    fileDescriptor_.length = -1;
    fileDescriptor_.length = -1;
}

void AVMetadataExtractorNapi::StartListenCurrentResource()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (extractorCb_ != nullptr) {
        extractorCb_->Start();
    }
}

void AVMetadataExtractorNapi::PauseListenCurrentResource()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (extractorCb_ != nullptr) {
        extractorCb_->Pause();
    }
}

void AVMetadataExtractorNapi::OnErrorCb(MediaServiceExtErrCodeAPI9 errorCode, const std::string &errorMsg)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (extractorCb_ != nullptr) {
        extractorCb_->OnErrorCb(errorCode, errorMsg);
    }
}

AVMetadataExtractorNapi* AVMetadataExtractorNapi::GetJsInstance(napi_env env, napi_callback_info info)
{
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, nullptr, "failed to napi_get_cb_info");

    AVMetadataExtractorNapi *extractor = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&extractor));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && extractor != nullptr, nullptr, "failed to napi_unwrap");

    return extractor;
}

AVMetadataExtractorNapi* AVMetadataExtractorNapi::GetJsInstanceWithParameter(napi_env env, napi_callback_info info,
    size_t &argc, napi_value *argv)
{
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, argv, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, nullptr, "failed to napi_get_cb_info");

    AVMetadataExtractorNapi *extractor = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&extractor));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && extractor != nullptr, nullptr, "failed to napi_unwrap");

    return extractor;
}
} // namespace Media
} // namespace OHOS