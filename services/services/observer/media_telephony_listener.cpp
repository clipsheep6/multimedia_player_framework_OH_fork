/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
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

#include "media_telephony_listener.h"
#include "media_log.h"
#include "incall_observer.h"
#include "call_manager_client.h"
#include "core_service_client.h"
#include "call_manager_base.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaTelephonyListener"};
}

using namespace OHOS::Telephony;
namespace OHOS {
namespace Media {
MediaTelephonyListener::MediaTelephonyListener()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaTelephonyListener::~MediaTelephonyListener()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void MediaTelephonyListener::OnCallStateUpdated(int32_t slotId, int32_t callState, const std::u16string &phoneNumber)
{
    MEDIA_LOGI("OnCallStateUpdated slotId = %{public}d, callState = %{public}d", slotId, callState);
    // skip no sim card CALL_STATUS_UNKNOWN
    if (callState == (int32_t)TelCallState::CALL_STATUS_ACTIVE ||
        callState == (int32_t)TelCallState::CALL_STATUS_DIALING ||
        callState == (int32_t)TelCallState::CALL_STATUS_INCOMING ||
        callState == (int32_t)TelCallState::CALL_STATUS_HOLDING ||
        callState == (int32_t)TelCallState::CALL_STATUS_WAITING) {
        InCallObserver::GetInstance().OnCallStateUpdated(true);
    } else if (callState == (int32_t)TelCallState::CALL_STATUS_DISCONNECTED) {
        InCallObserver::GetInstance().OnCallStateUpdated(false);
    }
}

void MediaTelephonyListener::OnSignalInfoUpdated(int32_t slotId,
    const std::vector<sptr<OHOS::Telephony::SignalInformation>> &vec)
{
    MEDIA_LOGI("OnSignalInfoUpdated slotId = %{public}d, signalInfoList.size = %{public}zu", slotId, vec.size());
}

void MediaTelephonyListener::OnNetworkStateUpdated(int32_t slotId,
    const sptr<OHOS::Telephony::NetworkState> &networkState)
{
    MEDIA_LOGI("OnNetworkStateUpdated slotId = %{public}d, networkState = %{public}d", slotId,
               networkState == nullptr);
}

void MediaTelephonyListener::OnCellInfoUpdated(int32_t slotId,
    const std::vector<sptr<OHOS::Telephony::CellInformation>> &vec)
{
    MEDIA_LOGI("OnCellInfoUpdated slotId = %{public}d, cell info size =  %{public}zu", slotId, vec.size());
}

void MediaTelephonyListener::OnSimStateUpdated(int32_t slotId, OHOS::Telephony::CardType type,
    OHOS::Telephony::SimState state, OHOS::Telephony::LockReason reason)
{
    MEDIA_LOGI("OnSimStateUpdated slotId = %{public}d, simState =  %{public}d", slotId, state);
}

void MediaTelephonyListener::OnCellularDataConnectStateUpdated(int32_t slotId, int32_t dataState, int32_t networkType)
{
    MEDIA_LOGI("OnCellularDataConnectStateUpdated slotId=%{public}d, dataState=%{public}d, networkType="
               "%{public}d", slotId, dataState, networkType);
}

void MediaTelephonyListener::OnCellularDataFlowUpdated(int32_t slotId, int32_t dataFlowType)
{
    MEDIA_LOGI("OnCellularDataFlowUpdated slotId = %{public}d, dataFlowType =  %{public}d", slotId, dataFlowType);
}

void MediaTelephonyListener::OnCfuIndicatorUpdated(int32_t slotId, bool cfuResult)
{
    MEDIA_LOGI("OnCfuIndicatorUpdated slotId = %{public}d, cfuResult = %{public}d", slotId, cfuResult);
}

void MediaTelephonyListener::OnVoiceMailMsgIndicatorUpdated(int32_t slotId, bool voiceMailMsgResult)
{
    MEDIA_LOGI("OnVoiceMailMsgIndicatorUpdated slotId = %{public}d, voiceMailMsgResult =  %{public}d", slotId,
               voiceMailMsgResult);
}

void MediaTelephonyListener::OnIccAccountUpdated()
{
    MEDIA_LOGI("OnIccAccountUpdated begin");
}
}
}