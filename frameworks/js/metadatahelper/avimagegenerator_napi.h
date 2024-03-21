/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef AV_IMAGE_GENERATOR_NAPI_H
#define AV_IMAGE_GENERATOR_NAPI_H

#include "media_errors.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "common_napi.h"
#include "audio_info.h"
#include "audio_effect.h"
#include "task_queue.h"
#include "avmetadatahelper_callback.h"

namespace OHOS {
namespace Media {
struct AVImageGeneratorAsyncContext;
using TaskRet = std::pair<int32_t, std::string>;
class AVImageGeneratorNapi : public AVMetadataHelperNotify {
public:
    __attribute__((visibility("default"))) static napi_value Init(napi_env env, napi_value exports);

private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    static napi_value JsCreateAVImageGenerator(napi_env env, napi_callback_info info);
    static napi_value JsFetchFrameAtTime(napi_env env, napi_callback_info info);
    static napi_value JsRelease(napi_env env, napi_callback_info info);
    /**
     * url: string
     */
    static napi_value JsSetUrl(napi_env env, napi_callback_info info);
    static napi_value JsGetUrl(napi_env env, napi_callback_info info);
    /**
     * fdSrc: AVFileDescriptor
     */
    static napi_value JsGetAVFileDescriptor(napi_env env, napi_callback_info info);
    static napi_value JsSetAVFileDescriptor(napi_env env, napi_callback_info info);

    static AVImageGeneratorNapi* GetJsInstance(napi_env env, napi_callback_info info);
    static AVImageGeneratorNapi* GetJsInstanceWithParameter(napi_env env, napi_callback_info info,
        size_t &argc, napi_value *argv);
    static void CreatePixelMapComplete(napi_env env, napi_status status, void *data);
    static void CommonCallbackRoutine(napi_env env, AVImageGeneratorAsyncContext* &asyncContext,
                                      const napi_value &valueParam);

    static bool CheckSystemApp(napi_env env);

    AVImageGeneratorNapi();
    ~AVImageGeneratorNapi() override;
    void SaveCallbackReference(const std::string &callbackName, std::shared_ptr<AutoRef> ref);
    void ClearCallbackReference();
    void ClearCallbackReference(const std::string &callbackName);
    void StartListenCurrentResource();
    void PauseListenCurrentResource();
    void OnErrorCb(MediaServiceExtErrCodeAPI9 errorCode, const std::string &errorMsg);
    void SetSource(std::string url);
    void ResetUserParameters();

    std::shared_ptr<TaskHandler<TaskRet>> FetchFrameAtTimeTask(
        std::unique_ptr<AVImageGeneratorAsyncContext> &promiseCtx);
    int32_t GetFetchFrameArgs(std::unique_ptr<AVImageGeneratorAsyncContext> &asyncCtx,
        napi_env env, napi_value timeUs, napi_value option, napi_value format);
    std::shared_ptr<TaskHandler<TaskRet>> ReleaseTask();
    void SetAVFileDescriptorTask(std::shared_ptr<AVMetadataHelper>& avHelper, AVFileDescriptor& fileDescriptor);

    std::string GetCurrentState();
    void NotifyState(HelperStates state) override;

    void StopTaskQue();
    void WaitTaskQueStop();

    std::condition_variable stopTaskQueCond_;
    bool taskQueStoped_ = false;

    static thread_local napi_ref constructor_;
    napi_env env_ = nullptr;
    std::shared_ptr<AVMetadataHelper> helper_ = nullptr;
    std::shared_ptr<AVMetadataHelperCallback> generatorCb_ = nullptr;
    std::atomic<bool> isReleased_ = false;
    std::string url_ = "";
    struct AVFileDescriptor fileDescriptor_;
    std::unique_ptr<TaskQueue> taskQue_;
    std::mutex mutex_;
    std::mutex taskMutex_;
    std::map<std::string, std::shared_ptr<AutoRef>> refMap_;
    HelperStates state_ = HELPER_IDLE;
    std::condition_variable stateChangeCond_;
    std::atomic<bool> stopWait_;
    int64_t timeUs_ = 0;
    int32_t option_ = 0;
    PixelMapParams param_;
};

struct AVImageGeneratorAsyncContext : public MediaAsyncContext {
    explicit AVImageGeneratorAsyncContext(napi_env env) : MediaAsyncContext(env) {}
    ~AVImageGeneratorAsyncContext() = default;

    AVImageGeneratorNapi *napi = nullptr;
    std::string opt_ = "";
    std::shared_ptr<TaskHandler<TaskRet>> task_ = nullptr;
    std::shared_ptr<PixelMap> pixel_ = nullptr;
    int32_t status;
};
} // namespace Media
} // namespace OHOS
#endif // AV_IMAGE_GENERATOR_NAPI_H