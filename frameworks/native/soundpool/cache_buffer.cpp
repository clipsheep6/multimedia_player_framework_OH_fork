
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
CacheBuffer::CacheBuffer(MediaAVCodec::Format trackFormat,
    std::deque<std::shared_ptr<AudioBufferEntry>> cacheData,
    int32_t soundID, int32_t streamID) : trackFormat_(trackFormat),
    cacheData_(cacheData), soundID_(soundID), streamID_(streamID)
{
    MEDIA_INFO_LOG("Construction CacheBuffer");
}

CacheBuffer::~CacheBuffer()
{
    MEDIA_INFO_LOG("Destruction CacheBuffer dec");
    isRunning_.store(false);
    if (audioRenderer_ != nullptr) {
        audioRenderer_->Stop();
        audioRenderer_->Release();
        audioRenderer_ = nullptr;
    }
}

std::unique_ptr<AudioStandard::AudioRenderer> CacheBuffer::CreateAudioRenderer(const int32_t streamID,
    const AudioStandard::AudioRendererInfo audioRendererInfo, const PlayParams playParams)
{
    MEDIA_INFO_LOG("CacheBuffer::%{public}s, streamID:%{public}d, streamID_:%{public}d", __func__, streamID, streamID_);
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
    // low-latency:1, non low-latency:0
    rendererOptions.rendererInfo.rendererFlags = 0;
    rendererOptions.privacyType = AudioStandard::PRIVACY_TYPE_PUBLIC;
    std::string cacheDir = "/data/storage/el2/base/temp";
    if (playParams.cacheDir != "") {
        cacheDir = playParams.cacheDir;
    }
    std::unique_ptr<AudioStandard::AudioRenderer> audioRenderer =
        AudioStandard::AudioRenderer::Create(cacheDir, rendererOptions);
    return audioRenderer;
}

int32_t CacheBuffer::PreparePlay(const int32_t streamID, const AudioStandard::AudioRendererInfo audioRendererInfo,
    const PlayParams playParams)
{
    // create audioRenderer
    if (audioRenderer_ == nullptr) {
        audioRenderer_ = CreateAudioRenderer(streamID, audioRendererInfo, playParams);
    } else {
        MEDIA_INFO_LOG("audio render inited.");
    }
    // deal play params
    DealPlayParamsBeforePaly(streamID, playParams);
    return MSERR_OK;
}

int32_t CacheBuffer::DoPlay(const int32_t streamID, const AudioStandard::AudioRendererInfo audioRendererInfo,
    const PlayParams playParams)
{
    CHECK_AND_RETURN_RET_LOG(streamID == streamID_, MSERR_INVALID_VAL, "Invalid streamID, failed to DoPlay.");
    if (isRunning_.load()) {
        Stop(streamID);
    }
    PreparePlay(streamID, audioRendererInfo, playParams);
    if (audioRenderer_ != nullptr) {
        if (!audioRenderer_->Start()) {
            MEDIA_ERR_LOG("audioRenderer Start failed");
            if (callback_ != nullptr) callback_->OnError(MSERR_INVALID_VAL);
            return MSERR_INVALID_VAL;
        }
    } else {
        MEDIA_ERR_LOG("Invalid audioRenderer.");
        return MSERR_INVALID_VAL;
    }

    isRunning_.store(true);
    AudioRendererRunning();
    return MSERR_OK;
}

int32_t CacheBuffer::DealPlayParamsBeforePaly(const int32_t streamID, const PlayParams playParams)
{
    MEDIA_INFO_LOG("CacheBuffer::%{public}s", __func__);
    SetLoop(streamID, playParams.loop);
    AudioStandard::AudioRendererRate renderRate;
    switch (playParams.rate) {
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
    MEDIA_INFO_LOG("CacheBuffer playParams.rate:%{public}d", playParams.rate);
    SetRate(streamID, renderRate);
    SetVolume(streamID, playParams.leftVolume, playParams.rightVolume);
    SetPriority(streamID, playParams.priority);
    SetParallelPlayFlag(streamID, playParams.parallelPlayFlag);

    return MSERR_OK;
}

void CacheBuffer::AudioRendererRunning()
{
    MEDIA_INFO_LOG("CacheBuffer::%{public}s", __func__);
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
        MEDIA_ERR_LOG("CacheBuffer loop_:%{public}d, runningCount:%{public}d", loop_, runningCount);
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

    CHECK_AND_RETURN_LOG(callback_ != nullptr, "Invalid callback.");
    if (ret == MSERR_OK) {
        callback_->OnPlayFinished();
    } else {
        MEDIA_INFO_LOG("cachebuffer soundpool callback invalid.");
        callback_->OnError(MSERR_INVALID_VAL);
    }
}

int32_t CacheBuffer::Stop(const int32_t streamID)
{
    MEDIA_INFO_LOG("CacheBuffer::%{public}s streamID:%{public}d, streamID_:%{public}d", __func__, streamID, streamID_);
    std::lock_guard lock(cachebufferLock_);
    if (streamID == streamID_) {
        isRunning_.store(false);
        if (audioRenderer_ != nullptr) {
            audioRenderer_->Stop();
        }
        return MSERR_OK;
    }

    return MSERR_INVALID_VAL;
}

int32_t CacheBuffer::SetVolume(const int32_t streamID, const float leftVolume, const float rightVolume)
{
    MEDIA_INFO_LOG("CacheBuffer::%{public}s streamID:%{public}d", __func__, streamID);
    std::lock_guard lock(cachebufferLock_);
    if (streamID == streamID_) {
        if (audioRenderer_ != nullptr) {
            // audio cannot support left & right volume, all use left volume.
            (void) rightVolume;
            return audioRenderer_->SetVolume(leftVolume);
        }
    }
    return MSERR_INVALID_VAL;
}

int32_t CacheBuffer::SetRate(const int32_t streamID, const AudioStandard::AudioRendererRate renderRate)
{
    MEDIA_INFO_LOG("CacheBuffer::%{public}s streamID:%{public}d", __func__, streamID);
    std::lock_guard lock(cachebufferLock_);
    if (streamID == streamID_) {
        if (audioRenderer_ != nullptr) {
            return audioRenderer_->SetRenderRate(renderRate);
        }
    }
    return MSERR_INVALID_VAL;
}

int32_t CacheBuffer::SetPriority(const int32_t streamID, const int32_t priority)
{
    MEDIA_INFO_LOG("CacheBuffer::%{public}s streamID:%{public}d", __func__, streamID);
    std::lock_guard lock(cachebufferLock_);
    if (streamID == streamID_) {
        priority_ = priority;
    }
    return MSERR_OK;
}

int32_t CacheBuffer::SetLoop(const int32_t streamID, const int32_t loop)
{
    MEDIA_INFO_LOG("CacheBuffer::%{public}s streamID:%{public}d, loop:%{public}d", __func__, streamID, loop);
    std::lock_guard lock(cachebufferLock_);
    if (streamID == streamID_) {
        loop_ = loop;
    }
    return MSERR_OK;
}

int32_t CacheBuffer::SetParallelPlayFlag(const int32_t streamID, const bool parallelPlayFlag)
{
    std::lock_guard lock(cachebufferLock_);
    MEDIA_INFO_LOG("CacheBuffer unsupport parallel config, streamID:%{public}d", streamID);
    parallelPlayFlag_ = parallelPlayFlag;
    (void) parallelPlayFlag_;
    // 预留接口
    return MSERR_OK;
}

int32_t CacheBuffer::SetCallback(const std::shared_ptr<ISoundPoolCallback> &callback)
{
    MEDIA_INFO_LOG("CacheBuffer SetCallback.");
    callback_ = callback;
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
