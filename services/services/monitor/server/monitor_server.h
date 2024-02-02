/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
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

#ifndef MONITOR_SERVICE_H
#define MONITOR_SERVICE_H

#include <list>
#include <map>
#include <thread>
#include "monitor_server_object.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class MonitorServer : public NoCopyable {
public:
    MonitorServer();
    ~MonitorServer();

    static MonitorServer &GetInstance();
    int32_t Click(int32_t pid);
    int32_t EnableMonitor(int32_t pid);
    int32_t DisableMonitor(int32_t pid);

    int32_t RegisterObj(int32_t pid, wptr<MonitorServerObject> obj);
    int32_t CancellationObj(int32_t pid, wptr<MonitorServerObject> obj);

    int32_t OnClientDie(int32_t pid);
    int32_t Dump(int32_t fd, bool needDetail);

private:
    uint64_t GetTimeMS();
    void MonitorThread();
    int32_t GetWaitTime();
    int32_t GetObjListByPid(int32_t pid, std::list<wptr<MonitorServerObject>> &list);
    int32_t ObjCtrl(std::list<wptr<MonitorServerObject>> &recoveryList,
        std::list<wptr<MonitorServerObject>> &abnormalList);

    struct TimeInfo {
        TimeInfo(int32_t t, bool flag)
            : time(t), triggerFlag(flag), alarmed(false)
        {
        }
        int32_t time;
        bool triggerFlag;
        bool alarmed;
    };
 
    bool waitingAgain_ = false;
    std::map<int32_t, std::list<wptr<MonitorServerObject>>> objListMap_;
    std::multimap<int32_t, TimeInfo> timesMap_;
    std::mutex mutex_;
    std::mutex thredMutex_;
    std::unique_ptr<std::thread> thread_ = nullptr;
    bool threadRunning_ = false;
    std::condition_variable cond_;
};
} // namespace Media
} // namespace OHOS
#endif // MONITOR_SERVICE_H
