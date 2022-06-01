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
#include "recorder_profiles_xml_parser.h"
#include "media_errors.h"
#include "media_log.h"
#include "string_ex.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderProfilesXmlParser"};
}

namespace OHOS {
namespace Media {
enum ProfileSourceType : int32_t {
    PORFILE_SOURCE_CAMERA_RECORDER = 0,
    PORFILE_SOURCE_VIRTUAL_DISPLAY
};

const std::vector<std::string> MP4_VIDEO_CODEC_VECTOR = {
    "video/mp4v-es",
    "video/avc",
};

const std::vector<std::string> MP4_AUDIO_CODEC_VECTOR = {
    "audio/mp4a-latm",
};

const std::vector<std::string> M4A_AUDIO_CODEC_VECTOR = {
    "audio/mp4a-latm",
};

const std::unordered_map<std::string, std::vector<std::string>> RECORDER_VIDEO_CAPS_MAP = {
    {"mp4", MP4_VIDEO_CODEC_VECTOR},
};

const std::unordered_map<std::string, std::vector<std::string>> RECORDER_AUDIO_CAPS_MAP = {
    {"mp4", MP4_AUDIO_CODEC_VECTOR},
    {"m4a", M4A_AUDIO_CODEC_VECTOR},
};

const std::unordered_map<std::string, int> SOURCE_TYPE_MAP = {
    {"CameraRecorder", PORFILE_SOURCE_CAMERA_RECORDER},
    {"VirtualDisplay", PORFILE_SOURCE_VIRTUAL_DISPLAY},
};

const std::unordered_map<std::string, std::string> SOURCE_TYPE_ID_MAP = {
    {"CameraRecorder", "cameraId"},
    {"VirtualDisplay", "displayId"},
};

const std::unordered_map<std::string, int> PROFILE_QUALITY_MAP = {
    {"low", RECORDER_QUALITY_LOW},
    {"high", RECORDER_QUALITY_HIGH},
    {"qcif", RECORDER_QUALITY_QCIF},
    {"cif", RECORDER_QUALITY_CIF},
    {"480p", RECORDER_QUALITY_480P},
    {"720p", RECORDER_QUALITY_720P},
    {"1080p", RECORDER_QUALITY_1080P},
    {"qvga", RECORDER_QUALITY_QVGA},
    {"2160p", RECORDER_QUALITY_2160P},
    {"timelapse_low", RECORDER_QUALITY_TIME_LAPSE_LOW},
    {"timelapse_high", RECORDER_QUALITY_TIME_LAPSE_HIGH},
    {"timelapse_qcif", RECORDER_QUALITY_TIME_LAPSE_QCIF},
    {"timelapse_cif", RECORDER_QUALITY_TIME_LAPSE_CIF},
    {"timelapse_480p", RECORDER_QUALITY_TIME_LAPSE_480P},
    {"timelapse_720p", RECORDER_QUALITY_TIME_LAPSE_720P},
    {"timelapse_1080p", RECORDER_QUALITY_TIME_LAPSE_1080P},
    {"timelapse_qvga", RECORDER_QUALITY_TIME_LAPSE_QVGA},
    {"timelapse_2160p", RECORDER_QUALITY_TIME_LAPSE_2160P},
    {"highspeed_low", RECORDER_QUALITY_HIGH_SPEED_LOW},
    {"highspeed_high", RECORDER_QUALITY_HIGH_SPEED_HIGH},
    {"highspeed_480p", RECORDER_QUALITY_HIGH_SPEED_480P},
    {"highspeed_720p", RECORDER_QUALITY_HIGH_SPEED_720P},
    {"highspeed_1080p", RECORDER_QUALITY_HIGH_SPEED_1080P},
};

RecorderProfilesXmlParser::RecorderProfilesXmlParser()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

RecorderProfilesXmlParser::~RecorderProfilesXmlParser()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool RecorderProfilesXmlParser::ParseInternal(xmlNode *node)
{
    containerFormatArray_.clear();
    videoEncoderCapsArray_.clear();
    audioEncoderCapsArray_.clear();
    capabilityDataArray_.clear();

    xmlNode *currNode = node;
    for (; currNode != nullptr; currNode = currNode->next) {
        if (XML_ELEMENT_NODE == currNode->type) {
            switch (GetNodeNameAsInt(currNode)) {
                case RECORDER_CAPS: {
                    ParseRecorderCapsData(currNode);
                    break;
                }
                case RECORDER_PROFILES: {
                    ParseRecorderProfilesData(currNode);
                    break;
                }
                default: {
                    ParseInternal(currNode->children);
                    break;
                }
            }
        }
    }
    return true;
}

bool RecorderProfilesXmlParser::SetCapabilityIntData(std::unordered_map<std::string,
    int32_t&> dataMap, const std::string &capabilityKey, const std::string &capabilityValue) const
{
    if (PROFILE_QUALITY_MAP.find(capabilityValue) != PROFILE_QUALITY_MAP.end()) {
        dataMap.at(capabilityKey) = PROFILE_QUALITY_MAP.at(capabilityValue);
    } else {
        int32_t value = 0;
        if (!StrToInt(capabilityValue, value)) {
            MEDIA_LOGE("call StrToInt func false, input str is: %{public}s", capabilityValue.c_str());
            return false;
        }
        dataMap.at(capabilityKey) = value;
    }
    return true;
}

bool RecorderProfilesXmlParser::SetCapabilityVectorData(std::unordered_map<std::string,
    std::vector<int32_t>&> dataMap, const std::string &capabilityKey, const std::string &capabilityValue) const
{
    std::vector<std::string> spilt;
    std::vector<int32_t> array;
    bool ret = SpiltKeyList(capabilityValue, ",", spilt);
    CHECK_AND_RETURN_RET_LOG(ret != false, false, "failed:can not split %{public}s", capabilityValue.c_str());
    if (spilt.size() > 0) {
        std::string probe = spilt[0];
        if (XmlParser::IsNumberArray(spilt)) {
            array = TransStrAsIntegerArray(spilt);
        } else {
            MEDIA_LOGE("The value of %{public}s in the configuration file is incorrect.", capabilityValue.c_str());
            return false;
        }
        dataMap.at(capabilityKey) = array;
    }
    MEDIA_LOGD("RecorderProfilesXmlParser::SetCapabilityVectorData end");
    return true;
}

bool RecorderProfilesXmlParser::ParseRecorderCapsData(xmlNode *node)
{
    xmlNode *child = node->xmlChildrenNode;
    std::string capabilityValue;

    for (; child; child = child->next) {
        if (0 == xmlStrcmp(child->name, reinterpret_cast<const xmlChar*>("ContainerFormat"))) {
            bool ret = ParseRecorderContainerFormatData(child);
            CHECK_AND_RETURN_RET_LOG(ret != false, false, "ParseRecorderContainerFormatData failed");
        } else if (0 == xmlStrcmp(child->name, reinterpret_cast<const xmlChar*>("VideoEncoderCaps"))) {
            bool ret = ParseRecorderEncodeCapsData(child, true);
            CHECK_AND_RETURN_RET_LOG(ret != false, false, "ParseRecorderEncodeCapsData failed");
        } else if (0 == xmlStrcmp(child->name, reinterpret_cast<const xmlChar*>("AudioEncoderCaps"))) {
            bool ret = ParseRecorderEncodeCapsData(child, false);
            CHECK_AND_RETURN_RET_LOG(ret != false, false, "ParseRecorderEncodeCapsData failed");
        } else {
            MEDIA_LOGE("not found node!");
        }
    }
    PackageRecorderCaps();
    MEDIA_LOGD("RecorderProfilesXmlParser::ParseRecorderCapsData end");
    return true;
}

bool RecorderProfilesXmlParser::ParseRecorderContainerFormatData(xmlNode *node)
{
    ContainerFormatInfo containerFormatInfo;
    bool ret = false;
    for (auto it = capabilityKeys_.begin(); it != capabilityKeys_.end(); it++) {
        std::string capabilityKey = *it;
        std::string capabilityValue;
        if (xmlHasProp(node, reinterpret_cast<xmlChar*>(const_cast<char*>(capabilityKey.c_str())))) {
            capabilityValue = std::string(reinterpret_cast<char*>(xmlGetProp(node,
                reinterpret_cast<xmlChar*>(const_cast<char*>(capabilityKey.c_str())))));
            ret = SetContainerFormat(containerFormatInfo, capabilityKey, capabilityValue);
            CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetContainerFormat failed");
        }
    }
    containerFormatArray_.emplace_back(containerFormatInfo);
    MEDIA_LOGD("RecorderProfilesXmlParser::ParseRecorderContainerFormatData end");
    return true;
}

bool RecorderProfilesXmlParser::ParseRecorderEncodeCapsData(xmlNode *node, bool isVideo)
{
    RecorderProfilesData capabilityData;
    bool ret = false;
    for (auto it = capabilityKeys_.begin(); it != capabilityKeys_.end(); it++) {
        std::string capabilityKey = *it;
        std::string capabilityValue;
        if (xmlHasProp(node, reinterpret_cast<xmlChar*>(const_cast<char*>(capabilityKey.c_str())))) {
            capabilityValue = std::string(reinterpret_cast<char*>(xmlGetProp(node,
                reinterpret_cast<xmlChar*>(const_cast<char*>(capabilityKey.c_str())))));
            if (isVideo) {
                ret = SetVideoRecorderCaps(capabilityData, capabilityKey, capabilityValue);
                CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetVideoRecorderCaps failed");
            } else {
                ret = SetAudioRecorderCaps(capabilityData, capabilityKey, capabilityValue);
                CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetAudioRecorderCaps failed");
            }
        }
    }

    if (isVideo) {
        videoEncoderCapsArray_.emplace_back(capabilityData);
    } else {
        audioEncoderCapsArray_.emplace_back(capabilityData);
    }
    MEDIA_LOGD("RecorderProfilesXmlParser::ParseRecorderEncodeCapsData end");
    return true;
}

bool RecorderProfilesXmlParser::SetContainerFormat(ContainerFormatInfo &data, const std::string &capabilityKey,
                                                   const std::string &capabilityValue)
{
    std::unordered_map<std::string, std::string&> capabilityStringMap = {
        {"name", data.name}, {"hasVideo", data.hasVideo}};

    bool ret = false;
    if (capabilityStringMap.find(capabilityKey) != capabilityStringMap.end()) {
        ret = SetCapabilityStringData(capabilityStringMap, capabilityKey, capabilityValue);
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetCapabilityStringData failed");
    } else {
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "can not find capabilityKey: %{public}s", capabilityKey.c_str());
    }
    MEDIA_LOGD("RecorderProfilesXmlParser::SetContainerFormat end");
    return true;
}

bool RecorderProfilesXmlParser::SetVideoRecorderCaps(RecorderProfilesData &data, const std::string &capabilityKey,
                                                     const std::string &capabilityValue)
{
    std::unordered_map<std::string, std::string&> capabilityStringMap = {
        {"codecMime", data.videoEncoderMime}};

    std::unordered_map<std::string, Range&> capabilityRangeMap = {
        {"bitrate", data.videoBitrateRange}, {"width", data.videoWidthRange}, {"height", data.videoHeightRange},
        {"frameRate", data.videoFramerateRange}};

    bool ret = false;
    if (capabilityStringMap.find(capabilityKey) != capabilityStringMap.end()) {
        ret = SetCapabilityStringData(capabilityStringMap, capabilityKey, capabilityValue);
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetCapabilityStringData failed");
    } else if (capabilityRangeMap.find(capabilityKey) != capabilityRangeMap.end()) {
        ret = SetCapabilityRangeData(capabilityRangeMap, capabilityKey, capabilityValue);
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetCapabilityRangeData failed");
    } else {
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "can not find capabilityKey: %{public}s", capabilityKey.c_str());
    }
    MEDIA_LOGD("RecorderProfilesXmlParser::SetVideoRecorderCaps end");
    return true;
}

bool RecorderProfilesXmlParser::SetAudioRecorderCaps(RecorderProfilesData &data, const std::string &capabilityKey,
                                                     const std::string &capabilityValue)
{
    std::unordered_map<std::string, std::string&> capabilityStringMap = {
        {"codecMime", data.mimeType}};

    std::unordered_map<std::string, Range&> capabilityRangeMap = {
        {"bitrate", data.bitrate}, {"channels", data.channels}};

    std::unordered_map<std::string, std::vector<int32_t>&> capabilityVectorMap = {
        {"sampleRate", data.sampleRate}};

    bool ret = false;
    if (capabilityStringMap.find(capabilityKey) != capabilityStringMap.end()) {
        ret = SetCapabilityStringData(capabilityStringMap, capabilityKey, capabilityValue);
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetCapabilityStringData failed");
    } else if (capabilityRangeMap.find(capabilityKey) != capabilityRangeMap.end()) {
        ret = SetCapabilityRangeData(capabilityRangeMap, capabilityKey, capabilityValue);
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetCapabilityRangeData failed");
    } else if (capabilityVectorMap.find(capabilityKey) != capabilityVectorMap.end()) {
        ret = SetCapabilityVectorData(capabilityVectorMap, capabilityKey, capabilityValue);
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetCapabilityVectorData failed");
    } else {
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "can not find capabilityKey: %{public}s", capabilityKey.c_str());
    }
    MEDIA_LOGD("RecorderProfilesXmlParser::SetAudioRecorderCaps end");
    return true;
}

void RecorderProfilesXmlParser::PackageRecorderCaps()
{
    for (auto it = containerFormatArray_.begin(); it != containerFormatArray_.end(); it++) {
        ContainerFormatInfo  containerFormat = (*it);
        if (containerFormat.hasVideo == "true") {
            PackageVideoRecorderCaps(containerFormat.name);
        } else {
            PackageAudioRecorderCaps(containerFormat.name);
        }
    }
    MEDIA_LOGD("RecorderProfilesXmlParser::PackageRecorderCaps end");
}

void RecorderProfilesXmlParser::PackageVideoRecorderCaps(const std::string &formatType)
{
    if ((RECORDER_VIDEO_CAPS_MAP.find(formatType) == RECORDER_VIDEO_CAPS_MAP.end()) ||
        (RECORDER_AUDIO_CAPS_MAP.find(formatType) == RECORDER_AUDIO_CAPS_MAP.end())) {
        MEDIA_LOGE("formatType not found in RECORDER_VIDEO_CAPS_MAP or RECORDER_AUDIO_CAPS_MAP");
        return;
    }
    RecorderProfilesData videoData;
    for (auto itVideo = videoEncoderCapsArray_.begin(); itVideo != videoEncoderCapsArray_.end(); itVideo++) {
        videoData = (*itVideo);
        videoData.mediaProfileType = RECORDER_TYPE_VIDEO_CAPS;
        videoData.containerFormatType = formatType;
        auto itVideoCodec = find(RECORDER_VIDEO_CAPS_MAP.at(formatType).begin(),
            RECORDER_VIDEO_CAPS_MAP.at(formatType).end(), videoData.videoEncoderMime);
        if (itVideoCodec != RECORDER_VIDEO_CAPS_MAP.at(formatType).end()) {
            PaddingVideoCapsByAudioCaps(formatType, videoData);
        }
    }
    MEDIA_LOGD("RecorderProfilesXmlParser::PackageVideoRecorderCaps end");
}

void RecorderProfilesXmlParser::PaddingVideoCapsByAudioCaps(
    const std::string &formatType, RecorderProfilesData &videoData)
{
    RecorderProfilesData audioData;
    for (auto itAudio = audioEncoderCapsArray_.begin(); itAudio != audioEncoderCapsArray_.end(); itAudio++) {
        audioData = (*itAudio);
        auto itAudioCodec = find(RECORDER_AUDIO_CAPS_MAP.at(formatType).begin(),
            RECORDER_AUDIO_CAPS_MAP.at(formatType).end(), audioData.mimeType);
        if (itAudioCodec != RECORDER_AUDIO_CAPS_MAP.at(formatType).end()) {
            videoData.audioEncoderMime = audioData.mimeType;
            videoData.audioBitrateRange = audioData.bitrate;
            videoData.audioSampleRates = audioData.sampleRate;
            videoData.audioChannelRange = audioData.channels;
            capabilityDataArray_.emplace_back(videoData);
        }
    }
    MEDIA_LOGD("RecorderProfilesXmlParser::PaddingVideoCapsByAudioCaps end");
}

void RecorderProfilesXmlParser::PackageAudioRecorderCaps(const std::string &formatType)
{
    if (RECORDER_AUDIO_CAPS_MAP.find(formatType) == RECORDER_AUDIO_CAPS_MAP.end()) {
        MEDIA_LOGE("formatType not found in RECORDER_AUDIO_CAPS_MAP");
        return;
    }
    for (auto itAudio = audioEncoderCapsArray_.begin(); itAudio != audioEncoderCapsArray_.end(); itAudio++) {
        RecorderProfilesData audioData = (*itAudio);
        audioData.mediaProfileType = RECORDER_TYPE_AUDIO_CAPS;
        audioData.containerFormatType = formatType;
        auto itAudioCodec = find(RECORDER_AUDIO_CAPS_MAP.at(formatType).begin(),
            RECORDER_AUDIO_CAPS_MAP.at(formatType).end(), audioData.mimeType);
        if (itAudioCodec != RECORDER_AUDIO_CAPS_MAP.at(formatType).end()) {
            capabilityDataArray_.emplace_back(audioData);
        }
    }
    MEDIA_LOGD("RecorderProfilesXmlParser::PackageAudioRecorderCaps end");
}

bool RecorderProfilesXmlParser::ParseRecorderProfilesData(xmlNode *node)
{
    xmlNode *child = node->xmlChildrenNode;
    bool ret = false;
    for (; child; child = child->next) {
        for (auto it = SOURCE_TYPE_ID_MAP.begin(); it != SOURCE_TYPE_ID_MAP.end(); it++) {
            std::string sourceType = it->first;
            ret = ParseRecorderProfilesSourceData(sourceType, child);
            CHECK_AND_RETURN_RET_LOG(ret != false, false, "ParseRecorderProfilesSourceData failed");
        }
    }
    MEDIA_LOGD("RecorderProfilesXmlParser::ParseRecorderProfilesData end");
    return true;
}

bool RecorderProfilesXmlParser::ParseRecorderProfilesSourceData(const std::string sourceType, xmlNode *node)
{
    if (0 == xmlStrcmp(node->name, reinterpret_cast<const xmlChar*>(sourceType.c_str()))) {
        std::string property = SOURCE_TYPE_ID_MAP.at(sourceType);
        if (xmlHasProp(node, reinterpret_cast<xmlChar*>(const_cast<char*>(property.c_str())))) {
            std::string capabilityValue = std::string(reinterpret_cast<char*>(xmlGetProp(node,
                reinterpret_cast<xmlChar*>(const_cast<char*>(property.c_str())))));
            int32_t id = 0;
            if (!StrToInt(capabilityValue, id)) {
                MEDIA_LOGE("call StrToInt func false, input str is: %{public}s", capabilityValue.c_str());
                return false;
            }
            if (SOURCE_TYPE_MAP.find(sourceType) == SOURCE_TYPE_MAP.end()) {
                MEDIA_LOGE("not found sourceType");
                return false;
            }
            int32_t type = SOURCE_TYPE_MAP.at(sourceType);
            RecorderProfilesData capabilityData;
            // 8 : 8-15 bits indicates the type of source
            capabilityData.sourceId = ((type & 0x000000ff) << 8) | (id & 0x000000ff);
            bool ret = ParseRecorderProfileSettingsData(node, capabilityData);
            CHECK_AND_RETURN_RET_LOG(ret != false, false, "ParseRecorderProfileSettingsData failed");
        }
    }
    return true;
}

bool RecorderProfilesXmlParser::ParseRecorderProfileSettingsData(xmlNode *node, RecorderProfilesData &capabilityData)
{
    xmlNode *child = node->xmlChildrenNode;
    for (; child; child = child->next) {
        if (0 == xmlStrcmp(child->name, reinterpret_cast<const xmlChar*>("ProfileSettings"))) {
            for (auto it = capabilityKeys_.begin(); it != capabilityKeys_.end(); it++) {
                std::string capabilityKey = *it;
                if (xmlHasProp(child, reinterpret_cast<xmlChar*>(const_cast<char*>(capabilityKey.c_str())))) {
                    std::string capabilityValue = std::string(reinterpret_cast<char*>(xmlGetProp(child,
                        reinterpret_cast<xmlChar*>(const_cast<char*>(capabilityKey.c_str())))));
                    bool ret = SetVideoRecorderProfiles(capabilityData, capabilityKey, capabilityValue);
                    CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetVideoRecorderProfiles failed");
                }
            }

            bool ret = ParseRecorderProfileVideoAudioData(child, capabilityData);
            CHECK_AND_RETURN_RET_LOG(ret != false, false, "ParseRecorderProfileVideoAudioData failed");
        }
    }
    MEDIA_LOGD("RecorderProfilesXmlParser::ParseRecorderProfileSettingsData end");
    return true;
}

bool RecorderProfilesXmlParser::ParseRecorderProfileVideoAudioData(xmlNode *node,
    RecorderProfilesData &capabilityData)
{
    xmlNode *leafChild = node->xmlChildrenNode;
    std::string capabilityValue;
    for (; leafChild; leafChild = leafChild->next) {
        if (0 == xmlStrcmp(leafChild->name, reinterpret_cast<const xmlChar*>("Video"))) {
            for (auto it = capabilityKeys_.begin(); it != capabilityKeys_.end(); it++) {
                std::string capabilityKey = *it;
                if (xmlHasProp(leafChild, reinterpret_cast<xmlChar*>(const_cast<char*>(capabilityKey.c_str())))) {
                    capabilityValue = std::string(reinterpret_cast<char*>(xmlGetProp(leafChild,
                        reinterpret_cast<xmlChar*>(const_cast<char*>(capabilityKey.c_str())))));
                    capabilityData.mediaProfileType = RECORDER_TYPE_PROFILE;
                    bool ret = SetVideoRecorderProfiles(capabilityData, capabilityKey, capabilityValue);
                    CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetVideoRecorderProfiles failed");
                }
            }
        } else if (0 == xmlStrcmp(leafChild->name, reinterpret_cast<const xmlChar*>("Audio"))) {
            for (auto it = capabilityKeys_.begin(); it != capabilityKeys_.end(); it++) {
                std::string capabilityKey = *it;
                if (xmlHasProp(leafChild, reinterpret_cast<xmlChar*>(const_cast<char*>(capabilityKey.c_str())))) {
                    capabilityValue = std::string(reinterpret_cast<char*>(xmlGetProp(leafChild,
                        reinterpret_cast<xmlChar*>(const_cast<char*>(capabilityKey.c_str())))));
                    bool ret = SetAudioRecorderProfiles(capabilityData, capabilityKey, capabilityValue);
                    CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetAudioRecorderProfiles failed");
                }
            }
        } else {
            MEDIA_LOGE("not found video or audio node!");
        }
    }
    MEDIA_LOGD("RecorderProfilesXmlParser::ParseRecorderProfileVideoAudioData end");
    capabilityDataArray_.emplace_back(capabilityData);
    return true;
}

bool RecorderProfilesXmlParser::SetVideoRecorderProfiles(RecorderProfilesData &data, const std::string &capabilityKey,
                                                         const std::string &capabilityValue)
{
    std::unordered_map<std::string, std::string&> capabilityStringMap = {
        {"format", data.containerFormatType}, {"codecMime", data.videoCodec}};

    std::unordered_map<std::string, int32_t&> capabilityIntMap = {
        {"duration", data.durationTime}, {"bitrate", data.videoBitrate},
        {"width", data.videoFrameWidth}, {"height", data.videoFrameHeight},
        {"frameRate", data.videoFrameRate}, {"quality", data.qualityLevel}};

    bool ret = false;
    if (capabilityStringMap.find(capabilityKey) != capabilityStringMap.end()) {
        ret = SetCapabilityStringData(capabilityStringMap, capabilityKey, capabilityValue);
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetCapabilityStringData failed");
    } else if (capabilityIntMap.find(capabilityKey) != capabilityIntMap.end()) {
        ret = SetCapabilityIntData(capabilityIntMap, capabilityKey, capabilityValue);
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetCapabilityIntData failed");
    } else {
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "can not find capabilityKey: %{public}s", capabilityKey.c_str());
    }
    MEDIA_LOGD("RecorderProfilesXmlParser::SetVideoRecorderProfiles end");
    return true;
}

bool RecorderProfilesXmlParser::SetAudioRecorderProfiles(RecorderProfilesData &data, const std::string &capabilityKey,
                                                         const std::string &capabilityValue)
{
    std::unordered_map<std::string, std::string&> capabilityStringMap = {
        {"format", data.containerFormatType}, {"codecMime", data.audioCodec}};

    std::unordered_map<std::string, int32_t&> capabilityIntMap = {
        {"bitrate", data.audioBitrate}, {"sampleRate", data.audioSampleRate},
        {"channels", data.audioChannels}};

    bool ret = false;
    if (capabilityStringMap.find(capabilityKey) != capabilityStringMap.end()) {
        ret = SetCapabilityStringData(capabilityStringMap, capabilityKey, capabilityValue);
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetCapabilityStringData failed");
    } else if (capabilityIntMap.find(capabilityKey) != capabilityIntMap.end()) {
        ret = SetCapabilityIntData(capabilityIntMap, capabilityKey, capabilityValue);
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "SetCapabilityIntData failed");
    } else {
        CHECK_AND_RETURN_RET_LOG(ret != false, false, "can not find capabilityKey: %{public}s", capabilityKey.c_str());
    }
    MEDIA_LOGD("RecorderProfilesXmlParser::SetAudioRecorderProfiles end");
    return true;
}

RecorderProfilesNodeName RecorderProfilesXmlParser::GetNodeNameAsInt(xmlNode *node)
{
    if (!xmlStrcmp(node->name, reinterpret_cast<const xmlChar*>("RecorderConfigurations"))) {
        return RECORDER_CONFIGURATIONS;
    } else if (!xmlStrcmp(node->name, reinterpret_cast<const xmlChar*>("RecorderCaps"))) {
        return RECORDER_CAPS;
    } else if (!xmlStrcmp(node->name, reinterpret_cast<const xmlChar*>("RecorderProfiles"))) {
        return RECORDER_PROFILES;
    } else {
        return UNKNOWN;
    }
}

std::vector<RecorderProfilesData> RecorderProfilesXmlParser::GetRecorderProfileDataArray()
{
    return this->capabilityDataArray_;
}
}  // namespace Media
}  // namespace OHOS