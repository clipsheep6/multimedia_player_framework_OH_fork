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

#include "avmetadatahelper_service_stub.h"
#include "media_server_manager.h"
#include "media_log.h"
#include "media_errors.h"
#include "avsharedmemory_ipc.h"
#include "media_data_source_proxy.h"
#include "media_dfx.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_METADATA, "AVMetadataHelperServiceStub"};
}

namespace OHOS {
namespace Media {
sptr<AVMetadataHelperServiceStub> AVMetadataHelperServiceStub::Create()
{
    sptr<AVMetadataHelperServiceStub> avMetadataHelperStub = new(std::nothrow) AVMetadataHelperServiceStub();
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperStub != nullptr, nullptr, "failed to new AVMetadataHelperServiceStub");

    int32_t ret = avMetadataHelperStub->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to avmetadatahlper stub init");
    StatisticEventWriteBundleName("create", "AVMetadataHelperServiceStub");
    return avMetadataHelperStub;
}

AVMetadataHelperServiceStub::AVMetadataHelperServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " create", FAKE_POINTER(this));
}

AVMetadataHelperServiceStub::~AVMetadataHelperServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVMetadataHelperServiceStub::Init()
{
    std::unique_lock<std::mutex> lock(mutex_);
    avMetadateHelperServer_ = AVMetadataHelperServer::Create();
    CHECK_AND_RETURN_RET_LOG(avMetadateHelperServer_ != nullptr, MSERR_NO_MEMORY,
        "failed to create AVMetadataHelper Service");

    avMetadataHelperFuncs_ = {
        { SET_URI_SOURCE,
            [this](MessageParcel &data, MessageParcel &reply) { return SetUriSource(data, reply); } },
        { SET_FD_SOURCE,
            [this](MessageParcel &data, MessageParcel &reply) { return SetFdSource(data, reply); } },
        { SET_MEDIA_DATA_SRC_OBJ,
            [this](MessageParcel &data, MessageParcel &reply) { return SetMediaDataSource(data, reply); } },
        { RESOLVE_METADATA,
            [this](MessageParcel &data, MessageParcel &reply) { return ResolveMetadata(data, reply); } },
        { RESOLVE_METADATA_MAP,
            [this](MessageParcel &data, MessageParcel &reply) { return ResolveMetadataMap(data, reply); } },
        { FETCH_ALBUM_COVER,
            [this](MessageParcel &data, MessageParcel &reply) { return FetchArtPicture(data, reply); } },
        { FETCH_FRAME_AT_TIME,
            [this](MessageParcel &data, MessageParcel &reply) { return FetchFrameAtTime(data, reply); } },
        { RELEASE,
            [this](MessageParcel &data, MessageParcel &reply) { return Release(data, reply); } },
        { DESTROY,
            [this](MessageParcel &data, MessageParcel &reply) { return DestroyStub(data, reply); } },
        { SET_CALLBACK,
            [this](MessageParcel &data, MessageParcel &reply) { return SetHelperCallback(data, reply); } },
        { SET_LISTENER_OBJ,
            [this](MessageParcel &data, MessageParcel &reply) { return SetListenerObject(data, reply); } },
        { GET_AVMETADATA,
            [this](MessageParcel &data, MessageParcel &reply) { return GetAVMetadata(data, reply); } },
        { GET_TIME_BY_FRAME_INDEX,
            [this](MessageParcel &data, MessageParcel &reply) { return GetTimeByFrameIndex(data, reply); } },
        { GET_FRAME_INDEX_BY_TIME,
            [this](MessageParcel &data, MessageParcel &reply) { return GetFrameIndexByTime(data, reply); } },
    };
    return MSERR_OK;
}

int32_t AVMetadataHelperServiceStub::DestroyStub()
{
    std::unique_lock<std::mutex> lock(mutex_);
    helperCallback_ = nullptr;
    avMetadateHelperServer_ = nullptr;
    MediaServerManager::GetInstance().DestroyStubObject(MediaServerManager::AVMETADATAHELPER, AsObject());
    return MSERR_OK;
}

int AVMetadataHelperServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Stub: OnRemoteRequest of code: %{public}u is received",
        FAKE_POINTER(this), code);

    auto remoteDescriptor = data.ReadInterfaceToken();
    if (AVMetadataHelperServiceStub::GetDescriptor() != remoteDescriptor) {
        MEDIA_LOGE("Invalid descriptor");
        return MSERR_INVALID_OPERATION;
    }

    auto itFunc = avMetadataHelperFuncs_.find(code);
    if (itFunc != avMetadataHelperFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            int32_t ret = memberFunc(data, reply);
            if (ret != MSERR_OK) {
                MEDIA_LOGE("Calling memberFunc is failed.");
            }
            return ret;
        }
    }
    MEDIA_LOGW("AVMetadataHelperServiceStub: no member func supporting, applying default process");

    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t AVMetadataHelperServiceStub::SetSource(const std::string &uri, int32_t usage)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadateHelperServer_ != nullptr, MSERR_NO_MEMORY, "avmetadatahelper server is nullptr");
    return avMetadateHelperServer_->SetSource(uri, usage);
}

int32_t AVMetadataHelperServiceStub::SetSource(int32_t fd, int64_t offset, int64_t size, int32_t usage)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadateHelperServer_ != nullptr, MSERR_NO_MEMORY, "avmetadatahelper server is nullptr");
    return avMetadateHelperServer_->SetSource(fd, offset, size, usage);
}

int32_t AVMetadataHelperServiceStub::SetSource(const sptr<IRemoteObject> &object)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_NO_MEMORY, "set mediadatasrc object is nullptr");
    CHECK_AND_RETURN_RET_LOG(avMetadateHelperServer_ != nullptr, MSERR_NO_MEMORY, "avmetadatahelper server is nullptr");

    sptr<IStandardMediaDataSource> proxy = iface_cast<IStandardMediaDataSource>(object);
    CHECK_AND_RETURN_RET_LOG(proxy != nullptr, MSERR_NO_MEMORY, "failed to convert MediaDataSourceProxy");

    std::shared_ptr<IMediaDataSource> mediaDataSrc = std::make_shared<MediaDataCallback>(proxy);
    CHECK_AND_RETURN_RET_LOG(mediaDataSrc != nullptr, MSERR_NO_MEMORY, "failed to new MetaListenerCallback");

    return avMetadateHelperServer_->SetSource(mediaDataSrc);
}

std::string AVMetadataHelperServiceStub::ResolveMetadata(int32_t key)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadateHelperServer_ != nullptr, "", "avmetadatahelper server is nullptr");
    return avMetadateHelperServer_->ResolveMetadata(key);
}

std::unordered_map<int32_t, std::string> AVMetadataHelperServiceStub::ResolveMetadataMap()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadateHelperServer_ != nullptr, {}, "avmetadatahelper server is nullptr");
    return avMetadateHelperServer_->ResolveMetadata();
}

std::shared_ptr<Meta> AVMetadataHelperServiceStub::GetAVMetadata()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadateHelperServer_ != nullptr, nullptr, "avmetadatahelper server is nullptr");
    return avMetadateHelperServer_->GetAVMetadata();
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperServiceStub::FetchArtPicture()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadateHelperServer_ != nullptr, nullptr, "avmetadatahelper server is nullptr");
    return avMetadateHelperServer_->FetchArtPicture();
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperServiceStub::FetchFrameAtTime(int64_t timeUs,
    int32_t option, const OutputConfiguration &param)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadateHelperServer_ != nullptr, nullptr, "avmetadatahelper server is nullptr");
    return avMetadateHelperServer_->FetchFrameAtTime(timeUs, option, param);
}

void AVMetadataHelperServiceStub::Release()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(avMetadateHelperServer_ != nullptr, "avmetadatahelper server is nullptr");
    return avMetadateHelperServer_->Release();
}

int32_t AVMetadataHelperServiceStub::SetHelperCallback()
{
    MEDIA_LOGD("SetHelperCallback");
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadateHelperServer_ != nullptr, MSERR_NO_MEMORY, "metadata server is nullptr");
    return avMetadateHelperServer_->SetHelperCallback(helperCallback_);
}

int32_t AVMetadataHelperServiceStub::SetListenerObject(const sptr<IRemoteObject> &object)
{
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_NO_MEMORY, "set listener object is nullptr");

    sptr<IStandardHelperListener> listener = iface_cast<IStandardHelperListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, MSERR_NO_MEMORY, "failed to convert IStandardHelperListener");

    std::shared_ptr<HelperCallback> callback = std::make_shared<HelperListenerCallback>(listener);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_NO_MEMORY, "failed to new HelperListenerCallback");

    helperCallback_ = callback;
    return MSERR_OK;
}

int32_t AVMetadataHelperServiceStub::GetTimeByFrameIndex(uint32_t index, int64_t &time)
{
    MEDIA_LOGI("GetTimeByFrameIndex");
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadateHelperServer_ != nullptr, 0, "avmetadatahelper server is nullptr");
    return avMetadateHelperServer_->GetTimeByFrameIndex(index, time);
}

int32_t AVMetadataHelperServiceStub::GetFrameIndexByTime(int64_t time, uint32_t &index)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadateHelperServer_ != nullptr, 0, "avmetadatahelper server is nullptr");
    return avMetadateHelperServer_->GetFrameIndexByTime(time, index);
}

int32_t AVMetadataHelperServiceStub::SetUriSource(MessageParcel &data, MessageParcel &reply)
{
    std::string uri = data.ReadString();
    int32_t usage = data.ReadInt32();
    reply.WriteInt32(SetSource(uri, usage));
    return MSERR_OK;
}

int32_t AVMetadataHelperServiceStub::SetFdSource(MessageParcel &data, MessageParcel &reply)
{
    int32_t fd = data.ReadFileDescriptor();
    int64_t offset = data.ReadInt64();
    int64_t size = data.ReadInt64();
    int32_t usage = data.ReadInt32();
    reply.WriteInt32(SetSource(fd, offset, size, usage));
    (void)::close(fd);
    return MSERR_OK;
}

int32_t AVMetadataHelperServiceStub::SetMediaDataSource(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    reply.WriteInt32(SetSource(object));
    return MSERR_OK;
}

int32_t AVMetadataHelperServiceStub::ResolveMetadata(MessageParcel &data, MessageParcel &reply)
{
    int32_t key = data.ReadInt32();
    reply.WriteString(ResolveMetadata(key));
    return MSERR_OK;
}

int32_t AVMetadataHelperServiceStub::ResolveMetadataMap(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    std::unordered_map<int32_t, std::string> metadata = ResolveMetadataMap();
    CHECK_AND_RETURN_RET_LOG(metadata.size() != 0, MSERR_INVALID_VAL, "No metadata");

    std::vector<int32_t> key;
    std::vector<std::string> dataStr;
    for (auto it = metadata.begin(); it != metadata.end(); it++) {
        key.push_back(it->first);
        dataStr.push_back(it->second);
    }

    reply.WriteInt32Vector(key);
    reply.WriteStringVector(dataStr);
    return MSERR_OK;
}

int32_t AVMetadataHelperServiceStub::GetAVMetadata(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    bool ret = true;
    std::shared_ptr<Meta> customInfo = std::make_shared<Meta>();
    auto metadata = GetAVMetadata();
    if (metadata == nullptr) {
        MEDIA_LOGE("metadata is null");
        metadata = std::make_shared<Meta>();
    }

    auto iter = metadata->Find("customInfo");
    if (iter != metadata->end()) {
        ret &= metadata->GetData("customInfo", customInfo);
        ret &= reply.WriteString("customInfo");
        ret &= customInfo->ToParcel(reply);
    }
    metadata->Remove("customInfo");
    ret &= reply.WriteString("AVMetadata");
    ret &= metadata->ToParcel(reply);
    if (!ret) {
        MEDIA_LOGE("GetAVMetadata ToParcel error");
    }
    return MSERR_OK;
}

int32_t AVMetadataHelperServiceStub::FetchArtPicture(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    auto result = FetchArtPicture();
    if (result == nullptr) {
        MEDIA_LOGE("result is null");
        return MSERR_INVALID_OPERATION;
    }

    return WriteAVSharedMemoryToParcel(result, reply);
}

int32_t AVMetadataHelperServiceStub::FetchFrameAtTime(MessageParcel &data, MessageParcel &reply)
{
    int64_t timeUs = data.ReadInt64();
    int32_t option = data.ReadInt32();
    OutputConfiguration param = {data.ReadInt32(), data.ReadInt32(), static_cast<PixelFormat>(data.ReadInt32())};
    std::shared_ptr<AVSharedMemory> ashMem = FetchFrameAtTime(timeUs, option, param);

    return WriteAVSharedMemoryToParcel(ashMem, reply);
}

int32_t AVMetadataHelperServiceStub::Release(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    (void)reply;
    Release();
    return MSERR_OK;
}

int32_t AVMetadataHelperServiceStub::DestroyStub(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(DestroyStub());
    return MSERR_OK;
}

int32_t AVMetadataHelperServiceStub::SetHelperCallback(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(SetHelperCallback());
    return MSERR_OK;
}

int32_t AVMetadataHelperServiceStub::SetListenerObject(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    reply.WriteInt32(SetListenerObject(object));
    return MSERR_OK;
}

int32_t AVMetadataHelperServiceStub::GetTimeByFrameIndex(MessageParcel &data, MessageParcel &reply)
{
    uint32_t index = data.ReadUint32();
    int64_t time = 0;
    auto res = GetTimeByFrameIndex(index, time);
    CHECK_AND_RETURN_RET(res == MSERR_OK, res);
    reply.WriteInt64(time);
    return MSERR_OK;
}

int32_t AVMetadataHelperServiceStub::GetFrameIndexByTime(MessageParcel &data, MessageParcel &reply)
{
    int64_t time = data.ReadInt64();
    uint32_t index = 0;
    auto res = GetFrameIndexByTime(time, index);
    CHECK_AND_RETURN_RET(res == MSERR_OK, res);
    reply.WriteUint32(index);
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
