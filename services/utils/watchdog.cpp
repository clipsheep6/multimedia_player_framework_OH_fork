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

#include <mutex>
#include <thread>
#include "watchdog.h"

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
    if (enable_.load()) {
        return;
    }

    DisableWatchDog();
    enable_.store(true);
    thread_ = std::make_unique<std::thread>(&WatchDog::WatchDogThread, this);
};

void WatchDog::DisableWatchDog()
{
    enable_.store(false);
    pause_.store(false);
    alarmed_.store(false);
    cond_.notify_all();
    pauseCond_.notify_all();
    if (thread_ != nullptr && thread_->joinable()) {
        thread_->join();
        thread_.reset();
        thread_ = nullptr;
    }
};

void WatchDog::PauseWatchDog()
{
    pause_.store(true);
    cond_.notify_all();
};

void WatchDog::ResumeWatchDog()
{
    pause_.store(false);
    pauseCond_.notify_all();
};

void WatchDog::SetWatchDogTimeout(uint32_t timeoutMs)
{
    timeoutMs_ = timeoutMs;
};

void WatchDog::Notify()
{
    count_++;
    cond_.notify_all();
    if (IsWatchDogAlarmed()) {
        AlarmRecovery();
    }
};

void WatchDog::SetWatchDogAlarmed(bool alarmed)
{
    alarmed_.store(alarmed);
};

bool WatchDog::IsWatchDogAlarmed()
{
    return alarmed_.load();
};

void WatchDog::Alarm()
{
    SetWatchDogAlarmed(true);
};

void WatchDog::AlarmRecovery()
{
    SetWatchDogAlarmed(false);
};

void WatchDog::WatchDogThread()
{
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait_for(lock, std::chrono::milliseconds(timeoutMs_), [this] {
            return (enable_.load() == false) || (pause_.load() == true) || (count_.load() > 0);
        });

        if (enable_.load() == false) {
            break;
        }

        if ((count_.load() == 0) && (pause_.load() == false)) {
            MEDIA_LOGI("Alarm!");
            Alarm();
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
