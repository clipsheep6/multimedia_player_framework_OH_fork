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

#include "avmetadata_collector.h"
#include <string>
#include "meta/video_types.h"
#include "buffer/avsharedmemorybase.h"

namespace OHOS {
namespace Media {
static const std::unordered_map<int32_t, std::string> AVMETA_KEY_TO_X_MAP = {
    { AV_KEY_ALBUM, Tag::MEDIA_ALBUM },
    { AV_KEY_ALBUM_ARTIST, Tag::MEDIA_ALBUM_ARTIST },
    { AV_KEY_ARTIST, Tag::MEDIA_ARTIST },
    { AV_KEY_AUTHOR, Tag::MEDIA_AUTHOR },
    { AV_KEY_COMPOSER, Tag::MEDIA_COMPOSER },
    { AV_KEY_DATE_TIME, Tag::MEDIA_DATE },
    { AV_KEY_DATE_TIME_FORMAT, "" },
    { AV_KEY_DURATION, Tag::MEDIA_DURATION },
    { AV_KEY_GENRE, Tag::MEDIA_GENRE },
    { AV_KEY_HAS_AUDIO, Tag::MEDIA_HAS_AUDIO },
    { AV_KEY_HAS_VIDEO, Tag::MEDIA_HAS_VIDEO },
    { AV_KEY_MIME_TYPE, Tag::MIME_TYPE },
    { AV_KEY_NUM_TRACKS, Tag::MEDIA_TRACK_COUNT },
    { AV_KEY_SAMPLE_RATE, Tag::AUDIO_SAMPLE_RATE },
    { AV_KEY_TITLE, Tag::MEDIA_TITLE },
    { AV_KEY_VIDEO_HEIGHT, Tag::VIDEO_HEIGHT },
    { AV_KEY_VIDEO_WIDTH, Tag::VIDEO_WIDTH },
    { AV_KEY_VIDEO_ORIENTATION, Tag::VIDEO_ROTATION },
};

AVMetaDataCollector::AVMetaDataCollector()
{
    MEDIA_LOG_D("enter ctor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

AVMetaDataCollector::~AVMetaDataCollector()
{
    MEDIA_LOG_D("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

std::unordered_map<int32_t, std::string> AVMetaDataCollector::GetMetadata(const std::shared_ptr<Meta> &globalInfo,
    const std::vector<std::shared_ptr<Meta>> &trackInfos)
{
    FALSE_RETURN_V_MSG_E(globalInfo != nullptr && trackInfos.size() != 0, {}, "globalInfo or trackInfos are invalid.");
    std::vector<TagType> keys;
    globalInfo->GetKeys(keys);
    for (int32_t i = 0; i < keys.size(); i++) {
        std::string strVal;
        int32_t intVal;
        if (!globalInfo->GetData(keys[i], strVal)) {
            globalInfo->GetData(keys[i], intVal);
        }
    }

    Metadata metadata;
    ConvertToAVMeta(globalInfo, metadata);

    size_t trackCount = trackInfos.size();
    for (size_t index = 0; index < trackCount; index++) {
        std::shared_ptr<Meta> meta = trackInfos[index];
        if (meta == nullptr) {
            MEDIA_LOG_E("meta is invalid, index: %zu", index);
            return metadata.tbl_;
        }

        Plugin::MediaType mediaType;
        if (!meta->GetData(Tag::MEDIA_TYPE, mediaType)) {
            MEDIA_LOG_E("mediaType not found, index: %zu", index);
            return metadata.tbl_;
        }

        ConvertToAVMeta(meta, metadata);
    }
    auto it = metadata.tbl_.begin();
    while (it != metadata.tbl_.end()) {
        MEDIA_LOG_I("metadata tbl, key: %{public}d, keyName: %{public}s, val: %{public}s", it->first,
            AVMETA_KEY_TO_X_MAP.find(it->first)->second.c_str(), it->second.c_str());
        it++;
    }
    return metadata.tbl_;
}

std::shared_ptr<AVSharedMemory> AVMetaDataCollector::GetArtPicture(const std::vector<std::shared_ptr<Meta>> &trackInfos)
{
    MEDIA_LOG_I("GetArtPicture In");
    size_t trackCount = trackInfos.size();
    for (size_t index = 0; index < trackCount; index++) {
        std::shared_ptr<Meta> meta = trackInfos[index];
        if (meta == nullptr) {
            MEDIA_LOG_W("meta is invalid, index: %zu", index);
            continue;
        }

        std::vector<uint8_t> coverAddr;
        auto mapIt = meta.Find(Tag::MEDIA_COVER);
        if (mapIt == meta.end()) {
            continue;
        }
        coverAddr = mapIt->second;
        if (coverAddr.size() == 0 || coverAddr.size() > artPictureMaxSize) {
            MEDIA_LOG_E("InvalidArtPictureSize %d", coverAddr.size());
            return nullptr;
        }
        uint8_t *addr = coverAddr.data();
        size_t size = coverAddr.size();z
        auto artPicMem = AVSharedMemoryBase::CreateFromLocal(
            static_cast<int32_t>(size), AVSharedMemory::FLAGS_READ_ONLY, "artpic");
        errno_t rc = memcpy_s(artPicMem->GetBase(), static_cast<size_t>(artPicMem->GetSize()), addr, size);
        if (rc != EOK) {
            MEDIA_LOG_E("memcpy_s failed, trackCount no %{public}d", index);
            return nullptr;
        }
        
        MEDIA_LOG_I("GetArtPicture Out");
        return artPicMem;
    }
    MEDIA_LOG_E("GetArtPicture Failed");
    return nullptr;
}

void AVMetaDataCollector::ConvertToAVMeta(const Meta &innerMeta, Metadata &avmeta) const
{
    for (const auto &[avKey, innerKey] : AVMETA_KEY_TO_X_MAP) {
        if (innerKey.compare("") == 0) {
            std::string strVal;
            if (innerMeta->GetData(innerKey, strVal) && strVal) {
                avmeta.SetMeta(avKey, ConvertTimestampToDatetime(strVal));
            } else {
                avmeta.SetMeta(avKey, "");
            }
        }

        if (innerKey.compare(Tag::MIME_TYPE) == 0) {
            continue;
        }

        Any type = OHOS::Media::GetDefaultAnyValue(innerKey);
        if (Any::IsSameTypeWith<int32_t>(type)) {
            int32_t intVal;
            avmeta.SetMeta(avKey, innerMeta->GetData(innerKey, intVal) ? std::to_string(intVal) : "");
        } else if (Any::IsSameTypeWith<std::string>(type)) {
            std::string strVal;
            avmeta.SetMeta(avKey, innerMeta->GetData(innerKey, duration) ? strVal : "");
        } else if (Any::IsSameTypeWith<Plugin::VideoRotation>(type)) {
            Plugin::VideoRotation rotation;
            avmeta.SetMeta(avKey, innerMeta->GetData(innerKey, duration) ? std::to_string(rotation) : "");
        } else if (Any::IsSameTypeWith<int64_t>(type)) {
            int64_t duration;
            avmeta.SetMeta(avKey, innerMeta->GetData(innerKey, duration) ? std::to_string(duration) : "");
        } else {
            MEDIA_LOG_E("not found type matched with innerKey: %{public}s", innerKey.c_str());
        }
    }
}

std::string AVMetaDataCollector::ConvertTimestampToDatetime(const std::string &timestamp)
{
    if (timestamp == nullptr || timestamp.empty()) {
        MEDIA_LOG_E("datetime is empty, format failed");
        return "";
    }
    time_t ts = stoi(timestamp);
    struct tm *ptm;
    char date[20], time[20];

    ptm = localtime(&ts);
    strftime(date, 20, "%Y-%m-%d", ptm);
    strftime(time, 20, "%H:%M:%S", ptm);

    std::string datetime = std::string(date) + " " + std::string(time);
    return datetime;
}
} // namespace Media
} // namespace OHOS