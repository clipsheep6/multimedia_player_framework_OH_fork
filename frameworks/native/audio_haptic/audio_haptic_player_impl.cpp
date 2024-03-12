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

#include "audio_haptic_player_impl.h"

#include <fcntl.h>

#include "isoundpool.h"
#include "media_log.h"
#include "media_errors.h"
#include "player.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioHapticPlayerImpl"};
}

namespace OHOS {
namespace Media {
const int32_t MAX_SOUND_POOL_STREAMS = 1; // ensure that only one system tone is playing.
const int32_t LOAD_WAIT_SECONDS = 2;
const int32_t LOAD_WAIT_SECONDS_FOR_LOOP = 20;

AudioHapticPlayerImpl::AudioHapticPlayerImpl()
    : playerType_(AUDIO_HAPTIC_TYPE_DEFAULT),
      muteAudio_(false),
      muteHaptic_(false),
      audioUri_(""),
      hapticUri_(""),
      configuredAudioUri_("")
{
}

AudioHapticPlayerImpl::~AudioHapticPlayerImpl()
{
    if (soundPoolPlayer_ != nullptr) {
        ReleaseSoundPoolPlayer();
    }
    if (avPlayer_ != nullptr) {
        ReleaseAVPlayer();
    }
}

void AudioHapticPlayerImpl::SetPlayerOptions(const bool &muteAudio, const bool &muteHaptic)
{
    muteAudio_ = muteAudio;
    muteHaptic_ = muteHaptic;
}

int32_t AudioHapticPlayerImpl::SetPlayerType(const AudioHapticPlayerType &audioHapticPlayerType)
{
    playerType_ = audioHapticPlayerType;
    return MSERR_OK;
}

int32_t AudioHapticPlayerImpl::SetPlayerStreamUsage(const AudioStandard::StreamUsage &streamUsage)
{
    streamUsage_ = streamUsage;
    return MSERR_OK;
}

int32_t AudioHapticPlayerImpl::SetPlayerSource(const std::string audioUri, const std::string hapticUri)
{
    audioUri_ = audioUri;
    hapticUri_ = hapticUri;
    return MSERR_OK;
}

int32_t AudioHapticPlayerImpl::StartVibrate()
{
    if (muteHaptic_) {
        MEDIA_LOGI("StartVibrate: muteHaptic is true. No need to vibrate");
        return MSERR_OK;
    }

    MEDIA_LOGI("Enter StartVibrate()");
    std::unique_lock<std::mutex> lockWait(waitStartVibrateMutex_);
    do {
        int32_t waitTime = loop_ ? LOAD_WAIT_SECONDS_FOR_LOOP : LOAD_WAIT_SECONDS;
        bool waitResult = condStartVibrate_.wait_for(lockWait, std::chrono::seconds(waitTime),
            [this]() { return isAudioPlayFirstFrame_ || isVibrationStopped_; });
        if (!waitResult) {
            MEDIA_LOGE("StartVibrate: Failed to start vibrate (time out).");
            return MSERR_INVALID_OPERATION;
        }
        if (isVibrationStopped_) {
            MEDIA_LOGI("StartVibrate: audio haptic player has been stopped.");
            return MSERR_OK;
        }

        isAudioPlayFirstFrame_ = false; // reset for next time.

        int32_t hapticDelay = audioHapticVibrator_->GetDelayTime();
        int32_t delay = (this->audioLatency_ - hapticDelay) > 0 ? this->audioLatency_ - hapticDelay : 0;
        waitResult = condStartVibrate_.wait_for(lockWait, std::chrono::milliseconds(delay),
            [this]() { return isVibrationStopped_; });
        if (isVibrationStopped_) {
            MEDIA_LOGI("StartVibrate: audio haptic player has been stopped.");
            return MSERR_OK;
        }
        AudioLatencyMode latencyMode = AUDIO_LATENCY_MODE_NORMAL;
        if (playerType_ == AUDIO_HAPTIC_TYPE_FAST) {
            latencyMode = AUDIO_LATENCY_MODE_FAST;
        }
        audioHapticVibrator_->StartVibrate(latencyMode);
    } while (loop_ && !isVibrationStopped_);

    return MSERR_OK;
}

void AudioHapticPlayerImpl::StopVibrate()
{
    MEDIA_LOGI("Stop vibrate for audio haptic player right now.");
    if (audioHapticVibrator_ != nullptr) {
        audioHapticVibrator_->StopVibrate();
    } else {
        MEDIA_LOGW("The audio haptic vibrator is nullptr!");
    }
    {
        std::lock_guard<std::mutex> lockVibrate(waitStartVibrateMutex_);
        isVibrationStopped_ = true;
        condStartVibrate_.notify_one();
    }
    if (vibrateThread_ != nullptr && vibrateThread_->joinable()) {
        vibrateThread_->join();
        vibrateThread_.reset();
    }
}

void AudioHapticPlayerImpl::ResetVibrateState()
{
    isVibrationStopped_ = false;
    if (audioHapticVibrator_ != nullptr) {
        audioHapticVibrator_->ResetStopState();
    } else {
        MEDIA_LOGW("The audio haptic vibrator is nullptr!");
    }
}

int32_t AudioHapticPlayerImpl::LoadVibrator()
{
    audioHapticVibrator_ = AudioHapticVibrator::CreateAudioHapticVibrator(*this);
    CHECK_AND_RETURN_RET_LOG(audioHapticVibrator_ != nullptr, MSERR_INVALID_OPERATION,
        "Failed to create audio haptic vibrator instance");
    return MSERR_OK;
}

int32_t AudioHapticPlayerImpl::PrepareVibrator()
{
    CHECK_AND_RETURN_RET_LOG(audioHapticVibrator_ != nullptr, MSERR_INVALID_OPERATION,
        "Audio haptic vibrator is nullptr");
    CHECK_AND_RETURN_RET_LOG(hapticUri_ != "", MSERR_OPEN_FILE_FAILED, "Invalid val: haptic uri is empty");
    return audioHapticVibrator_->PreLoad(hapticUri_);
}

int32_t AudioHapticPlayerImpl::LoadSoundPoolPlayer()
{
    MEDIA_LOGI("Enter LoadSoundPoolPlayer()");

    AudioStandard::AudioRendererInfo audioRendererInfo;
    audioRendererInfo.contentType = AudioStandard::ContentType::CONTENT_TYPE_UNKNOWN;
    audioRendererInfo.streamUsage = streamUsage_;
    audioRendererInfo.rendererFlags = 1;

    soundPoolPlayer_ = SoundPoolFactory::CreateSoundPool(MAX_SOUND_POOL_STREAMS, audioRendererInfo);
    CHECK_AND_RETURN_RET_LOG(soundPoolPlayer_ != nullptr, MSERR_INVALID_VAL,
        "Failed to create sound pool player instance");

    soundPoolCallback_ = std::make_shared<AudioHapticPlayerNativeCallback>(shared_from_this());
    CHECK_AND_RETURN_RET_LOG(soundPoolCallback_ != nullptr, MSERR_INVALID_VAL, "Failed to create callback object");
    soundPoolPlayer_->SetSoundPoolCallback(soundPoolCallback_);

    firstFrameCb_ = std::make_shared<AudioHapticFirstFrameCb>(shared_from_this());
    CHECK_AND_RETURN_RET_LOG(firstFrameCb_ != nullptr, MSERR_INVALID_VAL, "Failed to create callback object");
    soundPoolPlayer_->SetSoundPoolFrameWriteCallback(firstFrameCb_);

    configuredAudioUri_ = "";
    return MSERR_OK;
}

int32_t AudioHapticPlayerImpl::PrepareSoundPool()
{
    MEDIA_LOGI("Enter PrepareSoundPool()");
    std::lock_guard<std::mutex> lock(audioHapticPlayerLock_);
    CHECK_AND_RETURN_RET_LOG(soundPoolPlayer_ != nullptr, MSERR_INVALID_STATE, "soundpool player instance is null");

    if (!configuredAudioUri_.empty() && configuredAudioUri_ == audioUri_) {
        MEDIA_LOGI("Prepare: The audioUri_ uri has been loaded. Return directly.");
        return MSERR_OK;
    }

    MEDIA_LOGI("Set audio source to soundpool. audioUri [%{public}s]", audioUri_.c_str());
    if (fileDes_ != -1) {
        (void)close(fileDes_);
        fileDes_ = -1;
    }
    fileDes_ = open(audioUri_.c_str(), O_RDONLY);
    if (fileDes_ == -1) {
        MEDIA_LOGE("Prepare: Failed to open the audio uri for sound pool.");
        return MSERR_OPEN_FILE_FAILED;
    }
    std::string uri = "fd://" + std::to_string(fileDes_);

    int32_t soundID = soundPoolPlayer_->Load(uri);
    if (soundID < 0) {
        MEDIA_LOGE("Prepare: Failed to load soundPool uri.");
        return MSERR_OPEN_FILE_FAILED;
    }
    std::unique_lock<std::mutex> lockPrepare(prepareMutex_);
    prepareCond_.wait_for(lockPrepare, std::chrono::seconds(LOAD_WAIT_SECONDS),
        [this]() { return isPrepared_ || isReleased_ || isUnsupportedFile_; });
    CHECK_AND_RETURN_RET_LOG(!isReleased_, MSERR_INVALID_OPERATION,
        "The sound pool is released when it is preparing.");
    CHECK_AND_RETURN_RET_LOG(!isUnsupportedFile_, MSERR_UNSUPPORT_FILE,
        "Failed to load audio uri: report unsupported file err when loading source for sound pool.");
    CHECK_AND_RETURN_RET_LOG(isPrepared_, MSERR_OPEN_FILE_FAILED,
        "Failed to load audio uri: time out.");

    // The audio source has been loaded for sound pool
    soundID_ = soundID;
    configuredAudioUri_ = audioUri_;
    playerState_ = AudioHapticPlayerState::STATE_PREPARED;

    return MSERR_OK;
}

int32_t AudioHapticPlayerImpl::StartSoundPoolPlayer()
{
    MEDIA_LOGI("Enter StartSoundPoolPlayer()");
    std::lock_guard<std::mutex> lock(audioHapticPlayerLock_);
    if (playerState_ != AudioHapticPlayerState::STATE_PREPARED &&
        playerState_ != AudioHapticPlayerState::STATE_RUNNING &&
        playerState_ != AudioHapticPlayerState::STATE_STOPPED) {
        MEDIA_LOGE("SoundPoolPlayer not Prepared");
        return MSERR_START_FAILED;
    }
    if (vibrateThread_ == nullptr) {
        ResetVibrateState();
        vibrateThread_ = std::make_shared<std::thread>([this] { StartVibrate(); });
    }
    CHECK_AND_RETURN_RET_LOG(soundPoolPlayer_ != nullptr, MSERR_INVALID_STATE, "Sound pool player instance is null");

    PlayParams playParams {
        .loop = (loop_ ? -1 : 0),
        .rate = 0, // default AudioRendererRate::RENDER_RATE_NORMAL
        .leftVolume = volume_ * (muteAudio_ ? 0 : 1),
        .rightVolume = volume_ * (muteAudio_ ? 0 : 1),
        .priority = 0,
        .parallelPlayFlag = false,
    };
    streamID_ = soundPoolPlayer_->Play(soundID_, playParams);
    playerState_ = AudioHapticPlayerState::STATE_RUNNING;

    return MSERR_OK;
}

int32_t AudioHapticPlayerImpl::StopSoundPoolPlayer()
{
    MEDIA_LOGI("Enter StopSoundPoolPlayer()");
    std::lock_guard<std::mutex> lock(audioHapticPlayerLock_);
    CHECK_AND_RETURN_RET_LOG(soundPoolPlayer_ != nullptr, MSERR_INVALID_STATE, "Sound pool player instance is null");

    (void)soundPoolPlayer_->Stop(streamID_);
    StopVibrate();
    playerState_ = AudioHapticPlayerState::STATE_STOPPED;

    return MSERR_OK;
}

int32_t AudioHapticPlayerImpl::ReleaseSoundPoolPlayer()
{
    MEDIA_LOGI("Enter ReleaseSoundPoolPlayer()");
    {
        std::lock_guard<std::mutex> lockPrepare(prepareMutex_);
        isReleased_ = true;
        prepareCond_.notify_one();
    }
    std::lock_guard<std::mutex> lock(audioHapticPlayerLock_);
    CHECK_AND_RETURN_RET_LOG(playerState_ != AudioHapticPlayerState::STATE_RELEASED, MSERR_OK,
        "The audio haptic player has been released.");
    (void)audioHapticVibrator_->Release();
    {
        // When player is releasing，notify vibrate thread immediately
        std::lock_guard<std::mutex> lockVibrate(waitStartVibrateMutex_);
        isAudioPlayFirstFrame_ = true;
        isVibrationStopped_ = true;
        condStartVibrate_.notify_one();
    }
    if (vibrateThread_ != nullptr && vibrateThread_->joinable()) {
        vibrateThread_->join();
        vibrateThread_.reset();
    }

    CHECK_AND_RETURN_RET_LOG(soundPoolPlayer_ != nullptr, MSERR_INVALID_STATE, "Sound pool player instance is null");
    (void)soundPoolPlayer_->Release();
    soundPoolPlayer_ = nullptr;
    if (fileDes_ != -1) {
        (void)close(fileDes_);
        fileDes_ = -1;
    }
    soundPoolCallback_ = nullptr;

    playerState_ = AudioHapticPlayerState::STATE_RELEASED;

    return MSERR_OK;
}


bool AudioHapticPlayerImpl::IsMuted(const AudioHapticType &audioHapticType) const
{
    if (audioHapticType == AUDIO_HAPTIC_TYPE_AUDIO) {
        return muteAudio_;
    } else if (audioHapticType == AUDIO_HAPTIC_TYPE_HAPTIC) {
        return muteHaptic_;
    }
    MEDIA_LOGE("IsMuted: invalid audioHapticType %{public}d", audioHapticType);
    return false;
}

int32_t AudioHapticPlayerImpl::Prepare()
{
    int32_t result = MSERR_OK;
    if (playerType_ == AUDIO_HAPTIC_TYPE_NORMAL) {
        result = PrepareAVPlayer();
    } else if (playerType_ == AUDIO_HAPTIC_TYPE_FAST) {
        result = PrepareSoundPool();
    }
    CHECK_AND_RETURN_RET_LOG(result == MSERR_OK, result, "Failed to load audio file");

    result = PrepareVibrator();
    CHECK_AND_RETURN_RET_LOG(result == MSERR_OK, result, "Failed to load vobration file");
    return MSERR_OK;
}


int32_t AudioHapticPlayerImpl::Start()
{
    if (playerType_ == AUDIO_HAPTIC_TYPE_NORMAL) {
        StartAVPlayer();
    } else if (playerType_ == AUDIO_HAPTIC_TYPE_FAST) {
        StartSoundPoolPlayer();
    }
    return MSERR_OK;
}

int32_t AudioHapticPlayerImpl::Stop()
{
    if (playerType_ == AUDIO_HAPTIC_TYPE_NORMAL) {
        StopAVPlayer();
    } else if (playerType_ == AUDIO_HAPTIC_TYPE_FAST) {
        StopSoundPoolPlayer();
    }
    return MSERR_OK;
}

int32_t AudioHapticPlayerImpl::Release()
{
    if (playerType_ == AUDIO_HAPTIC_TYPE_NORMAL) {
        ReleaseAVPlayer();
    } else if (playerType_ == AUDIO_HAPTIC_TYPE_FAST) {
        ReleaseSoundPoolPlayer();
    }
    return MSERR_OK;
}

int32_t AudioHapticPlayerImpl::SetVolume(float volume)
{
    MEDIA_LOGI("AudioHapticPlayerImpl::SetVolume %{public}f", volume);
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
    if (playerType_ == AUDIO_HAPTIC_TYPE_NORMAL) {
        result = avPlayer_->SetVolume(actualVolume, actualVolume);
        // if the volume_ is 0.0f, stop vibrate for voice call.
        if (std::abs(volume_ - 0.0f) <= std::numeric_limits<float>::epsilon()) {
            StopVibrate();
        }
    } else if (playerType_ == AUDIO_HAPTIC_TYPE_FAST) {
        if (streamID_ != -1) {
            result = soundPoolPlayer_->SetVolume(streamID_, actualVolume, actualVolume);
        }
    }
    return result;
}

int32_t AudioHapticPlayerImpl::SetLoop(bool loop)
{
    MEDIA_LOGI("AudioHapticPlayerImpl::SetLoop %{public}d", loop);
    std::lock_guard<std::mutex> lock(audioHapticPlayerLock_);
    int32_t result = MSERR_OK;
    loop_ = loop;

    if (playerState_ != AudioHapticPlayerState::STATE_PREPARED &&
        playerState_ != AudioHapticPlayerState::STATE_RUNNING) {
        MEDIA_LOGI("Audio haptic player is not prepared or running. No need to modify player");
        return result;
    }
    if (playerType_ == AUDIO_HAPTIC_TYPE_NORMAL) {
        result = avPlayer_->SetLooping(loop_);
    } else if (playerType_ == AUDIO_HAPTIC_TYPE_FAST) {
        if (streamID_ != -1) {
            int32_t loopCount = loop_ ? -1 : 0;
            result = soundPoolPlayer_->SetLoop(streamID_, loopCount);
        }
    }
    return MSERR_OK;
}

int32_t AudioHapticPlayerImpl::SetAudioHapticPlayerCallback(
    const std::shared_ptr<AudioHapticPlayerCallback> &playerCallback)
{
    if (playerCallback == nullptr) {
        MEDIA_LOGE("The audio haptic player callback is nullptr.");
        return MSERR_INVALID_VAL;
    }

    std::lock_guard<std::mutex> lock(audioHapticPlayerLock_);
    audioHapticPlayerCallback_ = playerCallback;
    return MSERR_OK;
}

int32_t AudioHapticPlayerImpl::GetAudioCurrentTime()
{
    if (avPlayer_ == nullptr) {
        MEDIA_LOGE("GetAudioCurrentTime: avPlayer_ is nullptr. This function is only usable for avPlayer.");
        return -1;
    }
    int32_t currentTime = -1;
    (void)avPlayer_->GetCurrentTime(currentTime);
    return currentTime;
}

int32_t AudioHapticPlayerImpl::LoadAVPlayer()
{
    avPlayer_ = PlayerFactory::CreatePlayer();
    CHECK_AND_RETURN_RET_LOG(avPlayer_ != nullptr, MSERR_INVALID_VAL, "Failed to create AvPlayer instance");

    avPlayerCallback_ = std::make_shared<AudioHapticPlayerNativeCallback>(shared_from_this());
    CHECK_AND_RETURN_RET_LOG(avPlayerCallback_ != nullptr, MSERR_INVALID_VAL, "Failed to create callback object");

    avPlayer_->SetPlayerCallback(avPlayerCallback_);
    playerState_ = AudioHapticPlayerState::STATE_NEW;
    configuredAudioUri_ = "";

    return MSERR_OK;
}

int32_t AudioHapticPlayerImpl::PrepareAVPlayer()
{
    MEDIA_LOGI("PrepareAVPlayer");
    std::lock_guard<std::mutex> lock(audioHapticPlayerLock_);
    CHECK_AND_RETURN_RET_LOG(avPlayer_ != nullptr, MSERR_INVALID_VAL, "Audio haptic player instance is null");
    CHECK_AND_RETURN_RET_LOG(!audioUri_.empty(), MSERR_OPEN_FILE_FAILED, "The audio uri is empty");
    if (audioUri_ != configuredAudioUri_) {
        return ResetAVPlayer();
    }
    return MSERR_OK;
}

int32_t AudioHapticPlayerImpl::ResetAVPlayer()
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


int32_t AudioHapticPlayerImpl::StartAVPlayer()
{
    MEDIA_LOGI("StartAVPlayer");
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
    if (vibrateThread_ == nullptr) {
        ResetVibrateState();
        vibrateThread_ = std::make_shared<std::thread>([this] { StartVibrate(); });
    }

    auto ret = avPlayer_->Play();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_START_FAILED, "Start failed %{public}d", ret);

    playerState_ = AudioHapticPlayerState::STATE_RUNNING;
    return MSERR_OK;
}

int32_t AudioHapticPlayerImpl::StopAVPlayer()
{
    MEDIA_LOGI("StopAVPlayer");
    std::lock_guard<std::mutex> lock(audioHapticPlayerLock_);
    CHECK_AND_RETURN_RET_LOG(avPlayer_ != nullptr && playerState_ != AudioHapticPlayerState::STATE_INVALID,
        MSERR_INVALID_VAL, "StopAVPlayer: no available AVPlayer_");

    if (playerState_ != AudioHapticPlayerState::STATE_STOPPED) {
        (void)avPlayer_->Stop();
        StopVibrate();
    }

    playerState_ = AudioHapticPlayerState::STATE_STOPPED;

    return MSERR_OK;
}

int32_t AudioHapticPlayerImpl::ReleaseAVPlayer()
{
    MEDIA_LOGI("ReleaseAVPlayer");
    {
        std::lock_guard<std::mutex> lockPrepare(prepareMutex_);
        isReleased_ = true;
        prepareCond_.notify_one();
    }
    std::lock_guard<std::mutex> lock(audioHapticPlayerLock_);
    CHECK_AND_RETURN_RET_LOG(playerState_ != AudioHapticPlayerState::STATE_RELEASED, MSERR_OK,
        "The audio haptic player has been released.");

    audioHapticVibrator_->Release();
    {
        // When player is releasing，notify vibrate thread immediately
        std::lock_guard<std::mutex> lockVibrate(waitStartVibrateMutex_);
        isAudioPlayFirstFrame_ = true;
        isVibrationStopped_ = true;
        condStartVibrate_.notify_one();
    }
    if (vibrateThread_ != nullptr && vibrateThread_->joinable()) {
        vibrateThread_->join();
        vibrateThread_.reset();
    }

    if (avPlayer_ != nullptr) {
        (void)avPlayer_->Release();
        avPlayer_ = nullptr;
    }
    avPlayerCallback_ = nullptr;
    if (fileDes_ != -1) {
        (void)close(fileDes_);
        fileDes_ = -1;
    }

    playerState_ = AudioHapticPlayerState::STATE_RELEASED;
    return MSERR_OK;
}


void AudioHapticPlayerImpl::SetAVPlayerState(AudioHapticPlayerState playerState)
{
    MEDIA_LOGI("SetAVPlayerState, state %{public}d", playerState);
    playerState_ = playerState;
}

void AudioHapticPlayerImpl::NotifyPreparedEvent()
{
    std::lock_guard<std::mutex> lockPrepare(prepareMutex_);
    isPrepared_ = true;
    prepareCond_.notify_one();
}

void AudioHapticPlayerImpl::NotifyUnsupportedFileEvent()
{
    std::lock_guard<std::mutex> lockPrepare(prepareMutex_);
    isUnsupportedFile_ = true;
    prepareCond_.notify_one();
}

void AudioHapticPlayerImpl::NotifyInterruptEvent(AudioStandard::InterruptEvent &interruptEvent)
{
    std::shared_ptr<AudioHapticPlayerCallback> cb = audioHapticPlayerCallback_.lock();
    if (cb != nullptr) {
        MEDIA_LOGI("NotifyInterruptEvent for napi object");
        cb->OnInterrupt(interruptEvent);
    } else {
        MEDIA_LOGE("NotifyInterruptEvent: audioHapticPlayerCallback_ is nullptr");
    }
}

void AudioHapticPlayerImpl::NotifyEndOfStreamEvent()
{
    MEDIA_LOGI("NotifyEndOfStreamEvent");
    StopVibrate();
    playerState_ = AudioHapticPlayerState::STATE_STOPPED;
    std::shared_ptr<AudioHapticPlayerCallback> cb = audioHapticPlayerCallback_.lock();
    if (cb != nullptr) {
        MEDIA_LOGI("NotifyEndOfStreamEvent for napi object");
        cb->OnEndOfStream();
    } else {
        MEDIA_LOGE("NotifyEndOfStreamEvent: audioHapticPlayerCallback_ is nullptr");
    }
}

void AudioHapticPlayerImpl::NotifyStartVibrate(uint64_t latency)
{
    std::lock_guard<std::mutex> lock(this->waitStartVibrateMutex_);
    this->isAudioPlayFirstFrame_ = true;
    this->audioLatency_ = latency;
    this->condStartVibrate_.notify_one();
}

// Callback class symbols
AudioHapticPlayerNativeCallback::AudioHapticPlayerNativeCallback(
    std::shared_ptr<AudioHapticPlayerImpl> audioHapticPlayerImpl)
    : audioHapticPlayerImpl_(audioHapticPlayerImpl) {}

// SoundPool callback
void AudioHapticPlayerNativeCallback::OnLoadCompleted(int32_t soundId)
{
    MEDIA_LOGI("OnLoadCompleted reported from sound pool.");
    std::shared_ptr<AudioHapticPlayerImpl> player = audioHapticPlayerImpl_.lock();
    if (player == nullptr) {
        MEDIA_LOGE("The audio haptic player has been released.");
        return;
    }
    player->NotifyPreparedEvent();
}

void AudioHapticPlayerNativeCallback::OnPlayFinished()
{
    MEDIA_LOGI("OnPlayFinished reported from sound pool.");
    std::shared_ptr<AudioHapticPlayerImpl> player = audioHapticPlayerImpl_.lock();
    if (player == nullptr) {
        MEDIA_LOGE("The audio haptic player has been released.");
        return;
    }
    player->NotifyEndOfStreamEvent();
}

void AudioHapticPlayerNativeCallback::OnError(int32_t errorCode)
{
    MEDIA_LOGE("OnError reported from sound pool: %{public}d", errorCode);
    MediaServiceErrCode mediaErr = static_cast<MediaServiceErrCode>(errorCode);
    if (mediaErr == MSERR_UNSUPPORT_FILE) {
        std::shared_ptr<AudioHapticPlayerImpl> player = audioHapticPlayerImpl_.lock();
        if (player == nullptr) {
            MEDIA_LOGE("The audio haptic player has been released.");
            return;
        }
        player->NotifyUnsupportedFileEvent();
    }
}

// AVPlayer callback
void AudioHapticPlayerNativeCallback::OnError(int32_t errorCode, const std::string &errorMsg)
{
    MEDIA_LOGE("OnError reported from AVPlayer: %{public}d", errorCode);
    MediaServiceErrCode mediaErr = static_cast<MediaServiceErrCode>(errorCode);
    if (mediaErr == MSERR_UNSUPPORT_FILE) {
        std::shared_ptr<AudioHapticPlayerImpl> player = audioHapticPlayerImpl_.lock();
        if (player == nullptr) {
            MEDIA_LOGE("The audio haptic player has been released.");
            return;
        }
        player->NotifyUnsupportedFileEvent();
    }
}

void AudioHapticPlayerNativeCallback::OnInfo(Media::PlayerOnInfoType type, int32_t extra, const Media::Format &infoBody)
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

void AudioHapticPlayerNativeCallback::HandleStateChangeEvent(int32_t extra, const Format &infoBody)
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
    std::shared_ptr<AudioHapticPlayerImpl> player = audioHapticPlayerImpl_.lock();
    if (player == nullptr) {
        MEDIA_LOGE("The audio haptic player has been released.");
        return;
    }
    player->SetAVPlayerState(playerState_);

    if (avPlayerState == PLAYER_PREPARED) {
        player->NotifyPreparedEvent();
    } else if (avPlayerState == PLAYER_PLAYBACK_COMPLETE) {
        player->NotifyEndOfStreamEvent();
    }
}

void AudioHapticPlayerNativeCallback::HandleAudioInterruptEvent(int32_t extra, const Format &infoBody)
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

    std::shared_ptr<AudioHapticPlayerImpl> player = audioHapticPlayerImpl_.lock();
    if (player == nullptr) {
        MEDIA_LOGE("The audio haptic player has been released.");
        return;
    }
    player->NotifyInterruptEvent(interruptEvent);
}

void AudioHapticPlayerNativeCallback::HandleAudioFirstFrameEvent(int32_t extra, const Format &infoBody)
{
    int64_t value = 0;
    (void)infoBody.GetLongValue(PlayerKeys::AUDIO_FIRST_FRAME, value);
    uint64_t latency = static_cast<uint64_t>(value);
    MEDIA_LOGI("HandleAudioFirstFrameEvent from AVPlayer. Latency %{public}" PRIu64 "", latency);
    std::shared_ptr<AudioHapticPlayerImpl> player = audioHapticPlayerImpl_.lock();
    if (player == nullptr) {
        MEDIA_LOGE("The audio haptic player has been released.");
        return;
    }
    player->NotifyStartVibrate(latency);
}

AudioHapticFirstFrameCb::AudioHapticFirstFrameCb(std::shared_ptr<AudioHapticPlayerImpl> audioHapticPlayerImpl)
    : audioHapticPlayerImpl_(audioHapticPlayerImpl) {}

void AudioHapticFirstFrameCb::OnFirstAudioFrameWritingCallback(uint64_t &latency)
{
    MEDIA_LOGI("OnFirstAudioFrameWritingCallback from Soundpool. Latency %{public}" PRIu64 "", latency);
    std::shared_ptr<AudioHapticPlayerImpl> player = audioHapticPlayerImpl_.lock();
    if (player == nullptr) {
        MEDIA_LOGE("The audio haptic player has been released.");
        return;
    }
    player->NotifyStartVibrate(latency);
}
} // namesapce AudioStandard
} // namespace OHOS
