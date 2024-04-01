/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef AUDIO_HAPTIC_SOUND_NORMAL_IMPL_H
#define AUDIO_HAPTIC_SOUND_NORMAL_IMPL_H

#include "audio_haptic_sound.h"
#include "player.h"

namespace OHOS {
namespace Media {
class AudioHapticSoundNormalImpl : public AudioHapticSound,
    public std::enable_shared_from_this<AudioHapticSoundNormalImpl> {
public:
    AudioHapticSoundNormalImpl(const std::string &audioUri, const bool &muteAudio,
        const AudioStandard::StreamUsage &streamUsage);
    ~AudioHapticSoundNormalImpl();

    // AudioHapticSound override
    int32_t PrepareSound() override;
    int32_t StartSound() override;
    int32_t StopSound() override;
    int32_t ReleaseSound() override;
    int32_t SetVolume(float volume) override;
    int32_t SetLoop(bool loop) override;
    int32_t GetAudioCurrentTime() override;
    int32_t SetAudioHapticSoundCallback(const std::shared_ptr<AudioHapticSoundCallback> &callback) override;

    void SetAVPlayerState(AudioHapticPlayerState playerState);
    void NotifyPreparedEvent();
    void NotifyErrorEvent(int32_t errorCode);
    void NotifyFirstFrameEvent(uint64_t latency);
    void NotifyInterruptEvent(AudioStandard::InterruptEvent &interruptEvent);
    void NotifyEndOfStreamEvent();

private:
    int32_t LoadAVPlayer();
    int32_t ResetAVPlayer();
    void ReleaseAVPlayer();

    std::string audioUri_ = "";
    bool muteAudio_ = false;
    AudioStandard::StreamUsage streamUsage_ = AudioStandard::STREAM_USAGE_UNKNOWN;
    float volume_ = 1.0f;
    bool loop_ = false;
    std::string configuredAudioUri_ = "";
    AudioHapticPlayerState playerState_ = AudioHapticPlayerState::STATE_NEW;

    std::weak_ptr<AudioHapticSoundCallback> audioHapticPlayerCallback_;

    std::mutex audioHapticPlayerLock_;

    // var for avplayer
    std::shared_ptr<Media::Player> avPlayer_ = nullptr;
    std::shared_ptr<PlayerCallback> avPlayerCallback_ = nullptr;
    int32_t fileDes_ = -1;
    bool isPrepared_ = false;
    bool isReleased_ = false;
    bool isUnsupportedFile_ = false;
    std::mutex prepareMutex_;
    std::condition_variable prepareCond_;
};

class AHSoundNormalCallback : public PlayerCallback {
public:
    explicit AHSoundNormalCallback(std::shared_ptr<AudioHapticSoundNormalImpl> soundNormalImpl);
    virtual ~AHSoundNormalCallback() = default;

    // PlayerCallback override
    void OnError(int32_t errorCode, const std::string &errorMsg) override;
    void OnInfo(Media::PlayerOnInfoType type, int32_t extra, const Media::Format &infoBody) override;

private:
    void HandleStateChangeEvent(int32_t extra, const Format &infoBody);
    void HandleAudioInterruptEvent(int32_t extra, const Format &infoBody);
    void HandleAudioFirstFrameEvent(int32_t extra, const Format &infoBody);

    std::weak_ptr<AudioHapticSoundNormalImpl> soundNormalImpl_;
    AudioHapticPlayerState playerState_;
};
} // namespace Media
} // namespace OHOS
#endif // AUDIO_HAPTIC_SOUND_NORMAL_IMPL_H
