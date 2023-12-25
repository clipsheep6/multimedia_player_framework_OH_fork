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
#include "buffer/avsharedmemorybase.h"
#include "avdatasrcmemory.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVSharedMemoryIPC"};
}

namespace OHOS {
namespace Media {
int32_t WriteAVSharedMemoryToParcel(const std::shared_ptr<AVSharedMemory> &memory, MessageParcel &parcel)
{
    std::shared_ptr<AVSharedMemoryBase> baseMem = std::static_pointer_cast<AVSharedMemoryBase>(memory);
    CHECK_AND_RETURN_RET_LOG(baseMem != nullptr, MSERR_INVALID_VAL, "memory is nullptr");

    int32_t fd = baseMem->GetFd();
    int32_t size = baseMem->GetSize();

    (void)parcel.WriteFileDescriptor(fd);
    parcel.WriteInt32(size);
    parcel.WriteUint32(baseMem->GetFlags());
    parcel.WriteString(baseMem->GetName());

    return MSERR_OK;
}

std::shared_ptr<AVSharedMemory> ReadAVSharedMemoryFromParcel(MessageParcel &parcel)
{
    int32_t fd  = parcel.ReadFileDescriptor();
    int32_t size = parcel.ReadInt32();
    uint32_t flags = parcel.ReadUint32();
    std::string name = parcel.ReadString();

    std::shared_ptr<AVSharedMemory> memory = AVSharedMemoryBase::CreateFromRemote(fd, size, flags, name);
    CHECK_AND_RETURN_RET_LOG(memory != nullptr && memory->GetBase() != nullptr, nullptr,
        "create remote AVSharedMemoryBase failed");

    (void)::close(fd);
    return memory;
}

std::shared_ptr<AVSharedMemory> ReadAVDataSrcMemoryFromParcel(MessageParcel &parcel)
{
    int32_t fd  = parcel.ReadFileDescriptor();
    int32_t size = parcel.ReadInt32();
    uint32_t flags = parcel.ReadUint32();
    std::string name = parcel.ReadString();

    std::shared_ptr<AVSharedMemory> memory = AVDataSrcMemory::CreateFromRemote(fd, size, flags, name);
    CHECK_AND_RETURN_RET_LOG(memory != nullptr && memory->GetBase() != nullptr, nullptr,
        "create remote AVDataSrcMemory failed");

    (void)::close(fd);
    return memory;
}
} // namespace Media
} // namespace OHOS
