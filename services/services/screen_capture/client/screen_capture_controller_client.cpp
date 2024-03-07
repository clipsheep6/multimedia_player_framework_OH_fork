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

#include "screen_capture_controller_client.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ScreenCaptureControllerClient"};
}

namespace OHOS {
namespace Media {

std::shared_ptr<ScreenCaptureControllerClient> ScreenCaptureControllerClient::Create(
    const sptr<IStandardScreenCaptureController> &ipcProxy)
{
    MEDIA_LOGI("ScreenCaptureControllerClient::Create() start");
    std::shared_ptr<ScreenCaptureControllerClient> client =
        std::make_shared<ScreenCaptureControllerClient>(ipcProxy);

    CHECK_AND_RETURN_RET_LOG(client != nullptr, nullptr, "failed to new Screen Capture Controller");

    return client;
}

ScreenCaptureControllerClient::ScreenCaptureControllerClient(const sptr<IStandardScreenCaptureController> &ipcProxy)
    : screenCaptureControllerProxy_(ipcProxy)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

ScreenCaptureControllerClient::~ScreenCaptureControllerClient()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (screenCaptureControllerProxy_ != nullptr) {
        (void)screenCaptureControllerProxy_->DestroyStub();
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void ScreenCaptureControllerClient::MediaServerDied()
{
    std::lock_guard<std::mutex> lock(mutex_);
    screenCaptureControllerProxy_ = nullptr;
}

int32_t ScreenCaptureControllerClient::ReportAVScreenCaptureUserChoice(int32_t sessionId, std::string choice)
{
    MEDIA_LOGI("ScreenCaptureControllerClient::ReportAVScreenCaptureUserChoice start");
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(screenCaptureControllerProxy_ != nullptr, MSERR_NO_MEMORY, "proxy does not exist.");
    return screenCaptureControllerProxy_->ReportAVScreenCaptureUserChoice(sessionId, choice);
}

} // namespace Media
} // namespace OHOS