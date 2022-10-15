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

#include "avcodec_capability_napi.h"
#include "avcodec_napi_utils.h"
#include "avcodec_list.h"
#include "media_capability_vcaps_napi.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecCapabilityNapi"};
}

namespace OHOS {
namespace Media {
napi_value AVCodecCapabilityNapi::GetAudioDecoderCaps(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVCodecCapabilityAsyncContext>(env);

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
    napi_create_string_utf8(env, "GetAudioDecoderCaps", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AVCodecCapabilityAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            asyncCtx->JsResult = std::make_unique<MediaJsAudioCapsStatic>(true);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AVCodecCapabilityNapi::FindAudioDecoder(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVCodecCapabilityAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_object) {
        (void)AVCodecNapiUtil::ExtractMediaFormat(env, args[0], asyncCtx->format);
    } else {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Illegal argument");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[1]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "FindAudioDecoder", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AVCodecCapabilityAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            auto codecList = AVCodecListFactory::CreateAVCodecList();
            if (codecList == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "No memory");
                return;
            }
            std::string decoder = codecList->FindAudioDecoder(asyncCtx->format);
            asyncCtx->JsResult = std::make_unique<MediaJsResultString>(decoder);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AVCodecCapabilityNapi::GetAudioEncoderCaps(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVCodecCapabilityAsyncContext>(env);

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
    napi_create_string_utf8(env, "GetAudioEncoderCaps", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AVCodecCapabilityAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            asyncCtx->JsResult = std::make_unique<MediaJsAudioCapsStatic>(false);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AVCodecCapabilityNapi::FindAudioEncoder(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVCodecCapabilityAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_object) {
        (void)AVCodecNapiUtil::ExtractMediaFormat(env, args[0], asyncCtx->format);
    } else {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Illegal argument");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[1]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "FindAudioEncoder", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AVCodecCapabilityAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            auto codecList = AVCodecListFactory::CreateAVCodecList();
            if (codecList == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "No memory");
                return;
            }
            std::string encoder = codecList->FindAudioEncoder(asyncCtx->format);
            asyncCtx->JsResult = std::make_unique<MediaJsResultString>(encoder);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AVCodecCapabilityNapi::GetVideoDecoderCaps(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVCodecCapabilityAsyncContext>(env);

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
    napi_create_string_utf8(env, "GetVideoDecoderCaps", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AVCodecCapabilityAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            asyncCtx->JsResult = std::make_unique<MediaJsVideoCapsStatic>(true);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AVCodecCapabilityNapi::FindVideoDecoder(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVCodecCapabilityAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_object) {
        (void)AVCodecNapiUtil::ExtractMediaFormat(env, args[0], asyncCtx->format);
    } else {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Illegal argument");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[1]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "FindVideoDecoder", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AVCodecCapabilityAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            auto codecList = AVCodecListFactory::CreateAVCodecList();
            if (codecList == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "No memory");
                return;
            }
            std::string decoder = codecList->FindVideoDecoder(asyncCtx->format);
            asyncCtx->JsResult = std::make_unique<MediaJsResultString>(decoder);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AVCodecCapabilityNapi::GetVideoEncoderCaps(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVCodecCapabilityAsyncContext>(env);

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
    napi_create_string_utf8(env, "GetVideoEncoderCaps", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AVCodecCapabilityAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            asyncCtx->JsResult = std::make_unique<MediaJsVideoCapsStatic>(false);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_value AVCodecCapabilityNapi::FindVideoEncoder(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    auto asyncCtx = std::make_unique<AVCodecCapabilityAsyncContext>(env);

    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_object) {
        (void)AVCodecNapiUtil::ExtractMediaFormat(env, args[0], asyncCtx->format);
    } else {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Illegal argument");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[1]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "FindVideoEncoder", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void* data) {
            auto asyncCtx = reinterpret_cast<AVCodecCapabilityAsyncContext *>(data);
            if (asyncCtx == nullptr) {
                MEDIA_LOGE("Failed, asyncCtx is nullptr");
                return;
            } else if (asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            auto codecList = AVCodecListFactory::CreateAVCodecList();
            if (codecList == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "No memory");
                return;
            }
            std::string encoder = codecList->FindVideoEncoder(asyncCtx->format);
            asyncCtx->JsResult = std::make_unique<MediaJsResultString>(encoder);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}

napi_status AddCodecInfo(napi_env env, napi_value result, std::shared_ptr<AVCodecInfo> info)
{
    CHECK_AND_RETURN_RET(info != nullptr, napi_generic_failure);

    napi_value obj = nullptr;
    napi_status status = napi_create_object(env, &obj);
    CHECK_AND_RETURN_RET(status == napi_ok, status);

    (void)CommonNapi::SetPropertyString(env, obj, "name", info->GetName());
    (void)CommonNapi::SetPropertyInt32(env, obj, "type", static_cast<int32_t>(info->GetType()));
    (void)CommonNapi::SetPropertyString(env, obj, "mimeType", info->GetMimeType());
    (void)CommonNapi::SetPropertyInt32(env, obj, "isHardwareAccelerated",
        static_cast<int32_t>(info->IsHardwareAccelerated()));
    (void)CommonNapi::SetPropertyInt32(env, obj, "isSoftwareOnly", static_cast<int32_t>(info->IsSoftwareOnly()));
    (void)CommonNapi::SetPropertyInt32(env, obj, "isVendor", static_cast<int32_t>(info->IsVendor()));

    napi_value nameStr = nullptr;
    status = napi_create_string_utf8(env, "codecInfo", NAPI_AUTO_LENGTH, &nameStr);
    CHECK_AND_RETURN_RET(status == napi_ok, napi_generic_failure);

    status = napi_set_property(env, result, nameStr, obj);
    CHECK_AND_RETURN_RET(status == napi_ok, napi_generic_failure);

    return napi_ok;
}

napi_status MediaJsAudioCapsDynamic::GetJsResult(napi_env env, napi_value &result)
{
    auto codecList = AVCodecListFactory::CreateAVCodecList();
    CHECK_AND_RETURN_RET(codecList != nullptr, napi_generic_failure);

    std::vector<std::shared_ptr<AudioCaps>> audioCaps;
    if (isDecoder_) {
        audioCaps = codecList->GetAudioDecoderCaps();
    } else {
        audioCaps = codecList->GetAudioEncoderCaps();
    }

    napi_status status = napi_create_object(env, &result);
    CHECK_AND_RETURN_RET(status == napi_ok, napi_generic_failure);

    for (auto it = audioCaps.begin(); it != audioCaps.end(); it++) {
        CHECK_AND_CONTINUE((*it) != nullptr);

        auto info = (*it)->GetCodecInfo();
        CHECK_AND_CONTINUE(info != nullptr);

        if (info->GetName() != name_) {
            continue;
        }

        (void)AddCodecInfo(env, result, info);

        Range range = (*it)->GetSupportedBitrate();
        (void)CommonNapi::AddRangeProperty(env, result, "supportedBitrate", range.minVal, range.maxVal);

        range = (*it)->GetSupportedChannel();
        (void)CommonNapi::AddRangeProperty(env, result, "supportedChannel", range.minVal, range.maxVal);

        range = (*it)->GetSupportedComplexity();
        (void)CommonNapi::AddRangeProperty(env, result, "supportedComplexity", range.minVal, range.maxVal);

        std::vector<int32_t> vec = (*it)->GetSupportedFormats();
        (void)CommonNapi::AddArrayProperty(env, result, "supportedFormats", vec);

        vec = (*it)->GetSupportedSampleRates();
        (void)CommonNapi::AddArrayProperty(env, result, "supportedSampleRates", vec);

        vec = (*it)->GetSupportedProfiles();
        (void)CommonNapi::AddArrayProperty(env, result, "supportedProfiles", vec);

        vec = (*it)->GetSupportedLevels();
        (void)CommonNapi::AddArrayProperty(env, result, "supportedLevels", vec);
    }

    return napi_ok;
}

napi_status MediaJsAudioCapsStatic::GetJsResult(napi_env env, napi_value &result)
{
    auto codecList = AVCodecListFactory::CreateAVCodecList();
    CHECK_AND_RETURN_RET(codecList != nullptr, napi_generic_failure);

    std::vector<std::shared_ptr<AudioCaps>> audioCaps;
    if (isDecoder_) {
        audioCaps = codecList->GetAudioDecoderCaps();
    } else {
        audioCaps = codecList->GetAudioEncoderCaps();
    }

    napi_status status = napi_create_array_with_length(env, audioCaps.size(), &result);
    CHECK_AND_RETURN_RET(status == napi_ok, status);

    int32_t index = 0;
    for (auto it = audioCaps.begin(); it != audioCaps.end(); it++) {
        CHECK_AND_CONTINUE((*it) != nullptr);

        napi_value obj = nullptr;
        status = napi_create_object(env, &obj);
        CHECK_AND_CONTINUE(status == napi_ok);

        Range range = (*it)->GetSupportedBitrate();
        (void)CommonNapi::AddRangeProperty(env, obj, "supportedBitrate", range.minVal, range.maxVal);

        range = (*it)->GetSupportedChannel();
        (void)CommonNapi::AddRangeProperty(env, obj, "supportedChannel", range.minVal, range.maxVal);

        range = (*it)->GetSupportedComplexity();
        (void)CommonNapi::AddRangeProperty(env, obj, "supportedComplexity", range.minVal, range.maxVal);

        std::vector<int32_t> vec = (*it)->GetSupportedFormats();
        (void)CommonNapi::AddArrayProperty(env, obj, "supportedFormats", vec);

        vec = (*it)->GetSupportedSampleRates();
        (void)CommonNapi::AddArrayProperty(env, obj, "supportedSampleRates", vec);

        vec = (*it)->GetSupportedProfiles();
        (void)CommonNapi::AddArrayProperty(env, obj, "supportedProfiles", vec);

        vec = (*it)->GetSupportedLevels();
        (void)CommonNapi::AddArrayProperty(env, obj, "supportedLevels", vec);

        auto codecInfo = (*it)->GetCodecInfo();
        if (codecInfo != nullptr) {
            (void)AddCodecInfo(env, obj, codecInfo);
        }

        (void)napi_set_element(env, result, index, obj);
        index++;
    }

    return napi_ok;
}

napi_status MediaJsVideoCapsStatic::GetJsResult(napi_env env, napi_value &result)
{
    auto codecList = AVCodecListFactory::CreateAVCodecList();
    CHECK_AND_RETURN_RET(codecList != nullptr, napi_generic_failure);

    std::vector<std::shared_ptr<VideoCaps>> caps;
    if (isDecoder_) {
        caps = codecList->GetVideoDecoderCaps();
    } else {
        caps = codecList->GetVideoEncoderCaps();
    }

    CHECK_AND_RETURN_RET(napi_create_array_with_length(env, caps.size(), &result) == napi_ok, napi_generic_failure);

    int32_t index = 0;
    for (auto it = caps.begin(); it != caps.end(); it++) {
        CHECK_AND_CONTINUE((*it) != nullptr);

        napi_value videoCaps = MediaCapabilityVCapsNapi::Create(env, *it);
        CHECK_AND_CONTINUE(videoCaps != nullptr);

        (void)napi_set_element(env, result, index, videoCaps);
        index++;
    }

    return napi_ok;
}

napi_status MediaJsVideoCapsDynamic::GetJsResult(napi_env env, napi_value &result)
{
    auto codecList = AVCodecListFactory::CreateAVCodecList();
    CHECK_AND_RETURN_RET(codecList != nullptr, napi_generic_failure);

    std::vector<std::shared_ptr<VideoCaps>> caps;
    if (isDecoder_) {
        caps = codecList->GetVideoDecoderCaps();
    } else {
        caps = codecList->GetVideoEncoderCaps();
    }

    for (auto it = caps.begin(); it != caps.end(); it++) {
        CHECK_AND_CONTINUE((*it) != nullptr);
        auto info = (*it)->GetCodecInfo();
        CHECK_AND_CONTINUE(info != nullptr);
        if (info->GetName() != name_) {
            continue;
        }

        result = MediaCapabilityVCapsNapi::Create(env, *it);
        CHECK_AND_RETURN_RET(result != nullptr, napi_generic_failure);
        break;
    }

    return napi_ok;
}
}  // namespace Media
}  // namespace OHOS
