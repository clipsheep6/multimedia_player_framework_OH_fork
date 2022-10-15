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

#include "recorder_capability_napi.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderCapabilityNapi"};
}

namespace OHOS {
namespace Media {
napi_value RecorderCapabilityNapi::GetAudioRecorderCaps(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<RecorderCapabilityAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    napi_value resource = nullptr;
    
    napi_create_string_utf8(env, "GetAudioRecorderCaps", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<RecorderCapabilityAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            auto audioRecorderCaps = RecorderProfilesFactory::CreateRecorderProfiles().GetAudioRecorderCaps();
            asyncCtx->JsResult = std::make_unique<MediaJsAudioRecorderCapsArray>(audioRecorderCaps);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value RecorderCapabilityNapi::IsAudioRecorderConfigSupported(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<RecorderCapabilityAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_object) {
        (void)ExtractAudioRecorderProfile(env, args[0], asyncCtx->profile);
    } else {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Illegal argument");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[1]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "IsAudioRecorderConfigSupported", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<RecorderCapabilityAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            bool outResult = RecorderProfilesFactory::CreateRecorderProfiles().IsAudioRecorderConfigSupported(
                asyncCtx->profile);
            asyncCtx->JsResult = std::make_unique<MediaJsResultBoolean>(outResult);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value RecorderCapabilityNapi::GetVideoRecorderCaps(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<RecorderCapabilityAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "GetVideoRecorderCaps", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<RecorderCapabilityAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            auto videoRecorderCaps = RecorderProfilesFactory::CreateRecorderProfiles().GetVideoRecorderCaps();
            asyncCtx->JsResult = std::make_unique<MediaJsVideoRecorderCapsArray>(videoRecorderCaps);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value RecorderCapabilityNapi::GetVideoRecorderProfile(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<RecorderCapabilityAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[3] = {nullptr};
    size_t argCount = 3;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    napi_valuetype valueType = napi_undefined;
    if ((args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_number) &&
        (args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok && valueType == napi_number)) {
        NAPI_CALL(env, napi_get_value_int32(env, args[0], &asyncCtx->sourceId));
        NAPI_CALL(env, napi_get_value_int32(env, args[1], &asyncCtx->qualityLevel));
    } else {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Illegal argument");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[2]);  // 2 : two params
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "GetVideoRecorderProfile", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<RecorderCapabilityAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            auto videoRecorderProfile = RecorderProfilesFactory::CreateRecorderProfiles().GetVideoRecorderProfile(
                asyncCtx->sourceId, asyncCtx->qualityLevel);
            asyncCtx->JsResult = std::make_unique<MediaJsVideoRecorderProfile>(videoRecorderProfile);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value RecorderCapabilityNapi::HasVideoRecorderProfile(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<RecorderCapabilityAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[3] = {nullptr};
    size_t argCount = 3;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    napi_valuetype valueType = napi_undefined;
    if ((args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_number) &&
        (args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok && valueType == napi_number)) {
        NAPI_CALL(env, napi_get_value_int32(env, args[0], &asyncCtx->sourceId));
        NAPI_CALL(env, napi_get_value_int32(env, args[1], &asyncCtx->qualityLevel));
    } else {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Illegal argument");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[2]); // 2 : two params
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "HasVideoRecorderProfile", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<RecorderCapabilityAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            bool outResult = RecorderProfilesFactory::CreateRecorderProfiles().HasVideoRecorderProfile(
                asyncCtx->sourceId, asyncCtx->qualityLevel);
            asyncCtx->JsResult = std::make_unique<MediaJsResultBoolean>(outResult);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

bool RecorderCapabilityNapi::ExtractAudioRecorderProfile(napi_env env, napi_value profile, AudioRecorderProfile &result)
{
    CHECK_AND_RETURN_RET(profile != nullptr, false);
    bool ret = true;
    result.containerFormatType = CommonNapi::GetPropertyString(env, profile, "outputFormat");
    result.audioCodec = CommonNapi::GetPropertyString(env, profile, "audioEncoderMime");
    ret = CommonNapi::GetPropertyInt32(env, profile, "bitrate", result.audioBitrate);
    CHECK_AND_RETURN_RET(ret != false, false);
    ret = CommonNapi::GetPropertyInt32(env, profile, "sampleRate", result.audioSampleRate);
    CHECK_AND_RETURN_RET(ret != false, false);
    ret = CommonNapi::GetPropertyInt32(env, profile, "channel", result.audioChannels);
    CHECK_AND_RETURN_RET(ret != false, false);
    return ret;
}

napi_status MediaJsAudioRecorderCapsArray::GetJsResult(napi_env env, napi_value &result)
{
    CHECK_AND_RETURN_RET(!value_.empty(), napi_generic_failure);

    napi_status status = napi_create_array_with_length(env, value_.size(), &result);
    CHECK_AND_RETURN_RET(status == napi_ok, status);

    int32_t index = 0;
    for (auto it = value_.begin(); it != value_.end(); it++) {
        CHECK_AND_CONTINUE((*it) != nullptr);

        napi_value obj = nullptr;
        status = napi_create_object(env, &obj);
        CHECK_AND_CONTINUE(status == napi_ok);

        std::string outString = (*it)->containerFormatType;
        (void)CommonNapi::AddStringProperty(env, obj, "outputFormat", outString);

        outString = (*it)->mimeType;
        (void)CommonNapi::AddStringProperty(env, obj, "audioEncoderMime", outString);

        Range range = (*it)->bitrate;
        (void)CommonNapi::AddRangeProperty(env, obj, "bitrateRange", range.minVal, range.maxVal);

        std::vector<int32_t> vec = (*it)->sampleRate;
        (void)CommonNapi::AddArrayProperty(env, obj, "sampleRates", vec);

        range = (*it)->channels;
        (void)CommonNapi::AddRangeProperty(env, obj, "channelRange", range.minVal, range.maxVal);

        (void)napi_set_element(env, result, index, obj);
        index++;
    }

    return status;
}

napi_status MediaJsVideoRecorderCapsArray::GetJsResult(napi_env env, napi_value &result)
{
    CHECK_AND_RETURN_RET(!value_.empty(), napi_generic_failure);

    napi_status status = napi_create_array_with_length(env, value_.size(), &result);
    CHECK_AND_RETURN_RET(status == napi_ok, status);

    int32_t index = 0;
    for (auto it = value_.begin(); it != value_.end(); it++) {
        CHECK_AND_CONTINUE((*it) != nullptr);

        napi_value obj = nullptr;
        status = napi_create_object(env, &obj);
        CHECK_AND_CONTINUE(status == napi_ok);

        std::string outString = (*it)->containerFormatType;
        (void)CommonNapi::AddStringProperty(env, obj, "outputFormat", outString);

        outString = (*it)->audioEncoderMime;
        (void)CommonNapi::AddStringProperty(env, obj, "audioEncoderMime", outString);

        Range range = (*it)->audioBitrateRange;
        (void)CommonNapi::AddRangeProperty(env, obj, "audioBitrateRange", range.minVal, range.maxVal);

        std::vector<int32_t> vec = (*it)->audioSampleRates;
        (void)CommonNapi::AddArrayProperty(env, obj, "audioSampleRates", vec);

        range = (*it)->audioChannelRange;
        (void)CommonNapi::AddRangeProperty(env, obj, "audioChannelRange", range.minVal, range.maxVal);

        range = (*it)->videoBitrateRange;
        (void)CommonNapi::AddRangeProperty(env, obj, "videoBitrateRange", range.minVal, range.maxVal);

        range = (*it)->videoFramerateRange;
        (void)CommonNapi::AddRangeProperty(env, obj, "videoFramerateRange", range.minVal, range.maxVal);

        outString = (*it)->videoEncoderMime;
        (void)CommonNapi::AddStringProperty(env, obj, "videoEncoderMime", outString);

        range = (*it)->videoWidthRange;
        (void)CommonNapi::AddRangeProperty(env, obj, "videoWidthRange", range.minVal, range.maxVal);

        range = (*it)->videoHeightRange;
        (void)CommonNapi::AddRangeProperty(env, obj, "videoHeightRange", range.minVal, range.maxVal);

        (void)napi_set_element(env, result, index, obj);
        index++;
    }

    return status;
}

napi_status MediaJsVideoRecorderProfile::GetJsResult(napi_env env, napi_value &result)
{
    napi_status ret = napi_ok;
    bool setRet = true;
    CHECK_AND_RETURN_RET(value_ != nullptr, napi_generic_failure);
    CHECK_AND_RETURN_RET((ret = napi_create_object(env, &result)) == napi_ok, ret);

    setRet = CommonNapi::SetPropertyInt32(env, result, "audioBitrate", value_->audioBitrate);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyInt32(env, result, "audioChannels", value_->audioChannels);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyString(env, result, "audioCodec", value_->audioCodec);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyInt32(env, result, "audioSampleRate", value_->audioSampleRate);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyInt32(env, result, "durationTime", value_->durationTime);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyString(env, result, "fileFormat", value_->containerFormatType);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyInt32(env, result, "qualityLevel", value_->qualityLevel);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyInt32(env, result, "videoBitrate", value_->videoBitrate);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyString(env, result, "videoCodec", value_->videoCodec);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyInt32(env, result, "videoFrameWidth", value_->videoFrameWidth);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyInt32(env, result, "videoFrameHeight", value_->videoFrameHeight);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);
    setRet = CommonNapi::SetPropertyInt32(env, result, "videoFrameRate", value_->videoFrameRate);
    CHECK_AND_RETURN_RET(setRet == true, napi_generic_failure);

    return ret;
}
}  // namespace Media
}  // namespace OHOS
