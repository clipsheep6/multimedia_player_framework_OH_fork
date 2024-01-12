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

#include "system_sound_vibrator.h"

#ifdef SUPPORT_VIBRATOR
#include "vibrator_agent.h"
#endif

#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SystemSoundVibrator"};
}

namespace OHOS {
namespace Media {
#ifdef SUPPORT_VIBRATOR
const std::unordered_map<VibrationType, VibratorUsage> VIBRATOR_USAGE_MAP = {
    {VibrationType::VIBRATION_RINGTONE, USAGE_RING},
    {VibrationType::VIBRATION_SYSTEM_TONE, USAGE_NOTIFICATION},
};

const std::unordered_map<VibrationType, int32_t> LOOP_COUNT_MAP = {
    // Default loop count. Ringtone need be repeated.
    {VibrationType::VIBRATION_RINGTONE, 10},
    {VibrationType::VIBRATION_SYSTEM_TONE, 1},
};

const std::unordered_map<VibrationType, std::string> EFFECT_ID_MAP = {
    // Default effectId
    {VibrationType::VIBRATION_RINGTONE, "haptic.ringtone.Dream_It_Possible"},
    {VibrationType::VIBRATION_SYSTEM_TONE, "haptic.pattern.type4"},
};
#endif

int32_t SystemSoundVibrator::StartVibrator(VibrationType type)
{
    MEDIA_LOGD("StartVibrator: for vibration type %{public}d", type);
    int32_t result = MSERR_OK;
#ifdef SUPPORT_VIBRATOR
    bool setUsageRet = Sensors::SetUsage(VIBRATOR_USAGE_MAP.at(type));
    bool setLoopRet = Sensors::SetLoopCount(LOOP_COUNT_MAP.at(type));
    result = Sensors::StartVibrator(EFFECT_ID_MAP.at(type).c_str());
    MEDIA_LOGI("StartVibrator: setUsageRet %{public}d, setLoopRet %{public}d, startRet %{public}d",
        setUsageRet, setLoopRet, result);
#endif
    return result;
}

int32_t SystemSoundVibrator::StopVibrator()
{
    int32_t result = MSERR_OK;
#ifdef SUPPORT_VIBRATOR
    result = Sensors::Cancel();
    MEDIA_LOGI("StopVibrator: %{public}d", result);
#endif
    return result;
}
} // namesapce AudioStandard
} // namespace OHOS
