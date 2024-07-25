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

#ifndef SCREEN_CAPTURE_MONITOR_SERVICE_PROXY_H
#define SCREEN_CAPTURE_MONITOR_SERVICE_PROXY_H

#include "i_standard_screen_capture_monitor_service.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class ScreenCaptureMonitorServiceProxy : public IRemoteProxy<IStandardScreenCaptureMonitorService>, public NoCopyable {
public:
    explicit ScreenCaptureMonitorServiceProxy(const sptr<IRemoteObject> &impl);
    virtual ~ScreenCaptureMonitorServiceProxy();

    int32_t SetListenerObject(const sptr<IRemoteObject> &object) override;
    int32_t CloseListenerObject() override;
    int32_t IsScreenCaptureWorking() override;
    int32_t DestroyStub() override;

private:
    static inline BrokerDelegator<ScreenCaptureMonitorServiceProxy> delegator_;
};
} // namespace Media
} // namespace OHOS
#endif // SCREEN_CAPTURE_MONITOR_SERVICE_PROXY_H