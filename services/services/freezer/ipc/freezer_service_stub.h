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

#ifndef FREEZER_SERVICE_STUB_H
#define FREEZER_SERVICE_STUB_H

#include <map>
#include "i_standard_freezer_service.h"
#include "media_death_recipient.h"

namespace OHOS {
namespace Media {
class FreezerServiceStub : public IRemoteStub<IStandardFreezerService> {
public:
    DISALLOW_COPY_AND_MOVE(FreezerServiceStub);
    FreezerServiceStub();
    ~FreezerServiceStub() override = default;
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
private:
    int32_t HandleProxyApp(MessageParcel &data, MessageParcel &reply);
    int32_t HandleResetAll(MessageParcel &data, MessageParcel &reply);
    using FreezerStubFunc = int32_t(FreezerServiceStub::*)(MessageParcel &data, MessageParcel &reply);
    std::map<uint32_t, FreezerStubFunc> freezerFuncs_;
};
} // namespace Media
} // namespace OHOS
#endif // FREEZER_SERVICE_STUB_H
