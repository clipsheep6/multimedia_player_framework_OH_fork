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

#ifndef MEDIA_CAPABILITY_NAPI_H
#define MEDIA_CAPABILITY_NAPI_H

#include "media_errors.h"
#include "media_capability_utils.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "common_napi.h"
#include "recorder_profiles.h"

namespace OHOS {
namespace Media {
struct MediaCapsAsyncContext;

class MediaCapsNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);

private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    static napi_value GetMediaCapability(napi_env env, napi_callback_info info);
#ifdef SUPPORT_CODEC
    static napi_value GetAudioDecoderCaps(napi_env env, napi_callback_info info);
    static napi_value FindAudioDecoder(napi_env env, napi_callback_info info);
    static napi_value GetAudioEncoderCaps(napi_env env, napi_callback_info info);
    static napi_value FindAudioEncoder(napi_env env, napi_callback_info info);
    static napi_value GetVideoDecoderCaps(napi_env env, napi_callback_info info);
    static napi_value FindVideoDecoder(napi_env env, napi_callback_info info);
    static napi_value GetVideoEncoderCaps(napi_env env, napi_callback_info info);
    static napi_value FindVideoEncoder(napi_env env, napi_callback_info info);
#endif
#ifdef SUPPORT_RECORDER
    static napi_value GetAudioRecorderCaps(napi_env env, napi_callback_info info);
    static napi_value IsAudioRecorderConfigSupported(napi_env env, napi_callback_info info);
    static napi_value GetVideoRecorderCaps(napi_env env, napi_callback_info info);
    static napi_value GetVideoRecorderProfile(napi_env env, napi_callback_info info);
    static napi_value HasVideoRecorderProfile(napi_env env, napi_callback_info info);
#endif
#ifdef SUPPORT_MUXER
    static napi_value GetAVMuxerFormatList(napi_env env, napi_callback_info info);
#endif

    MediaCapsNapi();
    ~MediaCapsNapi();

    static thread_local napi_ref constructor_;
    napi_env env_ = nullptr;
    napi_ref wrap_ = nullptr;
};

struct MediaCapsAsyncContext : public MediaAsyncContext {
    explicit MediaCapsAsyncContext(napi_env env) : MediaAsyncContext(env) {}
    ~MediaCapsAsyncContext() = default;

    MediaCapsNapi *napi = nullptr;
    Format format;
    int32_t sourceId = 0;
    int32_t qualityLevel = 0;
    AudioRecorderProfile profile;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_CAPABILITY_NAPI_H
