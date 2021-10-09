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
#ifndef VIDEODECODER_FF_IMPL_H
#define VIDEODECODER_FF_IMPL_H

#include <mutex>
#include "videodecoder.h"
#include "nocopyable.h"
#include "i_videodecoder_engine.h"
#include "vdec_controller.h"

namespace OHOS {
namespace Media {
class VdecEngineFFImpl : public IVideoDecoderEngine {
public:
    VdecEngineFFImpl();
    ~VdecEngineFFImpl();
    DISALLOW_COPY_AND_MOVE(VdecEngineFFImpl);

    int32_t Init() override;
    int32_t Configure(const std::vector<Format> &format, sptr<Surface> surface) override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Flush() override;
    int32_t Reset() override;
    std::shared_ptr<AVSharedMemory> GetInputBuffer(uint32_t index) override;
    int32_t PushInputBuffer(uint32_t index, uint32_t offset, uint32_t size,
        int64_t presentationTimeUs, VideoDecoderInputBufferFlag flags) override;
    int32_t ReleaseOutputBuffer(uint32_t index, bool render) override;
    int32_t SetObs(const std::weak_ptr<IVideoDecoderEngineObs> &obs) override;

private:
    std::mutex mutex_;
    std::shared_ptr<VdecController> vdecCtrl_;
};
} // Media
} // OHOS
#endif // VIDEODECODER_FF_IMPL_H
