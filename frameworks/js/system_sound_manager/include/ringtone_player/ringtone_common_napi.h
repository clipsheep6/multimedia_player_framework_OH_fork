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

#ifndef RINGTONE_COMMON_NAPI_H
#define RINGTONE_COMMON_NAPI_H

#include <string>

#include "meta/format.h"

#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Media {
const int32_t  NAPI_ERR_INPUT_INVALID = 401;
const int32_t  NAPI_ERR_INVALID_PARAM = 6800101;
const int32_t  NAPI_ERR_NO_MEMORY = 6800102;
const int32_t  NAPI_ERR_UNSUPPORTED = 6800104;
const int32_t  NAPI_ERR_SYSTEM = 6800301;

const std::string NAPI_ERR_INPUT_INVALID_INFO = "input parameter type or number mismatch";
const std::string NAPI_ERR_INVALID_PARAM_INFO = "invalid parameter";
const std::string NAPI_ERR_NO_MEMORY_INFO = "allocate memory failed";
const std::string NAPI_ERR_SYSTEM_INFO = "system error";

class RingtoneCommonNapi {
public:
    RingtoneCommonNapi() = delete;
    ~RingtoneCommonNapi() = delete;
    static std::string GetStringArgument(napi_env env, napi_value value);
    static void ThrowError(napi_env env, int32_t code, const std::string &errMessage);
    static std::string GetMessageByCode(int32_t &code);
};
} // namespace Media
} // namespace OHOS
#endif // RINGTONE_COMMON_NAPI_H