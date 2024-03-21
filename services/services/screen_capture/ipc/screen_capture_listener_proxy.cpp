/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "screen_capture_listener_proxy.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ScreenCaptureListenerProxy"};
}

namespace OHOS {
namespace Media {
ScreenCaptureListenerProxy::ScreenCaptureListenerProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardScreenCaptureListener>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

ScreenCaptureListenerProxy::~ScreenCaptureListenerProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void ScreenCaptureListenerProxy::OnError(ScreenCaptureErrorType errorType, int32_t errorCode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);

    bool token = data.WriteInterfaceToken(ScreenCaptureListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "Failed to write descriptor!");

    data.WriteInt32(errorType);
    data.WriteInt32(errorCode);
    int error = Remote()->SendRequest(ScreenCaptureListenerMsg::ON_ERROR, data, reply, option);
    CHECK_AND_RETURN_LOG(error == MSERR_OK, "on error failed, error: %{public}d", error);
}

void ScreenCaptureListenerProxy::OnAudioBufferAvailable(bool isReady, AudioCaptureSourceType type)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);

    bool token = data.WriteInterfaceToken(ScreenCaptureListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "Failed to write descriptor!");

    data.WriteBool(isReady);
    data.WriteInt32(type);
    int error = Remote()->SendRequest(ScreenCaptureListenerMsg::ON_AUDIO_AVAILABLE, data, reply, option);
    CHECK_AND_RETURN_LOG(error == MSERR_OK, "OnAudioBufferAvailable failed, error: %{public}d", error);
}

void ScreenCaptureListenerProxy::OnVideoBufferAvailable(bool isReady)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);

    bool token = data.WriteInterfaceToken(ScreenCaptureListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "Failed to write descriptor!");

    data.WriteBool(isReady);
    int error = Remote()->SendRequest(ScreenCaptureListenerMsg::ON_VIDEO_AVAILABLE, data, reply, option);
    CHECK_AND_RETURN_LOG(error == MSERR_OK, "OnVideoBufferAvailable failed, error: %{public}d", error);
}

void ScreenCaptureListenerProxy::OnStateChange(AVScreenCaptureStateCode stateCode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);

    bool token = data.WriteInterfaceToken(ScreenCaptureListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "Failed to write descriptor!");

    data.WriteInt32(stateCode);
    int error = Remote()->SendRequest(ScreenCaptureListenerMsg::ON_STAGE_CHANGE, data, reply, option);
    CHECK_AND_RETURN_LOG(error == MSERR_OK, "OnStateChange failed, error: %{public}d, stateCode: %{public}d",
        error, stateCode);
}

ScreenCaptureListenerCallback::ScreenCaptureListenerCallback(const sptr<IStandardScreenCaptureListener> &listener)
    : listener_(listener)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

ScreenCaptureListenerCallback::~ScreenCaptureListenerCallback()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void ScreenCaptureListenerCallback::OnError(ScreenCaptureErrorType errorType, int32_t errorCode)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances isStopped:%{public}d, errorType:%{public}d, errorCode:%{public}d",
        FAKE_POINTER(this), isStopped_.load(), errorType, errorCode);
    CHECK_AND_RETURN(isStopped_ == false);
    if (listener_ != nullptr) {
        listener_->OnError(errorType, errorCode);
    }
}

void ScreenCaptureListenerCallback::OnAudioBufferAvailable(bool isReady, AudioCaptureSourceType type)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances isStopped:%{public}d, isReady:%{public}d, type:%{public}d",
        FAKE_POINTER(this), isStopped_.load(), isReady, type);
    CHECK_AND_RETURN(isStopped_ == false);
    if (listener_ != nullptr) {
        listener_->OnAudioBufferAvailable(isReady, type);
    }
}

void ScreenCaptureListenerCallback::OnVideoBufferAvailable(bool isReady)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances isStopped:%{public}d, isReady:%{public}d",
        FAKE_POINTER(this), isStopped_.load(), isReady);
    CHECK_AND_RETURN(isStopped_ == false);
    if (listener_ != nullptr) {
        listener_->OnVideoBufferAvailable(isReady);
    }
}

void ScreenCaptureListenerCallback::OnStateChange(AVScreenCaptureStateCode stateCode)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances isStopped:%{public}d, stateCode:%{public}d",
        FAKE_POINTER(this), isStopped_.load(), stateCode);
    if (listener_ != nullptr) {
        listener_->OnStateChange(stateCode);
    }
}
} // namespace Media
} // namespace OHOS