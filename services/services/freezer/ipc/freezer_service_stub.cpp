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

#include "freezer_service_stub.h"

#include <unistd.h>

#include "freezer_server.h"
#include "media_log.h"
#include "media_errors.h"
#include "media_parcel.h"
#include "parameter.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "FreezerServiceStub"};
}

namespace OHOS {
namespace Media {
sptr<FreezerServiceStub> FreezerServiceStub::Create()
{
    sptr<FreezerServiceStub> freezerStub = new(std::nothrow) FreezerServiceStub();
    CHECK_AND_RETURN_RET_LOG(freezerStub != nullptr, nullptr, "failed to new FreezerServiceStub");

    int32_t ret = freezerStub->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to freezer stub init");
    return freezerStub;
}

FreezerServiceStub::FreezerServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

FreezerServiceStub::~FreezerServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t FreezerServiceStub::Init()
{
    freezerFuncs_[static_cast<int32_t>(FreezerServiceMsg::PROXY_APP)] = &FreezerServiceStub::HandleProxyApp;
    freezerFuncs_[static_cast<int32_t>(FreezerServiceMsg::RESET_ALL)] = &FreezerServiceStub::HandleResetAll;
    return MSERR_OK;
}

int FreezerServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    MEDIA_LOGI("Stub: OnRemoteRequest of code: %{public}d is received", code);

    auto remoteDescriptor = data.ReadInterfaceToken();
    if (FreezerServiceStub::GetDescriptor() != remoteDescriptor) {
        MEDIA_LOGE("Invalid descriptor");
        return MSERR_INVALID_OPERATION;
    }

    auto itFunc = freezerFuncs_.find(code);
    if (itFunc != freezerFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            int32_t ret = (this->*memberFunc)(data, reply);
            if (ret != MSERR_OK) {
                MEDIA_LOGE("calling memberFunc is failed.");
            }
            return MSERR_OK;
        }
    }
    MEDIA_LOGW("FreezerServiceStub: no member func supporting, applying default process");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t FreezerServiceStub::HandleProxyApp(MessageParcel &data, MessageParcel &reply)
{
    std::unordered_set<int32_t> pidSet;
    int32_t size = data.ReadInt32();
    for (int32_t i = 0; i < size; i++) {
        pidSet.emplace(data.ReadInt32());
    }
    bool isFreeze = data.ReadBool();
    reply.WriteInt32(ProxyApp(pidSet, isFreeze));
    return MSERR_OK;
}
int32_t FreezerServiceStub::HandleResetAll(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(ResetAll());
    return MSERR_OK;
}

int32_t FreezerServiceStub::ProxyApp(const std::unordered_set<int32_t>& pidSet, const bool isFreeze)
{
    return DelayedSingleton<FreezerServer>::GetInstance()->ProxyApp(pidSet, isFreeze);
}

int32_t FreezerServiceStub::ResetAll()
{
    return DelayedSingleton<FreezerServer>::GetInstance()->ResetAll();
}
} // namespace Media
} // namespace OHOS
