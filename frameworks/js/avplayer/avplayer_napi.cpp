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

#include "avplayer_napi.h"
#include "avplayer_callback.h"
#include "media_log.h"
#include "media_errors.h"
#include "surface_utils.h"
#include "string_ex.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVPlayerNapi"};
}

namespace OHOS {
namespace Media {
thread_local napi_ref AVPlayerNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "AVPlayer";
AVPlayerNapi::AVPlayerNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVPlayerNapi::~AVPlayerNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value AVPlayerNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAVPlayer", JsCreateAVPlayer),
    };

    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("prepare", JsPrepare),
        DECLARE_NAPI_FUNCTION("play", JsPlay),
        DECLARE_NAPI_FUNCTION("pause", JsPause),
        DECLARE_NAPI_FUNCTION("stop", JsStop),
        DECLARE_NAPI_FUNCTION("reset", JsReset),
        DECLARE_NAPI_FUNCTION("release", JsRelease),
        DECLARE_NAPI_FUNCTION("seek", JsSeek),
        DECLARE_NAPI_FUNCTION("on", JsSetEventCallback),
        DECLARE_NAPI_FUNCTION("setVolume", JsSetVolume),
        DECLARE_NAPI_FUNCTION("getTrackDescription", JsGetTrackDescription),
        DECLARE_NAPI_FUNCTION("setSpeed", JsSetSpeed),

        DECLARE_NAPI_GETTER_SETTER("url", JsGetUrl, JsSetUrl),
        DECLARE_NAPI_GETTER_SETTER("fdSrc", JsGetAVFileDescriptor, JsSetAVFileDescriptor),
        DECLARE_NAPI_GETTER_SETTER("surfaceId", JsGetSurfaceID, JsSetSurfaceID),
        DECLARE_NAPI_GETTER_SETTER("loop", JsGetLoop, JsSetLoop),
        DECLARE_NAPI_GETTER_SETTER("videoScaleType", JsGetVideoScaleType, JsSetVideoScaleType),
        DECLARE_NAPI_GETTER_SETTER("audioInterruptMode", JsGetAudioInterruptMode, JsSetAudioInterruptMode),

        DECLARE_NAPI_GETTER("currentTime", JsGetCurrentTime),
        DECLARE_NAPI_GETTER("duration", JsGetDuration),
        DECLARE_NAPI_GETTER("state", JsGetState),
        DECLARE_NAPI_GETTER("width", JsGetWidth),
        DECLARE_NAPI_GETTER("height", JsGetHeight),
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define AVPlayer class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value AVPlayerNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "failed to napi_get_cb_info");

    AVPlayerNapi *jsPlayer = new(std::nothrow) AVPlayerNapi();
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to new AVPlayerNapi");

    jsPlayer->env_ = env;
    jsPlayer->player_ = PlayerFactory::CreatePlayer();
    CHECK_AND_RETURN_RET_LOG(jsPlayer->player_ != nullptr, result, "failed to CreatePlayer");

    jsPlayer->taskQue_ = std::make_unique<TaskQueue>("AVPlayerNapi");
    (void)jsPlayer->taskQue_->Start();
    jsPlayer->playerCb_ = std::make_shared<AVPlayerCallback>(env);
    (void)jsPlayer->player_->SetPlayerCallback(jsPlayer->playerCb_);

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(jsPlayer),
        AVPlayerNapi::Destructor, nullptr, &(jsPlayer->wrapper_));
    if (status != napi_ok) {
        delete jsPlayer;
        MEDIA_LOGE("Failed to wrap native instance");
        return result;
    }

    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void AVPlayerNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)finalize;
    if (nativeObject != nullptr) {
        AVPlayerNapi *jsPlayer = reinterpret_cast<AVPlayerNapi *>(nativeObject);
        (void)jsPlayer->taskQue_->Stop();
        if (jsPlayer->player_ != nullptr) {
            (void)jsPlayer->player_->Release();
        }
        jsPlayer->StopOnCallback();
        if (jsPlayer->wrapper_ != nullptr) {
            napi_delete_reference(env, wrapper_);
        }
        delete reinterpret_cast<AVPlayerNapi *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}

napi_value AVPlayerNapi::JsCreateAVPlayer(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsCreateAVPlayer In");

    std::unique_ptr<MediaAsyncContext> asyncContext = std::make_unique<MediaAsyncContext>(env);

    // get args
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
    asyncContext->ctorFlag = true;

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsCreateAVPlayer", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();

    return result;
}

napi_value AVPlayerNapi::JsPrepare(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsPrepare In");

    auto promiseCtx = std::make_unique<AVPlayerContext>(env);
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, result, "failed to napi_get_cb_info");

    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&promiseCtx->napi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && promiseCtx->napi != nullptr, result, "failed to napi_unwrap");

    promiseCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    promiseCtx->deferred = CommonNapi::CreatePromise(env, promiseCtx->callbackRef, result);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsPrepare", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            MEDIA_LOGD("Prepare Task");
            auto promiseCtx = reinterpret_cast<AVPlayerContext *>(data);
            CHECK_AND_RETURN_LOG(promiseCtx != nullptr, "promiseCtx is nullptr!");

            if (promiseCtx->napi == nullptr || promiseCtx->napi->player_ == nullptr) {
                return promiseCtx->SignError(MSERR_EXT_API9_NO_MEMORY, "napi or player is nullptr");
            }
            // async prepare
            if (napi->player_->PrepareAsync() != MSERR_OK) {
                return promiseCtx->SignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "player PrepareAsync failed");
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(promiseCtx.get()), &promiseCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, promiseCtx->work));
    promiseCtx.release();
    return result;
}

napi_value AVPlayerNapi::JsPlay(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsPlay In");

    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, result, "failed to napi_get_cb_info");

    AVPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&jsPlayer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsPlayer != nullptr, result, "failed to napi_unwrap");

    auto task = std::make_shared<TaskHandler<void>>([this, jsPlayer]() {
        MEDIA_LOGD("Play Task");
        if (jsPlayer->player_ == nullptr || jsPlayer->playerCb_ == nullptr) {
            return jsPlayer->OnErrorCb(MSERR_EXT_API9_NO_MEMORY, "player or playerCb is nullptr");
        }
    
        if (jsPlayer->player_->Play() != MSERR_OK) {
            return jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "player Play failed");
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    return result;
}

napi_value AVPlayerNapi::JsPause(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsPause In");

    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, result, "failed to napi_get_cb_info");

    AVPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&jsPlayer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsPlayer != nullptr, result, "failed to napi_unwrap");

    auto task = std::make_shared<TaskHandler<void>>([this, jsPlayer]() {
        MEDIA_LOGD("Pause Task");
        if (jsPlayer->player_ == nullptr || jsPlayer->playerCb_ == nullptr) {
            return jsPlayer->OnErrorCb(MSERR_EXT_API9_NO_MEMORY, "player or playerCb is nullptr");
        }
    
        if (jsPlayer->player_->Pause() != MSERR_OK) {
            return jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "player Pause failed");
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    return result;
}

napi_value AVPlayerNapi::JsStop(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsStop In");

    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, result, "failed to napi_get_cb_info");

    AVPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&jsPlayer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsPlayer != nullptr, result, "failed to napi_unwrap");

    auto task = std::make_shared<TaskHandler<void>>([this, jsPlayer]() {
        MEDIA_LOGD("Stop Task");
        if (jsPlayer->player_ == nullptr || jsPlayer->playerCb_ == nullptr) {
            return jsPlayer->OnErrorCb(MSERR_EXT_API9_NO_MEMORY, "player or playerCb is nullptr");
        }
    
        if (jsPlayer->player_->Stop() != MSERR_OK) {
            return jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "player Stop failed");
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    return result;
}

napi_value AVPlayerNapi::JsReset(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsReset In");

    auto promiseCtx = std::make_unique<AVPlayerContext>(env);
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, result, "failed to napi_get_cb_info");

    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&promiseCtx->napi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && promiseCtx->napi != nullptr, result, "failed to napi_unwrap");

    promiseCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    promiseCtx->deferred = CommonNapi::CreatePromise(env, promiseCtx->callbackRef, result);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsReset", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto promiseCtx = reinterpret_cast<AVPlayerContext *>(data);
            CHECK_AND_RETURN_LOG(promiseCtx != nullptr, "promiseCtx is nullptr!");

            if (promiseCtx->napi == nullptr || promiseCtx->napi->player_ == nullptr) {
                return promiseCtx->SignError(MSERR_EXT_API9_NO_MEMORY, "napi or player is nullptr");
            }
            // sync reset
            if (napi->player_->Reset() != MSERR_OK) {
                return promiseCtx->SignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "player Reset failed");
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(promiseCtx.get()), &promiseCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, promiseCtx->work));
    promiseCtx.release();
    return result;
}

napi_value AVPlayerNapi::JsRelease(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsReset In");

    auto promiseCtx = std::make_unique<AVPlayerContext>(env);
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, result, "failed to napi_get_cb_info");

    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&promiseCtx->napi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && promiseCtx->napi != nullptr, result, "failed to napi_unwrap");

    promiseCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    promiseCtx->deferred = CommonNapi::CreatePromise(env, promiseCtx->callbackRef, result);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsReset", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto promiseCtx = reinterpret_cast<AVPlayerContext *>(data);
            CHECK_AND_RETURN_LOG(promiseCtx != nullptr, "promiseCtx is nullptr!");

            if (promiseCtx->napi == nullptr || promiseCtx->napi->player_ == nullptr) {
                return promiseCtx->SignError(MSERR_EXT_API9_NO_MEMORY, "napi or player is nullptr");
            }
            // sync relase
            (void)napi->player_->Release();
            napi->player_ = nullptr;
            napi->playerCb_ = nullptr;
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(promiseCtx.get()), &promiseCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, promiseCtx->work));
    promiseCtx.release();
    return result;
}

napi_value AVPlayerNapi::JsSeek(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsSeek In");

    size_t argCount = 2; // seek(timeMs: number, mode?:SeekMode)
    napi_value args[2] = { nullptr, nullptr }; // seek(timeMs: number, mode?:SeekMode)
    napi_value jsThis = nullptr;

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, result, "failed to napi_get_cb_info");
    CHECK_AND_RETURN_RET_LOG(args[0] != nullptr, result, "args[0] is nullptr");

    AVPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&jsPlayer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsPlayer != nullptr, result, "failed to napi_unwrap");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "seek args[0] is not number");
        return result;
    }

    int32_t time = -1;
    status = napi_get_value_int32(env, args[0], &time);
    if (status != napi_ok || time < 0) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "invalid parameters, please check the input time");
        return result;
    }

    int32_t mode = SEEK_PREVIOUS_SYNC;  
    if (args[1] != nullptr) {
        if (napi_typeof(env, args[1], &valueType) != napi_ok || valueType != napi_number) {
            jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "seek args[1] is not number");
            return result;
        }
        status = napi_get_value_int32(env, args[1], &mode);
        if (status != napi_ok || mode < SEEK_NEXT_SYNC || mode > SEEK_CLOSEST) {
            jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "invalid parameters, please check the input mode");
            return result;
        }
    }

    auto task = std::make_shared<TaskHandler<void>>([this, jsPlayer, time, mode]() {
        MEDIA_LOGD("Seek Task");
        if (jsPlayer->player_ != nullptr) {
            if (jsPlayer->player_->Seek(time, mode) != MSERR_OK) {
                return jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "player Seek failed");
            }
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    return result;
}

napi_value AVPlayerNapi::JsSetSpeed(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsSetSpeed In");

    size_t argCount = 1; // setSpeed(speed: number)
    napi_value args[1] = { nullptr };
    napi_value jsThis = nullptr;

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, result, "failed to napi_get_cb_info");
    CHECK_AND_RETURN_RET_LOG(args[0] != nullptr, result, "args[0] is nullptr");

    AVPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&jsPlayer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsPlayer != nullptr, result, "failed to napi_unwrap");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "speed args[0] is not number");
        return result;
    }

    int32_t mode = SPEED_FORWARD_1_00_X;
    status = napi_get_value_int32(env, args[0], &mode);
    if (status != napi_ok || mode < SPEED_FORWARD_0_75_X || mode > SPEED_FORWARD_2_00_X) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "invalid parameters, please check the input mode");
        return result;
    }

    auto task = std::make_shared<TaskHandler<void>>([this, jsPlayer, mode]() {
        MEDIA_LOGD("Seek Task");
        if (jsPlayer->player_ != nullptr) {
            if (jsPlayer->player_->Speed(mode) != MSERR_OK) {
                return jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "player Speed failed");
            }
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    return result;
}

napi_value AVPlayerNapi::JsSetVolume(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsSetVolume In");

    size_t argCount = 1; // setVolume(vol: number)
    napi_value args[1] = { nullptr };
    napi_value jsThis = nullptr;

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, result, "failed to napi_get_cb_info");
    CHECK_AND_RETURN_RET_LOG(args[0] != nullptr, result, "args[0] is nullptr");

    AVPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&jsPlayer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsPlayer != nullptr, result, "failed to napi_unwrap");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "setVolume args[0] is not number");
        return result;
    }

    double volumeLevel = 1.0f;
    status = napi_get_value_double(env, args[0], &volumeLevel);
    if (status != napi_ok || volumeLevel < 0.0f || volumeLevel > 1.0f) {
        jsPlayer->OnErrorCb(MSERR_EXT_INVALID_VAL, "invalid parameters, please check the input parameters");
        return result;
    }

    auto task = std::make_shared<TaskHandler<void>>([this, jsPlayer, volumeLevel]() {
        MEDIA_LOGD("SetVolume Task");
        if (jsPlayer->player_ != nullptr) {
            if (jsPlayer->player_->SetVolume(mode) != MSERR_OK) {
                return jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "player SetVolume failed");
            }
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    return result;
}

napi_value AVPlayerNapi::JsSelectBitrate(napi_env env, napi_callback_info info)
{

}

napi_value AVPlayerNapi::JsSetUrl(napi_env env, napi_callback_info info)
{

}

napi_value AVPlayerNapi::JsGetUrl(napi_env env, napi_callback_info info)
{

}

napi_value AVPlayerNapi::JsGetAVFileDescriptor(napi_env env, napi_callback_info info)
{

}

napi_value AVPlayerNapi::JsSetAVFileDescriptor(napi_env env, napi_callback_info info)
{

}

napi_value AVPlayerNapi::JsSetSurfaceID(napi_env env, napi_callback_info info)
{

}

napi_value AVPlayerNapi::JsGetSurfaceID(napi_env env, napi_callback_info info)
{

}

napi_value AVPlayerNapi::JsSetLoop(napi_env env, napi_callback_info info)
{

}

napi_value AVPlayerNapi::JsGetLoop(napi_env env, napi_callback_info info)
{

}

napi_value AVPlayerNapi::JsSetVideoScaleType(napi_env env, napi_callback_info info)
{

}

napi_value AVPlayerNapi::JsGetVideoScaleType(napi_env env, napi_callback_info info)
{

}

napi_value AVPlayerNapi::JsGetAudioInterruptMode(napi_env env, napi_callback_info info)
{

}

napi_value AVPlayerNapi::JsSetAudioInterruptMode(napi_env env, napi_callback_info info)
{

}

napi_value AVPlayerNapi::JsGetCurrentTime(napi_env env, napi_callback_info info)
{

}

napi_value AVPlayerNapi::JsGetDuration(napi_env env, napi_callback_info info)
{

}

napi_value AVPlayerNapi::JsGetState(napi_env env, napi_callback_info info)
{

}

napi_value AVPlayerNapi::JsGetWidth(napi_env env, napi_callback_info info)
{

}

napi_value AVPlayerNapi::JsGetHeight(napi_env env, napi_callback_info info)
{

}

napi_value AVPlayerNapi::JsGetTrackDescription(napi_env env, napi_callback_info info)
{

}

napi_value AVPlayerNapi::JsSetEventCallback(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    static constexpr size_t minArgCount = 2;
    size_t argCount = minArgCount;
    napi_value args[minArgCount] = { nullptr, nullptr };
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr || args[1] == nullptr) {
        MEDIA_LOGE("failed to napi_get_cb_info");
        return result;
    }

    AVPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&jsPlayer));
    if (jsPlayer == nullptr) {
        MEDIA_LOGE("failed to napi_unwrap");
        return result;
    }

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "napi_typeof failed, please check the input parameters");
        return result;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGD("set callbackName: %{public}s", callbackName.c_str());

    napi_ref ref = nullptr;
    status = napi_create_reference(env, args[1], 1, &ref);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && ref != nullptr, result, "failed to create reference!");

    std::shared_ptr<AutoRef> autoRef = std::make_shared<AutoRef>(env, ref);
    jsPlayer->SaveCallbackReference(callbackName, autoRef);
    return result;
}

void AVPlayerNapi::SaveCallbackReference(const std::string &callbackName, std::shared_ptr<AutoRef> ref)
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_[callbackName] = ref;
    if (playerCb_ != nullptr) {
        std::shared_ptr<AVPlayerCallback> cb = std::static_pointer_cast<AVPlayerCallback>(playerCb_);
        cb->SaveCallbackReference(callbackName, ref);
    }
}

void AVPlayerNapi::StopOnCallback()
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_.clear();
    if (playerCb_ != nullptr) {
        std::shared_ptr<AVPlayerCallback> cb = std::static_pointer_cast<AVPlayerCallback>(playerCb_);
        cb->ClearCallbackReference();
    }
}

void AVPlayerNapi::OnErrorCb(MediaServiceExtErrCodeAPI9 errorCode, const std::string &errorMsg)
{
    if (playerCb_ != nullptr) {
        std::shared_ptr<AVPlayerCallback> cb = std::static_pointer_cast<AVPlayerCallback>(playerCb_);
        cb->OnErrorCb(errorCode, errorMsg);
    }
}
} // namespace Media
} // namespace OHOS