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

#include "parameter.h"
#include "soundpool.h"
#include "media_log.h"
#include "media_errors.h"
#include "stream_id_manager.h"

namespace {
    // audiorender max concurrency.
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "StreamIDManager"};
    static const std::string THREAD_POOL_NAME = "StreamIDManagerThreadPool";
    static const int32_t MAX_THREADS_NUM = std::thread::hardware_concurrency() >= 4 ? 2 : 1;
}

namespace OHOS {
namespace Media {
StreamIDManager::StreamIDManager(int32_t maxStreams,
    AudioStandard::AudioRendererInfo audioRenderInfo) : audioRendererInfo_(audioRenderInfo), maxStreams_(maxStreams)
{
    MEDIA_LOGI("Construction StreamIDManager.");
#ifdef SOUNDPOOL_SUPPORT_LOW_LATENCY
    char hardwareName[10] = {0};
    GetParameter("ohos.boot.hardware", "rk3568", hardwareName, sizeof(hardwareName));
    if (strcmp(hardwareName, "baltimore") != 0) {
        MEDIA_LOGI("Device unsupport low-latency, force set to normal play.");
        audioRendererInfo_.rendererFlags = 0;
    } else {
        MEDIA_LOGI("Device support low-latency, set renderer by user.");
    }
#else
    // Force all play to normal.
    audioRendererInfo_.rendererFlags = 0;
    MEDIA_LOGI("SoundPool unsupport low-latency.");
#endif

    InitThreadPool();
}

StreamIDManager::~StreamIDManager()
{
    MEDIA_LOGI("Destruction StreamIDManager");
    if (callback_ != nullptr) {
        callback_.reset();
    }
    if (frameWriteCallback_ != nullptr) {
        frameWriteCallback_.reset();
    }
    for (auto cacheBuffer : cacheBuffers_) {
        if (cacheBuffer.second != nullptr) {
            cacheBuffer.second->Release();
        }
    }
    cacheBuffers_.clear();
    if (isStreamPlayingThreadPoolStarted_.load()) {
        if (streamPlayingThreadPool_ != nullptr) {
            streamPlayingThreadPool_->Stop();
        }
        isStreamPlayingThreadPoolStarted_.store(false);
    }
}

int32_t StreamIDManager::InitThreadPool()
{
    if (isStreamPlayingThreadPoolStarted_.load()) {
        return MSERR_OK;
    }
    streamPlayingThreadPool_ = std::make_unique<ThreadPool>(THREAD_POOL_NAME);
    CHECK_AND_RETURN_RET_LOG(streamPlayingThreadPool_ != nullptr, MSERR_INVALID_VAL,
        "Failed to obtain playing ThreadPool");
    if (maxStreams_ > MAX_PLAY_STREAMS_NUMBER) {
        maxStreams_ = MAX_PLAY_STREAMS_NUMBER;
        MEDIA_LOGI("more than max play stream number, align to max play strem number.");
    }
    if (maxStreams_ < MIN_PLAY_STREAMS_NUMBER) {
        maxStreams_ = MIN_PLAY_STREAMS_NUMBER;
        MEDIA_LOGI("less than min play stream number, align to min play strem number.");
    }
    MEDIA_LOGI("stream playing thread pool maxStreams_:%{public}d", maxStreams_);
    // For stream priority logic, thread num need align to task num.
    streamPlayingThreadPool_->Start(maxStreams_);
    streamPlayingThreadPool_->SetMaxTaskNum(maxStreams_);
    isStreamPlayingThreadPoolStarted_.store(true);

    return MSERR_OK;
}

int32_t StreamIDManager::Play(std::shared_ptr<SoundParser> soundParser, PlayParams playParameters)
{
    CHECK_AND_RETURN_RET_LOG(soundParser != nullptr, -1, "Invalid soundParser.");
    int32_t soundID = soundParser->GetSoundID();
    int32_t streamID = GetFreshStreamID(soundID, playParameters);
    {
        std::lock_guard lock(streamIDManagerLock_);
        if (streamID <= 0) {
            do {
                nextStreamID_ = nextStreamID_ == INT32_MAX ? 1 : nextStreamID_ + 1;
            } while (FindCacheBuffer(nextStreamID_) != nullptr);
            streamID = nextStreamID_;
            std::deque<std::shared_ptr<AudioBufferEntry>> cacheData;
            soundParser->GetSoundData(cacheData);
            size_t cacheDataTotalSize = soundParser->GetSoundDataTotalSize();
            auto cacheBuffer =
                std::make_shared<CacheBuffer>(soundParser->GetSoundTrackFormat(), cacheData, cacheDataTotalSize,
                     soundID, streamID);
            CHECK_AND_RETURN_RET_LOG(cacheBuffer != nullptr, -1, "failed to create cache buffer");
            CHECK_AND_RETURN_RET_LOG(callback_ != nullptr, MSERR_INVALID_VAL, "Invalid callback.");
            cacheBuffer->SetCallback(callback_);
            cacheBufferCallback_ = std::make_shared<CacheBufferCallBack>(weak_from_this());
            CHECK_AND_RETURN_RET_LOG(cacheBufferCallback_ != nullptr, MSERR_INVALID_VAL,
                "Invalid cachebuffer callback");
            cacheBuffer->SetCacheBufferCallback(cacheBufferCallback_);
            if (frameWriteCallback_ != nullptr) {
                cacheBuffer->SetFrameWriteCallback(frameWriteCallback_);
            }
            cacheBuffers_.emplace(streamID, cacheBuffer);
        }
    }
    SetPlay(soundID, streamID, playParameters);
    return streamID;
}

int32_t StreamIDManager::SetPlay(const int32_t soundID, const int32_t streamID, const PlayParams playParameters)
{
    if (!isStreamPlayingThreadPoolStarted_.load()) {
        InitThreadPool();
    }

    CHECK_AND_RETURN_RET_LOG(streamPlayingThreadPool_ != nullptr, MSERR_INVALID_VAL,
        "Failed to obtain stream play threadpool.");
    MEDIA_LOGI("StreamIDManager cur task num:%{public}zu, maxStreams_:%{public}d",
        currentTaskNum_, maxStreams_);
    // CacheBuffer must prepare before play.
    std::shared_ptr<CacheBuffer> freshCacheBuffer = FindCacheBuffer(streamID);
    CHECK_AND_RETURN_RET_LOG(freshCacheBuffer != nullptr, -1, "Invalid fresh cache buffer");
    freshCacheBuffer->PreparePlay(streamID, audioRendererInfo_, playParameters);
    int32_t tempMaxStream = maxStreams_;
    if (currentTaskNum_ < static_cast<size_t>(tempMaxStream)) {
        AddPlayTask(streamID, playParameters);
    } else {
        int32_t playingStreamID = playingStreamIDs_.back();
        std::shared_ptr<CacheBuffer> playingCacheBuffer = FindCacheBuffer(playingStreamID);
        CHECK_AND_RETURN_RET_LOG(freshCacheBuffer != nullptr, -1, "Invalid fresh cache buffer");
        MEDIA_LOGI("StreamIDManager fresh sound priority:%{public}d, playing stream priority:%{public}d",
            freshCacheBuffer->GetPriority(), playingCacheBuffer->GetPriority());
        if (freshCacheBuffer->GetPriority() >= playingCacheBuffer->GetPriority()) {
            MEDIA_LOGI("StreamIDManager stop playing low priority sound:%{public}d", playingStreamID);
            playingCacheBuffer->Stop(playingStreamID);
            playingStreamIDs_.pop_back();
            MEDIA_LOGI("StreamIDManager to playing fresh sound:%{public}d.", streamID);
            AddPlayTask(streamID, playParameters);
        } else {
            std::lock_guard lock(streamIDManagerLock_);
            MEDIA_LOGI("StreamIDManager queue will play streams, streamID:%{public}d.", streamID);
            StreamIDAndPlayParamsInfo freshStreamIDAndPlayParamsInfo;
            freshStreamIDAndPlayParamsInfo.streamID = streamID;
            freshStreamIDAndPlayParamsInfo.playParameters = playParameters;
            QueueAndSortWillPlayStreamID(freshStreamIDAndPlayParamsInfo);
        }
    }

    return MSERR_OK;
}

// Sort in descending order
// 0 has the lowest priority, and the higher the value, the higher the priority
// The queue head has the highest value and priority
void StreamIDManager::QueueAndSortPlayingStreamID(int32_t streamID)
{
    if (playingStreamIDs_.empty()) {
        playingStreamIDs_.emplace_back(streamID);
    } else {
        bool shouldReCombinePlayingQueue = false;
        for (size_t i = 0; i < playingStreamIDs_.size(); i++) {
            int32_t playingStreamID = playingStreamIDs_[i];
            std::shared_ptr<CacheBuffer> freshCacheBuffer = FindCacheBuffer(streamID);
            std::shared_ptr<CacheBuffer> playingCacheBuffer = FindCacheBuffer(playingStreamID);
            if (playingCacheBuffer == nullptr) {
                playingStreamIDs_.erase(playingStreamIDs_.begin() + i);
                shouldReCombinePlayingQueue = true;
                break;
            }
            if (!playingCacheBuffer->IsRunning()) {
                playingStreamIDs_.erase(playingStreamIDs_.begin() + i);
                shouldReCombinePlayingQueue = true;
                break;
            }
            if (freshCacheBuffer == nullptr) {
                break;
            }
            if (freshCacheBuffer->GetPriority() <= playingCacheBuffer->GetPriority()) {
                playingStreamIDs_.insert(playingStreamIDs_.begin() + i, streamID);
                break;
            }
        }
        if (shouldReCombinePlayingQueue) {
            QueueAndSortPlayingStreamID(streamID);
        }
    }
}

// Sort in descending order.
// 0 has the lowest priority, and the higher the value, the higher the priority
// The queue head has the highest value and priority
void StreamIDManager::QueueAndSortWillPlayStreamID(StreamIDAndPlayParamsInfo freshStreamIDAndPlayParamsInfo)
{
    if (willPlayStreamInfos_.empty()) {
        willPlayStreamInfos_.emplace_back(freshStreamIDAndPlayParamsInfo);
    } else {
        bool shouldReCombineWillPlayQueue = false;
        for (size_t i = 0; i < willPlayStreamInfos_.size(); i++) {
            std::shared_ptr<CacheBuffer> freshCacheBuffer = FindCacheBuffer(freshStreamIDAndPlayParamsInfo.streamID);
            std::shared_ptr<CacheBuffer> willPlayCacheBuffer = FindCacheBuffer(willPlayStreamInfos_[i].streamID);
            if (willPlayCacheBuffer == nullptr) {
                willPlayStreamInfos_.erase(willPlayStreamInfos_.begin() + i);
                shouldReCombineWillPlayQueue = true;
                break;
            }
            if (freshCacheBuffer == nullptr) {
                break;
            }
            if (freshCacheBuffer->GetPriority() <= willPlayCacheBuffer->GetPriority()) {
                willPlayStreamInfos_.insert(willPlayStreamInfos_.begin() + i, freshStreamIDAndPlayParamsInfo);
                break;
            }
        }
        if (shouldReCombineWillPlayQueue) {
            QueueAndSortWillPlayStreamID(freshStreamIDAndPlayParamsInfo);
        }
    }
}

int32_t StreamIDManager::AddPlayTask(const int32_t streamID, const PlayParams playParameters)
{
    ThreadPool::Task streamPlayTask = std::bind(&StreamIDManager::DoPlay, this, streamID);
    CHECK_AND_RETURN_RET_LOG(streamPlayingThreadPool_ != nullptr, MSERR_INVALID_VAL,
        "Failed to obtain playing ThreadPool");
    CHECK_AND_RETURN_RET_LOG(streamPlayTask != nullptr, MSERR_INVALID_VAL, "Failed to obtain stream play Task");
    streamPlayingThreadPool_->AddTask(streamPlayTask);
    std::lock_guard lock(streamIDManagerLock_);
    currentTaskNum_++;
    QueueAndSortPlayingStreamID(streamID);
    return MSERR_OK;
}

int32_t StreamIDManager::DoPlay(const int32_t streamID)
{
    MEDIA_LOGI("StreamIDManager streamID:%{public}d", streamID);
    std::shared_ptr<CacheBuffer> cacheBuffer = FindCacheBuffer(streamID);
    CHECK_AND_RETURN_RET_LOG(cacheBuffer.get() != nullptr, MSERR_INVALID_VAL, "cachebuffer invalid.");
    if (cacheBuffer->DoPlay(streamID) == MSERR_OK) {
        return MSERR_OK;
    }
    return MSERR_INVALID_VAL;
}

std::shared_ptr<CacheBuffer> StreamIDManager::FindCacheBuffer(const int32_t streamID)
{
    if (cacheBuffers_.empty()) {
        MEDIA_LOGI("StreamIDManager cacheBuffers_ empty");
        return nullptr;
    }
    if (cacheBuffers_.find(streamID) != cacheBuffers_.end()) {
        return cacheBuffers_.at(streamID);
    }
    return nullptr;
}

int32_t StreamIDManager::GetStreamIDBySoundID(const int32_t soundID)
{
    PlayParams playParameters;
    return GetFreshStreamID(soundID, playParameters);
}

int32_t StreamIDManager::GetFreshStreamID(const int32_t soundID, PlayParams playParameters)
{
    int32_t streamID = 0;
    if (cacheBuffers_.empty()) {
        MEDIA_LOGI("StreamIDManager cacheBuffers_ empty");
        return streamID;
    }
    for (auto cacheBuffer : cacheBuffers_) {
        if (cacheBuffer.second == nullptr) {
            MEDIA_LOGE("Invalid cacheBuffer, soundID:%{public}d", soundID);
            continue;
        }
        if (soundID == cacheBuffer.second->GetSoundID()) {
            streamID = cacheBuffer.second->GetStreamID();
            MEDIA_LOGI("Have cache soundID:%{public}d, streamID:%{public}d", soundID, streamID);
            break;
        }
    }
    return streamID;
}

void StreamIDManager::OnPlayFinished()
{
    currentTaskNum_--;
    if (!willPlayStreamInfos_.empty()) {
        MEDIA_LOGI("StreamIDManager OnPlayFinished will play streams non empty, get the front.");
        StreamIDAndPlayParamsInfo willPlayStreamInfo =  willPlayStreamInfos_.front();
        AddPlayTask(willPlayStreamInfo.streamID, willPlayStreamInfo.playParameters);
        std::lock_guard lock(streamIDManagerLock_);
        willPlayStreamInfos_.pop_front();
    }
}

int32_t StreamIDManager::SetCallback(const std::shared_ptr<ISoundPoolCallback> &callback)
{
    callback_ = callback;
    return MSERR_OK;
}

int32_t StreamIDManager::SetFrameWriteCallback(const std::shared_ptr<ISoundPoolFrameWriteCallback> &callback)
{
    frameWriteCallback_ = callback;
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
