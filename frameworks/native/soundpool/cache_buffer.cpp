
/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "cache_buffer.h"
#include "media_log.h"
#include "media_errors.h"

namespace OHOS {
namespace Media {
CacheBuffer::CacheBuffer(const MediaAVCodec::Format trackFormat,
    const std::deque<std::shared_ptr<AudioBufferEntry>> cacheData,
    const int32_t soundID, const int32_t streamID) : trackFormat_(trackFormat),
    cacheData_(cacheData), soundID_(soundID), streamID_(streamID)
{
    MEDIA_INFO_LOG("Construction CacheBuffer");
}

CacheBuffer::~CacheBuffer()
{
    MEDIA_INFO_LOG("Destruction CacheBuffer dec");
    Release();
}

std::unique_ptr<AudioStandard::AudioRenderer> CacheBuffer::CreateAudioRenderer(const int32_t streamID,
    const AudioStandard::AudioRendererInfo audioRendererInfo, const PlayParams playParams)
{
    MEDIA_INFO_LOG("CacheBuffer");
    CHECK_AND_RETURN_RET_LOG(streamID == streamID_, nullptr,
        "Invalid streamID, failed to create normal audioRenderer.");
    int32_t sampleRate;
    int32_t sampleFormat;
    int32_t channelCount;
    AudioStandard::AudioRendererOptions rendererOptions = {};
    // Set to PCM encoding
    rendererOptions.streamInfo.encoding = AudioStandard::AudioEncodingType::ENCODING_PCM;
    // Get sample rate from trackFormat and set it to audiorender.
    trackFormat_.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sampleRate);
    rendererOptions.streamInfo.samplingRate = static_cast<AudioStandard::AudioSamplingRate>(sampleRate);
    // Get sample format from trackFormat and set it to audiorender.
    trackFormat_.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, sampleFormat);
    // Align audiorender capability
    rendererOptions.streamInfo.format = static_cast<AudioStandard::AudioSampleFormat>(sampleFormat);
    // Get channel count from trackFormat and set it to audiorender.
    trackFormat_.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, channelCount);
    rendererOptions.streamInfo.channels = static_cast<AudioStandard::AudioChannel>(channelCount);
    // contentType streamUsage rendererFlags come from user.
    rendererOptions.rendererInfo.contentType = audioRendererInfo.contentType;
    rendererOptions.rendererInfo.streamUsage = audioRendererInfo.streamUsage;
    rendererOptions.privacyType = AudioStandard::PRIVACY_TYPE_PUBLIC;
    std::string cacheDir = "/data/storage/el2/base/temp";
    if (playParams.cacheDir != "") {
        cacheDir = playParams.cacheDir;
    }

    // low-latency:1, non low-latency:0
    rendererOptions.rendererInfo.rendererFlags = audioRendererInfo.rendererFlags;
    std::unique_ptr<AudioStandard::AudioRenderer> audioRenderer =
        AudioStandard::AudioRenderer::Create(cacheDir, rendererOptions);
    MEDIA_INFO_LOG("CacheBuffer rendererFlags:%{public}d", audioRendererInfo.rendererFlags);
    return audioRenderer;
}

int32_t CacheBuffer::PreparePlay(const int32_t streamID, const AudioStandard::AudioRendererInfo audioRendererInfo,
    const PlayParams playParams)
{
    MEDIA_INFO_LOG("CacheBuffer should prepare before play");
    // create audioRenderer
    if (audioRenderer_ == nullptr) {
        audioRenderer_ = CreateAudioRenderer(streamID, audioRendererInfo, playParams);
    } else {
        MEDIA_INFO_LOG("audio render inited.");
    }
    // deal play params
    DealPlayParamsBeforePlay(streamID, playParams);
    return MSERR_OK;
}

int32_t CacheBuffer::DoPlay(const int32_t streamID)
{
    CHECK_AND_RETURN_RET_LOG(streamID == streamID_, MSERR_INVALID_VAL, "Invalid streamID, failed to DoPlay.");
    if (isRunning_.load()) {
        Stop(streamID);
    }
    std::unique_lock lock(cacheBufferLock_);
    if (audioRenderer_ != nullptr) {
        if (!audioRenderer_->Start()) {
            MEDIA_ERR_LOG("audioRenderer Start failed");
            if (callback_ != nullptr) callback_->OnError(MSERR_INVALID_VAL);
            if (cacheBufferCallback_ != nullptr) cacheBufferCallback_->OnError(MSERR_INVALID_VAL);
            return MSERR_INVALID_VAL;
        }
        audioRendererRunningThread_ = std::make_unique<std::thread>(&CacheBuffer::AudioRendererRunning, this);
        isRunning_.store(true);
        runningValid_.notify_all();
        return MSERR_OK;
    }
    MEDIA_ERR_LOG("Invalid audioRenderer.");
    return MSERR_INVALID_VAL;
}

int32_t CacheBuffer::DealPlayParamsBeforePlay(const int32_t streamID, const PlayParams playParams)
{
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, MSERR_INVALID_VAL, "Invalid audioRenderer.");
    loop_ = playParams.loop;
    audioRenderer_->SetRenderRate(CheckAndAlignRendererRate(playParams.rate));
    audioRenderer_->SetVolume(playParams.leftVolume);
    priority_ = playParams.priority;
    MEDIA_INFO_LOG("CacheBuffer parallelPlayFlag:%{public}d.", playParams.parallelPlayFlag);
    audioRenderer_->SetParallelPlayFlag(playParams.parallelPlayFlag);
    return MSERR_OK;
}

AudioStandard::AudioRendererRate CacheBuffer::CheckAndAlignRendererRate(const int32_t rate) const
{
    AudioStandard::AudioRendererRate renderRate = AudioStandard::AudioRendererRate::RENDER_RATE_NORMAL;
    switch (rate) {
        case AudioStandard::AudioRendererRate::RENDER_RATE_NORMAL:
            renderRate = AudioStandard::AudioRendererRate::RENDER_RATE_NORMAL;
            break;
        case AudioStandard::AudioRendererRate::RENDER_RATE_DOUBLE:
            renderRate = AudioStandard::AudioRendererRate::RENDER_RATE_DOUBLE;
            break;
        case AudioStandard::AudioRendererRate::RENDER_RATE_HALF:
            renderRate = AudioStandard::AudioRendererRate::RENDER_RATE_HALF;
            break;
        default:
            renderRate = AudioStandard::AudioRendererRate::RENDER_RATE_NORMAL;
            break;
    }
    return renderRate;
}

void CacheBuffer::AudioRendererRunning()
{
    MEDIA_INFO_LOG("CacheBuffer audion render running.");
    int ret = MSERR_OK;
    int32_t runningCount = 0;
    do {
        for (auto data : cacheData_) {
            if (audioRenderer_ == nullptr) {
                MEDIA_ERR_LOG("audioRenderer_ is nullptr, failed to write audiobuffer");
                ret = MSERR_INVALID_VAL;
                break;
            }
            if (!(isRunning_.load())) {
                MEDIA_ERR_LOG("stop by user.");
                break;
            }
            if (data == nullptr) {
                continue;
            }
            std::unique_lock<std::mutex> lock(amutex_);
            int32_t bytesWritten = audioRenderer_->Write(data->buffer, data->size);
            if (bytesWritten <= 0) {
                MEDIA_ERR_LOG("write audiobuffer failed");
            }
        }
        if (loop_ == runningCount) {
            MEDIA_ERR_LOG("CacheBuffer loop done, loop_:%{public}d, runningCount:%{public}d", loop_, runningCount);
            break;
        }
        if (!(isRunning_.load())) {
            MEDIA_ERR_LOG("stop by user.");
            break;
        }
        if (loop_ >= 0) {
            runningCount++;
        }
    } while (true);

    if (ret == MSERR_OK) {
        if (callback_ != nullptr) callback_->OnPlayFinished();
        if (cacheBufferCallback_ != nullptr) cacheBufferCallback_->OnPlayFinished();
    } else {
        if (callback_ != nullptr) callback_->OnError(MSERR_INVALID_VAL);
    }
}

int32_t CacheBuffer::Stop(const int32_t streamID)
{
    if (streamID == streamID_) {
        std::unique_lock lock(cacheBufferLock_);
        if (!isRunning_.load()) {
            MEDIA_INFO_LOG("CacheBuffer streamID:%{public}d the stream has not been played, wait it.", streamID);
            runningValid_.wait_for(lock, std::chrono::duration<int32_t, std::milli>(WAIT_TIME_BEFORE_RUNNING_CONFIG));
        }
        if (audioRenderer_ != nullptr) {
            isRunning_.store(false);
            audioRenderer_->Stop();
        }
        if (audioRendererRunningThread_ != nullptr && audioRendererRunningThread_->joinable()) {
            audioRendererRunningThread_ ->join();
            audioRendererRunningThread_.reset();
        }
        runningValid_.notify_all();
        return MSERR_OK;
    }
    return MSERR_INVALID_VAL;
}

int32_t CacheBuffer::SetVolume(const int32_t streamID, const float leftVolume, const float rightVolume)
{
    int32_t ret = MSERR_OK;
    if (streamID == streamID_) {
        std::unique_lock lock(cacheBufferLock_);
        if (!isRunning_.load()) {
            MEDIA_INFO_LOG("CacheBuffer streamID:%{public}d the stream has not been played, wait it.", streamID);
            runningValid_.wait_for(lock, std::chrono::duration<int32_t, std::milli>(WAIT_TIME_BEFORE_RUNNING_CONFIG));
        }
        if (audioRenderer_ != nullptr) {
            // audio cannot support left & right volume, all use left volume.
            (void) rightVolume;
            ret = audioRenderer_->SetVolume(leftVolume);
            runningValid_.notify_all();
        }
    }
    return ret;
}

int32_t CacheBuffer::SetRate(const int32_t streamID, const AudioStandard::AudioRendererRate renderRate)
{
    int32_t ret = MSERR_INVALID_VAL;
    if (streamID == streamID_) {
        std::unique_lock lock(cacheBufferLock_);
        if (!isRunning_.load()) {
            MEDIA_INFO_LOG("CacheBuffer streamID:%{public}d the stream has not been played, wait it.", streamID);
            runningValid_.wait_for(lock, std::chrono::duration<int32_t, std::milli>(WAIT_TIME_BEFORE_RUNNING_CONFIG));
        }
        if (audioRenderer_ != nullptr) {
            ret = audioRenderer_->SetRenderRate(CheckAndAlignRendererRate(renderRate));
            runningValid_.notify_all();
        }
    }
    return ret;
}

int32_t CacheBuffer::SetPriority(const int32_t streamID, const int32_t priority)
{
    if (streamID == streamID_) {
        std::unique_lock lock(cacheBufferLock_);
        if (!isRunning_.load()) {
            MEDIA_INFO_LOG("CacheBuffer streamID:%{public}d the stream has not been played, wait it.", streamID);
            runningValid_.wait_for(lock, std::chrono::duration<int32_t, std::milli>(WAIT_TIME_BEFORE_RUNNING_CONFIG));
        }
        priority_ = priority;
        runningValid_.notify_all();
    }
    return MSERR_OK;
}

int32_t CacheBuffer::SetLoop(const int32_t streamID, const int32_t loop)
{
    if (streamID == streamID_) {
        std::unique_lock lock(cacheBufferLock_);
        if (!isRunning_.load()) {
            MEDIA_INFO_LOG("CacheBuffer streamID:%{public}d the stream has not been played, wait it.", streamID);
            runningValid_.wait_for(lock, std::chrono::duration<int32_t, std::milli>(WAIT_TIME_BEFORE_RUNNING_CONFIG));
        }
        loop_ = loop;
        runningValid_.notify_all();
    }
    return MSERR_OK;
}

int32_t CacheBuffer::SetParallelPlayFlag(const int32_t streamID, const bool parallelPlayFlag)
{
    if (streamID == streamID_) {
        std::unique_lock lock(cacheBufferLock_);
        if (!isRunning_.load()) {
            MEDIA_INFO_LOG("CacheBuffer streamID:%{public}d the stream has not been played, wait it.", streamID);
            runningValid_.wait_for(lock, std::chrono::duration<int32_t, std::milli>(WAIT_TIME_BEFORE_RUNNING_CONFIG));
        }

        MEDIA_INFO_LOG("CacheBuffer parallelPlayFlag:%{public}d.", parallelPlayFlag);
        if (audioRenderer_ != nullptr) {
            audioRenderer_->SetParallelPlayFlag(parallelPlayFlag);
        }
        runningValid_.notify_all();
    }
    return MSERR_OK;
}

int32_t CacheBuffer::Release()
{
    MEDIA_INFO_LOG("CacheBuffer release, streamID:%{public}d", streamID_);
    isRunning_.store(false);
    if (audioRenderer_ != nullptr) {
        audioRenderer_->Stop();
        audioRenderer_->Release();
        audioRenderer_ = nullptr;
    }
    if (audioRendererRunningThread_ != nullptr && audioRendererRunningThread_->joinable()) {
        audioRendererRunningThread_ ->join();
        audioRendererRunningThread_.reset();
    }
    runningValid_.notify_all();
    return MSERR_OK;
}

int32_t CacheBuffer::SetCallback(const std::shared_ptr<ISoundPoolCallback> &callback)
{
    callback_ = callback;
    return MSERR_OK;
}

int32_t CacheBuffer::SetCacheBufferCallback(const std::shared_ptr<ISoundPoolCallback> &callback)
{
    cacheBufferCallback_ = callback;
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
