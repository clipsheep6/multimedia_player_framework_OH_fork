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

#ifndef SRC_BYTEBUFFER_IMPL_H
#define SRC_BYTEBUFFER_IMPL_H

#include "src_base.h"
#include "codec_common.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class SrcBytebufferImpl : public SrcBase {
public:
    SrcBytebufferImpl();
    virtual ~SrcBytebufferImpl();
    DISALLOW_COPY_AND_MOVE(SrcBytebufferImpl);

    int32_t AllocateBuffer() override;
    int32_t Init() override;
    int32_t Configure(std::shared_ptr<ProcessorConfig> config) override;
    int32_t Flush() override;
    int32_t GetBufferCount() override;
    std::shared_ptr<AVSharedMemory> GetInputBuffer(uint32_t index) override;
    int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;
    int32_t SetParameter(const Format &format) override;
    int32_t SetCallback(const std::weak_ptr<IAVCodecEngineObs> &obs) override;

private:
    static void InputAvailableCb(const GstElement *sink, uint32_t size, gpointer self);

    std::vector<std::shared_ptr<BufferWrapper>> bufferList_;
    std::vector<std::shared_ptr<AVSharedMemory>> shareMemList_;
    uint32_t bufferCount_ = 0;
    std::mutex mutex_;
    gulong signalId_ = 0;
    std::weak_ptr<IAVCodecEngineObs> obs_;
};
} // Media
} // OHOS
#endif // SRC_BYTEBUFFER_IMPL_H