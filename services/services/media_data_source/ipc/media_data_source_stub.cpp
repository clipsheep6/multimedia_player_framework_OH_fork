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

#include "media_data_source_stub.h"
#include "media_log.h"
#include "media_errors.h"
#include "media_data_source.h"
#include "avdatasrcmemory.h"
#include "avsharedmemory_ipc.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaDataSourceStub"};
}

namespace OHOS {
namespace Media {
class MediaDataSourceStub::BufferCache : public NoCopyable {
public:
    BufferCache() = default;
    ~BufferCache() = default;

    int32_t ReadFromParcel(MessageParcel &parcel, std::shared_ptr<AVSharedMemory> &memory)
    {
        CacheFlag flag = static_cast<CacheFlag>(parcel.ReadUint8());
        if (flag == CacheFlag::HIT_CACHE) {
            MEDIA_LOGD("HIT_CACHE");
            memory = caches_;
            return MSERR_OK;
        } else {
            MEDIA_LOGD("UPDATE_CACHE");
            memory = ReadAVDataSrcMemoryFromParcel(parcel);
            CHECK_AND_RETURN_RET(memory != nullptr, MSERR_INVALID_VAL);
            caches_ = memory;
            return MSERR_OK;
        }
    }

private:
    std::shared_ptr<AVSharedMemory> caches_;
};

MediaDataSourceStub::MediaDataSourceStub(const std::shared_ptr<IMediaDataSource> &dataSrc)
    : dataSrc_(dataSrc)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaDataSourceStub::~MediaDataSourceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int MediaDataSourceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (MediaDataSourceStub::GetDescriptor() != remoteDescriptor) {
        MEDIA_LOGE("Invalid descriptor");
        return MSERR_INVALID_OPERATION;
    }

    if (BufferCache_ == nullptr) {
        BufferCache_ = std::make_unique<BufferCache>();
    }

    switch (static_cast<ListenerMsg>(code)) {
        case ListenerMsg::READ_AT: {
            std::shared_ptr<AVSharedMemory> memory = nullptr;
            if (BufferCache_ != nullptr) {
                int32_t ret = BufferCache_->ReadFromParcel(data, memory);
                CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
            }
            uint32_t offset = data.ReadUint32();
            uint32_t length = data.ReadUint32();
            int64_t pos = data.ReadInt64();
            std::static_pointer_cast<AVDataSrcMemory>(memory)->SetOffset(offset);
            MEDIA_LOGD("offset is %{public}u", offset);
            int32_t realLen = ReadAt(memory, length, pos);
            reply.WriteInt32(realLen);
            return MSERR_OK;
        }
        case ListenerMsg::GET_SIZE: {
            int64_t size = 0;
            int32_t ret = GetSize(size);
            reply.WriteInt64(size);
            reply.WriteInt32(ret);
            return MSERR_OK;
        }
        default: {
            MEDIA_LOGE("default case, need check MediaDataSourceStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

int32_t MediaDataSourceStub::ReadAt(const std::shared_ptr<AVSharedMemory> &mem, uint32_t length, int64_t pos,
    bool isHiStreamer)
{
    MEDIA_LOGD("ReadAt in isHiStreamer =  %{public}d", isHiStreamer);
    CHECK_AND_RETURN_RET_LOG(dataSrc_ != nullptr, SOURCE_ERROR_IO, "dataSrc_ is nullptr");
    CHECK_AND_RETURN_RET_LOG(mem != nullptr, MSERR_NO_MEMORY, "mem is nullptr");
    return dataSrc_->ReadAt(mem, length, pos);
}

int32_t MediaDataSourceStub::GetSize(int64_t &size)
{
    CHECK_AND_RETURN_RET_LOG(dataSrc_ != nullptr, MSERR_INVALID_OPERATION, "dataSrc_ is nullptr");
    return dataSrc_->GetSize(size);
}
} // namespace Media
} // namespace OHOS