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

#ifndef AUDIO_DECODER_CALLBACK_NAPI_H
#define AUDIO_DECODER_CALLBACK_NAPI_H

#include "audio_decoder_napi.h"
#include "avcodec_audio_decoder.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "common_napi.h"

namespace OHOS {
namespace Media {
const std::string CONFIGURE_CALLBACK_NAME = "configure";
const std::string PREPARE_CALLBACK_NAME = "prepare";
const std::string START_CALLBACK_NAME = "start";
const std::string STOP_CALLBACK_NAME = "stop";
const std::string FLUSH_CALLBACK_NAME = "flush";
const std::string RESET_CALLBACK_NAME = "reset";
const std::string ERROR_CALLBACK_NAME = "error";
const std::string INPUT_CALLBACK_NAME = "inputBufferAvailable";
const std::string OUTPUT_CALLBACK_NAME = "outputBufferAvailable";

class AudioDecoderCallbackNapi : public AVCodecCallback {
public:
    explicit AudioDecoderCallbackNapi(napi_env env);
    virtual ~AudioDecoderCallbackNapi();

    void SaveCallbackReference(const std::string &callbackName, napi_value callback);
    void SendErrorCallback(MediaServiceExtErrCode errCode);
    void SendStateCallback(const std::string &callbackName);

protected:
    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info,
                                 AVCodecBufferType type, AVCodecBufferFlag flag) override;

private:
    struct AudioDecoderJsCallback {
        std::shared_ptr<AutoRef> callback = nullptr;
        std::string callbackName = "unknown";
        std::string errorMsg = "unknown";
        MediaServiceExtErrCode errorCode = MSERR_EXT_UNKNOWN;
    };
    void OnJsErrorCallBack(AudioDecoderJsCallback *jsCb) const;
    void OnJsStateCallBack(AudioDecoderJsCallback *jsCb) const;
    std::shared_ptr<AutoRef> StateCallbackSelect(const std::string &callbackName) const;
    napi_env env_ = nullptr;
    std::mutex mutex_;

    std::shared_ptr<AutoRef> configureCallback_ = nullptr;
    std::shared_ptr<AutoRef> prepareCallback_ = nullptr;
    std::shared_ptr<AutoRef> startCallback_ = nullptr;
    std::shared_ptr<AutoRef> stopCallback_ = nullptr;
    std::shared_ptr<AutoRef> flushCallback_ = nullptr;
    std::shared_ptr<AutoRef> resetCallback_ = nullptr;
    std::shared_ptr<AutoRef> errorCallback_ = nullptr;
    std::shared_ptr<AutoRef> inputCallback_ = nullptr;
    std::shared_ptr<AutoRef> outputCallback_ = nullptr;
};
}  // namespace Media
}  // namespace OHOS
#endif // AUDIO_DECODER_CALLBACK_NAPI_H
