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

#include "player_track_parse.h"
#include "media_log.h"
#include "media_errors.h"
#include "av_common.h"
#include "gst_utils.h"
#include "gst_meta_parser.h"
#include "player.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerTrackParse"};
}

namespace OHOS {
namespace Media {
inline constexpr std::string_view INNER_META_KEY_TRACK_VALID = "track_valid";
inline constexpr std::string_view INNER_META_KEY_TRACK_INNER_INDEX = "track_inner_index";

static const std::unordered_map<std::string_view, std::string_view> INNER_KEY_TO_PLAYER_KEY = {
    { INNER_META_KEY_BITRATE, PlayerKeys::PLAYER_BITRATE },
    { INNER_META_KEY_CHANNEL_COUNT, PlayerKeys::PLAYER_CHANNELS },
    { INNER_META_KEY_FRAMERATE, PlayerKeys::PLAYER_FRAMERATE },
    { INNER_META_KEY_VIDEO_HEIGHT, PlayerKeys::PLAYER_HEIGHT },
    { INNER_META_KEY_LANGUAGE, PlayerKeys::PLAYER_LANGUGAE },
    { INNER_META_KEY_MIME_TYPE, PlayerKeys::PLAYER_MIME },
    { INNER_META_KEY_SAMPLE_RATE, PlayerKeys::PLAYER_SAMPLE_RATE },
    { INNER_META_KEY_TRACK_INDEX, PlayerKeys::PLAYER_TRACK_INDEX },
    { INNER_META_KEY_TRACK_TYPE, PlayerKeys::PLAYER_TRACK_TYPE },
    { INNER_META_KEY_VIDEO_WIDTH, PlayerKeys::PLAYER_WIDTH },
};

std::shared_ptr<PlayerTrackParse> PlayerTrackParse::Create()
{
    std::shared_ptr<PlayerTrackParse> trackInfo = std::make_shared<PlayerTrackParse>();
    return trackInfo;
}

PlayerTrackParse::PlayerTrackParse()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerTrackParse::~PlayerTrackParse()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void PlayerTrackParse::OnElementSetup(GstElement &elem)
{
    const gchar *metadata = gst_element_get_metadata(&elem, GST_ELEMENT_METADATA_KLASS);
    CHECK_AND_RETURN_LOG(metadata != nullptr, "gst_element_get_metadata return nullptr");
    std::string elementName(GST_ELEMENT_NAME(&elem));

    std::string metaStr(metadata);
    if (metaStr.find("Codec/Demuxer") != std::string::npos && elementName.find("hlsdemux") == std::string::npos) {
        // Collect trackinfo information
        SetUpDemuxerElementCb(elem);
        DemuxInfo demux;
        demuxMap_.insert(std::pair<GstElement *, DemuxInfo>(&elem, demux));
    }

    if (metaStr.find("Codec/Parser") != std::string::npos) {
        // HLS stream demux has no width and height data, and requires prase as a supplement
        SetUpDemuxerElementCb(elem);
    }

    if (elementName.find("inputselector") != std::string::npos) {
        // Detect resolution switching events
        SetUpInputSelectElementCb(elem);
        inputSelectSet_.emplace(&elem);
    }

    if (elementName.find("uridecodebin") != std::string::npos) {
        // Detection does not support caps(pads) events
        gulong signalId = g_signal_connect(&elem, "unknown-type", G_CALLBACK(&PlayerTrackParse::UnknownType), this);
        CHECK_AND_RETURN_LOG(signalId != 0, "listen to pad-added failed");
        (void)signalIds_.emplace_back(SignalInfo { &elem, signalId });
    }
}

void PlayerTrackParse::OnElementUnSetup(GstElement &elem)
{
    const gchar *metadata = gst_element_get_metadata(&elem, GST_ELEMENT_METADATA_KLASS);
    CHECK_AND_RETURN_LOG(metadata != nullptr, "gst_element_get_metadata return nullptr");
    std::string elementName(GST_ELEMENT_NAME(&elem));
    std::string metaStr(metadata);

    if (metaStr.find("Codec/Demuxer") != std::string::npos) {
        MEDIA_LOGI("UnSetUp Demuxer elem %{public}s", ELEM_NAME(&elem));
        auto it = demuxMap_.find(&elem);
        if (it != demuxMap_.end()) {
            demuxMap_.erase(it);
        }

        if (currentDemux_ == &elem) {
            currentDemux_ = nullptr;
            MEDIA_LOGI("remove currentDemux_");
        }
    }

    if (elementName.find("inputselector") != std::string::npos) {
        MEDIA_LOGI("UnSetUp inputselector elem %{public}s", ELEM_NAME(&elem));
        auto it = inputSelectSet_.find(&elem);
        if (it != inputSelectSet_.end()) {
            inputSelectSet_.erase(it);
        }

        if (inputSelectSet_.empty()) {
            currentDemux_ = nullptr;
            MEDIA_LOGI("remove currentDemux_");
        }
    }
}

void PlayerTrackParse::SetUpInputSelectElementCb(GstElement &elem)
{
    MEDIA_LOGD("SetUpInputSelectElementCb elem %{public}s", ELEM_NAME(&elem));
    {
        std::unique_lock<std::mutex> lock(signalIdMutex_);
        gulong signalId = g_signal_connect(&elem, "pad-added",
            G_CALLBACK(PlayerTrackParse::OnInputSelectPadAddedCb), this);
        CHECK_AND_RETURN_LOG(signalId != 0, "listen to pad-added failed");
        (void)signalIds_.emplace_back(SignalInfo { &elem, signalId });
    }
}

void PlayerTrackParse::OnInputSelectPadAddedCb(const GstElement *element, GstPad *pad, gpointer userData)
{
    CHECK_AND_RETURN_LOG(element != nullptr && pad != nullptr && userData != nullptr, "param is nullptr");
    
    auto playerTrackParse = reinterpret_cast<PlayerTrackParse *>(userData);
    (void)playerTrackParse->InputSelectAddProbeToPad(element, pad);
}

bool PlayerTrackParse::InputSelectAddProbeToPad(const GstElement *element, GstPad *pad)
{
    MEDIA_LOGD("AddProbeToPad element %{public}s, pad %{public}s", ELEM_NAME(element), PAD_NAME(pad));
    std::unique_lock<std::mutex> lock(padProbeMutex_);
    gulong probeId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
        InputSelectProbeCallback, this, nullptr);
    CHECK_AND_RETURN_RET_LOG(probeId != 0, false, "add probe for %{public}s's pad %{public}s failed",
        GST_ELEMENT_NAME(GST_PAD_PARENT(pad)), PAD_NAME(pad));
    (void)padProbes_.emplace(pad, probeId);
    gst_object_ref(pad);
    return true;
}

GstPadProbeReturn PlayerTrackParse::InputSelectProbeCallback(GstPad *pad, GstPadProbeInfo *info, gpointer userData)
{
    CHECK_AND_RETURN_RET_LOG(pad != nullptr && info != nullptr && userData != nullptr,
        GST_PAD_PROBE_OK, "param is nullptr");

    auto playerTrackParse = reinterpret_cast<PlayerTrackParse *>(userData);
    return playerTrackParse->CheckDemux(pad, info);
}

GstPadProbeReturn PlayerTrackParse::CheckDemux(GstPad *pad, GstPadProbeInfo *info)
{
    (void)pad;
    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    if (currentDemux_ != nullptr) {
        return GST_PAD_PROBE_OK;
    }

    if ((static_cast<unsigned int>(info->type) & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) == 0) {
        return GST_PAD_PROBE_OK;
    }

    GstEvent *event = gst_pad_probe_info_get_event(info);
    CHECK_AND_RETURN_RET_LOG(event != nullptr, GST_PAD_PROBE_OK, "event is null");
    if (GST_EVENT_TYPE(event) != GST_EVENT_STREAM_START) {
        return GST_PAD_PROBE_OK;
    }

    const gchar *current_stream_id;
    gst_event_parse_stream_start (event, &current_stream_id);

    for (auto demuxIt = demuxMap_.begin(); demuxIt != demuxMap_.end(); demuxIt++) {
        for (auto padIt = demuxIt->second.trackInfos.begin(); padIt != demuxIt->second.trackInfos.end(); padIt++) {
            gchar *stream_id = gst_pad_get_stream_id (padIt->first);
            if (strcmp (current_stream_id, stream_id) == 0) {
                currentDemux_ = demuxIt->first;
                MEDIA_LOGD("Matched to a valid demux 0x%{public}06" PRIXPTR ", streamid:%{public}s",
                    FAKE_POINTER(padIt->first), stream_id);
                UpdateTrackInfo();
                g_free (stream_id);
                return GST_PAD_PROBE_OK;
            }
            g_free (stream_id);
        }
    }

    return GST_PAD_PROBE_OK;
}

void PlayerTrackParse::UnknownType(const GstElement *element, GstPad *pad, GstCaps *caps, gpointer userData)
{
    CHECK_AND_RETURN_LOG(userData != nullptr, "param is invalid");

    auto playerTrackParse = reinterpret_cast<PlayerTrackParse *>(userData);
    return playerTrackParse->OnUnknownType(element, pad, caps);
}

void PlayerTrackParse::OnUnknownType(const GstElement *element, GstPad *pad, GstCaps *caps)
{
    (void)caps;
    CHECK_AND_RETURN_LOG(element != nullptr && pad != nullptr, "param is invalid");

    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    for (auto demuxIt = demuxMap_.begin(); demuxIt != demuxMap_.end(); demuxIt++) {
        auto padIt = demuxIt->second.trackInfos.find(pad);
        if (padIt != demuxIt->second.trackInfos.end()) {
            padIt->second.PutIntValue(std::string(INNER_META_KEY_TRACK_VALID), false);
            MEDIA_LOGD("OnUnknownType element %{public}s, pad %{public}s:0x%{public}06" PRIXPTR,
                ELEM_NAME(element), PAD_NAME(pad), FAKE_POINTER(pad));
            return;
        }
    }
}

bool PlayerTrackParse::IsSameStreamId(GstPad *padA, GstPad *padB)
{
    gchar *streamIdA;
    gchar *streamIdB;
    
    streamIdA = gst_pad_get_stream_id (padA);
    streamIdB = gst_pad_get_stream_id (padB);
    bool ret = (strcmp (streamIdA, streamIdB) == 0);
    g_free (streamIdA);
    g_free (streamIdB);
    return ret;
}

int32_t PlayerTrackParse::GetTrackInfo(int32_t index, int32_t &innerIndex, int32_t &trackType)
{
    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    CHECK_AND_RETURN_RET_LOG(currentDemux_ != nullptr, MSERR_INVALID_OPERATION, "Plugin not found");
    auto demuxIt = demuxMap_.find(currentDemux_);
    CHECK_AND_RETURN_RET(demuxIt != demuxMap_.end(), MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(index >= 0 && index < static_cast<int32_t>(demuxIt->second.trackInfos.size()),
        MSERR_INVALID_VAL, "Invalid index %{public}d", index);

    StartUpdateTrackInfo();

    for (int32_t i = 0; i < static_cast<int32_t>(videoTracks_.size()); i++) {
        int32_t trackIndex = -1;
        videoTracks_[i].GetIntValue(std::string(PlayerKeys::PLAYER_TRACK_INDEX), trackIndex);
        if (trackIndex == index) {
            trackType = MediaType::MEDIA_TYPE_VID;
            videoTracks_[i].GetIntValue(std::string(INNER_META_KEY_TRACK_INNER_INDEX), innerIndex);
            MEDIA_LOGI("index:0x%{public}d inner:0x%{public}d Type:0x%{public}d", index, innerIndex, trackType);
            return MSERR_OK;
        }
    }

    for (int32_t i = 0; i < static_cast<int32_t>(audioTracks_.size()); i++) {
        int32_t trackIndex = -1;
        audioTracks_[i].GetIntValue(std::string(PlayerKeys::PLAYER_TRACK_INDEX), trackIndex);
        if (trackIndex == index) {
            trackType = MediaType::MEDIA_TYPE_AUD;
            audioTracks_[i].GetIntValue(std::string(INNER_META_KEY_TRACK_INNER_INDEX), innerIndex);
            MEDIA_LOGI("index:0x%{public}d inner:0x%{public}d Type:0x%{public}d", index, innerIndex, trackType);
            return MSERR_OK;
        }
    }
    
    return MSERR_INVALID_VAL;
}

int32_t PlayerTrackParse::GetTrackIndex(int32_t innerIndex, int32_t trackType, int32_t &index)
{
    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    CHECK_AND_RETURN_RET_LOG(currentDemux_ != nullptr, MSERR_INVALID_OPERATION, "Plugin not found");
    CHECK_AND_RETURN_RET_LOG(trackType >= MediaType::MEDIA_TYPE_AUD && trackType <= MediaType::MEDIA_TYPE_SUBTITLE,
        MSERR_INVALID_VAL, "Invalid trackType %{public}d", trackType);
    CHECK_AND_RETURN_RET_LOG(innerIndex >= 0, MSERR_INVALID_VAL, "Invalid innerIndex %{public}d", innerIndex);

    StartUpdateTrackInfo();

    if (trackType == MediaType::MEDIA_TYPE_AUD) {
        for (int32_t i = 0; i < static_cast<int32_t>(audioTracks_.size()); i++) {
            int32_t trackInnerIndex = -1;
            audioTracks_[i].GetIntValue(std::string(INNER_META_KEY_TRACK_INNER_INDEX), trackInnerIndex);
            if (trackInnerIndex == innerIndex) {
                audioTracks_[i].GetIntValue(std::string(PlayerKeys::PLAYER_TRACK_INDEX), index);
                MEDIA_LOGI("inner:0x%{public}d Type:0x%{public}d index:0x%{public}d", innerIndex, trackType, index);
                return MSERR_OK;
            }
        }
    } else if (trackType == MediaType::MEDIA_TYPE_VID) {
        for (int32_t i = 0; i < static_cast<int32_t>(videoTracks_.size()); i++) {
            int32_t trackInnerIndex = -1;
            videoTracks_[i].GetIntValue(std::string(INNER_META_KEY_TRACK_INNER_INDEX), trackInnerIndex);
            if (trackInnerIndex == innerIndex) {
                videoTracks_[i].GetIntValue(std::string(PlayerKeys::PLAYER_TRACK_INDEX), index);
                MEDIA_LOGI("inner:0x%{public}d Type:0x%{public}d index:0x%{public}d", innerIndex, trackType, index);
                return MSERR_OK;
            }
        }
    } else {
        index = innerIndex + videoTracks_.size() + audioTracks_.size();
    }
    
    return MSERR_OK;
}

void PlayerTrackParse::StartUpdateTrackInfo()
{
    if (!updateTrackInfo_) {
        updateTrackInfo_ = true;
        UpdateTrackInfo();
    }
}

void PlayerTrackParse::UpdateTrackInfo()
{
    if (!updateTrackInfo_) {
        return;
    }
    videoTracks_.clear();
    audioTracks_.clear();
    CHECK_AND_RETURN(currentDemux_ != nullptr);

    auto demuxIt = demuxMap_.find(currentDemux_);
    CHECK_AND_RETURN(demuxIt != demuxMap_.end());

    int32_t index;
    std::map<int32_t, Format> tracks;
    for (auto &[pad, innerMeta] : demuxIt->second.trackInfos) {
        // Sort trackinfo by index
        innerMeta.GetIntValue(std::string(INNER_META_KEY_TRACK_INDEX), index);
        (void)tracks.emplace(index, innerMeta);
    }

    int32_t audioCount = 0;
    int32_t videoCount = 0;
    int32_t trackType = MediaType::MEDIA_TYPE_AUD;
    int32_t valid = 0;
    for (auto &[ind, innerMeta] : tracks) {
        Format outMeta;
        innerMeta.GetIntValue(INNER_META_KEY_TRACK_TYPE, trackType);
        ConvertToPlayerKeys(innerMeta, outMeta);
        innerMeta.GetIntValue(std::string(INNER_META_KEY_TRACK_VALID), valid);
        if (valid == 0) {
            outMeta.PutIntValue(std::string(INNER_META_KEY_TRACK_INNER_INDEX), -1);
        }
        if (trackType == MediaType::MEDIA_TYPE_VID) {
            if (valid != 0) {
                outMeta.PutIntValue(std::string(INNER_META_KEY_TRACK_INNER_INDEX), videoCount);
                videoCount++;
            }
            videoTracks_.emplace_back(outMeta);
        } else if (trackType == MediaType::MEDIA_TYPE_AUD) {
            if (valid != 0) {
                outMeta.PutIntValue(std::string(INNER_META_KEY_TRACK_INNER_INDEX), audioCount);
                audioCount++;
            }
            audioTracks_.emplace_back(outMeta);
        }
    }
    return;
}

int32_t PlayerTrackParse::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    CHECK_AND_RETURN_RET_LOG(currentDemux_ != nullptr, MSERR_INVALID_OPERATION, "Plugin not found");
    StartUpdateTrackInfo();
    videoTrack.assign(videoTracks_.begin(), videoTracks_.end());
    for (int32_t i = 0; i < static_cast<int32_t>(videoTracks_.size()); i++) {
        videoTrack[i].RemoveKey(std::string(INNER_META_KEY_TRACK_INNER_INDEX));
    }
    return MSERR_OK;
}

int32_t PlayerTrackParse::GetAudioTrackInfo(std::vector<Format> &audioTrack)
{
    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    CHECK_AND_RETURN_RET_LOG(currentDemux_ != nullptr, MSERR_INVALID_OPERATION, "Plugin not found");
    StartUpdateTrackInfo();
    audioTrack.assign(audioTracks_.begin(), audioTracks_.end());
    for (int32_t i = 0; i < static_cast<int32_t>(audioTrack.size()); i++) {
        audioTrack[i].RemoveKey(std::string(INNER_META_KEY_TRACK_INNER_INDEX));
    }
    return MSERR_OK;
}

void PlayerTrackParse::ConvertToPlayerKeys(const Format &innerMeta, Format &outMeta) const
{
    for (const auto &[innerKey, playerKey] : INNER_KEY_TO_PLAYER_KEY) {
        if (!innerMeta.ContainKey(innerKey)) {
            continue;
        }

        std::string strVal;
        int32_t intVal;
        double douVal;
        FormatDataType type = innerMeta.GetValueType(innerKey);
        switch (type) {
            case FORMAT_TYPE_STRING:
                innerMeta.GetStringValue(innerKey, strVal);
                outMeta.PutStringValue(std::string(playerKey), strVal);
                break;
            case FORMAT_TYPE_INT32:
                innerMeta.GetIntValue(innerKey, intVal);
                outMeta.PutIntValue(std::string(playerKey), intVal);
                break;
            case FORMAT_TYPE_DOUBLE:
                innerMeta.GetDoubleValue(innerKey, douVal);
                outMeta.PutDoubleValue(std::string(playerKey), douVal);
                break;
            default:
                break;
        }
    }
}

GstPadProbeReturn PlayerTrackParse::ProbeCallback(GstPad *pad, GstPadProbeInfo *info, gpointer userData)
{
    if (pad == nullptr || info ==  nullptr || userData == nullptr) {
        MEDIA_LOGE("param is invalid");
        return GST_PAD_PROBE_OK;
    }

    auto playerTrackParse = reinterpret_cast<PlayerTrackParse *>(userData);
    return playerTrackParse->GetTrackParse(pad, info);
}

GstPadProbeReturn PlayerTrackParse::GetTrackParse(GstPad *pad, GstPadProbeInfo *info)
{
    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    bool isParsePad = (parsePadSet_.count(pad) != 0);
    if (!isParsePad) {
        for (auto demuxIt = demuxMap_.begin(); demuxIt != demuxMap_.end(); demuxIt++) {
            auto padIt = demuxIt->second.trackInfos.find(pad);
            if (padIt != demuxIt->second.trackInfos.end()) {
                return ParseTrackInfo(pad, info, padIt->second);
            }
        }
        return GST_PAD_PROBE_OK;
    }

    for (auto demuxIt = demuxMap_.begin(); demuxIt != demuxMap_.end(); demuxIt++) {
        for (auto padIt = demuxIt->second.trackInfos.begin(); padIt != demuxIt->second.trackInfos.end(); padIt++) {
            if (IsSameStreamId(pad, padIt->first)) {
                return ParseTrackInfo(pad, info, padIt->second);
            }
        }
    }
    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn PlayerTrackParse::ParseTrackInfo(GstPad *pad, GstPadProbeInfo *info, Format &format)
{
    if (static_cast<unsigned int>(info->type) & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) {
        GstEvent *event = gst_pad_probe_info_get_event(info);
        CHECK_AND_RETURN_RET_LOG(event != nullptr, GST_PAD_PROBE_OK, "event is null");

        if (GST_EVENT_TYPE(event) == GST_EVENT_TAG) {
            GstTagList *tagList = nullptr;
            gst_event_parse_tag(event, &tagList);
            CHECK_AND_RETURN_RET_LOG(tagList != nullptr, GST_PAD_PROBE_OK, "tags is nullptr");
            GstMetaParser::ParseTagList(*tagList, format);
            MEDIA_LOGI("catch tags at pad %{public}s:0x%{public}06" PRIXPTR, PAD_NAME(pad), FAKE_POINTER(pad));
        } else if (GST_EVENT_TYPE(event) == GST_EVENT_CAPS) {
            GstCaps *caps = nullptr;
            gst_event_parse_caps(event, &caps);
            CHECK_AND_RETURN_RET_LOG(caps != nullptr, GST_PAD_PROBE_OK, "caps is nullptr");
            GstMetaParser::ParseStreamCaps(*caps, format);
            MEDIA_LOGI("catch caps at pad %{public}s:0x%{public}06" PRIXPTR, PAD_NAME(pad), FAKE_POINTER(pad));
        }
        (void)UpdateTrackInfo();
    }
    return GST_PAD_PROBE_OK;
}

bool PlayerTrackParse::AddProbeToPad(const GstElement *element, GstPad *pad)
{
    MEDIA_LOGD("AddProbeToPad element %{public}s, pad %{public}s", ELEM_NAME(element), PAD_NAME(pad));
    {
        std::unique_lock<std::mutex> lock(padProbeMutex_);
        gulong probeId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, ProbeCallback, this, nullptr);
        if (probeId == 0) {
            MEDIA_LOGE("add probe for %{public}s's pad %{public}s failed",
                GST_ELEMENT_NAME(GST_PAD_PARENT(pad)), PAD_NAME(pad));
            return false;
        }
        (void)padProbes_.emplace(pad, probeId);
        gst_object_ref(pad);
    }
    {
        std::unique_lock<std::mutex> lock(trackInfoMutex_);

        const gchar *metadata = gst_element_get_metadata(const_cast<GstElement *>(element), GST_ELEMENT_METADATA_KLASS);
        CHECK_AND_RETURN_RET_LOG(metadata != nullptr, true, "gst_element_get_metadata return nullptr");
        std::string metaStr(metadata);
        if (metaStr.find("Codec/Parser") != std::string::npos) {
            parsePadSet_.insert(pad);
            return true;
        }

        auto demuxIt = demuxMap_.find(const_cast<GstElement *>(element));
        if (demuxIt != demuxMap_.end()) {
            // The order of pad creation is consistent with the index of "current-audio"/"current-text"
            Format innerMeta;
            innerMeta.PutIntValue(std::string(INNER_META_KEY_TRACK_INDEX), demuxIt->second.trackcount);
            innerMeta.PutIntValue(std::string(INNER_META_KEY_TRACK_VALID), true);
            demuxIt->second.trackcount++;
            MEDIA_LOGI("demux:0x%{public}06" PRIXPTR " trackcount:0x%{public}d",
                FAKE_POINTER(element), demuxIt->second.trackcount);
            (void)demuxIt->second.trackInfos.emplace(pad, innerMeta);
        }
    }

    return true;
}

bool PlayerTrackParse::AddProbeToPadList(const GstElement *element, GList &list)
{
    MEDIA_LOGD("AddProbeToPadList element %{public}s", ELEM_NAME(element));
    for (GList *padNode = g_list_first(&list); padNode != nullptr; padNode = padNode->next) {
        if (padNode->data == nullptr) {
            continue;
        }

        GstPad *pad = reinterpret_cast<GstPad *>(padNode->data);
        if (!AddProbeToPad(element, pad)) {
            return false;
        }
    }

    return true;
}

void PlayerTrackParse::OnPadAddedCb(const GstElement *element, GstPad *pad, gpointer userData)
{
    if (element == nullptr || pad ==  nullptr || userData == nullptr) {
        MEDIA_LOGE("param is nullptr");
        return;
    }

    auto playerTrackParse = reinterpret_cast<PlayerTrackParse *>(userData);
    (void)playerTrackParse->AddProbeToPad(element, pad);
}

bool PlayerTrackParse::GetDemuxerElementFind() const
{
    return currentDemux_ != nullptr;
}

void PlayerTrackParse::SetUpDemuxerElementCb(GstElement &elem)
{
    MEDIA_LOGD("SetUpDemuxerElementCb elem %{public}s", ELEM_NAME(&elem));
    if (!AddProbeToPadList(&elem, *elem.srcpads)) {
        return;
    }
    {
        std::unique_lock<std::mutex> lock(signalIdMutex_);
        gulong signalId = g_signal_connect(&elem, "pad-added", G_CALLBACK(PlayerTrackParse::OnPadAddedCb), this);
        CHECK_AND_RETURN_LOG(signalId != 0, "listen to pad-added failed");
        (void)signalIds_.emplace_back(SignalInfo { &elem, signalId });
    }
}

void PlayerTrackParse::Stop()
{
    MEDIA_LOGD("Stop");
    {
        std::unique_lock<std::mutex> lock(trackInfoMutex_);
        videoTracks_.clear();
        audioTracks_.clear();
        updateTrackInfo_ = false;
        currentDemux_ = nullptr;
        inputSelectSet_.clear();
        parsePadSet_.clear();
        demuxMap_.clear();
    }
    {
        std::unique_lock<std::mutex> lock(padProbeMutex_);
        // PlayerTrackParse::ProbeCallback
        for (auto &[pad, probeId] : padProbes_) {
            MEDIA_LOGD("remove_probe pad %{public}s", PAD_NAME(pad));
            gst_pad_remove_probe(pad, probeId);
            gst_object_unref(pad);
        }
        padProbes_.clear();
    }
    {
        std::unique_lock<std::mutex> lock(signalIdMutex_);
        // PlayerTrackParse::OnPadAddedCb
        for (auto &item : signalIds_) {
            g_signal_handler_disconnect(item.element, item.signalId);
        }
        signalIds_.clear();
    }
}
}
}
