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

#ifndef TIME_FORMAT_UTILS
#define TIME_FORMAT_UTILS

#include <string>

namespace OHOS {
namespace Media {
class TimeFormatUtils {
public:
    TimeFormatUtils();
    ~TimeFormatUtils();

    static constexpr int maxDateTimeSize = 20;
    static constexpr size_t standardDateStrSize = 10;
    static constexpr size_t standardTimeStrSize = 8;

    static std::string __attribute__((visibility("default"))) FormatDateTimeByTimeZone(const std::string &iso8601Str);
    static std::string __attribute__((visibility("default"))) FormatDataTimeByString(const std::string &dataTime);
    static std::string __attribute__((visibility("default"))) ConvertTimestampToDatetime(const std::string &timestamp);
};
}  // namespace Media
}  // namespace OHOS
#endif  // TIME_FORMAT_UTILS