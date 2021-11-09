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
#ifndef AVCODEC_COMMOM_H
#define AVCODEC_COMMOM_H

#include <cstdint>
#include <string>
#include "format.h"

namespace OHOS {
namespace Media {
enum AVCodecErrorType : int32_t {
    AVCODEC_ERROR_INTERNAL,
    AVCODEC_ERROR_EXTEND_START = 0X10000,
};

enum AVCodecType : int32_t {
    AVCODEC_TYPE_VIDEO_ENCODER = 0,
    AVCODEC_TYPE_VIDEO_DECODER,
    AVCODEC_TYPE_AUDIO_ENCODER,
    AVCODEC_TYPE_AUDIO_DECODER
};

enum AVCodecBufferFlag : int32_t {
    AVCODEC_BUFFER_FLAG_NONE = 0,
    AVCODEC_BUFFER_FLAG_EOS = 1 << 0,
    AVCODEC_BUFFER_FLAG_SYNC_FRAME = 1 << 1,
    AVCODEC_BUFFER_FLAG_PARTIAL_FRAME = 1 << 2,
    AVCODEC_BUFFER_FLAG_CODEDC_DATA = 1 << 3,
};

struct AVCodecBufferInfo {
    int64_t presentationTimeUs = 0;
    int32_t size = 0;
    int32_t offset = 0;
};

class AVCodecCallback {
public:
    virtual ~AVCodecCallback() = default;
    virtual void OnError(AVCodecErrorType errorType, int32_t errorCode) = 0;
    virtual void OnOutputFormatChanged(const Format &format) = 0;
    virtual void OnInputBufferAvailable(uint32_t index) = 0;
    virtual void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) = 0;
};

__attribute__((visibility("default"))) std::string AVCodecErrorTypeToString(AVCodecErrorType type);
} // Media
} // OHOS
#endif // AVCODEC_COMMOM_H
