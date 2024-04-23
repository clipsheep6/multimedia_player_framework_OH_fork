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
#include "media_log.h"
#include "media_errors.h"
#include "parameter.h"
#include "string_ex.h"
#include "sound_id_manager.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SoundIDManager"};
    static const std::string THREAD_POOL_NAME = "SoundParserThreadPool";
    static const int32_t MAX_THREADS_NUM = std::thread::hardware_concurrency() >= 4 ? 2 : 1;
}

namespace OHOS {
namespace Media {
SoundIDManager::SoundIDManager() : isParsingThreadPoolStarted_(false), quitQueue_(false)
{
    MEDIA_LOGI("Construction SoundIDManager");
    InitThreadPool();
}

SoundIDManager::~SoundIDManager()
{
    MEDIA_LOGI("Destruction SoundIDManager");
    {
        std::lock_guard lock(soundManagerLock_);
        quitQueue_ = true;
        queueSpaceValid_.notify_all(); // notify all load waiters
        queueDataValid_.notify_all();  // notify all worker threads
    }

    if (callback_ != nullptr) {
        callback_.reset();
    }
    for (auto soundParser : soundParsers_) {
        if (soundParser.second != nullptr) {
            soundParser.second->Release();
        }
    }
    soundParsers_.clear();

    if (isParsingThreadPoolStarted_) {
        if (soundParserThreadPool_ != nullptr) {
            soundParserThreadPool_->Stop();
        }
        isParsingThreadPoolStarted_ = false;
    }
}

int32_t SoundIDManager::InitThreadPool()
{
    if (isParsingThreadPoolStarted_) {
        return MSERR_OK;
    }
    soundParserThreadPool_ = std::make_unique<ThreadPool>(THREAD_POOL_NAME);
    CHECK_AND_RETURN_RET_LOG(soundParserThreadPool_ != nullptr, MSERR_INVALID_VAL, "Failed to obtain ThreadPool");
    soundParserThreadPool_->Start(MAX_THREADS_NUM);
    isParsingThreadPoolStarted_ = true;

    return MSERR_OK;
}

int32_t SoundIDManager::Load(std::string url)
{
    int32_t soundID;
    {
        std::lock_guard lock(soundManagerLock_);
        if (soundParsers_.size() >= MAX_LOAD_NUM) {
            MEDIA_LOGI("SoundPool MAX_LOAD_NUM:%{public}zu.", MAX_LOAD_NUM);
            return invalidSoundIDFlag;
        }
        const std::string fdHead = "fd://";
        if (url.find(fdHead) == std::string::npos) {
            return invalidSoundIDFlag;
        }
        int32_t fd = -1;
        StrToInt(url.substr(fdHead.size()), fd);
        if (fd < 0) {
            return invalidSoundIDFlag;
        }
        do {
            nextSoundID_ = nextSoundID_ == INT32_MAX ? 1 : nextSoundID_ + 1;
        } while (FindSoundParser(nextSoundID_) != nullptr);
        soundID = nextSoundID_;
        auto soundParser = std::make_shared<SoundParser>(soundID, url);
        CHECK_AND_RETURN_RET_LOG(soundParser != nullptr, -1, "failed to create soundParser");
        soundParsers_.emplace(soundID, soundParser);
    }
    DoLoad(soundID);
    return soundID;
}

int32_t SoundIDManager::Load(int32_t fd, int64_t offset, int64_t length)
{
    int32_t soundID;
    {
        std::lock_guard lock(soundManagerLock_);
        MEDIA_LOGI("SoundIDManager startLoad");
        if (soundParsers_.size() >= MAX_LOAD_NUM) {
            MEDIA_LOGI("SoundPool MAX_LOAD_NUM:%{public}zu.", MAX_LOAD_NUM);
            return invalidSoundIDFlag;
        }
        do {
            nextSoundID_ = nextSoundID_ == INT32_MAX ? 1 : nextSoundID_ + 1;
        } while (FindSoundParser(nextSoundID_) != nullptr);
        soundID = nextSoundID_;
        auto soundParser = std::make_shared<SoundParser>(soundID, fd, offset, length);
        CHECK_AND_RETURN_RET_LOG(soundParser != nullptr, -1, "failed to create soundParser");
        soundParsers_.emplace(soundID, soundParser);
    }
    DoLoad(soundID);
    return soundID;
}

int32_t SoundIDManager::DoLoad(int32_t soundID)
{
    MEDIA_LOGI("SoundIDManager soundID:%{public}d", soundID);
    if (!isParsingThreadPoolStarted_) {
        InitThreadPool();
    }
    {
        std::unique_lock lock(soundManagerLock_);
        while (soundIDs_.size() == MAX_SOUND_ID_QUEUE) {
            if (quitQueue_) return MSERR_OK;
            queueSpaceValid_.wait(lock);
        }
        if (quitQueue_) return MSERR_OK;
        soundIDs_.push_back(soundID);
        queueDataValid_.notify_one();
    }
    ThreadPool::Task soundParsingTask = std::bind(&SoundIDManager::DoParser, this);
    CHECK_AND_RETURN_RET_LOG(soundParserThreadPool_ != nullptr, MSERR_INVALID_VAL, "Failed to obtain ThreadPool");
    CHECK_AND_RETURN_RET_LOG(soundParsingTask != nullptr, MSERR_INVALID_VAL, "Failed to obtain Task");
    soundParserThreadPool_->AddTask(soundParsingTask);
    return MSERR_OK;
}

int32_t SoundIDManager::DoParser()
{
    std::unique_lock lock(soundManagerLock_);
    while (!quitQueue_) {
        if (soundIDs_.empty()) {
            queueDataValid_.wait_for(
                lock, std::chrono::duration<int32_t, std::milli>(WAIT_TIME_BEFORE_CLOSE_MS));
            if (soundIDs_.empty()) {
                // no new sound, exit this thread.
                break;
            }
            continue;
        }
        const int32_t soundID = soundIDs_.front();
        soundIDs_.pop_front();
        queueSpaceValid_.notify_one();
        lock.unlock();
        std::shared_ptr<SoundParser> soundParser = FindSoundParser(soundID);
        if (soundParser.get() != nullptr) {
            soundParser->SetCallback(callback_);
            soundParser->DoParser();
        }
        lock.lock();
    }
    return MSERR_OK;
}


std::shared_ptr<SoundParser> SoundIDManager::FindSoundParser(int32_t soundID) const
{
    MEDIA_LOGI("SoundIDManager soundID:%{public}d", soundID);
    if (soundParsers_.empty()) {
        return nullptr;
    }
    if (soundParsers_.find(soundID) != soundParsers_.end()) {
        return soundParsers_.at(soundID);
    }
    return nullptr;
}

int32_t SoundIDManager::Unload(int32_t soundID)
{
    MEDIA_LOGI("SoundIDManager soundID:%{public}d", soundID);
    CHECK_AND_RETURN_RET_LOG(!soundParsers_.empty(), MSERR_NO_MEMORY, "No sound in the soundParsers_");
    auto it = soundParsers_.find(soundID);
    if (it != soundParsers_.end()) {
        if (it->second != nullptr) {
            it->second.reset();
        }
        soundParsers_.erase(it);
    } else {
        MEDIA_LOGI("Invalid soundID, unload failed");
        return MSERR_INVALID_VAL;
    }
    return MSERR_OK;
}

int32_t SoundIDManager::SetCallback(const std::shared_ptr<ISoundPoolCallback> &callback)
{
    callback_ = callback;
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
