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
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Media {
struct MediaDescAsyncContext;

class MediaDescNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);

private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    static napi_value GetMediaCapability(napi_env env, napi_callback_info info);
    static napi_value GetAudioDecoderCaps(napi_env env, napi_callback_info info);
    static napi_value FindAudioDecoder(napi_env env, napi_callback_info info);
    static napi_value GetAudioEncoderCaps(napi_env env, napi_callback_info info);
    static napi_value FindAudioEncoder(napi_env env, napi_callback_info info);

    static void AsyncCallback(napi_env env, MediaDescAsyncContext *asyncCtx);
    static void CompleteAsyncFunc(napi_env env, napi_status status, void *data);
    static void AsyncCreator(napi_env env, void *data);

    MediaDescNapi();
    ~MediaDescNapi();

    static napi_ref constructor_;
    napi_env env_ = nullptr;
    napi_ref wrap_ = nullptr;
};

struct MediaDescAsyncContext {
    void SignError(int32_t code, std::string message) {
        success = false;
        errCode = code;
        errMessage = message;
    }
    // general variable
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    MediaDescNapi *napi = nullptr;
    napi_value asyncRet;
    bool success = false;
    int32_t errCode = 0;
    std::string errMessage = "";
};
}  // namespace Media
}  // namespace OHOS
#endif // MEDIA_CAPABILITY_NAPI_H
