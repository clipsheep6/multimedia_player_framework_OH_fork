
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
    const size_t cacheDataTotalSize, const int32_t soundID, const int32_t streamID) : trackFormat_(trackFormat),
    cacheData_(cacheData), cacheDataTotalSize_(cacheDataTotalSize), soundID_(soundID), streamID_(streamID),
    cacheDataFrameNum_(0), havePlayedCount_(0)

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

    MEDIA_INFO_LOG("CacheBuffer sampleRate:%{public}d, sampleFormat:%{public}d, channelCount:%{public}d.",
        sampleRate, sampleFormat, channelCount);
    // low-latency:1, non low-latency:0
    if ((rendererOptions.streamInfo.samplingRate == AudioStandard::AudioSamplingRate::SAMPLE_RATE_48000) &&
        (rendererOptions.streamInfo.channels == AudioStandard::AudioChannel::MONO ||
        rendererOptions.streamInfo.channels == AudioStandard::AudioChannel::STEREO)) {
        // samplingRate 48KHz, channelCount 1/2
        MEDIA_INFO_LOG("low latency support this rendererOptions.");
        rendererFlags_ = audioRendererInfo.rendererFlags;
    } else {
        MEDIA_INFO_LOG("low latency didn't support this rendererOptions, force normal play.");
        rendererFlags_ = NORMAL_PLAY_RENDERER_FLAGS;
    }
    rendererOptions.rendererInfo.rendererFlags = rendererFlags_;
    std::unique_ptr<AudioStandard::AudioRenderer> audioRenderer =
        AudioStandard::AudioRenderer::Create(cacheDir, rendererOptions);
    CHECK_AND_RETURN_RET_LOG(audioRenderer != nullptr, nullptr, "Invalid audioRenderer.");
    audioRenderer->SetRenderMode(AudioStandard::AudioRenderMode::RENDER_MODE_CALLBACK);
    int32_t ret = audioRenderer->SetRendererWriteCallback(shared_from_this());
    if (ret != MSERR_OK) {
        MEDIA_ERR_LOG("audio renderer write callback fail, ret %{public}d.", ret);
    }
    return audioRenderer;
}

int32_t CacheBuffer::PreparePlay(const int32_t streamID, const AudioStandard::AudioRendererInfo audioRendererInfo,
    const PlayParams playParams)
{
    // create audioRenderer
    if (audioRenderer_ == nullptr) {
        audioRenderer_ = CreateAudioRenderer(streamID, audioRendererInfo, playParams);
        ReCombineCacheData();
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
    std::lock_guard lock(cacheBufferLock_);
    if (audioRenderer_ != nullptr) {
        isRunning_.store(true);
        cacheDataFrameNum_ = 0;
        havePlayedCount_ = 0;
        if (!audioRenderer_->Start()) {
            MEDIA_ERR_LOG("audioRenderer Start failed");
            if (callback_ != nullptr) callback_->OnError(MSERR_INVALID_VAL);
            if (cacheBufferCallback_ != nullptr) cacheBufferCallback_->OnError(MSERR_INVALID_VAL);
            return MSERR_INVALID_VAL;
        }
        return MSERR_OK;
    }
    MEDIA_ERR_LOG("Invalid audioRenderer.");
    return MSERR_INVALID_VAL;
}

int32_t CacheBuffer::ReCombineCacheData()
{
    std::lock_guard lock(cacheBufferLock_);
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, MSERR_INVALID_VAL, "Invalid audioRenderer.");
    CHECK_AND_RETURN_RET_LOG(!cacheData_.empty(), MSERR_INVALID_VAL, "empty cache data.");
    size_t bufferSize;
    audioRenderer_->GetBufferSize(bufferSize);
    // Prevent data from crossing boundaries, ensure recombine data largest.
    size_t reCombineCacheDataSize = (cacheDataTotalSize_ + bufferSize - 1) / bufferSize;
    std::shared_ptr<AudioBufferEntry> preAudioBuffer = cacheData_.front();
    size_t preAudioBufferIndex = 0;
    for (size_t reCombineCacheDataNum = 0; reCombineCacheDataNum < reCombineCacheDataSize; reCombineCacheDataNum++) {
        uint8_t *reCombineBuf = new(std::nothrow) uint8_t[bufferSize];
        for (size_t bufferNum = 0; bufferNum < bufferSize; bufferNum++) {
            if (cacheData_.size() > 1 && (preAudioBufferIndex == static_cast<size_t>(preAudioBuffer->size))) {
                cacheData_.pop_front();
                preAudioBuffer = cacheData_.front();
                preAudioBufferIndex = 0;
            }
            if (cacheData_.size() == 1 && (preAudioBufferIndex == static_cast<size_t>(preAudioBuffer->size))) {
                preAudioBuffer = cacheData_.front();
                cacheData_.pop_front();
                preAudioBufferIndex = 0;
            }
            if (cacheData_.empty()) {
                reCombineBuf[bufferNum] = 0;
            } else {
                reCombineBuf[bufferNum] = preAudioBuffer->buffer[preAudioBufferIndex];
            }
            preAudioBufferIndex++;
        }
        reCombineCacheData_.push_back(std::make_shared<AudioBufferEntry>(reCombineBuf, bufferSize));
    }
    if (!cacheData_.empty()) {
        cacheData_.clear();
    }
    return MSERR_OK;
}

int32_t CacheBuffer::DealPlayParamsBeforePlay(const int32_t streamID, const PlayParams playParams)
{
    std::lock_guard lock(cacheBufferLock_);
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, MSERR_INVALID_VAL, "Invalid audioRenderer.");
    loop_ = playParams.loop;
    audioRenderer_->SetRenderRate(CheckAndAlignRendererRate(playParams.rate));
    audioRenderer_->SetVolume(playParams.leftVolume);
    priority_ = playParams.priority;
    audioRenderer_->SetParallelPlayFlag(playParams.parallelPlayFlag);
    return MSERR_OK;
}

AudioStandard::AudioRendererRate CacheBuffer::CheckAndAlignRendererRate(const int32_t rate)
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

void CacheBuffer::OnWriteData(size_t length)
{
    AudioStandard::BufferDesc bufDesc;
    if (audioRenderer_ == nullptr) {
        MEDIA_ERR_LOG("audioRenderer is nullptr.");
        return;
    }
    if (!isRunning_.load()) {
        MEDIA_ERR_LOG("audioRenderer is stop.");
        return;
    }
    if (cacheDataFrameNum_ == reCombineCacheData_.size()) {
        if (havePlayedCount_ == loop_) {
            MEDIA_INFO_LOG("CacheBuffer stream write finish, cacheDataFrameNum_:%{public}zu,"
                " havePlayedCount_:%{public}d, loop:%{public}d, try to stop.", cacheDataFrameNum_,
                havePlayedCount_, loop_);
            Stop(streamID_);
            return;
        }
        cacheDataFrameNum_ = 0;
        havePlayedCount_++;
    }
    audioRenderer_->GetBufferDesc(bufDesc);
    std::shared_ptr<AudioBufferEntry> audioBuffer = reCombineCacheData_[cacheDataFrameNum_];

    bufDesc.buffer = audioBuffer->buffer;
    bufDesc.bufLength = static_cast<size_t>(audioBuffer->size);
    bufDesc.dataLength = static_cast<size_t>(audioBuffer->size);

    audioRenderer_->Enqueue(bufDesc);
    cacheDataFrameNum_++;
}

int32_t CacheBuffer::Stop(const int32_t streamID)
{
    std::lock_guard lock(cacheBufferLock_);
    if (streamID == streamID_) {
        if (audioRenderer_ != nullptr && isRunning_.load()) {
            isRunning_.store(false);
            if (rendererFlags_ == LOW_LATENCY_PLAY_RENDERER_FLAGS) {
                audioRenderer_->Pause();
                audioRenderer_->Flush();
            } else {
                audioRenderer_->Stop();
            }
            cacheDataFrameNum_ = 0;
            havePlayedCount_ = 0;
            if (callback_ != nullptr) callback_->OnPlayFinished();
            if (cacheBufferCallback_ != nullptr) cacheBufferCallback_->OnPlayFinished();
        }
        return MSERR_OK;
    }
    return MSERR_INVALID_VAL;
}

int32_t CacheBuffer::SetVolume(const int32_t streamID, const float leftVolume, const float rightVolume)
{
    std::lock_guard lock(cacheBufferLock_);
    int32_t ret = MSERR_OK;
    if (streamID == streamID_) {
        if (audioRenderer_ != nullptr) {
            // audio cannot support left & right volume, all use left volume.
            (void) rightVolume;
            ret = audioRenderer_->SetVolume(leftVolume);
        }
    }
    return ret;
}

int32_t CacheBuffer::SetRate(const int32_t streamID, const AudioStandard::AudioRendererRate renderRate)
{
    std::lock_guard lock(cacheBufferLock_);
    int32_t ret = MSERR_INVALID_VAL;
    if (streamID == streamID_) {
        if (audioRenderer_ != nullptr) {
            ret = audioRenderer_->SetRenderRate(CheckAndAlignRendererRate(renderRate));
        }
    }
    return ret;
}

int32_t CacheBuffer::SetPriority(const int32_t streamID, const int32_t priority)
{
    std::lock_guard lock(cacheBufferLock_);
    if (streamID == streamID_) {
        priority_ = priority;
    }
    return MSERR_OK;
}

int32_t CacheBuffer::SetLoop(const int32_t streamID, const int32_t loop)
{
    std::lock_guard lock(cacheBufferLock_);
    if (streamID == streamID_) {
        loop_ = loop;
    }
    return MSERR_OK;
}

int32_t CacheBuffer::SetParallelPlayFlag(const int32_t streamID, const bool parallelPlayFlag)
{
    std::lock_guard lock(cacheBufferLock_);
    if (streamID == streamID_) {
        MEDIA_INFO_LOG("CacheBuffer parallelPlayFlag:%{public}d.", parallelPlayFlag);
        if (audioRenderer_ != nullptr) {
            audioRenderer_->SetParallelPlayFlag(parallelPlayFlag);
        }
    }
    return MSERR_OK;
}

int32_t CacheBuffer::Release()
{
    std::lock_guard lock(cacheBufferLock_);
    MEDIA_INFO_LOG("CacheBuffer release, streamID:%{public}d", streamID_);
    isRunning_.store(false);
    if (audioRenderer_ != nullptr) {
        audioRenderer_->Stop();
        audioRenderer_->Release();
        audioRenderer_ = nullptr;
    }
    if (!cacheData_.empty()) cacheData_.clear();
    if (!reCombineCacheData_.empty()) reCombineCacheData_.clear();
    if (callback_ != nullptr) callback_.reset();
    if (cacheBufferCallback_ != nullptr) cacheBufferCallback_.reset();
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
