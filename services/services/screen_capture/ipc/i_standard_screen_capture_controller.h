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

#ifndef I_STANDARD_SCREEN_CAPTURE_CONTROLLER_H
#define I_STANDARD_SCREEN_CAPTURE_CONTROLLER_H

#include <memory>
#include <atomic>
#include "securec.h"
#include "ipc_types.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "i_screen_capture_controller.h"

namespace OHOS {
namespace Media {
class IStandardScreenCaptureController : public IRemoteBroker {
public:
    virtual ~IStandardScreenCaptureController() = default;
    virtual int32_t ReportAVScreenCaptureUserChoice(int32_t sessionId, std::string choice) = 0;
    virtual int32_t DestroyStub() = 0;

    /**
     * IPC code ID
     */
    enum ScreenCaptureControllerMsg {
        REPORT_USER_CHOICE,
        DESTROY,
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardScreenCaptureController");
};
} // namespace Media
} // namespace OHOS
#endif // I_STANDARD_SCREEN_CAPTURE_CONTROLLER_H