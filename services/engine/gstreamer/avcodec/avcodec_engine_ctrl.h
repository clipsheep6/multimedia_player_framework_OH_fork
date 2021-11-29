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

#ifndef AVCODEC_ENGINE_CTRL_H
#define AVCODEC_ENGINE_CTRL_H

#include <cstdint>
#include "i_avcodec_engine.h"
#include "nocopyable.h"
#include "sink_base.h"
#include "src_base.h"
#include "processor_base.h"
#include "task_queue.h"

namespace OHOS {
namespace Media {
class AVCodecEngineCtrl {
public:
    AVCodecEngineCtrl();
    ~AVCodecEngineCtrl();

    int32_t Init(AVCodecType type, bool useSoftware, const std::string &name);
    int32_t Prepare(std::shared_ptr<ProcessorConfig> inputConfig, std::shared_ptr<ProcessorConfig> outputConfig);
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    void SetObs(const std::weak_ptr<IAVCodecEngineObs> &obs);
    sptr<Surface> CreateInputSurface();
    int32_t SetOutputSurface(sptr<Surface> surface);
    std::shared_ptr<AVSharedMemory> GetInputBuffer(uint32_t index);
    int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag);
    std::shared_ptr<AVSharedMemory> GetOutputBuffer(uint32_t index);
    int32_t ReleaseOutputBuffer(uint32_t index, bool render);
    int32_t SetParameter(const Format &format);

    DISALLOW_COPY_AND_MOVE(AVCodecEngineCtrl);

private:
    AVCodecType codecType_ = AVCODEC_TYPE_VIDEO_ENCODER;
    GstPipeline *gstPipeline_ = nullptr;
    GstElement *codecBin_ = nullptr;
    bool useSurfaceRender_ = false;
    std::weak_ptr<IAVCodecEngineObs> obs_;
    std::unique_ptr<SrcBase> src_;
    std::unique_ptr<SinkBase> sink_;
};
} // Media
} // OHOS
#endif // AVCODEC_ENGINE_CTRL_H
