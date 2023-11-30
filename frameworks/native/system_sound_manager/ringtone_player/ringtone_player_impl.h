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

#ifndef RINGTONE_PLAYER_IMPL_H
#define RINGTONE_PLAYER_IMPL_H

#include "player.h"
#include "system_sound_manager_impl.h"
#include "system_sound_vibrator.h"


namespace OHOS {
namespace Media {
class RingtonePlayerCallback;

class RingtonePlayerInterruptCallback;

class RingtonePlayerImpl : public RingtonePlayer {
public:
    RingtonePlayerImpl(const std::shared_ptr<AbilityRuntime::Context> &context,
        SystemSoundManagerImpl &sysSoundMgr, RingtoneType type);
    ~RingtonePlayerImpl();
    void SetPlayerState(RingtoneState ringtoneState);

    // IRingtone override
    RingtoneState GetRingtoneState() override;
    int32_t Configure(const float &volume, const bool &loop) override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Release() override;
    int32_t GetAudioRendererInfo(AudioStandard::AudioRendererInfo &rendererInfo) const override;
    std::string GetTitle() override;
    int32_t SetRingtonePlayerInterruptCallback(
        const std::shared_ptr<RingtonePlayerInterruptCallback> &interruptCallback) override;
    void NotifyInterruptEvent(AudioStandard::InterruptEvent &interruptEvent);

private:
    void InitPlayer();
    int32_t PrepareRingtonePlayer(bool isReInitNeeded);
    int32_t ApplyDefaultRingtoneUri(std::string &defaultUri);

    float volume_ = 1.0f;
    bool loop_ = false;
    bool isStartQueued_ = false;
    std::string configuredUri_ = "";
    std::shared_ptr<Media::Player> player_ = nullptr;
    std::shared_ptr<AbilityRuntime::Context> context_;
    std::shared_ptr<RingtonePlayerCallback> callback_ = nullptr;
    std::shared_ptr<RingtonePlayerInterruptCallback> interruptCallback_ = nullptr;
    SystemSoundManagerImpl &systemSoundMgr_;
    RingtoneType type_ = RINGTONE_TYPE_SIM_CARD_0;
    RingtoneState ringtoneState_ = STATE_NEW;
};

class RingtonePlayerCallback : public PlayerCallback {
public:
    explicit RingtonePlayerCallback(RingtonePlayerImpl &ringtonePlayerImpl);
    virtual ~RingtonePlayerCallback() = default;
    void OnError(int32_t errorCode, const std::string &errorMsg) override;
    void OnInfo(Media::PlayerOnInfoType type, int32_t extra, const Media::Format &infoBody) override;

private:
    void HandleStateChangeEvent(int32_t extra, const Format &infoBody);
    void HandleAudioInterruptEvent(int32_t extra, const Format &infoBody);

    Media::PlayerStates state_ = Media::PLAYER_IDLE;
    RingtoneState ringtoneState_ = STATE_NEW;
    RingtonePlayerImpl &ringtonePlayerImpl_;
};
} // namespace Media
} // namespace OHOS
#endif // RINGTONE_PLAYER_IMPL_H
