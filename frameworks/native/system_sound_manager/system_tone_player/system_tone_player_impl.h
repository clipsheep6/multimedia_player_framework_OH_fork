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

#include <condition_variable>
#include <mutex>

#include "audio_haptic_manager.h"
#include "system_sound_manager_impl.h"

namespace OHOS {
namespace Media {
class SystemTonePlayerCallback;

class SystemTonePlayerImpl : public SystemTonePlayer, public std::enable_shared_from_this<SystemTonePlayerImpl> {
public:
    SystemTonePlayerImpl(const std::shared_ptr<AbilityRuntime::Context> &context,
        SystemSoundManagerImpl &systemSoundMgr, SystemToneType systemToneType);
    ~SystemTonePlayerImpl();

    // SystemTonePlayer override
    std::string GetTitle() const override;
    int32_t Prepare() override;
    int32_t Start() override;
    int32_t Start(const SystemToneOptions &systemToneOptions) override;
    int32_t Stop(const int32_t &streamID) override;
    int32_t Release() override;

    void NotifyEndOfStream();

private:
    int32_t InitPlayer(const std::string &audioUri);
    bool GetMuteHapticsValue();

    std::shared_ptr<AudioHapticManager> audioHapticManager_ = nullptr;
    std::shared_ptr<AudioHapticPlayer> player_ = nullptr;
    std::shared_ptr<SystemTonePlayerCallback> callback_ = nullptr;
    bool muteAudio_ = false;
    bool muteHaptics_ = false;
    int32_t sourceId_ = -1;
    std::string configuredUri_ = "";

    std::shared_ptr<AbilityRuntime::Context> context_;
    SystemSoundManagerImpl &systemSoundMgr_;
    SystemToneType systemToneType_;
    SystemToneState systemToneState_ = SystemToneState::STATE_NEW;
    std::mutex systemTonePlayerMutex_;
};

class SystemTonePlayerCallback : public AudioHapticPlayerCallback {
public:
    explicit SystemTonePlayerCallback(std::shared_ptr<SystemTonePlayerImpl> systemTonePlayerImpl);
    virtual ~SystemTonePlayerCallback() = default;

    void OnInterrupt(const AudioStandard::InterruptEvent &interruptEvent);
    void OnEndOfStream(void);

private:
    std::weak_ptr<SystemTonePlayerImpl> systemTonePlayerImpl_;
};
} // namespace Media
} // namespace OHOS
#endif // SYSTEM_TONE_PLAYER_IMPL_H
