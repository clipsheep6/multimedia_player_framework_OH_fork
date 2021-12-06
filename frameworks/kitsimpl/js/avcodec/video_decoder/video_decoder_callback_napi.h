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

#ifndef VIDEO_DECODER_CALLBACK_NAPI_H
#define VIDEO_DECODER_CALLBACK_NAPI_H

#include "video_decoder_napi.h"
#include "avcodec_video_decoder.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Media {
class VideoDecoderCallbackNapi : public AVCodecCallback {
public:
    explicit VideoDecoderCallbackNapi(napi_env env);
    virtual ~VideoDecoderCallbackNapi();

protected:
    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;

private:
    napi_env env_ = nullptr;
};
}  // namespace Media
}  // namespace OHOS
#endif // VIDEO_DECODER_CALLBACK_NAPI_H
