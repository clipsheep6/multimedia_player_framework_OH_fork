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

#include "avmetadatastub_fuzzer.h"

namespace OHOS {
namespace Media {
bool FuzzAVMetadataStub(uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(int64_t)) {
        return true;
    }
    constexpr int32_t codeNum = 8;
    IStandardAVMetadataService::AVMetadataServiceMsg codeIdList[codeNum] {
        AVMetadataServiceProxyFuzzer::SET_URI_SOURCE,
        AVMetadataServiceProxyFuzzer::SET_FD_SOURCE,
        AVMetadataServiceProxyFuzzer::RESOLVE_METADATA,
        AVMetadataServiceProxyFuzzer::RESOLVE_METADATA_MAP,
        AVMetadataServiceProxyFuzzer::FETCH_ART_PICTURE,
        AVMetadataServiceProxyFuzzer::FETCH_FRAME_AT_TIME,
        AVMetadataServiceProxyFuzzer::RELEASE,
        AVMetadataServiceProxyFuzzer::DESTROY,
    };
    uint32_t codeId = *reinterpret_cast<uint32_t *>(data) % (codeNum);
    sptr<AVMetadataServiceProxyFuzzer> avmetaProxy = AVMetadataServiceProxyFuzzer::Create();
    if (avmetaProxy == nullptr) {
        return false;
    }
    if (codeIdList[codeId] <= AVMetadataServiceProxyFuzzer::SET_FD_SOURCE) {
        avmetaProxy->SendRequest(codeIdList[codeId], data, size, true);
    } else {
        avmetaProxy->SendRequest(AVMetadataServiceProxyFuzzer::SET_FD_SOURCE, data, size, false);
        avmetaProxy->SendRequest(codeIdList[codeId], data, size, true);
    }
    avmetaProxy->SendRequest(AVMetadataServiceProxyFuzzer::DESTROY, data, size, false);
    return true;
}
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::FuzzAVMetadataStub(data, size);
    return 0;
}