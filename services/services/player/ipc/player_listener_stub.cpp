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

#include "player_listener_stub.h"
#include "av_common.h"
#include "player.h"
#include "media_log.h"
#include "media_errors.h"
#include "media_parcel.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerListenerStub"};
}

namespace OHOS {
namespace Media {
PlayerListenerStub::PlayerListenerStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerListenerStub::~PlayerListenerStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int PlayerListenerStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    auto remoteDescriptor = data.ReadInterfaceToken();
    CHECK_AND_RETURN_RET_LOG(PlayerListenerStub::GetDescriptor() == remoteDescriptor,
        MSERR_INVALID_OPERATION, "Invalid descriptor");

    switch (code) {
        case PlayerListenerMsg::ON_ERROR: {
            int32_t errorType = data.ReadInt32();
            int32_t errorCode = data.ReadInt32();
            OnError(static_cast<PlayerErrorType>(errorType), errorCode);
            return MSERR_OK;
        }
        case PlayerListenerMsg::ON_INFO: {
            int32_t type = data.ReadInt32();
            int32_t extra = data.ReadInt32();
            Format format;
            (void)MediaParcel::Unmarshalling(data, format);
            std::string info = format.Stringify();
            MEDIA_LOGD("0x%{public}06" PRIXPTR " listen on info type: %{public}d extra %{public}d, format %{public}s",
                       FAKE_POINTER(this), type, extra, info.c_str());
            OnInfo(static_cast<PlayerOnInfoType>(type), extra, format);
            return MSERR_OK;
        }
        case PlayerListenerMsg::ON_ERROR_MSG: {
            int32_t errorCode = data.ReadInt32();
            std::string errorMsg = data.ReadString();
            OnError(errorCode, errorMsg);
            return MSERR_OK;
        }
        default: {
            MEDIA_LOGE("default case, need check PlayerListenerStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

void PlayerListenerStub::OnError(PlayerErrorType errorType, int32_t errorCode)
{
    std::shared_ptr<PlayerCallback> cb = callback_.lock();
    CHECK_AND_RETURN(cb != nullptr);
    (void)errorType;
    auto errorMsg = MSErrorToExtErrorString(static_cast<MediaServiceErrCode>(errorCode));
    cb->OnError(errorCode, errorMsg);
}

void PlayerListenerStub::OnMonitor(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    std::shared_ptr<MonitorClientObject> monitor = monitor_.lock();
    CHECK_AND_RETURN(monitor != nullptr);
    int32_t reason = StateChangeReason::USER;
    if (infoBody.ContainKey(PlayerKeys::PLAYER_STATE_CHANGED_REASON)) {
        (void)infoBody.GetIntValue(PlayerKeys::PLAYER_STATE_CHANGED_REASON, reason);
    }
    if (((type == INFO_TYPE_STATE_CHANGE) && (extra == PLAYER_PLAYBACK_COMPLETE || extra == PLAYER_STATE_ERROR)) ||
        ((type == INFO_TYPE_STATE_CHANGE) && extra == PLAYER_PAUSED && reason == StateChangeReason::BACKGROUND) ||
        ((type == INFO_TYPE_STATE_CHANGE_BY_AUDIO) && (extra == PLAYER_PAUSED))) {
        MEDIA_LOGI("DisableMonitor, type = %{public}d, extra = %{public}d.", type, extra);
        (void)monitor->DisableMonitor();
    }
}

__attribute__((no_sanitize("cfi"))) void PlayerListenerStub::OnInfo(PlayerOnInfoType type,
    int32_t extra, const Format &infoBody)
{
    std::shared_ptr<PlayerCallback> cb = callback_.lock();
    CHECK_AND_RETURN(cb != nullptr);

    if (type == INFO_TYPE_STATE_CHANGE && extra != lastStateExtra_) {
        cb->OnInfo(type, extra, infoBody);
        lastStateExtra_ = extra;
    } else if (type == INFO_TYPE_STATE_CHANGE && extra == lastStateExtra_) {
        MEDIA_LOGW("Intercept repeated change state oninfo, extra %{public}d", extra);
    } else {
        cb->OnInfo(type, extra, infoBody);
    }
    OnMonitor(type, extra, infoBody);
}

void PlayerListenerStub::OnError(int32_t errorCode, const std::string &errorMsg)
{
    std::shared_ptr<PlayerCallback> cb = callback_.lock();
    CHECK_AND_RETURN(cb != nullptr);
    cb->OnError(errorCode, errorMsg);
}

void PlayerListenerStub::SetPlayerCallback(const std::weak_ptr<PlayerCallback> &callback)
{
    callback_ = callback;
}

void PlayerListenerStub::SetMonitor(const std::weak_ptr<MonitorClientObject> &monitor)
{
    MEDIA_LOGD("SetMonitor");
    monitor_ = monitor;
}
} // namespace Media
} // namespace OHOS
