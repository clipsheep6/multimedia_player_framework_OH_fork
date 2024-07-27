/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "tone_attrs_napi.h"

#include "media_log.h"
#include "common_napi.h"

#include "tokenid_kit.h"
#include "ipc_skeleton.h"
#include "access_token.h"
#include "accesstoken_kit.h"
#include "ringtone_common_napi.h"

using OHOS::Security::AccessToken::AccessTokenKit;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO_NAPI, "ToneAttrsNapi"};
}

namespace OHOS {
namespace Media {

thread_local napi_ref ToneAttrsNapi::sConstructor_ = nullptr;
std::shared_ptr<ToneAttrs> ToneAttrsNapi::sToneAttrs_ = nullptr;

ToneAttrsNapi::ToneAttrsNapi() : env_(nullptr) {}

ToneAttrsNapi::~ToneAttrsNapi() = default;

napi_value ToneAttrsNapi::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;

    napi_property_descriptor tone_attrs_prop[] = {
        DECLARE_NAPI_FUNCTION("getTitle", GetTitle),
        DECLARE_NAPI_FUNCTION("setTitle", SetTitle),
        DECLARE_NAPI_FUNCTION("getFileName", GetFileName),
        DECLARE_NAPI_FUNCTION("setFileName", SetFileName),
        DECLARE_NAPI_FUNCTION("getUri", GetUri),
        DECLARE_NAPI_FUNCTION("getCustomizedType", GetCustomizedType),
        DECLARE_NAPI_FUNCTION("setCategory", SetCategory),
        DECLARE_NAPI_FUNCTION("getCategory", GetCategory),
    };

    status = napi_define_class(env, TONE_ATTRS_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH,
        Construct, nullptr, sizeof(tone_attrs_prop) / sizeof(tone_attrs_prop[0]),
        tone_attrs_prop, &ctorObj);
    if (status == napi_ok) {
        if (napi_create_reference(env, ctorObj, refCount, &sConstructor_) == napi_ok) {
            status = napi_set_named_property(env, exports, TONE_ATTRS_NAPI_CLASS_NAME.c_str(), ctorObj);
            if (status == napi_ok) {
                return exports;
            }
        }
    }

    return nullptr;
}

napi_value ToneAttrsNapi::Construct(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsThis = nullptr;
    napi_value thisVar = nullptr;

    status = napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr);
    if (status == napi_ok) {
        std::unique_ptr<ToneAttrsNapi> obj = std::make_unique<ToneAttrsNapi>();
        if (obj != nullptr && thisVar != nullptr) {
            obj->env_ = env;
            status = napi_wrap(env, thisVar, static_cast<void*>(obj.get()),
                               ToneAttrsNapi::Destructor, nullptr, nullptr);
            if (status == napi_ok) {
                obj.release();
                return thisVar;
            }
        }
    }
    MEDIA_LOGE("Failed in ToneAttrsNapi::Construct()!");
    napi_get_undefined(env, &jsThis);
    return jsThis;
}

void ToneAttrsNapi::Destructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    ToneAttrsNapi *ToneAttrsHelper = reinterpret_cast<ToneAttrsNapi*>(nativeObject);
    if (ToneAttrsHelper != nullptr) {
        ObjectRefMap<ToneAttrsNapi>::DecreaseRef(ToneAttrsHelper);
    }
}

napi_status ToneAttrsNapi::NewInstance(napi_env env, std::shared_ptr<ToneAttrs>& nativeToneAttrs, napi_value& out)
{
    napi_value constructor {};
    NAPI_CALL_BASE(env, napi_get_reference_value(env, sConstructor_, &constructor), napi_generic_failure);
    napi_value instance{};
    NAPI_CALL_BASE(env, napi_new_instance(env, constructor, 0, nullptr, &instance), napi_generic_failure);

    ToneAttrsNapi* toneAttrsNapi{};
    NAPI_CALL_BASE(env, napi_unwrap(env, instance, reinterpret_cast<void**>(&toneAttrsNapi)), napi_generic_failure);
    CHECK_AND_RETURN_RET_LOG(toneAttrsNapi != nullptr, napi_invalid_arg, "toneAttrsNapi is nullptr");

    toneAttrsNapi->toneAttrs_ = std::move(nativeToneAttrs);
    out = instance;
    return napi_ok;
}

std::shared_ptr<ToneAttrs> ToneAttrsNapi::GetToneAttrs()
{
    return toneAttrs_;
}

napi_value ToneAttrsNapi::ThrowErrorAndReturn(napi_env env, const std::string& errMsg, int32_t errCode)
{
    napi_value message = nullptr;
    napi_value code = nullptr;
    napi_value errVal = nullptr;
    napi_value errNameVal = nullptr;
    napi_value result{};
    napi_create_string_utf8(env, errMsg.c_str(), NAPI_AUTO_LENGTH, &message);
    napi_create_error(env, nullptr, message, &errVal);
    napi_create_int32(env, errCode, &code);
    napi_set_named_property(env, errVal, "code", code);
    napi_create_string_utf8(env, errMsg.c_str(), NAPI_AUTO_LENGTH, &errNameVal);
    napi_set_named_property(env, errVal, "BusinessError", errNameVal);
    napi_throw(env, errVal);
    napi_get_undefined(env, &result);
    return result;
}

bool ToneAttrsNapi::VerifySelfSystemPermission()
{
    Security::AccessToken::FullTokenID selfTokenID = IPCSkeleton::GetSelfTokenID();
    auto tokenTypeFlag = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(static_cast<uint32_t>(selfTokenID));
    if (tokenTypeFlag == Security::AccessToken::TOKEN_NATIVE ||
        tokenTypeFlag == Security::AccessToken::TOKEN_SHELL ||
        Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(selfTokenID)) {
        return true;
    }
    return false;
}

napi_value ToneAttrsNapi::GetTitle(napi_env env, napi_callback_info info)
{
    ToneAttrsNapi *toneAttrsNapi = nullptr;
    napi_value jsThis = nullptr;
    size_t argc = 0;
    CHECK_AND_RETURN_RET_LOG(VerifySelfSystemPermission(),
        ThrowErrorAndReturn(env, NAPI_ERR_PERMISSION_DENIED_INFO, NAPI_ERR_PERMISSION_DENIED),
        "No system permission");

    napi_status status = napi_get_cb_info(env, info, &argc, nullptr, &jsThis, nullptr);

    CHECK_AND_RETURN_RET_LOG((status == napi_ok) && (jsThis != nullptr), nullptr, "jsThis is nullptr");
    napi_unwrap(env, jsThis, reinterpret_cast<void**>(&toneAttrsNapi));
    CHECK_AND_RETURN_RET_LOG(toneAttrsNapi != nullptr, nullptr, "toneAttrsNapi is nullptr");
    CHECK_AND_RETURN_RET_LOG(toneAttrsNapi->toneAttrs_ != nullptr, nullptr, "toneAttrs_ is nullptr");
    napi_value result;
    napi_create_string_utf8(env,
        toneAttrsNapi->toneAttrs_->GetTitle().c_str(), NAPI_AUTO_LENGTH, &result);
    return result;
}

napi_value ToneAttrsNapi::SetTitle(napi_env env, napi_callback_info info)
{
    ToneAttrsNapi *toneAttrsNapi = nullptr;
    napi_value jsThis = nullptr;
    size_t argc = 1;
    napi_value argv[1] = {};
    CHECK_AND_RETURN_RET_LOG(VerifySelfSystemPermission(),
        ThrowErrorAndReturn(env, NAPI_ERR_PERMISSION_DENIED_INFO, NAPI_ERR_PERMISSION_DENIED),
        "No system permission");

    napi_status status = napi_get_cb_info(env, info, &argc, argv, &jsThis, nullptr);

    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, argv[0], &valueType);
    CHECK_AND_RETURN_RET_LOG(valueType == napi_string, nullptr, "title is not string");

    std::string toneAttrsTitle = CommonNapi::GetStringArgument(env, argv[0]);
    CHECK_AND_RETURN_RET_LOG(argc == 1 && !toneAttrsTitle.empty(),
        ThrowErrorAndReturn(env, NAPI_ERR_INPUT_INVALID_INFO, NAPI_ERR_INPUT_INVALID),
        "invalid arguments");
    CHECK_AND_RETURN_RET_LOG((status == napi_ok) && (jsThis != nullptr), nullptr, "jsThis is nullptr");

    napi_unwrap(env, jsThis, reinterpret_cast<void**>(&toneAttrsNapi));
    CHECK_AND_RETURN_RET_LOG(toneAttrsNapi != nullptr, nullptr, "toneAttrsNapi is nullptr");
    CHECK_AND_RETURN_RET_LOG(toneAttrsNapi->toneAttrs_ != nullptr, nullptr, "toneAttrs_ is nullptr");
    toneAttrsNapi->toneAttrs_->SetTitle(toneAttrsTitle);
    return nullptr;
}

napi_value ToneAttrsNapi::GetFileName(napi_env env, napi_callback_info info)
{
    ToneAttrsNapi *toneAttrsNapi = nullptr;
    napi_value jsThis = nullptr;
    size_t argc = 0;
    CHECK_AND_RETURN_RET_LOG(VerifySelfSystemPermission(),
        ThrowErrorAndReturn(env, NAPI_ERR_PERMISSION_DENIED_INFO, NAPI_ERR_PERMISSION_DENIED),
        "No system permission");

    napi_status status = napi_get_cb_info(env, info, &argc, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG((status == napi_ok) && (jsThis != nullptr), nullptr, "jsThis is nullptr");
    napi_unwrap(env, jsThis, reinterpret_cast<void**>(&toneAttrsNapi));
    CHECK_AND_RETURN_RET_LOG(toneAttrsNapi != nullptr, nullptr, "toneAttrsNapi is nullptr");
    CHECK_AND_RETURN_RET_LOG(toneAttrsNapi->toneAttrs_ != nullptr, nullptr, "toneAttrs_ is nullptr");
    napi_value result;
    napi_create_string_utf8(env,
        toneAttrsNapi->toneAttrs_->GetFileName().c_str(), NAPI_AUTO_LENGTH, &result);
    return result;
}

napi_value ToneAttrsNapi::SetFileName(napi_env env, napi_callback_info info)
{
    ToneAttrsNapi *toneAttrsNapi = nullptr;
    napi_value jsThis = nullptr;
    size_t argc = 1;
    napi_value argv[1] = {};
    CHECK_AND_RETURN_RET_LOG(VerifySelfSystemPermission(),
        ThrowErrorAndReturn(env, NAPI_ERR_PERMISSION_DENIED_INFO, NAPI_ERR_PERMISSION_DENIED),
        "No system permission");

    napi_status status = napi_get_cb_info(env, info, &argc, argv, &jsThis, nullptr);

    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, argv[0], &valueType);
    CHECK_AND_RETURN_RET_LOG(valueType == napi_string, nullptr, "filename is not string");

    std::string toneAttrsFileName = CommonNapi::GetStringArgument(env, argv[0]);
    CHECK_AND_RETURN_RET_LOG(argc == 1 && !toneAttrsFileName.empty(),
        ThrowErrorAndReturn(env, NAPI_ERR_INPUT_INVALID_INFO, NAPI_ERR_INPUT_INVALID),
        "invalid arguments");
    CHECK_AND_RETURN_RET_LOG((status == napi_ok) && (jsThis != nullptr), nullptr, "jsThis is nullptr");

    napi_unwrap(env, jsThis, reinterpret_cast<void**>(&toneAttrsNapi));
    CHECK_AND_RETURN_RET_LOG(toneAttrsNapi != nullptr, nullptr, "toneAttrsNapi is nullptr");
    CHECK_AND_RETURN_RET_LOG(toneAttrsNapi->toneAttrs_ != nullptr, nullptr, "toneAttrs_ is nullptr");
    toneAttrsNapi->toneAttrs_->SetFileName(toneAttrsFileName);
    return nullptr;
}

napi_value ToneAttrsNapi::GetUri(napi_env env, napi_callback_info info)
{
    ToneAttrsNapi *toneAttrsNapi = nullptr;
    napi_value jsThis = nullptr;
    size_t argc = 0;
    CHECK_AND_RETURN_RET_LOG(VerifySelfSystemPermission(),
        ThrowErrorAndReturn(env, NAPI_ERR_PERMISSION_DENIED_INFO, NAPI_ERR_PERMISSION_DENIED),
        "No system permission");

    napi_status status = napi_get_cb_info(env, info, &argc, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG((status == napi_ok) && (jsThis != nullptr), nullptr, "jsThis is nullptr");
    napi_unwrap(env, jsThis, reinterpret_cast<void**>(&toneAttrsNapi));
    CHECK_AND_RETURN_RET_LOG(toneAttrsNapi != nullptr, nullptr, "toneAttrsNapi is nullptr");
    CHECK_AND_RETURN_RET_LOG(toneAttrsNapi->toneAttrs_ != nullptr, nullptr, "toneAttrs_ is nullptr");
    napi_value result;
    napi_create_string_utf8(env,
        toneAttrsNapi->toneAttrs_->GetUri().c_str(), NAPI_AUTO_LENGTH, &result);
    return result;
}

napi_value ToneAttrsNapi::GetCustomizedType(napi_env env, napi_callback_info info)
{
    ToneAttrsNapi *toneAttrsNapi = nullptr;
    napi_value jsThis = nullptr;
    size_t argc = 0;
    CHECK_AND_RETURN_RET_LOG(VerifySelfSystemPermission(),
        ThrowErrorAndReturn(env, NAPI_ERR_PERMISSION_DENIED_INFO, NAPI_ERR_PERMISSION_DENIED),
        "No system permission");

    napi_status status = napi_get_cb_info(env, info, &argc, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG((status == napi_ok) && (jsThis != nullptr), nullptr, "jsThis is nullptr");
    napi_unwrap(env, jsThis, reinterpret_cast<void**>(&toneAttrsNapi));
    CHECK_AND_RETURN_RET_LOG(toneAttrsNapi != nullptr, nullptr, "toneAttrsNapi is nullptr");
    CHECK_AND_RETURN_RET_LOG(toneAttrsNapi->toneAttrs_ != nullptr, nullptr, "toneAttrs_ is nullptr");
    napi_value result;
    napi_create_int32(env, toneAttrsNapi->toneAttrs_->GetCustomizedType(), &result);
    return result;
}

napi_value ToneAttrsNapi::SetCategory(napi_env env, napi_callback_info info)
{
    ToneAttrsNapi *toneAttrsNapi = nullptr;
    napi_value jsThis = nullptr;
    size_t argc = 1;
    napi_value argv[1] = {};
    CHECK_AND_RETURN_RET_LOG(VerifySelfSystemPermission(),
        ThrowErrorAndReturn(env, NAPI_ERR_PERMISSION_DENIED_INFO, NAPI_ERR_PERMISSION_DENIED),
        "No system permission");

    napi_status status = napi_get_cb_info(env, info, &argc, argv, &jsThis, nullptr);

    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, argv[0], &valueType);

    bool isCategoryValid = false;
    int32_t toneAttrsCategory = TONE_CATEGORY_INVALID;
    napi_get_value_int32(env, argv[0], &toneAttrsCategory);
    if (toneAttrsCategory == TONE_CATEGORY_RINGTONE || toneAttrsCategory == TONE_CATEGORY_TEXT_MESSAGE ||
        toneAttrsCategory == TONE_CATEGORY_NOTIFICATION || toneAttrsCategory == TONE_CATEGORY_ALARM) {
        isCategoryValid = true;
    }
    CHECK_AND_RETURN_RET_LOG(argc == 1 && isCategoryValid,
        ThrowErrorAndReturn(env, NAPI_ERR_INPUT_INVALID_INFO, NAPI_ERR_INPUT_INVALID),
        "invalid arguments");
    CHECK_AND_RETURN_RET_LOG((status == napi_ok) && (jsThis != nullptr), nullptr, "jsThis is nullptr");

    napi_unwrap(env, jsThis, reinterpret_cast<void**>(&toneAttrsNapi));
    CHECK_AND_RETURN_RET_LOG(toneAttrsNapi != nullptr, nullptr, "toneAttrsNapi is nullptr");
    CHECK_AND_RETURN_RET_LOG(toneAttrsNapi->toneAttrs_ != nullptr, nullptr, "toneAttrs_ is nullptr");
    toneAttrsNapi->toneAttrs_->SetCategory(toneAttrsCategory);
    return nullptr;
}

napi_value ToneAttrsNapi::GetCategory(napi_env env, napi_callback_info info)
{
    ToneAttrsNapi *toneAttrsNapi = nullptr;
    napi_value jsThis = nullptr;
    size_t argc = 0;
    CHECK_AND_RETURN_RET_LOG(VerifySelfSystemPermission(),
        ThrowErrorAndReturn(env, NAPI_ERR_PERMISSION_DENIED_INFO, NAPI_ERR_PERMISSION_DENIED),
        "No system permission");

    napi_status status = napi_get_cb_info(env, info, &argc, nullptr, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG((status == napi_ok) && (jsThis != nullptr), nullptr, "jsThis is nullptr");
    napi_unwrap(env, jsThis, reinterpret_cast<void**>(&toneAttrsNapi));
    CHECK_AND_RETURN_RET_LOG(toneAttrsNapi != nullptr, nullptr, "toneAttrsNapi is nullptr");
    CHECK_AND_RETURN_RET_LOG(toneAttrsNapi->toneAttrs_ != nullptr, nullptr, "toneAttrs_ is nullptr");
    napi_value result;
    napi_create_int32(env, toneAttrsNapi->toneAttrs_->GetCategory(), &result);
    return result;
}

} // namespace Media
} // namespace OHOS