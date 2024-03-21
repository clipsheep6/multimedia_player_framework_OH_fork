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

#include "screen_capture_controller_server.h"
#include "media_log.h"
#include "media_errors.h"
#include "uri_helper.h"
#include "media_dfx.h"
#include "screen_capture_server.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ScreenCaptureControllerServer"};
}

namespace OHOS {
namespace Media {

std::shared_ptr<IScreenCaptureController> ScreenCaptureControllerServer::Create()
{
    MEDIA_LOGI("ScreenCaptureControllerServer::Create() start");
    std::shared_ptr<ScreenCaptureControllerServer> controllerServerTemp =
        std::make_shared<ScreenCaptureControllerServer>();
    CHECK_AND_RETURN_RET_LOG(controllerServerTemp != nullptr, nullptr, "Failed to new ScreenCaptureControllerServer");

    std::shared_ptr<IScreenCaptureController> controllerServer =
        std::static_pointer_cast<OHOS::Media::IScreenCaptureController>(controllerServerTemp);
    return controllerServer;
}

ScreenCaptureControllerServer::ScreenCaptureControllerServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

ScreenCaptureControllerServer::~ScreenCaptureControllerServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t ScreenCaptureControllerServer::ReportAVScreenCaptureUserChoice(int32_t sessionId, std::string choice)
{
    MEDIA_LOGI("ScreenCaptureControllerServer::ReportAVScreenCaptureUserChoice start");
    int32_t ret = ScreenCaptureServer::ReportAVScreenCaptureUserChoice(sessionId, choice);
    return ret;
}

} // namespace Media
} // namespace OHOS