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

#include "audio_caps_napi.h"
#include <climits>
#include "media_log.h"
#include "media_errors.h"
#include "common_napi.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioCapsNapi"};
}

namespace OHOS {
namespace Media {
napi_ref AudioCapsNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "AudioCaps";

AudioCapsNapi::AudioCapsNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AudioCapsNapi::~AudioCapsNapi()
{
    if (wrap_ != nullptr) {
        napi_delete_reference(env_, wrap_);
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value AudioCapsNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_GETTER("codecInfo", GetCodecInfo),
        DECLARE_NAPI_GETTER("supportedBitrate", GetSupportedBitrate),
        DECLARE_NAPI_GETTER("supportedChannel", GetSupportedChannel),
        DECLARE_NAPI_GETTER("supportedFormats", GetSupportedFormats),
        DECLARE_NAPI_GETTER("supportedSampleRates", GetSupportedSampleRates),
        DECLARE_NAPI_GETTER("supportedProfiles", GetSupportedProfiles),
        DECLARE_NAPI_GETTER("supportedLevels", GetSupportedLevels),
        DECLARE_NAPI_GETTER("supportedComplexity", GetSupportedComplexity),
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define AudioDecodeProcessor class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value AudioCapsNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    return undefined;
}

void AudioCapsNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        delete reinterpret_cast<AudioCapsNapi *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}

napi_value AudioCapsNapi::GetCodecInfo(napi_env env, napi_callback_info info)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    return undefined;
}

napi_value AudioCapsNapi::GetSupportedBitrate(napi_env env, napi_callback_info info)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    return undefined;
}

napi_value AudioCapsNapi::GetSupportedChannel(napi_env env, napi_callback_info info)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    return undefined;
}

napi_value AudioCapsNapi::GetSupportedFormats(napi_env env, napi_callback_info info)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    return undefined;
}

napi_value AudioCapsNapi::GetSupportedSampleRates(napi_env env, napi_callback_info info)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    return undefined;
}

napi_value AudioCapsNapi::GetSupportedProfiles(napi_env env, napi_callback_info info)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    return undefined;
}

napi_value AudioCapsNapi::GetSupportedLevels(napi_env env, napi_callback_info info)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    return undefined;
}

napi_value AudioCapsNapi::GetSupportedComplexity(napi_env env, napi_callback_info info)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    return undefined;
}
}  // namespace Media
}  // namespace OHOS
