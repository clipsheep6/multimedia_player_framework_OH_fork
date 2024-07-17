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

#include "media_service_proxy.h"
#include "media_log.h"
#include "media_errors.h"
#include "player_xcollie.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaServiceProxy"};
}

namespace OHOS {
namespace Media {
MediaServiceProxy::MediaServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardMediaService>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaServiceProxy::~MediaServiceProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

sptr<IRemoteObject> MediaServiceProxy::GetSubSystemAbility(IStandardMediaService::MediaSystemAbility subSystemId,
    const sptr<IRemoteObject> &listener)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    CHECK_AND_RETURN_RET_LOG(listener != nullptr, nullptr, "listener is nullptr!");
    bool token = data.WriteInterfaceToken(MediaServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, nullptr, "Failed to write descriptor!");

    (void)data.WriteInt32(static_cast<int32_t>(subSystemId));
    (void)data.WriteRemoteObject(listener);
    int32_t error = -1;
    error = Remote()->SendRequest(MediaServiceMsg::GET_SUBSYSTEM, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, nullptr,
        "Create player proxy failed, error: %{public}d", error);
    return reply.ReadRemoteObject();
}
} // namespace Media
} // namespace OHOS
