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

#include "audio_haptic_sound_normal_impl.h"

#include <fcntl.h>

#include "media_log.h"
#include "media_errors.h"
#include "player.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioHapticSoundNormalImpl"};
}

namespace OHOS {
namespace Media {
const int32_t LOAD_WAIT_SECONDS = 2;

AudioHapticSoundNormalImpl::AudioHapticSoundNormalImpl(const std::string &audioUri, const bool &muteAudio,
    const AudioStandard::StreamUsage &streamUsage)
    : audioUri_(audioUri),
      muteAudio_(muteAudio),
      streamUsage_(streamUsage)
{
}

AudioHapticSoundNormalImpl::~AudioHapticSoundNormalImpl()
{
    if (avPlayer_ != nullptr) {
        ReleaseAVPlayer();
    }
}

int32_t AudioHapticSoundNormalImpl::LoadAVPlayer()
{
    avPlayer_ = PlayerFactory::CreatePlayer();
    CHECK_AND_RETURN_RET_LOG(avPlayer_ != nullptr, MSERR_INVALID_VAL, "Failed to create AvPlayer instance");

    avPlayerCallback_ = std::make_shared<AHSoundNormalCallback>(shared_from_this());
    CHECK_AND_RETURN_RET_LOG(avPlayerCallback_ != nullptr, MSERR_INVALID_VAL, "Failed to create callback object");
    avPlayer_->SetPlayerCallback(avPlayerCallback_);

    configuredAudioUri_ = "";
    playerState_ = AudioHapticPlayerState::STATE_NEW;
    return MSERR_OK;
}

int32_t AudioHapticSoundNormalImpl::PrepareSound()
{
    MEDIA_LOGI("PrepareSound with AVPlayer");
    std::lock_guard<std::mutex> lock(audioHapticPlayerLock_);
    int32_t result = LoadAVPlayer();
    CHECK_AND_RETURN_RET_LOG(result == MSERR_OK && avPlayer_ != nullptr, MSERR_INVALID_VAL,
        "Audio haptic player(avplayer) instance is null");
    CHECK_AND_RETURN_RET_LOG(!audioUri_.empty(), MSERR_OPEN_FILE_FAILED, "The audio uri is empty");
    if (audioUri_ != configuredAudioUri_) {
        return ResetAVPlayer();
    }
    return MSERR_OK;
}

int32_t AudioHapticSoundNormalImpl::ResetAVPlayer()
{
    // Reset the player and reload it.
    MEDIA_LOGI("ResetAVPlayer");
    (void)avPlayer_->Reset();
    MEDIA_LOGI("Set audio source to avplayer. audioUri [%{public}s]", audioUri_.c_str());
    if (fileDes_ != -1) {
        (void)close(fileDes_);
        fileDes_ = -1;
    }
    fileDes_ = open(audioUri_.c_str(), O_RDONLY);
    if (fileDes_ == -1) {
        MEDIA_LOGE("Prepare: Failed to open the audio uri for avplayer.");
        return MSERR_OPEN_FILE_FAILED;
    }
    int32_t ret = avPlayer_->SetSource(fileDes_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_OPEN_FILE_FAILED, "Set source failed %{public}d", ret);

    Format format;
    format.PutIntValue(PlayerKeys::CONTENT_TYPE, AudioStandard::CONTENT_TYPE_UNKNOWN);
    format.PutIntValue(PlayerKeys::STREAM_USAGE, streamUsage_);
    ret = avPlayer_->SetParameter(format);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Set stream usage to AVPlayer failed %{public}d", ret);

    ret = avPlayer_->PrepareAsync();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Prepare failed %{public}d", ret);

    std::unique_lock<std::mutex> lockPrepare(prepareMutex_);
    prepareCond_.wait_for(lockPrepare, std::chrono::seconds(LOAD_WAIT_SECONDS),
        [this]() { return isPrepared_ || isReleased_ || isUnsupportedFile_; });
    CHECK_AND_RETURN_RET_LOG(!isReleased_, MSERR_INVALID_OPERATION,
        "The avplayer is released when it is preparing.");
    CHECK_AND_RETURN_RET_LOG(!isUnsupportedFile_, MSERR_UNSUPPORT_FILE,
        "Failed to load audio uri: report unsupported file err when preparing avplayer.");
    CHECK_AND_RETURN_RET_LOG(isPrepared_, MSERR_OPEN_FILE_FAILED,
        "Failed to load audio uri: time out.");

    // The avplayer has been prepared.
    float actualVolume = volume_ * (muteAudio_ ? 0 : 1);
    MEDIA_LOGI("AVPlayer has been prepared. Set volume %{public}f and loop %{public}d.", actualVolume, loop_);
    (void)avPlayer_->SetVolume(actualVolume, actualVolume);
    (void)avPlayer_->SetLooping(loop_);

    configuredAudioUri_ = audioUri_;
    playerState_ = AudioHapticPlayerState::STATE_PREPARED;
    return MSERR_OK;
}


int32_t AudioHapticSoundNormalImpl::StartSound()
{
    MEDIA_LOGI("StartSound with AVPlayer");
    std::lock_guard<std::mutex> lock(audioHapticPlayerLock_);
    CHECK_AND_RETURN_RET_LOG(avPlayer_ != nullptr && playerState_ != AudioHapticPlayerState::STATE_INVALID,
        MSERR_INVALID_VAL, "StartAVPlayer: no available AVPlayer_");
    CHECK_AND_RETURN_RET_LOG(!audioUri_.empty(), MSERR_OPEN_FILE_FAILED, "The audio uri is empty");

    if (playerState_ == AudioHapticPlayerState::STATE_RUNNING) {
        MEDIA_LOGE("The avplayer has been running. Cannot start again");
        return MSERR_START_FAILED;
    }

    // Player doesn't support play in stopped state. Hence reinitialise player for making start<-->stop to work
    if (playerState_ == AudioHapticPlayerState::STATE_STOPPED || audioUri_ != configuredAudioUri_) {
        ResetAVPlayer();
    }
    auto ret = avPlayer_->Play();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_START_FAILED, "Start failed %{public}d", ret);

    playerState_ = AudioHapticPlayerState::STATE_RUNNING;
    return MSERR_OK;
}

int32_t AudioHapticSoundNormalImpl::StopSound()
{
    MEDIA_LOGI("StopSound with AVPlayer");
    std::lock_guard<std::mutex> lock(audioHapticPlayerLock_);
    CHECK_AND_RETURN_RET_LOG(avPlayer_ != nullptr && playerState_ != AudioHapticPlayerState::STATE_INVALID,
        MSERR_INVALID_VAL, "StopAVPlayer: no available AVPlayer_");

    if (playerState_ != AudioHapticPlayerState::STATE_STOPPED) {
        (void)avPlayer_->Stop();
    }

    playerState_ = AudioHapticPlayerState::STATE_STOPPED;
    return MSERR_OK;
}

int32_t AudioHapticSoundNormalImpl::ReleaseSound()
{
    MEDIA_LOGI("ReleaseSound with AVPlayer");
    {
        std::lock_guard<std::mutex> lockPrepare(prepareMutex_);
        isReleased_ = true;
        prepareCond_.notify_one();
    }
    std::lock_guard<std::mutex> lock(audioHapticPlayerLock_);
    CHECK_AND_RETURN_RET_LOG(playerState_ != AudioHapticPlayerState::STATE_RELEASED, MSERR_OK,
        "The audio haptic player for normal mode has been released.");
    ReleaseAVPlayer();
    playerState_ = AudioHapticPlayerState::STATE_RELEASED;
    return MSERR_OK;
}

void AudioHapticSoundNormalImpl::ReleaseAVPlayer()
{
    if (avPlayer_ != nullptr) {
        (void)avPlayer_->Release();
        avPlayer_ = nullptr;
    }
    avPlayerCallback_ = nullptr;
    if (fileDes_ != -1) {
        (void)close(fileDes_);
        fileDes_ = -1;
    }
}

int32_t AudioHapticSoundNormalImpl::SetVolume(float volume)
{
    MEDIA_LOGI("AudioHapticSoundNormalImpl::SetVolume %{public}f", volume);
    if (volume < 0.0f || volume > 1.0f) {
        MEDIA_LOGE("SetVolume: the volume value is invalid.");
        return MSERR_INVALID_VAL;
    }

    std::lock_guard<std::mutex> lock(audioHapticPlayerLock_);
    int32_t result = MSERR_OK;
    volume_ = volume;

    if (playerState_ != AudioHapticPlayerState::STATE_PREPARED &&
        playerState_ != AudioHapticPlayerState::STATE_RUNNING) {
        MEDIA_LOGI("Audio haptic player is not prepared or running. No need to modify player");
        return result;
    }

    float actualVolume = volume_ * (muteAudio_ ? 0 : 1);
    result = avPlayer_->SetVolume(actualVolume, actualVolume);
    return result;
}

int32_t AudioHapticSoundNormalImpl::SetLoop(bool loop)
{
    MEDIA_LOGI("AudioHapticSoundNormalImpl::SetLoop %{public}d", loop);
    std::lock_guard<std::mutex> lock(audioHapticPlayerLock_);
    int32_t result = MSERR_OK;
    loop_ = loop;

    if (playerState_ != AudioHapticPlayerState::STATE_PREPARED &&
        playerState_ != AudioHapticPlayerState::STATE_RUNNING) {
        MEDIA_LOGI("Audio haptic player is not prepared or running. No need to modify player");
        return result;
    }

    result = avPlayer_->SetLooping(loop_);
    return result;
}

int32_t AudioHapticSoundNormalImpl::GetAudioCurrentTime()
{
    if (avPlayer_ == nullptr) {
        MEDIA_LOGE("GetAudioCurrentTime: avPlayer_ is nullptr. This function is only usable for avPlayer.");
        return -1;
    }
    int32_t currentTime = -1;
    (void)avPlayer_->GetCurrentTime(currentTime);
    return currentTime;
}

int32_t AudioHapticSoundNormalImpl::SetAudioHapticSoundCallback(
    const std::shared_ptr<AudioHapticSoundCallback> &callback)
{
    if (callback == nullptr) {
        MEDIA_LOGE("The audio haptic player callback is nullptr.");
        return MSERR_INVALID_VAL;
    }

    std::lock_guard<std::mutex> lock(audioHapticPlayerLock_);
    audioHapticPlayerCallback_ = callback;
    return MSERR_OK;
}

void AudioHapticSoundNormalImpl::SetAVPlayerState(AudioHapticPlayerState playerState)
{
    MEDIA_LOGI("SetAVPlayerState, state %{public}d", playerState);
    playerState_ = playerState;
}

void AudioHapticSoundNormalImpl::NotifyPreparedEvent()
{
    std::lock_guard<std::mutex> lockPrepare(prepareMutex_);
    isPrepared_ = true;
    prepareCond_.notify_one();
}

void AudioHapticSoundNormalImpl::NotifyErrorEvent(int32_t errorCode)
{
    MediaServiceErrCode mediaErr = static_cast<MediaServiceErrCode>(errorCode);
    if (mediaErr == MSERR_UNSUPPORT_FILE) {
        std::lock_guard<std::mutex> lockPrepare(prepareMutex_);
        isUnsupportedFile_ = true;
        prepareCond_.notify_one();
    }

    std::shared_ptr<AudioHapticSoundCallback> cb = audioHapticPlayerCallback_.lock();
    if (cb != nullptr) {
        MEDIA_LOGI("NotifyFirstFrameEvent for audio haptic player");
        cb->OnError(errorCode);
    } else {
        MEDIA_LOGE("NotifyFirstFrameEvent: audioHapticPlayerCallback_ is nullptr");
    }
}

void AudioHapticSoundNormalImpl::NotifyFirstFrameEvent(uint64_t latency)
{
    std::shared_ptr<AudioHapticSoundCallback> cb = audioHapticPlayerCallback_.lock();
    if (cb != nullptr) {
        MEDIA_LOGI("NotifyFirstFrameEvent for audio haptic player");
        cb->OnFirstFrameWriting(latency);
    } else {
        MEDIA_LOGE("NotifyFirstFrameEvent: audioHapticPlayerCallback_ is nullptr");
    }
}

void AudioHapticSoundNormalImpl::NotifyInterruptEvent(AudioStandard::InterruptEvent &interruptEvent)
{
    std::shared_ptr<AudioHapticSoundCallback> cb = audioHapticPlayerCallback_.lock();
    if (cb != nullptr) {
        MEDIA_LOGI("NotifyInterruptEvent for audio haptic player");
        cb->OnInterrupt(interruptEvent);
    } else {
        MEDIA_LOGE("NotifyInterruptEvent: audioHapticPlayerCallback_ is nullptr");
    }
}

void AudioHapticSoundNormalImpl::NotifyEndOfStreamEvent()
{
    MEDIA_LOGI("NotifyEndOfStreamEvent");
    playerState_ = AudioHapticPlayerState::STATE_STOPPED;
    std::shared_ptr<AudioHapticSoundCallback> cb = audioHapticPlayerCallback_.lock();
    if (cb != nullptr) {
        MEDIA_LOGI("NotifyEndOfStreamEvent for audio haptic player");
        cb->OnEndOfStream();
    } else {
        MEDIA_LOGE("NotifyEndOfStreamEvent: audioHapticPlayerCallback_ is nullptr");
    }
}

// Callback class symbols
AHSoundNormalCallback::AHSoundNormalCallback(std::shared_ptr<AudioHapticSoundNormalImpl> soundNormalImpl)
    : soundNormalImpl_(soundNormalImpl) {}

void AHSoundNormalCallback::OnError(int32_t errorCode, const std::string &errorMsg)
{
    MEDIA_LOGE("OnError reported from AVPlayer: %{public}d", errorCode);
    std::shared_ptr<AudioHapticSoundNormalImpl> soundNormalImpl = soundNormalImpl_.lock();
    if (soundNormalImpl == nullptr) {
        MEDIA_LOGE("The audio haptic player for normal mode has been released.");
        return;
    }
    soundNormalImpl->NotifyErrorEvent(errorCode);
}

void AHSoundNormalCallback::OnInfo(Media::PlayerOnInfoType type, int32_t extra, const Media::Format &infoBody)
{
    if (type == INFO_TYPE_STATE_CHANGE) {
        MEDIA_LOGI("OnInfo: state change reported from AVPlayer.");
        HandleStateChangeEvent(extra, infoBody);
    } else if (type == INFO_TYPE_INTERRUPT_EVENT) {
        MEDIA_LOGI("OnInfo: interrupt event reported from AVPlayer.");
        HandleAudioInterruptEvent(extra, infoBody);
    } else if (type == INFO_TYPE_AUDIO_FIRST_FRAME) {
        MEDIA_LOGI("OnInfo: first frame event reported from AVPlayer.");
        HandleAudioFirstFrameEvent(extra, infoBody);
    } else {
        return;
    }
}

void AHSoundNormalCallback::HandleStateChangeEvent(int32_t extra, const Format &infoBody)
{
    MEDIA_LOGI("HandleStateChangeEvent from AVPlayer");
    PlayerStates avPlayerState = static_cast<PlayerStates>(extra);
    switch (avPlayerState) {
        case PLAYER_STATE_ERROR:
            playerState_ = AudioHapticPlayerState::STATE_INVALID;
            break;
        case PLAYER_IDLE:
        case PLAYER_INITIALIZED:
        case PLAYER_PREPARING:
            playerState_ = AudioHapticPlayerState::STATE_NEW;
            break;
        case PLAYER_PREPARED:
            playerState_ = AudioHapticPlayerState::STATE_PREPARED;
            break;
        case PLAYER_STARTED:
            playerState_ = AudioHapticPlayerState::STATE_RUNNING;
            break;
        case PLAYER_PAUSED:
            playerState_ = AudioHapticPlayerState::STATE_PAUSED;
            break;
        case PLAYER_STOPPED:
        case PLAYER_PLAYBACK_COMPLETE:
            playerState_ = AudioHapticPlayerState::STATE_STOPPED;
            break;
        default:
            break;
    }
    std::shared_ptr<AudioHapticSoundNormalImpl> soundNormalImpl = soundNormalImpl_.lock();
    if (soundNormalImpl == nullptr) {
        MEDIA_LOGE("The audio haptic player for normal mode has been released.");
        return;
    }
    soundNormalImpl->SetAVPlayerState(playerState_);

    if (avPlayerState == PLAYER_PREPARED) {
        soundNormalImpl->NotifyPreparedEvent();
    } else if (avPlayerState == PLAYER_PLAYBACK_COMPLETE) {
        soundNormalImpl->NotifyEndOfStreamEvent();
    }
}

void AHSoundNormalCallback::HandleAudioInterruptEvent(int32_t extra, const Format &infoBody)
{
    MEDIA_LOGI("HandleAudioInterruptEvent from AVPlayer");
    AudioStandard::InterruptEvent interruptEvent;
    int32_t eventTypeValue = 0;
    int32_t forceTypeValue = 0;
    int32_t hintTypeValue = 0;
    (void)infoBody.GetIntValue(PlayerKeys::AUDIO_INTERRUPT_TYPE, eventTypeValue);
    (void)infoBody.GetIntValue(PlayerKeys::AUDIO_INTERRUPT_FORCE, forceTypeValue);
    (void)infoBody.GetIntValue(PlayerKeys::AUDIO_INTERRUPT_HINT, hintTypeValue);
    interruptEvent.eventType = static_cast<AudioStandard::InterruptType>(eventTypeValue);
    interruptEvent.forceType = static_cast<AudioStandard::InterruptForceType>(forceTypeValue);
    interruptEvent.hintType = static_cast<AudioStandard::InterruptHint>(hintTypeValue);

    std::shared_ptr<AudioHapticSoundNormalImpl> soundNormalImpl = soundNormalImpl_.lock();
    if (soundNormalImpl == nullptr) {
        MEDIA_LOGE("The audio haptic player for normal mode has been released.");
        return;
    }
    soundNormalImpl->NotifyInterruptEvent(interruptEvent);
}

void AHSoundNormalCallback::HandleAudioFirstFrameEvent(int32_t extra, const Format &infoBody)
{
    int64_t value = 0;
    (void)infoBody.GetLongValue(PlayerKeys::AUDIO_FIRST_FRAME, value);
    uint64_t latency = static_cast<uint64_t>(value);
    MEDIA_LOGI("HandleAudioFirstFrameEvent from AVPlayer. Latency %{public}" PRIu64 "", latency);
    std::shared_ptr<AudioHapticSoundNormalImpl> soundNormalImpl = soundNormalImpl_.lock();
    if (soundNormalImpl == nullptr) {
        MEDIA_LOGE("The audio haptic player for normal mode has been released.");
        return;
    }
    soundNormalImpl->NotifyFirstFrameEvent(latency);
}
} // namesapce AudioStandard
} // namespace OHOS