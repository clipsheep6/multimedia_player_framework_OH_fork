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

#include "format.h"
#include "securec.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "Format"};
}

namespace OHOS {
namespace Media {
void FreeAddr(Format::FormatDataMap &formatMap)
{
    for (auto it = formatMap.begin(); it != formatMap.end(); ++it) {
        if (it->second.type == FORMAT_TYPE_ADDR && it->second.addr != nullptr) {
            free(it->second.addr);
            it->second.addr = nullptr;
        }
    }
}

void CopyFormatDataMap(const Format::FormatDataMap &from, Format::FormatDataMap &to)
{
    FreeAddr(to);
    to = from;

    for (auto it = to.begin(); it != to.end(); ++it) {
        if (it->second.type == FORMAT_TYPE_ADDR) {
            it->second.addr = nullptr;
            it->second.addr = reinterpret_cast<uint8_t *>(malloc(it->second.size));
            CHECK_AND_RETURN_LOG(it->second.addr != nullptr,
                "malloc addr failed. Key: %{public}s", it->first.c_str());
            errno_t err = memcpy_s(reinterpret_cast<void *>(it->second.addr),
                it->second.size, reinterpret_cast<const void *>(from.at(it->first).addr), it->second.size);
            CHECK_AND_RETURN_LOG(err == EOK,
                "memcpy addr failed. Key: %{public}s", it->first.c_str());
        }
    }
}

Format::~Format()
{
    FreeAddr(formatMap_);
}

Format::Format(const Format &rhs)
{
    CHECK_AND_RETURN_LOG(&rhs != this, "Copying oneself is not supported");
    CopyFormatDataMap(rhs.formatMap_, formatMap_);
}

Format::Format(Format &&rhs) noexcept
{
    std::swap(formatMap_, rhs.formatMap_);
}

Format &Format::operator=(const Format &rhs)
{
    CHECK_AND_RETURN_RET_LOG(&rhs != this, *this, "Copying oneself is not supported");
    CopyFormatDataMap(rhs.formatMap_, this->formatMap_);
    return *this;
}

Format &Format::operator=(Format &&rhs) noexcept
{
    CHECK_AND_RETURN_RET_LOG(&rhs != this, *this, "Copying oneself is not supported");
    std::swap(this->formatMap_, rhs.formatMap_);
    return *this;
}

bool Format::PutIntValue(const std::string_view &key, int32_t value)
{
    FormatData data;
    data.type = FORMAT_TYPE_INT32;
    data.val.int32Val = value;
    RemoveKey(key);
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::PutLongValue(const std::string_view &key, int64_t value)
{
    FormatData data;
    data.type = FORMAT_TYPE_INT64;
    data.val.int64Val = value;
    RemoveKey(key);
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::PutFloatValue(const std::string_view &key, float value)
{
    FormatData data;
    data.type = FORMAT_TYPE_FLOAT;
    data.val.floatVal = value;
    RemoveKey(key);
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::PutDoubleValue(const std::string_view &key, double value)
{
    FormatData data;
    data.type = FORMAT_TYPE_DOUBLE;
    data.val.doubleVal = value;
    RemoveKey(key);
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::PutStringValue(const std::string_view &key, const std::string_view &value)
{
    FormatData data;
    data.type = FORMAT_TYPE_STRING;
    data.stringVal = value;
    RemoveKey(key);
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::GetStringValue(const std::string_view &key, std::string &value) const
{
    auto iter = formatMap_.find(key);
    CHECK_AND_RETURN_RET_LOG(iter != formatMap_.end(), false,
        "Format::GetFormat failed. Key: %{public}s", key.data());
    CHECK_AND_RETURN_RET_LOG(iter->second.type == FORMAT_TYPE_STRING, false,
        "Format::GetFormat failed. Key: %{public}s", key.data());
    value = iter->second.stringVal;
    return true;
}

bool Format::GetIntValue(const std::string_view &key, int32_t &value) const
{
    auto iter = formatMap_.find(key);
    CHECK_AND_RETURN_RET_LOG(iter != formatMap_.end(), false,
        "Format::GetFormat failed. Key: %{public}s", key.data());
    CHECK_AND_RETURN_RET_LOG(iter->second.type == FORMAT_TYPE_INT32, false,
        "Format::GetFormat failed. Key: %{public}s", key.data());
    value = iter->second.val.int32Val;
    return true;
}

bool Format::GetLongValue(const std::string_view &key, int64_t &value) const
{
    auto iter = formatMap_.find(key);
    CHECK_AND_RETURN_RET_LOG(iter != formatMap_.end(), false,
        "Format::GetFormat failed. Key: %{public}s", key.data());
    CHECK_AND_RETURN_RET_LOG(iter->second.type == FORMAT_TYPE_INT64, false,
        "Format::GetFormat failed. Key: %{public}s", key.data());
    value = iter->second.val.int64Val;
    return true;
}

bool Format::GetFloatValue(const std::string_view &key, float &value) const
{
    auto iter = formatMap_.find(key);
    CHECK_AND_RETURN_RET_LOG(iter != formatMap_.end(), false,
        "Format::GetFormat failed. Key: %{public}s", key.data());
    CHECK_AND_RETURN_RET_LOG(iter->second.type == FORMAT_TYPE_FLOAT, false,
        "Format::GetFormat failed. Key: %{public}s", key.data());
    value = iter->second.val.floatVal;
    return true;
}

bool Format::GetDoubleValue(const std::string_view &key, double &value) const
{
    auto iter = formatMap_.find(key);
    CHECK_AND_RETURN_RET_LOG(iter != formatMap_.end(), false,
        "Format::GetFormat failed. Key: %{public}s", key.data());
    CHECK_AND_RETURN_RET_LOG(iter->second.type == FORMAT_TYPE_DOUBLE, false,
        "Format::GetFormat failed. Key: %{public}s", key.data());
    value = iter->second.val.doubleVal;
    return true;
}

bool Format::PutBuffer(const std::string_view &key, const uint8_t *addr, size_t size)
{
    CHECK_AND_RETURN_RET_LOG(addr != nullptr, false, "put buffer error, addr is nullptr");

    constexpr size_t sizeMax = 1 * 1024 * 1024;
    CHECK_AND_RETURN_RET_LOG(size <= sizeMax, false,
        "PutBuffer input size too large. Key: %{public}s", key.data());

    FormatData data;
    data.type = FORMAT_TYPE_ADDR;
    data.addr = reinterpret_cast<uint8_t *>(malloc(size));
    CHECK_AND_RETURN_RET_LOG(addr != nullptr, false,
        "malloc addr failed. Key: %{public}s", key.data());

    errno_t err = memcpy_s(reinterpret_cast<void *>(data.addr), size, reinterpret_cast<const void *>(addr), size);
    if (err != EOK) {
        MEDIA_LOGE("PutBuffer memcpy addr failed. Key: %{public}s", key.data());
        free(data.addr);
        return false;
    }

    RemoveKey(key);

    data.size = size;
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::GetBuffer(const std::string_view &key, uint8_t **addr, size_t &size) const
{
    auto iter = formatMap_.find(key);
    CHECK_AND_RETURN_RET_LOG(iter != formatMap_.end(), false,
        "Format::GetFormat failed. Key: %{public}s", key.data());
    CHECK_AND_RETURN_RET_LOG(iter->second.type == FORMAT_TYPE_ADDR, false,
        "Format::GetFormat failed. Key: %{public}s", key.data());
    *addr = iter->second.addr;
    size = iter->second.size;
    return true;
}

bool Format::ContainKey(const std::string_view &key) const
{
    auto iter = formatMap_.find(key);
    if (iter != formatMap_.end()) {
        return true;
    }

    return false;
}

FormatDataType Format::GetValueType(const std::string_view &key) const
{
    auto iter = formatMap_.find(key);
    if (iter == formatMap_.end()) {
        return FORMAT_TYPE_NONE;
    }

    return iter->second.type;
}

void Format::RemoveKey(const std::string_view &key)
{
    auto iter = formatMap_.find(key);
    if (iter != formatMap_.end()) {
        if (iter->second.type == FORMAT_TYPE_ADDR && iter->second.addr != nullptr) {
            free(iter->second.addr);
            iter->second.addr = nullptr;
        }
        formatMap_.erase(iter);
    }
}

const Format::FormatDataMap &Format::GetFormatMap() const
{
    return formatMap_;
}

std::string Format::Stringify() const
{
    std::string outString;
    for (auto iter = formatMap_.begin(); iter != formatMap_.end(); iter++) {
        switch (GetValueType(iter->first)) {
            case FORMAT_TYPE_INT32:
                outString += iter->first + " = " + std::to_string(iter->second.val.int32Val) + " | ";
                break;
            case FORMAT_TYPE_INT64:
                outString += iter->first + " = " + std::to_string(iter->second.val.int64Val) + " | ";
                break;
            case FORMAT_TYPE_FLOAT:
                outString += iter->first + " = " + std::to_string(iter->second.val.floatVal) + " | ";
                break;
            case FORMAT_TYPE_DOUBLE:
                outString += iter->first + " = " + std::to_string(iter->second.val.doubleVal) + " | ";
                break;
            case FORMAT_TYPE_STRING:
                outString += iter->first + " = " + iter->second.stringVal + " | ";
                break;
            case FORMAT_TYPE_ADDR:
                break;
            default:
                MEDIA_LOGE("Format::Stringify failed. Key: %{public}s", iter->first.c_str());
        }
    }
    return outString;
}
} // namespace Media
} // namespace OHOS