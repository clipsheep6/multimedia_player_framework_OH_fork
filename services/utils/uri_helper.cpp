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

#include "uri_helper.h"
#include <cstring>
#include <climits>
#include <sys/types.h>
#include <sys/stat.h>
#include <type_traits>
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "UriHelper"};
}

namespace OHOS {
namespace Media {
static const std::map<std::string_view, uint8_t> g_validUriTypes = {
    {"", UriHelper::UriType::URI_TYPE_FILE }, // empty uri head is treated as the file type uri.
    {"file", UriHelper::UriType::URI_TYPE_FILE},
    {"fd", UriHelper::UriType::URI_TYPE_FD},
    {"http", UriHelper::UriType::URI_TYPE_HTTP}
};

static bool PathToRealFileUrl(const std::string_view &path, std::string &realPath)
{
    CHECK_AND_RETURN_RET_LOG(!path.empty(), false, "path is empty!");
    CHECK_AND_RETURN_RET_LOG(path.length() < PATH_MAX, false,
        "path len is error, the len is: [%{public}zu]", path.length());

    char tmpPath[PATH_MAX] = {0};
    char *pathAddr = realpath(path.data(), tmpPath);
    CHECK_AND_RETURN_RET_LOG(pathAddr != nullptr, false,
        "path to realpath error, %{public}s", path.data());

    int ret = access(tmpPath, F_OK);
    CHECK_AND_RETURN_RET_LOG(ret == 0, false,
        "check realpath (%{private}s) error", tmpPath);

    realPath = std::string("file://") + tmpPath;
    return true;
}

template<typename T, typename = std::enable_if_t<std::is_same_v<int64_t, T> || std::is_same_v<int32_t, T>>>
bool StrToInt(const std::string_view& str, T& value)
{
    if (str.empty() || (!isdigit(str.front()) && (str.front() != '-'))) {
        return false;
    }

    std::string valStr(str);
    char* end = nullptr;
    errno = 0;
    const char* addr = valStr.c_str();
    long long result = strtoll(addr, &end, 10); /* 10 means decimal */
    CHECK_AND_RETURN_RET_LOG(result >= LLONG_MIN && result <= LLONG_MAX, false,
        "call StrToInt func false,  input str is: %{public}s!", valStr.c_str());
    CHECK_AND_RETURN_RET_LOG(end != addr && end[0] == '\0' && errno != ERANGE, false,
        "call StrToInt func false,  input str is: %{public}s!", valStr.c_str());

    if constexpr (std::is_same<int32_t, T>::value) {
        CHECK_AND_RETURN_RET_LOG(result >= INT_MIN && result <= INT_MAX, false,
            "call StrToInt func false,  input str is: %{public}s!", valStr.c_str());
        value = static_cast<int32_t>(result);
        return true;
    }

    value = result;
    return true;
}

__attribute__((no_sanitize("cfi")))
std::pair<std::string_view, std::string_view> SplitUriHeadAndBody(const std::string_view &str)
{
    std::string_view::size_type start = str.find_first_not_of(' ');
    std::string_view::size_type end = str.find_last_not_of(' ');
    std::pair<std::string_view, std::string_view> result;
    std::string_view noSpaceStr;

    if (start == std::string_view::npos && end == std::string_view::npos) {
        result.first = "";
        result.second = "";
        return result;
    }

    if (end == std::string_view::npos) {
        noSpaceStr = str.substr(start);
    } else {
        noSpaceStr = str.substr(start, end - start + 1);
    }
    std::string_view delimiter = "://";
    std::string_view::size_type pos = noSpaceStr.find(delimiter);
    if (pos == std::string_view::npos) {
        result.first = "";
        result.second = noSpaceStr;
    } else {
        result.first = noSpaceStr.substr(0, pos);
        result.second = noSpaceStr.substr(pos + delimiter.size());
    }
    return result;
}

UriHelper::UriHelper(const std::string_view &uri)
{
    FormatMeForUri(uri);
}

UriHelper::UriHelper(int32_t fd, int64_t offset, int64_t size) : fd_(fd), offset_(offset), size_(size)
{
    FormatMeForFd();
}

UriHelper::~UriHelper()
{
    if (fd_ > 0) {
        (void)::close(fd_);
    }
}

void UriHelper::FormatMeForUri(const std::string_view &uri) noexcept
{
    CHECK_AND_RETURN_LOG(formattedUri_.empty(),
        "formattedUri is valid:%{public}s", formattedUri_.c_str());
    CHECK_AND_RETURN_LOG(!uri.empty(), "uri is empty");

    auto [head, body] = SplitUriHeadAndBody(uri);
    CHECK_AND_RETURN(g_validUriTypes.count(head) != 0);
    type_ = g_validUriTypes.at(head);

    // verify whether the uri is readable and generate the formatted uri.
    switch (type_) {
        case URI_TYPE_FILE: {
            if (!PathToRealFileUrl(body, formattedUri_)) {
                type_ = URI_TYPE_UNKNOWN;
                formattedUri_ = body;
            }
            rawFileUri_ = formattedUri_;
            if (rawFileUri_.size() > strlen("file://")) {
                rawFileUri_ = rawFileUri_.substr(strlen("file://"));
            }
            break;
        }
        case URI_TYPE_FD: {
            if (!ParseFdUri(body)) {
                type_ = URI_TYPE_UNKNOWN;
                formattedUri_ = "";
            }
            break;
        }
        default:
            formattedUri_ = std::string(head);
            formattedUri_ += body;
            break;
    }

    MEDIA_LOGD("0x%{public}06" PRIXPTR " formatted uri: %{private}s", FAKE_POINTER(this), formattedUri_.c_str());
}

void UriHelper::FormatMeForFd() noexcept
{
    CHECK_AND_RETURN_LOG(formattedUri_.empty(),
        "formattedUri is valid:%{public}s", formattedUri_.c_str());
    type_ = URI_TYPE_FD;
    (void)CorrectFdParam();
}

bool UriHelper::CorrectFdParam()
{
    int flags = fcntl(fd_, F_GETFL);
    CHECK_AND_RETURN_RET_LOG(flags != -1, false, "Fail to get File Status Flags");

    struct stat64 st;
    CHECK_AND_RETURN_RET_LOG(fstat64(fd_, &st) == 0, false,
        "can not get file state");

    int64_t fdSize = static_cast<int64_t>(st.st_size);
    int64_t stIno = static_cast<int64_t>(st.st_ino);
    int64_t stSec = static_cast<int64_t>(st.st_atim.tv_sec);
    MEDIA_LOGI("CorrectFdParam fd: %{public}d, fdSize: %{public}" PRId64 ", stIno: %{public}" PRId64
        ", stSec: %{public}" PRId64, fd_, fdSize, stIno, stSec);
    if (offset_ < 0 || offset_ > fdSize) {
        offset_ = 0;
    }

    if ((size_ <= 0) || (size_ > fdSize - offset_)) {
        size_ = fdSize - offset_;
    }

    fd_ = ::dup(fd_);
    formattedUri_ = std::string("fd://") + std::to_string(fd_) + "?offset=" +
        std::to_string(offset_) + "&size=" + std::to_string(size_);
    MEDIA_LOGI("CorrectFdParam formattedUri: %{public}s", formattedUri_.c_str());
    return true;
}

uint8_t UriHelper::UriType() const
{
    return type_;
}

std::string UriHelper::FormattedUri() const
{
    return formattedUri_;
}

bool UriHelper::AccessCheck(uint8_t flag) const
{
    CHECK_AND_RETURN_RET_LOG(type_ != URI_TYPE_UNKNOWN, false, "type is unknown");

    if (type_ == URI_TYPE_FILE) {
        uint32_t mode = (flag & URI_READ) ? R_OK : 0;
        mode |= (flag & URI_WRITE) ? W_OK : 0;
        int ret = access(rawFileUri_.data(), static_cast<int>(mode));
        CHECK_AND_RETURN_RET_LOG(ret == 0, false,
            "Fail to access path: %{public}s", rawFileUri_.data());
        return true;
    } else if (type_ == URI_TYPE_FD) {
        CHECK_AND_RETURN_RET_LOG(fd_ > 0, false, "Fail to get file descriptor from uri, fd %{public}d", fd_);

        int flags = fcntl(fd_, F_GETFL);
        CHECK_AND_RETURN_RET_LOG(flags != -1, false, "Fail to get File Status Flags, fd %{public}d", fd_);

        uint32_t mode = (flag & URI_WRITE) ? O_RDWR : O_RDONLY;
        return ((static_cast<unsigned int>(flags) & mode) != mode) ? false : true;
    }

    return true; // Not implemented, defaultly return true.
}

bool UriHelper::ParseFdUri(std::string_view uri)
{
    static constexpr std::string_view::size_type delim1Len = std::string_view("?offset=").size();
    static constexpr std::string_view::size_type delim2Len = std::string_view("&size=").size();
    std::string_view::size_type delim1 = uri.find("?");
    std::string_view::size_type delim2 = uri.find("&");
    if (delim1 == std::string_view::npos && delim2 == std::string_view::npos) {
        CHECK_AND_RETURN_RET_LOG(StrToInt(uri, fd_), false, "Invalid fd url");
    } else if (delim1 != std::string_view::npos && delim2 != std::string_view::npos) {
        std::string_view fdstr = uri.substr(0, delim1);
        int32_t fd = -1;
        CHECK_AND_RETURN_RET_LOG(StrToInt(fdstr, fd) && delim1 + delim1Len < uri.size()
            && delim2 - delim1 - delim1Len > 0, false, "Invalid fd url");
        std::string_view offsetStr = uri.substr(delim1 + delim1Len, delim2 - delim1 - delim1Len);
        CHECK_AND_RETURN_RET_LOG(StrToInt(offsetStr, offset_) && delim2 + delim2Len < uri.size(), false,
            "Invalid fd url");
        std::string_view sizeStr = uri.substr(delim2 + delim2Len);
        CHECK_AND_RETURN_RET_LOG(StrToInt(sizeStr, size_), false, "Invalid fd url");
        fd_ = fd;
    } else {
        MEDIA_LOGE("invalid fd uri: %{public}s", uri.data());
        return false;
    }

    MEDIA_LOGD("parse fd uri, fd: %{public}d, offset: %{public}" PRIi64 ", size: %{public}" PRIi64,
               fd_, offset_, size_);

    return CorrectFdParam();
}
} // namespace Media
} // namespace OHOS
