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

#include "playerstub_fuzzer.h"
#include <unistd.h>
#include "stub_common.h"
#include "media_server.h"
#include "media_parcel.h"
#include "i_standard_player_service.h"

namespace OHOS {
namespace Media {
bool FuzzPlayerStub(uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(int64_t)) {
        return true;
    }
    sptr<PlayerServiceProxyFuzzer> playerProxy = PlayerServiceProxyFuzzer::Create();
    if (playerProxy == nullptr) {
        return false;
    }
    for (uint32_t codeId = 0; codeId < PlayerServiceProxyFuzzer::MAX_IPC_ID; codeId++) {
        if (codeId != PlayerServiceProxyFuzzer::DESTROY) {
            playerProxy->SendRequest(codeId, data, size, true);
        }
    }
    playerProxy->SendRequest(PlayerServiceProxyFuzzer::DESTROY, data, size, false);
    return true;
}

const int32_t SYSTEM_ABILITY_ID = 3002;
const bool RUN_ON_CREATE = false;
bool FuzzPlayerStubLocal(uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(int64_t)) {
        return true;
    }
    std::shared_ptr<MediaServer> mediaServer =
        std::make_shared<MediaServer>(SYSTEM_ABILITY_ID, RUN_ON_CREATE);
    if (mediaServer == nullptr) {
        return false;
    }

    sptr<IRemoteObject> listener = new(std::nothrow) MediaListenerStubFuzzer();
    sptr<IRemoteObject> player = mediaServer->GetSubSystemAbility(
        IStandardMediaService::MediaSystemAbility::MEDIA_PLAYER, listener);
    if (player == nullptr) {
        return false;
    }

    sptr<IRemoteStub<IStandardPlayerService>> playerStub = iface_cast<IRemoteStub<IStandardPlayerService>>(player);
    if (playerStub == nullptr) {
        return false;
    }

    for (uint32_t code = 0; code < PlayerServiceProxyFuzzer::MAX_IPC_ID; code++) {
        MessageParcel msg;
        msg.WriteInterfaceToken(playerStub->GetDescriptor());
        msg.WriteBuffer(data, size);
        msg.RewindRead(0);
        MessageParcel reply;
        MessageOption option;
        playerStub->OnRemoteRequest(code, msg, reply, option);
    }

    return true;
}
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::FuzzPlayerStub(data, size);
    OHOS::Media::FuzzPlayerStubLocal(data, size);
    return 0;
}