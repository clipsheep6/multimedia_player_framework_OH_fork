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

#ifndef SCREEN_CAPTURE_MONITOR_SERVICE_STUB_H
#define SCREEN_CAPTURE_MONITOR_SERVICE_STUB_H

#include <map>
#include "i_standard_screen_capture_monitor_service.h"
#include "i_standard_screen_capture_monitor_listener.h"
#include "media_death_recipient.h"
#include "screen_capture_monitor_server.h"
#include "nocopyable.h"
#include "monitor_server_object.h"

namespace OHOS {
namespace Media {
class ScreenCaptureMonitorServiceStub : public IRemoteStub<IStandardScreenCaptureMonitorService>, public NoCopyable {
public:
    static sptr<ScreenCaptureMonitorServiceStub> Create();
    virtual ~ScreenCaptureMonitorServiceStub();

    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    int32_t SetListenerObject(const sptr<IRemoteObject> &object) override;
    int32_t CloseListenerObject() override;
    int32_t IsScreenCaptureWorking() override;
    int32_t DestroyStub() override;

private:
    ScreenCaptureMonitorServiceStub();
    int32_t Init();
    int32_t SetListenerObject(MessageParcel &data, MessageParcel &reply);
    int32_t CloseListenerObject(MessageParcel &data, MessageParcel &reply);
    int32_t IsScreenCaptureWorking(MessageParcel &data, MessageParcel &reply);
    int32_t DestroyStub(MessageParcel &data, MessageParcel &reply);

    std::shared_ptr<ScreenCaptureMonitorServer> screenCaptureMonitorServer_ = nullptr;
    using screenCaptureMonitorStubFunc =
        int32_t(ScreenCaptureMonitorServiceStub::*)(MessageParcel &data, MessageParcel &reply);
    std::map<uint32_t, screenCaptureMonitorStubFunc> screenCaptureMonitorStubFuncs_;
    std::mutex mutex_;
    sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> screenCaptureMonitorCallback_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif // SCREEN_CAPTURE_MONITOR_SERVICE_STUB_H