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

#ifndef PLAYER_TRACK_PARSE_H
#define PLAYER_TRACK_PARSE_H

#include <mutex>
#include <vector>
#include <unordered_map>
#include <map>
#include <set>
#include <gst/gst.h>
#include <gst/player/player.h>
#include "meta/format.h"

namespace OHOS {
namespace Media {
class PlayerTrackParse {
public:
    static std::shared_ptr<PlayerTrackParse> Create();
    int32_t GetVideoTrackInfo(std::vector<Format> &videoTrack);
    int32_t GetAudioTrackInfo(std::vector<Format> &audioTrack);
    int32_t GetSubtitleTrackInfo(std::vector<Format> &subtitleTrack);
    bool FindTrackInfo();
    void OnElementSetup(GstElement &elem);
    void OnElementUnSetup(GstElement &elem);
    void Stop();
    int32_t GetTrackInfo(int32_t index, int32_t &innerIndex, int32_t &trackType);
    int32_t GetTrackIndex(int32_t innerIndex, int32_t trackType, int32_t &index);
    PlayerTrackParse();
    ~PlayerTrackParse();

private:
    GstPadProbeReturn GetTrackParse(GstPad *pad, GstPadProbeInfo *info);
    GstPadProbeReturn ParseTrackInfo(GstPad *pad, GstPadProbeInfo *info, Format &format);
    void ConvertToPlayerKeys(const Format &innerMeta, Format &outMeta) const;
    bool AddProbeToPad(const GstElement *element, GstPad *pad);
    bool AddProbeToPadList(GstElement *element, GList &list);
    static GstPadProbeReturn ProbeCallback(GstPad *pad, GstPadProbeInfo *info, gpointer userData);
    static void OnPadAddedCb(const GstElement *element, GstPad *pad, gpointer userData);
    void SetUpDemuxerElementCb(GstElement &elem);
    
    void SetUpInputSelectElementCb(GstElement &elem);
    static void OnInputSelectPadAddedCb(const GstElement *element, GstPad *pad, gpointer userData);
    bool InputSelectAddProbeToPad(const GstElement *element, GstPad *pad);
    static GstPadProbeReturn InputSelectProbeCallback(GstPad *pad, GstPadProbeInfo *info, gpointer userData);
    GstPadProbeReturn GetUsedDemux(GstPad *pad, GstPadProbeInfo *info);

    static bool IsSameStreamId(GstPad *padA, GstPad *padB);
    bool HasSameStreamIdInDemux(GstPad *pad);
    void UpdateTrackInfo();
    void StartUpdateTrackInfo();
    int32_t GetInputSelectPadIndex(GstPad *pad);
    void ParseSubtitlePadCaps(const GstElement *element, GstPad *pad, int32_t index, Format &innerMeta);

    inline void AddSignalIds(GstElement *elem, gulong signalId)
    {
        if (signalIds_.find(elem) == signalIds_.end()) {
            signalIds_[elem] = {signalId};
        } else {
            signalIds_[elem].push_back(signalId);
        }
    }
    inline void RemoveSignalIds(GstElement *elem)
    {
        if (signalIds_.find(elem) != signalIds_.end()) {
            for (auto id : signalIds_[elem]) {
                g_signal_handler_disconnect(elem, id);
            }
            signalIds_.erase(elem);
        }
    }
    std::map<GstElement *, std::vector<gulong>> signalIds_;

    struct DemuxInfo {
        explicit DemuxInfo(GstElement *value): demux(value) {}
        ~DemuxInfo() = default;
        GstElement *demux = nullptr;
        bool inUse = false;
        int32_t trackcount = 0;
        std::map<GstPad *, Format> trackInfos;
    };

    struct InputSelectInfo {
        InputSelectInfo() = default;
        ~InputSelectInfo() = default;
        int32_t padCount = 0;
        std::map<GstPad *, int32_t> padIndexMap;
    };

    bool findTrackInfo_ = false;
    bool updateTrackInfo_ = false;
    std::atomic<bool> isStopping_ = false;
    std::set<GstPad *> parsePadSet_;
    std::map<GstElement *, InputSelectInfo> inputSelectMap_;
    std::vector<DemuxInfo> trackVec_;
    std::vector<Format> videoTracks_;
    std::vector<Format> audioTracks_;
    std::vector<Format> subtitleTracks_;
    struct PadInfo {
        GstPad *pad;
        gulong probeId;
    };
    std::unordered_map<GstPad *, gulong> padProbes_;
    std::mutex signalIdMutex_;
    std::mutex padProbeMutex_;
    std::mutex trackInfoMutex_;
};
}
}
#endif
