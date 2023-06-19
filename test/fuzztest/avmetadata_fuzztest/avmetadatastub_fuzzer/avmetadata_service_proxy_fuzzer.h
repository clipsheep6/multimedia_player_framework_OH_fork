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

#ifndef AVMETADATA_SERVICE_PROXY_FUZZER_H
#define AVMETADATA_SERVICE_PROXY_FUZZER_H

#include "stub_common.h"

namespace OHOS {
namespace Media {
class IStandardAVMetadataService : public IRemoteBroker {
public:
    virtual ~IStandardAVMetadataService() = default;

    enum AVMetadataServiceMsg {
        SET_URI_SOURCE = 0,
        SET_FD_SOURCE,
        RESOLVE_METADATA,
        RESOLVE_METADATA_MAP,
        FETCH_ART_PICTURE,
        FETCH_FRAME_AT_TIME,
        RELEASE,
        DESTROY,
    };

    enum AVMetadataUsage : int32_t {
        AV_META_USAGE_META_ONLY,
        AV_META_USAGE_PIXEL_MAP,
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardAVMetadataHelperService");
};

class AVMetadataServiceProxyFuzzer : public IRemoteProxy<IStandardAVMetadataService> {
public:
    static sptr<AVMetadataServiceProxyFuzzer> Create();
    explicit AVMetadataServiceProxyFuzzer(const sptr<IRemoteObject> &impl);
    virtual ~AVMetadataServiceProxyFuzzer() {}
    void SendRequest(int32_t code, uint8_t *inputData, size_t size, bool isFuzz);
private:
    int32_t SetUriSource(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SetFdSource(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t ResolveMetadata(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t ResolveMetadataMap(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t FetchArtPicture(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t FetchFrameAtTime(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t Release(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t DestroyStub(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SendRequest(uint32_t code, MessageParcel &inputData, MessageParcel &reply, MessageOption &option);
    static inline BrokerDelegator<AVMetadataServiceProxyFuzzer> delegator_;
    using AVMetaStubFunc = int32_t(AVMetadataServiceProxyFuzzer::*)(uint8_t *inputData, size_t size, bool isFuzz);
    std::map<uint32_t, AVMetaStubFunc> avmetaFuncs_;
};
}
}
#endif // AVMETADATA_SERVICE_PROXY_FUZZER_H
