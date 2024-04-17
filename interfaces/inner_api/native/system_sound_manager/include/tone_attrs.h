/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef TONE_ATTRS_H
#define TONE_ATTRS_H

#include <string>

#include "audio_info.h"


namespace OHOS {
namespace Media {

enum ToneCustomizedType {
    PRE_INSTALLED = 0,
    CUSTOMISED = 1,
};
constexpr int32_t TONE_CATEGORY_RINGTONE = 1;
constexpr int32_t TONE_CATEGORY_TEXT_MESSAGE = 2;
constexpr int32_t TONE_CATEGORY_NOTIFICATION = 4;
constexpr int32_t TONE_CATEGORY_ALARM = 8;

class ToneAttrs {
public:
    ToneAttrs(std::string title, std::string fileName,
        std::string url, ToneCustomizedType custType, int32_t category) : title_(title), fileName_(fileName), uri_(uri), custType_(custType) ,category_(category) {}
    virtual ~ToneAttrs() = default;
    /**
     * @brief Returns the title of the tone attrs.
     *
     * @return Returns title as string if the title is obtained successfully.
     * returns an empty string otherwise.
     * @since 12
     */
    virtual std::string GetTitle() const
    {
        return title_;
    }

    void SetTitle(const std::string title)
    {
        title_ = title;
    }

    virtual std::string GetFileName() const
    {
        return fileName_;
    }

    void SetFileName(const std::string fileName)
    {
        fileName_ = fileName;
    }

    virtual std::string GetUri() const
    {
        return uri_;
    }

    virtual ToneCustomizedType GetCustomizedType() const
    {
        return custType_;
    }

    void SetCategory(const int32_t category)
    {
        category_ = category;
    }

    virtual int32_t GetCategory() const
    {
        return category_;
    }

private:
    std::string title_ = "";
    std::string fileName_ = "";
    std::string uri_ = "";
    ToneCustomizedType custType_ = PRE_INSTALLED;
    int32_t category_ = 0;
};
} // namespace Media
} // namespace OHOS
#endif // TONE_ATTRS_H
