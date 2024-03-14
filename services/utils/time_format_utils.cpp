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

#include "time_format_utils.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>

#include "media_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "TimeFormatUtils"};
}

namespace OHOS {
namespace Media {
std::string TimeFormatUtils::FormatDateTimeByTimeZone(const std::string &iso8601Str)
{
    std::regex pattern(R"((\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2})(.\d{1,6})?((\+|-\d{4})?)Z?)");
    std::smatch match;
    if (!std::regex_match(iso8601Str, match, pattern)) {
        return iso8601Str;  // not standard ISO8601 type string
    }

    std::istringstream iss(iso8601Str);
    std::tm tm;
    if (!(iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S"))) {
        return iso8601Str;  // cant prase time
    }

    // time zone
    time_t tt = mktime(&tm);
    if (tt == -1) {
        return iso8601Str;
    }
    uint32_t length = iso8601Str.length();
    long diffTime = 0;
    if (iso8601Str.substr(length - 1, length).compare("Z") != 0) {
        int mins = std::stoi(iso8601Str.substr(length - 2, 2));
        int hours = std::stoi(iso8601Str.substr(length - 4, 2));
        char symbol = iso8601Str.at(length - 5);
        long seconds = (hours * 60 + mins) * 60;
        diffTime = symbol == '+' ? seconds : -seconds;
    }

    // convert time to localtime
    long timezone = 0;
    std::tm timeWithOffset = *localtime(&tt);
    if (timeWithOffset.tm_gmtoff != 0) {
        timezone = timeWithOffset.tm_gmtoff;
    }
    auto localTime =
        std::chrono::system_clock::from_time_t(std::mktime(&tm)) + std::chrono::seconds(timezone - diffTime);
    std::time_t localTimeT = std::chrono::system_clock::to_time_t(localTime);
    std::tm localTm = *std::localtime(&localTimeT);
    std::ostringstream oss;
    oss << std::put_time(&localTm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string TimeFormatUtils::FormatDataTimeByString(const std::string &dataTime)
{
    if (dataTime.compare("") == 0) {
        return dataTime;
    }
    std::string::size_type position = dataTime.find(" ");
    std::string data = "";
    std::string time = "";
    if (position == dataTime.npos) {
        data = dataTime;
        if (data.find("-") == data.npos) {
            data += "-01-01";
        } else if (data.find_first_of("-") == data.find_last_of("-")) {
            data += "-01";
        }
        time += " 00:00:00";
    } else {
        data = dataTime.substr(0, position);
        time = dataTime.substr(position);
        if (data.find("-") == data.npos) {
            data += "-01-01";
        } else if (data.find_first_of("-") == data.find_last_of("-")) {
            data += "-01";
        }
        if (time.find(":") == data.npos) {
            time += ":00:00";
        } else if (time.find_first_of(":") == time.find_last_of(":")) {
            time += ":00";
        } else {
            time = time.substr(0, time.find("."));
        }
    }
    MEDIA_LOGD("FormatDataTimeByString is: %{public}s%{public}s", data.c_str(), time.c_str());
    return data + time;
}

std::string TimeFormatUtils::ConvertTimestampToDatetime(const std::string &timestamp)
{
    if (timestamp.empty()) {
        MEDIA_LOGE("datetime is empty, format failed");
        return "";
    }

    time_t ts = stoi(timestamp);
    tm *pTime;
    char date[maxDateTimeSize];
    char time[maxDateTimeSize];
    pTime = localtime(&ts);
    size_t sizeDateStr = strftime(date, maxDateTimeSize, "%Y-%m-%d", pTime);
    size_t sizeTimeStr = strftime(time, maxDateTimeSize, "%H:%M:%S", pTime);
    if (sizeDateStr != standardDateStrSize || sizeTimeStr != standardTimeStrSize) {
        MEDIA_LOGE("datetime is invalid, format failed");
        return "";
    }

    return std::string(date) + " " + std::string(time);
}
}  // namespace Media
}  // namespace OHOS