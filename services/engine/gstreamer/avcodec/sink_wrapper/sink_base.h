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

#ifndef SINK_BASE_H
#define SINK_BASE_H

#include <cstdint>
#include "avcodec_common.h"
#include "avsharedmemory.h"
#include "format.h"
#include <gst/gst.h>
#include "media_errors.h"
#include "surface.h"

namespace OHOS {
namespace Media {
class SinkCallback {
public:
    virtual ~SinkCallback() = default;
    virtual void OnOutputBufferAvailable(uint32_t index) = 0;
};

class SinkBase {
public:
    virtual ~SinkBase() = default;

    virtual int32_t SetOutputSurface(sptr<Surface> surface)
    {
        return MSERR_INVALID_OPERATION;
    }

    virtual std::shared_ptr<AVSharedMemory> GetOutputBuffer(uint32_t index)
    {
        return nullptr;
    }

    virtual int32_t ReleaseOutputBuffer(uint32_t index, bool render) = 0;
    virtual int32_t SetParameter(const Format &format) = 0;
    virtual int32_t SetCallback(const std::shared_ptr<SinkCallback> &callback) = 0;
    virtual int32_t SetCaps(GstCaps *caps) = 0;
};
} // Media
} // OHOS
#endif // SINK_BASE_H