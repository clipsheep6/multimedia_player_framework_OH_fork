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

#ifndef VIDEO_DECODER_CALLBACK_NAPI_H_
#define VIDEO_DECODER_CALLBACK_NAPI_H_

#include "video_decoder_napi.h"
#include "videodecoder.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "common_napi.h"

namespace OHOS {
namespace Media {
const std::string CONFIGURE_CALLBACK_NAME = "configure";
const std::string START_CALLBACK_NAME = "start";
const std::string FLUSH_CALLBACK_NAME = "flush";
const std::string STOP_CALLBACK_NAME = "stop";
const std::string RESET_CALLBACK_NAME = "reset";
const std::string ERROR_CALLBACK_NAME = "error";
const std::string NEED_DATA_CALLBACK_NAME = "needData";
const std::string OUTPUT_AVAILABLE_CALLBACK_NAME = "outputAvailable";

class VideoDecoderCallbackNapi : public VideoDecoderCallback {
public:
    explicit VideoDecoderCallbackNapi(napi_env env);
    virtual ~VideoDecoderCallbackNapi();

    void SaveCallbackReference(const std::string &callbackName, napi_value callback);
    void SendErrorCallback(napi_env env, MediaServiceExtErrCode errCode);
    void SendCallback(napi_env env, const std::string &callbackName);

protected:
    void OnError(int32_t errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(std::vector<Format> format) override;
    void OnOutputBufferAvailable(uint32_t index, VideoDecoderBufferInfo info) override;
    void OnInputBufferAvailable(uint32_t index) override;

private:
    napi_env env_ = nullptr;
    std::mutex mutex_;
    std::shared_ptr<AutoRef> errorCallback_ = nullptr;
    std::shared_ptr<AutoRef> needDataCallback_ = nullptr;
    std::shared_ptr<AutoRef> outputAvailableCallback_ = nullptr;
    std::shared_ptr<AutoRef> configureCallback_ = nullptr;
    std::shared_ptr<AutoRef> startCallback_ = nullptr;
    std::shared_ptr<AutoRef> flushCallback_ = nullptr;
    std::shared_ptr<AutoRef> stopCallback_ = nullptr;
    std::shared_ptr<AutoRef> resetCallback_ = nullptr;
};
}  // namespace Media
}  // namespace OHOS
#endif // VIDEO_DECODER_CALLBACK_NAPI_H_
