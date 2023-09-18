/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "playerservicestub_fuzzer.h"
#include <iostream>
#include "i_standard_media_listener.h"
#include "i_standard_media_service.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;

namespace OHOS {
namespace Media {
class PlayerServiceListenerStubFuzzer : public IRemoteStub<IStandardMediaListener>, public NoCopyable {
public:
    PlayerServiceListenerStubFuzzer() = default;
    ~PlayerServiceListenerStubFuzzer() = default;
};

class MediaServiceProxyFuzzer : public IRemoteProxy<IStandardMediaService>, public NoCopyable {
public:
    explicit MediaServiceProxyFuzzer(const sptr<IRemoteObject> &impl) : IRemoteProxy<IStandardMediaService>(impl) {}
    virtual ~MediaServiceProxyFuzzer() {}

    sptr<IRemoteObject> GetSubSystemAbility(IStandardMediaService::MediaSystemAbility subSystemId,
        const sptr<IRemoteObject> &listener)
    {
        MessageParcel data;
        MessageParcel reply;
        MessageOption option;

        bool token = data.WriteInterfaceToken(MediaServiceProxyFuzzer::GetDescriptor());
        if (!token) {
            cout << "Failed to write descriptor!" << endl;
            return nullptr;
        }
        (void)data.WriteInt32(static_cast<int32_t>(subSystemId));
        (void)data.WriteRemoteObject(listener);
        (void)Remote()->SendRequest(MediaServiceMsg::GET_SUBSYSTEM, data, reply, option);
        return reply.ReadRemoteObject();
    }

private:
    static inline BrokerDelegator<MediaServiceProxyFuzzer> delegator_;
};

PlayerServiceStubFuzzer::PlayerServiceStubFuzzer()
{
}

PlayerServiceStubFuzzer::~PlayerServiceStubFuzzer()
{
}

bool PlayerServiceStubFuzzer::FuzzServiceStub(uint8_t *data, size_t size)
{
    constexpr int32_t subSystemIdList = 8;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    sptr<IRemoteObject> object = samgr->GetSystemAbility(OHOS::PLAYER_DISTRIBUTED_SERVICE_ID);
    if (object == nullptr) {
        cout << "media object is nullptr." << endl;
        return false;
    }
    sptr<IStandardMediaService> mediaProxy_ = nullptr;
    mediaProxy_ = iface_cast<IStandardMediaService>(object);
    if (mediaProxy_ == nullptr) {
        cout << "media proxy is nullptr." << endl;
        return false;
    }
    sptr<IRemoteObject> listenerStub_ = nullptr;
    listenerStub_ = new(std::nothrow) PlayerServiceListenerStubFuzzer();

    IStandardMediaService::MediaSystemAbility subSystemId[subSystemIdList] {
            IStandardMediaService::MediaSystemAbility::MEDIA_PLAYER,
            IStandardMediaService::MediaSystemAbility::MEDIA_RECORDER,
            IStandardMediaService::MediaSystemAbility::MEDIA_CODEC,
            IStandardMediaService::MediaSystemAbility::MEDIA_AVMETADATAHELPER,
            IStandardMediaService::MediaSystemAbility::MEDIA_CODECLIST,
            IStandardMediaService::MediaSystemAbility::MEDIA_AVCODEC,
            IStandardMediaService::MediaSystemAbility::RECORDER_PROFILES,
        };
    int32_t systemId = *reinterpret_cast<int32_t *>(data) % (subSystemIdList);
    mediaProxy_->GetSubSystemAbility(subSystemId[systemId], listenerStub_);
    return true;
}
}

bool FuzzPlayerServiceStub(uint8_t *data, size_t size)
{
    if ((data == nullptr) || (size < sizeof(int32_t))) {
        return true;
    }
    PlayerServiceStubFuzzer player;
    return player.FuzzServiceStub(data, size);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzPlayerServiceStub(data, size);
    return 0;
}
