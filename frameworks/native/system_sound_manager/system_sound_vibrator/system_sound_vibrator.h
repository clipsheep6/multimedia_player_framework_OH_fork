/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef SYSTEM_SOUND_VIBRATOR_H
#define SYSTEM_SOUND_VIBRATOR_H

#include <cstdint>

namespace OHOS {
namespace Media {
enum class VibrationType {
    VIBRATION_RINGTONE = 0,
    VIBRATION_SYSTEM_TONE,
};

class SystemSoundVibrator {
public:
    static int32_t StartVibrator(VibrationType type);
    static int32_t StopVibrator();
};
} // namespace Media
} // namespace OHOS
#endif // SYSTEM_SOUND_VIBRATOR_H
