/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef AUDIO_HAPTIC_COMMON_NAPI_H
#define AUDIO_HAPTIC_COMMON_NAPI_H

#include <string>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Media {
const int32_t  NAPI_ERR_INPUT_INVALID = 401;
const int32_t  NAPI_ERR_OPERATE_NOT_ALLOWED = 5400102;
const int32_t  NAPI_ERR_IO_ERROR = 5400103;
const int32_t  NAPI_ERR_SERVICE_DIED = 5400105;
const int32_t  NAPI_ERR_UNSUPPORTED_FORMAT = 5400106;

class AudioHapticCommonNapi {
public:
    AudioHapticCommonNapi() = delete;
    ~AudioHapticCommonNapi() = delete;
    static void ThrowError(napi_env env, int32_t code);
    static void ThrowError(napi_env env, int32_t code, const std::string &errMessage);
    static std::string GetMessageByCode(int32_t &code);
    static std::string GetStringArgument(napi_env env, napi_value value);
};

struct AutoRef {
    AutoRef(napi_env env, napi_ref cb)
        : env_(env), cb_(cb)
    {
    }
    ~AutoRef()
    {
        if (env_ != nullptr && cb_ != nullptr) {
            (void)napi_delete_reference(env_, cb_);
        }
    }
    napi_env env_;
    napi_ref cb_;
};
} // namespace Media
} // namespace OHOS
#endif // AUDIO_HAPTIC_COMMON_NAPI_H