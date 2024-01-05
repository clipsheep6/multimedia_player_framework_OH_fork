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
#ifndef AV_COMMOM_H
#define AV_COMMOM_H

#include <vector>
#include <string>
#include "meta/format.h"
#include "media_core.h"

namespace OHOS {
namespace Media {
/**
 * @brief
 *
 * @since 3.1
 * @version 3.1
 */
enum VideoPixelFormat {
    /**
     * yuv 420 planar.
     */
    YUVI420 = 1,
    /**
     *  NV12. yuv 420 semiplanar.
     */
    NV12 = 2,
    /**
     *  NV21. yvu 420 semiplanar.
     */
    NV21 = 3,
    /**
     * format from surface.
     */
    SURFACE_FORMAT = 4,
    /**
     * RGBA.
     */
    RGBA = 5,
};

/**
 * @brief Enumerates the video rotation.
 *
 * @since 3.2
 * @version 3.2
 */
enum VideoRotation : uint32_t {
    /**
    * Video without rotation
    */
    VIDEO_ROTATION_0 = 0,
    /**
    * Video rotated 90 degrees
    */
    VIDEO_ROTATION_90 = 90,
    /**
    * Video rotated 180 degrees
    */
    VIDEO_ROTATION_180 = 180,
    /**
    * Video rotated 270 degrees
    */
    VIDEO_ROTATION_270 = 270,
};

} // namespace Media
} // namespace OHOS
#endif // AV_COMMOM_H