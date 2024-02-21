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

#ifndef AUDIO_HAPTIC_PLAYER_IMPL_H
#define AUDIO_HAPTIC_PLAYER_IMPL_H

#include "audio_haptic_player.h"
#include "audio_haptic_vibrator.h"
#include "isoundpool.h"
#include "player.h"

namespace OHOS {
namespace Media {
enum AudioHapticPlayerType {
    AUDIO_HAPTIC_TYPE_NORMAL = 0,
    AUDIO_HAPTIC_TYPE_FAST = 1,
    AUDIO_HAPTIC_TYPE_DEFAULT = 2,
};

class AudioHapticPlayerNativeCallback;
class AudioHapticFirstFrameCb;

class AudioHapticPlayerImpl : public AudioHapticPlayer, public std::enable_shared_from_this<AudioHapticPlayerImpl> {
public:
    AudioHapticPlayerImpl();
    ~AudioHapticPlayerImpl();

    // AudioHapticPlayer override
    bool IsMuted(const AudioHapticType &audioHapticType) const override;
    int32_t Prepare() override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Release() override;
    int32_t SetVolume(float volume) override;
    int32_t SetLoop(bool loop) override;
    int32_t SetAudioHapticPlayerCallback(const std::shared_ptr<AudioHapticPlayerCallback> &playerCallback) override;
    int32_t GetAudioCurrentTime() override;

    void NotifyPreparedEvent();
    void NotifyInterruptEvent(AudioStandard::InterruptEvent &interruptEvent);
    void NotifyEndOfStreamEvent();
    void NotifyUnsupportedFileEvent();
    void SetPlayerOptions(const bool &muteAudio, const bool &muteHaptic);
    int32_t SetPlayerType(const AudioHapticPlayerType &audioHapticPlayerType);
    int32_t SetPlayerStreamUsage(const AudioStandard::StreamUsage &streamUsage);
    int32_t SetPlayerSource(const std::string audioUri, const std::string hapticUri);

    // func for vibration
    int32_t LoadVibrator();
    int32_t StartVibrate();
    void StopVibrate();
    void ResetVibrateState();
    void NotifyStartVibrate(uint64_t latency);

    // func for soundpool
    int32_t LoadSoundPoolPlayer();

    // func for AVPlayer
    int32_t LoadAVPlayer();
    void SetAVPlayerState(AudioHapticPlayerState playerState);

private:
    // func for soundpool
    int32_t PrepareSoundPool();
    int32_t StartSoundPoolPlayer();
    int32_t StopSoundPoolPlayer();
    int32_t ReleaseSoundPoolPlayer();

    // func for avplayer
    int32_t PrepareAVPlayer();
    int32_t ResetAVPlayer();
    int32_t StartAVPlayer();
    int32_t StopAVPlayer();
    int32_t ReleaseAVPlayer();

    // func for vibration
    int32_t PrepareVibrator();

    // var for all
    AudioHapticPlayerType playerType_;
    AudioStandard::StreamUsage streamUsage_;
    bool muteAudio_;
    bool muteHaptic_;
    std::string audioUri_;
    std::string hapticUri_;
    std::string configuredAudioUri_;
    float volume_ = 1.0f;
    bool loop_ = false;
    AudioHapticPlayerState playerState_ = AudioHapticPlayerState::STATE_INVALID;
    bool isPrepared_ = false;
    bool isReleased_ = false;
    bool isUnsupportedFile_ = false;
    std::mutex prepareMutex_;
    std::condition_variable prepareCond_;

    // var for callback
    std::shared_ptr<PlayerCallback> avPlayerCallback_ = nullptr;
    std::shared_ptr<ISoundPoolCallback> soundPoolCallback_ = nullptr;
    std::weak_ptr<AudioHapticPlayerCallback> audioHapticPlayerCallback_;
    std::mutex audioHapticPlayerLock_;
    std::shared_ptr<AudioHapticFirstFrameCb> firstFrameCb_ = nullptr;
    uint64_t audioLatency_ = 0;

    // var for vibrate
    std::unique_ptr<AudioHapticVibrator> audioHapticVibrator_ = nullptr;
    std::shared_ptr<std::thread> vibrateThread_;
    std::mutex waitStartVibrateMutex_;
    std::condition_variable condStartVibrate_;
    bool isAudioPlayFirstFrame_ = false;
    bool isVibrationStopped_ = false;

    // var for soundpool
    std::shared_ptr<Media::ISoundPool> soundPoolPlayer_ = nullptr;
    int32_t soundID_ = -1;
    int32_t streamID_ = -1;
    int32_t fileDes_ = -1;

    // var for avplayer
    std::shared_ptr<Media::Player> avPlayer_ = nullptr;
    bool isStartQueued_ = false;
};

class AudioHapticPlayerNativeCallback : public ISoundPoolCallback, public PlayerCallback {
public:
    explicit AudioHapticPlayerNativeCallback(std::shared_ptr<AudioHapticPlayerImpl> audioHapticPlayerImpl);
    virtual ~AudioHapticPlayerNativeCallback() = default;

    // ISoundPoolCallback
    void OnLoadCompleted(int32_t soundId) override;
    void OnPlayFinished() override;
    void OnError(int32_t errorCode) override;

    // PlayerCallback
    void OnError(int32_t errorCode, const std::string &errorMsg) override;
    void OnInfo(Media::PlayerOnInfoType type, int32_t extra, const Media::Format &infoBody) override;

private:
    std::weak_ptr<AudioHapticPlayerImpl> audioHapticPlayerImpl_;

    void HandleStateChangeEvent(int32_t extra, const Format &infoBody);
    void HandleAudioInterruptEvent(int32_t extra, const Format &infoBody);
    void HandleAudioFirstFrameEvent(int32_t extra, const Format &infoBody);

    AudioHapticPlayerState playerState_ = AudioHapticPlayerState::STATE_INVALID;
};

class AudioHapticFirstFrameCb : public ISoundPoolFrameWriteCallback {
public:
    explicit AudioHapticFirstFrameCb(std::shared_ptr<AudioHapticPlayerImpl> audioHapticPlayerImpl);
    virtual ~AudioHapticFirstFrameCb() = default;

    void OnFirstAudioFrameWritingCallback(uint64_t &latency) override;
private:
    std::weak_ptr<AudioHapticPlayerImpl> audioHapticPlayerImpl_;
};


} // namespace Media
} // namespace OHOS
#endif // AUDIO_HAPTIC_PLAYER_IMPL_H
