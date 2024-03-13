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

#include "appsrc_memory.h"
#include "avdatasrcmemory.h"
#include "media_dfx.h"
#include "media_log.h"
#include "media_errors.h"
#include "securec.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AppSrcMemory"};
}

namespace OHOS {
namespace Media {
// Check whether (srcB, endB) belongs to (srcA, endA)
static bool IsInnerRange(uint32_t srcA, uint32_t endA, uint32_t srcB, uint32_t endB)
{
    MEDIA_LOGD("srcA is: %{public}u, endA is: %{public}u, srcB is: %{public}u, endB is: %{public}u,",
        srcA, endA, srcB, endB);
    if (srcA < endA) {
        return srcB < endB && srcB >= srcA && endB <= endA;
    } else {
        return (srcB < endB && srcB >= srcA && endB >= srcA) || (srcB < endB && srcB <= endA && endB <= endA) ||
            (srcB >= srcA && endB <= endA);
    }
}

AppsrcMemory::AppsrcMemory()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AppsrcMemory::~AppsrcMemory()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

uint32_t AppsrcMemory::GetBufferSize() const
{
    return bufferSize_;
}

std::shared_ptr<AVSharedMemory> AppsrcMemory::GetMem()
{
    return mem_;
}

uint32_t AppsrcMemory::GetFreeSize() const
{
    MEDIA_LOGD("GetFreeSize, begin is: %{public}u, end is: %{public}u, availableBegin is: %{public}u",
        begin_, end_, availableBegin_);
    uint32_t freeSize;
    if (noFreeBuffer_) {
        freeSize = 0;
    } else {
        uint32_t end = end_;
        if (!unreturnedBuffers_.empty()) {
            MEDIA_LOGD("unreturnedBuffers begin: %{public}u - %{public}u",
                unreturnedBuffers_.front().first, unreturnedBuffers_.front().second);
            end = unreturnedBuffers_.front().first;
        }
        freeSize = begin_ <= end ? end - begin_ + 1 : bufferSize_ - begin_ + end + 1;
    }
    MEDIA_LOGD("GetFreeSize is: %{public}u", freeSize);
    MediaTrace::CounterTrace("AppsrcMemory::freeSize", freeSize);
    return freeSize;
}

uint32_t AppsrcMemory::GetAvailableSize() const
{
    MEDIA_LOGD("GetAvailableSize, begin is: %{public}u, end is: %{public}u, availableBegin is: %{public}u",
        begin_, end_, availableBegin_);
    uint32_t availableSize;
    if (availableBegin_ == begin_ && noAvailableBuffer_) {
        availableSize = 0;
    } else {
        availableSize = availableBegin_ < begin_ ?
            begin_ - availableBegin_ :
            bufferSize_ - availableBegin_ + begin_;
    }
    MEDIA_LOGD("GetAvailableSize is: %{public}u", availableSize);
    MediaTrace::CounterTrace("AppsrcMemory::availableSize", availableSize);
    return availableSize;
}

uint32_t AppsrcMemory::GetBeginPos() const
{
    return begin_;
}

uint32_t AppsrcMemory::GetAvailableBeginPos() const
{
    return availableBegin_;
}

uint64_t AppsrcMemory::GetPushOffset() const
{
    return pushOffset_;
}

uint64_t AppsrcMemory::GetFilePos() const
{
    return filePos_;
}

void AppsrcMemory::SetBufferSize(uint32_t bufferSize)
{
    bufferSize_ = bufferSize;
    MEDIA_LOGD("Set bufferSize_ is %{public}u", bufferSize_);
}

void AppsrcMemory::SetMem(std::shared_ptr<AVSharedMemory> mem)
{
    mem_ = mem;
}

void AppsrcMemory::SetNoFreeBuffer(bool flag)
{
    if (noFreeBuffer_ != flag) {
        noFreeBuffer_ = flag;
        MEDIA_LOGD("noFreeBuffer_ set to: %{public}u", noFreeBuffer_);
    }
}

void AppsrcMemory::SetNoAvailableBuffer(bool flag)
{
    if (noAvailableBuffer_ != flag) {
        noAvailableBuffer_ = flag;
        MEDIA_LOGD("noAvailableBuffer_ set to: %{public}u", noAvailableBuffer_);
    }
}

void AppsrcMemory::ResetMemParam()
{
    filePos_ = 0;
    begin_ = 0;
    end_ = bufferSize_ - 1;
    availableBegin_ = 0;
    pushOffset_ = 0;
    noFreeBuffer_ = false;
    noAvailableBuffer_ = true;
}

void AppsrcMemory::RestoreMemParam()
{
    availableBegin_ = 0;
    end_ = bufferSize_ - 1;
}

void AppsrcMemory::SeekAndChangePos(uint64_t pos)
{
    MEDIA_LOGD("Enter SeekAndChangePos");
    pushOffset_ = pos;
    bool hasUnreturnedBuffer = (((end_ + 1) % bufferSize_) != availableBegin_) ? true : false;
    uint32_t availableSize = GetAvailableSize();
    // Check availableBuffer is Successive and seek location is cached
    if ((IsMemSuccessive() || noFreeBuffer_) && filePos_ - availableSize <= pos && filePos_ >= pos) {
        // if availableBuffer is Successive and seek location is cached, Adjust end_ according to hasUnreturnedBuffer
        if (hasUnreturnedBuffer) {
            uint32_t unusedBufferEnd = (availableBegin_ + availableSize - (filePos_ - pos)) % bufferSize_;
            MEDIA_LOGD("unusedBuffers push begin: %{public}u, end: %{public}u", availableBegin_, unusedBufferEnd);
            PushUnusedBuffers({availableBegin_, unusedBufferEnd});
            availableBegin_ = begin_ - (filePos_ - pos);
        } else {
            uint32_t pad = noFreeBuffer_ ? bufferSize_ : 0;
            availableBegin_ = (begin_ + pad) - (filePos_ - pos);
            end_ = availableBegin_ == 0 ? bufferSize_ - 1 : availableBegin_ - 1;
        }
    } else if (filePos_ - availableSize <= pos && filePos_ >= pos) {
        // seek location is cached
        if (hasUnreturnedBuffer) {
            uint32_t unusedBufferEnd = (availableBegin_ + availableSize - (filePos_ - pos)) % bufferSize_;
            MEDIA_LOGD("unusedBuffers push begin: %{public}u, end: %{public}u", availableBegin_, unusedBufferEnd);
            PushUnusedBuffers({availableBegin_, unusedBufferEnd});
            availableBegin_ = ((begin_ + bufferSize_) - (filePos_ - pos)) % bufferSize_;
        } else {
            availableBegin_ = ((begin_ + bufferSize_) - (filePos_ - pos)) % bufferSize_;
            end_ = availableBegin_ == 0 ? bufferSize_ - 1 : availableBegin_ - 1;
        }
    } else {
        // seek location not cached
        filePos_ = pos;
        begin_ = availableBegin_;
        if (!hasUnreturnedBuffer) {
            end_ = availableBegin_ == 0 ? bufferSize_ - 1 : availableBegin_ - 1;
        }
        SetNoAvailableBuffer(true);
    }
    availableSize = GetAvailableSize();
    if (availableSize != bufferSize_) {
        SetNoFreeBuffer(false);
    }
}

void AppsrcMemory::PullBufferAndChangePos(uint32_t readSize)
{
    MEDIA_LOGD("Enter PullBufferAndChangePos");
    begin_ = (begin_ + readSize) % bufferSize_;
    if (begin_ == (end_ + 1) % bufferSize_) {
        SetNoFreeBuffer(true);
    }
    SetNoAvailableBuffer(false);
    filePos_ += readSize;
}

void AppsrcMemory::PushBufferAndChangePos(uint32_t pushSize, bool isCopy)
{
    MEDIA_LOGD("Enter PushBufferAndChangePos");
    if (isCopy) {
        if ((end_ + 1) % bufferSize_ == availableBegin_) {
            end_ = (end_ + pushSize) % bufferSize_;
        } else {
            PushUnusedBuffers({availableBegin_, (availableBegin_ + pushSize) % bufferSize_});
        }
    }
    pushOffset_ += pushSize;
    availableBegin_ = (availableBegin_ + pushSize) % bufferSize_;
    if (availableBegin_ == begin_) {
        SetNoAvailableBuffer(true);
    }
}

bool AppsrcMemory::FreeBufferAndChangePos(uint32_t offset, uint32_t length, bool isCopymode)
{
    MEDIA_LOGD("Enter FreeBufferAndChangePos");
    if ((end_ + 1) % bufferSize_ != offset && !isCopymode) {
        // Check the buffer after end_ is unusedbuffer, and Re-mark as freeBuffer
        RemoveUnusedBuffer();
        // Check the buffer to be returned is unreturnedbuffer and adjust unusedBuffers_ and unreturnedBuffers_
        bool isProcessed = ProcessBuffer(offset, length);
        if (isProcessed) {
            return true;
        }
        if ((end_ + 1) % bufferSize_ != offset) {
            CHECK_AND_RETURN_RET(PushBufferToUnreturnedBuffers(offset, length), false);
        }
    }
    end_ = offset + length - 1;
    SetNoFreeBuffer(false);
    return true;
}

bool AppsrcMemory::CopyBufferAndChangePos(std::shared_ptr<AppsrcMemory> &mem)
{
    MEDIA_LOGD("Enter CopyBufferAndChangePos");
    uint8_t *dstBase = std::static_pointer_cast<AVDataSrcMemory>(mem_)->GetInnerBase();
    uint8_t *srcBase = std::static_pointer_cast<AVDataSrcMemory>(mem->GetMem())->GetInnerBase();
    uint32_t size = mem->GetBufferSize();
    uint32_t copyBegin = mem->GetAvailableBeginPos();
    uint32_t availableSize = mem->GetAvailableSize();
    if (availableSize && mem->IsMemSuccessive()) {
        MEDIA_LOGD("Copy buffer, and buffer size is: %{public}u", availableSize);
        errno_t rc = memcpy_s(dstBase, availableSize, srcBase + copyBegin, availableSize);
        CHECK_AND_RETURN_RET_LOG(rc == EOK, false, "get mem is failed");
    } else if (availableSize) {
        uint32_t copySize = size - copyBegin;
        MEDIA_LOGD("Copy buffer, and buffer size is: %{public}u", copySize);
        errno_t rc = memcpy_s(dstBase, copySize, srcBase + copyBegin, copySize);
        CHECK_AND_RETURN_RET_LOG(rc == EOK, false, "get mem is failed");
        dstBase += copySize;
        copySize = availableSize - (size - copyBegin);
        if (copySize) {
            MEDIA_LOGD("Copy buffer, and buffer size is: %{public}u", copySize);
            rc = memcpy_s(dstBase, copySize, srcBase, copySize);
            CHECK_AND_RETURN_RET_LOG(rc == EOK, false, "get mem is failed");
        }
    }
    begin_ = availableSize;
    filePos_ = mem->GetFilePos();
    pushOffset_ = mem->GetPushOffset();
    if (availableSize == size) {
        mem->SetMem(nullptr);
        mem = nullptr;
    } else if (availableSize) {
        mem->PushUnusedBuffers({copyBegin, mem->GetBeginPos()});
    }
    PrintCurPos();
    MEDIA_LOGD("Exit CopyBufferAndChangePos");
    return true;
}

void AppsrcMemory::RemoveUnusedBuffer()
{
    MEDIA_LOGD("Enter RemoveUnusedBuffer");
    while (!unusedBuffers_.empty() && (end_ + 1) % bufferSize_ == unusedBuffers_.front().first) {
        end_ = unusedBuffers_.front().second - 1;
        MEDIA_LOGI("unusedBuffers pop %{public}u - %{public}u",
            unusedBuffers_.front().first, unusedBuffers_.front().second);
        unusedBuffers_.pop();
    }
}

bool AppsrcMemory::ProcessBuffer(uint32_t offset, uint32_t length)
{
    MEDIA_LOGD("Enter ProcessBuffer");
    if ((end_ + 1) % bufferSize_ == offset) {
        return false;
    }
    std::deque<std::pair<uint32_t, uint32_t>>::iterator iter;
    bool flag = IsUnreturnedBuffer(offset, length, iter);
    if (flag) {
        while (!unusedBuffers_.empty() && iter != unreturnedBuffers_.end() &&
            iter->first == unusedBuffers_.front().first) {
            MEDIA_LOGI("unusedBuffers %{public}u - %{public}u",
                unusedBuffers_.front().first, unusedBuffers_.front().second);
            MEDIA_LOGI("iter %{public}u - %{public}u", iter->first, iter->second);
            if (iter->second == unusedBuffers_.front().second) {
                iter = unreturnedBuffers_.erase(iter);
                MEDIA_LOGD("unreturnedBuffers pop");
            } else {
                iter->first = unusedBuffers_.front().second;
                MEDIA_LOGD("unreturnedBuffers first change to : %{public}u", iter->first);
            }
            unusedBuffers_.pop();
            MEDIA_LOGD("unusedBuffers pop");
        }
        return true;
    }

    return false;
}

bool AppsrcMemory::IsUnreturnedBuffer(uint32_t offset, uint32_t length,
    std::deque<std::pair<uint32_t, uint32_t>>::iterator &iter)
{
    uint32_t pad;
    for (iter = unreturnedBuffers_.begin(); iter != unreturnedBuffers_.end(); ++iter) {
        pad = iter->first < iter->second ? 0 : bufferSize_;
        if (!IsInnerRange(iter->first, iter->second + pad, offset, offset + length)) {
            continue;
        }
        if (offset == iter->first && offset + length == iter->second + pad) {
            iter = unreturnedBuffers_.erase(iter);
            MEDIA_LOGD("unreturnedBuffers pop");
        } else if (offset == iter->first) {
            iter->first = (offset + length) % bufferSize_;
            MEDIA_LOGD("unreturnedBuffers first change to : %{public}u", iter->first);
        } else if (offset + length == iter->second + pad) {
            iter->second = offset;
            MEDIA_LOGD("unreturnedBuffers second change to : %{public}u", iter->second);
        } else {
            iter = unreturnedBuffers_.insert(iter, {iter->first, offset});
            MEDIA_LOGD("unreturnedBuffers insert : %{public}u, %{public}u", iter->first, offset);
            (iter + 1)->first = (offset + length) % bufferSize_;
            MEDIA_LOGD("unreturnedBuffers first change to : %{public}u", (iter + 1)->first);
        }
        return true;
    }
    return false;
}

bool AppsrcMemory::PushBufferToUnreturnedBuffers(uint32_t offset, uint32_t length)
{
    MEDIA_LOGD("Enter PushBufferToUnreturnedBuffers");
    CHECK_AND_RETURN_RET_LOG(unreturnedBuffers_.empty() || !IsInnerRange(unreturnedBuffers_.begin()->first,
        unreturnedBuffers_.rbegin()->second, offset, (offset + length) % bufferSize_),
        false, "mempool error, end_ is %{public}u offset is %{public}u", end_, offset)
    unreturnedBuffers_.push_back({(end_ + 1) % bufferSize_, offset});
    MEDIA_LOGD("unreturnedBuffers push begin: %{public}u, end: %{public}u",
        (end_ + 1) % bufferSize_, offset);
    return true;
}

bool AppsrcMemory::IsMemSuccessive()
{
    return availableBegin_ < begin_;
}

bool AppsrcMemory::IsNeedCopy(uint32_t copySize)
{
    return copySize > (bufferSize_ - availableBegin_) ? true : false;
}

void AppsrcMemory::PushUnusedBuffers(std::pair<uint32_t, uint32_t> unusedBuffer)
{
    unusedBuffers_.push(unusedBuffer);
    MEDIA_LOGI("unusedBuffers push %{public}u - %{public}u", unusedBuffer.first, unusedBuffer.second);
}

void AppsrcMemory::PrintCurPos()
{
    MEDIA_LOGD("free mem begin is: %{public}u, free mem end is: %{public}u,"
        "available mem begin is: %{public}u, filePos is: %{public}" PRIu64 "",
        begin_, end_, availableBegin_, filePos_);
}

void AppsrcMemory::CheckBufferUsage()
{
    MEDIA_LOGD("Enter CheckBufferUsage");
    size_t queueSize = unusedBuffers_.size();
    for (size_t i = 0; i < queueSize; i++) {
        MEDIA_LOGD("unusedBuffers begin: %{public}u, end: %{public}u",
            unusedBuffers_.front().first, unusedBuffers_.front().second);
        unusedBuffers_.push(unusedBuffers_.front());
        unusedBuffers_.pop();
    }
    for (auto i = unreturnedBuffers_.begin(); i != unreturnedBuffers_.end(); ++i) {
        MEDIA_LOGD("unreturnedBuffers begin: %{public}u, end: %{public}u", i->first, i->second);
    }
}
}
}