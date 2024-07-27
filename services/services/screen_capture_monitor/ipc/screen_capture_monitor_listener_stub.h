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

#ifndef SCREEN_CAPTURE_MONITOR_LISTENER_STUB_H
#define SCREEN_CAPTURE_MONITOR_LISTENER_STUB_H

#include "i_standard_screen_capture_monitor_listener.h"
#include "screen_capture_monitor.h"
#include <set>

namespace OHOS {
namespace Media {
class ScreenCaptureMonitorListenerStub : public IRemoteStub<IStandardScreenCaptureMonitorListener> {
public:
    ScreenCaptureMonitorListenerStub();
    virtual ~ScreenCaptureMonitorListenerStub();
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    void OnScreenCaptureStarted(int32_t pid) override;
    void OnScreenCaptureFinished(int32_t pid) override;
    void RegisterScreenCaptureMonitorListener(sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> listener);
    void UnregisterScreenCaptureMonitorListener(sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> listener);

private:
    std::set<sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener>> screenCaptureMonitorCallbacks_;
};
} // namespace Media
} // namespace OHOS
#endif // SCREEN_CAPTURE_MONITOR_LISTENER_STUB_H