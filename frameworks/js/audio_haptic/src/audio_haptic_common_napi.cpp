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

#include "audio_haptic_common_napi.h"

#include "media_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioHapticCommonNapi"};
}

namespace OHOS {
namespace Media {
const std::string NAPI_ERR_INPUT_INVALID_INFO = "input parameter check failed";
const std::string NAPI_ERR_OPERATE_NOT_ALLOWED_INFO = "operate not allowed";
const std::string NAPI_ERR_IO_ERROR_INFO = "input or output error";
const std::string NAPI_ERR_SERVICE_DIED_INFO = "service died";
const std::string NAPI_ERR_UNSUPPORTED_FORMAT_INFO = "unsupport format";

void AudioHapticCommonNapi::ThrowError(napi_env env, int32_t code)
{
    std::string messageValue = AudioHapticCommonNapi::GetMessageByCode(code);
    napi_throw_error(env, (std::to_string(code)).c_str(), messageValue.c_str());
}

void AudioHapticCommonNapi::ThrowError(napi_env env, int32_t code, const std::string &errMessage)
{
    napi_throw_error(env, (std::to_string(code)).c_str(), errMessage.c_str());
}

std::string AudioHapticCommonNapi::GetMessageByCode(int32_t &code)
{
    std::string errMessage;
    switch (code) {
        case NAPI_ERR_INPUT_INVALID:
            errMessage = NAPI_ERR_INPUT_INVALID_INFO;
            break;
        case NAPI_ERR_OPERATE_NOT_ALLOWED:
            errMessage = NAPI_ERR_OPERATE_NOT_ALLOWED_INFO;
            break;
        case NAPI_ERR_IO_ERROR:
            errMessage = NAPI_ERR_IO_ERROR_INFO;
            break;
        case NAPI_ERR_SERVICE_DIED:
            errMessage = NAPI_ERR_SERVICE_DIED_INFO;
            break;
        case NAPI_ERR_UNSUPPORTED_FORMAT:
            errMessage = NAPI_ERR_UNSUPPORTED_FORMAT_INFO;
            break;
        default:
            errMessage = NAPI_ERR_OPERATE_NOT_ALLOWED_INFO;
            code = NAPI_ERR_OPERATE_NOT_ALLOWED;
            break;
    }
    return errMessage;
}

std::string AudioHapticCommonNapi::GetStringArgument(napi_env env, napi_value value)
{
    std::string strValue = "";
    size_t bufLength = 0;
    napi_status status = napi_get_value_string_utf8(env, value, nullptr, 0, &bufLength);
    if (status == napi_ok && bufLength > 0 && bufLength < PATH_MAX) {
        char *buffer = static_cast<char *>(malloc((bufLength + 1) * sizeof(char)));
        CHECK_AND_RETURN_RET_LOG(buffer != nullptr, strValue, "GetStringArgument: no memory");
        status = napi_get_value_string_utf8(env, value, buffer, bufLength + 1, &bufLength);
        if (status == napi_ok) {
            MEDIA_LOGI("GetStringArgument: argument = %{public}s", buffer);
            strValue = buffer;
        }
        free(buffer);
        buffer = nullptr;
    }
    return strValue;
}
} // namespace Media
} // namespace OHOS