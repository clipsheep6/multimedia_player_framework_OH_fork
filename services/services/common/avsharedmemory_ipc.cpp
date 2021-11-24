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

#include "avsharedmemory_ipc.h"
#include <unistd.h>
#include "avsharedmemorybase.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVShMemIPC"};
}

namespace OHOS {
namespace Media {
enum IpcDataType : uint8_t {
    SHMEM = 1, // advance to 1 to avoid the same default value of IPC
    INDEX,
};

class RemoteSharedMemory : public AVSharedMemory {
public:
    RemoteSharedMemory(std::shared_ptr<RefCntSharedMemory> memory) : memory_(memory) {}
    ~RemoteSharedMemory()
    {
        if (memory_ != nullptr) {
            (void)memory_->AddRefCount(-1);
        }
    }

    uint8_t *GetBase() const override
    {
        return (memory_ != nullptr) ? memory_->GetBase() : nullptr;
    }

    int32_t GetSize() const override
    {
        return (memory_ != nullptr) ? memory_->GetSize() : 0;
    }

    uint32_t GetFlags() const override
    {
        return (memory_ != nullptr) ? memory_->GetFlags() : 0;
    }

    DISALLOW_COPY_AND_MOVE(RemoteSharedMemory);

private:
    std::shared_ptr<RefCntSharedMemory> memory_;
};

int32_t AVShMemIPCStatic::WriteToParcel(const std::shared_ptr<AVSharedMemory> &memory, MessageParcel &parcel)
{
    std::shared_ptr<AVSharedMemoryBase> baseMem = std::static_pointer_cast<AVSharedMemoryBase>(memory);
    if (baseMem == nullptr)  {
        MEDIA_LOGE("invalid pointer");
        return MSERR_INVALID_VAL;
    }

    int32_t fd = baseMem->GetFd();
    int32_t size = baseMem->GetSize();
    CHECK_AND_RETURN_RET_LOG(fd > 0 || size > 0, MSERR_INVALID_VAL, "fd or size invalid");

    (void)parcel.WriteFileDescriptor(fd);
    parcel.WriteInt32(size);
    parcel.WriteUint32(baseMem->GetFlags());
    parcel.WriteString(baseMem->GetName());

    return MSERR_OK;
}

std::shared_ptr<AVSharedMemory> AVShMemIPCStatic::ReadFromParcel(MessageParcel &parcel)
{
    int32_t fd  = parcel.ReadFileDescriptor();
    int32_t size = parcel.ReadInt32();
    uint32_t flags = parcel.ReadUint32();
    std::string name = parcel.ReadString();

    std::shared_ptr<AVSharedMemoryBase> memory = std::make_shared<AVSharedMemoryBase>(fd, size, flags, name);
    int32_t ret = memory->Init();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("create remote AVSharedMemoryBase failed, ret = %{public}d", ret);
        memory = nullptr;
    }

    (void)::close(fd);
    return memory;
}

int32_t AVShMemIPCLocal::WriteToParcel(const std::shared_ptr<AVSharedMemory> &memory, MessageParcel &parcel)
{
    // clear the invalid cache
    for (auto iter = memoryCache_.begin(); iter != memoryCache_.end();) {
        if (iter->first->IsDeadObject()) {
            iter = memoryCache_.erase(iter);
        } else {
            iter++;
        }
    }

    std::shared_ptr<AVSharedMemoryBase> baseMem = std::static_pointer_cast<AVSharedMemoryBase>(memory);
    CHECK_AND_RETURN_RET_LOG(baseMem != nullptr, MSERR_INVALID_VAL, "invalid pointer");

    auto refCntMem = baseMem->GetInterface<RefCntSharedMemory>(baseMem);
    CHECK_AND_RETURN_RET_LOG(refCntMem != nullptr, MSERR_INVALID_VAL, "not RefCntShMem pointer");

    int32_t fd = refCntMem->GetFd();
    int32_t size = refCntMem->GetSize();
    CHECK_AND_RETURN_RET_LOG(fd > 0 && size > 0, MSERR_INVALID_VAL, "fd or size invalid");

    auto iter = memoryCache_.find(refCntMem);
    if (iter != memoryCache_.end()) {
        (void)parcel.WriteUint8(IpcDataType::INDEX);
        (void)parcel.WriteUint32(iter->second);
    } else {
        size_t index = static_cast<uint32_t>(memoryCache_.size());
        auto rst = memoryCache_.emplace(refCntMem, index);
        CHECK_AND_RETURN_RET_LOG(rst.second, MSERR_NO_MEMORY, "cache RefCntShMem failed");
        (void)refCntMem->AddRefCount(1);

        (void)parcel.WriteUint8(IpcDataType::SHMEM);
        (void)parcel.WriteUint32(index);
        (void)parcel.WriteFileDescriptor(fd);
        (void)parcel.WriteInt32(size);
        (void)parcel.WriteUint32(baseMem->GetFlags());
        (void)parcel.WriteString(baseMem->GetName());
    }

    return MSERR_OK;
}

std::shared_ptr<AVSharedMemory> AVShMemIPCRemote::ReadFromParcel(MessageParcel &parcel)
{
    // clear the invalid cache
    for (auto iter = indexCache_.begin(); iter != indexCache_.end();) {
        if (iter->second->IsDeadObject()) {
            iter = indexCache_.erase(iter);
        } else {
            iter++;
        }
    }

    uint8_t ipcDataType = parcel.ReadUint8();
    if (ipcDataType == IpcDataType::INDEX) {
        uint32_t index = parcel.ReadUint32();
        auto iter = indexCache_.find(index);
        CHECK_AND_RETURN_RET_LOG(iter != indexCache_.end(), nullptr, "invalid index");
        return std::make_shared<RemoteSharedMemory>(iter->second);
    }

    if (ipcDataType == IpcDataType::SHMEM) {
        uint32_t index = parcel.ReadUint32();
        int32_t fd  = parcel.ReadFileDescriptor();
        int32_t size = parcel.ReadInt32();
        uint32_t flags = parcel.ReadUint32();
        std::string name = parcel.ReadString();

        auto refCntMem = std::make_shared<RefCntSharedMemory>(fd, size, flags, name);
        if (refCntMem->Init() != MSERR_OK) {
            MEDIA_LOGE("invalid memory, map to remote process failed");
            return nullptr;
        }

        auto remoteMem = std::make_shared<RemoteSharedMemory>(refCntMem);
        auto rst = indexCache_.emplace(index, refCntMem);
        CHECK_AND_RETURN_RET_LOG(rst.second, remoteMem, "cache failed, will not cache the memory");
        return remoteMem;
    }

    MEDIA_LOGE("invalid type flag: %{public}hhu, can not resolve the parcel", ipcDataType);
    return nullptr;
}
}
}
