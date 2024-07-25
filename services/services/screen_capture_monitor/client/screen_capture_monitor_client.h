/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef SCREEN_CAPTURE_MONITOR_CLIENT_H
#define SCREEN_CAPTURE_MONITOR_CLIENT_H

#include "i_screen_capture_monitor_service.h"
#include "i_standard_screen_capture_monitor_service.h"
#include "screen_capture_monitor_listener_stub.h"

namespace OHOS {
namespace Media {
class ScreenCaptureMonitorClient : public IScreenCaptureMonitorService, public NoCopyable {
public:
    static std::shared_ptr<ScreenCaptureMonitorClient> Create(
        const sptr<IStandardScreenCaptureMonitorService> &ipcProxy);
    explicit ScreenCaptureMonitorClient(const sptr<IStandardScreenCaptureMonitorService> &ipcProxy);
    ~ScreenCaptureMonitorClient();

    int32_t IsScreenCaptureWorking() override;
    void RegisterScreenCaptureMonitorListener(
        sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> listener) override;
    void UnregisterScreenCaptureMonitorListener(
        sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> listener) override;
    void MediaServerDied();

private:
    int32_t CreateListenerObject();
    int32_t CloseListenerObject();
    sptr<IStandardScreenCaptureMonitorService> screenCaptureMonitorProxy_ = nullptr;
    sptr<ScreenCaptureMonitorListenerStub> listenerStub_ = nullptr;
    std::mutex mutex_;
    bool listenerStubIPCExist_ = false;
    std::set<sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener>> screenCaptureMonitorClientCallbacks_;
};
} // namespace Media
} // namespace OHOS
#endif // SCREEN_CAPTURE_MONITOR_CLIENT_H