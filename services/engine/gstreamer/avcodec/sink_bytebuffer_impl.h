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

#ifndef SINK_BYTEBUFFER_IMPL_H
#define SINK_BYTEBUFFER_IMPL_H

#include "sink_base.h"
#include <mutex>
#include <vector>
#include "avsharedmemory.h"
#include "codec_common.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class SinkBytebufferImpl : public SinkBase {
public:
    SinkBytebufferImpl();
    virtual ~SinkBytebufferImpl();
    DISALLOW_COPY_AND_MOVE(SinkBytebufferImpl);

    int32_t AllocateBuffer() override;
    int32_t Init() override;
    int32_t Configure(std::shared_ptr<ProcessorConfig> config) override;
    int32_t Flush() override;
    std::shared_ptr<AVSharedMemory> GetOutputBuffer(uint32_t index) override;
    int32_t ReleaseOutputBuffer(uint32_t index, bool render = false) override;
    int32_t SetParameter(const Format &format) override;
    int32_t SetCallback(const std::weak_ptr<IAVCodecEngineObs> &obs) override;

private:
    static void OutputAvailableCb(const GstElement *sink, uint32_t size, gpointer self);

    std::vector<std::shared_ptr<BufferWrapper>> bufferList_;
    std::vector<std::shared_ptr<AVSharedMemory>> shareMemList_;
    uint32_t bufferCount_ = 0;
    std::mutex mutex_;
    gulong signalId_ = 0;
    std::weak_ptr<IAVCodecEngineObs> obs_;
};
} // Media
} // OHOS
#endif // SINK_BYTEBUFFER_IMPL_H