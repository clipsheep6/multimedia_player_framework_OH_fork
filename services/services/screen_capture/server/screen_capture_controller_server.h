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
#ifndef SCREEN_CAPTURE_CONTROLLER_SERVER_H
#define SCREEN_CAPTURE_CONTROLLER_SERVER_H

#include <mutex>
#include <cstdlib>
#include <string>
#include <memory>
#include <atomic>
#include <queue>
#include <vector>
#include "i_screen_capture_controller.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class ScreenCaptureControllerServer : public IScreenCaptureController, public NoCopyable {
public:
    static std::shared_ptr<IScreenCaptureController> Create();
    ScreenCaptureControllerServer();
    ~ScreenCaptureControllerServer();

    int32_t ReportAVScreenCaptureUserChoice(int32_t sessionId, std::string choice) override;
};

} // namespace Media
} // namespace OHOS
#endif // SCREEN_CAPTURE_CONTROLLER_SERVER_H