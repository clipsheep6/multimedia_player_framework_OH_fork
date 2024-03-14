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

#include "monitor_server.h"
#include <sys/time.h>
#include <string>
#include <unistd.h>
#include "media_log.h"
#include "media_errors.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MonitorServer"};
}

namespace OHOS {
namespace Media {
const int32_t MONITOR_TIMEMS = 15000; // IPC disconnection limit 15000 ms.
constexpr uint64_t SEC_TO_MS = 1000;
constexpr uint64_t NS_TO_MS = 1000000;

MonitorServer::MonitorServer()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MonitorServer::~MonitorServer()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    threadRunning_ = false;
    std::lock_guard<std::mutex> threadLock(thredMutex_);
    if (thread_ != nullptr) {
        if (thread_->joinable()) {
            cond_.notify_all();
            thread_->join();
        }
        thread_.reset();
        thread_ = nullptr;
    }
    MEDIA_LOGI("Instances destroy end");
}

MonitorServer &MonitorServer::GetInstance()
{
    static MonitorServer instance;
    return instance;
}

int32_t MonitorServer::Dump(int32_t fd, bool needDetail)
{
    (void)needDetail;
    std::unique_lock<std::mutex> lock(mutex_);
    std::string dumpString = "------------------Monitor------------------\n";
    int32_t i = 0;
    for (auto it = objListMap_.begin(); it != objListMap_.end(); it++) {
        dumpString += "-----Instance #:" + std::to_string(i) + " pid = " + std::to_string(it->first);
        dumpString += " objsize = " + std::to_string(it->second.size());

        auto timeInfoIt = timesMap_.find(it->first);
        if (timeInfoIt != timesMap_.end()) {
            dumpString += " remainder = " + std::to_string(timeInfoIt->second.time) + "ms";
            dumpString += " alarmed = " + std::to_string(timeInfoIt->second.alarmed);
        } else {
            dumpString += " DisableMonitor";
        }

        dumpString += "\n";
        i++;
    }

    if (fd != -1) {
        write(fd, dumpString.c_str(), dumpString.size());
        dumpString.clear();
    }
    return MSERR_OK;
}

int32_t MonitorServer::Click(int32_t pid)
{
    MEDIA_LOGD("Click from %{public}d", pid);
    std::unique_lock<std::mutex> lock(mutex_);

    auto timeInfoIt = timesMap_.find(pid);
    CHECK_AND_RETURN_RET_LOG(timeInfoIt != timesMap_.end(), MSERR_OK,
        "Process %{public}d is not in the monitoring queue!", pid);

    timeInfoIt->second.time = MONITOR_TIMEMS;
    timeInfoIt->second.triggerFlag = true;

    // Wake up thread retiming wait
    waitingAgain_ = true;
    cond_.notify_all();
    return MSERR_OK;
}

int32_t MonitorServer::EnableMonitor(int32_t pid)
{
    MEDIA_LOGI("EnableMonitor from %{public}d", pid);
    {
        std::unique_lock<std::mutex> lock(mutex_);
        CHECK_AND_RETURN_RET_LOG(timesMap_.find(pid) == timesMap_.end(), MSERR_OK,
            "Process %{public}d is already in the monitoring queue!", pid);

        timesMap_.insert(std::pair<int32_t, TimeInfo>(pid, TimeInfo(MONITOR_TIMEMS, true)));

        // Wake up thread retiming wait
        waitingAgain_ = true;
        cond_.notify_all();
        if (threadRunning_) {
            return MSERR_OK;
        }
        threadRunning_ = true;
    }

    std::lock_guard<std::mutex> threadLock(thredMutex_);
    // The original thread has already exited. Need to recycle resources
    if (thread_ != nullptr) {
        if (thread_->joinable()) {
            thread_->join();
        }
        thread_.reset();
        thread_ = nullptr;
    }

    // Start Thread
    thread_ = std::make_unique<std::thread>(&MonitorServer::MonitorThread, this);

    return MSERR_OK;
}

int32_t MonitorServer::DisableMonitor(int32_t pid)
{
    MEDIA_LOGI("DisableMonitor from %{public}d", pid);
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET_LOG(timesMap_.find(pid) != timesMap_.end(), MSERR_OK,
        "Process %{public}d is not in the monitoring queue!", pid);

    timesMap_.erase(pid);
    
    // Exit Thread
    if (timesMap_.empty()) {
        cond_.notify_all();
    }

    return MSERR_OK;
}

int32_t MonitorServer::RegisterObj(int32_t pid, wptr<MonitorServerObject> obj)
{
    MEDIA_LOGI("pid %{public}d obj 0x%{public}06" PRIXPTR " Register", pid, FAKE_POINTER(obj.GetRefPtr()));
    std::unique_lock<std::mutex> lock(mutex_);

    auto objListIt = objListMap_.find(pid);
    if (objListIt != objListMap_.end()) {
        MEDIA_LOGI("The pid %{public}d has already been registered", pid);
        auto objIt = std::find(objListIt->second.begin(), objListIt->second.end(), obj);
        CHECK_AND_RETURN_RET_LOG(objIt == objListIt->second.end(), MSERR_OK, "The obj has already been registered");

        // Add obj to monitoring queue
        objListIt->second.push_back(obj);
        return MSERR_OK;
    }

    // Add pid and obj to the monitoring queue
    std::list<wptr<MonitorServerObject>> objList;
    objList.push_back(obj);
    objListMap_[pid] = objList;

    return MSERR_OK;
}

int32_t MonitorServer::CancellationObj(int32_t pid, wptr<MonitorServerObject> obj)
{
    MEDIA_LOGI("pid %{public}d obj 0x%{public}06" PRIXPTR " Cancellation", pid, FAKE_POINTER(obj.GetRefPtr()));
    std::unique_lock<std::mutex> lock(mutex_);

    auto objListIt = objListMap_.find(pid);
    CHECK_AND_RETURN_RET_LOG(objListIt != objListMap_.end(), MSERR_OK, "This pid has not been registered");

    auto objIt = std::find(objListIt->second.begin(), objListIt->second.end(), obj);
    CHECK_AND_RETURN_RET_LOG(objIt != objListIt->second.end(), MSERR_OK, "The obj has not been registered");

    // Remove obj from monitoring queue
    objListIt->second.erase(objIt);

    // Remove pid from monitoring queue
    if (objListIt->second.empty()) {
        objListMap_.erase(objListIt);
    }

    return MSERR_OK;
}

int32_t MonitorServer::OnClientDie(int32_t pid)
{
    MEDIA_LOGI("pid %{public}d OnClientDie", pid);
    std::unique_lock<std::mutex> lock(mutex_);
    objListMap_.erase(pid);
    timesMap_.erase(pid);

    // Exit Thread
    if (timesMap_.empty()) {
        cond_.notify_all();
    }

    return MSERR_OK;
}

uint64_t MonitorServer::GetTimeMS()
{
    struct timespec timestamp = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    return timestamp.tv_sec * SEC_TO_MS + timestamp.tv_nsec / NS_TO_MS;
}

int32_t MonitorServer::ObjCtrl(std::list<wptr<MonitorServerObject>> &recoveryList,
    std::list<wptr<MonitorServerObject>> &abnormalList)
{
    for (auto objIt = recoveryList.begin(); objIt != recoveryList.end(); objIt++) {
        sptr<MonitorServerObject> obj = objIt->promote();
        CHECK_AND_CONTINUE(obj != nullptr);
        (void)obj->IpcRecovery(true);
    }

    for (auto objIt = abnormalList.begin(); objIt != abnormalList.end(); objIt++) {
        sptr<MonitorServerObject> obj = objIt->promote();
        CHECK_AND_CONTINUE(obj != nullptr);
        (void)obj->IpcAbnormality();
    }

    return MSERR_OK;
}

int32_t MonitorServer::GetObjListByPid(int32_t pid, std::list<wptr<MonitorServerObject>> &list)
{
    auto objListIt = objListMap_.find(pid);
    CHECK_AND_RETURN_RET_LOG(objListIt != objListMap_.end(), MSERR_OK,
        "The pid %{public}d is not in the objList", pid);

    for (auto objIt = objListIt->second.begin(); objIt != objListIt->second.end(); objIt++) {
        list.push_back(*objIt);
    }

    return MSERR_OK;
}

int32_t MonitorServer::GetWaitTime()
{
    int32_t minTime = MONITOR_TIMEMS;
    for (auto timeInfoIt = timesMap_.begin(); timeInfoIt != timesMap_.end(); timeInfoIt++) {
        if (timeInfoIt->second.time > 0 && timeInfoIt->second.time < minTime) {
            minTime = timeInfoIt->second.time;
        }
    }
    return minTime;
}

void MonitorServer::MonitorThread()
{
    uint64_t timeStart;
    int32_t waitTime;
    MEDIA_LOGI("MonitorThread start");
    pthread_setname_np(pthread_self(), "MonitorServer");

    while (true) {
        std::list<wptr<MonitorServerObject>> recoveryList;
        std::list<wptr<MonitorServerObject>> abnormalList;
        {
            std::unique_lock<std::mutex> lock(mutex_);

            timeStart = GetTimeMS();
            cond_.wait_for(lock, std::chrono::milliseconds(GetWaitTime()), [this] {
                return timesMap_.empty() || waitingAgain_ || !threadRunning_;
            });
            
            if (!threadRunning_) {
                MEDIA_LOGI("Stop running and exit loop.");
                break;
            }

            if (timesMap_.empty()) {
                MEDIA_LOGI("TimesMap empty. MonitorThread Stop.");
                threadRunning_ = false;
                break;
            }

            waitTime = static_cast<int32_t>(GetTimeMS() - timeStart);

            for (auto it = timesMap_.begin(); it != timesMap_.end(); it++) {
                MEDIA_LOGD("pid %{public}d, waitTime %{public}d, timeout %{public}d, trigger %{public}d",
                    it->first, waitTime, it->second.time, it->second.triggerFlag);
                if (it->second.triggerFlag == true) {
                    it->second.triggerFlag = false;
                } else {
                    it->second.time -= waitTime;
                }

                if (it->second.alarmed == true && it->second.time > 0) {
                    it->second.alarmed = false;
                    (void)GetObjListByPid(it->first, recoveryList);
                    MEDIA_LOGW("%{public}d Recovery", it->first);
                }

                if (it->second.time <= 0 && it->second.alarmed == false) {
                    it->second.alarmed = true;
                    (void)GetObjListByPid(it->first, abnormalList);
                    MEDIA_LOGW("%{public}d Abnormality", it->first);
                }
            }
            waitingAgain_ = false;
        }
        (void)ObjCtrl(recoveryList, abnormalList);
    }
    MEDIA_LOGI("MonitorThread end");
}
} // namespace Media
} // namespace OHOS
