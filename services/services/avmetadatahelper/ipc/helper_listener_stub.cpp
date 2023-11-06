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

#include "helper_listener_stub.h"
#include "av_common.h"
#include "avmetadatahelper.h"
#include "media_log.h"
#include "media_errors.h"
#include "media_parcel.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HelperListenerStub"};
}

namespace OHOS {
namespace Media {
HelperListenerStub::HelperListenerStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

HelperListenerStub::~HelperListenerStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int HelperListenerStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    auto remoteDescriptor = data.ReadInterfaceToken();
    CHECK_AND_RETURN_RET_LOG(HelperListenerStub::GetDescriptor() == remoteDescriptor,
        MSERR_INVALID_OPERATION, "Invalid descriptor");

    switch (code) {
        case HelperListenerMsg::ON_INFO: {
            int32_t type = data.ReadInt32();
            int32_t extra = data.ReadInt32();
            Format format;
            (void)MediaParcel::Unmarshalling(data, format);
            std::string info = format.Stringify();
            MEDIA_LOGI("0x%{public}06" PRIXPTR " listen on info type: %{public}d extra %{public}d, format %{public}s",
                       FAKE_POINTER(this), type, extra, info.c_str());
            OnInfo(static_cast<HelperOnInfoType>(type), extra, format);
            return MSERR_OK;
        }
        case HelperListenerMsg::ON_ERROR_MSG: {
            int32_t errorCode = data.ReadInt32();
            std::string errorMsg = data.ReadString();
            OnError(errorCode, errorMsg);
            return MSERR_OK;
        }
        default: {
            MEDIA_LOGE("default case, need check HelperListenerStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

void HelperListenerStub::OnError(HelperErrorType errorType, int32_t errorCode)
{
    std::shared_ptr<HelperCallback> cb = callback_.lock();
    CHECK_AND_RETURN(cb != nullptr);
    (void)errorType;
    auto errorMsg = MSErrorToExtErrorString(static_cast<MediaServiceErrCode>(errorCode));
    cb->OnError(errorCode, errorMsg);
}

void HelperListenerStub::OnInfo(HelperOnInfoType type, int32_t extra, const Format &infoBody)
{
    std::shared_ptr<HelperCallback> cb = callback_.lock();
    CHECK_AND_RETURN(cb != nullptr);
    cb->OnInfo(type, extra, infoBody);
}

void HelperListenerStub::OnError(int32_t errorCode, const std::string &errorMsg)
{
    std::shared_ptr<HelperCallback> cb = callback_.lock();
    CHECK_AND_RETURN(cb != nullptr);
    cb->OnError(errorCode, errorMsg);
}

void HelperListenerStub::SetHelperCallback(const std::weak_ptr<HelperCallback> &callback)
{
    callback_ = callback;
}
} // namespace Media
} // namespace OHOS
