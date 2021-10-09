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

#ifndef VDEC_CONTROLLER_H
#define VDEC_CONTROLLER_H

#include <memory>
#include "nocopyable.h"
#include "i_videodecoder_engine.h"
#include "vdec_h264.h"
#include "vdec_renderer.h"
#include "videodecoder.h"
#include "task_queue.h"

namespace OHOS {
namespace Media {
class VdecController {
public:
    VdecController();
    ~VdecController();

    int32_t Init();
    int32_t Configure(sptr<Surface> surface);
    int32_t Start();
    int32_t Stop();
    int32_t SetSurface(sptr<Surface> surface);
    std::shared_ptr<AVSharedMemory> GetInputBuffer(uint32_t index);
    int32_t PushInputBuffer(uint32_t index, uint32_t offset, uint32_t size,
        int64_t presentationTimeUs, VideoDecoderInputBufferFlag flags);
    int32_t ReleaseOutputBuffer(uint32_t index, bool render);
    void SetObs(const std::weak_ptr<IVideoDecoderEngineObs> &obs);

    DISALLOW_COPY_AND_MOVE(VdecController);

private:
    void OnOutputBufferAvailableCallback();
    void OnInputBufferAvailableCallback();

    std::shared_ptr<VdecH264> vdec_;
    std::shared_ptr<VdecRenderer> vdecRender_;
    std::unique_ptr<TaskQueue> taskQueue_;
    std::weak_ptr<IVideoDecoderEngineObs> obs_;
    std::shared_ptr<AVSharedMemory> memInput_;
    std::shared_ptr<AVSharedMemory> memOutput_;
    AVFrame *frame_;
    bool dump_ = false;
};
}
}
#endif // VDEC_CONTROLLER_H
