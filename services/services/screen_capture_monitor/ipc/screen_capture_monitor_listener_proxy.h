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

#ifndef SCREEN_CAPTURE_MONITOR_LISTENER_PROXY_H
#define SCREEN_CAPTURE_MONITOR_LISTENER_PROXY_H

#include "i_standard_screen_capture_monitor_listener.h"
#include "media_death_recipient.h"
#include "screen_capture_monitor.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {

class ScreenCaptureMonitorListenerCallback : public ScreenCaptureMonitor::ScreenCaptureMonitorListener,
    public NoCopyable {
public:
    explicit ScreenCaptureMonitorListenerCallback(const sptr<IStandardScreenCaptureMonitorListener> &listener);
    virtual ~ScreenCaptureMonitorListenerCallback();

    void OnScreenCaptureStarted(int32_t pid) override;
    void OnScreenCaptureFinished(int32_t pid) override;

private:
    sptr<IStandardScreenCaptureMonitorListener> listener_ = nullptr;
};

class ScreenCaptureMonitorListenerProxy : public IRemoteProxy<IStandardScreenCaptureMonitorListener>,
    public NoCopyable {
public:
    explicit ScreenCaptureMonitorListenerProxy(const sptr<IRemoteObject> &impl);
    virtual ~ScreenCaptureMonitorListenerProxy();

    void OnScreenCaptureStarted(int32_t pid) override;
    void OnScreenCaptureFinished(int32_t pid) override;
private:
    static inline BrokerDelegator<ScreenCaptureMonitorListenerProxy> delegator_;
};
}
} // namespace OHOS
#endif // SCREEN_CAPTURE_MONITOR_LISTENER_PROXY_H