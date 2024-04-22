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

#ifndef AV_META_DATA_EXTRACTOR_NAPI_H
#define AV_META_DATA_EXTRACTOR_NAPI_H

#include "media_errors.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "helper_data_source_callback.h"
#include "common_napi.h"
#include "audio_info.h"
#include "audio_effect.h"
#include "task_queue.h"
#include "avmetadatahelper_callback.h"

namespace OHOS {
namespace Media {
struct AVMetadataExtractorAsyncContext;
using TaskRet = std::pair<int32_t, std::string>;
class AVMetadataExtractorNapi : public AVMetadataHelperNotify {
public:
    __attribute__((visibility("default"))) static napi_value Init(napi_env env, napi_value exports);

private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    static napi_value JsCreateAVMetadataExtractor(napi_env env, napi_callback_info info);
    static napi_value JsResolveMetadata(napi_env env, napi_callback_info info);
    static napi_value JsFetchArtPicture(napi_env env, napi_callback_info info);
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
    /**
     * dataSrc: DataSrcDescriptor
     */
    static napi_value JsSetDataSrc(napi_env env, napi_callback_info info);
    static napi_value JsGetDataSrc(napi_env env, napi_callback_info info);

    static AVMetadataExtractorNapi* GetJsInstance(napi_env env, napi_callback_info info);
    static AVMetadataExtractorNapi* GetJsInstanceWithParameter(napi_env env, napi_callback_info info,
        size_t &argc, napi_value *argv);
    static void FetchArtPictureComplete(napi_env env, napi_status status, void *data);
    static void CommonCallbackRoutine(napi_env env, AVMetadataExtractorAsyncContext* &asyncContext,
                                      const napi_value &valueParam);
    static void ResolveMetadataComplete(napi_env env, napi_status status, void *data);

    AVMetadataExtractorNapi();
    ~AVMetadataExtractorNapi() override;
    void SaveCallbackReference(const std::string &callbackName, std::shared_ptr<AutoRef> ref);
    void ClearCallbackReference();
    void ClearCallbackReference(const std::string &callbackName);
    void StartListenCurrentResource();
    void PauseListenCurrentResource();
    void OnErrorCb(MediaServiceExtErrCodeAPI9 errorCode, const std::string &errorMsg);
    void SetSource(std::string url);
    void ResetUserParameters();

    std::shared_ptr<TaskHandler<TaskRet>> ResolveMetadataTask(
        std::unique_ptr<AVMetadataExtractorAsyncContext> &promiseCtx);
    std::shared_ptr<TaskHandler<TaskRet>> FetchArtPictureTask(
        std::unique_ptr<AVMetadataExtractorAsyncContext> &promiseCtx);
    std::shared_ptr<TaskHandler<TaskRet>> ReleaseTask();
    void SetAVFileDescriptorTask(std::shared_ptr<AVMetadataHelper>& avHelper, AVFileDescriptor& fileDescriptor);
    void SetDataSrcTask(std::shared_ptr<AVMetadataHelper>& avHelper,
        std::shared_ptr<HelperDataSourceCallback>& dataSrcCb);

    std::string GetCurrentState();
    void NotifyState(HelperStates state) override;

    void StopTaskQue();
    void WaitTaskQueStop();

    std::condition_variable stopTaskQueCond_;
    bool taskQueStoped_ = false;

    static thread_local napi_ref constructor_;
    napi_env env_ = nullptr;
    std::shared_ptr<AVMetadataHelper> helper_ = nullptr;
    std::shared_ptr<AVMetadataHelperCallback> extractorCb_ = nullptr;
    std::shared_ptr<HelperDataSourceCallback> dataSrcCb_ = nullptr;
    std::atomic<bool> isReleased_ = false;
    std::string url_ = "";
    struct AVFileDescriptor fileDescriptor_;
    struct AVDataSrcDescriptor dataSrcDescriptor_;
    std::unique_ptr<TaskQueue> taskQue_;
    std::mutex mutex_;
    std::mutex taskMutex_;
    std::map<std::string, std::shared_ptr<AutoRef>> refMap_;
    HelperStates state_ = HELPER_IDLE;
    std::condition_variable stateChangeCond_;
    std::atomic<bool> stopWait_;
    PixelMapParams param_;
};

struct AVMetadataExtractorAsyncContext : public MediaAsyncContext {
    explicit AVMetadataExtractorAsyncContext(napi_env env) : MediaAsyncContext(env) {}
    ~AVMetadataExtractorAsyncContext() = default;

    AVMetadataExtractorNapi *napi = nullptr;
    std::string opt_ = "";
    std::shared_ptr<TaskHandler<TaskRet>> task_ = nullptr;
    std::shared_ptr<Meta> metadata_ = nullptr;
    std::shared_ptr<PixelMap> artPicture_ = nullptr;
    int32_t status;
};
} // namespace Media
} // namespace OHOS
#endif // AV_META_DATA_EXTRACTOR_NAPI_H