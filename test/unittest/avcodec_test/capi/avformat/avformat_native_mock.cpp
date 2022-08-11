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

#include "avformat_native_mock.h"

namespace OHOS {
namespace Media {
AVFormatNativeMock::~AVFormatNativeMock()
{
    if (format_ != nullptr) {
        OH_AVFormat_Destroy(format_);
    }
}

bool AVFormatNativeMock::PutIntValue(const std::string_view &key, int32_t value)
{
    if (format_ != nullptr) {
        return OH_AVFormat_SetIntValue(format_, key.c_str(), value);
    }
    return false;
}

bool AVFormatNativeMock::GetIntValue(const std::string_view &key, int32_t &value)
{
    if (format_ != nullptr) {
        return OH_AVFormat_GetIntValue(format_, key.c_str(), &value);
    }
    return false;
}

bool AVFormatNativeMock::PutStringValue(const std::string_view &key, const std::string_view &value)
{
    if (format_ != nullptr) {
        return OH_AVFormat_SetStringValue(format_, key.c_str(), value.c_str());
    }
    return false;
}

bool AVFormatNativeMock::GetStringValue(const std::string_view &key, std::string &value)
{
    if (format_ != nullptr) {
        char *out = nullptr;
        OH_AVFormat_GetStringValue(format_, key.c_str(), &out);
        value = out;
        return true;
    }
    return false;
}

AVFormat *AVFormatNativeMock::GetFormat()
{
    return format_;
}
} // Media
} // OHOS