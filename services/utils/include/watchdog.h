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
#include <mutex>
#include "media_dfx.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
// If Notify() is not used to trigger the dog feeding action for more than timeoutMs_, the Alarm() action is triggered.
class __attribute__((visibility("default"))) WatchDog {
public:
    WatchDog() = default;
    ~WatchDog();

    bool IsWatchDogEnable();
    void EnableWatchDog();
    void DisableWatchDog();
    void PauseWatchDog();
    void ResumeWatchDog();
    void SetTimeout(int32_t timeoutMs);
    void Notify();
    void WatchDogThread();
    virtual void Alarm() = 0;

private:
    std::atomic<bool> enable_ = false;
    std::atomic<bool> pause_ = false;
    int32_t timeoutMs_ = 0;
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
