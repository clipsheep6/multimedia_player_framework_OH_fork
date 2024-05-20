/*
* Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#ifndef TONE_ATTRS_NAPI_H
#define TONE_ATTRS_NAPI_H

#include <map>
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "tone_attrs.h"

namespace OHOS {
namespace Media {
    static const std::string TONE_ATTRS_NAPI_CLASS_NAME = "ToneAttrs";

class ToneAttrsNapi {
public:

    ToneAttrsNapi();
    ~ToneAttrsNapi();
    static napi_value Init(napi_env env, napi_value exports);
    static napi_status NewInstance(napi_env env, std::shared_ptr<ToneAttrs>& nativeToneAttrs, napi_value& out);
    std::shared_ptr<ToneAttrs> GetToneAttrs();

private:
    static void Destructor(napi_env env, void *nativeObject, void *finalize_hint);
    static napi_value Construct(napi_env env, napi_callback_info info);
    static napi_value ThrowErrorAndReturn(napi_env env, const std::string& napiMessage, int32_t napiCode);
    static bool VerifySelfSystemPermission();

    static napi_value GetTitle(napi_env env, napi_callback_info info);
    static napi_value SetTitle(napi_env env, napi_callback_info info);
    static napi_value GetFileName(napi_env env, napi_callback_info info);
    static napi_value SetFileName(napi_env env, napi_callback_info info);
    static napi_value GetUri(napi_env env, napi_callback_info info);
    static napi_value GetCustomizedType(napi_env env, napi_callback_info info);
    static napi_value SetCategory(napi_env env, napi_callback_info info);
    static napi_value GetCategory(napi_env env, napi_callback_info info);

    napi_env env_;
    std::shared_ptr<ToneAttrs> toneAttrs_;
    static napi_ref sConstructor_;
    static std::shared_ptr<ToneAttrs> sToneAttrs_;
};
} // namespace Media
} // namespace OHOS
#endif // SYSTEM_TONE_NAPI_H
