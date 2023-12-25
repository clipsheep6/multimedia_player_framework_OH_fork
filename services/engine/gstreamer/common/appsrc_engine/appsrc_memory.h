/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
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

#ifndef APPSRC_MEMORY_H
#define APPSRC_MEMORY_H

#include <queue>
#include "buffer/avsharedmemory.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class AppsrcMemory : public NoCopyable {
public:
    AppsrcMemory();
    ~AppsrcMemory();
    uint32_t GetBufferSize() const;
    std::shared_ptr<AVSharedMemory> GetMem();
    uint32_t GetFreeSize() const;
    uint32_t GetAvailableSize() const;
    uint32_t GetBeginPos() const;
    uint32_t GetAvailableBeginPos() const;
    uint64_t GetPushOffset() const;
    uint64_t GetFilePos() const;

    void SetBufferSize(uint32_t bufferSize);
    void SetMem(std::shared_ptr<AVSharedMemory> mem);
    void SetNoFreeBuffer(bool flag);
    void SetNoAvailableBuffer(bool flag);
    void ResetMemParam();
    void RestoreMemParam();

    void SeekAndChangePos(uint64_t pos);
    void PullBufferAndChangePos(uint32_t readSize);
    void PushBufferAndChangePos(uint32_t pushSize, bool isCopy);
    bool FreeBufferAndChangePos(uint32_t offset, uint32_t length, bool isCopymode);
    bool CopyBufferAndChangePos(std::shared_ptr<AppsrcMemory> &mem);
    
    bool IsMemSuccessive();
    bool IsNeedCopy(uint32_t copySize);
    void PrintCurPos();
    void CheckBufferUsage();
private:
    void PushUnusedBuffers(std::pair<uint32_t, uint32_t> unusedBuffer);
    void RemoveUnusedBuffer();
    bool ProcessBuffer(uint32_t offset, uint32_t length);
    bool IsUnreturnedBuffer(uint32_t offset, uint32_t length,
        std::deque<std::pair<uint32_t, uint32_t>>::iterator &iter);
    bool PushBufferToUnreturnedBuffers(uint32_t offset, uint32_t length);

    std::shared_ptr<AVSharedMemory> mem_;
    uint32_t bufferSize_;
    // The position of mem in file
    uint64_t filePos_;
    // The start position that can be filled
    uint32_t begin_;
    // The end position that can be filled
    uint32_t end_;
    // The start position that can be push to appsrc
    uint32_t availableBegin_;
    // The offset of the next data to be pushed
    uint64_t pushOffset_;
    bool noFreeBuffer_ = false;
    bool noAvailableBuffer_ = true;
    // These buffers are not be used
    std::queue<std::pair<uint32_t, uint32_t>> unusedBuffers_;
    // These buffers are used but not returned
    std::deque<std::pair<uint32_t, uint32_t>> unreturnedBuffers_;
};
} // namespace Media
} // namespace OHOS
#endif /* APPSRC_MEMORY_H */