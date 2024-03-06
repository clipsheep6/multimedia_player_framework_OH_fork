/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "avmetadata_frame_converter.h"

#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetadataFrameConverter"};
    
    constexpr int32_t NUM_2 = 2;
    constexpr int32_t NUM_3 = 3;
    constexpr int32_t NUM_4 = 4;
    constexpr int32_t NUM_5 = 5;
    constexpr int32_t NUM_11 = 11;
}

namespace OHOS {
namespace Media {
std::unique_ptr<PixelMap> AVMetadataFrameConverter::RGBxToRGB565(const std::unique_ptr<PixelMap> &srcPixelMap)
{
    MEDIA_LOGI("Convert thumb from RGBA_8888 to RGB_565");
    CHECK_AND_RETURN_RET_LOG(srcPixelMap != nullptr, nullptr, "srcPixelMap is nullptr.");

    int32_t height = srcPixelMap->GetHeight();
    int32_t width = srcPixelMap->GetWidth();
    InitializationOptions initOpts;
    initOpts.size = {width, height};
    initOpts.srcPixelFormat = PixelFormat::RGB_565;
    std::unique_ptr<PixelMap> dstPixelMap = PixelMap::Create(initOpts);

    CHECK_AND_RETURN_RET_LOG(dstPixelMap != nullptr, nullptr, "create pixel map failed.");
    const uint8_t *srcPtr = srcPixelMap->GetPixels();
    uint16_t *dstPtr = static_cast<uint16_t *>(dstPixelMap->GetWritablePixels());

    // rgba8888 to rgb565
    for (int32_t i = 0; i < height; i++) {
        const uint8_t *srcPixel = srcPtr + width * i * NUM_4;
        uint16_t *dstPixel = dstPtr + width * i;
        for (int32_t j = 0; j < width; j++) {
            uint16_t r = *srcPixel >> NUM_3;
            uint16_t g = *(++srcPixel) >> NUM_2;
            uint16_t b = *(++srcPixel) >> NUM_3;

            *dstPixel = static_cast<uint16_t>(r << NUM_11 | g << NUM_5 | b);
            srcPixel += NUM_2;
            dstPixel++;
        }
    }
    return dstPixelMap;
}

std::unique_ptr<PixelMap> AVMetadataFrameConverter::RGBxToRGB888(const std::unique_ptr<PixelMap> &srcPixelMap)
{
    MEDIA_LOGI("Convert thumb from RGBA_8888 to RGB_888");
    CHECK_AND_RETURN_RET_LOG(srcPixelMap != nullptr, nullptr, "srcPixelMap is nullptr.");

    int32_t height = srcPixelMap->GetHeight();
    int32_t width = srcPixelMap->GetWidth();
    InitializationOptions initOpts;
    initOpts.size = {width, height};
    initOpts.srcPixelFormat = PixelFormat::RGB_888;
    std::unique_ptr<PixelMap> dstPixelMap = PixelMap::Create(initOpts);

    CHECK_AND_RETURN_RET_LOG(dstPixelMap != nullptr, nullptr, "create pixel map failed.");
    const uint8_t *srcPtr = srcPixelMap->GetPixels();
    uint8_t *dstPtr = static_cast<uint8_t*>(dstPixelMap->GetWritablePixels());

    // rgba8888 to rgb888
    for (int32_t i = 0; i < height; i++) {
        for (int32_t j = 0; j < width; j++) {
            *(dstPtr++) = *(srcPtr++);
            *(dstPtr++) = *(srcPtr++);
            *(dstPtr++) = *(srcPtr++);
            srcPtr++;
        }
    }
    return dstPixelMap;
}
} // namespace Media
} // namespace OHOS