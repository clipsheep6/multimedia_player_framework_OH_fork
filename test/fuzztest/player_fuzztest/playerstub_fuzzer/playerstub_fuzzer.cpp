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
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::FuzzPlayerStub(data, size);
    return 0;
}