/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef PLAYER_XCOLLIE_H
#define PLAYER_XCOLLIE_H

#include <string>
#include <map>
#include <mutex>

namespace OHOS {
namespace Media {
class __attribute__((visibility("default"))) PlayerXCollie {
public:
    static PlayerXCollie &GetInstance();
    PlayerXCollie() = default;
    PlayerXCollie(const std::string &name, bool recovery = false, uint32_t timeout = 10)
    {
        id_ = SetTimer(name, recovery, timeout);
    };

    PlayerXCollie(const std::string &name, uint32_t timeout = 10)
    {
        id_ = SetTimerByLog(name, timeout);
    }
    int32_t Dump(int32_t fd);
    
    ~PlayerXCollie()
    {
        CancelTimer(id_);
    }
    constexpr static uint32_t timerTimeout = 10;
private:
    int32_t SetTimer(const std::string &name, bool recovery = false, uint32_t timeout = 10); // 10s
    int32_t SetTimerByLog(const std::string &name, uint32_t timeout = 10); // 10s
    void CancelTimer(int32_t id);
    void TimerCallback(void *data);

    std::mutex mutex_;
    std::map<int32_t, std::string> dfxDumper_;
    uint32_t threadDeadlockCount_ = 0;
    int32_t id_ = 0;
};

#define Listener(statement, args...) { PlayerXCollie xCollie(args); statement; }
} // namespace Media
} // namespace OHOS
#endif