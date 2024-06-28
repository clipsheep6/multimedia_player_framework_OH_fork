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

#ifndef AVMETADATAHELPER_SERVICE_STUB_H
#define AVMETADATAHELPER_SERVICE_STUB_H

#include <map>
#include "i_standard_avmetadatahelper_service.h"
#include "media_death_recipient.h"
#include "avmetadatahelper_server.h"
#include "i_standard_helper_listener.h"
#include "helper_listener_proxy.h"

namespace OHOS {
namespace Media {
class AVMetadataHelperServiceStub : public IRemoteStub<IStandardAVMetadataHelperService>, public NoCopyable {
public:
    static sptr<AVMetadataHelperServiceStub> Create();
    virtual ~AVMetadataHelperServiceStub();

    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    int32_t SetSource(const std::string &uri, int32_t usage) override;
    int32_t SetSource(int32_t fd, int64_t offset, int64_t size, int32_t usage) override;
    int32_t SetSource(const sptr<IRemoteObject> &object) override;
    std::string ResolveMetadata(int32_t key) override;
    std::unordered_map<int32_t, std::string> ResolveMetadataMap() override;
    std::shared_ptr<Meta> GetAVMetadata() override;
    std::shared_ptr<AVSharedMemory> FetchArtPicture() override;
    std::shared_ptr<AVSharedMemory> FetchFrameAtTime(int64_t timeUs,
        int32_t option, const OutputConfiguration &param) override;
    void Release() override;
    int32_t DestroyStub() override;
    int32_t SetHelperCallback() override;
    int32_t SetListenerObject(const sptr<IRemoteObject> &object) override;

private:
    AVMetadataHelperServiceStub();
    int32_t Init();
    int32_t SetUriSource(MessageParcel &data, MessageParcel &reply);
    int32_t SetFdSource(MessageParcel &data, MessageParcel &reply);
    int32_t SetMediaDataSource(MessageParcel &data, MessageParcel &reply);
    int32_t ResolveMetadata(MessageParcel &data, MessageParcel &reply);
    int32_t ResolveMetadataMap(MessageParcel &data, MessageParcel &reply);
    int32_t GetAVMetadata(MessageParcel &data, MessageParcel &reply);
    int32_t FetchArtPicture(MessageParcel &data, MessageParcel &reply);
    int32_t FetchFrameAtTime(MessageParcel &data, MessageParcel &reply);
    int32_t Release(MessageParcel &data, MessageParcel &reply);
    int32_t DestroyStub(MessageParcel &data, MessageParcel &reply);
    int32_t SetHelperCallback(MessageParcel &data, MessageParcel &reply);
    int32_t SetListenerObject(MessageParcel &data, MessageParcel &reply);

    std::mutex mutex_;
    std::shared_ptr<IAVMetadataHelperService> avMetadateHelperServer_ = nullptr;
    using AVMetadataHelperStubFunc = int32_t(AVMetadataHelperServiceStub::*)(MessageParcel &data, MessageParcel &reply);
    std::map<uint32_t, AVMetadataHelperStubFunc> avMetadataHelperFuncs_;
    std::shared_ptr<HelperCallback> helperCallback_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif // AVMETADATAHELPER_SERVICE_STUB_H
