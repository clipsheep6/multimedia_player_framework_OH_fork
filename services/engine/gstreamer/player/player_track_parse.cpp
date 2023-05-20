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

int32_t PlayerTrackParse::GetTrackInfo(int32_t index, int32_t &innerIndex, int32_t &trackType)
{
    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    CHECK_AND_RETURN_RET_LOG(demuxerElementFind_, MSERR_INVALID_OPERATION, "Plugin not found");
    CHECK_AND_RETURN_RET_LOG(index >= 0 && index < static_cast<int32_t>(trackInfos_.size()), MSERR_INVALID_VAL,
        "Invalid index %{public}d", index);

    StartUpdatTrackInfo();

    int32_t size = videoTracks.size() + audioTracks.size();
    if (index >= size) {
        innerIndex = index - size;
        trackType = MediaType::MEDIA_TYPE_SUBTITLE;
    } else if (index >= static_cast<int32_t>(videoTracks.size())) {
        innerIndex = index - videoTracks.size();
        trackType = MediaType::MEDIA_TYPE_AUD;
    } else {
        innerIndex = index;
        trackType = MediaType::MEDIA_TYPE_VID;
    }

    MEDIA_LOGI("index:0x%{public}d innerIndex:0x%{public}d trackType:0x%{public}d ", index, innerIndex, trackType);
    return MSERR_OK;
}

int32_t PlayerTrackParse::GetTrackIndex(int32_t innerIndex, int32_t trackType, int32_t &index)
{
    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    CHECK_AND_RETURN_RET_LOG(demuxerElementFind_, MSERR_INVALID_OPERATION, "Plugin not found");
    CHECK_AND_RETURN_RET_LOG(trackType >= MediaType::MEDIA_TYPE_AUD && trackType <= MediaType::MEDIA_TYPE_SUBTITLE,
        MSERR_INVALID_VAL, "Invalid trackType %{public}d", trackType);
    CHECK_AND_RETURN_RET_LOG(innerIndex >= 0, MSERR_INVALID_VAL, "Invalid innerIndex %{public}d", innerIndex);

    StartUpdatTrackInfo();

    if (trackType == MediaType::MEDIA_TYPE_AUD) {
        CHECK_AND_RETURN_RET_LOG(innerIndex < static_cast<int32_t>(audioTracks.size()),
            MSERR_INVALID_VAL, "Invalid innerIndex %{public}d", innerIndex);
        index = innerIndex + videoTracks.size();
    } else if (trackType == MediaType::MEDIA_TYPE_VID) {
        CHECK_AND_RETURN_RET_LOG(innerIndex < static_cast<int32_t>(videoTracks.size()),
            MSERR_INVALID_VAL, "Invalid innerIndex %{public}d", innerIndex);
        index = innerIndex;
    } else {
        int32_t maxSize = static_cast<int32_t>(trackInfos_.size() - audioTracks.size() - videoTracks.size());
        CHECK_AND_RETURN_RET_LOG(innerIndex < maxSize, MSERR_INVALID_VAL, "Invalid innerIndex %{public}d", innerIndex);
        index = innerIndex + videoTracks.size() + audioTracks.size();
    }

    MEDIA_LOGI("innerIndex:0x%{public}d trackType:0x%{public}d index:0x%{public}d", innerIndex, trackType, index);
    return MSERR_OK;
}

void PlayerTrackParse::StartUpdatTrackInfo()
{
    if (!updateTrackInfo_) {
        updateTrackInfo_ = true;
        UpdatTrackInfo();
    }
}

void PlayerTrackParse::UpdatTrackInfo()
{
    if (!updateTrackInfo_) {
        return;
    }

    int32_t index;
    std::map<int32_t, Format> tracks;
    for (auto &[pad, innerMeta] : trackInfos_) {
        // Sort trackinfo by index
        innerMeta.GetIntValue(INNER_META_KEY_TRACK_INDEX, index);
        (void)tracks.emplace(index, innerMeta);
    }

    videoTracks.clear();
    audioTracks.clear();
    for (auto &[ind, innerMeta] : tracks) {
        int32_t trackType;
        Format outMeta;
        innerMeta.GetIntValue(INNER_META_KEY_TRACK_TYPE, trackType);
        ConvertToPlayerKeys(innerMeta, outMeta);
        if (trackType == MediaType::MEDIA_TYPE_VID) {
            videoTracks.emplace_back(outMeta);
        } else if (trackType == MediaType::MEDIA_TYPE_AUD) {
            audioTracks.emplace_back(outMeta);
        }
    }
    return;
}

int32_t PlayerTrackParse::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    CHECK_AND_RETURN_RET_LOG(demuxerElementFind_, MSERR_INVALID_OPERATION, "Plugin not found");
    StartUpdatTrackInfo();
    videoTrack.assign(videoTracks.begin(), videoTracks.end());
    return MSERR_OK;
}

int32_t PlayerTrackParse::GetAudioTrackInfo(std::vector<Format> &audioTrack)
{
    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    CHECK_AND_RETURN_RET_LOG(demuxerElementFind_, MSERR_INVALID_OPERATION, "Plugin not found");
    StartUpdatTrackInfo();
    audioTrack.assign(audioTracks.begin(), audioTracks.end());
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
    auto it = trackInfos_.find(pad);
    CHECK_AND_RETURN_RET_LOG(it != trackInfos_.end(), GST_PAD_PROBE_OK,
        "unrecognized pad %{public}s", PAD_NAME(pad));

    if (static_cast<unsigned int>(info->type) & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) {
        GstEvent *event = gst_pad_probe_info_get_event(info);
        CHECK_AND_RETURN_RET_LOG(event != nullptr, GST_PAD_PROBE_OK, "event is null");

        if (GST_EVENT_TYPE(event) == GST_EVENT_TAG) {
            GstTagList *tagList = nullptr;
            gst_event_parse_tag(event, &tagList);
            CHECK_AND_RETURN_RET_LOG(tagList != nullptr, GST_PAD_PROBE_OK, "tags is nullptr")
            MEDIA_LOGI("catch tags at pad %{public}s", PAD_NAME(pad));
            GstMetaParser::ParseTagList(*tagList, it->second);
        } else if (GST_EVENT_TYPE(event) == GST_EVENT_CAPS) {
            GstCaps *caps = nullptr;
            gst_event_parse_caps(event, &caps);
            CHECK_AND_RETURN_RET_LOG(caps != nullptr, GST_PAD_PROBE_OK, "caps is nullptr")
            MEDIA_LOGI("catch caps at pad %{public}s", PAD_NAME(pad));
            GstMetaParser::ParseStreamCaps(*caps, it->second);
        }
    }
    (void)UpdatTrackInfo();
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
        Format innerMeta;
        // The order of pad creation is consistent with the index of "current-audio"/"current-text"
        innerMeta.PutIntValue(INNER_META_KEY_TRACK_INDEX, trackcount_);
        trackcount_++;
        MEDIA_LOGI("trackcount_:0x%{public}d", trackcount_);
        (void)trackInfos_.emplace(pad, innerMeta);
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

void PlayerTrackParse::SetDemuxerElementFind(bool isFind)
{
    demuxerElementFind_ = isFind;
}

bool PlayerTrackParse::GetDemuxerElementFind() const
{
    return demuxerElementFind_;
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

void PlayerTrackParse::SetUpParseElementCb(GstElement &elem)
{
    MEDIA_LOGD("SetUpParseElementCb elem %{public}s", ELEM_NAME(&elem));
    (void)AddProbeToPadList(&elem, *elem.srcpads);
}

void PlayerTrackParse::Stop()
{
    MEDIA_LOGD("Stop");
    {
        std::unique_lock<std::mutex> lock(trackInfoMutex_);
        trackInfos_.clear();
        videoTracks.clear();
        audioTracks.clear();
        updateTrackInfo_ = false;
        demuxerElementFind_ = false;
        trackcount_ = 0;
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
