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

#ifndef I_SCREEN_CAPTURE_CONTROLLER_H
#define I_SCREEN_CAPTURE_CONTROLLER_H

#include <string>
#include "refbase.h"

namespace OHOS {
namespace Media {
class IScreenCaptureController {
public:
    virtual int32_t ReportAVScreenCaptureUserChoice(int32_t sessionId, std::string choice) = 0;
};
} // namespace Media
} // namespace OHOS
#endif // I_SCREEN_CAPTURE_CONTROLLER_H