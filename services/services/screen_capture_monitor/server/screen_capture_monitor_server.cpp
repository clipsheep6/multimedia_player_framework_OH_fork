/*
* Copyright (C) 2024 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "screen_capture_monitor_server.h"
#include "map"
#include "media_log.h"
#include "media_errors.h"
#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "media_dfx.h"
#include "hitrace/tracechain.h"
#include "media_utils.h"
#include "screen_capture_server.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SCREENCAPTURE, "ScreenCaptureMonitorServer"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<ScreenCaptureMonitorServer> screenCaptureMonitorServer = nullptr;
std::shared_ptr<ScreenCaptureMonitorServer> ScreenCaptureMonitorServer::GetInstance()
{
    if (!screenCaptureMonitorServer) {
        screenCaptureMonitorServer = std::make_shared<ScreenCaptureMonitorServer>();
        int32_t ret = screenCaptureMonitorServer->Init();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to init ScreenCaptureMonitorServer");
    }
    return screenCaptureMonitorServer;
}

ScreenCaptureMonitorServer::ScreenCaptureMonitorServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

ScreenCaptureMonitorServer::~ScreenCaptureMonitorServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    Release();
}

int32_t ScreenCaptureMonitorServer::Init()
{
    MediaTrace trace("ScreenCaptureMonitorServer::Init");
    return MSERR_OK;
}

int32_t ScreenCaptureMonitorServer::Release()
{
    MEDIA_LOGI("ScreenCaptureMonitorServer:0x%{public}06" PRIXPTR " Release S", FAKE_POINTER(this));
    std::lock_guard<std::mutex> lockCb(mutexCb_);
    screenCaptureMonitorCbSet_.clear();
    return MSERR_OK;
}

int32_t ScreenCaptureMonitorServer::IsScreenCaptureWorking()
{
    MEDIA_LOGI("ScreenCaptureMonitorServer:0x%{public}06" PRIXPTR " IsScreenCaptureWorking S", FAKE_POINTER(this));
    int32_t pid = -1;
    OHOS::Media::ScreenCaptureServer::GetRunningScreenCaptureInstancePid(pid);
    MEDIA_LOGI("IsScreenCaptureWorking pid %{public}d E", pid);
    return pid;
}

void ScreenCaptureMonitorServer::SetScreenCaptureMonitorCallback(
    sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> callback)
{
    MediaTrace trace("ScreenCaptureMonitorServer::SetScreenCaptureCallback");
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(callback != nullptr, "SetScreenCaptureCallback failed, callback is nullptr");
    screenCaptureMonitorCbSet_.insert(callback);
    MEDIA_LOGI("ScreenCaptureMonitorServer: 0x%{public}06" PRIXPTR "SetScreenCaptureCallback OK.", FAKE_POINTER(this));
}

void ScreenCaptureMonitorServer::RemoveScreenCaptureMonitorCallback(
    sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> callback)
{
    MediaTrace trace("ScreenCaptureMonitorServer::RemoveScreenCaptureMonitorCallback");
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(callback != nullptr, "RemoveScreenCaptureMonitorCallback failed, callback is nullptr");
    screenCaptureMonitorCbSet_.erase(callback);
    MEDIA_LOGI("ScreenCaptureMonitorServer: 0x%{public}06" PRIXPTR "RemoveScreenCaptureMonitorCallback OK.",
        FAKE_POINTER(this));
}

void ScreenCaptureMonitorServer::RegisterScreenCaptureMonitorListener(
    sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> callback)
{
    MEDIA_LOGI("ScreenCaptureMonitorServer:0x%{public}06" PRIXPTR " RegisterScreenCaptureMonitorListener",
        FAKE_POINTER(this));
}

void ScreenCaptureMonitorServer::UnregisterScreenCaptureMonitorListener(
    sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> listener)
{
    MEDIA_LOGI("ScreenCaptureMonitorServer:0x%{public}06" PRIXPTR " UnregisterScreenCaptureMonitorListener",
        FAKE_POINTER(this));
}

int32_t ScreenCaptureMonitorServer::CallOnScreenCaptureStarted(int32_t pid)
{
    std::lock_guard<std::mutex> lockCb(mutexCb_);
    MEDIA_LOGI("ScreenCaptureMonitorServer::CallOnScreenCaptureStarted S");
    for (const auto& value : screenCaptureMonitorCbSet_) {
        if (value != nullptr) {
            value->OnScreenCaptureStarted(pid);
        }
    }
    return MSERR_OK;
}

int32_t ScreenCaptureMonitorServer::CallOnScreenCaptureFinished(int32_t pid)
{
    std::lock_guard<std::mutex> lockCb(mutexCb_);
    MEDIA_LOGI("ScreenCaptureMonitorServer::CallOnScreenCaptureFinished S");
    for (const auto& value : screenCaptureMonitorCbSet_) {
        if (value != nullptr) {
            value->OnScreenCaptureFinished(pid);
        }
    }
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS