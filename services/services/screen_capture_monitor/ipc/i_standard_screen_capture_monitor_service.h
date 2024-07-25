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

#ifndef I_STANDARD_SCREEN_CAPTURE_MONITOR_SERVICE_H
#define I_STANDARD_SCREEN_CAPTURE_MONITOR_SERVICE_H

#include "ipc_types.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"

namespace OHOS {
namespace Media {
class IStandardScreenCaptureMonitorService : public IRemoteBroker {
public:
    virtual ~IStandardScreenCaptureMonitorService() = default;
    virtual int32_t SetListenerObject(const sptr<IRemoteObject> &object) = 0;
    virtual int32_t CloseListenerObject() = 0;
    virtual int32_t IsScreenCaptureWorking() = 0;
    virtual int32_t DestroyStub() = 0;

    /**
     * IPC code ID
     */
    enum ScreenCaptureMonitorServiceMsg {
        SET_LISTENER_OBJ = 0,
        IS_SCREEN_CAPTURE_WORKING = 1,
        DESTROY = 2,
        CLOSE_LISTENER_OBJ = 3,
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardScreenCaptureMonitorService");
};
} // namespace Media
} // namespace OHOS
#endif // I_STANDARD_SCREEN_CAPTURE_MONITOR_SERVICE_H