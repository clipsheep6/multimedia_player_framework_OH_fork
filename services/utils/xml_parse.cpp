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

#include "xml_parse.h"
#include "media_errors.h"
#include "media_log.h"
#include "string_ex.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "XmlParser"};
}

namespace OHOS {
namespace Media {
XmlParser::~XmlParser()
{
    Destroy();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool XmlParser::LoadConfiguration(const char *xmlPath)
{
    mDoc_ = xmlReadFile(xmlPath, nullptr, 0);
    CHECK_AND_RETURN_RET_LOG(mDoc_ != nullptr, false, "XmlParser xmlReadFile failed");
    return true;
}

bool XmlParser::Parse()
{
    xmlNode *root = xmlDocGetRootElement(mDoc_);
    CHECK_AND_RETURN_RET_LOG(root != nullptr, false, "XmlParser xmlDocGetRootElement failed");
    return ParseInternal(root);
}

void XmlParser::Destroy()
{
    if (mDoc_ != nullptr) {
        xmlFreeDoc(mDoc_);
    }
    return;
}

bool XmlParser::IsNumberArray(const std::vector<std::string> &strArray) const
{
    for (auto iter = strArray.begin(); iter != strArray.end(); iter++) {
        for (char const &c : *iter) {
            if (std::isdigit(c) == 0) {
                return false;
            }
        }
    }
    return true;
}

bool XmlParser::TransStrAsRange(const std::string &str, Range &range) const
{
    CHECK_AND_RETURN_RET_LOG(str != "null", false, "str is null");
    CHECK_AND_RETURN_RET_LOG(str != "", false, "str is empty");
    size_t pos = str.find("-");
    if (pos != str.npos && pos + 1 < str.size()) {
        std::string head = str.substr(0, pos);
        std::string tail = str.substr(pos + 1);
        bool ret = StrToInt(head, range.minVal);
        CHECK_AND_RETURN_RET_LOG(ret == true, false,
            "call StrToInt func false, input head is: %{public}s", head.c_str());

        ret = StrToInt(tail, range.maxVal);
        CHECK_AND_RETURN_RET_LOG(ret == true, false,
            "call StrToInt func false, input tail is: %{public}s", tail.c_str());
    } else {
        MEDIA_LOGD("Can not find the delimiter of \"-\" in : %{public}s", str.c_str());
        return false;
    }
    return true;
}

std::vector<int32_t> XmlParser::TransStrAsIntegerArray(const std::vector<std::string> &spilt) const
{
    std::vector<int32_t> array;
    for (auto iter = spilt.begin(); iter != spilt.end(); iter++) {
        int32_t num = -1;
        bool ret = StrToInt(*iter, num);
        CHECK_AND_RETURN_RET_LOG(ret == true, array,
            "call StrToInt func false, input str is: %{public}s", iter->c_str());
        array.push_back(num);
    }
    return array;
}

bool XmlParser::SpiltKeyList(const std::string &str, const std::string &delim,
    std::vector<std::string> &spilt) const
{
    CHECK_AND_RETURN_RET_LOG(str != "", false, "str is null");
    std::string strAddDelim = str;
    if (str.back() != delim.back()) {
        strAddDelim = str + delim;
    }
    size_t size = strAddDelim.size();
    for (size_t i = 0; i < size; ++i) {
        size_t pos = strAddDelim.find(delim, i);
        if (pos != strAddDelim.npos) {
            std::string s = strAddDelim.substr(i, pos - i);
            spilt.push_back(s);
            i = pos + delim.size() - 1;
        }
    }
    return true;
}

bool XmlParser::SetCapabilityStringData(std::unordered_map<std::string, std::string&> dataMap,
    const std::string &capabilityKey, const std::string &capabilityValue) const
{
    dataMap.at(capabilityKey) = capabilityValue;
    return true;
}

bool XmlParser::SetCapabilityRangeData(std::unordered_map<std::string, Range&> dataMap,
    const std::string &capabilityKey, const std::string &capabilityValue) const
{
    Range range;
    bool ret = TransStrAsRange(capabilityValue, range);
    CHECK_AND_RETURN_RET_LOG(ret != false, false, "failed:can not trans %{public}s", capabilityValue.c_str());
    dataMap.at(capabilityKey) = range;
    return true;
}
}  // namespace Media
}  // namespace OHOS