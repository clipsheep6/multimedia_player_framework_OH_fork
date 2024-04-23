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

#include "avmetadatahelper.h"
#include "buffer/avsharedmemorybase.h"
#include "media_log.h"
#include "meta/video_types.h"
#include "meta/any.h"
#include "time_format_utils.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN, "AVMetaDataCollector" };
}  // namespace

namespace OHOS {
namespace Media {
static constexpr int PICTURE_MAX_SIZE = 1024 * 1024;
static constexpr int SECOND_DEVIDE_MS = 1000;

static const std::unordered_map<Plugins::FileType, std::string> fileTypeMap = {
    { Plugins::FileType::UNKNOW, "uknown" },
    { Plugins::FileType::MP4, "mp4" },
    { Plugins::FileType::MPEGTS, "mpeg" },
    { Plugins::FileType::MKV, "mkv" },
    { Plugins::FileType::AMR, "amr" },
    { Plugins::FileType::AAC, "aac-adts" },
    { Plugins::FileType::MP3, "mpeg" },
    { Plugins::FileType::FLAC, "flac" },
    { Plugins::FileType::OGG, "ogg" },
    { Plugins::FileType::M4A, "mp4" },
    { Plugins::FileType::WAV, "wav" }
};

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
    { AV_KEY_VIDEO_IS_HDR_VIVID, Tag::VIDEO_IS_HDR_VIVID },
    { AV_KEY_LOCATION_LONGITUDE, Tag::MEDIA_LONGITUDE},
    { AV_KEY_LOCATION_LATITUDE, Tag::MEDIA_LATITUDE},
    { AV_KEY_CUSTOMINFO, "customInfo"},
};

AVMetaDataCollector::AVMetaDataCollector(std::shared_ptr<MediaDemuxer> &mediaDemuxer) : mediaDemuxer_(mediaDemuxer)
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

AVMetaDataCollector::~AVMetaDataCollector()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

std::unordered_map<int32_t, std::string> AVMetaDataCollector::ExtractMetadata()
{
    if (collectedMeta_.size() != 0) {
        return collectedMeta_;
    }
    const std::shared_ptr<Meta> globalInfo = mediaDemuxer_->GetGlobalMetaInfo();
    const std::vector<std::shared_ptr<Meta>> trackInfos = mediaDemuxer_->GetStreamMetaInfo();
    collectedMeta_ = GetMetadata(globalInfo, trackInfos);
    return collectedMeta_;
}

std::shared_ptr<Meta> AVMetaDataCollector::GetAVMetadata()
{
    if (collectedAVMetaData_ != nullptr) {
        return collectedAVMetaData_;
    }
    collectedAVMetaData_ = std::make_shared<Meta>();
    ExtractMetadata();
    CHECK_AND_RETURN_RET_LOG(collectedMeta_.size() != 0, nullptr, "globalInfo or trackInfos are invalid.");
    for (const auto &[avKey, value] : collectedMeta_) {
        if (avKey == AV_KEY_LOCATION_LATITUDE || avKey == AV_KEY_LOCATION_LONGITUDE) {
            continue;
        }
        if (avKey == AV_KEY_VIDEO_IS_HDR_VIVID) {
            int32_t hdr;
            if (value == "yes") {
                hdr = static_cast<int32_t>(HdrType::AV_HDR_TYPE_VIVID);
            } else {
                hdr = static_cast<int32_t>(HdrType::AV_HDR_TYPE_NONE);
            }
            collectedAVMetaData_->SetData("hdrType", hdr);
            continue;
        }
        auto iter = g_MetadataCodeMap.find(avKey);
        if (iter != g_MetadataCodeMap.end()) {
            collectedAVMetaData_->SetData(iter->second, value);
        }
    }

    customInfo_ = mediaDemuxer_->GetUserMeta();
    if (customInfo_ == nullptr) {
        MEDIA_LOGW("No valid user data");
    } else {
        if (AVMETA_KEY_TO_X_MAP.find(AV_KEY_CUSTOMINFO) != AVMETA_KEY_TO_X_MAP.end()) {
            collectedAVMetaData_->SetData(AVMETA_KEY_TO_X_MAP.find(AV_KEY_CUSTOMINFO)->second, customInfo_);
        }
    }
    return collectedAVMetaData_;
}

std::string AVMetaDataCollector::ExtractMetadata(int32_t key)
{
    auto metadata = GetAVMetadata();
    CHECK_AND_RETURN_RET_LOG(collectedMeta_.size() != 0, "", "Failed to call ExtractMetadata");

    auto it = collectedMeta_.find(key);
    if (it == collectedMeta_.end() || it->second.empty()) {
        MEDIA_LOGE("The specified metadata %{public}d cannot be obtained from the specified stream.", key);
        return "";
    }
    return collectedMeta_[key];
}

std::unordered_map<int32_t, std::string> AVMetaDataCollector::GetMetadata(
    const std::shared_ptr<Meta> &globalInfo, const std::vector<std::shared_ptr<Meta>> &trackInfos)
{
    CHECK_AND_RETURN_RET_LOG(
        globalInfo != nullptr && trackInfos.size() != 0, {}, "globalInfo or trackInfos are invalid.");

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

        // skip the image track
        std::string mime;
        meta->Get<Tag::MIME_TYPE>(mime);
        int32_t imageTypeLength = 5;
        if (mime.substr(0, imageTypeLength).compare("image") == 0) {
            MEDIA_LOGI("0x%{public}06" PRIXPTR " skip image track", FAKE_POINTER(this));
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
        MEDIA_LOGD("metadata tbl, key: %{public}d, keyName: %{public}s, val: %{public}s", it->first,
            AVMETA_KEY_TO_X_MAP.find(it->first)->second.c_str(), it->second.c_str());
        it++;
    }
    return metadata.tbl_;
}

std::shared_ptr<AVSharedMemory> AVMetaDataCollector::GetArtPicture()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " GetArtPicture In", FAKE_POINTER(this));

    if (collectedArtPicture_ != nullptr) {
        return collectedArtPicture_;
    }
    const std::vector<std::shared_ptr<Meta>> trackInfos = mediaDemuxer_->GetStreamMetaInfo();
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
        if (coverAddr.size() == 0 || static_cast<int>(coverAddr.size()) > PICTURE_MAX_SIZE) {
            MEDIA_LOGE("InvalidArtPictureSize %d", coverAddr.size());
            return nullptr;
        }
        uint8_t *addr = coverAddr.data();
        size_t size = coverAddr.size();
        auto artPicMem =
            AVSharedMemoryBase::CreateFromLocal(static_cast<int32_t>(size), AVSharedMemory::FLAGS_READ_ONLY, "artpic");
        errno_t rc = memcpy_s(artPicMem->GetBase(), static_cast<size_t>(artPicMem->GetSize()), addr, size);
        if (rc != EOK) {
            MEDIA_LOGE("memcpy_s failed, trackCount no %{public}d", index);
            return nullptr;
        }
        collectedArtPicture_ = artPicMem;
        return collectedArtPicture_;
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
                avmeta.SetMeta(avKey, TimeFormatUtils::ConvertTimestampToDatetime(strVal));
            }
        }
        if (innerKey.compare("customInfo") == 0) {
            continue;
        }
        if (!SetStringByValueType(innerMeta, avmeta, avKey, innerKey)) {
            break;
        }
        SetEmptyStringIfNoData(avmeta, avKey);
    }
}

void AVMetaDataCollector::FormatAVMeta(
    Metadata &avmeta, int32_t imageTrackCount, const std::shared_ptr<Meta> &globalInfo)
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
        formattedDateTime = TimeFormatUtils::FormatDateTimeByTimeZone(date);
    } else if (!creationTime.empty()) {
        formattedDateTime = TimeFormatUtils::FormatDateTimeByTimeZone(creationTime);
    }
    avmeta.SetMeta(AV_KEY_DATE_TIME, formattedDateTime);
    avmeta.SetMeta(AV_KEY_DATE_TIME_FORMAT,
        formattedDateTime.compare(date) != 0 ? formattedDateTime : TimeFormatUtils::FormatDataTimeByString(date));
}

void AVMetaDataCollector::SetEmptyStringIfNoData(Metadata &avmeta, int32_t avKey) const
{
    if (!avmeta.HasMeta(avKey)) {
        avmeta.SetMeta(avKey, "");
    }
}

bool AVMetaDataCollector::SetStringByValueType(const std::shared_ptr<Meta> &innerMeta,
    Metadata &avmeta, int32_t avKey, std::string innerKey) const
{
    Any type = OHOS::Media::GetDefaultAnyValue(innerKey);
    if (Any::IsSameTypeWith<int32_t>(type)) {
        int32_t intVal;
        if (innerMeta->GetData(innerKey, intVal) && intVal != 0) {
            avmeta.SetMeta(avKey, std::to_string(intVal));
        }
    } else if (Any::IsSameTypeWith<std::string>(type)) {
        std::string strVal;
        if (innerMeta->GetData(innerKey, strVal)) {
            avmeta.SetMeta(avKey, strVal);
        }
    } else if (Any::IsSameTypeWith<Plugins::VideoRotation>(type)) {
        Plugins::VideoRotation rotation;
        if (innerMeta->GetData(innerKey, rotation)) {
            avmeta.SetMeta(avKey, std::to_string(rotation));
        }
    } else if (Any::IsSameTypeWith<int64_t>(type)) {
        int64_t duration;
        if (innerMeta->GetData(innerKey, duration)) {
            avmeta.SetMeta(avKey, std::to_string(duration / SECOND_DEVIDE_MS));
        }
    } else if (Any::IsSameTypeWith<bool>(type)) {
        bool isTrue;
        if (innerMeta->GetData(innerKey, isTrue)) {
            avmeta.SetMeta(avKey, isTrue ? "yes" : "");
        }
    } else if (Any::IsSameTypeWith<float>(type)) {
        float value;
        if (innerMeta->GetData(innerKey, value) && collectedAVMetaData_ != nullptr) {
            collectedAVMetaData_->SetData(innerKey, value);
        }
    } else {
        MEDIA_LOGE("not found type matched with innerKey: %{public}s", innerKey.c_str());
        return false;
    }
    return true;
}

void AVMetaDataCollector::Reset()
{
    mediaDemuxer_->Reset();
    collectedMeta_.clear();
    collectedArtPicture_ = nullptr;
}

void AVMetaDataCollector::Destroy()
{
    mediaDemuxer_ = nullptr;
}
}  // namespace Media
}  // namespace OHOS