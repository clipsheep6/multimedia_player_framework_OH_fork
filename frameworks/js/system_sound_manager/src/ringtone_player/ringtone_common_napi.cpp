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

#include "ringtone_common_napi.h"
#include <climits>
#include "media_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RingtonePlayerCallbackNapi"};
}

namespace OHOS {
namespace Media {
std::string RingtoneCommonNapi::GetStringArgument(napi_env env, napi_value value)
{
    std::string strValue = "";
    size_t bufLength = 0;
    napi_status status = napi_get_value_string_utf8(env, value, nullptr, 0, &bufLength);
    if (status == napi_ok && bufLength > 0 && bufLength < PATH_MAX) {
        char *buffer = static_cast<char *>(malloc((bufLength + 1) * sizeof(char)));
        CHECK_AND_RETURN_RET_LOG(buffer != nullptr, strValue, "no memory");
        status = napi_get_value_string_utf8(env, value, buffer, bufLength + 1, &bufLength);
        if (status == napi_ok) {
            MEDIA_LOGI("argument = %{public}s", buffer);
            strValue = buffer;
        }
        free(buffer);
        buffer = nullptr;
    }
    return strValue;
}

void RingtoneCommonNapi::ThrowError(napi_env env, int32_t code, const std::string &errMessage)
{
    std::string messageValue;
    if (code == NAPI_ERR_INVALID_PARAM || code == NAPI_ERR_INPUT_INVALID) {
        messageValue = errMessage.c_str();
    } else {
        messageValue = RingtoneCommonNapi::GetMessageByCode(code);
    }
    napi_throw_error(env, (std::to_string(code)).c_str(), messageValue.c_str());
}

std::string RingtoneCommonNapi::GetMessageByCode(int32_t &code)
{
    std::string errMessage;
    switch (code) {
        case NAPI_ERR_INVALID_PARAM:
            errMessage = NAPI_ERR_INVALID_PARAM_INFO;
            break;
        case NAPI_ERR_NO_MEMORY:
            errMessage = NAPI_ERR_NO_MEMORY_INFO;
            break;
        case NAPI_ERR_SYSTEM:
            errMessage = NAPI_ERR_SYSTEM_INFO;
            break;
        case NAPI_ERR_INPUT_INVALID:
            errMessage = NAPI_ERR_INPUT_INVALID_INFO;
            break;
        default:
            errMessage = NAPI_ERR_SYSTEM_INFO;
            code = NAPI_ERR_SYSTEM;
            break;
    }
    return errMessage;
}
} // namespace Media
} // namespace OHOS