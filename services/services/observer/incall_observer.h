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

#ifndef IN_CALL_OBSERVER_H
#define IN_CALL_OBSERVER_H

#include <mutex>
#include <vector>
#include <unordered_map>
#include <chrono>
#include "media_telephony_listener.h"
#include "screen_capture.h"

namespace OHOS {
namespace Media {

class InCallObserverCallBack {
public:
    virtual ~InCallObserverCallBack() = default;
    virtual bool StopAndRelease();
};

class InCallObserver {
public:

    static InCallObserver& GetInstance();
    bool RegisterObserver();
    void UnRegisterObserver();
    bool OnCallStateUpdated(bool inCall);
    bool IsInCall();
    bool RegisterInCallObserverCallBack(std::weak_ptr<InCallObserverCallBack> inCallObserverCallBack);
    void UnRegisterInCallObserverCallBack();
    bool OnCallCountUpdated(int32_t slotId, int32_t callState, const std::u16string &phoneNumber);
    bool HasOtherCall(int32_t slotId, int32_t callState, const std::u16string &phoneNumber);

private:

    std::vector<MediaTelephonyListener *> mediaTelephonyListeners_;
    std::weak_ptr<InCallObserverCallBack> inCallObserverCallBack_;
    InCallObserver();
    ~InCallObserver();
    bool inCall_{false};
    std::mutex mutex_;
    bool Init();
    bool isTelephonyStateListenerDied_ = true;
    std::atomic<int32_t> allInCallNum_{0};
};
}
}
#endif // IN_CALL_OBSERVER_H