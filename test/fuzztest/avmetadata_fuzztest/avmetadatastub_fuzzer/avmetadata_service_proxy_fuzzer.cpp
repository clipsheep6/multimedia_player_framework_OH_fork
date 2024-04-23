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

#include "avmetadata_service_proxy_fuzzer.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

namespace OHOS {
namespace Media {
AVMetadataServiceProxyFuzzer::AVMetadataServiceProxyFuzzer(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardAVMetadataHelperService>(impl)
{
    avmetaFuncs_[SET_URI_SOURCE] = &AVMetadataServiceProxyFuzzer::SetUriSource;
    avmetaFuncs_[SET_FD_SOURCE] = &AVMetadataServiceProxyFuzzer::SetFdSource;
    avmetaFuncs_[RESOLVE_METADATA] = &AVMetadataServiceProxyFuzzer::ResolveMetadata;
    avmetaFuncs_[RESOLVE_METADATA_MAP] = &AVMetadataServiceProxyFuzzer::ResolveMetadataMap;
    avmetaFuncs_[GET_AVMETADATA] = &AVMetadataServiceProxyFuzzer::GetAVMetadata;
    avmetaFuncs_[FETCH_ART_PICTURE] = &AVMetadataServiceProxyFuzzer::FetchArtPicture;
    avmetaFuncs_[FETCH_FRAME_AT_TIME] = &AVMetadataServiceProxyFuzzer::FetchFrameAtTime;
    avmetaFuncs_[RELEASE] = &AVMetadataServiceProxyFuzzer::Release;
    avmetaFuncs_[DESTROY] = &AVMetadataServiceProxyFuzzer::DestroyStub;
}

sptr<AVMetadataServiceProxyFuzzer> AVMetadataServiceProxyFuzzer::Create()
{
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    sptr<IRemoteObject> obj = samgr->GetSystemAbility(OHOS::PLAYER_DISTRIBUTED_SERVICE_ID);
    if (obj == nullptr) {
        std::cout << "media object is nullptr." << std::endl;
        return nullptr;
    }
    sptr<IStandardMediaService> proxy = iface_cast<IStandardMediaService>(obj);
    if (proxy == nullptr) {
        std::cout << "media proxy is nullptr." << std::endl;
        return nullptr;
    }
    sptr<IRemoteObject> listenerStub = new(std::nothrow) MediaListenerStubFuzzer();
    sptr<IRemoteObject> avmetaObject = proxy->GetSubSystemAbility(
        IStandardMediaService::MediaSystemAbility::MEDIA_AVMETADATAHELPER, listenerStub);
    if (avmetaObject == nullptr) {
        std::cout << "avmetaObject is nullptr." << std::endl;
        return nullptr;
    }

    sptr<AVMetadataServiceProxyFuzzer> avmetaProxy = iface_cast<AVMetadataServiceProxyFuzzer>(avmetaObject);
    if (avmetaProxy == nullptr) {
        std::cout << "avmetaProxy is nullptr." << std::endl;
        return nullptr;
    }
    return avmetaProxy;
}

void AVMetadataServiceProxyFuzzer::SendRequest(int32_t code, uint8_t *inputData, size_t size, bool isFuzz)
{
    auto itFunc = avmetaFuncs_.find(code);
    if (itFunc != avmetaFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            (this->*memberFunc)(inputData, size, isFuzz);
        }
    }
}

int32_t AVMetadataServiceProxyFuzzer::SetUriSource(uint8_t *inputData, size_t size, bool isFuzz)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(AVMetadataServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "SetUriSource:Failed to write descriptor!" << std::endl;
        return false;
    }
    std::string url(reinterpret_cast<const char *>(inputData), size);
    (void)data.WriteString(url);
    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(SET_URI_SOURCE, data, reply, option);
}

int32_t AVMetadataServiceProxyFuzzer::SetFdSource(uint8_t *inputData, size_t size, bool isFuzz)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(AVMetadataServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "Failed to write descriptor!" << std::endl;
        return false;
    }

    int32_t fdValue;
    int64_t offset;
    int64_t lengthValue;
    int32_t usage;
    if (isFuzz) {
        fdValue = *reinterpret_cast<int32_t *>(inputData);
        lengthValue = *reinterpret_cast<int64_t *>(inputData);
        offset = *reinterpret_cast<uint32_t *>(inputData) % *reinterpret_cast<int64_t *>(inputData);
        usage = *reinterpret_cast<int32_t *>(inputData);
        (void)data.WriteFileDescriptor(fdValue);
        (void)data.WriteInt64(offset);
        (void)data.WriteInt64(lengthValue);
        (void)data.WriteInt32(usage);
        return SendRequest(SET_FD_SOURCE, data, reply, option);
    } else {
        const std::string filePath = "/data/test/media/H264_AAC.mp4";
        fdValue = open(filePath.c_str(), O_RDONLY);
        offset = 0;
        if (fdValue < 0) {
            std::cout << "Open file failed." << std::endl;
            (void)close(fdValue);
            return -1;
        }

        struct stat64 buf;
        if (fstat64(fdValue, &buf) != 0) {
            std::cout << "Get file state failed." << std::endl;
            (void)close(fdValue);
            return -1;
        }
        lengthValue = static_cast<int64_t>(buf.st_size);
        usage = AVMetadataUsage::AV_META_USAGE_PIXEL_MAP;
        (void)data.WriteFileDescriptor(fdValue);
        (void)data.WriteInt64(offset);
        (void)data.WriteInt64(lengthValue);
        (void)data.WriteInt32(usage);
        int32_t ret = SendRequest(SET_FD_SOURCE, data, reply, option);
        (void)close(fdValue);
        return ret;
    }
}

int32_t AVMetadataServiceProxyFuzzer::ResolveMetadata(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)isFuzz;
    (void)size;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(AVMetadataServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "ResolveMetadata:Failed to write descriptor!" << std::endl;
        return false;
    }
    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(RESOLVE_METADATA, data, reply, option);
}

int32_t AVMetadataServiceProxyFuzzer::ResolveMetadataMap(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageOption option;
    MessageParcel reply;

    bool token = data.WriteInterfaceToken(AVMetadataServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "ResolveMetadataMap:Failed to write descriptor!" << std::endl;
        return false;
    }
    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(RESOLVE_METADATA_MAP, data, reply, option);
}

int32_t AVMetadataServiceProxyFuzzer::GetAVMetadata(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageOption option;
    MessageParcel reply;

    bool token = data.WriteInterfaceToken(AVMetadataServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "GetAVMetadata:Failed to write descriptor!" << std::endl;
        return false;
    }
    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(RESOLVE_METADATA_MAP, data, reply, option);
}

int32_t AVMetadataServiceProxyFuzzer::FetchArtPicture(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageOption option;
    MessageParcel data;
    MessageParcel reply;

    bool token = data.WriteInterfaceToken(AVMetadataServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "FetchArtPicture:Failed to write descriptor!" << std::endl;
        return false;
    }
    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(FETCH_ART_PICTURE, data, reply, option);
}

int32_t AVMetadataServiceProxyFuzzer::FetchFrameAtTime(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)isFuzz;
    (void)size;
    MessageOption option;
    MessageParcel data;
    MessageParcel reply;

    bool token = data.WriteInterfaceToken(AVMetadataServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "FetchFrameAtTime:Failed to write descriptor!" << std::endl;
        return false;
    }
    (void)data.WriteInt64(*reinterpret_cast<int64_t *>(inputData));
    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(FETCH_FRAME_AT_TIME, data, reply, option);
}

int32_t AVMetadataServiceProxyFuzzer::Release(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageOption option;
    MessageParcel reply;

    bool token = data.WriteInterfaceToken(AVMetadataServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "Release:Failed to write descriptor!" << std::endl;
        return false;
    }
    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(RELEASE, data, reply, option);
}

int32_t AVMetadataServiceProxyFuzzer::DestroyStub(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    MessageParcel data;
    MessageOption option;
    MessageParcel reply;

    bool token = data.WriteInterfaceToken(AVMetadataServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "DestroyStub:Failed to write descriptor!" << std::endl;
        return false;
    }
    if (isFuzz) {
        (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    }
    return SendRequest(DESTROY, data, reply, option);
}

int32_t AVMetadataServiceProxyFuzzer::SendRequest(uint32_t code,
    MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    return Remote()->SendRequest(code, data, reply, option);
}
}
}