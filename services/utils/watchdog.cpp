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
#include <thread>

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
    std::unique_lock<std::mutex> lock(mutex_);
    if (enable_.load()) {
        return;
    }

    enable_.store(true);
    thread_ = std::make_unique<std::thread>(&WatchDog::WatchDogThread, this);
};

void WatchDog::DisableWatchDog()
{
    std::unique_lock<std::mutex> lock(mutex_);
    count_++; // Prevent accidental touch of alarm().
    enable_.store(false);
    cond_.notify_all();
    pauseCond_.notify_all();
    if (thread_ != nullptr && thread_->joinable()) {
        thread_->join();
        thread_.reset();
        thread_ = nullptr;
    }

    // It may be changed by thread. Assign value after thread recycle.
    pause_.store(false);
    alarmed_ = false;
};

void WatchDog::PauseWatchDog()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (enable_.load()) {
        count_++; // Prevent accidental touch of alarm().
        pause_.store(true);
        cond_.notify_all();
    }
};

void WatchDog::ResumeWatchDog()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (enable_.load()) {
        count_++; // Prevent accidental touch of alarm().
        pause_.store(false);
        pauseCond_.notify_all();
    }
};

void WatchDog::SetWatchDogTimeout(uint32_t timeoutMs)
{
    std::unique_lock<std::mutex> condLock(condMutex_);
    timeoutMs_ = timeoutMs;
};

void WatchDog::Notify()
{
    std::unique_lock<std::mutex> alarmLock(alarmMutex_);
    count_++;
    cond_.notify_all();

    if (alarmed_) {
        alarmed_ = false;
        AlarmRecovery();
        pause_.store(false);
        pauseCond_.notify_all();
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
        {
            std::unique_lock<std::mutex> condLock(condMutex_);
            cond_.wait_for(condLock, std::chrono::milliseconds(timeoutMs_), [this] {
                return (enable_.load() == false) || (pause_.load() == true) || (count_.load() > 0);
            });
        }

        if (enable_.load() == false) {
            break;
        }

        {
            std::unique_lock<std::mutex> alarmLock(alarmMutex_);
            if ((count_.load() == 0) && (pause_.load() == false)) {
                MEDIA_LOGI("Watchdog timeout!");
                if (alarmed_ == false) {
                    alarmed_ = true;
                    Alarm();
                    pause_.store(true);
                }
            }
        }

        count_.store(0);

        if (pause_.load()) {
            std::unique_lock<std::mutex> pauseLock(pauseMutex_);
            pauseCond_.wait(pauseLock, [this] {
                return (enable_.load() == false) || (pause_.load() == false);
            });
        }
    }
}
} // namespace Media
} // namespace OHOS
