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

#ifndef SCREEN_CAPTURE_CONTROLLER_H
#define SCREEN_CAPTURE_CONTROLLER_H

#include <cstdint>
#include <memory>
#include <string>

namespace OHOS {
namespace Media {

class ScreenCaptureController {
public:
    virtual void ReportAVScreenCaptureUserChoice(int32_t sessionId, std::string choice) = 0;
    virtual ~ScreenCaptureController() = default;
};

class __attribute__((visibility("default"))) ScreenCaptureControllerFactory {
public:
#ifdef UNSUPPORT_SCREEN_CAPTURE
    static std::shared_ptr<ScreenCaptureController> CreateScreenCaptureController()
    {
        return nullptr;
    }
#else
    static std::shared_ptr<ScreenCaptureController> CreateScreenCaptureController();
#endif

private:
    ScreenCaptureControllerFactory() = default;
    ~ScreenCaptureControllerFactory() = default;
};

} // namespace Media
} // namespace OHOS
#endif // SCREEN_CAPTURE_CONTROLLER_H