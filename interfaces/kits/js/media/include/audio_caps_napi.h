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

#ifndef AUDIO_CAPS_NAPI_H
#define AUDIO_CAPS_NAPI_H

#include "media_errors.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Media {
class AudioCapsNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);

private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    static napi_value GetCodecInfo(napi_env env, napi_callback_info info);
    static napi_value GetSupportedBitrate(napi_env env, napi_callback_info info);
    static napi_value GetSupportedChannel(napi_env env, napi_callback_info info);
    static napi_value GetSupportedFormats(napi_env env, napi_callback_info info);
    static napi_value GetSupportedSampleRates(napi_env env, napi_callback_info info);
    static napi_value GetSupportedProfiles(napi_env env, napi_callback_info info);
    static napi_value GetSupportedLevels(napi_env env, napi_callback_info info);
    static napi_value GetSupportedComplexity(napi_env env, napi_callback_info info);

    AudioCapsNapi();
    ~AudioCapsNapi();

    static napi_ref constructor_;
    napi_env env_ = nullptr;
    napi_ref wrap_ = nullptr;
};
}  // namespace Media
}  // namespace OHOS
#endif // AUDIO_CAPS_NAPI_H
