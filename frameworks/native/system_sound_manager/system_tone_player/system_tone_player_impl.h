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

#ifndef SYSTEM_TONE_PLAYER_IMPL_H
#define SYSTEM_TONE_PLAYER_IMPL_H

#include "isoundpool.h"
#include "system_sound_manager.h"
#include "system_sound_vibrator.h"

namespace OHOS {
namespace Media {
class SystemTonePlayerImpl : public SystemTonePlayer {
public:
    SystemTonePlayerImpl(const std::shared_ptr<AbilityRuntime::Context> &context, SystemSoundManager &systemSoundMgr,
        SystemToneType systemToneType);
    ~SystemTonePlayerImpl();

    // SystemTonePlayer override
    std::string GetTitle() const override;
    int32_t Prepare() override;
    int32_t Start() override;
    int32_t Start(SystemToneOptions systemToneOptions) override;
    int32_t Stop(int32_t streamID) override;
    int32_t Release() override;

private:
    void InitPlayer();
    int32_t ApplyDefaultSystemToneUri(std::string &defaultUri);

    std::shared_ptr<Media::ISoundPool> player_ = nullptr;
    std::shared_ptr<AbilityRuntime::Context> context_;
    SystemSoundManager &systemSoundMgr_;
    SystemToneType systemToneType_;
    std::string configuredUri_ = "";
    int32_t soundID_ = -1;
};
} // namespace Media
} // namespace OHOS
#endif // SYSTEM_TONE_PLAYER_IMPL_H
