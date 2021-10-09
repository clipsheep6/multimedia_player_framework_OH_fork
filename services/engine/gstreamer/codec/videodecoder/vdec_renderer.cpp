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

#include "vdec_renderer.h"
#include "display_type.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VdecRenderer"};
    constexpr uint32_t DEFAULT_WIDTH = 480;
    constexpr uint32_t DEFAULT_HEIGHT = 640;
}

namespace OHOS {
namespace Media {
VdecRenderer::VdecRenderer()
{
    MEDIA_LOGD("enter ctor");
}

VdecRenderer::~VdecRenderer()
{
    MEDIA_LOGD("enter dtor");
}

int32_t VdecRenderer::SetSurface(sptr<Surface> surface)
{
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, MSERR_INVALID_VAL, "Surface is nullptr");
    surface_ = surface;
    width_ = DEFAULT_WIDTH;
    height_ = DEFAULT_HEIGHT;
    return MSERR_OK;
}

int32_t VdecRenderer::Render(AVFrame *frame)
{
    CHECK_AND_RETURN_RET_LOG(surface_ != nullptr, MSERR_INVALID_VAL, "Surface is nullptr");
    CHECK_AND_RETURN_RET_LOG(frame != nullptr, MSERR_INVALID_VAL, "Buffer is nullptr");

    BufferRequestConfig requestConfig = {};
    requestConfig.width = width_;
    requestConfig.height = height_;
    requestConfig.strideAlignment = 8;
    requestConfig.format = PIXEL_FMT_RGBA_8888;
    requestConfig.usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA;
    requestConfig.timeout = 0;

    int32_t releaseFence = 0;
    sptr<SurfaceBuffer> surfaceBuffer;
    SurfaceError ret = surface_->RequestBuffer(surfaceBuffer, releaseFence, requestConfig);
    CHECK_AND_RETURN_RET_LOG(ret == SURFACE_ERROR_OK, MSERR_INVALID_OPERATION, "Failed to requestBuffer");
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, MSERR_INVALID_OPERATION, "SurfaceBuffer is nullptr");
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer->GetVirAddr() != nullptr, MSERR_INVALID_OPERATION, "Invalid buffer");

    TransPixelFormat(frame, width_, height_, (uint8_t *)surfaceBuffer->GetVirAddr());
    BufferFlushConfig flushConfig = {};
    flushConfig.damage.x = 0;
    flushConfig.damage.y = 0;
    flushConfig.damage.w = width_;
    flushConfig.damage.h = height_;
    ret = surface_->FlushBuffer(surfaceBuffer, -1, flushConfig);
    CHECK_AND_RETURN_RET_LOG(ret == SURFACE_ERROR_OK, MSERR_INVALID_OPERATION, "Failed to flush");
    return MSERR_OK;
}

void VdecRenderer::TransPixelFormat(AVFrame *frame, uint32_t width, uint32_t height, uint8_t *dest) {
    uint8_t *avY = frame->data[0];
    uint8_t *avU = frame->data[1];
    uint8_t *avV = frame->data[2];
    int32_t r = 0;
    int32_t g = 0;
    int32_t b = 0;
    for (int32_t j = 0; j < height; j++) {
        for (int32_t i = 0; i < width; i++) {
            int32_t y = avY[(j * width) + i];
            int32_t u = avU[((j / 2) * (width / 2)) + (i / 2)];
            int32_t v = avV[((j / 2) * (width / 2)) + (i / 2)];

            r = y + 1.402f * (v - 128);
            g = y - 0.344f * (u - 128) - 0.714f * (v - 128);
            b = y + 1.772f * (u - 128);

            *dest++ = Clamp(r);
            *dest++ = Clamp(g);
            *dest++ = Clamp(b);
            *dest++ = 0xff;
        }
    }
}

uint8_t VdecRenderer::Clamp(int16_t value)
{
    return value < 0 ? 0 : (value > 255 ? 255 : value);
}
}
}
