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
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerTrackParse"};
}

namespace OHOS {
namespace Media {
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
    { INNER_META_KEY_TRACK_INNER_INDEX, INNER_META_KEY_TRACK_INNER_INDEX },
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
    CHECK_AND_RETURN_LOG(isStopping_.load() == false, "playbin is stopping");
    const gchar *metadata = gst_element_get_metadata(&elem, GST_ELEMENT_METADATA_KLASS);
    CHECK_AND_RETURN_LOG(metadata != nullptr, "gst_element_get_metadata return nullptr");
    std::string elementName(GST_ELEMENT_NAME(&elem));

    std::string metaStr(metadata);
    if (metaStr.find("Codec/Demuxer") != std::string::npos && elementName.find("hlsdemux") == std::string::npos) {
        // Collect trackinfo information
        std::unique_lock<std::mutex> lock(trackInfoMutex_);
        DemuxInfo demux(&elem);
        trackVec_.push_back(demux);
        lock.unlock();
        SetUpDemuxerElementCb(elem);
    }

    if (metaStr.find("Codec/Parser") != std::string::npos) {
        // HLS stream demux has no width and height data, and requires prase as a supplement
        SetUpDemuxerElementCb(elem);
    }

    if (elementName.find("inputselector") != std::string::npos) {
        // Detect resolution switching events
        std::unique_lock<std::mutex> lock(trackInfoMutex_);
        InputSelectInfo info;
        inputSelectMap_.insert(std::pair<GstElement *, InputSelectInfo>(&elem, info));
        lock.unlock();
        SetUpInputSelectElementCb(elem);
    }
}

void PlayerTrackParse::OnElementUnSetup(GstElement &elem)
{
    const gchar *metadata = gst_element_get_metadata(&elem, GST_ELEMENT_METADATA_KLASS);
    CHECK_AND_RETURN_LOG(metadata != nullptr, "gst_element_get_metadata return nullptr");
    std::string elementName(GST_ELEMENT_NAME(&elem));
    std::string metaStr(metadata);

    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    if (metaStr.find("Codec/Demuxer") != std::string::npos) {
        MEDIA_LOGI("UnSetUp Demuxer elem %{public}s", ELEM_NAME(&elem));
        for (auto it = trackVec_.begin(); it != trackVec_.end(); it++) {
            if (it->demux == &elem) {
                trackVec_.erase(it);
                MEDIA_LOGI("remove demux from trackVec 0x%{public}06" PRIXPTR, FAKE_POINTER(&elem));
                break;
            }
        }
    }

    if (elementName.find("inputselector") != std::string::npos) {
        MEDIA_LOGI("UnSetUp inputselector elem %{public}s", ELEM_NAME(&elem));
        
        auto it = inputSelectMap_.find(&elem);
        if (it != inputSelectMap_.end()) {
            inputSelectMap_.erase(it);
        }

        if (inputSelectMap_.empty()) {
            findTrackInfo_ = false;
            MEDIA_LOGI("0x%{public}06" PRIXPTR " Remove all inputselect plugins", FAKE_POINTER(this));
        }
    }

    RemoveSignalIds(&elem);
}

void PlayerTrackParse::SetUpInputSelectElementCb(GstElement &elem)
{
    MEDIA_LOGD("SetUpInputSelectElementCb elem %{public}s", ELEM_NAME(&elem));
    {
        std::unique_lock<std::mutex> lock(signalIdMutex_);
        gulong signalId = g_signal_connect(&elem, "pad-added",
            G_CALLBACK(PlayerTrackParse::OnInputSelectPadAddedCb), this);
        CHECK_AND_RETURN_LOG(signalId != 0, "listen to pad-added failed");
        AddSignalIds(&elem, signalId);
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
    {
        std::unique_lock<std::mutex> lock(trackInfoMutex_);
        auto it = inputSelectMap_.find(const_cast<GstElement *>(element));
        CHECK_AND_RETURN_RET_LOG(it != inputSelectMap_.end(), false, "Unregistered elements");
        it->second.padIndexMap[pad] = it->second.padCount;
        it->second.padCount++;
        MEDIA_LOGD("AddProbeToPad element %{public}s, pad %{public}s, count %{public}d",
            ELEM_NAME(element), PAD_NAME(pad), it->second.padCount);
    }

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
    return playerTrackParse->GetUsedDemux(pad, info);
}

GstPadProbeReturn PlayerTrackParse::GetUsedDemux(GstPad *pad, GstPadProbeInfo *info)
{
    (void)pad;
    CHECK_AND_RETURN_RET((static_cast<unsigned int>(info->type) & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) != 0,
        GST_PAD_PROBE_OK);

    GstEvent *event = gst_pad_probe_info_get_event(info);
    CHECK_AND_RETURN_RET_LOG(event != nullptr, GST_PAD_PROBE_OK, "event is null");
    if (GST_EVENT_TYPE(event) != GST_EVENT_STREAM_START) {
        return GST_PAD_PROBE_OK;
    }

    const gchar *current_stream_id;
    gst_event_parse_stream_start (event, &current_stream_id);
    CHECK_AND_RETURN_RET_LOG(current_stream_id != nullptr, GST_PAD_PROBE_OK, "current_stream_id is nullptr");

    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    for (int32_t i = 0; i < static_cast<int32_t>(trackVec_.size()); i++) {
        for (auto padIt = trackVec_[i].trackInfos.begin(); padIt != trackVec_[i].trackInfos.end(); padIt++) {
            gchar *stream_id = gst_pad_get_stream_id(padIt->first);
            CHECK_AND_CONTINUE(stream_id != nullptr && current_stream_id != nullptr);
            if (strcmp(current_stream_id, stream_id) == 0) {
                findTrackInfo_ = true;
                trackVec_[i].inUse = true;
                int32_t index = GetInputSelectPadIndex(pad);
                padIt->second.PutIntValue(std::string(INNER_META_KEY_TRACK_INNER_INDEX), index);
                MEDIA_LOGD("Matched to pad:0x%{public}06" PRIXPTR ", index:%{public}d, streamid:%{public}s",
                    FAKE_POINTER(padIt->first), index, stream_id);
                UpdateTrackInfo();
                g_free (stream_id);
                return GST_PAD_PROBE_OK;
            }
            g_free (stream_id);
        }
    }

    return GST_PAD_PROBE_OK;
}

int32_t PlayerTrackParse::GetInputSelectPadIndex(GstPad *pad)
{
    CHECK_AND_RETURN_RET(pad != nullptr, -1);
    int32_t index = -1;
    for (auto inputIt = inputSelectMap_.begin(); inputIt != inputSelectMap_.end(); inputIt++) {
        auto inputPadIt = inputIt ->second.padIndexMap.find(pad);
        if (inputPadIt != inputIt ->second.padIndexMap.end()) {
            index = inputPadIt->second;
            break;
        }
    }
    return index;
}

bool PlayerTrackParse::IsSameStreamId(GstPad *padA, GstPad *padB)
{
    gchar *streamIdA = gst_pad_get_stream_id(padA);
    CHECK_AND_RETURN_RET_LOG(streamIdA != nullptr, false, "streamIdA is nullptr");
    ON_SCOPE_EXIT(0) { g_free(streamIdA); };
    gchar *streamIdB = gst_pad_get_stream_id(padB);
    CHECK_AND_RETURN_RET_LOG(streamIdB != nullptr, false, "streamIdB is nullptr");
    CANCEL_SCOPE_EXIT_GUARD(0);

    bool ret = (strcmp(streamIdA, streamIdB) == 0);
    g_free(streamIdA);
    g_free(streamIdB);
    return ret;
}

bool PlayerTrackParse::HasSameStreamIdInDemux(GstPad *pad)
{
    for (int32_t i = 0; i < static_cast<int32_t>(trackVec_.size()); i++) {
        for (auto padIt = trackVec_[i].trackInfos.begin(); padIt != trackVec_[i].trackInfos.end(); padIt++) {
            CHECK_AND_RETURN_RET(!IsSameStreamId(pad, padIt->first), true);
        }
    }

    return false;
}

int32_t PlayerTrackParse::GetTrackInfo(int32_t index, int32_t &innerIndex, int32_t &trackType)
{
    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    CHECK_AND_RETURN_RET_LOG(findTrackInfo_, MSERR_INVALID_OPERATION, "trackinfo not found");
    CHECK_AND_RETURN_RET_LOG(index >= 0, MSERR_INVALID_VAL, "Invalid index %{public}d", index);

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

    for (int32_t i = 0; i < static_cast<int32_t>(subtitleTracks_.size()); i++) {
        int32_t trackIndex = -1;
        subtitleTracks_[i].GetIntValue(std::string(PlayerKeys::PLAYER_TRACK_INDEX), trackIndex);
        if (trackIndex == index) {
            trackType = MediaType::MEDIA_TYPE_SUBTITLE;
            subtitleTracks_[i].GetIntValue(std::string(INNER_META_KEY_TRACK_INNER_INDEX), innerIndex);
            MEDIA_LOGI("index:0x%{public}d inner:0x%{public}d Type:0x%{public}d", index, innerIndex, trackType);
            return MSERR_OK;
        }
    }

    return MSERR_INVALID_VAL;
}

int32_t PlayerTrackParse::GetTrackIndex(int32_t innerIndex, int32_t trackType, int32_t &index)
{
    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    CHECK_AND_RETURN_RET_LOG(findTrackInfo_, MSERR_INVALID_OPERATION, "trackinfo not found");
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
        index = static_cast<size_t>(innerIndex) + videoTracks_.size() + audioTracks_.size();
        MEDIA_LOGI("inner:0x%{public}d Type:0x%{public}d index:0x%{public}d", innerIndex, trackType, index);
        return MSERR_OK;
    }

    return MSERR_INVALID_VAL;
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
    CHECK_AND_RETURN(findTrackInfo_);
    videoTracks_.clear();
    audioTracks_.clear();
    subtitleTracks_.clear();

    int32_t index;
    int32_t baseIndex = 0;
    std::map<int32_t, Format> tracks;
    for (int32_t i = 0; i < static_cast<int32_t>(trackVec_.size()); i++) {
        if (!trackVec_[i].inUse) {
            continue;
        }
        for (auto &[pad, innerMeta] : trackVec_[i].trackInfos) {
            // Sort trackinfo by index
            innerMeta.GetIntValue(std::string(INNER_META_KEY_TRACK_INDEX), index);
            (void)tracks.emplace(index + baseIndex, innerMeta);
        }
        baseIndex += trackVec_[i].trackcount;
    }

    int32_t trackType = MediaType::MEDIA_TYPE_AUD;
    for (auto &[ind, innerMeta] : tracks) {
        Format outMeta;
        innerMeta.GetIntValue(INNER_META_KEY_TRACK_TYPE, trackType);
        ConvertToPlayerKeys(innerMeta, outMeta);
        outMeta.PutIntValue(std::string(PlayerKeys::PLAYER_TRACK_INDEX), ind);
        if (trackType == MediaType::MEDIA_TYPE_VID) {
            videoTracks_.emplace_back(outMeta);
        } else if (trackType == MediaType::MEDIA_TYPE_AUD) {
            audioTracks_.emplace_back(outMeta);
        } else if (trackType == MediaType::MEDIA_TYPE_SUBTITLE) {
            subtitleTracks_.emplace_back(outMeta);
        }
    }
    return;
}

int32_t PlayerTrackParse::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    CHECK_AND_RETURN_RET_LOG(findTrackInfo_, MSERR_INVALID_OPERATION, "trackinfo not found");
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
    CHECK_AND_RETURN_RET_LOG(findTrackInfo_, MSERR_INVALID_OPERATION, "trackinfo not found");
    StartUpdateTrackInfo();
    audioTrack.assign(audioTracks_.begin(), audioTracks_.end());
    for (int32_t i = 0; i < static_cast<int32_t>(audioTrack.size()); i++) {
        audioTrack[i].RemoveKey(std::string(INNER_META_KEY_TRACK_INNER_INDEX));
    }
    return MSERR_OK;
}

int32_t PlayerTrackParse::GetSubtitleTrackInfo(std::vector<Format> &subtitleTrack)
{
    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    CHECK_AND_RETURN_RET_LOG(findTrackInfo_, MSERR_INVALID_OPERATION, "trackinfo not found");
    StartUpdateTrackInfo();
    subtitleTrack.assign(subtitleTracks_.begin(), subtitleTracks_.end());
    for (int32_t i = 0; i < static_cast<int32_t>(subtitleTrack.size()); i++) {
        subtitleTrack[i].RemoveKey(std::string(INNER_META_KEY_TRACK_INNER_INDEX));
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
    CHECK_AND_RETURN_RET_LOG(pad != nullptr && info != nullptr && userData != nullptr,
        GST_PAD_PROBE_OK, "param is invalid");

    auto playerTrackParse = reinterpret_cast<PlayerTrackParse *>(userData);
    return playerTrackParse->GetTrackParse(pad, info);
}

GstPadProbeReturn PlayerTrackParse::GetTrackParse(GstPad *pad, GstPadProbeInfo *info)
{
    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    bool isDemuxPad = (parsePadSet_.count(pad) == 0);
    for (int32_t i = 0; i < static_cast<int32_t>(trackVec_.size()); i++) {
        if (isDemuxPad) {
            auto padIt = trackVec_[i].trackInfos.find(pad);
            if (padIt != trackVec_[i].trackInfos.end()) {
                return ParseTrackInfo(pad, info, padIt->second);
            }
            continue;
        }

        for (auto padIt = trackVec_[i].trackInfos.begin(); padIt != trackVec_[i].trackInfos.end(); padIt++) {
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
            MEDIA_LOGD("catch tags at pad %{public}s:0x%{public}06" PRIXPTR, PAD_NAME(pad), FAKE_POINTER(pad));
            (void)UpdateTrackInfo();
        } else if (GST_EVENT_TYPE(event) == GST_EVENT_CAPS) {
            GstCaps *caps = nullptr;
            gst_event_parse_caps(event, &caps);
            CHECK_AND_RETURN_RET_LOG(caps != nullptr, GST_PAD_PROBE_OK, "caps is nullptr");
            GstMetaParser::ParseStreamCaps(*caps, format);
            MEDIA_LOGI("catch caps at pad %{public}s:0x%{public}06" PRIXPTR, PAD_NAME(pad), FAKE_POINTER(pad));
            (void)UpdateTrackInfo();
        }
    }
    return GST_PAD_PROBE_OK;
}

void PlayerTrackParse::ParseSubtitlePadCaps(const GstElement *element, GstPad *pad, int32_t index, Format &innerMeta)
{
    GstCaps *caps = gst_pad_query_caps(pad, nullptr);
    GstMetaParser::ParseStreamCaps(*caps, innerMeta);
    trackVec_[index].inUse = true;
    (void)trackVec_[index].trackInfos.emplace(pad, innerMeta);
    UpdateTrackInfo();
    MEDIA_LOGI("subtitle parse:0x%{public}06" PRIXPTR " trackcount:0x%{public}d pad:0x%{public}06" PRIXPTR,
        FAKE_POINTER(element), trackVec_[index].trackcount, FAKE_POINTER(pad));
}

bool PlayerTrackParse::AddProbeToPad(const GstElement *element, GstPad *pad)
{
    MEDIA_LOGD("AddProbeToPad element %{public}s, pad %{public}s", ELEM_NAME(element), PAD_NAME(pad));
    {
        std::unique_lock<std::mutex> lock(padProbeMutex_);
        gulong probeId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, ProbeCallback, this, nullptr);
        CHECK_AND_RETURN_RET_LOG(probeId != 0, false,
            "add probe for %{public}s's pad %{public}s failed", GST_ELEMENT_NAME(GST_PAD_PARENT(pad)), PAD_NAME(pad));
        (void)padProbes_.emplace(pad, probeId);
        gst_object_ref(pad);
    }
    {
        std::unique_lock<std::mutex> lock(trackInfoMutex_);

        const gchar *metadata = gst_element_get_metadata(const_cast<GstElement *>(element),
            GST_ELEMENT_METADATA_KLASS);
        CHECK_AND_RETURN_RET_LOG(metadata != nullptr, true, "gst_element_get_metadata return nullptr");
        std::string metaStr(metadata);
        if (metaStr.find("Codec/Parser/Subtitle") != std::string::npos && !HasSameStreamIdInDemux(pad)) {
            MEDIA_LOGD("external subtitle parser, handle it as demux");
            DemuxInfo demux(const_cast<GstElement *>(element));
            trackVec_.push_back(demux);
        } else if (metaStr.find("Codec/Parser") != std::string::npos) {
            parsePadSet_.insert(pad);
            MEDIA_LOGI("Parser pad:0x%{public}06" PRIXPTR, FAKE_POINTER(pad));
            return true;
        }

        for (int32_t i = 0; i < static_cast<int32_t>(trackVec_.size()); i++) {
            if (trackVec_[i].demux != element) {
                continue;
            }

            Format innerMeta;
            innerMeta.PutIntValue(std::string(INNER_META_KEY_TRACK_INNER_INDEX), -1);
            innerMeta.PutIntValue(std::string(INNER_META_KEY_TRACK_INDEX), trackVec_[i].trackcount);
            trackVec_[i].trackcount++;
            if (metaStr.find("Codec/Parser/Subtitle") != std::string::npos && !HasSameStreamIdInDemux(pad)) {
                ParseSubtitlePadCaps(element, pad, i, innerMeta);
                continue;
            }
            (void)trackVec_[i].trackInfos.emplace(pad, innerMeta);
            MEDIA_LOGI("demux:0x%{public}06" PRIXPTR " trackcount:0x%{public}d pad:0x%{public}06" PRIXPTR,
                FAKE_POINTER(element), trackVec_[i].trackcount, FAKE_POINTER(pad));
        }
    }

    return true;
}

bool PlayerTrackParse::AddProbeToPadList(GstElement *element, GList &list)
{
    MEDIA_LOGD("AddProbeToPadList element %{public}s", ELEM_NAME(element));
    for (GList *padNode = g_list_first(&list); padNode != nullptr; padNode = padNode->next) {
        CHECK_AND_CONTINUE(padNode->data != nullptr);

        GstPad *pad = reinterpret_cast<GstPad *>(padNode->data);
        CHECK_AND_RETURN_RET(AddProbeToPad(element, pad), false);
    }

    return true;
}

void PlayerTrackParse::OnPadAddedCb(const GstElement *element, GstPad *pad, gpointer userData)
{
    CHECK_AND_RETURN_LOG(element != nullptr && pad != nullptr && userData != nullptr, "param is nullptr");

    auto playerTrackParse = reinterpret_cast<PlayerTrackParse *>(userData);
    (void)playerTrackParse->AddProbeToPad(element, pad);
}

bool PlayerTrackParse::FindTrackInfo()
{
    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    return findTrackInfo_;
}

void PlayerTrackParse::SetUpDemuxerElementCb(GstElement &elem)
{
    MEDIA_LOGD("SetUpDemuxerElementCb elem %{public}s", ELEM_NAME(&elem));
    CHECK_AND_RETURN(AddProbeToPadList(&elem, *elem.srcpads));
    {
        std::unique_lock<std::mutex> lock(signalIdMutex_);
        gulong signalId = g_signal_connect(&elem, "pad-added", G_CALLBACK(PlayerTrackParse::OnPadAddedCb), this);
        CHECK_AND_RETURN_LOG(signalId != 0, "listen to pad-added failed");
        AddSignalIds(&elem, signalId);
    }
}

void PlayerTrackParse::Stop()
{
    MEDIA_LOGD("Stop");
    isStopping_ = true;
    {
        std::unique_lock<std::mutex> lock(trackInfoMutex_);
        videoTracks_.clear();
        audioTracks_.clear();
        updateTrackInfo_ = false;
        findTrackInfo_ = false;
        inputSelectMap_.clear();
        parsePadSet_.clear();
        trackVec_.clear();
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
            for (auto id : item.second) {
                g_signal_handler_disconnect(item.first, id);
            }
        }
        signalIds_.clear();
    }
}
}
}
