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

#include "media_data_source_test_noseek.h"
#include <iostream>
#include "media_errors.h"
#include "directory_ex.h"
#include "media_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaDataSourceTestNoSeek"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<MediaDataSourceTest> MediaDataSourceTestNoSeek::Create(const std::string &uri, int32_t size)
{
    std::string realPath;
    if (!PathToRealPath(uri, realPath)) {
        std::cout << "Path is unaccessable: " << uri << std::endl;
        return nullptr;
    }
    std::shared_ptr<MediaDataSourceTestNoSeek> dataSrc = std::make_shared<MediaDataSourceTestNoSeek>(realPath, size);
    if (dataSrc->Init() != MSERR_OK) {
        std::cout << "init source failed" << std::endl;
        return nullptr;
    }
    return dataSrc;
}

MediaDataSourceTestNoSeek::MediaDataSourceTestNoSeek(const std::string &uri, int32_t size)
    : uri_(uri),
      fixedSize_(size)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaDataSourceTestNoSeek::~MediaDataSourceTestNoSeek()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    if (fd_ != nullptr) {
        (void)fclose(fd_);
        fd_ = nullptr;
    }
}

int32_t MediaDataSourceTestNoSeek::Init()
{
    fd_ = fopen(uri_.c_str(), "rb+");
    if (fd_ == nullptr) {
        std::cout<<"open file failed"<<std::endl;
        return MSERR_INVALID_VAL;
    }
    return MSERR_OK;
}

void MediaDataSourceTestNoSeek::Reset()
{
    std::cout<< "reset data source" << std::endl;
    pos_ = 0;
    (void)fseek(fd_, 0, SEEK_SET);
}

int32_t MediaDataSourceTestNoSeek::ReadAt(const std::shared_ptr<AVSharedMemory> &mem, uint32_t length, int64_t pos)
{
    MEDIA_LOGD("ReadAt in");
    (void)pos;
    CHECK_AND_RETURN_RET_LOG(mem != nullptr, MSERR_INVALID_VAL, "Mem is nullptr");
    size_t readRet = 0;
    int32_t realLen = static_cast<int32_t>(length);
    if (mem->GetBase() == nullptr) {
        MEDIA_LOGI("Is null mem");
        return SOURCE_ERROR_IO;
    }
    readRet = fread(mem->GetBase(), static_cast<size_t>(length), 1, fd_);
    if (ferror(fd_)) {
        MEDIA_LOGI("Failed to call fread");
        return SOURCE_ERROR_IO;
    }
    if (readRet == 0) {
        realLen = static_cast<int32_t>(fixedSize_ - pos_);
    }
    MEDIA_LOGD("length %{public}u realLen %{public}d", length, realLen);
    if (realLen == 0) {
        return SOURCE_ERROR_EOF;
    }
    pos_ += realLen;
    return realLen;
}

int32_t MediaDataSourceTestNoSeek::ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
{
    (void)pos;
    (void)length;
    (void)mem;
    return MSERR_OK;
}

int32_t MediaDataSourceTestNoSeek::ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
{
    (void)length;
    (void)mem;
    return MSERR_OK;
}

int32_t MediaDataSourceTestNoSeek::GetSize(int64_t &size)
{
    size = -1;
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS