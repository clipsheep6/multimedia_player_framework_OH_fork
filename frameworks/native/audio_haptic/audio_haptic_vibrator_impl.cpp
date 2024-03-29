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

#include "audio_haptic_vibrator_impl.h"

#include <fcntl.h>
#include <sys/stat.h>

#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioHapticVibratorImpl"};
#ifdef SUPPORT_VIBRATOR
constexpr int32_t MIN_WAITING_TIME_FOR_VIBRATOR = 1200; // ms
constexpr uint64_t MILLISECONDS_FOR_ONE_SECOND = 1000; // ms
constexpr int32_t MAX_WAITING_LOOP_COUNT = 10;
#endif
}

namespace OHOS {
namespace Media {

AudioHapticVibratorImpl::AudioHapticVibratorImpl(AudioHapticPlayer &audioHapticPlayer)
    : audioHapticPlayer_(audioHapticPlayer)
{
    if (audioHapticPlayer_.IsMuted(AUDIO_HAPTIC_TYPE_HAPTIC)) {
        MEDIA_LOGW("The muteHaptic value of audioHapticPlayer_ is true. No need to vibrate.");
    }
}

AudioHapticVibratorImpl::~AudioHapticVibratorImpl() {}

std::mutex AudioHapticVibrator::createVibratorMutex_;

std::shared_ptr<AudioHapticVibrator> AudioHapticVibrator::CreateAudioHapticVibrator(
    AudioHapticPlayer &audioHapticPlayer)
{
    std::lock_guard<std::mutex> lock(createVibratorMutex_);
    auto audioHapticVibrator = std::make_shared<AudioHapticVibratorImpl>(audioHapticPlayer);
    return audioHapticVibrator;
}

int32_t AudioHapticVibratorImpl::PreLoad(const std::string &hapticUri)
{
#ifdef SUPPORT_VIBRATOR
    MEDIA_LOGI("PreLoad vibration source. hapticUri [%{public}s]", hapticUri.c_str());
    int32_t fd = open(hapticUri.c_str(), O_RDONLY);
    if (fd == -1) {
        // open file failed, return.
        return MSERR_OPEN_FILE_FAILED;
    }
    vibratorFD_ = std::make_shared<VibratorFileDescription>();
    vibratorPkg_ = std::make_shared<VibratorPackage>();

    struct stat64 statbuf = { 0 };
    if (fstat64(fd, &statbuf) == 0) {
        vibratorFD_->fd = fd;
        vibratorFD_->offset = 0;
        vibratorFD_->length = statbuf.st_size;
    } else {
        return MSERR_OPEN_FILE_FAILED;
    }

    int32_t result = Sensors::PreProcess(*vibratorFD_, *vibratorPkg_);
    if (result != 0) {
        MEDIA_LOGE("PreProcess: %{public}d", result);
        return MSERR_UNSUPPORT_FILE;
    }
#endif
    return MSERR_OK;
}

int32_t AudioHapticVibratorImpl::Release()
{
    std::lock_guard<std::mutex> lock(vibrateMutex_);
    isStopped_ = true;
#ifdef SUPPORT_VIBRATOR
    int32_t result = Sensors::Cancel();
    if (result != 0) {
        MEDIA_LOGE("Failed to stop vibrator: result %{public}d", result);
    }
    vibrateCV_.notify_one();

    if (vibratorPkg_ != nullptr) {
        Sensors::FreeVibratorPackage(*vibratorPkg_);
        vibratorPkg_ = nullptr;
    }
    vibratorFD_ = nullptr;

#endif
    return MSERR_OK;
}

void AudioHapticVibratorImpl::ResetStopState()
{
    std::lock_guard<std::mutex> lock(vibrateMutex_);
    isStopped_ = false;
}

int32_t AudioHapticVibratorImpl::StartVibrate(const AudioLatencyMode &latencyMode)
{
    MEDIA_LOGD("StartVibrate: for latency mode %{public}d", latencyMode);
    if (latencyMode == AUDIO_LATENCY_MODE_NORMAL) {
        return StartVibrateForAVPlayer();
    } else if (latencyMode == AUDIO_LATENCY_MODE_FAST) {
        return StartVibrateForSoundPool();
    } else {
        return MSERR_INVALID_OPERATION;
    }
}

int32_t AudioHapticVibratorImpl::StartVibrateForSoundPool()
{
    std::unique_lock<std::mutex> lock(vibrateMutex_);
    if (isStopped_) {
        MEDIA_LOGW("Vibrator has been stopped. Return ok immediately");
        return MSERR_OK;
    }

    int32_t result = MSERR_OK;
#ifdef SUPPORT_VIBRATOR
    if (vibratorPkg_ == nullptr || vibratorFD_ == nullptr) {
        MEDIA_LOGE("Vibration source file is not prepared. Can not start vibrating");
        return MSERR_INVALID_OPERATION;
    }
    int32_t vibrateTime = 0; // record the pattern time which has been played
    for (int32_t i = 0; i < vibratorPkg_->patternNum; ++i) {
        int32_t patternTime = vibratorPkg_->patterns[i].time - vibrateTime; // calculate the time of single pattern
        vibrateTime = vibratorPkg_->patterns[i].time;
        (void)vibrateCV_.wait_for(lock, std::chrono::milliseconds(patternTime),
            [this]() { return isStopped_; });
        CHECK_AND_RETURN_RET_LOG(!isStopped_, result,
            "StartVibrateForSoundPool: Stop() is call when waiting");
        result = Sensors::PlayPattern(vibratorPkg_->patterns[i]);
        if (result != 0) {
            MEDIA_LOGE("StartVibrateForSoundPool: PlayPattern error %{public}d", result);
            return result;
        }
    }
#endif
    return result;
}

int32_t AudioHapticVibratorImpl::StartVibrateForAVPlayer()
{
    std::unique_lock<std::mutex> lock(vibrateMutex_);
    if (isStopped_) {
        MEDIA_LOGW("Vibrator has been stopped. Return ok immediately");
        return MSERR_OK;
    }

    int32_t result = MSERR_OK;
#ifdef SUPPORT_VIBRATOR
    if (vibratorPkg_ == nullptr || vibratorFD_ == nullptr) {
        MEDIA_LOGE("Vibration source file is not prepared. Can not start vibrating");
        return MSERR_INVALID_OPERATION;
    }
    int32_t vibrateTime = 0; // record the pattern time which has been played
    for (int32_t i = 0; i < vibratorPkg_->patternNum; ++i) {
        // the delay time of first frame has been handled in audio haptic player
        int32_t patternTime = vibratorPkg_->patterns[i].time - vibrateTime; // calculate the time of single pattern
        (void)vibrateCV_.wait_for(lock, std::chrono::milliseconds(patternTime),
            [this]() { return isStopped_; });
        CHECK_AND_RETURN_RET_LOG(!isStopped_, result,
            "StartVibrateForAVPlayer: Stop() is call when waiting");
        result = Sensors::PlayPattern(vibratorPkg_->patterns[i]);
        CHECK_AND_RETURN_RET_LOG(result == 0, result,
            "StartVibrateForAVPlayer: PlayPattern error %{public}d", result);

        // get the audio time every second and handle the delay time
        if (i + 1 >= vibratorPkg_->patternNum) {
            // the last pattern has been played, break.
            break;
        }
        int32_t nextVibratorTime = vibratorPkg_->patterns[i + 1].time;
        vibrateTime = audioHapticPlayer_.GetAudioCurrentTime() + GetDelayTime();
        int32_t count = 0;
        while (nextVibratorTime - vibrateTime > MIN_WAITING_TIME_FOR_VIBRATOR && count < MAX_WAITING_LOOP_COUNT) {
            (void)vibrateCV_.wait_for(lock, std::chrono::milliseconds(MILLISECONDS_FOR_ONE_SECOND),
                [this]() { return isStopped_; });
            CHECK_AND_RETURN_RET_LOG(!isStopped_, result,
                "StartVibrateForAVPlayer: Stop() is call when waiting");
            vibrateTime = audioHapticPlayer_.GetAudioCurrentTime() + GetDelayTime();
            count++;
        }
        if (count == MAX_WAITING_LOOP_COUNT) {
            MEDIA_LOGE("StartVibrateForAVPlayer: loop count has reached the max value.");
            return MSERR_INVALID_OPERATION;
        }
    }
#endif
    return result;
}

int32_t AudioHapticVibratorImpl::StopVibrate()
{
    std::lock_guard<std::mutex> lock(vibrateMutex_);
    isStopped_ = true;
    int32_t result = MSERR_OK;
#ifdef SUPPORT_VIBRATOR
    vibrateCV_.notify_one();
    result = Sensors::Cancel();
    MEDIA_LOGI("StopVibrate: %{public}d", result);
#endif
    return result;
}

int32_t AudioHapticVibratorImpl::GetDelayTime()
{
    int32_t delayTime = 0;
#ifdef SUPPORT_VIBRATOR
    (void)Sensors::GetDelayTime(delayTime);
#endif
    return delayTime;
}
} // namesapce Media
} // namespace OHOS