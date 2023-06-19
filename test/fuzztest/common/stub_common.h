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

#ifndef STUB_COMMON_H
#define STUB_COMMON_H

#include "i_standard_media_listener.h"
#include "i_standard_media_service.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace Media {
class MediaListenerStubFuzzer : public IRemoteStub<IStandardMediaListener>, public NoCopyable {
public:
    MediaListenerStubFuzzer() = default;
    ~MediaListenerStubFuzzer() = default;
};

class MediaServiceProxyFuzzer : public IRemoteProxy<IStandardMediaService>, public NoCopyable {
public:
    explicit MediaServiceProxyFuzzer(const sptr<IRemoteObject> &impl) : IRemoteProxy<IStandardMediaService>(impl) {}
    virtual ~MediaServiceProxyFuzzer() {}
    sptr<IRemoteObject> GetSubSystemAbility(IStandardMediaService::MediaSystemAbility subSystemId,
        const sptr<IRemoteObject> &listener);
private:
    static inline BrokerDelegator<MediaServiceProxyFuzzer> delegator_;
};
}
}
#endif // STUB_COMMON_H
