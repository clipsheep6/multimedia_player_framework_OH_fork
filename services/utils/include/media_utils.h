/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#ifndef HISTREAMER_MEDIA_UTILS_H
#define HISTREAMER_MEDIA_UTILS_H

#include "i_player_engine.h"
#include "common/status.h"
#include "meta/media_types.h"
enum class PlayerStateId {
    IDLE = 0,
    INIT = 1,
    PREPARING = 2,
    READY = 3,
    PAUSE = 4,
    PLAYING = 5,
    STOPPED = 6,
    EOS = 7,
    ERROR = 8,
};

namespace OHOS {
namespace Media {
    std::string __attribute__((visibility("default"))) GetClientBundleName(int32_t uid);
    std::string __attribute__((visibility("default"))) GetBundleResourceLabel(std::string bundleName);
    int __attribute__((visibility("default"))) TransStatus(Status status);
    PlayerStates __attribute__((visibility("default"))) TransStateId2PlayerState(PlayerStateId state);
    Plugins::SeekMode __attribute__((visibility("default"))) Transform2SeekMode(PlayerSeekMode mode);
    const std::string& __attribute__((visibility("default"))) StringnessPlayerState(PlayerStates state);
    float __attribute__((visibility("default"))) TransformPlayRate2Float(PlaybackRateMode rateMode);
    inline PlaybackRateMode __attribute__((visibility("default"))) TransformFloat2PlayRate(float rate);
    double __attribute__((visibility("default"))) ChangeModeToSpeed(const PlaybackRateMode& mode);
    bool __attribute__((visibility("default"))) IsEnableOptimizeDecode();
    constexpr double SPEED_0_75_X = 0.75;
    constexpr double SPEED_1_00_X = 1.00;
    constexpr double SPEED_1_25_X = 1.25;
    constexpr double SPEED_1_75_X = 1.75;
    constexpr double SPEED_2_00_X = 2.00;
}  // namespace Media
}  // namespace OHOS

#endif  // HISTREAMER_MEDIA_UTILS_H