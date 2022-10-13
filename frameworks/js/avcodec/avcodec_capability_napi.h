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

#ifndef AVCODEC_CAPABILITY_NAPI_H
#define AVCODEC_CAPABILITY_NAPI_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "common_napi.h"

namespace OHOS {
namespace Media {
class MediaJsAudioCapsStatic : public MediaJsResult {
public:
    explicit MediaJsAudioCapsStatic(bool isDecoder)
        : isDecoder_(isDecoder)
    {
    }
    ~MediaJsAudioCapsStatic() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;

private:
    bool isDecoder_;
};

class MediaJsVideoCapsStatic : public MediaJsResult {
public:
    explicit MediaJsVideoCapsStatic(bool isDecoder)
        : isDecoder_(isDecoder)
    {
    }
    ~MediaJsVideoCapsStatic() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;

private:
    bool isDecoder_;
};

class MediaJsVideoCapsDynamic : public MediaJsResult {
public:
    MediaJsVideoCapsDynamic(const std::string &name, const bool &isDecoder)
        : name_(name),
          isDecoder_(isDecoder)
    {
    }
    ~MediaJsVideoCapsDynamic() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;

private:
    std::string name_;
    bool isDecoder_;
};

class MediaJsAudioCapsDynamic : public MediaJsResult {
public:
    MediaJsAudioCapsDynamic(const std::string &name, const bool &isDecoder)
        : name_(name),
          isDecoder_(isDecoder)
    {
    }
    ~MediaJsAudioCapsDynamic() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;

private:
    std::string name_;
    bool isDecoder_;
};

class AVCodecCapabilityNapi {
public:
    static napi_value GetAudioDecoderCaps(napi_env env, napi_callback_info info);
    static napi_value FindAudioDecoder(napi_env env, napi_callback_info info);
    static napi_value GetAudioEncoderCaps(napi_env env, napi_callback_info info);
    static napi_value FindAudioEncoder(napi_env env, napi_callback_info info);
    static napi_value GetVideoDecoderCaps(napi_env env, napi_callback_info info);
    static napi_value FindVideoDecoder(napi_env env, napi_callback_info info);
    static napi_value GetVideoEncoderCaps(napi_env env, napi_callback_info info);
    static napi_value FindVideoEncoder(napi_env env, napi_callback_info info);

private:
    AVCodecCapabilityNapi() = delete;
    ~AVCodecCapabilityNapi() = delete;
};

struct AVCodecCapabilityAsyncContext : public MediaAsyncContext {
    explicit AVCodecCapabilityAsyncContext(napi_env env) : MediaAsyncContext(env) {}
    ~AVCodecCapabilityAsyncContext() = default;

    AVCodecCapabilityNapi *napi = nullptr;
    Format format;
};
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_CAPABILITY_NAPI_H