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

#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <cstdint>
#include <vector>
#include "surface.h"
#include "format.h"
#include "avsharedmemory.h"

namespace OHOS {
namespace Media {
enum VideoDecoderInputBufferFlag : int32_t {
    BUFFER_FLAG_NONE = 0,
    BUFFER_FLAG_CODEC,
    BUFFER_FLAG_END_OF_STREAM,
    BUFFER_FLAG_KEY_FRAME,
};

struct VideoDecoderBufferInfo {
    int64_t presentationTimeUs = 0;
    int32_t size = 0;
    int32_t offset = 0;
};

class VideoDecoderCallback {
public:
    virtual ~VideoDecoderCallback() = default;
    virtual void OnError(int32_t errorType, int32_t errorCode) = 0;
    virtual void OnOutputFormatChanged(std::vector<Format> format) = 0;
    virtual void OnOutputBufferAvailable(uint32_t index, VideoDecoderBufferInfo info) = 0;
    virtual void OnInputBufferAvailable(uint32_t index) = 0;
};

class VideoDecoder {
public:
    virtual ~VideoDecoder() = default;
    virtual int32_t Configure(const std::vector<Format> &format, sptr<Surface> surface) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Flush() = 0;
    virtual int32_t Reset() = 0;
    virtual int32_t Release() = 0;
    virtual std::shared_ptr<AVSharedMemory> GetInputBuffer(uint32_t index) = 0;
    virtual int32_t PushInputBuffer(uint32_t index, uint32_t offset, uint32_t size, int64_t presentationTimeUs,
                                    VideoDecoderInputBufferFlag flags) = 0;
    virtual int32_t ReleaseOutputBuffer(uint32_t index, bool render) = 0;
    virtual int32_t SetCallback(const std::shared_ptr<VideoDecoderCallback> &callback) = 0;
};

class __attribute__((visibility("default"))) VideoDecoderFactory {
public:
    static std::shared_ptr<VideoDecoder> CreateVideoDecoder(const std::string &name);
private:
    VideoDecoderFactory() = default;
    ~VideoDecoderFactory() = default;
};
} // Media
} // OHOS
#endif // VIDEODECODER_H
