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

#include "avcodec_napi_utils.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecNapiUtil"};
}

namespace OHOS {
namespace Media {
napi_value AVCodecNapiUtil::CreateCodecBuffer(napi_env env, uint32_t index,
    std::shared_ptr<AVSharedMemory> memory, const AVCodecBufferInfo &info, AVCodecBufferFlag flag)
{
    napi_value buffer = nullptr;
    napi_status status = napi_create_object(env, &buffer);
    CHECK_AND_RETURN_RET(status == napi_ok, nullptr);

    CHECK_AND_RETURN_RET(AddNumberProperty(env, buffer, "index", static_cast<int32_t>(index)) == true, nullptr);
    CHECK_AND_RETURN_RET(AddNumberProperty(env, buffer, "offset", info.offset) == true, nullptr);
    CHECK_AND_RETURN_RET(AddNumberProperty(env, buffer, "length", info.size) == true, nullptr);
    CHECK_AND_RETURN_RET(AddNumberProperty(env, buffer, "flags", static_cast<int32_t>(flag)) == true, nullptr);
    CHECK_AND_RETURN_RET(AddNumberProperty(env, buffer, "timeMs", info.presentationTimeUs / 1000) == true, nullptr);

    napi_value dataStr = nullptr;
    status = napi_create_string_utf8(env, "data", NAPI_AUTO_LENGTH, &dataStr);
    CHECK_AND_RETURN_RET(status == napi_ok, nullptr);

    napi_value dataVal = nullptr;
    status = napi_create_arraybuffer(env, info.size, reinterpret_cast<void **>(memory->GetBase()), &dataVal);
    CHECK_AND_RETURN_RET(status == napi_ok, nullptr);

    status = napi_set_property(env, buffer, dataStr, dataVal);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set property");

    return buffer;
}

bool AVCodecNapiUtil::AddNumberProperty(napi_env env, napi_value obj, const std::string &key, int32_t value)
{
    napi_value keyNapi = nullptr;
    napi_status status = napi_create_string_utf8(env, key.c_str(), NAPI_AUTO_LENGTH, &keyNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    napi_value valueNapi = nullptr;
    status = napi_create_int32(env, value, &valueNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    status = napi_set_property(env, obj, keyNapi, valueNapi);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, false, "Failed to set property");

    return true;
}
}
}
