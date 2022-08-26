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

#include "freezer_service_proxy.h"
#include "media_log.h"
#include "media_errors.h"
#include "media_parcel.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "FreezerServiceProxy"};
}

namespace OHOS {
namespace Media {
FreezerServiceProxy::FreezerServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardFreezerService>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

int32_t FreezerServiceProxy::ProxyApp(const std::unordered_set<int32_t>& pidSet, const bool isFreeze)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};

    if (!data.WriteInterfaceToken(FreezerServiceProxy::GetDescriptor())) {
        MEDIA_LOGE("Failed to write descriptor");
        return MSERR_UNKNOWN;
    }

    int32_t size = pidSet.size();
    data.WriteInt32(size);
    for (const auto pid : pidSet) {
        data.WriteInt32(pid);
    }
    data.WriteBool(isFreeze);
    MEDIA_LOGE("errorCode value is %{public}d", MSERR_SERVICE_DIED);
    int error = Remote()->SendRequest(PROXY_APP, data, reply, option);
    if (error != MSERR_OK) {
        MEDIA_LOGE("ProxyApp failed, error: %{public}d", error);
        return error;
    }
    return reply.ReadInt32();
}

int32_t FreezerServiceProxy::ResetAll()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};

    if (!data.WriteInterfaceToken(FreezerServiceProxy::GetDescriptor())) {
        MEDIA_LOGE("Failed to write descriptor");
        return MSERR_UNKNOWN;
    }

    int error = Remote()->SendRequest(RESET_ALL, data, reply, option);
    if (error != MSERR_OK) {
        MEDIA_LOGE("ResetAll obj failed, error: %{public}d", error);
        return error;
    }

    return reply.ReadInt32();
}
} // namespace Media
} // namespace OHOS
