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

#include "soundpool.h"
#include "media_log.h"
#include "media_errors.h"
#include "stream_id_manager.h"

namespace {
    // audiorender max concurrency.
    static const std::string THREAD_POOL_NAME = "StreamIDManagerThreadPool";
    static const int32_t MAX_THREADS_NUM = std::thread::hardware_concurrency();
}

namespace OHOS {
namespace Media {
StreamIDManager::StreamIDManager(int32_t maxStreams,
    AudioStandard::AudioRendererInfo audioRenderInfo) : audioRendererInfo_(audioRenderInfo), maxStreams_(maxStreams)
{
    MEDIA_INFO_LOG("Construction StreamIDManager");
    InitThreadPool();
}

StreamIDManager::~StreamIDManager()
{
    MEDIA_INFO_LOG("Destruction StreamIDManager");
    {
        std::lock_guard lock(streamIDManagerLock_);
        quitQueue_ = true;
        queueSpaceValid_.notify_all(); // notify all load waiters
        queueDataValid_.notify_all();  // notify all worker threads
    }

    if (isStreamPlayingThreadPoolStarted_.load()) {
        if (streamPlayingThreadPool_ != nullptr) {
            streamPlayingThreadPool_->Stop();
        }
        isStreamPlayingThreadPoolStarted_.store(false);
    }

    if (callback_ != nullptr) callback_ = nullptr;
    cacheBuffers_.clear();
}

int32_t StreamIDManager::InitThreadPool()
{
    if (isStreamPlayingThreadPoolStarted_.load()) {
        return MSERR_OK;
    }
    streamPlayingThreadPool_ = std::make_unique<ThreadPool>(THREAD_POOL_NAME);
    MEDIA_INFO_LOG("stream playing thread pool maxStreams_:%{public}d", maxStreams_);
    CHECK_AND_RETURN_RET_LOG(streamPlayingThreadPool_ != nullptr, MSERR_INVALID_VAL,
        "Failed to obtain playing ThreadPool");
    if (maxStreams_ > MAX_THREADS_NUM) {
        maxStreams_ = MAX_THREADS_NUM;
    }
    streamPlayingThreadPool_->Start(maxStreams_);
    streamPlayingThreadPool_->SetMaxTaskNum(MAX_STREAMS_TASK_NUMBER);
    isStreamPlayingThreadPoolStarted_.store(true);

    return MSERR_OK;
}

int32_t StreamIDManager::Play(std::shared_ptr<SoundParser> soundParser, PlayParams playParameters)
{
    int32_t soundID = soundParser->GetSoundID();
    int32_t streamID = GetNewStreamID(soundID, playParameters);
    {
        std::lock_guard lock(streamIDManagerLock_);
        if (streamID <= 0) {
            do {
                nextStreamID_ = nextStreamID_ == INT32_MAX ? 1 : nextStreamID_ + 1;
            } while (FindCacheBuffer(nextStreamID_) != nullptr);
            streamID = nextStreamID_;
            std::deque<std::shared_ptr<AudioBufferEntry>> cacheData;
            soundParser->GetSoundData(cacheData);
            auto cacheBuffer =
                std::make_shared<CacheBuffer>(soundParser->GetSoundTrackFormat(), cacheData, soundID, streamID);
            CHECK_AND_RETURN_RET_LOG(cacheBuffer != nullptr, -1, "failed to create cache buffer");
            CHECK_AND_RETURN_RET_LOG(callback_ != nullptr, MSERR_INVALID_VAL, "Invalid callback.");
            cacheBuffer->SetCallback(callback_);
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
    {
        std::unique_lock lock(streamIDManagerLock_);
        while (streamIDs_.size() == MAX_STREAM_ID_QUEUE) {
            if (quitQueue_) return MSERR_OK;
            MEDIA_INFO_LOG("waiting streamID:%{public}d, size:%{public}zu", streamID, streamIDs_.size());
            queueSpaceValid_.wait(lock);
        }
        if (quitQueue_) return MSERR_OK;
        streamIDs_.push_back(streamID);
        queueDataValid_.notify_one();
    }
    ThreadPool::Task streamPlayTask = std::bind(&StreamIDManager::DoPlay, this, streamID, playParameters);
    CHECK_AND_RETURN_RET_LOG(streamPlayingThreadPool_ != nullptr, MSERR_INVALID_VAL,
        "Failed to obtain stream play threadpool.");
    CHECK_AND_RETURN_RET_LOG(streamPlayTask != nullptr, MSERR_INVALID_VAL, "Failed to obtain stream play Task");
    streamPlayingThreadPool_->AddTask(streamPlayTask);
    return MSERR_OK;
}

int32_t StreamIDManager::DoPlay(const int32_t streamID, const PlayParams playParameters)
{
    MEDIA_INFO_LOG("StreamIDManager streamID:%{public}d", streamID);
    std::unique_lock lock(streamIDManagerLock_);
    std::shared_ptr<CacheBuffer> cacheBuffer = FindCacheBuffer(streamID);
    CHECK_AND_RETURN_RET_LOG(cacheBuffer.get() != nullptr, MSERR_INVALID_VAL, "cachebuffer invalid.");
    cacheBuffer->DoPlay(streamID, audioRendererInfo_, playParameters);
    return MSERR_OK;
}

std::shared_ptr<CacheBuffer> StreamIDManager::FindCacheBuffer(const int32_t streamID)
{
    if (cacheBuffers_.empty()) {
        MEDIA_INFO_LOG("StreamIDManager cacheBuffers_ empty");
        return nullptr;
    }
    if (cacheBuffers_.find(streamID) != cacheBuffers_.end()) {
        MEDIA_INFO_LOG("StreamIDManager cacheBuffers_ at : %{public}d", streamID);
        return cacheBuffers_.at(streamID);
    }
    return nullptr;
}

int32_t StreamIDManager::GetNewStreamID(const int32_t soundID, PlayParams playParameters)
{
    int32_t streamID = 0;
    if (cacheBuffers_.empty()) {
        MEDIA_INFO_LOG("StreamIDManager cacheBuffers_ empty");
        return streamID;
    }
    for (auto cacheBuffer : cacheBuffers_) {
        if (soundID == cacheBuffer.second->GetSoundID()) {
            streamID = cacheBuffer.second->GetStreamID();
            MEDIA_INFO_LOG("Have cache soundID:%{public}d, streamID:%{public}d", soundID, streamID);
            break;
        }
    }
    return streamID;
}

int32_t StreamIDManager::SetCallback(const std::shared_ptr<ISoundPoolCallback> &callback)
{
    callback_ = callback;
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
