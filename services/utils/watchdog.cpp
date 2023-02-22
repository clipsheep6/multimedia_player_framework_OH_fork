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

#include "watchdog.h"
#include <mutex>

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "watchdog"};
}

namespace OHOS {
namespace Media {
WatchDog::~WatchDog()
{
    DisableWatchDog();
}

void WatchDog::EnableWatchDog()
{
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (enable_) {
            return;
        }

        if (disabling.load()) {
            continue; // Wait for disable execution to finish.
        }

        enable_ = true;
        thread_ = std::make_unique<std::thread>(&WatchDog::WatchDogThread, this);
        break;
    }
};

void WatchDog::DisableWatchDog()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (disabling.load() == false) {
        disabling.store(true);
        enable_ = false;
        cond_.notify_all();
        lock.unlock(); // Make the thread acquire the lock and exit.
        if (thread_ != nullptr && thread_->joinable()) {
            thread_->join();
            thread_.reset();
            thread_ = nullptr;
        }

        // It may be changed by thread. Assign value after thread recycle.
        pause_ = false;
        alarmed_ = false;
        disabling.store(false);
    }
};

void WatchDog::PauseWatchDog()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (enable_) {
        pause_ = true;
        paused_ = true;
        cond_.notify_all();
    }
};

void WatchDog::ResumeWatchDog()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (enable_) {
        pause_ = false;
        cond_.notify_all();
    }
};

void WatchDog::SetWatchDogTimeout(uint32_t timeoutMs)
{
    std::unique_lock<std::mutex> lock(mutex_);
    timeoutMs_ = timeoutMs;
};

void WatchDog::Notify()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (enable_) {
        if (alarmed_) {
            alarmed_ = false;
            AlarmRecovery();
            pause_ = false;
            cond_.notify_all();
        }

        count_++;
        cond_.notify_all();
    }
};

void WatchDog::Alarm()
{
    MEDIA_LOGI("Alarm!");
};

void WatchDog::AlarmRecovery()
{
    MEDIA_LOGI("AlarmRecovery!");
};

void WatchDog::WatchDogThread()
{
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // For pause/resume control, wait only when paused.
        cond_.wait(lock, [this] {
            return (enable_ == false) || (pause_ == false);
        });

        if (paused_) {
            paused_ = false;
        }

        // For timeout detection.
        cond_.wait_for(lock, std::chrono::milliseconds(timeoutMs_), [this] {
            return (enable_ == false) || (pause_ == true) || (count_ > 0);
        });

        if (enable_ == false) {
            break;
        }

        if (pause_ == true || paused_ == true) {
            continue;
        }
        
        if (count_ == 0) {
            MEDIA_LOGI("Watchdog timeout!");
            if (alarmed_ == false) {
                alarmed_ = true;
                Alarm();
                pause_ = true;
            }
        }

        count_ = 0;
    }
}
} // namespace Media
} // namespace OHOS
