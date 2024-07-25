/*
* Copyright (C) 2024 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "screen_capture_monitor_listener_stub.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SCREENCAPTURE, "ScreenCaptureMonitorListenerStub"};
}

namespace OHOS {
namespace Media {
ScreenCaptureMonitorListenerStub::ScreenCaptureMonitorListenerStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

ScreenCaptureMonitorListenerStub::~ScreenCaptureMonitorListenerStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int ScreenCaptureMonitorListenerStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (ScreenCaptureMonitorListenerStub::GetDescriptor() != remoteDescriptor) {
        MEDIA_LOGE("Invalid descriptor");
        return MSERR_INVALID_OPERATION;
    }
    switch (code) {
        case ScreenCaptureMonitorListenerMsg::ON_SCREEN_CAPTURE_STARTED: {
            int pid = data.ReadInt32();
            OnScreenCaptureStarted(pid);
            return MSERR_OK;
        }
        case ScreenCaptureMonitorListenerMsg::ON_SCREEN_CAPTURE_FINISHED: {
            int pid = data.ReadInt32();
            OnScreenCaptureFinished(pid);
            return MSERR_OK;
        }
        default: {
            MEDIA_LOGE("default case, need check ScreenCaptureMonitorListenerStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

void ScreenCaptureMonitorListenerStub::OnScreenCaptureStarted(int32_t pid)
{
    MEDIA_LOGI("ScreenCaptureMonitorListenerStub:0x%{public}06" PRIXPTR " OnScreenCaptureStarted",
        FAKE_POINTER(this));
    for (const auto& value : screenCaptureMonitorCallbacks_) {
        if (value != nullptr) {
            value->OnScreenCaptureStarted(pid);
        }
    }
}

void ScreenCaptureMonitorListenerStub::OnScreenCaptureFinished(int32_t pid)
{
    MEDIA_LOGI("ScreenCaptureMonitorListenerStub:0x%{public}06" PRIXPTR " OnScreenCaptureFinished",
        FAKE_POINTER(this));
    for (const auto& value : screenCaptureMonitorCallbacks_) {
        if (value != nullptr) {
            value->OnScreenCaptureFinished(pid);
        }
    }
}

void ScreenCaptureMonitorListenerStub::RegisterScreenCaptureMonitorListener(
    sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> listener)
{
    MEDIA_LOGI("ScreenCaptureMonitorListenerStub:0x%{public}06" PRIXPTR " RegisterScreenCaptureMonitorListener",
        FAKE_POINTER(this));
    screenCaptureMonitorCallbacks_.insert(listener);
}
void ScreenCaptureMonitorListenerStub::UnregisterScreenCaptureMonitorListener(
    sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> listener)
{
    MEDIA_LOGI("ScreenCaptureMonitorListenerStub:0x%{public}06" PRIXPTR " UnregisterScreenCaptureMonitorListener",
        FAKE_POINTER(this));
    auto it = screenCaptureMonitorCallbacks_.find(listener);
    if (it != screenCaptureMonitorCallbacks_.end()) {
        screenCaptureMonitorCallbacks_.erase(it);
    }
}
} // namespace Media
} // namespace OHOS