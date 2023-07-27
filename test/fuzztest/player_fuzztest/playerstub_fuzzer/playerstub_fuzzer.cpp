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

namespace OHOS {
namespace Media {
bool FuzzPlayerStub(uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(int64_t)) {
        return true;
    }
    constexpr int32_t codeNum = 26;
    PlayerServiceProxyFuzzer::PlayerServiceMsg codeIdList[codeNum] {
        PlayerServiceProxyFuzzer::SET_SOURCE,
        PlayerServiceProxyFuzzer::SET_MEDIA_DATA_SRC_OBJ,
        PlayerServiceProxyFuzzer::SET_FD_SOURCE,
        PlayerServiceProxyFuzzer::PLAY,
        PlayerServiceProxyFuzzer::PREPARE,
        PlayerServiceProxyFuzzer::PREPAREASYNC,
        PlayerServiceProxyFuzzer::PAUSE,
        PlayerServiceProxyFuzzer::STOP,
        PlayerServiceProxyFuzzer::RESET,
        PlayerServiceProxyFuzzer::RELEASE,
        PlayerServiceProxyFuzzer::SET_VOLUME,
        PlayerServiceProxyFuzzer::SEEK,
        PlayerServiceProxyFuzzer::GET_CURRENT_TIME,
        PlayerServiceProxyFuzzer::GET_VIDEO_TRACK_INFO,
        PlayerServiceProxyFuzzer::GET_AUDIO_TRACK_INFO,
        PlayerServiceProxyFuzzer::GET_VIDEO_WIDTH,
        PlayerServiceProxyFuzzer::GET_DURATION,
        PlayerServiceProxyFuzzer::SET_PLAYERBACK_SPEED,
        PlayerServiceProxyFuzzer::GET_PLAYERBACK_SPEED,
        PlayerServiceProxyFuzzer::SET_VIDEO_SURFACE,
        PlayerServiceProxyFuzzer::IS_PLAYING,
        PlayerServiceProxyFuzzer::IS_LOOPING,
        PlayerServiceProxyFuzzer::SET_LOOPING,
        PlayerServiceProxyFuzzer::SET_RENDERER_DESC,
        PlayerServiceProxyFuzzer::SET_CALLBACK,
        PlayerServiceProxyFuzzer::SELECT_BIT_RATE,
    };
    uint32_t codeId = *reinterpret_cast<uint32_t *>(data) % (codeNum);
    sptr<PlayerServiceProxyFuzzer> playerProxy = PlayerServiceProxyFuzzer::Create();
    if (playerProxy == nullptr) {
        return false;
    }
    if (codeIdList[codeId] <= PlayerServiceProxyFuzzer::SET_FD_SOURCE) {
        playerProxy->SendRequest(codeIdList[codeId], data, size, true);
    } else {
        playerProxy->SendRequest(PlayerServiceProxyFuzzer::SET_FD_SOURCE, data, size, false);
        playerProxy->SendRequest(PlayerServiceProxyFuzzer::PREPARE, data, size, false);
        sleep(1);
        playerProxy->SendRequest(codeIdList[codeId], data, size, true);
    }
    playerProxy->SendRequest(PlayerServiceProxyFuzzer::DESTROY, data, size, false);
    return true;
}
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::FuzzPlayerStub(data, size);
    return 0;
}