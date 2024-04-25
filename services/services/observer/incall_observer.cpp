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

#include "incall_observer.h"
#include <unistd.h>
#include <functional>
#include "media_log.h"
#include "media_errors.h"
#include "hisysevent.h"
#include "telephony_observer_client.h"
#include "telephony_types.h"
#include "telephony_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "InCallObserver"};
}

using namespace OHOS;
using namespace OHOS::Telephony;
namespace OHOS {
namespace Media {

InCallObserver& InCallObserver::GetInstance()
{
    static InCallObserver instance;
    instance.Init();
    return instance;
}

InCallObserver::InCallObserver()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

InCallObserver::~InCallObserver()
{
    UnRegisterObserver();
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool InCallObserver::IsInCall()
{
    return inCall_;
}

bool InCallObserver::HasOtherCall(int32_t slotId, int32_t callState, const std::u16string &phoneNumber)
{
    (void)slotId;
    (void)callState;
    (void)phoneNumber;
    std::unique_lock<std::mutex> lock(mutex_);
    allInCallNum_.store(allInCallNum_.load() - 1);
    MEDIA_LOGI("0x%{public}06" PRIXPTR " HasOtherCall In allInCallNum = %{public}d",
                   FAKE_POINTER(this), allInCallNum_.load());
    if (allInCallNum_.load() > 0) {
        return true;
    } else {
        allInCallNum_.store(0);
        return false;
    }
}

bool InCallObserver::RegisterInCallObserverCallBack(std::weak_ptr<InCallObserverCallBack> callback)
{
    std::unique_lock<std::mutex> lock(mutex_);
    auto callbackPtr = callback.lock();
    if (callbackPtr) {
        inCallObserverCallBack_ = callback;
        return true;
    } else {
        MEDIA_LOGI("0x%{public}06" PRIXPTR "InCallObserver CallBack is null", FAKE_POINTER(this));
    }
    return false;
}

void InCallObserver::UnRegisterInCallObserverCallBack()
{
    if (!inCallObserverCallBack_.expired()) {
        inCallObserverCallBack_.reset();
    }
}

bool InCallObserver::OnCallStateUpdated(bool inCall)
{
    std::unique_lock<std::mutex> lock(mutex_);
    inCall_ = inCall;
    if (inCall) {
        auto callbackPtr = inCallObserverCallBack_.lock();
        if (callbackPtr) {
            MEDIA_LOGI("0x%{public}06" PRIXPTR " Stop and Release CallBack", FAKE_POINTER(this));
            return callbackPtr->StopAndRelease();
        } else {
            MEDIA_LOGI("0x%{public}06" PRIXPTR "InCallObserver CallBack is null", FAKE_POINTER(this));
        }
    }
    return true;
}

bool InCallObserver::OnCallCountUpdated(int32_t slotId, int32_t callState, const std::u16string &phoneNumber)
{
    (void)slotId;
    (void)callState;
    (void)phoneNumber;
    std::unique_lock<std::mutex> lock(mutex_);
    allInCallNum_.store(allInCallNum_.load() + 1);
    MEDIA_LOGI("0x%{public}06" PRIXPTR " OnCallCountUpdated In allInCallNum = %{public}d",
            FAKE_POINTER(this), allInCallNum_.load());
    return true;
}

bool InCallObserver::Init()
{
    if (isTelephonyStateListenerDied_) {
        MEDIA_LOGI("0x%{public}06" PRIXPTR " InCallObserver Init, Register Observer", FAKE_POINTER(this));
        UnRegisterObserver();
        RegisterObserver();
        isTelephonyStateListenerDied_ = false;
        return true;
    } else {
        MEDIA_LOGI("InCallObserver exist : %{public}d", isTelephonyStateListenerDied_);
    }
    return true;
}

bool InCallObserver::RegisterObserver()
{
    MEDIA_LOGI("InCallObserver Register InCall Listener");
    std::unique_lock<std::mutex> lock(mutex_);
    bool ret = false;
    for (int slotId = 0; slotId < SIM_SLOT_COUNT; slotId++) {
        MEDIA_LOGI("InCallObserver Register Listener slotId:%{public}d", slotId);
        auto telephonyObserver_ = std::make_unique<MediaTelephonyListener>().release();
        auto res = TelephonyObserverClient::GetInstance().AddStateObserver(telephonyObserver_, slotId,
            TelephonyObserverBroker::OBSERVER_MASK_CALL_STATE, true);
        MEDIA_LOGI("InCallObserver Register  Listener observer ret:%{public}d", res);
        if (res == OHOS::Telephony::TELEPHONY_SUCCESS) {
            ret = true;
            mediaTelephonyListeners_.push_back(telephonyObserver_);
        }
    }
    return ret;
}

void InCallObserver::UnRegisterObserver()
{
    MEDIA_LOGI("UnRegister InCall Listener");
    std::unique_lock<std::mutex> lock(mutex_);
    for (int slotId = 0; slotId < SIM_SLOT_COUNT; slotId++) {
        MEDIA_LOGI("UnRegister Listener slotId:%{public}d", slotId);
        TelephonyObserverClient::GetInstance().RemoveStateObserver(slotId,
            TelephonyObserverBroker::OBSERVER_MASK_CALL_STATE);
    }
    mediaTelephonyListeners_.clear();
}
}
}