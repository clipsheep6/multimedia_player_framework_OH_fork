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

#ifndef AVSHAREDMEMORYBASE_H
#define AVSHAREDMEMORYBASE_H

#include <string>
#include <unordered_set>
#include "nocopyable.h"
#include "avsharedmemory.h"

namespace OHOS {
namespace Media {
class __attribute__((visibility("default"))) AVSharedMemoryBase : public AVSharedMemory {
public:
    /**
     * @brief A simple RTTI mechanism is used to uniquely identify the type of the AVSharedMemoryBase object.
     * When extending a new subclass, please add the "TYPE" to "typeSet_" at the constructor of the new subclass.
     * The enumeration must be unique, or the "GetInterface" can not work properly.
     */
    enum : uint32_t {
        TYPE = 'ASMB'
    };

    /**
     * @brief Securely convert the base class pointer to a subclass pointer. For example:
     * std::shared<AVSharedMemoryBase> base = std::make_shared<Derived>(...);
     * std::shared<Derived> derived = base->GetInterface<Derived>(base);
     */
    template<typename T>
    static std::shared_ptr<T> GetInterface(std::shared_ptr<AVSharedMemoryBase> &base)
    {
        if (base->typeSet_.count(T::TYPE) != 0) {
            return std::static_pointer_cast<T>(base);
        }
        return nullptr;
    }

    /**
     * @brief Construct a new AVSharedMemoryBase object. This function should only be used in the
     * local process.
     *
     * @param size the memory's size, bytes.
     * @param flags the memory's accessible flags, refer to {@AVSharedMemory::Flags}.
     * @param name the debug string
     */
    AVSharedMemoryBase(int32_t size, uint32_t flags, const std::string &name);

    /**
     * @brief Construct a new AVSharedMemoryBase object. This function should only be used in the
     * remote process.
     *
     * @param fd the memory's fd
     * @param size the memory's size, bytes.
     * @param flags the memory's accessible flags, refer to {@AVSharedMemory::Flags}.
     * @param name the debug string
     */
    AVSharedMemoryBase(int32_t fd, int32_t size, uint32_t flags, const std::string &name);

    virtual ~AVSharedMemoryBase();

    /**
     * @brief Intialize the memory. Call this interface firstly before the other interface.
     * @return MSERR_OK if success, otherwise the errcode.
     */
    int32_t Init();

    /**
     * @brief Get the memory's virtual address
     * @return the memory's virtual address if the memory is valid, otherwise nullptr.
     */
    virtual uint8_t *GetBase() const override
    {
        return base_;
    }

    /**
     * @brief Get the memory's size
     * @return the memory's size if the memory is valid, otherwise -1.
     */
    virtual int32_t GetSize() const override
    {
        return (base_ != nullptr) ? size_ : -1;
    }

    /**
     * @brief Get the memory's flags set by the creator, refer to {@Flags}
     * @return the memory's flags if the memory is valid, otherwise 0.
     */
    virtual uint32_t GetFlags() const final
    {
        return (base_ != nullptr) ? flags_ : 0;
    }

    /**
     * @brief Get the memory's fd, which only valid when the underlying memory
     * chunk is allocated through the ashmem.
     * @return the memory's fd if the memory is allocated through the ashmem, otherwise -1.
     */
    int32_t GetFd() const
    {
        return fd_;
    }

    /**
     * @brief query whether the object is created at the remote process.
     * @return true if the object belongs to the remote process.
     */
    bool IsRemote() const
    {
        return isRemote_;
    }

    /**
     * @brief Get the memory's debug name
     * @return the memory's debug name.
     */
    std::string GetName() const
    {
        return name_;
    }

    DISALLOW_COPY_AND_MOVE(AVSharedMemoryBase);

protected:
    std::unordered_set<uint32_t> typeSet_;

private:
    int32_t MapMemory(bool isRemote);
    void Close() noexcept;

    uint8_t *base_;
    int32_t size_;
    uint32_t flags_;
    std::string name_;
    int32_t fd_;
    bool isRemote_;
};
}
}

#endif