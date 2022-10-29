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

#ifndef RECORDER_CAPABILITY_NAPI_H
#define RECORDER_CAPABILITY_NAPI_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "common_napi.h"
#include "recorder_profiles.h"

namespace OHOS {
namespace Media {
class MediaJsAudioRecorderCapsArray : public MediaJsResult {
public:
    explicit MediaJsAudioRecorderCapsArray(const std::vector<std::shared_ptr<OHOS::Media::AudioRecorderCaps>> &value)
        : value_(value)
    {
    }
    ~MediaJsAudioRecorderCapsArray() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;

private:
    std::vector<std::shared_ptr<OHOS::Media::AudioRecorderCaps>> value_;
};

class MediaJsVideoRecorderCapsArray : public MediaJsResult {
public:
    explicit MediaJsVideoRecorderCapsArray(const std::vector<std::shared_ptr<OHOS::Media::VideoRecorderCaps>> &value)
        : value_(value)
    {
    }
    ~MediaJsVideoRecorderCapsArray() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;

private:
    std::vector<std::shared_ptr<OHOS::Media::VideoRecorderCaps>> value_;
};

class MediaJsVideoRecorderProfile : public MediaJsResult {
public:
    explicit MediaJsVideoRecorderProfile(std::shared_ptr<OHOS::Media::VideoRecorderProfile> value)
        : value_(value)
    {
    }
    ~MediaJsVideoRecorderProfile() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;

private:
    std::shared_ptr<OHOS::Media::VideoRecorderProfile> value_;
};

class RecorderCapabilityNapi {
public:
    static napi_value GetAudioRecorderCaps(napi_env env, napi_callback_info info);
    static napi_value IsAudioRecorderConfigSupported(napi_env env, napi_callback_info info);
    static napi_value GetVideoRecorderCaps(napi_env env, napi_callback_info info);
    static napi_value GetVideoRecorderProfile(napi_env env, napi_callback_info info);
    static napi_value HasVideoRecorderProfile(napi_env env, napi_callback_info info);

private:
    static bool ExtractAudioRecorderProfile(napi_env env, napi_value profile, AudioRecorderProfile &result);
    RecorderCapabilityNapi() = delete;
    ~RecorderCapabilityNapi() = delete;
};

struct RecorderCapabilityAsyncContext : public MediaAsyncContext {
    explicit RecorderCapabilityAsyncContext(napi_env env) : MediaAsyncContext(env) {}
    ~RecorderCapabilityAsyncContext() = default;

    RecorderCapabilityNapi *napi = nullptr;
    Format format;
    int32_t sourceId = 0;
    int32_t qualityLevel = 0;
    AudioRecorderProfile profile;
};
} // namespace Media
} // namespace OHOS
#endif // RECORDER_CAPABILITY_NAPI_H