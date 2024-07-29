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

#include "transcoder_listener_stub.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_PLAYER, "TransCoderListenerStub"};
}

namespace OHOS {
namespace Media {
TransCoderListenerStub::TransCoderListenerStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

TransCoderListenerStub::~TransCoderListenerStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int TransCoderListenerStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    auto remoteDescriptor = data.ReadInterfaceToken();
    CHECK_AND_RETURN_RET_LOG(TransCoderListenerStub::GetDescriptor() == remoteDescriptor, MSERR_INVALID_OPERATION,
        "Invalid descriptor");

    switch (code) {
        case TransCoderListenerMsg::ON_ERROR: {
            int32_t errorCode = data.ReadInt32();
            std::string errorMsg = data.ReadString();
            OnError(errorCode, errorMsg);
            return MSERR_OK;
        }
        case TransCoderListenerMsg::ON_INFO: {
            int type = data.ReadInt32();
            int extra = data.ReadInt32();
            OnInfo(type, extra);
            return MSERR_OK;
        }
        default: {
            MEDIA_LOGE("default case, need check TransCoderListenerStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

void TransCoderListenerStub::OnError(int32_t errorCode, const std::string &errorMsg)
{
    if (callback_ != nullptr) {
        callback_->OnError(errorCode, errorMsg);
    }

    std::shared_ptr<MonitorClientObject> monitor = monitor_.lock();
    CHECK_AND_RETURN(monitor != nullptr);
    (void)monitor->DisableMonitor();
}

void TransCoderListenerStub::OnInfo(int32_t type, int32_t extra)
{
    CHECK_AND_RETURN(callback_ != nullptr);
    callback_->OnInfo(type, extra);
}

void TransCoderListenerStub::SetTransCoderCallback(const std::shared_ptr<TransCoderCallback> &callback)
{
    callback_ = callback;
}

void TransCoderListenerStub::SetMonitor(const std::weak_ptr<MonitorClientObject> &monitor)
{
    MEDIA_LOGI("SetMonitor");
    monitor_ = monitor;
}
} // namespace Media
} // namespace OHOS
