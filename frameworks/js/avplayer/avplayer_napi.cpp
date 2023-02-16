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
#ifdef SUPPORT_VIDEO
#include "surface_utils.h"
#endif
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
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVPlayerNapi::~AVPlayerNapi()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
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
        DECLARE_NAPI_FUNCTION("off", JsClearOnCallback),
        DECLARE_NAPI_FUNCTION("setVolume", JsSetVolume),
        DECLARE_NAPI_FUNCTION("setSpeed", JsSetSpeed),
        DECLARE_NAPI_FUNCTION("setBitrate", JsSelectBitrate),
        DECLARE_NAPI_FUNCTION("getTrackDescription", JsGetTrackDescription),

        DECLARE_NAPI_GETTER_SETTER("url", JsGetUrl, JsSetUrl),
        DECLARE_NAPI_GETTER_SETTER("fdSrc", JsGetAVFileDescriptor, JsSetAVFileDescriptor),
        DECLARE_NAPI_GETTER_SETTER("surfaceId", JsGetSurfaceID, JsSetSurfaceID),
        DECLARE_NAPI_GETTER_SETTER("loop", JsGetLoop, JsSetLoop),
        DECLARE_NAPI_GETTER_SETTER("videoScaleType", JsGetVideoScaleType, JsSetVideoScaleType),
        DECLARE_NAPI_GETTER_SETTER("audioInterruptMode", JsGetAudioInterruptMode, JsSetAudioInterruptMode),

        DECLARE_NAPI_GETTER("state", JsGetState),
        DECLARE_NAPI_GETTER("currentTime", JsGetCurrentTime),
        DECLARE_NAPI_GETTER("duration", JsGetDuration),
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

    MEDIA_LOGI("Init success");
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
        AVPlayerNapi::Destructor, nullptr, nullptr);
    if (status != napi_ok) {
        delete jsPlayer;
        MEDIA_LOGE("Failed to wrap native instance");
        return result;
    }

    MEDIA_LOGI("Constructor success");
    return jsThis;
}

void AVPlayerNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)finalize;
    if (nativeObject != nullptr) {
        AVPlayerNapi *jsPlayer = reinterpret_cast<AVPlayerNapi *>(nativeObject);
        jsPlayer->ClearCallbackReference();
        auto task = jsPlayer->ReleaseTask();
        if (task != nullptr) {
            MEDIA_LOGI("Destructor Wait Release Task Start");
            task->GetResult(); // sync release
            MEDIA_LOGI("Destructor Wait Release Task End");
        }
        jsPlayer->taskQue_->Stop();
        delete jsPlayer;
    }
    MEDIA_LOGI("Destructor success");
}

napi_value AVPlayerNapi::JsCreateAVPlayer(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsCreateAVPlayer In");

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
    MEDIA_LOGI("JsCreateAVPlayer Out");
    return result;
}

std::shared_ptr<TaskHandler<TaskRet>> AVPlayerNapi::PrepareTask()
{
    auto task = std::make_shared<TaskHandler<TaskRet>>([this]() {
        MEDIA_LOGI("Prepare Task In");
        std::unique_lock<std::mutex> lock(taskMutex_);

        auto state = GetCurrentState();
        if (state == AVPlayerState::STATE_INITIALIZED ||
            state == AVPlayerState::STATE_STOPPED) {
            int32_t ret = player_->PrepareAsync();
            if (ret != MSERR_OK) {
                auto errCode = MSErrorToExtErrorAPI9(static_cast<MediaServiceErrCode>(ret));
                return TaskRet(errCode, "failed to prepare");
            }
            preparingCond_.wait(lock, [this]() {
                auto state = GetCurrentState();
                return (state == AVPlayerState::STATE_PREPARED ||
                        state == AVPlayerState::STATE_ERROR ||
                        state == AVPlayerState::STATE_IDLE ||
                        state == AVPlayerState::STATE_RELEASED);
            });
            if (GetCurrentState() == AVPlayerState::STATE_ERROR) {
                return TaskRet(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
                    "failed to prepare, avplayer enter error status, please check error callback messages!");
            }
        } else if (state == AVPlayerState::STATE_PREPARED) {
            MEDIA_LOGI("current state is prepared, invalid operation");
        } else {
            return TaskRet(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
                "current state is not stopped or initialized, unsupport prepare operation");
        }

        MEDIA_LOGI("Prepare Task Out");
        return TaskRet(MSERR_EXT_API9_OK, "Success");
    });

    (void)taskQue_->EnqueueTask(task);
    return task;
}

napi_value AVPlayerNapi::JsPrepare(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsPrepare In");

    auto promiseCtx = std::make_unique<AVPlayerContext>(env);
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    promiseCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    promiseCtx->deferred = CommonNapi::CreatePromise(env, promiseCtx->callbackRef, result);
    auto state = jsPlayer->GetCurrentState();
    if (state != AVPlayerState::STATE_INITIALIZED &&
        state != AVPlayerState::STATE_STOPPED &&
        state != AVPlayerState::STATE_PREPARED) {
        promiseCtx->SignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
            "current state is not stopped or initialized, unsupport prepare operation");
    } else {
        promiseCtx->asyncTask = jsPlayer->PrepareTask();
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsPrepare", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void *data) {
            MEDIA_LOGI("Wait Prepare Task Start");
            auto promiseCtx = reinterpret_cast<AVPlayerContext *>(data);
            CHECK_AND_RETURN_LOG(promiseCtx != nullptr, "promiseCtx is nullptr!");
            promiseCtx->CheckTaskResult();
            MEDIA_LOGI("Wait Prepare Task End");
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(promiseCtx.get()), &promiseCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, promiseCtx->work));
    promiseCtx.release();
    MEDIA_LOGI("JsPrepare Out");
    return result;
}

std::shared_ptr<TaskHandler<TaskRet>> AVPlayerNapi::PlayTask()
{
    auto task = std::make_shared<TaskHandler<TaskRet>>([this]() {
        MEDIA_LOGI("Play Task In");
        std::unique_lock<std::mutex> lock(taskMutex_);

        auto state = GetCurrentState();
        if (state == AVPlayerState::STATE_PREPARED ||
            state == AVPlayerState::STATE_PAUSED ||
            state == AVPlayerState::STATE_COMPLETED) {
            int32_t ret = player_->Play();
            if (ret != MSERR_OK) {
                auto errCode = MSErrorToExtErrorAPI9(static_cast<MediaServiceErrCode>(ret));
                return TaskRet(errCode, "failed to Play");
            }
            stateChangeCond_.wait(lock, [this]() {
                auto state = GetCurrentState();
                return (state == AVPlayerState::STATE_PLAYING ||
                        state == AVPlayerState::STATE_COMPLETED ||
                        state == AVPlayerState::STATE_ERROR ||
                        state == AVPlayerState::STATE_IDLE ||
                        state == AVPlayerState::STATE_RELEASED);
            });
        } else if (state == AVPlayerState::STATE_PLAYING) {
            MEDIA_LOGI("current state is playing, invalid operation");
        } else {
            return TaskRet(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
                "current state is not prepared/paused/completed, unsupport play operation");
        }

        MEDIA_LOGI("Play Task Out");
        return TaskRet(MSERR_EXT_API9_OK, "Success");
    });
    (void)taskQue_->EnqueueTask(task);
    return task;
}

napi_value AVPlayerNapi::JsPlay(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsPlay In");

    auto promiseCtx = std::make_unique<AVPlayerContext>(env);
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    promiseCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    promiseCtx->deferred = CommonNapi::CreatePromise(env, promiseCtx->callbackRef, result);
    auto state = jsPlayer->GetCurrentState();
    if (state != AVPlayerState::STATE_PREPARED &&
        state != AVPlayerState::STATE_PAUSED &&
        state != AVPlayerState::STATE_COMPLETED &&
        state != AVPlayerState::STATE_PLAYING) {
        promiseCtx->SignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
            "current state is not prepared/paused/completed, unsupport play operation");
    } else {
        promiseCtx->asyncTask = jsPlayer->PlayTask();
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsPlay", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void *data) {
            MEDIA_LOGI("Wait JsPlay Task Start");
            auto promiseCtx = reinterpret_cast<AVPlayerContext *>(data);
            CHECK_AND_RETURN_LOG(promiseCtx != nullptr, "promiseCtx is nullptr!");
            promiseCtx->CheckTaskResult();
            MEDIA_LOGI("Wait JsPlay Task End");
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(promiseCtx.get()), &promiseCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, promiseCtx->work));
    promiseCtx.release();
    MEDIA_LOGI("JsPlay Out");
    return result;
}

std::shared_ptr<TaskHandler<TaskRet>> AVPlayerNapi::PauseTask()
{
    auto task = std::make_shared<TaskHandler<TaskRet>>([this]() {
        MEDIA_LOGI("Pause Task In");
        std::unique_lock<std::mutex> lock(taskMutex_);

        auto state = GetCurrentState();
        if (state == AVPlayerState::STATE_PLAYING) {
            int32_t ret = player_->Pause();
            if (ret != MSERR_OK) {
                auto errCode = MSErrorToExtErrorAPI9(static_cast<MediaServiceErrCode>(ret));
                return TaskRet(errCode, "failed to Pause");
            }
            stateChangeCond_.wait(lock, [this]() {
                auto state = GetCurrentState();
                return (state == AVPlayerState::STATE_PAUSED ||
                        state == AVPlayerState::STATE_COMPLETED ||
                        state == AVPlayerState::STATE_ERROR ||
                        state == AVPlayerState::STATE_IDLE ||
                        state == AVPlayerState::STATE_RELEASED);
            });
        } else if (state == AVPlayerState::STATE_PAUSED) {
            MEDIA_LOGI("current state is paused, invalid operation");
        } else {
            return TaskRet(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
                "current state is not playing, unsupport pause operation");
        }

        MEDIA_LOGI("Pause Task Out");
        return TaskRet(MSERR_EXT_API9_OK, "Success");
    });
    (void)taskQue_->EnqueueTask(task);
    return task;
}

napi_value AVPlayerNapi::JsPause(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsPause In");

    auto promiseCtx = std::make_unique<AVPlayerContext>(env);
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    promiseCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    promiseCtx->deferred = CommonNapi::CreatePromise(env, promiseCtx->callbackRef, result);
    auto state = jsPlayer->GetCurrentState();
    if (state != AVPlayerState::STATE_PLAYING &&
        state != AVPlayerState::STATE_PAUSED) {
        promiseCtx->SignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
            "current state is not playing, unsupport pause operation");
    } else {
        promiseCtx->asyncTask = jsPlayer->PauseTask();
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsPause", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void *data) {
            MEDIA_LOGI("Wait JsPause Task Start");
            auto promiseCtx = reinterpret_cast<AVPlayerContext *>(data);
            CHECK_AND_RETURN_LOG(promiseCtx != nullptr, "promiseCtx is nullptr!");
            promiseCtx->CheckTaskResult();
            MEDIA_LOGI("Wait JsPause Task End");
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(promiseCtx.get()), &promiseCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, promiseCtx->work));
    promiseCtx.release();
    MEDIA_LOGI("JsPause Out");
    return result;
}

std::shared_ptr<TaskHandler<TaskRet>> AVPlayerNapi::StopTask()
{
    auto task = std::make_shared<TaskHandler<TaskRet>>([this]() {
        MEDIA_LOGI("Stop Task In");
        std::unique_lock<std::mutex> lock(taskMutex_);

        if (IsControllable()) {
            int32_t ret = player_->Stop();
            if (ret != MSERR_OK) {
                auto errCode = MSErrorToExtErrorAPI9(static_cast<MediaServiceErrCode>(ret));
                return TaskRet(errCode, "failed to Stop");
            }
            stateChangeCond_.wait(lock, [this]() {
                auto state = GetCurrentState();
                return (state == AVPlayerState::STATE_STOPPED ||
                        state == AVPlayerState::STATE_ERROR ||
                        state == AVPlayerState::STATE_IDLE ||
                        state == AVPlayerState::STATE_RELEASED);
            });
        } else if (GetCurrentState() == AVPlayerState::STATE_STOPPED) {
            MEDIA_LOGI("current state is stopped, invalid operation");
        }  else {
            return TaskRet(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
                "current state is not prepared/playing/paused/completed, unsupport stop operation");
        }

        MEDIA_LOGI("Stop Task Out");
        return TaskRet(MSERR_EXT_API9_OK, "Success");
    });
    (void)taskQue_->EnqueueTask(task);
    return task;
}

napi_value AVPlayerNapi::JsStop(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsStop In");

    auto promiseCtx = std::make_unique<AVPlayerContext>(env);
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    promiseCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    promiseCtx->deferred = CommonNapi::CreatePromise(env, promiseCtx->callbackRef, result);
    auto state = jsPlayer->GetCurrentState();
    if (state == AVPlayerState::STATE_IDLE ||
        state == AVPlayerState::STATE_INITIALIZED ||
        state == AVPlayerState::STATE_RELEASED ||
        state == AVPlayerState::STATE_ERROR) {
        promiseCtx->SignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
            "current state is not prepared/playing/paused/completed, unsupport stop operation");
    } else {
        promiseCtx->asyncTask = jsPlayer->StopTask();
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsStop", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void *data) {
            MEDIA_LOGI("Wait JsStop Task Start");
            auto promiseCtx = reinterpret_cast<AVPlayerContext *>(data);
            CHECK_AND_RETURN_LOG(promiseCtx != nullptr, "promiseCtx is nullptr!");
            promiseCtx->CheckTaskResult();
            MEDIA_LOGI("Wait JsStop Task End");
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(promiseCtx.get()), &promiseCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, promiseCtx->work));
    promiseCtx.release();
    MEDIA_LOGI("JsStop Out");
    return result;
}

std::shared_ptr<TaskHandler<TaskRet>> AVPlayerNapi::ResetTask()
{
    auto task = std::make_shared<TaskHandler<TaskRet>>([this]() {
        MEDIA_LOGI("Reset Task In");
        PauseListenCurrentResource(); // Pause event listening for the current resource
        ResetUserParameters();
        {
            std::unique_lock<std::mutex> lock(taskMutex_);
            if (GetCurrentState() == AVPlayerState::STATE_RELEASED) {
                return TaskRet(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
                    "current state is not playing, unsupport pause operation");
            } else if (GetCurrentState() == AVPlayerState::STATE_IDLE) {
                MEDIA_LOGI("current state is idle, invalid operation");
            } else {
                int32_t ret = player_->Reset();
                if (ret != MSERR_OK) {
                    auto errCode = MSErrorToExtErrorAPI9(static_cast<MediaServiceErrCode>(ret));
                    return TaskRet(errCode, "failed to Reset");
                }
                resettingCond_.wait(lock, [this]() {
                    auto state = GetCurrentState();
                    return state == AVPlayerState::STATE_IDLE || state == AVPlayerState::STATE_RELEASED;
                });
            }
        }
        MEDIA_LOGI("Reset Task Out");
        return TaskRet(MSERR_EXT_API9_OK, "Success");
    });

    {
        std::unique_lock<std::mutex> lock(taskMutex_);
        (void)taskQue_->EnqueueTask(task, true); // CancelNotExecutedTask
        preparingCond_.notify_all(); // stop prepare
        stateChangeCond_.notify_all(); // stop play/pause/stop
    }
    return task;
}

napi_value AVPlayerNapi::JsReset(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsReset In");

    auto promiseCtx = std::make_unique<AVPlayerContext>(env);
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");
    promiseCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    promiseCtx->deferred = CommonNapi::CreatePromise(env, promiseCtx->callbackRef, result);
    if (jsPlayer->GetCurrentState() == AVPlayerState::STATE_RELEASED) {
        promiseCtx->SignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
            "current state is released, unsupport reset operation");
    } else {
        promiseCtx->asyncTask = jsPlayer->ResetTask();
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsReset", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void *data) {
            auto promiseCtx = reinterpret_cast<AVPlayerContext *>(data);
            CHECK_AND_RETURN_LOG(promiseCtx != nullptr, "promiseCtx is nullptr!");
            if (promiseCtx->asyncTask != nullptr) {
                MEDIA_LOGI("Wait Reset Task Start");
                promiseCtx->CheckTaskResult();
                MEDIA_LOGI("Wait Reset Task Stop");
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(promiseCtx.get()), &promiseCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, promiseCtx->work));
    promiseCtx.release();
    MEDIA_LOGI("JsReset Out");
    return result;
}

std::shared_ptr<TaskHandler<TaskRet>> AVPlayerNapi::ReleaseTask()
{
    std::shared_ptr<TaskHandler<TaskRet>> task = nullptr;
    if (!isReleased_.load()) {
        task = std::make_shared<TaskHandler<TaskRet>>([this]() {
            MEDIA_LOGI("Release Task In");
            PauseListenCurrentResource(); // Pause event listening for the current resource
            ResetUserParameters();

            if (player_ != nullptr) {
                (void)player_->ReleaseSync();
                player_ = nullptr;
            }

            if (playerCb_ != nullptr) {
                playerCb_->Release();
            }
            MEDIA_LOGI("Release Task Out");
            return TaskRet(MSERR_EXT_API9_OK, "Success");
        });

        std::unique_lock<std::mutex> lock(taskMutex_);
        isReleased_.store(true);
        (void)taskQue_->EnqueueTask(task, true); // CancelNotExecutedTask
        preparingCond_.notify_all(); // stop wait prepare
        resettingCond_.notify_all(); // stop wait reset
        stateChangeCond_.notify_all(); // stop wait play/pause/stop
    }
    return task;
}

napi_value AVPlayerNapi::JsRelease(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsRelease In");

    auto promiseCtx = std::make_unique<AVPlayerContext>(env);
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");
    promiseCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    promiseCtx->deferred = CommonNapi::CreatePromise(env, promiseCtx->callbackRef, result);
    promiseCtx->asyncTask = jsPlayer->ReleaseTask();

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JsRelease", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void *data) {
            auto promiseCtx = reinterpret_cast<AVPlayerContext *>(data);
            CHECK_AND_RETURN_LOG(promiseCtx != nullptr, "promiseCtx is nullptr!");
            if (promiseCtx->asyncTask != nullptr) {
                MEDIA_LOGI("Wait Release Task Start");
                promiseCtx->CheckTaskResult();
                MEDIA_LOGI("Wait Release Task Stop");
            }
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(promiseCtx.get()), &promiseCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, promiseCtx->work));
    promiseCtx.release();
    MEDIA_LOGI("JsRelease Out");
    return result;
}

napi_value AVPlayerNapi::JsSeek(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsSeek In");

    napi_value args[2] = { nullptr }; // args[0]:timeMs, args[1]:SeekMode
    size_t argCount = 2; // args[0]:timeMs, args[1]:SeekMode
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "seek time is not number");
        return result;
    }

    int32_t time = -1;
    napi_status status = napi_get_value_int32(env, args[0], &time);
    if (status != napi_ok || time < 0) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER,
            "invalid parameters, please check the input seek time");
        return result;
    }

    int32_t mode = SEEK_PREVIOUS_SYNC;
    if (args[1] != nullptr) {
        if (napi_typeof(env, args[1], &valueType) != napi_ok || valueType != napi_number) {
            jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "seek mode is not number");
            return result;
        }
        status = napi_get_value_int32(env, args[1], &mode);
        if (status != napi_ok || mode < SEEK_NEXT_SYNC || mode > SEEK_CLOSEST) {
            jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER,
                "invalid parameters, please check the input seek mode");
            return result;
        }
    }

    if (!jsPlayer->IsControllable()) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
            "current state is not prepared/playing/paused/completed, unsupport seek operation");
        return result;
    }

    auto task = std::make_shared<TaskHandler<void>>([jsPlayer, time, mode]() {
        MEDIA_LOGI("Seek Task");
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
    MEDIA_LOGI("JsSetSpeed In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1; // setSpeed(speed: number)
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "speed mode is not number");
        return result;
    }

    int32_t mode = SPEED_FORWARD_1_00_X;
    napi_status status = napi_get_value_int32(env, args[0], &mode);
    if (status != napi_ok || mode < SPEED_FORWARD_0_75_X || mode > SPEED_FORWARD_2_00_X) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER,
            "invalid parameters, please check the speed mode");
        return result;
    }

    if (!jsPlayer->IsControllable()) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
            "current state is not prepared/playing/paused/completed, unsupport speed operation");
        return result;
    }

    auto task = std::make_shared<TaskHandler<void>>([jsPlayer, mode]() {
        MEDIA_LOGI("Speed Task");
        if (jsPlayer->player_ != nullptr) {
            (void)jsPlayer->player_->SetPlaybackSpeed(static_cast<PlaybackRateMode>(mode));
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    MEDIA_LOGI("JsSetSpeed Out");
    return result;
}

napi_value AVPlayerNapi::JsSetVolume(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsSetVolume In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1; // setVolume(vol: number)
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "setVolume level is not number");
        return result;
    }

    double volumeLevel = 1.0f;
    napi_status status = napi_get_value_double(env, args[0], &volumeLevel);
    if (status != napi_ok || volumeLevel < 0.0f || volumeLevel > 1.0f) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "invalid parameters, check volume level");
        return result;
    }

    if (!jsPlayer->IsControllable()) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
            "current state is not prepared/playing/paused/completed, unsupport volume operation");
        return result;
    }

    auto task = std::make_shared<TaskHandler<void>>([jsPlayer, volumeLevel]() {
        MEDIA_LOGI("SetVolume Task");
        if (jsPlayer->player_ != nullptr) {
            (void)jsPlayer->player_->SetVolume(volumeLevel, volumeLevel);
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    MEDIA_LOGI("JsSetVolume Out");
    return result;
}

napi_value AVPlayerNapi::JsSelectBitrate(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsSelectBitrate In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1; // selectBitrate(bitRate: number)
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "selectBitrate is not number");
        return result;
    }

    int32_t bitrate = 0;
    napi_status status = napi_get_value_int32(env, args[0], &bitrate);
    if (status != napi_ok || bitrate < 0) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "invalid parameters, please check the input bitrate");
        return result;
    }

    if (!jsPlayer->IsControllable()) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
            "current state is not prepared/playing/paused/completed, unsupport select bitrate operation");
        return result;
    }

    auto task = std::make_shared<TaskHandler<void>>([jsPlayer, bitrate]() {
        MEDIA_LOGI("SelectBitRate Task");
        if (jsPlayer->player_ != nullptr) {
            (void)jsPlayer->player_->SelectBitRate(static_cast<uint32_t>(bitrate));
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    return result;
}

void AVPlayerNapi::SetSource(std::string url)
{
    MEDIA_LOGI("input url is %{public}s!", url.c_str());
    bool isFd = (url.find("fd://") != std::string::npos) ? true : false;
    bool isNetwork = (url.find("http") != std::string::npos) ? true : false;
    if (isNetwork) {
        auto task = std::make_shared<TaskHandler<void>>([this, url]() {
            MEDIA_LOGI("SetNetworkSource Task");
            if (player_ != nullptr) {
                if (player_->SetSource(url) != MSERR_OK) {
                    OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "failed to SetSourceNetWork");
                }
            }
        });
        (void)taskQue_->EnqueueTask(task);
        task->GetResult();
    } else if (isFd) {
        const std::string fdHead = "fd://";
        std::string inputFd = url.substr(fdHead.size());
        int32_t fd = -1;
        if (!StrToInt(inputFd, fd) || fd < 0) {
            OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER,
                "invalid parameters, The input parameter is not a fd://+numeric string");
            return;
        }

        auto task = std::make_shared<TaskHandler<void>>([this, fd]() {
            MEDIA_LOGI("SetFdSource Task");
            if (player_ != nullptr) {
                if (player_->SetSource(fd, 0, -1) != MSERR_OK) {
                    OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "failed to SetSourceFd");
                }
            }
        });
        (void)taskQue_->EnqueueTask(task);
        task->GetResult();
    } else {
        OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER,
            "invalid parameters, The input parameter is not fd:// or network address");
    }
}

napi_value AVPlayerNapi::JsSetUrl(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsSetUrl In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1; // url: string
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    if (jsPlayer->GetCurrentState() != AVPlayerState::STATE_IDLE) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not idle, unsupport set url");
        return result;
    }

    jsPlayer->StartListenCurrentResource(); // Listen to the events of the current resource
    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_string) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "url is not string");
        return result;
    }

    // get url from js
    jsPlayer->url_ = CommonNapi::GetStringArgument(env, args[0]);
    jsPlayer->SetSource(jsPlayer->url_);

    MEDIA_LOGI("JsSetUrl Out");
    return result;
}

napi_value AVPlayerNapi::JsGetUrl(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsGetUrl In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    napi_value value = nullptr;
    (void)napi_create_string_utf8(env, jsPlayer->url_.c_str(), NAPI_AUTO_LENGTH, &value);

    MEDIA_LOGI("JsGetUrl Out Current Url: %{public}s", jsPlayer->url_.c_str());
    return value;
}

napi_value AVPlayerNapi::JsSetAVFileDescriptor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsSetAVFileDescriptor In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1; // url: string
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    if (jsPlayer->GetCurrentState() != AVPlayerState::STATE_IDLE) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is not idle, unsupport set fd");
        return result;
    }

    jsPlayer->StartListenCurrentResource(); // Listen to the events of the current resource
    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_object) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "SetAVFileDescriptor is not napi_object");
        return result;
    }

    if (!CommonNapi::GetFdArgument(env, args[0], jsPlayer->fileDescriptor_)) {
        MEDIA_LOGE("get fileDescriptor argument failed!");
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER,
            "invalid parameters, please check the input parameters(fileDescriptor)");
        return result;
    }

    auto task = std::make_shared<TaskHandler<void>>([jsPlayer]() {
        MEDIA_LOGI("SetAVFileDescriptor Task");
        if (jsPlayer->player_ != nullptr) {
            auto playerFd = jsPlayer->fileDescriptor_;
            if (jsPlayer->player_->SetSource(playerFd.fd, playerFd.offset, playerFd.length) != MSERR_OK) {
                jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "player SetSource FileDescriptor failed");
            }
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    task->GetResult();

    MEDIA_LOGI("JsSetAVFileDescriptor Out");
    return result;
}

napi_value AVPlayerNapi::JsGetAVFileDescriptor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsGetAVFileDescriptor In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    napi_value value = nullptr;
    (void)napi_create_object(env, &value);
    (void)CommonNapi::AddNumberPropInt32(env, value, "fd", jsPlayer->fileDescriptor_.fd);
    (void)CommonNapi::AddNumberPropInt64(env, value, "offset", jsPlayer->fileDescriptor_.offset);
    (void)CommonNapi::AddNumberPropInt64(env, value, "length", jsPlayer->fileDescriptor_.length);

    MEDIA_LOGI("JsGetAVFileDescriptor Out");
    return value;
}

#ifdef SUPPORT_VIDEO
void AVPlayerNapi::SetSurface(const std::string &surfaceStr)
{
    MEDIA_LOGI("get surface, surfaceStr = %{public}s", surfaceStr.c_str());
    uint64_t surfaceId = 0;
    if (surfaceStr.empty() || surfaceStr[0] < '0' || surfaceStr[0] > '9') {
        OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER,
            "Please obtain the surface from XComponentController.getXComponentSurfaceId");
        return;
    }
    surfaceId = std::stoull(surfaceStr);
    MEDIA_LOGI("get surface, surfaceId = (%{public}" PRIu64 ")", surfaceId);

    auto surface = SurfaceUtils::GetInstance()->GetSurface(surfaceId);
    if (surface == nullptr) {
        OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "SurfaceUtils cannot convert ID to Surface");
        return;
    }

    auto task = std::make_shared<TaskHandler<void>>([this, surface]() {
        MEDIA_LOGI("SetSurface Task");
        if (player_ != nullptr) {
            (void)player_->SetVideoSurface(surface);
        }
    });
    (void)taskQue_->EnqueueTask(task);
    task->GetResult();
}
#else
void AVPlayerNapi::SetSurface(const std::string &surfaceStr)
{
    (void)surfaceStr;
    OnErrorCb(MSERR_EXT_API9_UNSUPPORT_CAPABILITY, "The music player does not need to support (Surface)");
}
#endif

napi_value AVPlayerNapi::JsSetSurfaceID(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsSetSurfaceID In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1; // surfaceId?: string
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_string) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "the attribute(SurfaceID) input is not string");
        return result;
    }

    if (jsPlayer->GetCurrentState() != AVPlayerState::STATE_INITIALIZED) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
            "the attribute(SurfaceID) can only be set in the initialized state");
        return result;
    }

    // get url from js
    jsPlayer->surface_ = CommonNapi::GetStringArgument(env, args[0]);
    jsPlayer->SetSurface(jsPlayer->surface_);
    MEDIA_LOGI("JsSetSurfaceID Out");
    return result;
}

napi_value AVPlayerNapi::JsGetSurfaceID(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsGetSurfaceID In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    napi_value value = nullptr;
    (void)napi_create_string_utf8(env, jsPlayer->surface_.c_str(), NAPI_AUTO_LENGTH, &value);

    MEDIA_LOGI("JsGetSurfaceID Out Current SurfaceID: %{public}s", jsPlayer->surface_.c_str());
    return value;
}

napi_value AVPlayerNapi::JsSetLoop(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsSetLoop In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1; // loop: boolenan
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    if (!jsPlayer->IsControllable()) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
            "current state is not prepared/playing/paused/completed, unsupport loop operation");
        return result;
    }

    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_boolean) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "SetLoop is not napi_boolean");
        return result;
    }

    napi_status status = napi_get_value_bool(env, args[0], &jsPlayer->loop_);
    if (status != napi_ok) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER,
            "invalid parameters, please check the input loop");
        return result;
    }

    auto task = std::make_shared<TaskHandler<void>>([jsPlayer]() {
        MEDIA_LOGI("SetLooping Task");
        if (jsPlayer->player_ != nullptr) {
            (void)jsPlayer->player_->SetLooping(jsPlayer->loop_);
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    MEDIA_LOGI("JsSetLoop Out");
    return result;
}

napi_value AVPlayerNapi::JsGetLoop(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsGetLoop In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    napi_value value = nullptr;
    (void)napi_get_boolean(env, jsPlayer->loop_, &value);
    MEDIA_LOGI("JsGetLoop Out Current Loop: %{public}d", jsPlayer->loop_);
    return value;
}

napi_value AVPlayerNapi::JsSetVideoScaleType(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsSetVideoScaleType In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    if (!jsPlayer->IsControllable()) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
            "current state is not prepared/playing/paused/completed, unsupport video scale operation");
        return result;
    }

    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "SetVideoScaleType is not napi_number");
        return result;
    }

    int32_t videoScaleType = 0;
    napi_status status = napi_get_value_int32(env, args[0], &videoScaleType);
    if (status != napi_ok || videoScaleType < VIDEO_SCALE_TYPE_FIT || videoScaleType > VIDEO_SCALE_TYPE_FIT_CROP) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "invalid parameters, please check the input scale type");
        return result;
    }
    jsPlayer->videoScaleType_ = videoScaleType;

    auto task = std::make_shared<TaskHandler<void>>([jsPlayer, videoScaleType]() {
        MEDIA_LOGI("SetVideoScaleType Task");
        if (jsPlayer->player_ != nullptr) {
            Format format;
            (void)format.PutIntValue(PlayerKeys::VIDEO_SCALE_TYPE, videoScaleType);
            (void)jsPlayer->player_->SetParameter(format);
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    MEDIA_LOGI("JsSetVideoScaleType Out");
    return result;
}

napi_value AVPlayerNapi::JsGetVideoScaleType(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsGetVideoScaleType In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    napi_value value = nullptr;
    (void)napi_create_int32(env, static_cast<int32_t>(jsPlayer->videoScaleType_), &value);
    MEDIA_LOGI("JsGetVideoScaleType Out Current VideoScale: %{public}d", jsPlayer->videoScaleType_);
    return value;
}

napi_value AVPlayerNapi::JsSetAudioInterruptMode(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsSetAudioInterruptMode In");

    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    if (!jsPlayer->IsControllable()) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
            "current state is not prepared/playing/paused/completed, unsupport audio interrupt operation");
        return result;
    }

    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "SetAudioInterruptMode is not napi_number");
        return result;
    }

    int32_t interruptMode = 0;
    napi_status status = napi_get_value_int32(env, args[0], &interruptMode);
    if (status != napi_ok ||
        interruptMode < AudioStandard::InterruptMode::SHARE_MODE ||
        interruptMode > AudioStandard::InterruptMode::INDEPENDENT_MODE) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER,
            "invalid parameters, please check the input interrupt Mode");
        return result;
    }
    jsPlayer->interruptMode_ = static_cast<AudioStandard::InterruptMode>(interruptMode);

    auto task = std::make_shared<TaskHandler<void>>([jsPlayer]() {
        MEDIA_LOGI("SetAudioInterruptMode Task");
        if (jsPlayer->player_ != nullptr) {
            Format format;
            (void)format.PutIntValue(PlayerKeys::AUDIO_INTERRUPT_MODE, jsPlayer->interruptMode_);
            (void)jsPlayer->player_->SetParameter(format);
        }
    });
    (void)jsPlayer->taskQue_->EnqueueTask(task);
    MEDIA_LOGI("JsSetAudioInterruptMode Out");
    return result;
}

napi_value AVPlayerNapi::JsGetAudioInterruptMode(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsGetAudioInterruptMode In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    napi_value value = nullptr;
    (void)napi_create_int32(env, static_cast<int32_t>(jsPlayer->interruptMode_), &value);
    MEDIA_LOGI("JsGetAudioInterruptMode Out");
    return value;
}

napi_value AVPlayerNapi::JsGetCurrentTime(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsGetCurrentTime In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    int32_t currentTime = -1;
    if (jsPlayer->IsControllable()) {
        currentTime = jsPlayer->position_;
    }

    napi_value value = nullptr;
    (void)napi_create_int32(env, currentTime, &value);
    std::string curState = jsPlayer->GetCurrentState();
    MEDIA_LOGI("JsGetCurrenTime Out, state %{public}s, time: %{public}d", curState.c_str(), currentTime);
    return value;
}

napi_value AVPlayerNapi::JsGetDuration(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsGetDuration In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    int32_t duration = -1;
    if (jsPlayer->IsControllable()) {
        duration = jsPlayer->duration_;
    }

    napi_value value = nullptr;
    (void)napi_create_int32(env, duration, &value);
    std::string curState = jsPlayer->GetCurrentState();
    MEDIA_LOGI("JsGetDuration Out, state %{public}s, duration %{public}d", curState.c_str(), duration);
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
    if (isReleased_.load()) {
        return AVPlayerState::STATE_RELEASED;
    } else {
        std::string curState = AVPlayerState::STATE_ERROR;
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
        return curState;
    }
}

napi_value AVPlayerNapi::JsGetState(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsGetState In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    std::string curState = jsPlayer->GetCurrentState();
    napi_value value = nullptr;
    (void)napi_create_string_utf8(env, curState.c_str(), NAPI_AUTO_LENGTH, &value);
    MEDIA_LOGI("JsGetState Out");
    return value;
}

napi_value AVPlayerNapi::JsGetWidth(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsGetWidth In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    int32_t width = 0;
    if (jsPlayer->IsControllable()) {
        width = jsPlayer->width_;
    }

    napi_value value = nullptr;
    (void)napi_create_int32(env, width, &value);
    MEDIA_LOGI("JsGetWidth Out");
    return value;
}

napi_value AVPlayerNapi::JsGetHeight(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsGetHeight In");

    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstance(env, info);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstance");

    int32_t height = 0;
    if (jsPlayer->IsControllable()) {
        height = jsPlayer->height_;
    }

    napi_value value = nullptr;
    (void)napi_create_int32(env, height, &value);
    MEDIA_LOGI("JsGetHeight Out");
    return value;
}

napi_value AVPlayerNapi::JsGetTrackDescription(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("GetTrackDescription In");

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
            MEDIA_LOGI("GetTrackDescription Task");
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
                return promiseCtx->SignError(MSERR_EXT_API9_OPERATE_NOT_PERMIT,
                    "current state unsupport get track description");
            }
            promiseCtx->JsResult = std::make_unique<MediaJsResultArray>(trackInfo);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(promiseCtx.get()), &promiseCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, promiseCtx->work));
    promiseCtx.release();
    MEDIA_LOGI("GetTrackDescription Out");
    return result;
}

napi_value AVPlayerNapi::JsSetOnCallback(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsSetOnCallback In");

    napi_value args[2] = { nullptr }; // args[0]:type, args[1]:callback
    size_t argCount = 2; // args[0]:type, args[1]:callback
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    if (jsPlayer->GetCurrentState() == AVPlayerState::STATE_RELEASED) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is released, unsupport to on event");
        return result;
    }

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        args[1] == nullptr || napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "napi_typeof failed, please check the input parameters");
        return result;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGI("set callbackName: %{public}s", callbackName.c_str());

    napi_ref ref = nullptr;
    napi_status status = napi_create_reference(env, args[1], 1, &ref);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && ref != nullptr, result, "failed to create reference!");

    std::shared_ptr<AutoRef> autoRef = std::make_shared<AutoRef>(env, ref);
    jsPlayer->SaveCallbackReference(callbackName, autoRef);

    MEDIA_LOGI("JsSetOnCallback Out");
    return result;
}

napi_value AVPlayerNapi::JsClearOnCallback(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGI("JsClearOnCallback In");

    napi_value args[2] = { nullptr }; // args[0]:type, args[1]:callback
    size_t argCount = 2; // args[0]:type, args[1]:callback
    AVPlayerNapi *jsPlayer = AVPlayerNapi::GetJsInstanceWithParameter(env, info, argCount, args);
    CHECK_AND_RETURN_RET_LOG(jsPlayer != nullptr, result, "failed to GetJsInstanceWithParameter");

    if (jsPlayer->GetCurrentState() == AVPlayerState::STATE_RELEASED) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_OPERATE_NOT_PERMIT, "current state is released, unsupport to off event");
        return result;
    }

    napi_valuetype valueType0 = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string) {
        jsPlayer->OnErrorCb(MSERR_EXT_API9_INVALID_PARAMETER, "napi_typeof failed, please check the input parameters");
        return result;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGI("set callbackName: %{public}s", callbackName.c_str());

    jsPlayer->ClearCallbackReference(callbackName);
    MEDIA_LOGI("JsClearOnCallback Out");
    return result;
}

void AVPlayerNapi::SaveCallbackReference(const std::string &callbackName, std::shared_ptr<AutoRef> ref)
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_[callbackName] = ref;
    if (playerCb_ != nullptr) {
        playerCb_->SaveCallbackReference(callbackName, ref);
    }
}

void AVPlayerNapi::ClearCallbackReference()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (playerCb_ != nullptr) {
        playerCb_->ClearCallbackReference();
    }
    refMap_.clear();
}

void AVPlayerNapi::ClearCallbackReference(const std::string &callbackName)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (playerCb_ != nullptr) {
        playerCb_->ClearCallbackReference(callbackName);
    }
    refMap_.erase(callbackName);
}

void AVPlayerNapi::NotifyDuration(int32_t duration)
{
    duration_ = duration;
}

void AVPlayerNapi::NotifyPosition(int32_t position)
{
    position_ = position;
}

void AVPlayerNapi::NotifyState(PlayerStates state)
{
    std::lock_guard<std::mutex> lock(taskMutex_);
    if (state_ != state) {
        state_ = state;
        MEDIA_LOGI("notify %{public}s OK", GetCurrentState().c_str());
        switch (state_) {
            case PLAYER_PREPARED:
                preparingCond_.notify_all();
                break;
            case PLAYER_IDLE:
                resettingCond_.notify_all();
                break;
            case PLAYER_STARTED:
            case PLAYER_PAUSED:
            case PLAYER_STOPPED:
            case PLAYER_PLAYBACK_COMPLETE:
                stateChangeCond_.notify_all();
                break;
            case PLAYER_STATE_ERROR:
                preparingCond_.notify_all();
                resettingCond_.notify_all();
                stateChangeCond_.notify_all();
                break;
            default:
                break;
        }
    }
}

void AVPlayerNapi::NotifyVideoSize(int32_t width, int32_t height)
{
    width_ = width;
    height_ = height;
}

void AVPlayerNapi::ResetUserParameters()
{
    url_.clear();
    fileDescriptor_.fd = 0;
    fileDescriptor_.offset = 0;
    fileDescriptor_.length = -1;
    width_ = 0;
    height_ = 0;
    position_ = -1;
    duration_ = -1;
    loop_ = false;
}

void AVPlayerNapi::StartListenCurrentResource()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (playerCb_ != nullptr) {
        playerCb_->Start();
    }
}

void AVPlayerNapi::PauseListenCurrentResource()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (playerCb_ != nullptr) {
        playerCb_->Pause();
    }
}

void AVPlayerNapi::OnErrorCb(MediaServiceExtErrCodeAPI9 errorCode, const std::string &errorMsg)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (playerCb_ != nullptr) {
        playerCb_->OnErrorCb(errorCode, errorMsg);
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

AVPlayerNapi* AVPlayerNapi::GetJsInstanceWithParameter(napi_env env, napi_callback_info info,
    size_t &argc, napi_value *argv)
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