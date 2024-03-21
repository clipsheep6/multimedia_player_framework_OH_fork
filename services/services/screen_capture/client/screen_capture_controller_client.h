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

#ifndef SCREEN_CAPTURE_CONTROLLER_CLIENT_H
#define SCREEN_CAPTURE_CONTROLLER_CLIENT_H

#include "i_screen_capture_controller.h"
#include "i_standard_screen_capture_controller.h"

namespace OHOS {
namespace Media {
class ScreenCaptureControllerClient : public IScreenCaptureController, public NoCopyable {
public:
    static std::shared_ptr<ScreenCaptureControllerClient>
        Create(const sptr<IStandardScreenCaptureController> &ipcProxy);
    explicit ScreenCaptureControllerClient(const sptr<IStandardScreenCaptureController> &ipcProxy);
    ~ScreenCaptureControllerClient();

    // ScreenCaptureControllerClient
    void MediaServerDied();
    int32_t ReportAVScreenCaptureUserChoice(int32_t sessionId, std::string choice) override;

private:
    sptr<IStandardScreenCaptureController> screenCaptureControllerProxy_ = nullptr;
    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS
#endif // SCREEN_CAPTURE_CONTROLLER_CLIENT_H