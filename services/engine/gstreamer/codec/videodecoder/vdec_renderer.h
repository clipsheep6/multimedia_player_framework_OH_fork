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

#ifndef VDEC_RENDERER_H
#define VDEC_RENDERER_H

#include <memory>
#include "nocopyable.h"
#include "i_videodecoder_engine.h"
#include "vdec_h264.h"

namespace OHOS {
namespace Media {
class VdecRenderer {
public:
    VdecRenderer();
    ~VdecRenderer();

    int32_t SetSurface(sptr<Surface> surface);
    int32_t Render(AVFrame *frame);

    DISALLOW_COPY_AND_MOVE(VdecRenderer);

private:
    void TransPixelFormat(AVFrame *frame, uint32_t width, uint32_t height, uint8_t *out);
    uint8_t Clamp(int16_t value);
    sptr<Surface> surface_ = nullptr;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
};
}
}
#endif // VDEC_RENDERER_H
