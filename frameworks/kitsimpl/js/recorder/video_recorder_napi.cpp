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

#include "video_recorder_napi.h"
#include "recorder_callback_napi.h"
#include "media_log.h"
#include "media_errors.h"
#include "common_napi.h"
#include "directory_ex.h"
#include "string_ex.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoRecorderNapi"};
}

namespace OHOS {
namespace Media {
napi_ref VideoRecorderNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "VideoRecorder";

VideoRecorderNapi::VideoRecorderNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR "Instances create", FAKE_POINTER(this));
}

VideoRecorderNapi::~VideoRecorderNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
    callbackNapi_ = nullptr;
    recorder_ = nullptr;
    MEDIA_LOGD("0x%{public}06" PRIXPTR "Instances destroy", FAKE_POINTER(this));
}

napi_value VideoRecorderNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("prepare", Prepare),
        DECLARE_NAPI_FUNCTION("getInputSurface", GetInputSurface),
        DECLARE_NAPI_FUNCTION("start", Start),
        DECLARE_NAPI_FUNCTION("pause", Pause),
        DECLARE_NAPI_FUNCTION("resume", Resume),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("reset", Reset),
        DECLARE_NAPI_FUNCTION("release", Release),
        // DECLARE_NAPI_FUNCTION("setNextOutputFile", SetNextOutputFile),
        DECLARE_NAPI_FUNCTION("on", On),

        DECLARE_NAPI_GETTER("state", GetState),
    };

    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createVideoRecorder", CreateVideoRecorder),
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define VideoRecorder class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value VideoRecorderNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    napi_value jsThis = nullptr;
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok) {
        MEDIA_LOGE ("Failed to retrieve details about the callback");
        return result;
    }

    VideoRecorderNapi *recorderNapi = new(std::nothrow) VideoRecorderNapi();
    CHECK_AND_RETURN_RET_LOG(recorderNapi != nullptr, result, "No memory!");

    recorderNapi->env_ = env;
    recorderNapi->recorder_ = RecorderFactory::CreateRecorder();
    CHECK_AND_RETURN_RET_LOG(recorderNapi->recorder_ != nullptr, result, "No memory!");

    if (recorderNapi->callbackNapi_ == nullptr) {
        recorderNapi->callbackNapi_ = std::make_shared<RecorderCallbackNapi>(env); // jhp1
        (void)recorderNapi->recorder_->SetRecorderCallback(recorderNapi->callbackNapi_);
    }

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(recorderNapi),
        VideoRecorderNapi::Destructor, nullptr, &(recorderNapi->wrapper_));
    if (status != napi_ok) {
        delete recorderNapi;
        MEDIA_LOGE("Failed to warp native instance!");
        return result;
    }

    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void VideoRecorderNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        delete reinterpret_cast<VideoRecorderNapi *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}

napi_value VideoRecorderNapi::CreateVideoRecorder(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    //get args
    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "failed to napi_get_cb_info");

    std::unique_ptr<VideoRecorderAsyncContext> asyncCtx = std::make_unique<VideoRecorderAsyncContext>(env);

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);
    asyncCtx->JsResult = std::make_unique<MediaJsResultInstance>(constructor_);

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "createVideoRecorder", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value VideoRecorderNapi::Prepare(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    napi_value jsThis = nullptr;
    napi_value args[2] = { nullptr };

    auto asyncCtx = std::make_unique<VideoRecorderAsyncContext>(env);
    asyncCtx->asyncWorkType = AsyncWorkType::ASYNC_WORK_PREPARE;

    // get args
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    // get recordernapi
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    // get param
    napi_valuetype valueType = napi_undefined;
    if (args[0] == nullptr || napi_typeof(env, args[0], &valueType) != napi_ok || valueType != napi_object) {
        asyncCtx->SignError(MSERR_EXT_UNKNOWN, "set properties failed!");
    }

    std::string urlPath = "invalid url";

    VideoRecorderProperties videoProperties;
    urlPath = CommonNapi::GetPropertyString(env, args[0], "url");
    asyncCtx->napi->GetConfig(env, args[0], videoProperties);
    if (asyncCtx->napi->GetVideoRecorderProperties(env, args[0], videoProperties) != MSERR_OK) {
        asyncCtx->SignError(MSERR_EXT_UNKNOWN, "get properties failed!");
    }

    // set param
    if (asyncCtx->napi->SetVideoRecorderProperties(urlPath, videoProperties) != MSERR_OK) {
        asyncCtx->SignError(MSERR_EXT_UNKNOWN, "set properties failed!");
    }
    if (asyncCtx->napi->SetUrl(urlPath) != MSERR_OK) {
        asyncCtx->SignError(MSERR_EXT_UNKNOWN, "set URL failed!");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[1]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    asyncCtx->napi->currentStates_ = VideoRecorderState::STATE_PREPARED;
    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Prepare", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        CompleteAsyncWork, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();
    return result;
}

napi_value VideoRecorderNapi::Start(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoRecorderAsyncContext>(env);
    asyncCtx->asyncWorkType = AsyncWorkType::ASYNC_WORK_START;

    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    // get recordernapi
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    asyncCtx->napi->currentStates_ = VideoRecorderState::STATE_PLAYING;

    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Start", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        CompleteAsyncWork, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();
    return result;
}

napi_value VideoRecorderNapi::Pause(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoRecorderAsyncContext>(env);
    asyncCtx->asyncWorkType = AsyncWorkType::ASYNC_WORK_PAUSE;

    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    // get recordernapi
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    asyncCtx->napi->currentStates_ = VideoRecorderState::STATE_PAUSED;

    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Pause", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        CompleteAsyncWork, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();
    return result;
}

napi_value VideoRecorderNapi::Resume(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoRecorderAsyncContext>(env);
    asyncCtx->asyncWorkType = AsyncWorkType::ASYNC_WORK_START;

    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    // get recordernapi
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    asyncCtx->napi->currentStates_ = VideoRecorderState::STATE_PLAYING;

    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Resume", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        CompleteAsyncWork, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();
    return result;
}

napi_value VideoRecorderNapi::Stop(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoRecorderAsyncContext>(env);
    asyncCtx->asyncWorkType = AsyncWorkType::ASYNC_WORK_STOP;

    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    // get recordernapi
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    asyncCtx->napi->currentStates_ = VideoRecorderState::STATE_STOPPED;

    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Stop", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        CompleteAsyncWork, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();
    return result;
}

napi_value VideoRecorderNapi::Reset(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoRecorderAsyncContext>(env);
    asyncCtx->asyncWorkType = AsyncWorkType::ASYNC_WORK_RESET;

    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    // get recordernapi
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    asyncCtx->napi->currentStates_ = VideoRecorderState::STATE_IDLE;

    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Reset", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        CompleteAsyncWork, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();
    return result;
}

napi_value VideoRecorderNapi::Release(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<VideoRecorderAsyncContext>(env);
    asyncCtx->asyncWorkType = AsyncWorkType::ASYNC_WORK_RELEASE;

    napi_value jsThis = nullptr;
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "failed to napi_get_cb_info");
    }

    // get recordernapi
    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    asyncCtx->napi->currentStates_ = VideoRecorderState::STATE_IDLE;

    // async work
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Release", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        CompleteAsyncWork, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();
    return result;
}

napi_value VideoRecorderNapi::On(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    static const size_t MIN_REQUIRED_ARG_COUNT = 2;
    size_t argCount = MIN_REQUIRED_ARG_COUNT;
    napi_value args[MIN_REQUIRED_ARG_COUNT] = { nullptr, nullptr };
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr || args[0] == nullptr || args[1] == nullptr) {
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return result;
    }

    VideoRecorderNapi *recorderNapi = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&recorderNapi));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && recorderNapi != nullptr, result, "Failed to retrieve instance");

    napi_valuetype valueType0 = napi_undefined;
    napi_valuetype valueType1 = napi_undefined;
    if (napi_typeof(env, args[0], &valueType0) != napi_ok || valueType0 != napi_string ||
        napi_typeof(env, args[1], &valueType1) != napi_ok || valueType1 != napi_function) {
        recorderNapi->ErrorCallback(MSERR_EXT_INVALID_VAL);
        return result;
    }

    std::string callbackName = CommonNapi::GetStringArgument(env, args[0]);
    MEDIA_LOGD("callbackName: %{public}s", callbackName.c_str());

    CHECK_AND_RETURN_RET_LOG(recorderNapi->callbackNapi_ != nullptr, result, "callbackNapi_ is nullptr");
    auto cb = std::static_pointer_cast<RecorderCallbackNapi>(recorderNapi->callbackNapi_);
    cb->SaveCallbackReference(callbackName, args[1]);
    return result;
}


void VideoRecorderNapi::GetConfig(napi_env env, napi_value args, VideoRecorderProperties &properties)
{
    int32_t audioSource, videoSource;
    (void)CommonNapi::GetPropertyInt32(env, args, "audioSourceType", audioSource);
    (void)CommonNapi::GetPropertyInt32(env, args, "videoSourceType", videoSource);
    properties.audioSourceType = static_cast<AudioSourceType>(audioSource);
    properties.videoSourceType = static_cast<VideoSourceType>(videoSource);
    (void)CommonNapi::GetPropertyInt32(env, args, "orientationHint", properties.orientationHint);

    napi_value geoLocation = nullptr;
    napi_get_named_property(env, args, "location", &geoLocation);
    double tempLatitude, tempLongitude;
    (void)CommonNapi::GetPropertyDouble(env, geoLocation, "latitude", tempLatitude);
    (void)CommonNapi::GetPropertyDouble(env, geoLocation, "longitude", tempLongitude);
    properties.location.latitude = static_cast<float>(tempLatitude);
    properties.location.longitude = static_cast<float>(tempLongitude);
}

int32_t VideoRecorderNapi::GetVideoRecorderProperties(napi_env env, napi_value args,
    VideoRecorderProperties &properties)
{
    napi_value item = nullptr;
    napi_get_named_property(env, args, "profile", &item);

    (void)CommonNapi::GetPropertyInt32(env, item, "audioBitrate", properties.profile.audioBitrate);
    (void)CommonNapi::GetPropertyInt32(env, item, "audioChannels", properties.profile.audioChannels);
    std::string audioCodec = CommonNapi::GetPropertyString(env, item, "audioCodec");
    MapCodecMime(audioCodec, properties.profile.audioCodec);
    (void)CommonNapi::GetPropertyInt32(env, item, "audioSampleRate", properties.profile.auidoSampleRate);
    (void)CommonNapi::GetPropertyInt32(env, item, "durationTime", properties.profile.duration);
    std::string outputFile = CommonNapi::GetPropertyString(env, item, "fileFormat");
    MapContainerFormat(outputFile, properties.profile.fileFormat);
    (void)CommonNapi::GetPropertyInt32(env, item, "videoBitrate", properties.profile.videoBitrate);
    std::string videoCodec = CommonNapi::GetPropertyString(env, item, "videoCodec");
    MapCodecMime(videoCodec, properties.profile.videoCodec);
    (void)CommonNapi::GetPropertyInt32(env, item, "videoFrameWidth", properties.profile.videoFrameWidth);
    (void)CommonNapi::GetPropertyInt32(env, item, "videoFrameHeight", properties.profile.videoFrameHeight);
    (void)CommonNapi::GetPropertyInt32(env, item, "videoFrameRate", properties.profile.videoFrameRate);

    return MSERR_OK;
}

int32_t VideoRecorderNapi::SetVideoRecorderProperties(const std::string urlPath, const VideoRecorderProperties &properties)
{
    CHECK_AND_RETURN_RET(recorder_ != nullptr, MSERR_INVALID_OPERATION);
    int32_t audioSourceId = 0;
    int32_t videoSourceId = 0;


    int32_t ret = recorder_->SetVideoSource(properties.videoSourceType, videoSourceId);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Fail to set VideoSource");

    ret = recorder_->SetAudioSource(properties.audioSourceType, audioSourceId);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Fail to set AudioSource");

    OutputFormatType outputFile;
    MapOutputFormatType(properties.profile.fileFormat, outputFile);
    ret = recorder_->SetOutputFormat(outputFile);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Fail to set OutputFormat");

    VideoCodecFormat videoCodec;
    MapVideoCodec(properties.profile.videoCodec, videoCodec);
    ret = recorder_->SetVideoEncoder(videoSourceId, videoCodec);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Fail to set videoCodec");

    ret = recorder_->SetVideoSize(videoSourceId, properties.profile.videoFrameWidth,
        properties.profile.videoFrameHeight);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Fail to set videoSize");

    ret = recorder_->SetVideoFrameRate(videoSourceId, properties.profile.videoFrameRate);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Fail to set videoFrameRate");

    ret = recorder_->SetVideoEncodingBitRate(videoSourceId, properties.profile.videoBitrate);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Fail to set videoBitrate");

    AudioCodecFormat audioCodec;
    MapAudioCodec(properties.profile.audioCodec, audioCodec);
    ret = recorder_->SetAudioEncoder(audioSourceId, audioCodec);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Fail to set audioCodec");

    ret = recorder_->SetAudioSampleRate(audioSourceId, properties.profile.auidoSampleRate);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Fail to set auidoSampleRate");

    ret = recorder_->SetAudioChannels(audioSourceId, properties.profile.audioChannels);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Fail to set audioChannels");

    ret = recorder_->SetAudioEncodingBitRate(audioSourceId, properties.profile.audioBitrate);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Fail to set audioBitrate");

    recorder_->SetLocation(properties.location.latitude, properties.location.longitude);
    recorder_->SetOrientationHint(properties.orientationHint);

    return MSERR_OK;
}

int32_t VideoRecorderNapi::SetUrl(const std::string &urlPath)
{
    CHECK_AND_RETURN_RET_LOG(recorder_ != nullptr, MSERR_INVALID_OPERATION, "No memory");
    const std::string fileHead = "file://";
    const std::string fdHead = "fd://";
    int32_t fd = -1;

    if (urlPath.find(fileHead) != std::string::npos) {
        std::string filePath = urlPath.substr(fileHead.size());
        int32_t ret = recorder_->SetOutputPath(filePath);
        CHECK_AND_RETURN_RET(ret == MSERR_OK, MSERR_INVALID_OPERATION);
    } else if (urlPath.find(fdHead) != std::string::npos) {
        std::string inputFd = urlPath.substr(fdHead.size());
        CHECK_AND_RETURN_RET(StrToInt(inputFd, fd) == true, MSERR_INVALID_VAL);
        CHECK_AND_RETURN_RET(fd >= 0, MSERR_INVALID_OPERATION);
        CHECK_AND_RETURN_RET(recorder_->SetOutputFile(fd) == MSERR_OK, MSERR_INVALID_OPERATION);
    } else {
        MEDIA_LOGE("invalid input uri, neither file nor fd!");
        return MSERR_INVALID_OPERATION;
    }

    return MSERR_OK;
}

void VideoRecorderNapi::CompleteAsyncWork(napi_env env, napi_status status, void *data)
{
    MEDIA_LOGD("CompleteAsyncFunc In");
    auto asyncCtx = reinterpret_cast<VideoRecorderAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "VideoRecorderAsyncContext is nullptr!");

    if (status != napi_ok) {
        return MediaAsyncContext::CompleteCallback(env, status, data);
    }

    if (asyncCtx->napi == nullptr || asyncCtx->napi->recorder_ == nullptr ||
        asyncCtx->napi->callbackNapi_ == nullptr) {
        asyncCtx->SignError(MSERR_EXT_NO_MEMORY, "recorderNapi or nativeRecorder is nullptr");
        return MediaAsyncContext::CompleteCallback(env, status, data);;
    }

    int32_t ret = MSERR_OK;
    auto recorder = asyncCtx->napi->recorder_;
    if (asyncCtx->asyncWorkType == AsyncWorkType::ASYNC_WORK_PREPARE) {
        ret = recorder->Prepare();
    } else if (asyncCtx->asyncWorkType == AsyncWorkType::ASYNC_WORK_GET_SURFACE) {
        // asyncCtx->surface = recorder->GetSurface(); //
    } else if (asyncCtx->asyncWorkType == AsyncWorkType::ASYNC_WORK_START) {
        ret = recorder->Start();
    } else if (asyncCtx->asyncWorkType == AsyncWorkType::ASYNC_WORK_PAUSE) {
        ret = recorder->Pause();
    } else if (asyncCtx->asyncWorkType == AsyncWorkType::ASYNC_WORK_PAUSE) {
        ret = recorder->Pause();
    } else if (asyncCtx->asyncWorkType == AsyncWorkType::ASYNC_WORK_RESUME) {
        ret = recorder->Resume();
    } else if (asyncCtx->asyncWorkType == AsyncWorkType::ASYNC_WORK_STOP) {
        ret = recorder->Stop(false);
    } else if (asyncCtx->asyncWorkType == AsyncWorkType::ASYNC_WORK_RESET) {
        ret = recorder->Reset();
    } else if (asyncCtx->asyncWorkType == AsyncWorkType::ASYNC_WORK_RELEASE) {
        ret = recorder->Release();
        asyncCtx->napi->callbackNapi_ = nullptr;
    }  else {
        asyncCtx->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "asyncWorkType is error");
        return MediaAsyncContext::CompleteCallback(env, status, data);
    }

    if (ret != MSERR_OK) {
        asyncCtx->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "asyncWorkType is error");
        return MediaAsyncContext::CompleteCallback(env, status, data);
    }
}

napi_value VideoRecorderNapi::GetState(napi_env env, napi_callback_info info)
{
    napi_value jsThis = nullptr;
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "Failed to retrieve details about the callback");

    VideoRecorderNapi *recorderNapi = nullptr;
    status = napi_unwrap(env, jsThis, (void **)&recorderNapi);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "Failed to retrieve instance");

    std::string curState = VideoRecorderState::STATE_ERROR;
    if (recorderNapi->callbackNapi_ != nullptr) {
        auto cb = std::static_pointer_cast<RecorderCallbackNapi>(recorderNapi->callbackNapi_);
        curState = recorderNapi->currentStates_;
        MEDIA_LOGD("GetState success, State: %{public}s", curState.c_str());
    }

    napi_value jsResult = nullptr;
    status = napi_create_string_utf8(env, curState.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "napi_create_string_utf8 error");
    return jsResult;
}

// 同步的 不带 promise callback 的属性接口 可以通过这个上报，比如getstates
void VideoRecorderNapi::ErrorCallback(MediaServiceExtErrCode errCode)
{
    if (callbackNapi_ != nullptr) {
        auto napiCb = std::static_pointer_cast<RecorderCallbackNapi>(callbackNapi_);
        napiCb->SendErrorCallback(errCode);
    }
}
} // namespace Media
} // namespace OHOS
