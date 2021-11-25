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

#ifndef REFCNT_SHARED_MEMORY_H
#define REFCNT_SHARED_MEMORY_H

#include <atomic>
#include <functional>
#include <string>
#include "nocopyable.h"
#include "avsharedmemorybase.h"

namespace OHOS {
namespace Media {
/**
 * @brief A simple AVSharedMemoryBase extended implementation based on cross-process refcount.
 *
 * Use this version if you don't have the explicit communication method to notify the local
 * process that the remote process has already ended up the access to the shared memory chunk,
 * then the local process can securely write the shared memory chunk again without worring
 * about overwriting the data.
 *
 * Notice, you should use this implementation only when the memory is allocated from memory pool,
 * or it will be meanless to utilize the cross-process refcount.
 *
 * Notice, the constructor and destructor of the RefCntSharedMemory will not increase and decrease
 * the refcount of the underlying shared memory. The refcount have to be changed by calling the
 * "AddRefCount" interface.
 */
class __attribute__((visibility("default"))) RefCntSharedMemory : public AVSharedMemoryBase {
public:
    enum : uint32_t {
        TYPE = 'RCSM'
    };

    RefCntSharedMemory(int32_t size, uint32_t flags, const std::string &name)
        : AVSharedMemoryBase(size + sizeof(SharedControlBlock), flags, name)
    {
        typeSet_.insert(TYPE);
    }

    RefCntSharedMemory(int32_t fd, int32_t size, uint32_t flags, const std::string &name)
        : AVSharedMemoryBase(fd, size + sizeof(SharedControlBlock), flags, name)
    {
        typeSet_.insert(TYPE);
    }

    ~RefCntSharedMemory()
    {
        uint8_t *addr = AVSharedMemoryBase::GetBase();
        if (addr == nullptr || IsRemote()) {
            return;
        }

        auto block = reinterpret_cast<SharedControlBlock *>(addr);
        std::atomic_store(&block->deadObject, true);
    }

    uint8_t *GetBase() const override
    {
        uint8_t *addr = AVSharedMemoryBase::GetBase();
        return (addr != nullptr) ? (addr + sizeof(SharedControlBlock)) : nullptr;
    }

    int32_t GetSize() const override
    {
        uint8_t *addr = AVSharedMemoryBase::GetBase();
        int32_t size = AVSharedMemoryBase::GetSize();
        return (addr != nullptr) ? (size - sizeof(SharedControlBlock)) : 0;
    }

    uint32_t GetRefCount()
    {
        uint8_t *addr = AVSharedMemoryBase::GetBase();
        if (addr == nullptr) {
            return 0;
        }

        auto block = reinterpret_cast<SharedControlBlock *>(addr);
        return std::atomic_load(&block->refCount);
    }

    int32_t AddRefCount(int32_t refCount)
    {
        uint8_t *addr = AVSharedMemoryBase::GetBase();
        if (addr == nullptr) {
            return 0;
        }

        auto block = reinterpret_cast<SharedControlBlock *>(addr);
        return std::atomic_fetch_and(&block->refCount, refCount);
    }

    bool IsDeadObject() const
    {
        uint8_t *addr = AVSharedMemoryBase::GetBase();
        if (addr == nullptr) {
            return true;
        }

        auto block = reinterpret_cast<SharedControlBlock *>(addr);
        return std::atomic_load(&block->deadObject);
    }

    // Not MT-Safe
    void SetDeathNotifier(std::function<void(RefCntSharedMemory *)> notifier)
    {
        if (notifier_ != nullptr) {
            notifier_ = notifier;
        }
    }

    DISALLOW_COPY_AND_MOVE(RefCntSharedMemory);

private:
    struct SharedControlBlock {
        std::atomic<int> refCount;
        std::atomic<bool> deadObject;
    };

    std::function<void(RefCntSharedMemory *)> notifier_;
};
}
}
#endif
