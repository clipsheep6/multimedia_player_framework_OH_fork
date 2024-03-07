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

#ifndef SCREEN_CAPTURE_CONTROLLER_STUB_H
#define SCREEN_CAPTURE_CONTROLLER_STUB_H

#include <map>
#include "i_standard_screen_capture_controller.h"
#include "screen_capture_controller_server.h"
#include "media_death_recipient.h"

namespace OHOS {
namespace Media {
class ScreenCaptureControllerStub : public IRemoteStub<IStandardScreenCaptureController>, public NoCopyable {
public:
    static sptr<ScreenCaptureControllerStub> Create();
    virtual ~ScreenCaptureControllerStub();

    int32_t ReportAVScreenCaptureUserChoice(int32_t sessionId, std::string choice) override;
    int32_t DestroyStub() override;
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

private:
    ScreenCaptureControllerStub();
    int32_t Init();
    int32_t ReportAVScreenCaptureUserChoice(MessageParcel &data, MessageParcel &reply);
    int32_t DestroyStub(MessageParcel &data, MessageParcel &reply);

    std::mutex mutex_;
    std::shared_ptr<IScreenCaptureController> screenCaptureControllerServer_ = nullptr;
    using screenCaptureControllerStubFuncs =
        int32_t(ScreenCaptureControllerStub::*)(MessageParcel &data, MessageParcel &reply);
    std::map<uint32_t, screenCaptureControllerStubFuncs> screenCaptureControllerStubFuncs_;
};
} // namespace Media
} // namespace OHOS
#endif // SCREEN_CAPTURE_CONTROLLER_STUB_H