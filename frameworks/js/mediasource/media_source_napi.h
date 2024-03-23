/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef MEDIA_SOURCE_NAPI_H
#define MEDIA_SOURCE_NAPI_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "common_napi.h"

namespace OHOS {
namespace Media {

class MediaSourceNapi {
public:
    __attribute__((visibility("default"))) static napi_value Init(napi_env env, napi_value exports);
    static std::shared_ptr<AVMediaSourceTmp> GetMediaSource(napi_env env, napi_value jsMediaSource);
private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    /**
     * function createMediaSourceWithUrl(url: string, header?: string): MediaSource
     */
    static napi_value JsCreateMediaSourceWithUrl(napi_env env, napi_callback_info info);
 
    MediaSourceNapi() = default;
    virtual ~MediaSourceNapi() = default;

    static thread_local napi_ref constructor_;
    napi_env env_ = nullptr;
    std::shared_ptr<AVMediaSourceTmp> mediaSource_ {nullptr};
};

} // Media
} // OHOS

#endif // MEDIA_SOURCE_NAPI_H