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

#include "avformat_native_mock.h"
#include "securec.h"

namespace OHOS {
namespace Media {
bool AVFormatNativeMock::PutIntValue(const std::string_view &key, int32_t value)
{
    return format_.PutIntValue(key, value);
}

bool AVFormatNativeMock::GetIntValue(const std::string_view &key, int32_t &value)
{
    return format_.GetIntValue(key, value);
}

bool AVFormatNativeMock::PutStringValue(const std::string_view &key, const std::string_view &value)
{
    return format_.PutStringValue(key, value);
}

bool AVFormatNativeMock::GetStringValue(const std::string_view &key, std::string &value)
{
    return format_.GetStringValue(key, value);
}

void AVFormatNativeMock::Destroy()
{
    if (dumpInfo_ != nullptr) {
        free(dumpInfo_);
        dumpInfo_ = nullptr;
    }
    return;
}

Format &AVFormatNativeMock::GetFormat()
{
    return format_;
}

bool AVFormatNativeMock::PutLongValue(const std::string_view &key, int64_t value)
{
    return format_.PutLongValue(key, value);
}

bool AVFormatNativeMock::GetLongValue(const std::string_view &key, int64_t &value)
{
    return format_.GetLongValue(key, value);
}

bool AVFormatNativeMock::PutFloatValue(const std::string_view &key, float value)
{
    return format_.PutFloatValue(key, value);
}

bool AVFormatNativeMock::GetFloatValue(const std::string_view &key, float &value)
{
    return format_.GetFloatValue(key, value);
}

bool AVFormatNativeMock::PutDoubleValue(const std::string_view &key, double value)
{
    return format_.PutDoubleValue(key, value);
}

bool AVFormatNativeMock::GetDoubleValue(const std::string_view &key, double &value)
{
    return format_.GetDoubleValue(key, value);
}

bool AVFormatNativeMock::GetBuffer(const std::string_view &key, uint8_t **addr, size_t &size)
{
    return format_.GetBuffer(key, addr, size);
}

bool AVFormatNativeMock::PutBuffer(const std::string_view &key, const uint8_t *addr, size_t size)
{
    return format_.PutBuffer(key, addr, size);
}

const char *AVFormatNativeMock::DumpInfo()
{
    if (dumpInfo_ != nullptr) {
        free(dumpInfo_);
        dumpInfo_ = nullptr;
    }
    std::string info = format_.Stringify();
    if (info.empty()) {
        return nullptr;
    }
    constexpr uint32_t maxDumpLength = 1024;
    uint32_t bufLength = info.size() > maxDumpLength ? maxDumpLength : info.size();
    dumpInfo_ = static_cast<char *>(malloc((bufLength + 1) * sizeof(char)));
    if (dumpInfo_ == nullptr) {
        return nullptr;
    }
    if (strcpy_s(dumpInfo_, bufLength + 1, info.c_str()) != 0) {
        free(dumpInfo_);
        dumpInfo_ = nullptr;
    }
    return dumpInfo_;
}
} // Media
} // OHOS