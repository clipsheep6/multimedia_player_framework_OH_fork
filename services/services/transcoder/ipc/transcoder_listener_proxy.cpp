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

#include "transcoder_listener_proxy.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "TransCoderListenerProxy"};
}

namespace OHOS {
namespace Media {
TransCoderListenerProxy::TransCoderListenerProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardTransCoderListener>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

TransCoderListenerProxy::~TransCoderListenerProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void TransCoderListenerProxy::OnError(int32_t errorCode, const std::string &errorMsg)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);

    bool token = data.WriteInterfaceToken(TransCoderListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "Failed to write descriptor!");

    data.WriteInt32(errorCode);
    data.WriteString(errorMsg);
    int error = Remote()->SendRequest(TransCoderListenerMsg::ON_ERROR, data, reply, option);
    CHECK_AND_RETURN_LOG(error == MSERR_OK, "on error failed, error: %{public}d", error);
}

void TransCoderListenerProxy::OnInfo(int32_t type, int32_t extra)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);

    bool token = data.WriteInterfaceToken(TransCoderListenerProxy::GetDescriptor());
    CHECK_AND_RETURN_LOG(token, "Failed to write descriptor!");

    data.WriteInt32(static_cast<int>(type));
    data.WriteInt32(static_cast<int>(extra));
    int error = Remote()->SendRequest(TransCoderListenerMsg::ON_INFO, data, reply, option);
    CHECK_AND_RETURN_LOG(error == MSERR_OK, "on info failed, error: %{public}d", error);
}

TransCoderListenerCallback::TransCoderListenerCallback(const sptr<IStandardTransCoderListener> &listener)
    : listener_(listener)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

TransCoderListenerCallback::~TransCoderListenerCallback()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void TransCoderListenerCallback::OnError(int32_t errorCode, const std::string &errorMsg)
{
    if (listener_ != nullptr) {
        listener_->OnError(errorCode, errorMsg);
    }
}

void TransCoderListenerCallback::OnInfo(int32_t type, int32_t extra)
{
    if (listener_ != nullptr) {
        listener_->OnInfo(type, extra);
    }
}
} // namespace Media
} // namespace OHOS
