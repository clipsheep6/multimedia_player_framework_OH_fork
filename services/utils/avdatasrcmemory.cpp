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

#include "avdatasrcmemory.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVDataSrcMemory"};
}

namespace OHOS {
namespace Media {
struct AVSharedMemoryBaseImpl : public AVDataSrcMemory {
public:
    AVSharedMemoryBaseImpl(int32_t fd, int32_t size, uint32_t flags, const std::string &name)
        : AVDataSrcMemory(fd, size, flags, name) {}
};

std::shared_ptr<AVSharedMemory> AVDataSrcMemory::CreateFromLocal(
    int32_t size, uint32_t flags, const std::string &name)
{
    std::shared_ptr<AVDataSrcMemory> memory = std::make_shared<AVDataSrcMemory>(size, flags, name);
    int32_t ret = memory->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "Create avsharedmemory failed, ret = %{public}d", ret);

    return memory;
}

std::shared_ptr<AVSharedMemory> AVDataSrcMemory::CreateFromRemote(
    int32_t fd, int32_t size, uint32_t flags, const std::string &name)
{
    std::shared_ptr<AVDataSrcMemory> memory = std::make_shared<AVSharedMemoryBaseImpl>(fd, size, flags, name);
    int32_t ret = memory->Init();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Create avsharedmemory failed, ret = %{public}d", ret);
        return nullptr;
    }

    return memory;
}

AVDataSrcMemory::AVDataSrcMemory(int32_t size, uint32_t flags, const std::string &name)
    : AVSharedMemoryBase(size, flags, name)
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR ", name = %{public}s",
               FAKE_POINTER(this), name.c_str());
    offset_ = 0;
}

AVDataSrcMemory::AVDataSrcMemory(int32_t fd, int32_t size, uint32_t flags, const std::string &name)
    : AVSharedMemoryBase(fd, size, flags, name)
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR ", name = %{public}s",
               FAKE_POINTER(this), name.c_str());
    offset_ = 0;
}

AVDataSrcMemory::~AVDataSrcMemory()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR ", name = %{public}s",
               FAKE_POINTER(this), GetName().c_str());
}
} // namespace Media
} // namespace OHOS
