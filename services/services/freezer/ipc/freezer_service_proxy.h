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

#ifndef FREEZER_SERVICE_PROXY_H
#define FREEZER_SERVICE_PROXY_H

#include "i_standard_freezer_service.h"
#include "media_parcel.h"

namespace OHOS {
namespace Media {
class FreezerServiceProxy : public IRemoteProxy<IStandardFreezerService> {
public:
    explicit FreezerServiceProxy(const sptr<IRemoteObject> &impl);
    ~FreezerServiceProxy() override = default;
    int32_t ProxyApp(const std::unordered_set<int32_t>& pidSet, const bool isFreeze) override;
    int32_t ResetAll() override;

private:
    static inline BrokerDelegator<FreezerServiceProxy> delegator_;
};
} // namespace Media
} // namespace OHOS
#endif // FREEZER_SERVICE_PROXY_H
