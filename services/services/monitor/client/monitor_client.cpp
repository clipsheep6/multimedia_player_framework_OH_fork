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
#include "scope_guard.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MonitorClient"};
}

namespace OHOS {
namespace Media {
std::mutex MonitorClient::instanceMutex_;
std::shared_ptr<MonitorClient> MonitorClient::monitorClient_ = std::make_shared<MonitorClient>();
MonitorClient::Destroy MonitorClient::destroy_;

constexpr uint8_t TIME_INTERVAL = 1; // Heartbeat once per second

MonitorClient::MonitorClient()
{
    MEDIA_LOGI("Instances create");
}

MonitorClient::~MonitorClient()
{
    MEDIA_LOGI("Instances destroy");
    clientDestroy_ = true;
    std::lock_guard<std::mutex> threadLock(thredMutex_);
    if (clickThread_ != nullptr) {
        MEDIA_LOGI("clear monitor client thread");
        if (clickThread_->joinable()) {
            clickCond_.notify_all();
            clickThread_->join();
        }
        clickThread_.reset();
        clickThread_ = nullptr;
    }
    MEDIA_LOGI("Instances Destroy end");
}

std::shared_ptr<MonitorClient> MonitorClient::GetInstance()
{
    std::lock_guard<std::mutex> lock(instanceMutex_);
    return monitorClient_;
}

bool MonitorClient::IsVaildProxy()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isVaildProxy_) {
            monitorProxy_ = nullptr;
        }
    }

    if (monitorProxy_ == nullptr) {
        monitorProxy_ = MediaServiceFactory::GetInstance().GetMonitorProxy();
        CHECK_AND_RETURN_RET_LOG(monitorProxy_ != nullptr, false, "monitorProxy_ is nullptr!");
        std::lock_guard<std::mutex> lock(mutex_);
        isVaildProxy_ = true;
    }

    return true;
}

int32_t MonitorClient::StartClick(MonitorClientObject *obj)
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " StartClick", FAKE_POINTER(obj));
    {
        std::lock_guard<std::mutex> lock(mutex_);
        CHECK_AND_RETURN_RET_LOG(objSet_.count(obj) == 0, MSERR_OK, "It has already been activated");
        objSet_.insert(obj);
        if (threadRunning_) {
            return MSERR_OK;
        }
        threadRunning_ = true;
    }

    std::lock_guard<std::mutex> threadLock(thredMutex_);
    // The original thread has already exited. Need to recycle resources
    if (clickThread_ != nullptr) {
        if (clickThread_->joinable()) {
            clickThread_->join();
        }
        clickThread_.reset();
        clickThread_ = nullptr;
    }

    // Start Thread
    CHECK_AND_RETURN_RET(!clientDestroy_, MSERR_OK);
    clickThread_ = std::make_unique<std::thread>(&MonitorClient::ClickThreadCtrl, this);

    return MSERR_OK;
}

int32_t MonitorClient::StopClick(MonitorClientObject *obj)
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " StopClick", FAKE_POINTER(obj));
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(objSet_.count(obj), MSERR_OK, "Not started");

    objSet_.erase(obj);
    if (objSet_.empty()) {
        clickCond_.notify_all();
    }

    return MSERR_OK;
}

void MonitorClient::MediaServerDied()
{
    MEDIA_LOGI("MediaServerDied");
    std::lock_guard<std::mutex> lock(mutex_);
    objSet_.clear();
    isVaildProxy_ = false;
    clickCond_.notify_all();
}

void MonitorClient::ClickThread()
{
    pthread_setname_np(pthread_self(), "OS_MonitorClick");
    MEDIA_LOGD("ClickThread start");

    CHECK_AND_RETURN_LOG(IsVaildProxy(), "Proxy is invaild!");
    CHECK_AND_RETURN_LOG(monitorProxy_->EnableMonitor() == MSERR_OK, "failed to EnableMonitor");

    while (true) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            clickCond_.wait_for(lock, std::chrono::seconds(TIME_INTERVAL), [this] {
                return objSet_.empty() || !isVaildProxy_ || clientDestroy_;
            });

            CHECK_AND_RETURN_LOG(!clientDestroy_, "clientDestroy, Normal exit")

            if (objSet_.empty()) {
                MEDIA_LOGI("0x%{public}06" PRIXPTR " objSet empty.", FAKE_POINTER(this));
                break;
            }
            if (!isVaildProxy_) {
                monitorProxy_ = nullptr;
                MEDIA_LOGI("Proxy is invaild.");
                return;
            }
        }
        CHECK_AND_RETURN_LOG(monitorProxy_ != nullptr, "monitorProxy_ is nullptr!");
        CHECK_AND_CONTINUE(monitorProxy_->Click() == MSERR_OK);
    }

    CHECK_AND_RETURN_LOG(IsVaildProxy(), "Proxy is invaild!");
    CHECK_AND_RETURN_LOG(monitorProxy_->DisableMonitor() == MSERR_OK, "failed to DisableMonitor");
    MEDIA_LOGI("0x%{public}06" PRIXPTR " ClickThread End", FAKE_POINTER(this));
}

void MonitorClient::ClickThreadCtrl()
{
    while (true) {
        ClickThread();
        std::this_thread::sleep_for(std::chrono::seconds(TIME_INTERVAL));
        std::unique_lock<std::mutex> lock(mutex_);
        if (objSet_.empty() || clientDestroy_) {
            threadRunning_ = false;
            MEDIA_LOGI("objSetsize %{public}zu, clientDestroy %{public}d.",
                objSet_.size(), clientDestroy_.load());
            return;
        }
    }
}

MonitorClient::Destroy::~Destroy()
{
    MEDIA_LOGI("MonitorClient Destroy start");
    std::shared_ptr<MonitorClient> temp;
    std::lock_guard<std::mutex> lock(instanceMutex_);
    CHECK_AND_RETURN_LOG(monitorClient_ != nullptr, "MonitorClient Destroy end");
    temp = monitorClient_;
    monitorClient_ = nullptr;
    MEDIA_LOGI("MonitorClient Destroy end");
}
} // namespace Media
} // namespace OHOS
