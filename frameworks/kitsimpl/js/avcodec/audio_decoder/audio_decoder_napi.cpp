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

#include "audio_decoder_napi.h"
#include <climits>
#include "audio_decoder_callback_napi.h"
#include "media_log.h"
#include "media_errors.h"
#include "common_napi.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioDecoderNapi"};
}

namespace OHOS {
namespace Media {
napi_ref AudioDecoderNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "AudioDecodeProcessor";

AudioDecoderNapi::AudioDecoderNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AudioDecoderNapi::~AudioDecoderNapi()
{
    cbNapi_ = nullptr;
    adec_ = nullptr;
    if (wrap_ != nullptr) {
        napi_delete_reference(env_, wrap_);
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value AudioDecoderNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("configure", Configure),
        DECLARE_NAPI_FUNCTION("prepare", Prepare),
        DECLARE_NAPI_FUNCTION("start", Start),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("flush", Flush),
        DECLARE_NAPI_FUNCTION("reset", Reset),
        DECLARE_NAPI_FUNCTION("queueInput", QueueInput),
        DECLARE_NAPI_FUNCTION("releaseOutput", ReleaseOutput),
        DECLARE_NAPI_FUNCTION("setParameter", SetParameter),
        DECLARE_NAPI_FUNCTION("getOutputMediaDescription", GetOutputMediaDescription),
        DECLARE_NAPI_FUNCTION("getAudioDecoderCaps", GetAudioDecoderCaps),
        DECLARE_NAPI_FUNCTION("on", On),
    };
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createAudioDecoderByMime", CreateAudioDecoderByMime),
        DECLARE_NAPI_STATIC_FUNCTION("createAudioDecoderByName", CreateAudioDecoderByName)
    };

    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define AudioDecoder class");

    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");

    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");

    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");

    MEDIA_LOGD("Init success");
    return exports;
}

napi_value AudioDecoderNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    napi_value jsThis = nullptr;
    size_t argCount = 2;
    napi_value args[2] = {0};
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET(status == napi_ok && args[0] != nullptr && args[1] != nullptr, undefined);

    AudioDecoderNapi *adecNapi = new(std::nothrow) AudioDecoderNapi();
    CHECK_AND_RETURN_RET(adecNapi != nullptr, undefined);

    adecNapi->env_ = env;
    std::string name = CommonNapi::GetStringArgument(env, args[0]);

    bool useMime = false;
    status = napi_get_value_bool(env, args[1], &useMime);
    CHECK_AND_RETURN_RET(status == napi_ok, undefined);
    if (useMime) {
        adecNapi->adec_ = AudioDecoderFactory::CreateByMimeType(name);
    } else {
        adecNapi->adec_ = AudioDecoderFactory::CreateByDecoderName(name);
    }
    CHECK_AND_RETURN_RET(adecNapi->adec_ != nullptr, undefined);

    if (adecNapi->cbNapi_ == nullptr) {
        adecNapi->cbNapi_ = std::make_shared<AudioDecoderCallbackNapi>(env);
        (void)adecNapi->adec_->SetCallback(adecNapi->cbNapi_);
    }

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(adecNapi),
        AudioDecoderNapi::Destructor, nullptr, &(adecNapi->wrap_));
    if (status != napi_ok) {
        delete adecNapi;
        MEDIA_LOGE("Failed to wrap native instance");
        return undefined;
    }

    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void AudioDecoderNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        delete reinterpret_cast<AudioDecoderNapi *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}

napi_value AudioDecoderNapi::CreateAudioDecoderByMime(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    return undefinedResult;
}

napi_value AudioDecoderNapi::CreateAudioDecoderByName(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    return undefinedResult;
}

napi_value AudioDecoderNapi::Configure(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    return undefinedResult;
}

napi_value AudioDecoderNapi::Prepare(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    return undefinedResult;
}

napi_value AudioDecoderNapi::Start(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    return undefinedResult;
}

napi_value AudioDecoderNapi::Stop(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    return undefinedResult;
}

napi_value AudioDecoderNapi::Flush(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    return undefinedResult;
}

napi_value AudioDecoderNapi::Reset(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    return undefinedResult;
}

napi_value AudioDecoderNapi::QueueInput(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    return undefinedResult;
}

napi_value AudioDecoderNapi::ReleaseOutput(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    return undefinedResult;
}

napi_value AudioDecoderNapi::SetParameter(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    return undefinedResult;
}

napi_value AudioDecoderNapi::GetOutputMediaDescription(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    return undefinedResult;
}

napi_value AudioDecoderNapi::GetAudioDecoderCaps(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    return undefinedResult;
}

napi_value AudioDecoderNapi::On(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    return undefinedResult;
}
}  // namespace Media
}  // namespace OHOS
