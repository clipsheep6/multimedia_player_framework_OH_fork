/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "stub_common.h"
#include <iostream>

namespace OHOS {
namespace Media {
sptr<IRemoteObject> MediaServiceProxyFuzzer::GetSubSystemAbility(IStandardMediaService::MediaSystemAbility subSystemId,
    const sptr<IRemoteObject> &listener)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(MediaServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "Failed to write descriptor!" << std::endl;
        return nullptr;
    }
    (void)data.WriteInt32(static_cast<int32_t>(subSystemId));
    (void)data.WriteRemoteObject(listener);
    if (Remote()->SendRequest(MediaServiceMsg::GET_SUBSYSTEM, data, reply, option) != 0) {
        std::cout << "Failed to SendRequest" << std::endl;
        return nullptr;
    }
    return reply.ReadRemoteObject();
}
}
}