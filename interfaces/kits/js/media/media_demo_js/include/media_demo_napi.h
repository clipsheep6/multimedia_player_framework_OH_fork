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

#ifndef MEDIA_TEST_NAPI_H_
#define MEDIA_TEST_NAPI_H_

#include <atomic>
#include <thread>
#include <string>

#include "media_errors.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "surface.h"
#include "window_manager.h"
#include "nocopyable.h"
#include "common_napi.h"

namespace OHOS {
namespace Media {
class MediaDemoNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);
    static std::string GetStringArgument(napi_env env, napi_value value);
    void BufferLoop();
    static bool StrToUint64(const std::string &str, uint64_t &value);
private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    static napi_value CreateMediaTest(napi_env env, napi_callback_info info);
    static napi_value StartStream(napi_env env, napi_callback_info info);
    static napi_value CloseStream(napi_env env, napi_callback_info info);
    static napi_value SetResolution(napi_env env, napi_callback_info info);
    static napi_value SetFrameCount(napi_env env, napi_callback_info info);
    static napi_value SetFrameRate(napi_env env, napi_callback_info info);
   
    MediaDemoNapi();
    ~MediaDemoNapi();

    static napi_ref constructor_;
    napi_env env_ = nullptr;
    napi_ref wrapper_ = nullptr;
    int64_t pts_ = 0;

    int32_t count_ = 0;
    unsigned char color_ = 0;
    int32_t totalFrameCount_ = 300;
    int32_t frameRate_ = 30;
    int32_t width_ = 256;
    int32_t height_ = 256;
    OHOS::sptr<OHOS::Surface> producerSurface_ = nullptr;
    std::atomic<bool> isStart_ = false;
    std::unique_ptr<std::thread> cameraHDIThread_;
};

}  // namespace Media
}  // namespace OHOS
#endif // MEDIA_TEST_NAPI_H_
