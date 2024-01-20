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

#include "media_data_source_proxy.h"
#include "media_log.h"
#include "media_errors.h"
#include "media_dfx.h"
#include "avdatasrcmemory.h"
#include "avsharedmemory_ipc.h"
#include "meta/any.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaDataSourceProxy"};
}

namespace OHOS {
namespace Media {
class MediaDataSourceProxy::BufferCache : public NoCopyable {
public:
    BufferCache()
    {
        caches_ = nullptr;
    }
    ~BufferCache()
    {
        caches_ = nullptr;
    }

    int32_t WriteToParcel(const std::shared_ptr<AVSharedMemory> &memory, MessageParcel &parcel)
    {
        CHECK_AND_RETURN_RET_LOG(memory != nullptr, MSERR_NO_MEMORY, "memory is nullptr");
        CacheFlag flag;
        if (caches_ != nullptr && caches_ == memory.get()) {
            MEDIA_LOGI("HIT_CACHE");
            flag = CacheFlag::HIT_CACHE;
            parcel.WriteUint8(static_cast<uint8_t>(flag));
            return MSERR_OK;
        } else {
            MEDIA_LOGI("UPDATE_CACHE");
            flag = CacheFlag::UPDATE_CACHE;
            caches_ = memory.get();
            parcel.WriteUint8(static_cast<uint8_t>(flag));
            return WriteAVSharedMemoryToParcel(memory, parcel);
        }
    }

private:
    AVSharedMemory *caches_;
};

MediaDataCallback::MediaDataCallback(const sptr<IStandardMediaDataSource> &ipcProxy)
    : callbackProxy_(ipcProxy)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaDataCallback::~MediaDataCallback()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}


int32_t MediaDataCallback::ReadAt(const std::shared_ptr<AVSharedMemory> &mem, uint32_t length, int64_t pos)
{
    MEDIA_LOGD("ReadAt in");
    CHECK_AND_RETURN_RET_LOG(callbackProxy_ != nullptr, SOURCE_ERROR_IO, "callbackProxy_ is nullptr");
    CHECK_AND_RETURN_RET_LOG(mem != nullptr, MSERR_NO_MEMORY, "memory is nullptr");
    return callbackProxy_->ReadAt(mem, length, pos, false);
}

int32_t MediaDataCallback::ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
{
    MEDIA_LOGD("ReadAt in");
    CHECK_AND_RETURN_RET_LOG(callbackProxy_ != nullptr, SOURCE_ERROR_IO, "callbackProxy_ is nullptr");
    CHECK_AND_RETURN_RET_LOG(mem != nullptr, MSERR_NO_MEMORY, "memory is nullptr");
    return callbackProxy_->ReadAt(mem, length, pos, true);
}

int32_t MediaDataCallback::ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
{
    MEDIA_LOGD("ReadAt in");
    CHECK_AND_RETURN_RET_LOG(callbackProxy_ != nullptr, SOURCE_ERROR_IO, "callbackProxy_ is nullptr");
    CHECK_AND_RETURN_RET_LOG(mem != nullptr, MSERR_NO_MEMORY, "memory is nullptr");
    return callbackProxy_->ReadAt(mem, length, 0, true);
}

int32_t MediaDataCallback::GetSize(int64_t &size)
{
    CHECK_AND_RETURN_RET_LOG(callbackProxy_ != nullptr, MSERR_INVALID_OPERATION, "callbackProxy_ is nullptr");
    return callbackProxy_->GetSize(size);
}

MediaDataSourceProxy::MediaDataSourceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardMediaDataSource>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaDataSourceProxy::~MediaDataSourceProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t MediaDataSourceProxy::ReadAt(const std::shared_ptr<AVSharedMemory> &mem, uint32_t length, int64_t pos,
    bool isHistreamer)
{
    MEDIA_LOGD("ReadAt in");
    CHECK_AND_RETURN_RET_LOG(mem != nullptr, MSERR_NO_MEMORY, "mem is nullptr");
    MediaTrace trace("DataSrc::ReadAt");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);

    bool token = data.WriteInterfaceToken(MediaDataSourceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    if (BufferCache_ == nullptr) {
        BufferCache_ = std::make_unique<BufferCache>();
    }
    CHECK_AND_RETURN_RET_LOG(BufferCache_ != nullptr, MSERR_NO_MEMORY, "Failed to create BufferCache_!");

    uint32_t offset = 0;
    if (!isHistreamer) {
        offset = std::static_pointer_cast<AVDataSrcMemory>(mem)->GetOffset();
    }
    MEDIA_LOGD("offset is %{public}u", offset);
    BufferCache_->WriteToParcel(mem, data);
    data.WriteUint32(offset);
    data.WriteUint32(length);
    data.WriteInt64(pos);
    int error = Remote()->SendRequest(static_cast<uint32_t>(ListenerMsg::READ_AT), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, 0, "ReadAt failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t MediaDataSourceProxy::GetSize(int64_t &size)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);

    bool token = data.WriteInterfaceToken(MediaDataSourceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int error = Remote()->SendRequest(static_cast<uint32_t>(ListenerMsg::GET_SIZE), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, -1, "GetSize failed, error: %{public}d", error);

    size = reply.ReadInt64();
    return reply.ReadInt32();
}
} // namespace Media
} // namespace OHOS