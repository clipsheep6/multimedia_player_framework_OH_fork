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

#include "screen_capture_monitor_impl.h"
#include "media_log.h"
#include "media_errors.h"
#include "i_media_service.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SCREENCAPTURE, "ScreenCaptureMonitorImpl"};
}
namespace OHOS {
namespace Media {

ScreenCaptureMonitor *ScreenCaptureMonitor::GetInstance()
{
    static ScreenCaptureMonitorImpl screenCaptureMonitorImpl;
    screenCaptureMonitorImpl.Init();
    return &screenCaptureMonitorImpl;
}

void ScreenCaptureMonitor::RegisterScreenCaptureMonitorListener(
    sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> listener)
{
    static_cast<ScreenCaptureMonitorImpl *>(ScreenCaptureMonitor::GetInstance())->
        RegisterScreenCaptureMonitorListener(listener);
}

void ScreenCaptureMonitor::UnregisterScreenCaptureMonitorListener(
    sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> listener)
{
    static_cast<ScreenCaptureMonitorImpl *>(ScreenCaptureMonitor::GetInstance())->
        UnregisterScreenCaptureMonitorListener(listener);
}

int32_t ScreenCaptureMonitor::IsScreenCaptureWorking()
{
    return static_cast<ScreenCaptureMonitorImpl *>(ScreenCaptureMonitor::GetInstance())->IsScreenCaptureWorking();
}

int32_t ScreenCaptureMonitorImpl::Init()
{
    MEDIA_LOGD("ScreenCaptureMonitorImpl:0x%{public}06" PRIXPTR " Init in", FAKE_POINTER(this));
    if (!screenCaptureMonitorService_) {
        screenCaptureMonitorService_ = MediaServiceFactory::GetInstance().CreateScreenCaptureMonitorService();
    }
    CHECK_AND_RETURN_RET_LOG(screenCaptureMonitorService_ != nullptr, MSERR_UNKNOWN,
        "failed to create ScreenCaptureMonitor service");
    return MSERR_OK;
}

void ScreenCaptureMonitorImpl::RegisterScreenCaptureMonitorListener(
    sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> listener)
{
    MEDIA_LOGD("ScreenCaptureMonitorImpl:0x%{public}06" PRIXPTR " RegisterMonitorListener in", FAKE_POINTER(this));
    CHECK_AND_RETURN_LOG(listener != nullptr, "input listener is nullptr.");
    CHECK_AND_RETURN_LOG(screenCaptureMonitorService_ != nullptr, "screen capture monitor service does not exist..");
    screenCaptureMonitorService_->RegisterScreenCaptureMonitorListener(listener);
}

void ScreenCaptureMonitorImpl::UnregisterScreenCaptureMonitorListener(
    sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> listener)
{
    MEDIA_LOGD("ScreenCaptureMonitorImpl:0x%{public}06" PRIXPTR " UnregisterMonitorListener in", FAKE_POINTER(this));
    CHECK_AND_RETURN_LOG(listener != nullptr, "input listener is nullptr.");
    CHECK_AND_RETURN_LOG(screenCaptureMonitorService_ != nullptr, "screen capture monitor service does not exist..");
    screenCaptureMonitorService_->UnregisterScreenCaptureMonitorListener(listener);
}

int32_t ScreenCaptureMonitorImpl::IsScreenCaptureWorking()
{
    MEDIA_LOGD("ScreenCaptureMonitorImpl:0x%{public}06" PRIXPTR " IsScreenCaptureWorking in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenCaptureMonitorService_ != nullptr,  MSERR_INVALID_OPERATION,
        "screen capture monitor service does not exist..");
    return screenCaptureMonitorService_->IsScreenCaptureWorking();
}

ScreenCaptureMonitorImpl::ScreenCaptureMonitorImpl()
{
    MEDIA_LOGD("ScreenCaptureMonitorImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

ScreenCaptureMonitorImpl::~ScreenCaptureMonitorImpl()
{
    if (screenCaptureMonitorService_ != nullptr) {
        (void)MediaServiceFactory::GetInstance().DestroyScreenCaptureMonitorService(screenCaptureMonitorService_);
        screenCaptureMonitorService_ = nullptr;
    }
    MEDIA_LOGD("ScreenCaptureMonitorImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}
} // namespace Media
} // namespace OHOS