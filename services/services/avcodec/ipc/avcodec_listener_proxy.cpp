/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "avcodec_listener_proxy.h"
#include "media_errors.h"
#include "media_log.h"
#include "media_parcel.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecListenerProxy"};
}

namespace OHOS {
namespace Media {
AVCodecListenerProxy::AVCodecListenerProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardAVCodecListener>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecListenerProxy::~AVCodecListenerProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void AVCodecListenerProxy::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    data.WriteInt32(static_cast<int32_t>(errorType));
    data.WriteInt32(errorCode);
    int error = Remote()->SendRequest(AVCodecListenerMsg::ON_ERROR, data, reply, option);
    if (error != MSERR_OK) {
        MEDIA_LOGE("OnError failed, error: %{public}d", error);
    }
}

void AVCodecListenerProxy::OnStreamChanged(const Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    (void)MediaParcel::Marshalling(data, format);
    int error = Remote()->SendRequest(AVCodecListenerMsg::ON_OUTPUT_FORMAT_CHANGED, data, reply, option);
    if (error != MSERR_OK) {
        MEDIA_LOGE("OnStreamChanged failed, error: %{public}d", error);
    }
}

void AVCodecListenerProxy::OnNeedInputData(uint32_t index)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    data.WriteUint32(index);
    int error = Remote()->SendRequest(AVCodecListenerMsg::ON_INPUT_BUFFER_AVAILABLE, data, reply, option);
    if (error != MSERR_OK) {
        MEDIA_LOGE("OnNeedInputData failed, error: %{public}d", error);
    }
}

void AVCodecListenerProxy::OnNewOutputData(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    data.WriteUint32(index);
    data.WriteInt64(info.presentationTimeUs);
    data.WriteInt32(info.size);
    data.WriteInt32(info.offset);
    data.WriteInt32(static_cast<int32_t>(flag));
    int error = Remote()->SendRequest(AVCodecListenerMsg::ON_OUTPUT_BUFFER_AVAILABLE, data, reply, option);
    if (error != MSERR_OK) {
        MEDIA_LOGE("OnNewOutputData failed, error: %{public}d", error);
    }
}

AVCodecListenerCallback::AVCodecListenerCallback(const sptr<IStandardAVCodecListener> &listener)
    : listener_(listener)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecListenerCallback::~AVCodecListenerCallback()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void AVCodecListenerCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    if (listener_ != nullptr) {
        listener_->OnError(errorType, errorCode);
    }
}

void AVCodecListenerCallback::OnStreamChanged(const Format &format)
{
    if (listener_ != nullptr) {
        listener_->OnStreamChanged(format);
    }
}

void AVCodecListenerCallback::OnNeedInputData(uint32_t index)
{
    if (listener_ != nullptr) {
        listener_->OnNeedInputData(index);
    }
}

void AVCodecListenerCallback::OnNewOutputData(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    if (listener_ != nullptr) {
        listener_->OnNewOutputData(index, info, flag);
    }
}
} // namespace Media
} // namespace OHOS
