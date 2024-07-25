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

#ifndef SCREEN_CAPTURE_MONITOR_SERVICE_SERVER_H
#define SCREEN_CAPTURE_MONITOR_SERVICE_SERVER_H

#include <chrono>

#include "i_screen_capture_monitor_service.h"
#include "nocopyable.h"
#include "task_queue.h"
#include "watchdog.h"
#include "meta/meta.h"
#include <set>

namespace OHOS {
namespace Media {

class ScreenCaptureMonitorServer : public IScreenCaptureMonitorService, public NoCopyable {
public:
    static std::shared_ptr<ScreenCaptureMonitorServer> GetInstance();

    ScreenCaptureMonitorServer();
    ~ScreenCaptureMonitorServer();
    // IScreenCaptureMonitorService override
    int32_t IsScreenCaptureWorking() override;

    void SetScreenCaptureMonitorCallback(sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> listener);
    void RemoveScreenCaptureMonitorCallback(sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> listener);

    void RegisterScreenCaptureMonitorListener(
        sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> listener) override;
    void UnregisterScreenCaptureMonitorListener(
        sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> listener) override;
    int32_t CallOnScreenCaptureStarted(int32_t pid);
    int32_t CallOnScreenCaptureFinished(int32_t pid);
    int32_t Release();
private:
    int32_t Init();
    std::mutex mutex_;
    std::mutex mutexCb_;
    std::set<sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener>> screenCaptureMonitorCbSet_;
};
} // namespace Media
} // namespace OHOS
#endif // SCREEN_CAPTURE_MONITOR_SERVICE_SERVER_H