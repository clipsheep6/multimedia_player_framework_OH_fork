/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef FREEZER_IMPL_H
#define FREEZER_IMPL_H

#include "singleton.h"
#include "event_handler.h"
#include "event_runner.h"
#include "nocopyable.h"
#include "iremote_object.h"

#include "freezer.h"
#include "i_freezer_service.h"

namespace OHOS {
namespace Media {
class FreezerImpl : public Freezer, public NoCopyable {
    DECLARE_DELAYED_SINGLETON(FreezerImpl);
public:
    int32_t ProxyApp(const std::unordered_set<int32_t>& pidSet, const bool isFreeze) override;
    int32_t ResetAll() override;
    bool GetFreezerService();
private:
    class FreezerImplDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        /*
        * function: FreezerImplDeathRecipient, default constructor.
        */
        FreezerImplDeathRecipient() = default;
        /*
        * function: ~FreezerImplDeathRecipient, default destructor.
        */
        ~FreezerImplDeathRecipient() = default;
        /*
        * function: OnRemoteDied, PostTask when service(bundleActiveProxy_) is died.
        */
        void OnRemoteDied(const wptr<IRemoteObject> &object) override;
        /*
        * function: OnServiceDiedInner, get bundleActiveProxy_ and registerGroupCallBack again.
        */
        void OnServiceDiedInner(const wptr<IRemoteObject> &object);
    };
private:
    std::shared_ptr<IFreezerService> freezerService_ {nullptr};
    sptr<FreezerImplDeathRecipient> recipient_ {nullptr};
    std::shared_ptr<AppExecFwk::EventRunner> freezerServiceRunner_ {nullptr};
    std::shared_ptr<AppExecFwk::EventHandler> freezerServiceHandler_ {nullptr};
};
} // namespace Media
} // namespace OHOS
#endif // FREEZER_IMPL_H