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

#include "media_errors.h"
#include "media_log.h"
#include "ability.h"
#include "napi_base_context.h"
#include "soundpool_napi.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SoundPoolNapi"};
}

namespace OHOS {
namespace Media {
int32_t SoundPoolNapi::maxStreams = 0;
AudioStandard::AudioRendererInfo SoundPoolNapi::rendererInfo;
thread_local napi_ref SoundPoolNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "SoundPool";

SoundPoolNapi::~SoundPoolNapi()
{
    MEDIA_LOGI("%s", __FUNCTION__);
}

napi_value SoundPoolNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createSoundPool", JsCreateSoundPool),
    };

    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("load", JsLoad),
        DECLARE_NAPI_FUNCTION("play", JsPlay),
        DECLARE_NAPI_FUNCTION("stop", JsStop),
        DECLARE_NAPI_FUNCTION("setLoop", JsSetLoop),
        DECLARE_NAPI_FUNCTION("setPriority", JsSetPriority),
        DECLARE_NAPI_FUNCTION("setRate", JsSetRate),
        DECLARE_NAPI_FUNCTION("setVolume", JsSetVolume),
        DECLARE_NAPI_FUNCTION("unload", JsUnload),
        DECLARE_NAPI_FUNCTION("release", JsRelease),
        DECLARE_NAPI_FUNCTION("on", JsSetOnCallback),
        DECLARE_NAPI_FUNCTION("off", JsClearOnCallback),
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define SoundPool class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value SoundPoolNapi::Constructor(napi_env env, napi_callback_info info)
{
    MEDIA_LOGI("Constructor enter");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "failed to napi_get_cb_info");

    SoundPoolNapi *soundPoolNapi = new(std::nothrow) SoundPoolNapi();
    CHECK_AND_RETURN_RET_LOG(soundPoolNapi != nullptr, result, "No memory!");

    soundPoolNapi->env_ = env;
    soundPoolNapi->soundPool_ = SoundPoolFactory::CreateSoundPool(maxStreams, rendererInfo);
    if (soundPoolNapi->soundPool_ == nullptr) {
        delete soundPoolNapi;
        MEDIA_LOGE("failed to CreateSoundPool");
        return result;
    }

    if (soundPoolNapi->callbackNapi_ == nullptr && soundPoolNapi->soundPool_ != nullptr) {
        soundPoolNapi->callbackNapi_ = std::make_shared<SoundPoolCallBackNapi>(env);
        (void)soundPoolNapi->soundPool_->SetSoundPoolCallback(soundPoolNapi->callbackNapi_);
    }

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(soundPoolNapi),
        SoundPoolNapi::Destructor, nullptr, nullptr);
    if (status != napi_ok) {
        delete soundPoolNapi;
        MEDIA_LOGE("Failed to warp native instance!");
        return result;
    }
    MEDIA_LOGI("Constructor success");
    return jsThis;
}

void SoundPoolNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        SoundPoolNapi *napi = reinterpret_cast<SoundPoolNapi *>(nativeObject);
        napi->callbackNapi_ = nullptr;

        if (napi->soundPool_) {
            napi->soundPool_->Release();
            napi->soundPool_ = nullptr;
        }
        delete napi;
    }
    MEDIA_LOGD("Destructor success");
}

napi_value SoundPoolNapi::JsCreateSoundPool(napi_env env, napi_callback_info info)
{
    MediaTrace trace("SoundPool::JsCreateSoundPool");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    // get args
    napi_value jsThis = nullptr;
    napi_value args[PARAM3] = { nullptr };
    size_t argCount = PARAM3;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "failed to napi_get_cb_info");

    // get create soundpool Parameter
    status = GetJsInstanceWithParameter(env, args);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "failed to Get InstanceWithParameter");

    std::unique_ptr<SoundPoolAsyncContext> asyncCtx = std::make_unique<SoundPoolAsyncContext>(env);

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[PARAM2]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    asyncCtx->JsResult = std::make_unique<MediaJsResultInstance>(constructor_);
    asyncCtx->ctorFlag = true;

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsCreateSoundPool", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
        MEDIA_LOGD("JsCreateSoundPool napi_create_async_work");
    }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    asyncCtx.release();

    return result;
}

napi_value SoundPoolNapi::JsLoad(napi_env env, napi_callback_info info)
{
    MediaTrace trace("SoundPool::JsLoad");
    size_t argCount = PARAM4;
    napi_value args[PARAM4] = { nullptr };
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<SoundPoolAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get SoundPoolAsyncContext");
    asyncCtx->napi = SoundPoolNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceAndArgs");

    if (argCount == PARAM4) {
        asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[PARAM3]);
    } else {
        asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[PARAM1]);
    }
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsLoad", NAPI_AUTO_LENGTH, &resource);
    if (asyncCtx->napi->ParserLoadOptionFromJs(asyncCtx, env, args, argCount) == MSERR_OK) {
        NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
            SoundPoolAsyncContext* asyncCtx = reinterpret_cast<SoundPoolAsyncContext *>(data);
            CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "asyncCtx is nullptr!");
            CHECK_AND_RETURN_LOG(asyncCtx->napi->soundPool_ != nullptr, "soundPool_ is nullptr!");
            int32_t soundId;
            if (asyncCtx->url_.empty()) {
                soundId = asyncCtx->napi->soundPool_->Load(asyncCtx->fd_, asyncCtx->offset_, asyncCtx->length_);
            } else {
                soundId = asyncCtx->napi->soundPool_->Load(asyncCtx->url_);
            }
            if (soundId < 0) {
                asyncCtx->SignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "load sound failed");
            } else {
                asyncCtx->JsResult = std::make_unique<MediaJsResultInt>(soundId);
            }
            MEDIA_LOGI("The js thread of load finishes execution and returns, soundId: %{public}d", soundId);
        }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    } else {
        NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
            MEDIA_LOGD("JsLoad napi_create_async_work");
        }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    }
    NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    asyncCtx.release();
    return result;
}

napi_value SoundPoolNapi::JsPlay(napi_env env, napi_callback_info info)
{
    MediaTrace trace("SoundPool::JsPlay");

    size_t argCount = PARAM3;
    napi_value args[PARAM3] = { nullptr };

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<SoundPoolAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get SoundPoolAsyncContext");
    asyncCtx->napi = SoundPoolNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceAndArgs");

    if (argCount == PARAM3) {
        asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[PARAM2]);
    } else {
        asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[PARAM1]);
    }
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsPlay", NAPI_AUTO_LENGTH, &resource);
    if (asyncCtx->napi->ParserPlayOptionFromJs(asyncCtx, env, args, argCount) == MSERR_OK) {
        NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
            SoundPoolAsyncContext* asyncCtx = reinterpret_cast<SoundPoolAsyncContext *>(data);
            CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "asyncCtx is nullptr!");
            CHECK_AND_RETURN_LOG(asyncCtx->napi->soundPool_ != nullptr, "soundPool_ is nullptr!");
            int32_t streamId = asyncCtx->napi->soundPool_->Play(asyncCtx->soundId_, asyncCtx->playParameters_);
            if (streamId < 0) {
                asyncCtx->SignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "play sound failed");
            } else {
                asyncCtx->JsResult = std::make_unique<MediaJsResultInt>(streamId);
            }
            MEDIA_LOGI("The js thread of play finishes execution and returns, streamId: %{public}d", streamId);
        }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    } else {
        NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
            MEDIA_LOGD("JsPlay napi_create_async_work");
        }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    }
    NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    asyncCtx.release();
    return result;
}

napi_value SoundPoolNapi::JsStop(napi_env env, napi_callback_info info)
{
    MediaTrace trace("SoundPool::JsStop");

    size_t argCount = PARAM2;
    napi_value args[PARAM2] = { nullptr };

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<SoundPoolAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get SoundPoolAsyncContext");
    asyncCtx->napi = SoundPoolNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceAndArgs");

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[PARAM1]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    napi_status status = napi_get_value_int32(env, args[PARAM0], &asyncCtx->streamId_);
    if (status != napi_ok || asyncCtx->streamId_ <= 0) {
        asyncCtx->SignError(MSERR_EXT_API9_INVALID_PARAMETER, "stop streamId failed, invaild value");
    }
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsStop", NAPI_AUTO_LENGTH, &resource);
    if (status == napi_ok && asyncCtx->streamId_ > 0) {
        NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
            SoundPoolAsyncContext* asyncCtx = reinterpret_cast<SoundPoolAsyncContext *>(data);
            CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "asyncCtx is nullptr!");
            CHECK_AND_RETURN_LOG(asyncCtx->napi->soundPool_ != nullptr, "soundPool_ is nullptr!");
            int32_t ret = asyncCtx->napi->soundPool_->Stop(asyncCtx->streamId_);
            if (ret != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "stop streamId failed");
            }
            MEDIA_LOGI("The js thread of stop finishes execution and returns");
        }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
        NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    } else {
        NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
            MEDIA_LOGD("JsStop napi_create_async_work");
        }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
        NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    }
    asyncCtx.release();

    return result;
}

napi_value SoundPoolNapi::JsSetLoop(napi_env env, napi_callback_info info)
{
    MediaTrace trace("SoundPool::JsSetLoop");

    size_t argCount = PARAM3;
    napi_value args[PARAM3] = { nullptr };

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<SoundPoolAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get SoundPoolAsyncContext");
    asyncCtx->napi = SoundPoolNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceAndArgs");

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[PARAM2]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    napi_status status = napi_get_value_int32(env, args[PARAM0], &asyncCtx->streamId_);
    if (status != napi_ok || asyncCtx->streamId_ <= 0) {
        asyncCtx->SignError(MSERR_EXT_API9_INVALID_PARAMETER, "SetLoop streamId failed,invaild value");
    }
    status = napi_get_value_int32(env, args[PARAM1], &asyncCtx->loop_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "failed to setloop id");

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsSetLoop", NAPI_AUTO_LENGTH, &resource);
    if (status == napi_ok && asyncCtx->streamId_ > 0) {
        NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
            SoundPoolAsyncContext* asyncCtx = reinterpret_cast<SoundPoolAsyncContext *>(data);
            CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "asyncCtx is nullptr!");
            CHECK_AND_RETURN_LOG(asyncCtx->napi->soundPool_ != nullptr, "soundPool_ is nullptr!");
            int32_t ret = asyncCtx->napi->soundPool_->SetLoop(asyncCtx->streamId_, asyncCtx->loop_);
            if (ret != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "setLoop streamId failed");
            }
            MEDIA_LOGI("The js thread of SetLoop finishes execution and returns");
        }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
        NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    } else {
        NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
            MEDIA_LOGD("JsSetLoop napi_create_async_work");
        }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
        NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    }
    asyncCtx.release();
    return result;
}

napi_value SoundPoolNapi::JsSetPriority(napi_env env, napi_callback_info info)
{
    MediaTrace trace("SoundPool::JsSetPriority");

    size_t argCount = PARAM3;
    napi_value args[PARAM3] = { nullptr };

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<SoundPoolAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get SoundPoolAsyncContext");
    asyncCtx->napi = SoundPoolNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceAndArgs");

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[PARAM2]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    napi_status status = napi_get_value_int32(env, args[PARAM0], &asyncCtx->streamId_);
    if (status != napi_ok || asyncCtx->streamId_ <= 0) {
        asyncCtx->SignError(MSERR_EXT_API9_INVALID_PARAMETER, "SetPriority streamId failed");
    }
    status = napi_get_value_int32(env, args[PARAM1], &asyncCtx->priority_);
    if (status != napi_ok || asyncCtx->priority_ < 0) {
        asyncCtx->SignError(MSERR_EXT_API9_INVALID_PARAMETER, "SetPriority priority failed");
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsSetPriority", NAPI_AUTO_LENGTH, &resource);
    if (status == napi_ok && asyncCtx->streamId_ > 0) {
        NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
            SoundPoolAsyncContext* asyncCtx = reinterpret_cast<SoundPoolAsyncContext *>(data);
            CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "asyncCtx is nullptr!");
            CHECK_AND_RETURN_LOG(asyncCtx->napi->soundPool_ != nullptr, "soundPool_ is nullptr!");
            int32_t ret = asyncCtx->napi->soundPool_->SetPriority(asyncCtx->streamId_, asyncCtx->priority_);
            if (ret != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "SetPriority streamId failed");
            }
            MEDIA_LOGI("The js thread of SetPriority finishes execution and returns");
        }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
        NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    } else {
        NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
            MEDIA_LOGD("JsSetPriority napi_create_async_work");
        }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
        NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    }
    asyncCtx.release();
    return result;
}

napi_value SoundPoolNapi::JsSetRate(napi_env env, napi_callback_info info)
{
    MediaTrace trace("SoundPool::JsSetRate");

    size_t argCount = PARAM3;
    napi_value args[PARAM3] = { nullptr };

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<SoundPoolAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get SoundPoolAsyncContext");
    asyncCtx->napi = SoundPoolNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceAndArgs");

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[PARAM2]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsPlay", NAPI_AUTO_LENGTH, &resource);
    if (asyncCtx->napi->ParserRateOptionFromJs(asyncCtx, env, args) == MSERR_OK) {
        NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
            SoundPoolAsyncContext* asyncCtx = reinterpret_cast<SoundPoolAsyncContext *>(data);
            CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "asyncCtx is nullptr!");
            CHECK_AND_RETURN_LOG(asyncCtx->napi->soundPool_ != nullptr, "soundPool_ is nullptr!");
            int32_t ret = asyncCtx->napi->soundPool_->SetRate(asyncCtx->streamId_, asyncCtx->renderRate_);
            if (ret != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "SetRate streamId failed");
            }
            MEDIA_LOGI("The js thread of SetRate finishes execution and returns");
        }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
        NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    } else {
        NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
            MEDIA_LOGD("JsSetRate napi_create_async_work");
        }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
        NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    }
    asyncCtx.release();
    return result;
}

napi_value SoundPoolNapi::JsSetVolume(napi_env env, napi_callback_info info)
{
    MediaTrace trace("SoundPool::JsSetVolume");

    size_t argCount = PARAM4;
    napi_value args[PARAM4] = { nullptr };

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<SoundPoolAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get SoundPoolAsyncContext");
    asyncCtx->napi = SoundPoolNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceAndArgs");

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[PARAM3]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsSetVolume", NAPI_AUTO_LENGTH, &resource);
    if (asyncCtx->napi->ParserVolumeOptionFromJs(asyncCtx, env, args) == MSERR_OK) {
        NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
            SoundPoolAsyncContext* asyncCtx = reinterpret_cast<SoundPoolAsyncContext *>(data);
            CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "asyncCtx is nullptr!");
            CHECK_AND_RETURN_LOG(asyncCtx->napi->soundPool_ != nullptr, "soundPool_ is nullptr!");
            int32_t ret = asyncCtx->napi->soundPool_->SetVolume(asyncCtx->streamId_,
                asyncCtx->leftVolume_, asyncCtx->rightVolume_);
            if (ret != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "setVolume streamId failed");
            }
            MEDIA_LOGI("The js thread of SetVolume finishes execution and returns");
        }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
        NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    } else {
        NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
            MEDIA_LOGD("JsSetVolume napi_create_async_work");
        }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
        NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    }
    asyncCtx.release();
    return result;
}

napi_value SoundPoolNapi::JsUnload(napi_env env, napi_callback_info info)
{
    MediaTrace trace("SoundPool::JsUnload");

    size_t argCount = PARAM2;
    napi_value args[PARAM2] = { nullptr };

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<SoundPoolAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get SoundPoolAsyncContext");
    asyncCtx->napi = SoundPoolNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceAndArgs");

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[PARAM1]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    napi_status status = napi_get_value_int32(env, args[PARAM0], &asyncCtx->soundId_);
    if (status != napi_ok || asyncCtx->soundId_ <= 0) {
        asyncCtx->SignError(MSERR_EXT_API9_IO, "unLoad failed,inavild value");
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsUnload", NAPI_AUTO_LENGTH, &resource);
    if (status == napi_ok && asyncCtx->soundId_ > 0) {
        NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
            SoundPoolAsyncContext* asyncCtx = reinterpret_cast<SoundPoolAsyncContext *>(data);
            CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "asyncCtx is nullptr!");
            CHECK_AND_RETURN_LOG(asyncCtx->napi->soundPool_ != nullptr, "soundPool_ is nullptr!");
            int32_t ret = asyncCtx->napi->soundPool_->Unload(asyncCtx->soundId_);
            if (ret != MSERR_OK) {
                asyncCtx->SignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "unLoad soundID failed");
            }
            MEDIA_LOGI("The js thread of Unload finishes execution and returns, soundID: %{public}d",
                asyncCtx->soundId_);
        }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
        NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    } else {
        NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
            MEDIA_LOGD("JsUnload napi_create_async_work");
        }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
        NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    }
    asyncCtx.release();

    return result;
}

napi_value SoundPoolNapi::JsRelease(napi_env env, napi_callback_info info)
{
    MediaTrace trace("SoundPool::JsRelease");

    size_t argCount = PARAM1;
    napi_value args[PARAM1] = { nullptr };

    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<SoundPoolAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncCtx != nullptr, result, "failed to get SoundPoolAsyncContext");
    asyncCtx->napi = SoundPoolNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(asyncCtx->napi != nullptr, result, "failed to GetJsInstanceAndArgs");

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[PARAM0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsRelease", NAPI_AUTO_LENGTH, &resource);
        NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {
            SoundPoolAsyncContext* asyncCtx = reinterpret_cast<SoundPoolAsyncContext *>(data);
            CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "asyncCtx is nullptr!");
            CHECK_AND_RETURN_LOG(asyncCtx->napi->soundPool_ != nullptr, "soundPool_ is nullptr!");
            int32_t ret = asyncCtx->napi->soundPool_->Release();
            CHECK_AND_RETURN_LOG(ret == MSERR_OK, "Release failed!");
            asyncCtx->napi->CancelCallback();
            MEDIA_LOGI("The js thread of JsRelease finishes execution and returns");
        }, MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
        NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncCtx->work, napi_qos_user_initiated));
    asyncCtx.release();

    return result;
}

napi_value SoundPoolNapi::JsSetOnCallback(napi_env env, napi_callback_info info)
{
    MediaTrace trace("SoundPool::JsSetOnCallback");
    MEDIA_LOGD("JsSetOnCallback Start");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 2;
    napi_value args[2] = { nullptr, nullptr };
    SoundPoolNapi *soundPoolNapi = SoundPoolNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(soundPoolNapi != nullptr, result, "Failed to retrieve instance");

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        soundPoolNapi->ErrorCallback(MSERR_INVALID_VAL, "SetEventCallback");
        return result;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGI("set callbackName: %{public}s", callbackName.c_str());
    if (callbackName != SoundPoolEvent::EVENT_LOAD_COMPLETED && callbackName != SoundPoolEvent::EVENT_PLAY_FINISHED &&
        callbackName != SoundPoolEvent::EVENT_ERROR) {
        soundPoolNapi->ErrorCallback(MSERR_INVALID_VAL, "SetEventCallback");
        return result;
    }

    napi_ref ref = nullptr;
    napi_status status = napi_create_reference(env, args[1], 1, &ref);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && ref != nullptr, result, "failed to create reference!");

    std::shared_ptr<AutoRef> autoRef = std::make_shared<AutoRef>(env, ref);
    soundPoolNapi->SetCallbackReference(callbackName, autoRef);

    MEDIA_LOGI("JsSetOnCallback callbackName: %{public}s success", callbackName.c_str());
    return result;
}

napi_value SoundPoolNapi::JsClearOnCallback(napi_env env, napi_callback_info info)
{
    MediaTrace trace("SoundPool::JsClearOnCallback");
    MEDIA_LOGD("JsClearOnCallback Start");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 1;
    napi_value args[1] = { nullptr };
    SoundPoolNapi *soundPoolNapi = SoundPoolNapi::GetJsInstanceAndArgs(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(soundPoolNapi != nullptr, result, "Failed to retrieve instance");

    napi_valuetype valueType0 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string) {
        soundPoolNapi->ErrorCallback(MSERR_INVALID_VAL, "CancelEventCallback");
        return result;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    if (callbackName != SoundPoolEvent::EVENT_LOAD_COMPLETED && callbackName != SoundPoolEvent::EVENT_PLAY_FINISHED &&
        callbackName != SoundPoolEvent::EVENT_ERROR) {
        soundPoolNapi->ErrorCallback(MSERR_INVALID_VAL, "CancelEventCallback");
        return result;
    }

    soundPoolNapi->CancelCallbackReference(callbackName);

    MEDIA_LOGI("0x%{public}06" PRIXPTR " JsClearOnCallback success", FAKE_POINTER(soundPoolNapi));
    return result;
}

SoundPoolNapi* SoundPoolNapi::GetJsInstanceAndArgs(napi_env env, napi_callback_info info,
    size_t &argCount, napi_value *args)
{
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, nullptr, "failed to napi_get_cb_info");
    MEDIA_LOGI("0x:%{public}06" PRIXPTR " instance argCount:%{public}zu", FAKE_POINTER(jsThis), argCount);

    SoundPoolNapi *soundPoolNapi = nullptr;

    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&soundPoolNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && soundPoolNapi != nullptr, nullptr, "failed to retrieve instance");

    return soundPoolNapi;
}

napi_status SoundPoolNapi::GetJsInstanceWithParameter(napi_env env, napi_value *argv)
{
    napi_status status = napi_get_value_int32(env, argv[PARAM0], &maxStreams); // get maxStreams
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "failed to get napi maxStreams");

    napi_value tempValue = nullptr;
    int32_t intValue = {0};
    status = napi_get_named_property(env, argv[PARAM1], "content", &tempValue);
    if (status == napi_ok) {
        napi_get_value_int32(env, tempValue, &intValue);
        rendererInfo.contentType = static_cast<AudioStandard::ContentType>(intValue);
    }

    status = napi_get_named_property(env, argv[PARAM1], "usage", &tempValue);
    if (status == napi_ok) {
        napi_get_value_int32(env, tempValue, &intValue);
        rendererInfo.streamUsage = static_cast<AudioStandard::StreamUsage>(intValue);
    }

    status = napi_get_named_property(env, argv[PARAM1], "rendererFlags", &tempValue);
    if (status == napi_ok) {
        napi_get_value_int32(env, tempValue, &(rendererInfo.rendererFlags));
    }

    return status;
}

int32_t SoundPoolNapi::ParserLoadOptionFromJs(std::unique_ptr<SoundPoolAsyncContext> &asyncCtx,
    napi_env env, napi_value *argv, size_t argCount)
{
    int32_t ret = MSERR_OK;
    MEDIA_LOGI("ParserLoadOptionFromJs argCount %{public}zu", argCount);
    if ((argCount < PARAM3) && (argCount > 0)) {
        asyncCtx->url_ = CommonNapi::GetStringArgument(env, argv[PARAM0]);
        CHECK_AND_RETURN_RET(asyncCtx->url_ != "",
            (asyncCtx->SoundPoolAsyncSignError(MSERR_OPEN_FILE_FAILED, "geturl", "url"), MSERR_OPEN_FILE_FAILED));
    } else if ((argCount >= PARAM3) && (argCount < MAX_PARAM)) {
        napi_status status = napi_get_value_int32(env, argv[PARAM0], &asyncCtx->fd_);
        CHECK_AND_RETURN_RET((status == napi_ok && asyncCtx->fd_ > 0),
            (asyncCtx->SoundPoolAsyncSignError(MSERR_OPEN_FILE_FAILED, "getfd", "fd"), MSERR_OPEN_FILE_FAILED));

        status = napi_get_value_int64(env, argv[PARAM1], &asyncCtx->offset_);
        CHECK_AND_RETURN_RET((status == napi_ok && asyncCtx->offset_ >= 0),
            (asyncCtx->SoundPoolAsyncSignError(MSERR_OPEN_FILE_FAILED, "getoffset", "offset"), MSERR_OPEN_FILE_FAILED));

        status = napi_get_value_int64(env, argv[PARAM2], &asyncCtx->length_);
        CHECK_AND_RETURN_RET((status == napi_ok && asyncCtx->length_ > 0),
            (asyncCtx->SoundPoolAsyncSignError(MSERR_OPEN_FILE_FAILED, "getlength", "length"), MSERR_OPEN_FILE_FAILED));
    } else {
        MEDIA_LOGI("Get Value error,return error:MSERR_INVALID_VAL");
        return MSERR_INVALID_VAL;
    }
    return ret;
}

static std::shared_ptr<AbilityRuntime::Context> GetAbilityContext(napi_env env)
{
    auto ability = OHOS::AbilityRuntime::GetCurrentAbility(env);
    if (ability == nullptr) {
        MEDIA_LOGE("Failed to obtain ability in FA mode");
        return nullptr;
    }
    auto faContext = ability->GetAbilityContext();
    if (faContext == nullptr) {
        MEDIA_LOGE("GetAbilityContext returned null in FA model");
        return nullptr;
    }
    return faContext;
}

int32_t SoundPoolNapi::ParserPlayOptionFromJs(std::unique_ptr<SoundPoolAsyncContext> &asyncCtx,
    napi_env env, napi_value *argv, size_t argCount)
{
    int32_t ret = MSERR_OK;
    MEDIA_LOGI("ParserPlayOptionFromJs argCount %{public}zu", argCount);
    napi_status status = napi_get_value_int32(env, argv[PARAM0], &asyncCtx->soundId_);
    CHECK_AND_RETURN_RET((status == napi_ok && asyncCtx->soundId_ > 0),
        (asyncCtx->SoundPoolAsyncSignError(MSERR_INVALID_VAL, "getplaysoundId", "soundId"), MSERR_INVALID_VAL));

    CommonNapi::GetPropertyInt32(env, argv[PARAM1], "loop", asyncCtx->playParameters_.loop);
    CommonNapi::GetPropertyInt32(env, argv[PARAM1], "rate", asyncCtx->playParameters_.rate);
    double leftVolume;
    ret = CommonNapi::GetPropertyDouble(env, argv[PARAM1], "leftVolume", leftVolume);
    if (ret > 0) {
        asyncCtx->playParameters_.leftVolume = static_cast<float>(leftVolume);
    }
    ret = CommonNapi::GetPropertyDouble(env, argv[PARAM1], "rightVolume", leftVolume);
    if (ret > 0) {
        asyncCtx->playParameters_.rightVolume = static_cast<float>(leftVolume);
    }
    CommonNapi::GetPropertyInt32(env, argv[PARAM1], "priority", asyncCtx->playParameters_.priority);
    GetPropertyBool(env, argv[PARAM1], "parallelPlayFlag", asyncCtx->playParameters_.parallelPlayFlag);

    std::shared_ptr<AbilityRuntime::Context> abilityContext = GetAbilityContext(env);
    if (abilityContext != nullptr) {
        asyncCtx->playParameters_.cacheDir = abilityContext->GetCacheDir();
    } else {
        asyncCtx->playParameters_.cacheDir = "/data/storage/el2/base/temp";
    }
    MEDIA_LOGI("playParameters_ loop:%{public}d, rate:%{public}d, leftVolume:%{public}f, rightvolume:%{public}f,"
        "priority:%{public}d, parallelPlayFlag:%{public}d", asyncCtx->playParameters_.loop,
        asyncCtx->playParameters_.rate, asyncCtx->playParameters_.leftVolume,
        asyncCtx->playParameters_.rightVolume, asyncCtx->playParameters_.priority,
        asyncCtx->playParameters_.parallelPlayFlag);
    return MSERR_OK;
}

int32_t SoundPoolNapi::ParserRateOptionFromJs(std::unique_ptr<SoundPoolAsyncContext> &asyncCtx,
    napi_env env, napi_value *argv)
{
    int32_t ret = MSERR_OK;
    napi_status status = napi_get_value_int32(env, argv[PARAM0], &asyncCtx->streamId_);
    CHECK_AND_RETURN_RET((status == napi_ok && asyncCtx->streamId_ > 0),
        (asyncCtx->SoundPoolAsyncSignError(MSERR_INVALID_VAL, "getratestreamId", "streamId"), MSERR_INVALID_VAL));
    int32_t rendderRate;
    status = napi_get_value_int32(env, argv[PARAM1], &rendderRate);
    CHECK_AND_RETURN_RET(status == napi_ok,
        (asyncCtx->SoundPoolAsyncSignError(MSERR_INVALID_VAL, "getaudiorennderrate",
        "audiorennderrate"), MSERR_INVALID_VAL));
    asyncCtx->renderRate_ = static_cast<AudioStandard::AudioRendererRate>(rendderRate);

    return ret;
}

int32_t SoundPoolNapi::ParserVolumeOptionFromJs(std::unique_ptr<SoundPoolAsyncContext> &asyncCtx,
    napi_env env, napi_value *argv)
{
    int32_t ret = MSERR_OK;
    napi_status status = napi_get_value_int32(env, argv[PARAM0], &asyncCtx->streamId_);
    CHECK_AND_RETURN_RET((status == napi_ok && asyncCtx->streamId_ > 0),
        (asyncCtx->SoundPoolAsyncSignError(MSERR_INVALID_VAL, "getvolumestreamId", "streamId"), MSERR_INVALID_VAL));
    double tempvolume;
    status = napi_get_value_double(env, argv[PARAM1], &tempvolume);
    CHECK_AND_RETURN_RET(status == napi_ok,
        (asyncCtx->SoundPoolAsyncSignError(MSERR_INVALID_VAL, "getleftvolme", "leftvolme"), MSERR_INVALID_VAL));
    asyncCtx->leftVolume_ = static_cast<float>(tempvolume);

    status = napi_get_value_double(env, argv[PARAM2], &tempvolume);
    CHECK_AND_RETURN_RET(status == napi_ok,
        (asyncCtx->SoundPoolAsyncSignError(MSERR_INVALID_VAL, "getrightvolme", "rightvolme"), MSERR_INVALID_VAL));
    asyncCtx->rightVolume_ = static_cast<float>(tempvolume);

    return ret;
}

void SoundPoolNapi::ErrorCallback(int32_t errCode, const std::string &operate, const std::string &add)
{
    MEDIA_LOGE("failed to %{public}s, errCode = %{public}d", operate.c_str(), errCode);
    CHECK_AND_RETURN_LOG(callbackNapi_ != nullptr, "soundpoolCb_ is nullptr!");
    auto napiCb = std::static_pointer_cast<SoundPoolCallBackNapi>(callbackNapi_);

    MediaServiceExtErrCodeAPI9 err = MSErrorToExtErrorAPI9(static_cast<MediaServiceErrCode>(errCode));
    std::string msg = MSExtErrorAPI9ToString(err, operate, "") + add;
    napiCb->SendErrorCallback(errCode, msg);
}

void SoundPoolNapi::SetCallbackReference(const std::string &callbackName, std::shared_ptr<AutoRef> ref)
{
    eventCbMap_[callbackName] = ref;
    CHECK_AND_RETURN_LOG(callbackNapi_ != nullptr, "soundpoolCb_ is nullptr!");
    auto napiCb = std::static_pointer_cast<SoundPoolCallBackNapi>(callbackNapi_);
    napiCb->SaveCallbackReference(callbackName, ref);
}

void SoundPoolNapi::CancelCallbackReference(const std::string &callbackName)
{
    CHECK_AND_RETURN_LOG(callbackNapi_ != nullptr, "soundpoolCb_ is nullptr!");
    auto napiCb = std::static_pointer_cast<SoundPoolCallBackNapi>(callbackNapi_);
    napiCb->CancelCallbackReference(callbackName);
    eventCbMap_[callbackName] = nullptr;
}

void SoundPoolNapi::CancelCallback()
{
    CHECK_AND_RETURN_LOG(callbackNapi_ != nullptr, "soundpoolCb_ is nullptr!");
    auto napiCb = std::static_pointer_cast<SoundPoolCallBackNapi>(callbackNapi_);
    napiCb->ClearCallbackReference();
}

bool SoundPoolNapi::GetPropertyBool(napi_env env, napi_value configObj, const std::string &type, bool &result)
{
    napi_value item = nullptr;
    bool exist = false;
    napi_status status = napi_has_named_property(env, configObj, type.c_str(), &exist);
    if (status != napi_ok || !exist) {
        MEDIA_LOGE("can not find %{public}s property", type.c_str());
        return false;
    }

    if (napi_get_named_property(env, configObj, type.c_str(), &item) != napi_ok) {
        MEDIA_LOGE("get %{public}s property fail", type.c_str());
        return false;
    }

    if (napi_get_value_bool(env, item, &result) != napi_ok) {
        MEDIA_LOGE("get %{public}s property value fail", type.c_str());
        return false;
    }
    return true;
}

RetInfo GetRetInfo(int32_t errCode, const std::string &operate, const std::string &param, const std::string &add = "")
{
    MEDIA_LOGE("failed to %{public}s, param %{public}s, errCode = %{public}d", operate.c_str(), param.c_str(), errCode);
    MediaServiceExtErrCodeAPI9 err = MSErrorToExtErrorAPI9(static_cast<MediaServiceErrCode>(errCode));
    if (errCode == MSERR_UNSUPPORT_VID_PARAMS) {
        return RetInfo(err, "The video parameter is not supported. Please check the type and range.");
    }

    if (errCode == MSERR_UNSUPPORT_AUD_PARAMS) {
        return RetInfo(err, "The audio parameter is not supported. Please check the type and range.");
    }

    std::string message;
    if (err == MSERR_EXT_API9_INVALID_PARAMETER) {
        message = MSExtErrorAPI9ToString(err, param, "") + add;
    } else {
        message = MSExtErrorAPI9ToString(err, operate, "") + add;
    }

    MEDIA_LOGE("errCode: %{public}d, errMsg: %{public}s", err, message.c_str());
    return RetInfo(err, message);
}

void SoundPoolAsyncContext::SoundPoolAsyncSignError(int32_t errCode, const std::string &operate,
    const std::string &param, const std::string &add)
{
    RetInfo retInfo = GetRetInfo(errCode, operate, param, add);
    SignError(retInfo.first, retInfo.second);
}
} // namespace Media
} // namespace OHOS