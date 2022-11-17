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

int32_t PlayerTrackParse::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    int32_t trackType;
    for (auto &[pad, innerMeta] : trackInfos_) {
        if (innerMeta.GetIntValue(INNER_META_KEY_TRACK_TYPE, trackType) && trackType == MediaType::MEDIA_TYPE_VID) {
            Format outMeta;
            ConvertToPlayerKeys(innerMeta, outMeta);
            videoTrack.emplace_back(outMeta);
        }
    }
    return MSERR_OK;
}

int32_t PlayerTrackParse::GetAudioTrackInfo(std::vector<Format> &audioTrack)
{
    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    int32_t trackType;
    for (auto &[pad, innerMeta] : trackInfos_) {
        if (innerMeta.GetIntValue(INNER_META_KEY_TRACK_TYPE, trackType) && trackType == MediaType::MEDIA_TYPE_AUD) {
            Format outMeta;
            ConvertToPlayerKeys(innerMeta, outMeta);
            audioTrack.emplace_back(outMeta);
        }
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
            it->second.PutIntValue(INNER_META_KEY_TRACK_INDEX, trackcount_);
            trackcount_++;
            MEDIA_LOGD("GetTrackParse tarckcount %{public}d", trackcount_);
        }
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
        Format innerMeta;
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
