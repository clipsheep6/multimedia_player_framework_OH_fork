/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef AVDATASRCMEMORY_H
#define AVDATASRCMEMORY_H

#include <string>
#include "nocopyable.h"
#include "buffer/avsharedmemorybase.h"

namespace OHOS {
namespace Media {
class __attribute__((visibility("default"))) AVDataSrcMemory
    : public AVSharedMemoryBase {
public:
    /**
     * @brief Construct a new AVDataSrcMemory object. This function should only be used in the
     * local process.
     *
     * @param size the memory's size, bytes.
     * @param flags the memory's accessible flags, refer to {@AVSharedMemory::Flags}.
     * @param name the debug string
     */
    static std::shared_ptr<AVSharedMemory> CreateFromLocal(
        int32_t size, uint32_t flags, const std::string &name);

    /**
     * @brief Construct a new AVDataSrcMemory object. This function should only be used in the
     * remote process.
     *
     * @param fd the memory's fd
     * @param size the memory's size, bytes.
     * @param flags the memory's accessible flags, refer to {@AVSharedMemory::Flags}.
     * @param name the debug string
     */
    static std::shared_ptr<AVSharedMemory> CreateFromRemote(
        int32_t fd, int32_t size, uint32_t flags, const std::string &name);

    virtual ~AVDataSrcMemory();

    /**
     * @brief Construct a new AVSharedMemoryBase object. This function should only be used in the
     * local process.
     *
     * @param size the memory's size, bytes.
     * @param flags the memory's accessible flags, refer to {@AVSharedMemory::Flags}.
     * @param name the debug string
     */
    AVDataSrcMemory(int32_t size, uint32_t flags, const std::string &name);

    /**
     * @brief Get the memory's virtual address
     * @return the memory's virtual address if the memory is valid, otherwise nullptr.
     */
    virtual uint8_t *GetBase() const override
    {
        return AVSharedMemoryBase::GetBase() + offset_;
    }

    uint8_t *GetInnerBase() const
    {
        return AVSharedMemoryBase::GetBase();
    }

    void SetOffset(uint32_t offset)
    {
        offset_ = offset;
    }

    uint32_t GetOffset() const
    {
        return offset_;
    }
protected:
    AVDataSrcMemory(int32_t fd, int32_t size, uint32_t flags, const std::string &name);
private:
    uint32_t offset_;
};
} // namespace Media
} // namespace OHOS
#endif