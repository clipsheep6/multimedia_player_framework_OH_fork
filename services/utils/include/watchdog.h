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

#ifndef TIME_PERF_H
#define TIME_PERF_H

#include <atomic>
#include <condition_variable>
#include "media_dfx.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
/**
 * More than timeoutMs_ If Notify() is not used to trigger the dog feeding action within, the Alarm() action
 * will be triggered. When notify() is restored, the AlarmRecovery() action will be triggered.
 * See interface details for specific usage.
 */
class __attribute__((visibility("default"))) WatchDog {
public:
    WatchDog() = default;
    WatchDog(uint32_t timeoutMs) : timeoutMs_(timeoutMs) {};
    ~WatchDog();

    /**
     * Create a listening thread. Repeated calls do not take effect.
     */
    void EnableWatchDog();

    /**
     * End and destroy the listening thread.
     */
    void DisableWatchDog();

    /**
     * The listening thread enters the paused state (semaphore waiting).
     */
    void PauseWatchDog();

    /**
     * The listening thread resumes running and starts a new round of timeout waiting.
     */
    void ResumeWatchDog();

    /**
     * Watchdog feeding action.
     * It needs to be called regularly within the timeoutMs_ time, otherwise Alarm() will be triggered.
     * When the alarmed_ is true, AlarmRecovery() will be triggered.
     */
    void Notify();

    /**
     * Set the alarmed_. When the alarmed_ is true, the notify() interface will trigger AlarmRecovery().
     */
    void SetWatchDogAlarmed(bool alarmed);

    /**
     * The watchdog timeout will trigger this event. Please inherit and override the interface.
     */
    virtual void Alarm();

    /**
     * This event will be triggered when the watchdog is recovered and the alarmed_ is true.
     */
    virtual void AlarmRecovery();

    /**
     * TThe thread used to monitor the watchdog.
     */
    void WatchDogThread();

    void SetWatchDogTimeout(uint32_t timeoutMs);
    bool IsWatchDogAlarmed();

private:
    std::atomic<bool> enable_ = false;
    std::atomic<bool> pause_ = false;
    std::atomic<bool> alarmed_ = false;
    uint32_t timeoutMs_ = 1000; // Default 1000ms.
    std::atomic<uint32_t> count_ = 0;
    std::condition_variable cond_;
    std::mutex mutex_;
    std::condition_variable pauseCond_;
    std::mutex pauseMutex_;
    std::unique_ptr<std::thread> thread_;
};
} // namespace Media
} // namespace OHOS

#endif
