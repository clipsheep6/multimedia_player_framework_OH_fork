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
#include "string"
#include "iostream"
#include "sstream"
#include "regex"
#include "iomanip"
#include "ctime"
#include "chrono"
#include "media_log.h"
#include "meta/video_types.h"
#include "buffer/avsharedmemorybase.h"
#include "meta/any.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetaDataCollector"};
}

namespace OHOS {
namespace Media {
static const std::unordered_map<int32_t, std::string> AVMETA_KEY_TO_X_MAP = {
    { AV_KEY_ALBUM, Tag::MEDIA_ALBUM },
    { AV_KEY_ALBUM_ARTIST, Tag::MEDIA_ALBUM_ARTIST },
    { AV_KEY_ARTIST, Tag::MEDIA_ARTIST },
    { AV_KEY_AUTHOR, Tag::MEDIA_AUTHOR },
    { AV_KEY_COMPOSER, Tag::MEDIA_COMPOSER },
    { AV_KEY_DATE_TIME, Tag::MEDIA_DATE },
    { AV_KEY_DATE_TIME_FORMAT, Tag::MEDIA_CREATION_TIME },
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
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

AVMetaDataCollector::~AVMetaDataCollector()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

std::unordered_map<int32_t, std::string> AVMetaDataCollector::GetMetadata(const std::shared_ptr<Meta> &globalInfo,
    const std::vector<std::shared_ptr<Meta>> &trackInfos)
{
    CHECK_AND_RETURN_RET_LOG(globalInfo != nullptr && trackInfos.size() != 0,
        {}, "globalInfo or trackInfos are invalid.");

    Metadata metadata;
    ConvertToAVMeta(globalInfo, metadata);

    int32_t imageTrackCount = 0;
    size_t trackCount = trackInfos.size();
    for (size_t index = 0; index < trackCount; index++) {
        std::shared_ptr<Meta> meta = trackInfos[index];
        if (meta == nullptr) {
            MEDIA_LOGE("meta is invalid, index: %zu", index);
            return metadata.tbl_;
        }

        std::string mime;
        meta->Get<Tag::MIME_TYPE>(mime);
        int32_t imageTypeLength = 5;
        if (mime.substr(0, imageTypeLength).compare("image") == 0) {
            MEDIA_LOGI("skip image track");
            ++imageTrackCount;
            continue;
        }

        Plugins::MediaType mediaType;
        if (!meta->GetData(Tag::MEDIA_TYPE, mediaType)) {
            MEDIA_LOGE("mediaType not found, index: %zu", index);
            return metadata.tbl_;
        }

        ConvertToAVMeta(meta, metadata);
    }
    FormatAVMeta(metadata, imageTrackCount, globalInfo);
    auto it = metadata.tbl_.begin();
    while (it != metadata.tbl_.end()) {
        MEDIA_LOGI("metadata tbl, key: %{public}d, keyName: %{public}s, val: %{public}s", it->first,
            AVMETA_KEY_TO_X_MAP.find(it->first)->second.c_str(), it->second.c_str());
        it++;
    }
    return metadata.tbl_;
}

std::shared_ptr<AVSharedMemory> AVMetaDataCollector::GetArtPicture(const std::vector<std::shared_ptr<Meta>> &trackInfos)
{
    MEDIA_LOGI("GetArtPicture In");
    size_t trackCount = trackInfos.size();
    for (size_t index = 0; index < trackCount; index++) {
        std::shared_ptr<Meta> meta = trackInfos[index];
        if (meta == nullptr) {
            MEDIA_LOGW("meta is invalid, index: %zu", index);
            continue;
        }

        std::vector<uint8_t> coverAddr;
        auto mapIt = meta->Find(Tag::MEDIA_COVER);
        if (mapIt == meta->end()) {
            continue;
        }
        if (Any::IsSameTypeWith<std::vector<uint8_t>>(mapIt->second)) {
            coverAddr = AnyCast<std::vector<uint8_t>>(mapIt->second);
        }
        if (coverAddr.size() == 0 || static_cast<int>(coverAddr.size()) > artPictureMaxSize) {
            MEDIA_LOGE("InvalidArtPictureSize %d", coverAddr.size());
            return nullptr;
        }
        uint8_t *addr = coverAddr.data();
        size_t size = coverAddr.size();
        auto artPicMem = AVSharedMemoryBase::CreateFromLocal(
            static_cast<int32_t>(size), AVSharedMemory::FLAGS_READ_ONLY, "artpic");
        errno_t rc = memcpy_s(artPicMem->GetBase(), static_cast<size_t>(artPicMem->GetSize()), addr, size);
        if (rc != EOK) {
            MEDIA_LOGE("memcpy_s failed, trackCount no %{public}d", index);
            return nullptr;
        }
        
        MEDIA_LOGI("GetArtPicture Out");
        return artPicMem;
    }
    MEDIA_LOGE("GetArtPicture Failed");
    return nullptr;
}

void AVMetaDataCollector::ConvertToAVMeta(const std::shared_ptr<Meta> &innerMeta, Metadata &avmeta) const
{
    for (const auto &[avKey, innerKey] : AVMETA_KEY_TO_X_MAP) {
        if (innerKey.compare("") == 0) {
            std::string strVal;
            if (innerMeta->GetData(innerKey, strVal) && !strVal.empty()) {
                avmeta.SetMeta(avKey, ConvertTimestampToDatetime(strVal));
            }
            SetEmptyStringIfNoData(avmeta, avKey);
        }

        Any type = OHOS::Media::GetDefaultAnyValue(innerKey);
        if (Any::IsSameTypeWith<int32_t>(type)) {
            int32_t intVal;
            if (innerMeta->GetData(innerKey, intVal) && intVal != 0) {
                avmeta.SetMeta(avKey, std::to_string(intVal));
            }
            SetEmptyStringIfNoData(avmeta, avKey);
        } else if (Any::IsSameTypeWith<std::string>(type)) {
            std::string strVal;
            if (innerMeta->GetData(innerKey, strVal)) {
                avmeta.SetMeta(avKey, strVal);
            }
            SetEmptyStringIfNoData(avmeta, avKey);
        } else if (Any::IsSameTypeWith<Plugins::VideoRotation>(type)) {
            Plugins::VideoRotation rotation;
            if (innerMeta->GetData(innerKey, rotation)) {
                avmeta.SetMeta(avKey, std::to_string(rotation));
            }
            SetEmptyStringIfNoData(avmeta, avKey);
        } else if (Any::IsSameTypeWith<int64_t>(type)) {
            int64_t duration;
            if (innerMeta->GetData(innerKey, duration)) {
                avmeta.SetMeta(avKey, std::to_string(duration / secondDividMs));
            }
            SetEmptyStringIfNoData(avmeta, avKey);
        } else if (Any::IsSameTypeWith<bool>(type)) {
            bool isTrue;
            if (innerMeta->GetData(innerKey, isTrue)) {
                avmeta.SetMeta(avKey, isTrue ? "yes" : "");
            }
            SetEmptyStringIfNoData(avmeta, avKey);
        } else {
            MEDIA_LOGE("not found type matched with innerKey: %{public}s", innerKey.c_str());
        }
    }
}

void AVMetaDataCollector::FormatAVMeta(Metadata &avmeta, int32_t imageTrackCount,
    const std::shared_ptr<Meta> &globalInfo)
{
    std::string str = avmeta.GetMeta(AV_KEY_NUM_TRACKS);
    if (!str.empty()) {
        avmeta.SetMeta(AV_KEY_NUM_TRACKS, std::to_string(std::stoi(str) - imageTrackCount));
    }
    FormatMimeType(avmeta, globalInfo);
    FormatDateTime(avmeta, globalInfo);
}

void AVMetaDataCollector::FormatMimeType(Metadata &avmeta, const std::shared_ptr<Meta> &globalInfo)
{
    Plugins::FileType fileType;
    globalInfo->GetData(Tag::MEDIA_FILE_TYPE, fileType);
    CHECK_AND_RETURN_LOG(fileType != Plugins::FileType::UNKNOW, "unknown file type");
    if (fileTypeMap.find(fileType) == fileTypeMap.end()) {
        return;
    }
    if (avmeta.GetMeta(AV_KEY_HAS_VIDEO).compare("yes") == 0) {
        avmeta.SetMeta(AV_KEY_MIME_TYPE, "video/" + fileTypeMap.at(fileType));
        return;
    }
    if (avmeta.GetMeta(AV_KEY_HAS_AUDIO).compare("yes") == 0) {
        avmeta.SetMeta(AV_KEY_MIME_TYPE, "audio/" + fileTypeMap.at(fileType));
    }
}

void AVMetaDataCollector::FormatDateTime(Metadata &avmeta, const std::shared_ptr<Meta> &globalInfo)
{
    std::string date;
    std::string creationTime;
    globalInfo->GetData(Tag::MEDIA_DATE, date);
    globalInfo->GetData(Tag::MEDIA_CREATION_TIME, creationTime);
    std::string formattedDateTime;
    if (!date.empty()) {
        formattedDateTime = FormatDateTimeByTimeZone(date);
    } else if (!creationTime.empty()) {
        formattedDateTime = FormatDateTimeByTimeZone(creationTime);
    }
    avmeta.SetMeta(AV_KEY_DATE_TIME, formattedDateTime);
    avmeta.SetMeta(AV_KEY_DATE_TIME_FORMAT,
        formattedDateTime.compare(date) != 0 ? formattedDateTime : FormatDataTimeByString(date));
}

std::string AVMetaDataCollector::FormatDateTimeByTimeZone(const std::string &iso8601Str)
{
    std::regex pattern(R"((\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2})(.\d{1,6})?((\+|-\d{4})?)Z?)");
    std::smatch match;
    if (!std::regex_match(iso8601Str, match, pattern)) {
        return iso8601Str; // not standard ISO8601 type string
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
        long seconds = (hours * 60  + mins) * 60;
        diffTime = symbol == '+' ? seconds : -seconds;
    }

    // convert time to localtime
    long timezone = 0;
    std::tm timeWithOffset = *localtime(&tt);
    if (timeWithOffset.tm_gmtoff != 0) {
        timezone = timeWithOffset.tm_gmtoff;
    }
    auto localTime = std::chrono::system_clock::from_time_t(std::mktime(&tm))
        + std::chrono::seconds(timezone - diffTime);
    std::time_t localTimeT = std::chrono::system_clock::to_time_t(localTime);
    std::tm localTm = *std::localtime(&localTimeT);
    std::ostringstream oss;
    oss << std::put_time(&localTm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string AVMetaDataCollector::FormatDataTimeByString(const std::string &dataTime)
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
    MEDIA_LOGD("AV_KEY_DATE_TIME_FORMAT is: %{public}s%{public}s", data.c_str(), time.c_str());
    return data + time;
}

void AVMetaDataCollector::SetEmptyStringIfNoData(Metadata &avmeta, int32_t avKey) const
{
    if (!avmeta.HasMeta(avKey)) {
        avmeta.SetMeta(avKey, "");
    }
}

std::string AVMetaDataCollector::ConvertTimestampToDatetime(const std::string &timestamp) const
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
} // namespace Media
} // namespace OHOS