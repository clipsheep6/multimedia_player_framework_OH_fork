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
    std::lock_guard<std::mutex> cmdLock(cmdMutex_);
    std::unique_lock<std::mutex> threadLock(threadMutex_);
    enableThread_ = false;
    if (thread_ != nullptr && thread_->joinable()) {
        MEDIA_LOGI("clear monitor server thread");
        cond_.notify_all();
        threadLock.unlock();
        thread_->join();
        thread_.reset();
        thread_ = nullptr;
    }
}

MonitorServer &MonitorServer::GetInstance()
{
    static MonitorServer instance;
    return instance;
}

int32_t MonitorServer::Dump(int32_t fd, bool needDetail)
{
    (void)needDetail;
    std::lock_guard<std::mutex> cmdLock(cmdMutex_);
    std::unique_lock<std::mutex> threadLock(threadMutex_);
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
    MEDIA_LOGI("Click from %{public}d", pid);
    std::lock_guard<std::mutex> cmdLock(cmdMutex_);
    std::unique_lock<std::mutex> threadLock(threadMutex_);

    auto timeInfoIt = timesMap_.find(pid);
    CHECK_AND_RETURN_RET_LOG(timeInfoIt != timesMap_.end(), MSERR_OK,
        "Process %{public}d is not in the monitoring queue!", pid);

    if (timeInfoIt->second.alarmed) {
        timeInfoIt->second.alarmed = false;
        Recovery(pid);
    }

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
    std::lock_guard<std::mutex> cmdLock(cmdMutex_);
    std::unique_lock<std::mutex> threadLock(threadMutex_);

    CHECK_AND_RETURN_RET_LOG(timesMap_.find(pid) == timesMap_.end(), MSERR_OK,
        "Process %{public}d is already in the monitoring queue!", pid);

    // Update timeout
    timesMap_.insert(std::pair<int32_t, TimeInfo>(pid, TimeInfo(MONITOR_TIMEMS, true)));

    // Wake up thread retiming wait
    waitingAgain_ = true;
    cond_.notify_all();

    // Start Thread
    if (timesMap_.size() > 0 && thread_ == nullptr) {
        enableThread_ = true;
        thread_ = std::make_unique<std::thread>(&MonitorServer::MonitorThread, this);
    }

    return MSERR_OK;
}

int32_t MonitorServer::DisableMonitor(int32_t pid)
{
    MEDIA_LOGI("DisableMonitor from %{public}d", pid);
    std::lock_guard<std::mutex> cmdLock(cmdMutex_);
    std::unique_lock<std::mutex> threadLock(threadMutex_);

    CHECK_AND_RETURN_RET_LOG(timesMap_.find(pid) != timesMap_.end(), MSERR_OK,
        "Process %{public}d is not in the monitoring queue!", pid);

    timesMap_.erase(pid);
    
    // Exit Thread
    if (timesMap_.empty()) {
        enableThread_ = false;
        if (thread_ != nullptr && thread_->joinable()) {
            cond_.notify_all();
            threadLock.unlock();
            thread_->join();
            thread_.reset();
            thread_ = nullptr;
        }
    }

    return MSERR_OK;
}

int32_t MonitorServer::RegisterObj(int32_t pid, wptr<MonitorServerObject> obj)
{
    MEDIA_LOGI("pid %{public}d obj 0x%{public}06" PRIXPTR " Register", pid, FAKE_POINTER(obj.GetRefPtr()));
    std::lock_guard<std::mutex> cmdLock(cmdMutex_);
    std::unique_lock<std::mutex> threadLock(threadMutex_);

    auto objListIt = objListMap_.find(pid);
    if (objListIt != objListMap_.end()) {
        MEDIA_LOGI("The pid has already been registered");
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
    std::lock_guard<std::mutex> cmdLock(cmdMutex_);
    std::unique_lock<std::mutex> threadLock(threadMutex_);
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
    std::lock_guard<std::mutex> cmdLock(cmdMutex_);
    std::unique_lock<std::mutex> threadLock(threadMutex_);
    objListMap_.erase(pid);
    timesMap_.erase(pid);

    // Exit Thread
    if (timesMap_.empty()) {
        enableThread_ = false;
        if (thread_ != nullptr && thread_->joinable()) {
            cond_.notify_all();
            threadLock.unlock();
            thread_->join();
            thread_.reset();
            thread_ = nullptr;
        }
    }

    return MSERR_OK;
}

uint64_t MonitorServer::GetTimeMS()
{
    struct timespec timestamp = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    return timestamp.tv_sec * SEC_TO_MS + timestamp.tv_nsec / NS_TO_MS;
}

int32_t MonitorServer::Abnormality(int32_t pid)
{
    MEDIA_LOGE("%{public}d Abnormality", pid);
    auto objListIt = objListMap_.find(pid);
    CHECK_AND_RETURN_RET_LOG(objListIt != objListMap_.end(), MSERR_OK,
        "The pid %{public}d is not in the objList", pid);

    for (auto objIt = objListIt->second.begin(); objIt != objListIt->second.end(); objIt++) {
        sptr<MonitorServerObject> obj = objIt->promote();
        (void)obj->IpcAbnormality();
    }

    return MSERR_OK;
}

int32_t MonitorServer::Recovery(int32_t pid)
{
    MEDIA_LOGE("%{public}d Recovery", pid);
    auto objListIt = objListMap_.find(pid);
    CHECK_AND_RETURN_RET_LOG(objListIt != objListMap_.end(), MSERR_OK,
        "The pid %{public}d is not in the objList", pid);

    for (auto objIt = objListIt->second.begin(); objIt != objListIt->second.end(); objIt++) {
        sptr<MonitorServerObject> obj = objIt->promote();
        (void)obj->IpcRecovery(true);
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
        std::unique_lock<std::mutex> threadLock(threadMutex_);

        timeStart = GetTimeMS();
        cond_.wait_for(threadLock, std::chrono::milliseconds(GetWaitTime()), [this] {
            return !enableThread_ || waitingAgain_;
        });

        CHECK_AND_BREAK_LOG(enableThread_, "MonitorThread Stop.");

        waitTime = static_cast<int32_t>(GetTimeMS() - timeStart);

        for (auto it = timesMap_.begin(); it != timesMap_.end(); it++) {
            MEDIA_LOGI("pid %{public}d, waitTime %{public}d, timeout %{public}d, trigger %{public}d",
                it->first, waitTime, it->second.time, it->second.triggerFlag);
            if (it->second.triggerFlag == true) {
                it->second.triggerFlag = false;
            } else {
                it->second.time -= waitTime;
            }

            if (it->second.time <= 0 && it->second.alarmed == false) {
                it->second.alarmed = true;
                (void)Abnormality(it->first);
            }
        }
        waitingAgain_ = false;
    }
    MEDIA_LOGI("MonitorThread end");
}
} // namespace Media
} // namespace OHOS
