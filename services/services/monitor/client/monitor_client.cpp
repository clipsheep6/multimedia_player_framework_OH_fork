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

#include "monitor_client.h"
#include "i_media_service.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MonitorClient"};
}

namespace OHOS {
namespace Media {
MonitorClient::MonitorClient()
{
    MEDIA_LOGI("Instances create");
}

MonitorClient::~MonitorClient()
{
    MEDIA_LOGI("Instances destroy");
}

MonitorClient &MonitorClient::GetInstance()
{
    static MonitorClient monitor;
    return monitor;
}

bool MonitorClient::IsVaildProxy()
{
    if (monitorProxy_ == nullptr) {
        monitorProxy_ = MediaServiceFactory::GetInstance().GetMonitorProxy();
    }
    CHECK_AND_RETURN_RET_LOG(monitorProxy_ != nullptr, false, "monitorProxy_ is nullptr!");

    return true;
}

int32_t MonitorClient::StartClick(MonitorClientObject *obj)
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " StartClick", FAKE_POINTER(obj));
    std::lock_guard<std::mutex> cmdLock(cmdMutex_);
    std::lock_guard<std::mutex> threadLock(threadMutex_);
    CHECK_AND_RETURN_RET_LOG(objSet_.count(obj) == 0, MSERR_OK, "It has already been activated");

    CHECK_AND_RETURN_RET_LOG(IsVaildProxy(), MSERR_INVALID_OPERATION, "Proxy is invaild!");
    objSet_.insert(obj);
    if (objSet_.size() > 0 && clickThread_ == nullptr) {
        enableThread_ = true;
        clickThread_ = std::make_unique<std::thread>(&MonitorClient::ClickThread, this);

        CHECK_AND_RETURN_RET_LOG(monitorProxy_ != nullptr, MSERR_INVALID_OPERATION, "Proxy is invaild!");
        (void)monitorProxy_->EnableMonitor();
    }

    return MSERR_OK;
}

int32_t MonitorClient::StopClick(MonitorClientObject *obj)
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " StopClick", FAKE_POINTER(obj));
    std::lock_guard<std::mutex> cmdLock(cmdMutex_);
    std::unique_lock<std::mutex> threadLock(threadMutex_);
    CHECK_AND_RETURN_RET_LOG(objSet_.count(obj), MSERR_OK, "Not started");

    objSet_.erase(obj);
    if (objSet_.empty()) {
        CHECK_AND_RETURN_RET_LOG(monitorProxy_ != nullptr, MSERR_OK, "Proxy is invaild!");
        (void)monitorProxy_->DisableMonitor();

        enableThread_ = false;
        if (clickThread_ != nullptr && clickThread_->joinable()) {
            clickCond_.notify_all();
            threadLock.unlock();
            clickThread_->join();
            clickThread_.reset();
            clickThread_ = nullptr;
        }
    }

    return MSERR_OK;
}

void MonitorClient::MediaServerDied()
{
    MEDIA_LOGI("MediaServerDied");
    std::lock_guard<std::mutex> cmdLock(cmdMutex_);
    std::unique_lock<std::mutex> threadLock(threadMutex_);
    objSet_.clear();
    enableThread_ = false;
    monitorProxy_ = nullptr;

    if (clickThread_ != nullptr && clickThread_->joinable()) {
        clickCond_.notify_all();
        threadLock.unlock();
        clickThread_->join();
        clickThread_.reset();
        clickThread_ = nullptr;
    }
}

void MonitorClient::ClickThread()
{
    pthread_setname_np(pthread_self(), "MonitorClick");
    MEDIA_LOGI("ClickThread start");
    static constexpr uint8_t timeInterval = 1; // Heartbeat once per second

    while (true) {
        std::unique_lock<std::mutex> threadLock(threadMutex_);
        clickCond_.wait_for(threadLock, std::chrono::seconds(timeInterval), [this] { return !enableThread_; });

        CHECK_AND_BREAK_LOG(enableThread_, "ClickThread Stop.");
        CHECK_AND_BREAK_LOG(monitorProxy_ != nullptr, "Proxy is invaild!");
        CHECK_AND_BREAK_LOG(monitorProxy_->Click() == MSERR_OK, "failed to Click");
    }
    MEDIA_LOGI("ClickThread End");
}
} // namespace Media
} // namespace OHOS
