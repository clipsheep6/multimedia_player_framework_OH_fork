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

#include "avsharedmemorybase.h"
#include <sys/mman.h>
#include <unistd.h>
#include "ashmem.h"
#include "media_errors.h"
#include "media_log.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVSharedMemoryBase"};
    constexpr int32_t MAX_ALLOWED_SIZE = 100 * 1024 * 1024;
}

namespace OHOS {
namespace Media {
AVSharedMemoryBase::AVSharedMemoryBase(int32_t size, uint32_t flags, const std::string &name)
    : base_(nullptr), size_(size), flags_(flags), name_(name), fd_(-1), isRemote_(false)
{
    typeSet_.insert(TYPE);

    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR ", name = %{public}s",
               FAKE_POINTER(this), name_.c_str());
}

AVSharedMemoryBase::AVSharedMemoryBase(int32_t fd, int32_t size, uint32_t flags, const std::string &name)
    : base_(nullptr), size_(size), flags_(flags), name_(name), fd_(dup(fd)), isRemote_(true)
{
    typeSet_.insert(TYPE);

    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR ", name = %{public}s",
               FAKE_POINTER(this), name_.c_str());
}

AVSharedMemoryBase::~AVSharedMemoryBase()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR ", name = %{public}s",
               FAKE_POINTER(this), name_.c_str());
    Close();
}

int32_t AVSharedMemoryBase::Init()
{
    ON_SCOPE_EXIT(0) { Close(); };

    if (size_ <= 0 || size_ > MAX_ALLOWED_SIZE) {
        MEDIA_LOGE("create memory failed, size %{public}d is illegal, name = %{public}s",
                   size_, name_.c_str());
        return MSERR_INVALID_VAL;
    }

#ifdef USE_SHARED_MEM
    if (fd_ > 0) {
        int size = AshmemGetSize(fd_);
        if (size != size_) {
            MEDIA_LOGE("get size failed, name = %{public}s, size = %{public}d, "
                       "flags = 0x%{public}x, fd = %{public}d", name_.c_str(), size_, flags_, fd_);
            return MSERR_INVALID_VAL;
        }
    }

    if (fd_ <= 0) {
        fd_ = AshmemCreate(name_.c_str(), static_cast<size_t>(size_));
        if (fd_ <= 0) {
            MEDIA_LOGE("create mem failed, name = %{public}s, size = %{public}d, "
                       "flags = 0x%{public}x, fd = %{public}d", name_.c_str(), size_, flags_, fd_);
            return MSERR_INVALID_VAL;
        }
    }

    int32_t ret = MapMemory(isRemote_);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("map memory failed, name = %{public}s, size = %{public}d, "
                   "flags = 0x%{public}x, fd = %{public}d", name_.c_str(), size_, flags_, fd_);
        return ret;
    }

#else
    base_ = new (std::nothrow) uint8_t[size_];
    if (base_ == nullptr) {
        MEDIA_LOGE("create memory failed, new failed, size = %{public}d , name = %{public}s",
                   size_, name_.c_str());
        return MSERR_INVALID_OPERATION;
    }
#endif

    CANCEL_SCOPE_EXIT_GUARD(0);
    return MSERR_OK;
}

#ifdef USE_SHARED_MEM
int32_t AVSharedMemoryBase::MapMemory(bool isRemote)
{
    unsigned int prot = PROT_READ | PROT_WRITE;
    if (isRemote && (flags_ & FLAGS_READ_ONLY)) {
        prot &= ~PROT_WRITE;
    }

    int result = AshmemSetProt(fd_, static_cast<int>(prot));
    CHECK_AND_RETURN_RET(result >= 0, MSERR_INVALID_OPERATION);

    void *addr = ::mmap(nullptr, static_cast<size_t>(size_), static_cast<int>(prot), MAP_SHARED, fd_, 0);
    CHECK_AND_RETURN_RET(addr != MAP_FAILED, MSERR_INVALID_OPERATION);

    base_ = reinterpret_cast<uint8_t*>(addr);
    return MSERR_OK;
}
#endif

void AVSharedMemoryBase::Close() noexcept
{
#ifdef USE_SHARED_MEM
    if (base_ != nullptr) {
        (void)::munmap(base_, static_cast<size_t>(size_));
        base_ = nullptr;
        size_ = 0;
        flags_ = 0;
    }

    if (fd_ > 0) {
        (void)::close(fd_);
        fd_ = -1;
    }
#else
    if (base_ != nullptr) {
        delete [] base_;
    }
#endif
}
}
}
