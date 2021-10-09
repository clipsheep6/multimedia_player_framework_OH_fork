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
#ifndef I_VIDEODECODER_ENGINE_H
#define I_VIDEODECODER_ENGINE_H

#include <vector>
#include <cstdint>
#include <string>
#include <refbase.h>
#include "videodecoder.h"
#include "nocopyable.h"

namespace OHOS {
class Surface;

namespace Media {
class IVideoDecoderEngineObs : public std::enable_shared_from_this<IVideoDecoderEngineObs> {
public:
    virtual ~IVideoDecoderEngineObs() = default;
    virtual void OnError(int32_t errorType, int32_t errorCode) = 0;
    virtual void OnOutputFormatChanged(std::vector<Format> format) = 0;
    virtual void OnOutputBufferAvailable(uint32_t index, VideoDecoderBufferInfo info) = 0;
    virtual void OnInputBufferAvailable(uint32_t index) = 0;
};

class IVideoDecoderEngine {
public:
    virtual ~IVideoDecoderEngine() = default;

    virtual int32_t Init() = 0;
    virtual int32_t Configure(const std::vector<Format> &format, sptr<Surface> surface) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Flush() = 0;
    virtual int32_t Reset() = 0;
    virtual std::shared_ptr<AVSharedMemory> GetInputBuffer(uint32_t index) = 0;
    virtual int32_t PushInputBuffer(uint32_t index, uint32_t offset, uint32_t size, int64_t presentationTimeUs,
                                    VideoDecoderInputBufferFlag flags) = 0;
    virtual int32_t ReleaseOutputBuffer(uint32_t index, bool render) = 0;
    virtual int32_t SetObs(const std::weak_ptr<IVideoDecoderEngineObs> &obs) = 0;
};
} // Media
} // OHOS
#endif // I_VIDEODECODER_ENGINE_H
