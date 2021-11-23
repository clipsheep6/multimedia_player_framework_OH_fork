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

#ifndef AVSHAREDMEMORY_IPC_H
#define AVSHAREDMEMORY_IPC_H

#include <unordered_map>
#include <message_parcel.h>
#include "nocopyable.h"
#include "avsharedmemory.h"
#include "refcnt_shared_memory.h"

namespace OHOS {
namespace Media {
class AVShMemIPCStatic {
public:
    static int32_t WriteToParcel(const std::shared_ptr<AVSharedMemory> &memory, MessageParcel &parcel);
    static std::shared_ptr<AVSharedMemory> ReadFromParcel(MessageParcel &parcel);

private:
    AVShMemIPCStatic() = default;
    ~AVShMemIPCStatic() = default;
};

/**
 * @brief Write the AVSharedMemory to IPC Parcel for local process. This object should be utilized
 * with the AVShMemIPCRemote and RefCntSharedMemory. It will cache all memory to decrease the
 * overhead of mmap.
 */
class AVShMemIPCLocal {
public:
    AVShMemIPCLocal() = default;
    ~AVShMemIPCLocal() = default;

    int32_t WriteToParcel(const std::shared_ptr<AVSharedMemory> &memory, MessageParcel &parcel);

    DISALLOW_COPY_AND_MOVE(AVShMemIPCLocal);

private:
    std::unordered_map<std::shared_ptr<RefCntSharedMemory>, uint32_t> memoryCache_;
};

/**
 * @brief Read the AVSharedMemory from IPC Parcel for remote process. This object should be utilized
 * with the AVShMemIPCLocal and RefCntSharedMemory. It will cache all memory to decrease the overhead
 * of mmap.
 */
class AVShMemIPCRemote {
public:
    AVShMemIPCRemote() = default;
    ~AVShMemIPCRemote() = default;

    std::shared_ptr<AVSharedMemory> ReadFromParcel(MessageParcel &parcel);

    DISALLOW_COPY_AND_MOVE(AVShMemIPCRemote);

private:
    std::unordered_map<uint32_t, std::shared_ptr<RefCntSharedMemory>> indexCache_;
};
}
}
#endif
