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

#include "screen_capture_controller_impl.h"
#include "media_log.h"
#include "media_errors.h"
#include "i_media_service.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ScreenCaptureController"};
}
namespace OHOS {
namespace Media {
std::shared_ptr<ScreenCaptureController> ScreenCaptureControllerFactory::CreateScreenCaptureController()
{
    std::shared_ptr<ScreenCaptureControllerImpl> impl = std::make_shared<ScreenCaptureControllerImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "failed to new ScreenCaptureControllerImpl");

    return impl;
}

void ScreenCaptureControllerImpl::ReportAVScreenCaptureUserChoice(int32_t sessionId, std::string choice)
{
    MEDIA_LOGI("ScreenCaptureController::ReportAVScreenCaptureUserChoice start");
    std::shared_ptr<IScreenCaptureController> controllerClient =
        MediaServiceFactory::GetInstance().CreateScreenCaptureControllerClient();

    int32_t ret = controllerClient->ReportAVScreenCaptureUserChoice(sessionId, choice);
    MEDIA_LOGI("ScreenCaptureController::ReportAVScreenCaptureUserChoice Report result: %{public}d", ret);

    (void)MediaServiceFactory::GetInstance().DestroyScreenCaptureControllerClient(controllerClient);
    controllerClient = nullptr;
}

ScreenCaptureControllerImpl::ScreenCaptureControllerImpl()
{
    MEDIA_LOGD("ScreenCaptureControllerImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

ScreenCaptureControllerImpl::~ScreenCaptureControllerImpl()
{
    MEDIA_LOGD("ScreenCaptureControllerImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

} // namespace Media
} // namespace OHOS