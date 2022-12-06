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
        DECLARE_NAPI_FUNCTION("on", JsSetOnCallback),
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

    jsPlayer->playerCb_ = std::make_shared<AVPlayerCallback>(env, jsPlayer);
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
        jsPlayer->ReleasePlayer();
        if (jsPlayer->wrapper_ != nullptr) {
            napi_delete_reference(env, jsPlayer->wrapper_);
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
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, nullptr, "failed to napi_get_cb_info");

    asyncContext->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncContext->deferred = CommonNapi::CreatePromise(env, asyncContext->callbackRef, result);
    asyncContext->JsResult = std::make_unique<MediaJsResultInstance>(constructor_);
    asyncContext->ctorFlag = true;

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsCreateAVPlayer", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void *data) {},
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
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    promiseCtx->napi = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    promiseCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    promiseCtx->deferred = CommonNapi::CreatePromise(env, promiseCtx->callbackRef, result);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsPrepare", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void *data) {
            MEDIA_LOGD("Prepare Task");
            auto promiseCtx = reinterpret_cast<AVPlayerContext *>(data);
            CHECK_AND_RETURN_LOG(promiseCtx != nullptr, "promiseCtx is nullptr!");

            auto jsPlayer = promiseCtx->napi;
            if (jsPlayer == nullptr) {
                return promiseCtx->SignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "avplayer is deconstructed");
            }

            auto state = jsPlayer->GetCurrentState();
            if (state != AVPlayerState::STATE_INITIALIZED &&
                state != AVPlayerState::STATE_STOPPED) {
                return promiseCtx->SignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
                    "current state is not stopped or initialized");
            }

            auto task = std::make_shared<TaskHandler<void>>([jsPlayer]() {
                MEDIA_LOGD("Prepare Task");
                (void)jsPlayer->player_->PrepareAsync();
            });

            std::unique_lock<std::mutex> lock(jsPlayer->mutex_);
            (void)jsPlayer->taskQue_->EnqueueTask(task);
            jsPlayer->preparingCond_.wait(lock);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(promiseCtx.get()), &promiseCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, promiseCtx->work));
    promiseCtx.release();
    MEDIA_LOGD("JsPrepare In");
    return result;
}

napi_value AVPlayerNapi::JsPlay(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsPlay In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    auto state = jsPlayer->GetCurrentState();
    if (state != AVPlayerState::STATE_PREPARED &&
        state != AVPlayerState::STATE_PAUSED &&
        state != AVPlayerState::STATE_COMPLETED) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not prepared/paused/completed");
        return result;
    }

    auto task = std::make_shared<TaskHandler<void>>([jsPlayer]() {
        MEDIA_LOGD("Play Task");
        if (jsPlayer->player_ != nullptr) {
            (void)jsPlayer->player_->Play();
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    MEDIA_LOGD("JsPlay Out");
    return result;
}

napi_value AVPlayerNapi::JsPause(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsPause In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    if (jsPlayer->GetCurrentState() != AVPlayerState::STATE_PLAYING) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not playing");
        return result;
    }

    auto task = std::make_shared<TaskHandler<void>>([jsPlayer]() {
        MEDIA_LOGD("Pause Task");
        if (jsPlayer->player_ != nullptr) {
            (void)jsPlayer->player_->Pause();
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    MEDIA_LOGD("JsPause Out");
    return result;
}

napi_value AVPlayerNapi::JsStop(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsStop In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    if (!jsPlayer->IsControllable()) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not support stop");
        return result;
    }

    auto task = std::make_shared<TaskHandler<void>>([jsPlayer]() {
        MEDIA_LOGD("Stop Task");
        if (jsPlayer->player_ != nullptr) {
            (void)jsPlayer->player_->Stop();
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    MEDIA_LOGD("JsStop Out");
    return result;
}

napi_value AVPlayerNapi::JsReset(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsReset In");

    auto promiseCtx = std::make_unique<AVPlayerContext>(env);
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    promiseCtx->napi = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    promiseCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    promiseCtx->deferred = CommonNapi::CreatePromise(env, promiseCtx->callbackRef, result);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsReset", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void *data) {
            auto promiseCtx = reinterpret_cast<AVPlayerContext *>(data);
            CHECK_AND_RETURN_LOG(promiseCtx != nullptr, "promiseCtx is nullptr!");

            auto jsPlayer = promiseCtx->napi;
            if (jsPlayer == nullptr) {
                return promiseCtx->SignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "avplayer is deconstructed");
            }

            auto state = jsPlayer->GetCurrentState();
            if (state == AVPlayerState::STATE_RELEASED || state == AVPlayerState::STATE_IDLE) {
                MEDIA_LOGW("current state is idle/released, unsupport reset");
                return;
            }

            jsPlayer->preparingCond_.notify_all(); // stop prepare
            (void)jsPlayer->taskQue_->Stop();
            jsPlayer->PauseListenCurrentResource(); // Pause event listening for the current resource
            (void)jsPlayer->player_->Reset(); // sync reset
            jsPlayer->ResetUserParameters();
            (void)jsPlayer->taskQue_->Start();
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(promiseCtx.get()), &promiseCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, promiseCtx->work));
    promiseCtx.release();
    MEDIA_LOGD("JsReset Out");
    return result;
}

void AVPlayerNapi::ReleasePlayer()
{
    ResetUserParameters();
    if (!isReleased_) {
        preparingCond_.notify_all(); // stop prepare

        if (taskQue_ != nullptr) {
            (void)taskQue_->Stop();
            taskQue_ = nullptr;
        }

        if (playerCb_ != nullptr) {
            std::shared_ptr<AVPlayerCallback> cb = std::static_pointer_cast<AVPlayerCallback>(playerCb_);
            cb->Release();
            playerCb_ = nullptr;
        }

        if (player_ != nullptr) {
            (void)player_->Release();
            player_ = nullptr;
        }
        isReleased_ = true;
    }
}

napi_value AVPlayerNapi::JsRelease(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsRelease In");

    auto promiseCtx = std::make_unique<AVPlayerContext>(env);
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    promiseCtx->napi = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    promiseCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    promiseCtx->deferred = CommonNapi::CreatePromise(env, promiseCtx->callbackRef, result);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsRelease", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void *data) {
            auto promiseCtx = reinterpret_cast<AVPlayerContext *>(data);
            CHECK_AND_RETURN_LOG(promiseCtx != nullptr, "promiseCtx is nullptr!");

            auto jsPlayer = promiseCtx->napi;
            if (jsPlayer->GetCurrentState() == AVPlayerState::STATE_RELEASED) {
                MEDIA_LOGW("current state is released, unsupport release");
                return;
            }
            jsPlayer->ReleasePlayer();
        },
        AVPlayerNapi::ReleaseCallback, static_cast<void *>(promiseCtx.get()), &promiseCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, promiseCtx->work));
    promiseCtx.release();
    MEDIA_LOGD("JsRelease Out");
    return result;
}

void AVPlayerNapi::ReleaseCallback(napi_env env, napi_status status, void *data)
{
    auto promiseCtx = reinterpret_cast<AVPlayerContext *>(data);
    CHECK_AND_RETURN_LOG(promiseCtx != nullptr, "promiseCtx is nullptr!");

    auto jsPlayer = promiseCtx->napi;
    CHECK_AND_RETURN_LOG(jsPlayer != nullptr, "jsPlayer is nullptr!");

    NapiCallback::StateChange *cb = new(std::nothrow) NapiCallback::StateChange();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new StateChange");

    cb->callback = jsPlayer->refMap_.at(AVPlayerEvent::EVENT_STATE_CHANGE);
    cb->callbackName = AVPlayerEvent::EVENT_STATE_CHANGE;
    cb->state = AVPlayerState::STATE_RELEASED;
    cb->reason = StateChangeReason::USER;
    NapiCallback::CompleteCallback(env, cb);
    MediaAsyncContext::CompleteCallback(env, status, data);
}

napi_value AVPlayerNapi::JsSeek(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsSeek In");

    napi_value args[2] = { nullptr }; // args[0]:timeMs, args[1]:SeekMode
    size_t argCount = 2; // args[0]:timeMs, args[1]:SeekMode
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "seek args[0] is not number");
        return result;
    }

    int32_t time = -1;
    napi_status status = napi_get_value_int32(env, args[0], &time);
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

    if (!jsPlayer->IsControllable()) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not support seek");
        return result;
    }

    auto task = std::make_shared<TaskHandler<void>>([jsPlayer, time, mode]() {
        MEDIA_LOGD("Seek Task");
        if (jsPlayer->player_ != nullptr) {
            (void)jsPlayer->player_->Seek(time, static_cast<PlayerSeekMode>(mode));
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

    napi_value args[1] = { nullptr };
    size_t argCount = 1; // setSpeed(speed: number)
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "setSpeed args[0] is not number");
        return result;
    }

    int32_t mode = SPEED_FORWARD_1_00_X;
    napi_status status = napi_get_value_int32(env, args[0], &mode);
    if (status != napi_ok || mode < SPEED_FORWARD_0_75_X || mode > SPEED_FORWARD_2_00_X) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "invalid parameters, please check the input mode");
        return result;
    }

    if (!jsPlayer->IsControllable()) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not support set speed");
        return result;
    }

    auto task = std::make_shared<TaskHandler<void>>([jsPlayer, mode]() {
        MEDIA_LOGD("Seek Task");
        if (jsPlayer->player_ != nullptr) {
            (void)jsPlayer->player_->SetPlaybackSpeed(static_cast<PlaybackRateMode>(mode));
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

    napi_value args[1] = { nullptr };
    size_t argCount = 1; // setVolume(vol: number)
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "setVolume args[0] is not number");
        return result;
    }

    double volumeLevel = 1.0f;
    napi_status status = napi_get_value_double(env, args[0], &volumeLevel);
    if (status != napi_ok || volumeLevel < 0.0f || volumeLevel > 1.0f) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "invalid parameters, check it");
        return result;
    }

    if (!jsPlayer->IsControllable()) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not support set volume");
        return result;
    }

    auto task = std::make_shared<TaskHandler<void>>([jsPlayer, volumeLevel]() {
        MEDIA_LOGD("SetVolume Task");
        if (jsPlayer->player_ != nullptr) {
            (void)jsPlayer->player_->SetVolume(volumeLevel, volumeLevel);
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    return result;
}

napi_value AVPlayerNapi::JsSelectBitrate(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsSelectBitrate In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1; // selectBitrate(bitRate: number)
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "selectBitrate args[0] is not number");
        return result;
    }

    int32_t bitrate = 0;
    napi_status status = napi_get_value_int32(env, args[0], &bitrate);
    if (status != napi_ok || bitrate < 0) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "invalid parameters, please check the input bitrate");
        return result;
    }

    if (!jsPlayer->IsControllable()) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not support select bitrate");
        return result;
    }

    auto task = std::make_shared<TaskHandler<void>>([jsPlayer, bitrate]() {
        MEDIA_LOGD("SelectBitRate Task");
        if (jsPlayer->player_ != nullptr) {
            (void)jsPlayer->player_->SelectBitRate(bitrate);
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    return result;
}

void AVPlayerNapi::SetSource(std::string url)
{
    MEDIA_LOGD("input url is %{public}s!", url.c_str());
    bool isFd = (url.find("fd://") != std::string::npos) ? true : false;
    bool isNetwork = (url.find("http") != std::string::npos) ? true : false;
    if (isNetwork) {
        auto task = std::make_shared<TaskHandler<void>>([this, url]() {
            MEDIA_LOGD("SetNetworkSource Task");
            if (player_ != nullptr) {
                if (player_->SetSource(url) != MSERR_OK) {
                    OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "player SetSourceUrl failed");
                }
            }
        });
        (void)taskQue_->EnqueueTask(task);
    } else if (isFd) {
        const std::string fdHead = "fd://";
        std::string inputFd = url.substr(fdHead.size());
        int32_t fd = -1;
        if (!StrToInt(inputFd, fd) || fd < 0) {
            OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "invalid parameters, please check the input parameters");
            return;
        }

        auto task = std::make_shared<TaskHandler<void>>([this, fd]() {
            MEDIA_LOGD("SetFdSource Task");
            if (player_ != nullptr) {
                if (player_->SetSource(fd, 0, -1) != MSERR_OK) {
                    OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "player SetSourceFd failed");
                }
            }
        });
        (void)taskQue_->EnqueueTask(task);
    } else {
        MEDIA_LOGE("input url error!");
        OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "invalid parameters, please check the input parameters");
    }
}

napi_value AVPlayerNapi::JsSetUrl(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsSetUrl In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1; // url: string
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    if (jsPlayer->GetCurrentState() != AVPlayerState::STATE_IDLE) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not idle");
        return result;
    }

    jsPlayer->StartListenCurrentResource(); // Listen to the events of the current resource
    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_string) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "SetUrl args[0] is not string");
        return result;
    }

    // get url from js
    jsPlayer->url_ = CommonNapi::GetStringArgument(env, args[0]);
    jsPlayer->SetSource(jsPlayer->url_);

    MEDIA_LOGD("JsSetUrl Out");
    return result;
}

napi_value AVPlayerNapi::JsGetUrl(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsGetUrl In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    napi_value value = nullptr;
    (void)napi_create_string_utf8(env, jsPlayer->url_.c_str(), NAPI_AUTO_LENGTH, &value);

    MEDIA_LOGD("JsGetUrl Out Current Url: %{public}s", jsPlayer->url_.c_str());
    return value;
}

napi_value AVPlayerNapi::JsSetAVFileDescriptor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsSetAVFileDescriptor In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1; // url: string
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    if (jsPlayer->GetCurrentState() != AVPlayerState::STATE_IDLE) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not idle");
        return result;
    }

    jsPlayer->StartListenCurrentResource(); // Listen to the events of the current resource
    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_object) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "SetAVFileDescriptor args[0] is not napi_object");
        return result;
    }

    if (!CommonNapi::GetFdArgument(env, args[0], jsPlayer->fileDescriptor_)) {
        MEDIA_LOGE("get fileDescriptor argument failed!");
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "invalid parameters, please check the input parameters");
        return result;
    }

    auto task = std::make_shared<TaskHandler<void>>([jsPlayer]() {
        MEDIA_LOGD("SetAVFileDescriptor Task");
        if (jsPlayer->player_ != nullptr) {
            auto playerFd = jsPlayer->fileDescriptor_;
            if (jsPlayer->player_->SetSource(playerFd.fd, playerFd.offset, playerFd.length) != MSERR_OK) {
                jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "player SetSource FileDescriptor failed");
            }
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);

    MEDIA_LOGD("JsSetAVFileDescriptor Out");
    return result;
}

napi_value AVPlayerNapi::JsGetAVFileDescriptor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsGetAVFileDescriptor In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    napi_value value = nullptr;
    (void)napi_create_object(env, &value);
    (void)CommonNapi::AddNumberPropInt32(env, value, "fd", jsPlayer->fileDescriptor_.fd);
    (void)CommonNapi::AddNumberPropInt64(env, value, "offset", jsPlayer->fileDescriptor_.offset);
    (void)CommonNapi::AddNumberPropInt64(env, value, "length", jsPlayer->fileDescriptor_.length);

    MEDIA_LOGD("JsGetAVFileDescriptor Out");
    return result;
}

void AVPlayerNapi::SetSurface(const std::string &surfaceStr)
{
    MEDIA_LOGD("get surface, surfaceStr = %{public}s", surfaceStr.c_str());
    uint64_t surfaceId = 0;
    if (surfaceStr.empty() || surfaceStr[0] < '0' || surfaceStr[0] > '9') {
        OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "input surface id is invalid");
        return;
    }
    surfaceId = std::stoull(surfaceStr);
    MEDIA_LOGD("get surface, surfaceId = (%{public}" PRIu64 ")", surfaceId);

    auto surface = SurfaceUtils::GetInstance()->GetSurface(surfaceId);
    if (surface == nullptr) {
        OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "SurfaceUtils::GetSurface is nullptr");
        return;
    }

    auto task = std::make_shared<TaskHandler<void>>([this, surface]() {
        MEDIA_LOGD("SetSurface Task");
        if (player_ != nullptr) {
            if (player_->SetVideoSurface(surface) != MSERR_OK) {
                return OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "player SetVideoSurface failed");
            }
        }
    });
    (void)taskQue_->EnqueueTask(task);
}

napi_value AVPlayerNapi::JsSetSurfaceID(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsSetSurfaceID In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1; // surfaceId?: string
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_string) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "SetUrl args[0] is not string");
        return result;
    }

    if (jsPlayer->GetCurrentState() != AVPlayerState::STATE_INITIALIZED) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not support set surface");
        return result;
    }

    // get url from js
    jsPlayer->surface_ = CommonNapi::GetStringArgument(env, args[0]);
    jsPlayer->SetSurface(jsPlayer->surface_);
    return result;
}

napi_value AVPlayerNapi::JsGetSurfaceID(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsGetSurfaceID In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    napi_value value = nullptr;
    (void)napi_create_string_utf8(env, jsPlayer->surface_.c_str(), NAPI_AUTO_LENGTH, &value);

    MEDIA_LOGD("JsGetSurfaceID Out Current SurfaceID: %{public}s", jsPlayer->surface_.c_str());
    return value;
}

napi_value AVPlayerNapi::JsSetLoop(napi_env env, napi_callback_info info)
{
     napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsSetLoop In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1; // loop: boolenan
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_boolean) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "SetUrl args[0] is not napi_boolean");
        return result;
    }

    napi_status status = napi_get_value_bool(env, args[0], &jsPlayer->loop_);
    if (status != napi_ok) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "invalid parameters, please check the input loop");
        return result;
    }

    if (!jsPlayer->IsControllable()) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not support set Loop");
        return result;
    }

    auto task = std::make_shared<TaskHandler<void>>([jsPlayer]() {
        MEDIA_LOGD("SetLooping Task");
        if (jsPlayer->player_ != nullptr) {
            (void)jsPlayer->player_->SetLooping(jsPlayer->loop_);
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    MEDIA_LOGD("JsSetLoop Out");
    return result;
}

napi_value AVPlayerNapi::JsGetLoop(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsGetLoop In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    napi_value value = nullptr;
    (void)napi_get_boolean(env, jsPlayer->loop_, &value);
    MEDIA_LOGD("JsGetLoop Out Current Loop: %{public}d", jsPlayer->loop_);
    return value;
}

napi_value AVPlayerNapi::JsSetVideoScaleType(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsSetVideoScaleType In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "SetVideoScaleType args[0] is not napi_number");
        return result;
    }

    int32_t videoScaleType = 0;
    napi_status status = napi_get_value_int32(env, args[0], &videoScaleType);
    if (status != napi_ok || videoScaleType < VIDEO_SCALE_TYPE_FIT || videoScaleType > VIDEO_SCALE_TYPE_FIT_CROP) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "invalid parameters, please check the input scale type");
        return result;
    }
    jsPlayer->videoScaleType_ = videoScaleType;

    if (!jsPlayer->IsControllable()) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not support set video scale type");
        return result;
    }

    auto task = std::make_shared<TaskHandler<void>>([jsPlayer, videoScaleType]() {
        MEDIA_LOGD("SetVideoScaleType Task");
        if (jsPlayer->player_ != nullptr) {
            Format format;
            (void)format.PutIntValue(PlayerKeys::VIDEO_SCALE_TYPE, videoScaleType);
            (void)jsPlayer->player_->SetParameter(format);
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    MEDIA_LOGD("JsSetVideoScaleType Out");
    return result;
}

napi_value AVPlayerNapi::JsGetVideoScaleType(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsGetVideoScaleType In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    napi_value value = nullptr;
    (void)napi_create_int32(env, static_cast<int32_t>(jsPlayer->videoScaleType_), &value);
    MEDIA_LOGD("JsGetVideoScaleType Out Current VideoScale: %{public}d", jsPlayer->videoScaleType_);
    return value;
}

napi_value AVPlayerNapi::JsSetAudioInterruptMode(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsSetAudioInterruptMode In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "SetVideoScaleType args[0] is not napi_number");
        return result;
    }

    int32_t interruptMode = 0;
    napi_status status = napi_get_value_int32(env, args[0], &interruptMode);
    if (status != napi_ok ||
        interruptMode < AudioStandard::InterruptMode::SHARE_MODE ||
        interruptMode > AudioStandard::InterruptMode::INDEPENDENT_MODE) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "invalid parameters, please check the input interrupt Mode");
        return result;
    }
    jsPlayer->interruptMode_ = static_cast<AudioStandard::InterruptMode>(interruptMode);

    if (!jsPlayer->IsControllable()) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not support set audio interrupt mode");
        return result;
    }

    auto task = std::make_shared<TaskHandler<void>>([jsPlayer]() {
        MEDIA_LOGD("SetAudioInterruptMode Task");
        if (jsPlayer->player_ != nullptr) {
            Format format;
            (void)format.PutIntValue(PlayerKeys::AUDIO_INTERRUPT_MODE, jsPlayer->interruptMode_);
            (void)jsPlayer->player_->SetParameter(format);
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    MEDIA_LOGD("JsSetAudioInterruptMode Out");
    return result;
}

napi_value AVPlayerNapi::JsGetAudioInterruptMode(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsGetAudioInterruptMode In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    napi_value value = nullptr;
    (void)napi_create_int32(env, static_cast<int32_t>(jsPlayer->interruptMode_), &value);
    MEDIA_LOGD("JsGetAudioInterruptMode Out");
    return value;
}

napi_value AVPlayerNapi::JsGetCurrentTime(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsGetCurrentTime In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    int32_t currentTime = -1;
    if (jsPlayer->IsControllable()) {
        currentTime = jsPlayer->position_;
    }

    napi_value value = nullptr;
    (void)napi_create_int32(env, currentTime, &value);
    MEDIA_LOGD("JsGetCurrenTime Out, Current time: %{public}d", currentTime);
    return value;
}

napi_value AVPlayerNapi::JsGetDuration(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsGetDuration In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    int32_t duration = -1;
    if (jsPlayer->IsControllable()) {
        duration = jsPlayer->duration_;
    }

    napi_value value = nullptr;
    (void)napi_create_int32(env, duration, &value);
    MEDIA_LOGD("JsGetDuration Out, Duration: %{public}d", duration);
    return value;
}

bool AVPlayerNapi::IsControllable()
{
    auto state = GetCurrentState();
    if (state == AVPlayerState::STATE_PREPARED || state == AVPlayerState::STATE_PLAYING ||
        state == AVPlayerState::STATE_PAUSED || state == AVPlayerState::STATE_COMPLETED) {
        return true;
    } else {
        return false;
    }
}

std::string AVPlayerNapi::GetCurrentState()
{
    std::string curState = AVPlayerState::STATE_IDLE;
    static const std::map<PlayerStates, std::string> stateMap = {
        {PLAYER_IDLE, AVPlayerState::STATE_IDLE},
        {PLAYER_INITIALIZED, AVPlayerState::STATE_INITIALIZED},
        {PLAYER_PREPARED, AVPlayerState::STATE_PREPARED},
        {PLAYER_STARTED, AVPlayerState::STATE_PLAYING},
        {PLAYER_PAUSED, AVPlayerState::STATE_PAUSED},
        {PLAYER_STOPPED, AVPlayerState::STATE_STOPPED},
        {PLAYER_PLAYBACK_COMPLETE, AVPlayerState::STATE_COMPLETED},
        {PLAYER_STATE_ERROR, AVPlayerState::STATE_ERROR},
    };

    if (stateMap.find(state_) != stateMap.end()) {
        curState = stateMap.at(state_);
    }

    if (isReleased_) {
        curState = AVPlayerState::STATE_RELEASED;
    }
    return curState;
}

napi_value AVPlayerNapi::JsGetState(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsGetState In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    std::string curState = jsPlayer->GetCurrentState();
    napi_value value = nullptr;
    (void)napi_create_string_utf8(env, curState.c_str(), NAPI_AUTO_LENGTH, &value);
    MEDIA_LOGD("JsGetState Out");
    return value;
}

napi_value AVPlayerNapi::JsGetWidth(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsGetWidth In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    napi_value value = nullptr;
    (void)napi_create_int32(env, jsPlayer->width_, &value);
    MEDIA_LOGD("JsGetWidth Out");
    return value;
}

napi_value AVPlayerNapi::JsGetHeight(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsGetHeight In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    napi_value value = nullptr;
    (void)napi_create_int32(env, jsPlayer->height_, &value);
    MEDIA_LOGD("JsGetHeight Out");
    return value;
}

napi_value AVPlayerNapi::JsGetTrackDescription(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("GetTrackDescription In");

    auto promiseCtx = std::make_unique<AVPlayerContext>(env);
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    promiseCtx->napi = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    promiseCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    promiseCtx->deferred = CommonNapi::CreatePromise(env, promiseCtx->callbackRef, result);

    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsGetTrackDescription", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void *data) {
            MEDIA_LOGD("GetTrackDescription Task");
            auto promiseCtx = reinterpret_cast<AVPlayerContext *>(data);
            CHECK_AND_RETURN_LOG(promiseCtx != nullptr, "promiseCtx is nullptr!");

            auto jsPlayer = promiseCtx->napi;
            if (jsPlayer == nullptr) {
                return promiseCtx->SignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "avplayer is deconstructed");
            }

            std::vector<Format> &trackInfo = jsPlayer->trackInfoVec_;
            trackInfo.clear();
            if (jsPlayer->IsControllable()) {
                (void)jsPlayer->player_->GetVideoTrackInfo(trackInfo);
                (void)jsPlayer->player_->GetAudioTrackInfo(trackInfo);
            } else {
                MEDIA_LOGW("current state is not support get track description");
            }
            promiseCtx->JsResult = std::make_unique<MediaJsResultArray>(trackInfo);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(promiseCtx.get()), &promiseCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, promiseCtx->work));
    promiseCtx.release();
    return result;
}

napi_value AVPlayerNapi::JsSetOnCallback(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("JsSetOnCallback In");

    napi_value args[2] = { nullptr }; // args[0]:type, args[1]:callback
    size_t argCount = 2; // args[0]:type, args[1]:callback
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        args[1] == nullptr || napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "napi_typeof failed, please check the input parameters");
        return result;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGD("set callbackName: %{public}s", callbackName.c_str());

    napi_ref ref = nullptr;
    napi_status status = napi_create_reference(env, args[1], 1, &ref);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && ref != nullptr, result, "failed to create reference!");

    std::shared_ptr<AutoRef> autoRef = std::make_shared<AutoRef>(env, ref);
    jsPlayer->SaveCallbackReference(callbackName, autoRef);

    MEDIA_LOGD("JsSetOnCallback Out");
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

void AVPlayerNapi::ClearCallbackReference()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (playerCb_ != nullptr) {
        std::shared_ptr<AVPlayerCallback> cb = std::static_pointer_cast<AVPlayerCallback>(playerCb_);
        cb->ClearCallbackReference();
    }
    refMap_.clear();
}

void AVPlayerNapi::NotifyDuration(int32_t duration)
{
    std::lock_guard<std::mutex> lock(mutex_);
    duration_ = duration;
}

void AVPlayerNapi::NotifyPosition(int32_t position)
{
    std::lock_guard<std::mutex> lock(mutex_);
    position_ = position;
}

void AVPlayerNapi::NotifyState(PlayerStates state)
{
    std::lock_guard<std::mutex> lock(mutex_);
    state_ = state;
    if (state_ == PLAYER_STATE_ERROR ||
        state_ == PLAYER_PREPARED) {
        preparingCond_.notify_all();
    }
}

void AVPlayerNapi::NotifyVideoSize(int32_t width, int32_t height)
{
    std::lock_guard<std::mutex> lock(mutex_);
    width_ = width;
    height_ = height;
}

void AVPlayerNapi::ResetUserParameters()
{
    std::lock_guard<std::mutex> lock(mutex_);
    url_.clear();
    fileDescriptor_.fd = 0;
    fileDescriptor_.offset = 0;
    fileDescriptor_.length = -1;
    width_ = 0;
    height_ = 0;
}

void AVPlayerNapi::StartListenCurrentResource()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (playerCb_ != nullptr) {
        std::shared_ptr<AVPlayerCallback> cb = std::static_pointer_cast<AVPlayerCallback>(playerCb_);
        cb->Start();
    }
}

void AVPlayerNapi::PauseListenCurrentResource()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (playerCb_ != nullptr) {
        std::shared_ptr<AVPlayerCallback> cb = std::static_pointer_cast<AVPlayerCallback>(playerCb_);
        cb->Pause();
    }
}

void AVPlayerNapi::OnErrorCb(MediaServiceExtErrCodeAPI9 errorCode, const std::string &errorMsg)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (playerCb_ != nullptr) {
        std::shared_ptr<AVPlayerCallback> cb = std::static_pointer_cast<AVPlayerCallback>(playerCb_);
        cb->OnErrorCb(errorCode, errorMsg);
    }
}

AVPlayerNapi* AVPlayerNapi::GetJsInstance(napi_env env, napi_callback_info info)
{
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, nullptr, "failed to napi_get_cb_info");

    AVPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&jsPlayer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsPlayer != nullptr, nullptr, "failed to napi_unwrap");

    return jsPlayer;
}

AVPlayerNapi* AVPlayerNapi::GetJsInstanceWithParameter(napi_env env, napi_callback_info info, size_t &argc, napi_value *argv)
{
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, argv, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, nullptr, "failed to napi_get_cb_info");

    AVPlayerNapi *jsPlayer = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&jsPlayer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsPlayer != nullptr, nullptr, "failed to napi_unwrap");

    return jsPlayer;
}
} // namespace Media
} // namespace OHOS