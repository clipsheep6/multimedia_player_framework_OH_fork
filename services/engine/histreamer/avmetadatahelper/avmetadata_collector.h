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

#ifndef AVMETA_DATA_COLLECTOR_H
#define AVMETA_DATA_COLLECTOR_H

#include <unordered_map>
#include <set>
#include <condition_variable>
#include <mutex>
#include <nocopyable.h>
#include "media_errors.h"
#include "common/log.h"
#include "meta/media_types.h"
#include "meta/meta.h"
#include "meta/meta_key.h"
#include "avmetadatahelper.h"
#include "ctime"

namespace OHOS {
namespace Media {
struct Metadata {
    Metadata() = default;
    ~Metadata() = default;

    void SetMeta(int32_t key, const std::string &value)
    {
        tbl_[key] = value;
    }

    bool TryGetMeta(int32_t key, std::string &value) const
    {
        auto it = tbl_.find(key);
        if (it == tbl_.end()) {
            return false;
        }
        value = it->second;
        return true;
    }

    bool HasMeta(int32_t key) const
    {
        return tbl_.count(key) != 0;
    }

    std::string GetMeta(int32_t key) const
    {
        if (tbl_.count(key) != 0) {
            return tbl_.at(key);
        }
        return "";
    }

    std::unordered_map<int32_t, std::string> tbl_;
};

class AVMetaDataCollector : public NoCopyable {
public:
    AVMetaDataCollector();
    ~AVMetaDataCollector();

    const int artPictureMaxSize = 1024 * 1024;

    std::unordered_map<int32_t, std::string> GetMetadata(const std::shared_ptr<Meta> &globalInfo,
        const std::vector<std::shared_ptr<Meta>> &trackInfos);
    std::shared_ptr<AVSharedMemory> GetArtPicture(const std::vector<std::shared_ptr<Meta>> &trackInfos);

private:
    void ConvertToAVMeta(const Meta &innerMeta, Metadata &avmeta) const;
    std::string convertTimestampToDatetime(const std::string &timestamp);
    void setEmptyStringToMeta
};
} // namespace Media
} // namespace OHOS
#endif // AVMETA_DATA_COLLECTOR_H