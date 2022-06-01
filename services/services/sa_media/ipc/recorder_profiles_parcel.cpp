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

#include "recorder_profiles_parcel.h"
#include "media_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderProfilesParcel"};
}

namespace OHOS {
namespace Media {
bool RecorderProfilesParcel::Marshalling(
    MessageParcel &parcel, const std::vector<RecorderProfilesData> &profileCapabilityDataArray)
{
    parcel.WriteUint32(profileCapabilityDataArray.size());
    for (auto it = profileCapabilityDataArray.begin(); it != profileCapabilityDataArray.end(); it++) {
        //  string
        (void)parcel.WriteString(it->containerFormatType);
        (void)parcel.WriteString(it->mimeType);
        (void)parcel.WriteString(it->audioEncoderMime);
        (void)parcel.WriteString(it->videoEncoderMime);
        (void)parcel.WriteString(it->audioCodec);
        (void)parcel.WriteString(it->videoCodec);

        //  Range
        (void)parcel.WriteInt32(it->bitrate.minVal);
        (void)parcel.WriteInt32(it->bitrate.maxVal);
        (void)parcel.WriteInt32(it->channels.minVal);
        (void)parcel.WriteInt32(it->channels.maxVal);
        (void)parcel.WriteInt32(it->audioBitrateRange.minVal);
        (void)parcel.WriteInt32(it->audioBitrateRange.maxVal);
        (void)parcel.WriteInt32(it->audioChannelRange.minVal);
        (void)parcel.WriteInt32(it->audioChannelRange.maxVal);
        (void)parcel.WriteInt32(it->videoBitrateRange.minVal);
        (void)parcel.WriteInt32(it->videoBitrateRange.maxVal);
        (void)parcel.WriteInt32(it->videoFramerateRange.minVal);
        (void)parcel.WriteInt32(it->videoFramerateRange.maxVal);
        (void)parcel.WriteInt32(it->videoWidthRange.minVal);
        (void)parcel.WriteInt32(it->videoWidthRange.maxVal);
        (void)parcel.WriteInt32(it->videoHeightRange.minVal);
        (void)parcel.WriteInt32(it->videoHeightRange.maxVal);

        //  int32_t
        (void)parcel.WriteInt32(it->sourceId);
        (void)parcel.WriteInt32(it->mediaProfileType);
        (void)parcel.WriteInt32(it->audioBitrate);
        (void)parcel.WriteInt32(it->audioChannels);
        (void)parcel.WriteInt32(it->audioSampleRate);
        (void)parcel.WriteInt32(it->durationTime);
        (void)parcel.WriteInt32(it->qualityLevel);
        (void)parcel.WriteInt32(it->videoBitrate);
        (void)parcel.WriteInt32(it->videoFrameWidth);
        (void)parcel.WriteInt32(it->videoFrameHeight);
        (void)parcel.WriteInt32(it->videoFrameRate);

        //  std::vector<int32_t>
        (void)parcel.WriteInt32Vector(it->sampleRate);
        (void)parcel.WriteInt32Vector(it->audioSampleRates);
    }
    MEDIA_LOGD("success to Marshalling profileCapabilityDataArray");

    return true;
}

bool RecorderProfilesParcel::Marshalling(MessageParcel &parcel, const RecorderProfilesData &profileCapabilityData)
{
    //  string
    (void)parcel.WriteString(profileCapabilityData.containerFormatType);
    (void)parcel.WriteString(profileCapabilityData.mimeType);
    (void)parcel.WriteString(profileCapabilityData.audioEncoderMime);
    (void)parcel.WriteString(profileCapabilityData.videoEncoderMime);
    (void)parcel.WriteString(profileCapabilityData.audioCodec);
    (void)parcel.WriteString(profileCapabilityData.videoCodec);

    //  Range
    (void)parcel.WriteInt32(profileCapabilityData.bitrate.minVal);
    (void)parcel.WriteInt32(profileCapabilityData.bitrate.maxVal);
    (void)parcel.WriteInt32(profileCapabilityData.channels.minVal);
    (void)parcel.WriteInt32(profileCapabilityData.channels.maxVal);
    (void)parcel.WriteInt32(profileCapabilityData.audioBitrateRange.minVal);
    (void)parcel.WriteInt32(profileCapabilityData.audioBitrateRange.maxVal);
    (void)parcel.WriteInt32(profileCapabilityData.audioChannelRange.minVal);
    (void)parcel.WriteInt32(profileCapabilityData.audioChannelRange.maxVal);
    (void)parcel.WriteInt32(profileCapabilityData.videoBitrateRange.minVal);
    (void)parcel.WriteInt32(profileCapabilityData.videoBitrateRange.maxVal);
    (void)parcel.WriteInt32(profileCapabilityData.videoFramerateRange.minVal);
    (void)parcel.WriteInt32(profileCapabilityData.videoFramerateRange.maxVal);
    (void)parcel.WriteInt32(profileCapabilityData.videoWidthRange.minVal);
    (void)parcel.WriteInt32(profileCapabilityData.videoWidthRange.maxVal);
    (void)parcel.WriteInt32(profileCapabilityData.videoHeightRange.minVal);
    (void)parcel.WriteInt32(profileCapabilityData.videoHeightRange.maxVal);

    //  int32_t
    (void)parcel.WriteInt32(profileCapabilityData.sourceId);
    (void)parcel.WriteInt32(profileCapabilityData.mediaProfileType);
    (void)parcel.WriteInt32(profileCapabilityData.audioBitrate);
    (void)parcel.WriteInt32(profileCapabilityData.audioChannels);
    (void)parcel.WriteInt32(profileCapabilityData.audioSampleRate);
    (void)parcel.WriteInt32(profileCapabilityData.durationTime);
    (void)parcel.WriteInt32(profileCapabilityData.qualityLevel);
    (void)parcel.WriteInt32(profileCapabilityData.videoBitrate);
    (void)parcel.WriteInt32(profileCapabilityData.videoFrameWidth);
    (void)parcel.WriteInt32(profileCapabilityData.videoFrameHeight);
    (void)parcel.WriteInt32(profileCapabilityData.videoFrameRate);

    //  std::vector<int32_t>
    (void)parcel.WriteInt32Vector(profileCapabilityData.sampleRate);
    (void)parcel.WriteInt32Vector(profileCapabilityData.audioSampleRates);

    MEDIA_LOGD("success to Marshalling profileCapabilityData");

    return true;
}

bool RecorderProfilesParcel::Unmarshalling(
    MessageParcel &parcel, std::vector<RecorderProfilesData> &profileCapabilityDataArray)
{
    uint32_t size = parcel.ReadUint32();
    for (uint32_t index = 0; index < size; index++) {
        RecorderProfilesData profileCapabilityData;

        //  string
        profileCapabilityData.containerFormatType = parcel.ReadString();
        profileCapabilityData.mimeType = parcel.ReadString();
        profileCapabilityData.audioEncoderMime = parcel.ReadString();
        profileCapabilityData.videoEncoderMime = parcel.ReadString();
        profileCapabilityData.audioCodec = parcel.ReadString();
        profileCapabilityData.videoCodec = parcel.ReadString();

        //  Range
        profileCapabilityData.bitrate.minVal = parcel.ReadInt32();
        profileCapabilityData.bitrate.maxVal = parcel.ReadInt32();
        profileCapabilityData.channels.minVal = parcel.ReadInt32();
        profileCapabilityData.channels.maxVal = parcel.ReadInt32();
        profileCapabilityData.audioBitrateRange.minVal = parcel.ReadInt32();
        profileCapabilityData.audioBitrateRange.maxVal = parcel.ReadInt32();
        profileCapabilityData.audioChannelRange.minVal = parcel.ReadInt32();
        profileCapabilityData.audioChannelRange.maxVal = parcel.ReadInt32();
        profileCapabilityData.videoBitrateRange.minVal = parcel.ReadInt32();
        profileCapabilityData.videoBitrateRange.maxVal = parcel.ReadInt32();
        profileCapabilityData.videoFramerateRange.minVal = parcel.ReadInt32();
        profileCapabilityData.videoFramerateRange.maxVal = parcel.ReadInt32();
        profileCapabilityData.videoWidthRange.minVal = parcel.ReadInt32();
        profileCapabilityData.videoWidthRange.maxVal = parcel.ReadInt32();
        profileCapabilityData.videoHeightRange.minVal = parcel.ReadInt32();
        profileCapabilityData.videoHeightRange.maxVal = parcel.ReadInt32();

        //  int32_t
        profileCapabilityData.sourceId = parcel.ReadInt32();
        profileCapabilityData.mediaProfileType = parcel.ReadInt32();
        profileCapabilityData.audioBitrate = parcel.ReadInt32();
        profileCapabilityData.audioChannels = parcel.ReadInt32();
        profileCapabilityData.audioSampleRate = parcel.ReadInt32();
        profileCapabilityData.durationTime = parcel.ReadInt32();
        profileCapabilityData.qualityLevel = parcel.ReadInt32();
        profileCapabilityData.videoBitrate = parcel.ReadInt32();
        profileCapabilityData.videoFrameWidth = parcel.ReadInt32();
        profileCapabilityData.videoFrameHeight = parcel.ReadInt32();
        profileCapabilityData.videoFrameRate = parcel.ReadInt32();

        //  std::vector<int32_t>
        parcel.ReadInt32Vector(&profileCapabilityData.sampleRate);
        parcel.ReadInt32Vector(&profileCapabilityData.audioSampleRates);

        profileCapabilityDataArray.push_back(profileCapabilityData);
    }
    MEDIA_LOGD("success to Unmarshalling profileCapabilityDataArray");

    return true;
}

bool RecorderProfilesParcel::Unmarshalling(MessageParcel &parcel, RecorderProfilesData &profileCapabilityData)
{
    //  string
    profileCapabilityData.containerFormatType = parcel.ReadString();
    profileCapabilityData.mimeType = parcel.ReadString();
    profileCapabilityData.audioEncoderMime = parcel.ReadString();
    profileCapabilityData.videoEncoderMime = parcel.ReadString();
    profileCapabilityData.audioCodec = parcel.ReadString();
    profileCapabilityData.videoCodec = parcel.ReadString();

    //  Range
    profileCapabilityData.bitrate.minVal = parcel.ReadInt32();
    profileCapabilityData.bitrate.maxVal = parcel.ReadInt32();
    profileCapabilityData.channels.minVal = parcel.ReadInt32();
    profileCapabilityData.channels.maxVal = parcel.ReadInt32();
    profileCapabilityData.audioBitrateRange.minVal = parcel.ReadInt32();
    profileCapabilityData.audioBitrateRange.maxVal = parcel.ReadInt32();
    profileCapabilityData.audioChannelRange.minVal = parcel.ReadInt32();
    profileCapabilityData.audioChannelRange.maxVal = parcel.ReadInt32();
    profileCapabilityData.videoBitrateRange.minVal = parcel.ReadInt32();
    profileCapabilityData.videoBitrateRange.maxVal = parcel.ReadInt32();
    profileCapabilityData.videoFramerateRange.minVal = parcel.ReadInt32();
    profileCapabilityData.videoFramerateRange.maxVal = parcel.ReadInt32();
    profileCapabilityData.videoWidthRange.minVal = parcel.ReadInt32();
    profileCapabilityData.videoWidthRange.maxVal = parcel.ReadInt32();
    profileCapabilityData.videoHeightRange.minVal = parcel.ReadInt32();
    profileCapabilityData.videoHeightRange.maxVal = parcel.ReadInt32();

    //  int32_t
    profileCapabilityData.sourceId = parcel.ReadInt32();
    profileCapabilityData.mediaProfileType = parcel.ReadInt32();
    profileCapabilityData.audioBitrate = parcel.ReadInt32();
    profileCapabilityData.audioChannels = parcel.ReadInt32();
    profileCapabilityData.audioSampleRate = parcel.ReadInt32();
    profileCapabilityData.durationTime = parcel.ReadInt32();
    profileCapabilityData.qualityLevel = parcel.ReadInt32();
    profileCapabilityData.videoBitrate = parcel.ReadInt32();
    profileCapabilityData.videoFrameWidth = parcel.ReadInt32();
    profileCapabilityData.videoFrameHeight = parcel.ReadInt32();
    profileCapabilityData.videoFrameRate = parcel.ReadInt32();

    //  std::vector<int32_t>
    parcel.ReadInt32Vector(&profileCapabilityData.sampleRate);
    parcel.ReadInt32Vector(&profileCapabilityData.audioSampleRates);

    MEDIA_LOGD("success to Unmarshalling profileCapabilityData");
    return true;
}
}  // namespace Media
}  // namespace OHOS
