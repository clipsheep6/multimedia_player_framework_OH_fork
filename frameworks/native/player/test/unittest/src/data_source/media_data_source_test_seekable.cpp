/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "media_data_source_test_seekable.h"
#include <iostream>
#include "media_errors.h"
#include "directory_ex.h"
#include "media_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaDataSourceTestSeekable"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<MediaDataSourceTest> MediaDataSourceTestSeekable::Create(const std::string &uri, int32_t size)
{
    std::string realPath;
    if (!PathToRealPath(uri, realPath)) {
        std::cout << "Path is unaccessable: " << uri << std::endl;
        return nullptr;
    }
    std::shared_ptr<MediaDataSourceTestSeekable> dataSrc =
        std::make_shared<MediaDataSourceTestSeekable>(realPath, size);
    if (dataSrc->Init() != MSERR_OK) {
        std::cout << "init source failed" << std::endl;
        return nullptr;
    }
    return dataSrc;
}

MediaDataSourceTestSeekable::MediaDataSourceTestSeekable(const std::string &uri, int32_t size)
    : uri_(uri),
      fixedLen_(size)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaDataSourceTestSeekable::~MediaDataSourceTestSeekable()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    if (fd_ != nullptr) {
        (void)fclose(fd_);
        fd_ = nullptr;
    }
}

int32_t MediaDataSourceTestSeekable::Init()
{
    fd_ = fopen(uri_.c_str(), "rb+");
    if (fd_ == nullptr) {
        std::cout<<"open file failed"<<std::endl;
        return MSERR_INVALID_VAL;
    }
    return MSERR_OK;
}

void MediaDataSourceTestSeekable::Reset()
{
    std::cout<< "reset data source" << std::endl;
    position_ = 0;
    (void)fseek(fd_, 0, SEEK_SET);
}

int32_t MediaDataSourceTestSeekable::ReadAt(const std::shared_ptr<AVSharedMemory> &mem, uint32_t length, int64_t pos)
{
    MEDIA_LOGD("ReadAt in, pos is %{public}" PRIu64 "", pos);
    CHECK_AND_RETURN_RET_LOG(mem != nullptr, MSERR_INVALID_VAL, "Mem is nullptr");
    if (pos != position_) {
        (void)fseek(fd_, static_cast<long>(pos), SEEK_SET);
        position_ = pos;
    }
    size_t readRet = 0;
    int32_t realLength = static_cast<int32_t>(length);
    if (position_ >= fixedLen_) {
        MEDIA_LOGI("Is eos");
        return SOURCE_ERROR_EOF;
    }
    CHECK_AND_RETURN_RET_LOG(mem->GetBase() != nullptr, SOURCE_ERROR_IO, "Is null mem");
    readRet = fread(mem->GetBase(), static_cast<size_t>(length), 1, fd_);
    if (ferror(fd_)) {
        MEDIA_LOGI("Failed to call fread");
        return SOURCE_ERROR_IO;
    }
    if (readRet == 0) {
        realLength = static_cast<int32_t>(fixedLen_ - position_);
    }
    MEDIA_LOGD("length %{public}u realLength %{public}d", length, realLength);
    position_ += realLength;
    return realLength;
}

int32_t MediaDataSourceTestSeekable::ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
{
    (void)pos;
    (void)length;
    (void)mem;
    return MSERR_OK;
}

int32_t MediaDataSourceTestSeekable::ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
{
    (void)length;
    (void)mem;
    return MSERR_OK;
}

int32_t MediaDataSourceTestSeekable::GetSize(int64_t &size)
{
    size = fixedLen_;
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
