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

#include "screen_capture_monitor_listener_proxy.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SCREENCAPTURE, "ScreenCaptureMonitorListenerProxy"};
}

namespace OHOS {
namespace Media {
ScreenCaptureMonitorListenerProxy::ScreenCaptureMonitorListenerProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardScreenCaptureMonitorListener>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

ScreenCaptureMonitorListenerProxy::~ScreenCaptureMonitorListenerProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void ScreenCaptureMonitorListenerProxy::OnScreenCaptureStarted(int32_t pid)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);

    bool token = data.WriteInterfaceToken(ScreenCaptureMonitorListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "Failed to write descriptor!");

    data.WriteInt32(pid);
    int error = Remote()->SendRequest(ScreenCaptureMonitorListenerMsg::ON_SCREEN_CAPTURE_STARTED, data, reply, option);
    CHECK_AND_RETURN_LOG(error == MSERR_OK, "on error failed, error: %{public}d", error);
}

void ScreenCaptureMonitorListenerProxy::OnScreenCaptureFinished(int32_t pid)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);

    bool token = data.WriteInterfaceToken(ScreenCaptureMonitorListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "Failed to write descriptor!");

    data.WriteInt32(pid);
    int error = Remote()->SendRequest(ScreenCaptureMonitorListenerMsg::ON_SCREEN_CAPTURE_FINISHED, data, reply, option);
    CHECK_AND_RETURN_LOG(error == MSERR_OK, "on error failed, error: %{public}d", error);
}

ScreenCaptureMonitorListenerCallback::ScreenCaptureMonitorListenerCallback(
    const sptr<IStandardScreenCaptureMonitorListener> &listener) : listener_(listener)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

ScreenCaptureMonitorListenerCallback::~ScreenCaptureMonitorListenerCallback()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void ScreenCaptureMonitorListenerCallback::OnScreenCaptureStarted(int32_t pid)
{
    MEDIA_LOGI("OnScreenCaptureStarted");
    if (listener_ != nullptr) {
        listener_->OnScreenCaptureStarted(pid);
    }
}

void ScreenCaptureMonitorListenerCallback::OnScreenCaptureFinished(int32_t pid)
{
    MEDIA_LOGI("OnScreenCaptureFinished");
    if (listener_ != nullptr) {
        listener_->OnScreenCaptureFinished(pid);
    }
}
} // namespace Media
} // namespace OHOS
