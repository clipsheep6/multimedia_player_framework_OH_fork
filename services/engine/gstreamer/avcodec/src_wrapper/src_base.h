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

#ifndef SRC_BASE_H
#define SRC_BASE_H

#include <cstdint>
#include "avcodec_common.h"
#include "avsharedmemory.h"
#include "format.h"
#include <gst/gst.h>
#include "media_errors.h"
#include "surface.h"

namespace OHOS {
namespace Media {
class SrcCallback {
public:
    virtual ~SrcCallback() = default;
    virtual void OnInputBufferAvailable(uint32_t index) = 0;
};

class SrcBase {
public:
    virtual ~SrcBase() = default;

    virtual sptr<Surface> CreateInputSurface()
    {
        return nullptr;
    }

    virtual std::shared_ptr<AVSharedMemory> GetInputBuffer(uint32_t index)
    {
        return nullptr;
    }

    virtual int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info,
                                     AVCodecBufferType type, AVCodecBufferFlag flag)
    {
        return MSERR_INVALID_OPERATION;
    }

    virtual int32_t SetParameter(const Format &format) = 0;
    virtual int32_t SetCallback(const std::shared_ptr<SrcCallback> &callback) = 0;
    virtual int32_t SetCaps(GstCaps *caps) = 0;
};
} // Media
} // OHOS
#endif // SRC_BASE_H