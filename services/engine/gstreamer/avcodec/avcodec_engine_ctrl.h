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
#include "nocopyable.h"
#include "src_wrapper/src_base.h"
#include "sink_wrapper/sink_base.h"

namespace OHOS {
namespace Media {
class AVCodecEngineCtrl {
public:
    AVCodecEngineCtrl();
    ~AVCodecEngineCtrl();

    int32_t Init(AVCodecType type, bool useSoftware);
    int32_t Prepare();
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Release();

    DISALLOW_COPY_AND_MOVE(AVCodecEngineCtrl);

private:
    GstPipeline *gstPipeline_ = nullptr;
    GstElement *codecBin_ = nullptr;
};
} // Media
} // OHOS
#endif // AVCODEC_ENGINE_CTRL_H
