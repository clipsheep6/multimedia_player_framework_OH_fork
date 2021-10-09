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

#ifndef I_VIDEODECODER_SERVICE_H
#define I_VIDEODECODER_SERVICE_H

#include <string>
#include "videodecoder.h"
#include "refbase.h"
#include "surface.h"

namespace OHOS {
namespace Media {
class IVideoDecoderService {
public:
    virtual ~IVideoDecoderService() = default;

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
} // namespace Media
} // namespace OHOS
#endif // I_VIDEODECODER_SERVICE_H
