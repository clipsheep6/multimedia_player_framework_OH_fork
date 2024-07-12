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
#include <unistd.h>
#include <functional>
#include "media_log.h"
#include "media_errors.h"
#include "screen_capture.h"
#include "screen_capture_server.h"

#define PLAYER_FRAMEWORK_SCREEN_CAPTURE

using namespace OHOS;
namespace OHOS {
namespace Media {

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ScreenCaptureMonitor"};
}

ScreenCaptureMonitor& ScreenCaptureMonitor::GetInstance()
{
    static ScreenCaptureMonitor instance;
    instance.Init();
    return instance;
}

ScreenCaptureMonitor::ScreenCaptureMonitor()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

ScreenCaptureMonitor::~ScreenCaptureMonitor()
{
    listeners_.clear();
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool ScreenCaptureMonitor::Init()
{
    if (isScreenCaptureMonitorDied_) {
        MEDIA_LOGI("0x%{public}06" PRIXPTR " InCallObserver Init, Register Observer", FAKE_POINTER(this));
        isScreenCaptureMonitorDied_ = false;
    }
    return true;
}

int32_t ScreenCaptureMonitor::isScreenCaptureWorking()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR "isScreenCaptureWorking in", FAKE_POINTER(this));
    int32_t pid = -1;
    OHOS::Media::ScreenCaptureServer::GetRunningScreenCaptureInstancePid(pid);
    MEDIA_LOGI("isScreenCaptureWorking pid %{public}d", pid);
    return pid;
}

void ScreenCaptureMonitor::RegisterScreenCaptureMonitorListener(sptr<IScreenCaptureMonitorListener> listener)
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR "RegisterScreenCaptureMonitorListener in", FAKE_POINTER(this));
    std::unique_lock<std::mutex> lock(mutex_);
    listeners_.emplace_back(listener);
}

void ScreenCaptureMonitor::UnregisterScreenCaptureMonitorListener(sptr<IScreenCaptureMonitorListener> listener)
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR "UnregisterScreenCaptureMonitorListener in", FAKE_POINTER(this));
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto iter = listeners_.begin(); iter!=listeners_.end();) {
        if (*iter == listener) {
            iter = listeners_.erase(iter);
        } else {
            iter++;
        }
    }
}

void ScreenCaptureMonitor::NotifyAllListener(int32_t pid, bool onStart)
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR "NotifyAllListener in start or stop %{public}d",
        FAKE_POINTER(this), static_cast<int32_t>(onStart));
    for (auto iter = listeners_.begin(); iter!=listeners_.end(); iter++) {
        if (onStart) {
            (*iter)->OnScreenCaptureStarted(pid);
        } else {
            (*iter)->OnScreenCaptureFinished(pid);
        }
    }
}
}
}