/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "audio_capturer_wrapper.h"

#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ScreenCaptureACW"};
}

namespace OHOS {
namespace Media {
void AudioCapturerCallbackImpl::OnInterrupt(const InterruptEvent &interruptEvent)
{
    MEDIA_LOGI("AudioCapturerCallbackImpl OnInterrupt hintType:%{public}d, eventType:%{public}d, forceType:%{public}d",
        interruptEvent.hintType, interruptEvent.eventType, interruptEvent.forceType);
}

void AudioCapturerCallbackImpl::OnStateChange(const CapturerState state)
{
    MEDIA_LOGI("AudioCapturerCallbackImpl OnStateChange state:%{public}d", state);
    switch (state) {
        case CAPTURER_PREPARED:
            MEDIA_LOGD("AudioCapturerCallbackImpl OnStateChange CAPTURER_PREPARED");
            break;
        default:
            MEDIA_LOGD("AudioCapturerCallbackImpl OnStateChange NOT A VALID state");
            break;
    }
}

int32_t AudioCapturerWrapper::Start(const OHOS::AudioStandard::AppInfo &appInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (isRunning_.load()) {
        MEDIA_LOGE("Start failed, is running, threadName:%{public}s", threadName_.c_str());
        return MSERR_UNKNOWN;
    }

    std::shared_ptr<AudioCapturer> audioCapturer = CreateAudioCapturer(appInfo);
    CHECK_AND_RETURN_RET_LOG(audioCapturer != nullptr, MSERR_UNKNOWN, "Start failed, create AudioCapturer failed");
    if (!audioCapturer->Start()) {
        MEDIA_LOGE("Start failed, AudioCapturer Start failed, threadName:%{public}s", threadName_.c_str());
        audioCapturer->Release();
        audioCapturer = nullptr;
        OnStartFailed(ScreenCaptureErrorType::SCREEN_CAPTURE_ERROR_INTERNAL, SCREEN_CAPTURE_ERR_UNKNOWN);
        return MSERR_UNKNOWN;
    }
    MEDIA_LOGI("0x%{public}06" PRIXPTR "Start success, threadName:%{public}s", FAKE_POINTER(this), threadName_.c_str());

    isRunning_.store(true);
    readAudioLoop_ = std::make_unique<std::thread>(&AudioCapturerWrapper::CaptureAudio, this);
    audioCapturer_ = audioCapturer;
    return MSERR_OK;
}

int32_t AudioCapturerWrapper::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Stop S, threadName:%{public}s", FAKE_POINTER(this), threadName_.c_str());
    if (isRunning_.load()) {
        isRunning_.store(false);
        if (readAudioLoop_ != nullptr && readAudioLoop_->joinable()) {
            readAudioLoop_->join();
            readAudioLoop_.reset();
            readAudioLoop_ = nullptr;
        }
        if (audioCapturer_ != nullptr) {
            audioCapturer_->Stop();
            audioCapturer_->Release();
            audioCapturer_ = nullptr;
        }
    }

    std::unique_lock<std::mutex> bufferLock(bufferMutex_);
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Stop pop, threadName:%{public}s", FAKE_POINTER(this), threadName_.c_str());
    while (!availBuffers_.empty()) {
        if (availBuffers_.front() != nullptr) {
            free(availBuffers_.front()->buffer);
            availBuffers_.front()->buffer = nullptr;
        }
        availBuffers_.pop();
    }
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Stop E, threadName:%{public}s", FAKE_POINTER(this), threadName_.c_str());
    return MSERR_OK;
}

void AudioCapturerWrapper::SetIsMuted(bool isMuted)
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " SetIsMuted isMuted:%{public}d", FAKE_POINTER(this), isMuted);
    isMuted_.store(isMuted);
}

void AudioCapturerWrapper::SetInnerStreamUsage(std::vector<OHOS::AudioStandard::StreamUsage> &usages)
{
    // If do not call this function, the audio framework use MUSIC/MOVIE/GAME/AUDIOBOOK
    usages.push_back(OHOS::AudioStandard::StreamUsage::STREAM_USAGE_MUSIC);
    usages.push_back(OHOS::AudioStandard::StreamUsage::STREAM_USAGE_ALARM);
    usages.push_back(OHOS::AudioStandard::StreamUsage::STREAM_USAGE_MOVIE);
    usages.push_back(OHOS::AudioStandard::StreamUsage::STREAM_USAGE_GAME);
    usages.push_back(OHOS::AudioStandard::StreamUsage::STREAM_USAGE_AUDIOBOOK);
    usages.push_back(OHOS::AudioStandard::StreamUsage::STREAM_USAGE_NAVIGATION);
    usages.push_back(OHOS::AudioStandard::StreamUsage::STREAM_USAGE_UNKNOWN);
}

std::shared_ptr<AudioCapturer> AudioCapturerWrapper::CreateAudioCapturer(const OHOS::AudioStandard::AppInfo &appInfo)
{
    AudioCapturerOptions capturerOptions;
    capturerOptions.streamInfo.samplingRate = static_cast<AudioSamplingRate>(audioInfo_.audioSampleRate);
    capturerOptions.streamInfo.channels = static_cast<AudioChannel>(audioInfo_.audioChannels);
    capturerOptions.streamInfo.encoding = AudioEncodingType::ENCODING_PCM;
    capturerOptions.streamInfo.format = AudioSampleFormat::SAMPLE_S16LE;
    if (audioInfo_.audioSource == AudioCaptureSourceType::SOURCE_DEFAULT ||
        audioInfo_.audioSource == AudioCaptureSourceType::MIC) {
        capturerOptions.capturerInfo.sourceType = SourceType::SOURCE_TYPE_MIC; // Audio Source Type Mic is 0
    } else if (audioInfo_.audioSource == AudioCaptureSourceType::ALL_PLAYBACK ||
        audioInfo_.audioSource == AudioCaptureSourceType::APP_PLAYBACK) {
        capturerOptions.capturerInfo.sourceType = SourceType::SOURCE_TYPE_PLAYBACK_CAPTURE;
        SetInnerStreamUsage(capturerOptions.playbackCaptureConfig.filterOptions.usages);
    }
    if (contentFilter_.filteredAudioContents.find(
        AVScreenCaptureFilterableAudioContent::SCREEN_CAPTURE_NOTIFICATION_AUDIO) ==
        contentFilter_.filteredAudioContents.end()) {
        capturerOptions.playbackCaptureConfig.filterOptions.usages.push_back(
            OHOS::AudioStandard::StreamUsage::STREAM_USAGE_NOTIFICATION);
    }
    if (contentFilter_.filteredAudioContents.find(
        AVScreenCaptureFilterableAudioContent::SCREEN_CAPTURE_CURRENT_APP_AUDIO) !=
        contentFilter_.filteredAudioContents.end()) {
        capturerOptions.playbackCaptureConfig.filterOptions.pids.push_back(appInfo.appPid);
        capturerOptions.playbackCaptureConfig.filterOptions.pidFilterMode =
            OHOS::AudioStandard::FilterMode::EXCLUDE;
        MEDIA_LOGI("createAudioCapturer exclude current app audio");
    }
    capturerOptions.capturerInfo.capturerFlags = 0;
    std::shared_ptr<AudioCapturer> audioCapturer = AudioCapturer::Create(capturerOptions, appInfo);
    CHECK_AND_RETURN_RET_LOG(audioCapturer != nullptr, nullptr, "AudioCapturer::Create failed");
    std::shared_ptr<AudioCapturerCallbackImpl> callback = std::make_shared<AudioCapturerCallbackImpl>();
    int ret = audioCapturer->SetCapturerCallback(callback);
    if (ret != MSERR_OK) {
        audioCapturer->Release();
        MEDIA_LOGE("SetCapturerCallback failed, threadName:%{public}s", threadName_.c_str());
        return nullptr;
    }
    audioCaptureCallback_ = callback;
    return audioCapturer;
}

int32_t AudioCapturerWrapper::CaptureAudio()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " CaptureAudio S, name:%{public}s", FAKE_POINTER(this), threadName_.c_str());
    std::string name = threadName_.substr(0, std::min(threadName_.size(), static_cast<size_t>(MAX_THREAD_NAME_LENGTH)));
    pthread_setname_np(pthread_self(), name.c_str());

    size_t bufferLen;
    CHECK_AND_RETURN_RET_LOG(audioCapturer_ != nullptr && audioCapturer_->GetBufferSize(bufferLen) >= 0,
        MSERR_NO_MEMORY, "CaptureAudio GetBufferSize failed");

    Timestamp timestamp;
    std::shared_ptr<AudioBuffer> audioBuffer;
    while (true) {
        CHECK_AND_RETURN_RET_LOG(isRunning_.load(), MSERR_OK, "CaptureAudio is not running, stop capture");
        uint8_t *buffer = static_cast<uint8_t *>(malloc(bufferLen));
        CHECK_AND_RETURN_RET_LOG(buffer != nullptr, MSERR_OK, "CaptureAudio buffer is no momery, stop capture");
        audioBuffer = std::make_shared<AudioBuffer>(buffer, 0, 0, audioInfo_.audioSource);
        memset_s(audioBuffer->buffer, bufferLen, 0, bufferLen);
        int32_t bufferRead = audioCapturer_->Read(*(audioBuffer->buffer), bufferLen, true);
        CHECK_AND_CONTINUE_LOG(bufferRead > 0, "CaptureAudio read audio buffer failed, continue");
        audioBuffer->length = bufferRead;
        audioCapturer_->GetAudioTime(timestamp, Timestamp::Timestampbase::MONOTONIC);
        int64_t audioTime = timestamp.time.tv_nsec + timestamp.time.tv_sec * SEC_TO_NANOSECOND;
        audioBuffer->timestamp = audioTime;
        {
            std::unique_lock<std::mutex> lock(bufferMutex_);
            CHECK_AND_RETURN_RET_LOG(isRunning_.load(), MSERR_OK, "CaptureAudio is not running, ignore and stop");
            CHECK_AND_CONTINUE_LOG(availBuffers_.size() <= MAX_AUDIO_BUFFER_SIZE, "consume slow, drop audio frame");
            if (isMuted_) {
                memset_s(audioBuffer->buffer, bufferLen, 0, bufferLen);
            }
            availBuffers_.push(audioBuffer);
        }
        bufferCond_.notify_all();
        CHECK_AND_RETURN_RET_LOG(isRunning_.load(), MSERR_OK, "CaptureAudio is not running, ignore and stop");
        CHECK_AND_RETURN_RET_LOG(screenCaptureCb_ != nullptr, MSERR_OK, "no consumer, will drop audio frame");
        screenCaptureCb_->OnAudioBufferAvailable(true, audioInfo_.audioSource);
    }
    MEDIA_LOGI("0x%{public}06" PRIXPTR " CaptureAudio E, name:%{public}s", FAKE_POINTER(this), threadName_.c_str());
    return MSERR_OK;
}

int32_t AudioCapturerWrapper::AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audioBuffer)
{
    using namespace std::chrono_literals;
    std::unique_lock<std::mutex> lock(bufferMutex_);
    CHECK_AND_RETURN_RET_LOG(isRunning_.load(), MSERR_UNKNOWN, "AcquireAudioBuffer failed, not running");
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Acquire Buffer S, name:%{public}s", FAKE_POINTER(this), threadName_.c_str());

    if (!bufferCond_.wait_for(
        lock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] { return !availBuffers_.empty(); })) {
        MEDIA_LOGE("AcquireAudioBuffer timeout, threadName:%{public}s", threadName_.c_str());
        return MSERR_UNKNOWN;
    }
    audioBuffer = availBuffers_.front();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Acquire Buffer E, name:%{public}s", FAKE_POINTER(this), threadName_.c_str());
    return MSERR_OK;
}

int32_t AudioCapturerWrapper::GetBufferSize(size_t &size)
{
    using namespace std::chrono_literals;
    std::unique_lock<std::mutex> lock(bufferMutex_);
    MEDIA_LOGD("0x%{public}06" PRIXPTR " GetBufferSize Buffer S, name:%{public}s",
        FAKE_POINTER(this), threadName_.c_str());
    CHECK_AND_RETURN_RET_LOG(isRunning_.load(), MSERR_UNKNOWN, "GetBufferSize failed, not running");
    CHECK_AND_RETURN_RET_LOG(audioCapturer_ != nullptr && audioCapturer_->GetBufferSize(size) >= 0,
        MSERR_NO_MEMORY, "CaptureAudio GetBufferSize failed");
    MEDIA_LOGD("0x%{public}06" PRIXPTR " GetBufferSize Buffer E, name:%{public}s",
        FAKE_POINTER(this), threadName_.c_str());
    return MSERR_OK;
}

int32_t AudioCapturerWrapper::ReleaseAudioBuffer()
{
    using namespace std::chrono_literals;
    std::unique_lock<std::mutex> lock(bufferMutex_);
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Release Buffer S, name:%{public}s", FAKE_POINTER(this), threadName_.c_str());
    CHECK_AND_RETURN_RET_LOG(isRunning_.load(), MSERR_UNKNOWN, "ReleaseAudioBuffer failed, not running");
    CHECK_AND_RETURN_RET_LOG(!availBuffers_.empty(), MSERR_UNKNOWN, "ReleaseAudioBuffer failed, no frame to release");
    availBuffers_.pop();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Release Buffer E, name:%{public}s", FAKE_POINTER(this), threadName_.c_str());
    return MSERR_OK;
}

void AudioCapturerWrapper::OnStartFailed(ScreenCaptureErrorType errorType, int32_t errorCode)
{
    if (screenCaptureCb_ != nullptr) {
        screenCaptureCb_->OnError(errorType, errorCode);
    }
}

AudioCapturerWrapper::~AudioCapturerWrapper()
{
    Stop();
}

void MicAudioCapturerWrapper::OnStartFailed(ScreenCaptureErrorType errorType, int32_t errorCode)
{
    (void)errorType;
    (void)errorCode;
    if (screenCaptureCb_ != nullptr) {
        screenCaptureCb_->OnStateChange(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_MIC_UNAVAILABLE);
    }
}
} // namespace Media
} // namespace OHOS