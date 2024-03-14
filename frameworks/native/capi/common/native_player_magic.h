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

#ifndef NATIVE_SCREEN_CAPTURE_MAGIC_H
#define NATIVE_SCREEN_CAPTURE_MAGIC_H

#include <refbase.h>
#include "screen_capture.h"
#include "player.h"

struct OH_AVScreenCapture : public OHOS::RefBase {
    OH_AVScreenCapture() = default;
    virtual ~OH_AVScreenCapture() = default;
};

struct OH_AVScreenCapture_ContentFilter : public OHOS::RefBase {
    OH_AVScreenCapture_ContentFilter() = default;
    virtual ~OH_AVScreenCapture_ContentFilter() = default;
};

struct OH_AVPlayer : public OHOS::RefBase {
    OH_AVPlayer() = default;
    virtual ~OH_AVPlayer() = default;
    OHOS::Media::PlayerStates state_ = OHOS::Media::PLAYER_IDLE;
};

#endif // NATIVE_SCREEN_CAPTURE_MAGIC_H