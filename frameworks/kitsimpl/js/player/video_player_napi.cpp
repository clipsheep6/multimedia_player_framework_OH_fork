
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

#include "video_player_napi.h"
#include <climits>
#include "video_callback_napi.h"
#include "media_log.h"
#include "media_errors.h"
#include "media_data_source_napi.h"
#include "media_data_source_callback.h"
#include "media_description.h"
#include "common_napi.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoPlayerNapi"};
}

namespace OHOS {
namespace Media {
namespace VideoPlayState {
const std::string STATE_IDLE = "idle";
const std::string STATE_PREPARED = "prepared";
const std::string STATE_PLAYING = "playing";
const std::string STATE_PAUSED = "paused";
const std::string STATE_STOPPED = "stopped";
const std::string STATE_ERROR = "error";
};
napi_ref VideoPlayerNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "VideoPlayer";

VideoPlayerNapi::VideoPlayerNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

VideoPlayerNapi::~VideoPlayerNapi()
{
    callbackNapi_ = nullptr;
    nativePlayer_ = nullptr;
    dataSrcCallBack_ = nullptr;
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value VideoPlayerNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createVideoPlayer", CreateVideoPlayer),
    };

    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("prepare", Prepare),
        DECLARE_NAPI_FUNCTION("play", Play),
        DECLARE_NAPI_FUNCTION("pause", Pause),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("reset", Reset),
        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_FUNCTION("seek", Seek),
        DECLARE_NAPI_FUNCTION("on", On),
        DECLARE_NAPI_FUNCTION("setVolume", SetVolume),
        DECLARE_NAPI_FUNCTION("getTrackDescription", GetTrackDescription),
        DECLARE_NAPI_FUNCTION("setSpeed", SetSpeed),

        DECLARE_NAPI_GETTER_SETTER("dataSrc", GetDataSrc, SetDataSrc),
        DECLARE_NAPI_GETTER_SETTER("url", GetUrl, SetUrl),
        DECLARE_NAPI_GETTER_SETTER("loop", GetLoop, SetLoop),

        DECLARE_NAPI_GETTER("currentTime", GetCurrentTime),
        DECLARE_NAPI_GETTER("duration", GetDuration),
        DECLARE_NAPI_GETTER("state", GetState),
        DECLARE_NAPI_GETTER("width", GetWidth),
        DECLARE_NAPI_GETTER("height", GetHeight),
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define AudioPlayer class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value VideoPlayerNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value jsThis = nullptr;
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return result;
    }

    VideoPlayerNapi *playerNapi = new(std::nothrow) VideoPlayerNapi();
    CHECK_AND_RETURN_RET_LOG(playerNapi != nullptr, nullptr, "failed to new VideoPlayerNapi");

    playerNapi->env_ = env;
    playerNapi->nativePlayer_ = PlayerFactory::CreatePlayer();
    CHECK_AND_RETURN_RET_LOG(playerNapi->nativePlayer_ != nullptr, nullptr, "failed to CreatePlayer");

    if (playerNapi->callbackNapi_ == nullptr) {
        playerNapi->callbackNapi_ = std::make_shared<VideoCallbackNapi>(env);
        (void)playerNapi->nativePlayer_->SetPlayerCallback(playerNapi->callbackNapi_);
    }

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(playerNapi),
        VideoPlayerNapi::Destructor, nullptr, &(playerNapi->wrapper_));
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
        delete playerNapi;
        MEDIA_LOGE("Failed to wrap native instance");
        return result;
    }

    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void VideoPlayerNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        delete reinterpret_cast<VideoPlayerNapi *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}

void VideoPlayerNapi::CompleteAsyncFunc(napi_env env, napi_status status, void *data)
{
    MEDIA_LOGD("CompleteAsyncFunc In");
    auto asyncContext = reinterpret_cast<VideoPlayerAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "VideoPlayerAsyncContext is nullptr!");
    if (status == napi_ok) {
        if (!asyncContext->success) {
            (void)CommonNapi::CreateError(env,
                asyncContext->errCode, asyncContext->errMessage, asyncContext->asyncRet);
        }
    } else {
        (void)CommonNapi::CreateError(env, -1, "status != napi_ok", asyncContext->asyncRet);
    }

    auto cb = std::static_pointer_cast<VideoCallbackNapi>(asyncContext->playerNapi->callbackNapi_);
    if (asyncContext->needAsyncCallback && cb != nullptr) {
        asyncContext->env = env;
        cb->QueueAsyncWork(asyncContext);
        napi_delete_async_work(env, asyncContext->work);
    } else {
        VideoPlayerAsyncContext::AsyncCallback(env, asyncContext);
    }
}

void VideoPlayerNapi::AsyncCreateVideoPlayer(napi_env env, void *data)
{
    MEDIA_LOGD("AsyncCreateVideoPlayer In");
    auto asyncContext = reinterpret_cast<VideoPlayerAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "VideoPlayerAsyncContext is nullptr!");  
    napi_value constructor = nullptr;
    napi_status ret = napi_get_reference_value(env, constructor_, &constructor);
    if (ret != napi_ok || constructor == nullptr) {
        asyncContext->SignError(MSERR_EXT_UNKNOWN, "Failed to napi get reference value");
        return;
    }
    ret = napi_new_instance(env, constructor, 0, nullptr, &asyncContext->asyncRet);
    if (ret != napi_ok || asyncContext->asyncRet == nullptr) {
        asyncContext->SignError(MSERR_EXT_UNKNOWN, "Failed to napi new instance VideoPlayer");
        return;
    }
    MEDIA_LOGD("AsyncCreateVideoPlayer Out");
}

napi_value VideoPlayerNapi::CreateVideoPlayer(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    MEDIA_LOGD("CreateVideoPlayer In");

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "failed to napi_get_cb_info");  

    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>();
    napi_get_undefined(env, &asyncContext->asyncRet); // undefined result

    // get callback
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[0], refCount, &asyncContext->callbackRef);
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "CreateVideoPlayer", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, VideoPlayerNapi::AsyncCreateVideoPlayer,
        VideoPlayerNapi::CompleteAsyncFunc, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();

    return undefinedResult;
}

napi_value VideoPlayerNapi::SetUrl(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    MEDIA_LOGD("SetUrl In");
    // get args and jsThis
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    // get VideoPlayerNapi
    VideoPlayerNapi *playerNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&playerNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && playerNapi != nullptr, undefinedResult, "Failed to retrieve instance");

    // get url from js
    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_string) {
        playerNapi->url_.clear();
        playerNapi->OnErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }
    playerNapi->url_ = CommonNapi::GetStringArgument(env, args[0]);
    
    // set url to server
    int32_t ret = playerNapi->nativePlayer_->SetSource(playerNapi->url_);
    if (ret != MSERR_OK) {
        playerNapi->OnErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    MEDIA_LOGD("SetUrl success");
    return undefinedResult;
}

napi_value VideoPlayerNapi::GetUrl(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    // get jsThis
    napi_value jsThis = nullptr;
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("failed to napi_get_cb_info");
        return undefinedResult;
    }

    // get VideoPlayerNapi
    VideoPlayerNapi *playerNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&playerNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && playerNapi != nullptr, undefinedResult, "Failed to retrieve instance");

    if (playerNapi->url_.empty()) {
        playerNapi->OnErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    napi_value jsResult = nullptr;
    status = napi_create_string_utf8(env, playerNapi->url_.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_create_string_utf8 error");

    MEDIA_LOGD("GetSrc success");
    return jsResult;
}

napi_value VideoPlayerNapi::SetDataSrc(napi_env env, napi_callback_info info)
{
    MEDIA_LOGD("SetMediaDataSrc start");
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr && args[0] != nullptr,
        undefinedResult, "Failed to retrieve details about the callback");

    VideoPlayerNapi *playerNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&playerNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && playerNapi != nullptr, undefinedResult, "get player napi error");

    if (playerNapi->dataSrcCallBack_ != nullptr) {
        playerNapi->OnErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }
    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_object) {
        playerNapi->OnErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    playerNapi->dataSrcCallBack_ = MediaDataSourceCallback::Create(env, args[0]);
    if (playerNapi->dataSrcCallBack_ == nullptr) {
        playerNapi->OnErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }
    int32_t ret = playerNapi->nativePlayer_->SetSource(playerNapi->dataSrcCallBack_);
    if (ret != MSERR_OK) {
        playerNapi->OnErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }
    MEDIA_LOGD("SetMediaDataSrc success");
    return undefinedResult;
}

napi_value VideoPlayerNapi::GetDataSrc(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    napi_value jsThis = nullptr;
    napi_value jsResult = nullptr;
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr,
        undefinedResult, "Failed to retrieve details about the callback");

    VideoPlayerNapi *playerNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&playerNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && playerNapi != nullptr, undefinedResult, "Failed to retrieve instance");

    if (playerNapi->dataSrcCallBack_ == nullptr) {
        playerNapi->OnErrorCallback(MSERR_EXT_NO_MEMORY);
        return undefinedResult;
    }

    jsResult = playerNapi->dataSrcCallBack_->GetDataSrc();
    CHECK_AND_RETURN_RET_LOG(jsResult != nullptr, undefinedResult, "Failed to get the representation of src object");

    MEDIA_LOGD("GetDataSrc success");
    return jsResult;
}

void VideoPlayerNapi::ReleaseDataSource(std::shared_ptr<MediaDataSourceCallback> dataSourceCb)
{
    if (dataSourceCb != nullptr) {
        dataSourceCb->Release();
        dataSourceCb = nullptr;
    }
}

void VideoPlayerNapi::AsyncWork(napi_env env, void *data)
{
    auto asyncContext = reinterpret_cast<VideoPlayerAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "VideoPlayerAsyncContext is nullptr!");  

    if (asyncContext->playerNapi == nullptr || asyncContext->playerNapi->nativePlayer_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "playerNapi or nativePlayer is nullptr");
        return;
    }

    int32_t ret = MSERR_OK;
    auto player = asyncContext->playerNapi->nativePlayer_;
    if (asyncContext->asyncWorkType == AsyncWorkType::ASYNC_WORK_PREPARE) {
        ret = player->PrepareAsync();
    } else if (asyncContext->asyncWorkType == AsyncWorkType::ASYNC_WORK_PLAY) {
        ret = player->Play();
    } else if (asyncContext->asyncWorkType == AsyncWorkType::ASYNC_WORK_PAUSE) {
        ret = player->Pause();
    } else if (asyncContext->asyncWorkType == AsyncWorkType::ASYNC_WORK_STOP) {
        ret = player->Stop();
    } else if (asyncContext->asyncWorkType == AsyncWorkType::ASYNC_WORK_RESET) {
        asyncContext->playerNapi->ReleaseDataSource(asyncContext->playerNapi->dataSrcCallBack_);
        ret = player->Reset();
    } else if (asyncContext->asyncWorkType == AsyncWorkType::ASYNC_WORK_RELEASE) {
        asyncContext->playerNapi->ReleaseDataSource(asyncContext->playerNapi->dataSrcCallBack_);
        ret = player->Release();
        asyncContext->playerNapi->callbackNapi_ = nullptr;
        asyncContext->playerNapi->url_.clear();
    } else if (asyncContext->asyncWorkType == AsyncWorkType::ASYNC_WORK_VOLUME) {
        float volume = static_cast<float>(asyncContext->volume);
        ret = player->SetVolume(volume, volume);
    } else if (asyncContext->asyncWorkType == AsyncWorkType::ASYNC_WORK_SEEK) {
        PlayerSeekMode seekMode = static_cast<PlayerSeekMode>(asyncContext->seekMode);
        ret = player->Seek(asyncContext->seekPosition, seekMode);
    } else if (asyncContext->asyncWorkType == AsyncWorkType::ASYNC_WORK_SPEED) {
        PlaybackRateMode speedMode = static_cast<PlaybackRateMode>(asyncContext->speedMode);
        ret = player->SetPlaybackSpeed(speedMode);
    } else {
        asyncContext->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "asyncWorkType is error");
        return;
    }

    if (ret != MSERR_OK) {
        asyncContext->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "failed to operate playback");
    }
}

napi_value VideoPlayerNapi::Prepare(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    MEDIA_LOGD("Prepare In");
    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("failed to napi_get_cb_info");
        return undefinedResult;
    }

    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>();
    napi_get_undefined(env, &asyncContext->asyncRet); // undefined result
    asyncContext->needAsyncCallback = true;
    asyncContext->asyncWorkType = AsyncWorkType::ASYNC_WORK_PREPARE; 
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Prepare", NAPI_AUTO_LENGTH, &resource);

    // get playerNapi
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->playerNapi));
    // get callback
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        (void)napi_create_reference(env, args[0], refCount, &asyncContext->callbackRef);
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        (void)napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }
    // async work
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, VideoPlayerNapi::AsyncWork,
        VideoPlayerNapi::CompleteAsyncFunc, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return undefinedResult;
}

napi_value VideoPlayerNapi::Play(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    MEDIA_LOGD("Play In");
    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("failed to napi_get_cb_info");
        return undefinedResult;
    }

    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>();
    napi_get_undefined(env, &asyncContext->asyncRet); // undefined result
    asyncContext->needAsyncCallback = true;
    asyncContext->asyncWorkType = AsyncWorkType::ASYNC_WORK_PLAY; 
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Play", NAPI_AUTO_LENGTH, &resource);

    // get playerNapi
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->playerNapi));
    // get callback
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[0], refCount, &asyncContext->callbackRef);
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }
    // async work
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, VideoPlayerNapi::AsyncWork,
        VideoPlayerNapi::CompleteAsyncFunc, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return undefinedResult;
}

napi_value VideoPlayerNapi::Pause(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("failed to napi_get_cb_info");
        return undefinedResult;
    }

    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>();
    napi_get_undefined(env, &asyncContext->asyncRet); // undefined result
    asyncContext->needAsyncCallback = true;
    asyncContext->asyncWorkType = AsyncWorkType::ASYNC_WORK_PAUSE; 
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Pause", NAPI_AUTO_LENGTH, &resource);

    // get playerNapi
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->playerNapi));
    // get callback
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[0], refCount, &asyncContext->callbackRef);
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }
    // async work
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, VideoPlayerNapi::AsyncWork,
        VideoPlayerNapi::CompleteAsyncFunc, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return undefinedResult;
}

napi_value VideoPlayerNapi::Stop(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("failed to napi_get_cb_info");
        return undefinedResult;
    }

    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>();
    napi_get_undefined(env, &asyncContext->asyncRet); // undefined result
    asyncContext->needAsyncCallback = true;
    asyncContext->asyncWorkType = AsyncWorkType::ASYNC_WORK_STOP; 
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Stop", NAPI_AUTO_LENGTH, &resource);

    // get playerNapi
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->playerNapi));
    // get callback
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[0], refCount, &asyncContext->callbackRef);
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }
    // async work
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, VideoPlayerNapi::AsyncWork,
        VideoPlayerNapi::CompleteAsyncFunc, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return undefinedResult;
}

napi_value VideoPlayerNapi::Reset(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("failed to napi_get_cb_info");
        return undefinedResult;
    }

    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>();
    napi_get_undefined(env, &asyncContext->asyncRet); // undefined result
    asyncContext->needAsyncCallback = true;
    asyncContext->asyncWorkType = AsyncWorkType::ASYNC_WORK_RESET; 
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Reset", NAPI_AUTO_LENGTH, &resource);

    // get playerNapi
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->playerNapi));
    // get callback
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[0], refCount, &asyncContext->callbackRef);
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }
    // async work
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, VideoPlayerNapi::AsyncWork,
        VideoPlayerNapi::CompleteAsyncFunc, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return undefinedResult;
}

napi_value VideoPlayerNapi::Release(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("failed to napi_get_cb_info");
        return undefinedResult;
    }

    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>();
    napi_get_undefined(env, &asyncContext->asyncRet); // undefined result
    asyncContext->needAsyncCallback = true;
    asyncContext->asyncWorkType = AsyncWorkType::ASYNC_WORK_RELEASE; 
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Release", NAPI_AUTO_LENGTH, &resource);

    // get playerNapi
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->playerNapi));
    // get callback
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[0], refCount, &asyncContext->callbackRef);
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }
    // async work
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, VideoPlayerNapi::AsyncWork,
        VideoPlayerNapi::CompleteAsyncFunc, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return undefinedResult;
}

napi_value VideoPlayerNapi::Seek(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    // get args
    napi_value jsThis = nullptr;
    napi_value args[3] = { nullptr };
    size_t argCount = 3;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("failed to napi_get_cb_info");
        return undefinedResult;
    }

    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>();
    napi_get_undefined(env, &asyncContext->asyncRet); // undefined result
    asyncContext->needAsyncCallback = true;
    asyncContext->asyncWorkType = AsyncWorkType::ASYNC_WORK_SEEK; 
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Seek", NAPI_AUTO_LENGTH, &resource);

    // get playerNapi
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->playerNapi));
    // get seek time 
    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "failed to get seek time");
    }
    status = napi_get_value_int32(env, args[0], &asyncContext->seekPosition);
    if (status != napi_ok || asyncContext->seekPosition < 0) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "seek position < 0");
    }
    if (args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok) {
        if (valueType == napi_number) { // get seek mode
            status = napi_get_value_int32(env, args[1], &asyncContext->seekMode);
            if (status != napi_ok || asyncContext->seekMode < SEEK_PREVIOUS_SYNC || asyncContext->seekMode > SEEK_CLOSEST) {
                asyncContext->SignError(MSERR_EXT_INVALID_VAL, "seek mode invalid");
            }
        } else if (valueType == napi_function) {
            const size_t refCount = 1;
            napi_create_reference(env, args[1], refCount, &asyncContext->callbackRef);
        }
    }
    if (args[2] != nullptr && napi_typeof(env, args[2], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[1], refCount, &asyncContext->callbackRef);      
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, VideoPlayerNapi::AsyncWork,
        VideoPlayerNapi::CompleteAsyncFunc, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return undefinedResult;
}

napi_value VideoPlayerNapi::SetSpeed(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    // get args
    napi_value jsThis = nullptr;
    napi_value args[2] = { nullptr };
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("failed to napi_get_cb_info");
        return undefinedResult;
    }

    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>();
    napi_get_undefined(env, &asyncContext->asyncRet); // undefined result
    asyncContext->needAsyncCallback = true;
    asyncContext->asyncWorkType = AsyncWorkType::ASYNC_WORK_SPEED; 
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "SetSpeed", NAPI_AUTO_LENGTH, &resource);

    // get playerNapi
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->playerNapi));
    // get speed mode 
    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "failed get speed mode");
    }
    status = napi_get_value_int32(env, args[0], &asyncContext->speedMode);
    if (status != napi_ok || asyncContext->speedMode < SPEED_FORWARD_0_75_X || asyncContext->speedMode > SPEED_FORWARD_2_00_X) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "speed mode invalid");
    }
    // get callback
    if (args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[1], refCount, &asyncContext->callbackRef);      
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, VideoPlayerNapi::AsyncWork,
        VideoPlayerNapi::CompleteAsyncFunc, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return undefinedResult;
}

void VideoPlayerNapi::AsyncGetTrackDescription(napi_env env, void *data)
{
    auto asyncContext = reinterpret_cast<VideoPlayerAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "VideoPlayerAsyncContext is nullptr!");  

    if (asyncContext->playerNapi == nullptr || asyncContext->playerNapi->nativePlayer_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "playerNapi or nativePlayer is nullptr");
        return;
    }

    auto player = asyncContext->playerNapi->nativePlayer_;
    std::vector<VideoTrackInfo> &videoInfo = asyncContext->playerNapi->videoTranckInfoVec_;
    videoInfo.clear();
    // int32_t ret = player->GetVideoTrackInfo(videoInfo);
    // if (ret != MSERR_OK) {
    //     asyncContext->SignError(MSERR_EXT_UNKNOWN, "failed to operate playback");
    //     return;
    // }

    std::vector<AudioTrackInfo> audioInfo;
    // ret = player->GetAudioTrackInfo(audioInfo);
    // if (ret != MSERR_OK) {
    //     asyncContext->SignError(MSERR_EXT_UNKNOWN, "failed to operate playback");
    //     return;
    // }
    // 获取视音频组合
    videoInfo.insert(videoInfo.end(), audioInfo.begin(), audioInfo.end());
    if (videoInfo.empty()) {
        asyncContext->SignError(MSERR_EXT_UNKNOWN, "video tranck info is empty");
        return;
    }

    // create Description
    napi_value videoArray = nullptr;
    napi_status status = napi_create_array(env, &videoArray);
    if (status != napi_ok) {
        asyncContext->SignError(MSERR_EXT_UNKNOWN, "failed to napi_create_array");
        return;
    }

    auto vecSize = videoInfo.size();
    for (size_t index = 0; index < vecSize; ++index) {        
        napi_value description = nullptr;
        description = MediaDescriptionNapi::CreateMediaDescription(env, videoInfo[index]);
        if (description == nullptr || napi_set_element(env, videoArray, index, description) != napi_ok) {
            asyncContext->SignError(MSERR_EXT_UNKNOWN, "failed to CreateMediaDescription");
            return;
        }
    }
    asyncContext->asyncRet = videoArray;
}

napi_value VideoPlayerNapi::GetTrackDescription(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("failed to napi_get_cb_info");
        return undefinedResult;
    }

    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>();
    napi_get_undefined(env, &asyncContext->asyncRet); // undefined result
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "GetTrackDescription", NAPI_AUTO_LENGTH, &resource);

    // get playerNapi
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->playerNapi));
    // get callback
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[0], refCount, &asyncContext->callbackRef);
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }
    // async work
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, VideoPlayerNapi::AsyncGetTrackDescription,
        VideoPlayerNapi::CompleteAsyncFunc, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    return undefinedResult;
}

napi_value VideoPlayerNapi::SetVolume(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    // get args
    napi_value jsThis = nullptr;
    napi_value args[2] = { nullptr };
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr) {
        MEDIA_LOGE("failed to napi_get_cb_info");
        return undefinedResult;
    }

    std::unique_ptr<VideoPlayerAsyncContext> asyncContext = std::make_unique<VideoPlayerAsyncContext>();
    napi_get_undefined(env, &asyncContext->asyncRet); // undefined result
    asyncContext->needAsyncCallback = true;
    asyncContext->asyncWorkType = AsyncWorkType::ASYNC_WORK_VOLUME; 
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "SetVolume", NAPI_AUTO_LENGTH, &resource);

    // get playerNapi
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->playerNapi));
    // get volume
    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_number) {
        asyncContext->SignError(MSERR_EXT_INVALID_VAL, "get volume napi_typeof is't napi_number");
    } else {
        status = napi_get_value_double(env, args[0], &asyncContext->volume);
        if (status != napi_ok || asyncContext->volume < 0.0f || asyncContext->volume > 1.0f) {
            asyncContext->SignError(MSERR_EXT_INVALID_VAL, "get volume input volume < 0.0f or > 1.0f");
        }
    }
    // get callback
    if (args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok && valueType == napi_function) {
        const size_t refCount = 1;
        napi_create_reference(env, args[1], refCount, &asyncContext->callbackRef);
    }
    // get promise
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &undefinedResult);
    }
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, VideoPlayerNapi::AsyncWork,
        VideoPlayerNapi::CompleteAsyncFunc, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();

    return undefinedResult;
}

napi_value VideoPlayerNapi::On(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    static const size_t MIN_REQUIRED_ARG_COUNT = 2;
    size_t argCount = MIN_REQUIRED_ARG_COUNT;
    napi_value args[MIN_REQUIRED_ARG_COUNT] = { nullptr, nullptr };
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr || args[1] == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    VideoPlayerNapi *playerNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&playerNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && playerNapi != nullptr, undefinedResult, "Failed to retrieve instance");

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        playerNapi->OnErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGD("callbackName: %{public}s", callbackName.c_str());

    CHECK_AND_RETURN_RET_LOG(playerNapi->callbackNapi_ != nullptr, undefinedResult, "callbackNapi_ is nullptr");
    auto cb = std::static_pointer_cast<VideoCallbackNapi>(playerNapi->callbackNapi_);
    cb->SaveCallbackReference(callbackName, args[1]);
    return undefinedResult;
}

napi_value VideoPlayerNapi::SetLoop(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 1;
    napi_value args[1] = { nullptr };
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    VideoPlayerNapi *playerNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&playerNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && playerNapi != nullptr, undefinedResult, "Failed to retrieve instance");

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_boolean) {
        playerNapi->OnErrorCallback(MSERR_EXT_INVALID_VAL);
        return undefinedResult;
    }

    bool loopFlag = false;
    status = napi_get_value_bool(env, args[0], &loopFlag);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_get_value_bool error");

    CHECK_AND_RETURN_RET_LOG(playerNapi->nativePlayer_ != nullptr, undefinedResult, "No memory");
    int32_t ret = playerNapi->nativePlayer_->SetLooping(loopFlag);
    if (ret != MSERR_OK) {
        playerNapi->OnErrorCallback(MSERR_EXT_UNKNOWN);
        return undefinedResult;
    }
    MEDIA_LOGD("SetLoop success");
    return undefinedResult;
}


napi_value VideoPlayerNapi::GetLoop(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    napi_value jsThis = nullptr;
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    VideoPlayerNapi *playerNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&playerNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && playerNapi != nullptr, undefinedResult, "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(playerNapi->nativePlayer_ != nullptr, undefinedResult, "No memory");
    bool loopFlag = playerNapi->nativePlayer_->IsLooping();

    napi_value jsResult = nullptr;
    status = napi_get_boolean(env, loopFlag, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_get_boolean error");
    MEDIA_LOGD("GetSrc success loop Status: %{public}d", loopFlag);
    return jsResult;
}

napi_value VideoPlayerNapi::GetCurrentTime(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    VideoPlayerNapi *playerNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&playerNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && playerNapi != nullptr, undefinedResult, "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(playerNapi->nativePlayer_ != nullptr, undefinedResult, "No memory");
    int32_t currentTime = -1;
    int32_t ret = playerNapi->nativePlayer_->GetCurrentTime(currentTime);
    if (ret != MSERR_OK || currentTime < 0) {
        playerNapi->OnErrorCallback(MSERR_EXT_UNKNOWN);
        return undefinedResult;
    }

    napi_value jsResult = nullptr;
    status = napi_create_int32(env, currentTime, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_create_int32 error");
    MEDIA_LOGD("GetCurrenTime success, Current time: %{public}d", currentTime);
    return jsResult;
}

napi_value VideoPlayerNapi::GetDuration(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    napi_value jsThis = nullptr;
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return undefinedResult;
    }

    VideoPlayerNapi *playerNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&playerNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && playerNapi != nullptr, undefinedResult, "Failed to retrieve instance");

    CHECK_AND_RETURN_RET_LOG(playerNapi->nativePlayer_ != nullptr, undefinedResult, "No memory");
    int32_t duration = -1;
    int32_t ret = playerNapi->nativePlayer_->GetDuration(duration);
    if (ret != MSERR_OK) {
        playerNapi->OnErrorCallback(MSERR_EXT_UNKNOWN);
        return undefinedResult;
    }

    napi_value jsResult = nullptr;
    status = napi_create_int32(env, duration, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_create_int32 error");

    MEDIA_LOGD("GetDuration success, Current time: %{public}d", duration);
    return jsResult;
}

static std::string GetJSState(PlayerStates currentState)
{
    std::string result;
    MEDIA_LOGD("GetJSState()! is called!, %{public}d", currentState);
    switch (currentState) {
        case PLAYER_IDLE:
        case PLAYER_INITIALIZED:
            result = VideoPlayState::STATE_IDLE;
            break;
        case PLAYER_PREPARED:
            result = VideoPlayState::STATE_PREPARED;
            break;
        case PLAYER_STARTED:
            result = VideoPlayState::STATE_PLAYING;
            break;
        case PLAYER_PAUSED:
        case PLAYER_PLAYBACK_COMPLETE:
            result = VideoPlayState::STATE_PAUSED;
            break;
        case PLAYER_STOPPED:
            result = VideoPlayState::STATE_STOPPED;
            break;
        default:
            // Considering default state as stopped
            MEDIA_LOGE("Unknown state!, %{public}d", currentState);
            result = VideoPlayState::STATE_ERROR;
            break;
    }
    return result;
}

napi_value VideoPlayerNapi::GetState(napi_env env, napi_callback_info info)
{
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "Failed to retrieve details about the callback");

    VideoPlayerNapi *playerNapi = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&playerNapi);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "Failed to retrieve instance");

    std::string curState = VideoPlayState::STATE_ERROR;
    if (playerNapi->callbackNapi_ != nullptr) {
        auto cb = std::static_pointer_cast<VideoCallbackNapi>(playerNapi->callbackNapi_);
        curState = GetJSState(cb->GetCurrentState());
        MEDIA_LOGD("GetState success, State: %{public}s", curState.c_str());
    }

    napi_value jsResult = nullptr;
    status = napi_create_string_utf8(env, curState.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_create_string_utf8 error");
    return jsResult;
}

napi_value VideoPlayerNapi::GetWidth(napi_env env, napi_callback_info info)
{
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "Failed to retrieve details about the callback");

    VideoPlayerNapi *playerNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&playerNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && playerNapi != nullptr, undefinedResult, "Failed to retrieve instance");

    int32_t width = 0;
    if (playerNapi->callbackNapi_ != nullptr) {
        auto cb = std::static_pointer_cast<VideoCallbackNapi>(playerNapi->callbackNapi_);
        width = cb->GetVideoWidth();
    }

    napi_value jsResult = nullptr;
    status = napi_create_int32(env, width, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_create_int32 error");
    return jsResult;
}

napi_value VideoPlayerNapi::GetHeight(napi_env env, napi_callback_info info)
{
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "Failed to retrieve details about the callback");

    VideoPlayerNapi *playerNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&playerNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && playerNapi != nullptr, undefinedResult, "Failed to retrieve instance");

    int32_t height = 0;
    if (playerNapi->callbackNapi_ != nullptr) {
        auto cb = std::static_pointer_cast<VideoCallbackNapi>(playerNapi->callbackNapi_);
        height = cb->GetVideoHeight();
    }

    napi_value jsResult = nullptr;
    status = napi_create_int32(env, height, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, undefinedResult, "napi_create_int32 error");
    return jsResult;
}

void VideoPlayerNapi::OnErrorCallback(MediaServiceExtErrCode errCode)
{
    if (callbackNapi_ != nullptr) {
        auto cb = std::static_pointer_cast<VideoCallbackNapi>(callbackNapi_);
        cb->SendErrorCallback(errCode);
    }
}
}  // namespace Media
}  // namespace OHOS