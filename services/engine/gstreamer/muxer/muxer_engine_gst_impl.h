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

#ifndef MUXER_ENGINE_GST_IMPL_H
#define MUXER_ENGINE_GST_IMPL_H

#include <set>
#include <map>
#include "nocopyable.h"
#include "i_muxer_engine.h"
#include "gst_mux_bin.h"
#include "gst_msg_processor.h"

namespace OHOS {
namespace Media {
constexpr uint32_t MAX_VIDEO_TRACK_NUM = 1;
constexpr uint32_t MAX_AUDIO_TRACK_NUM = 16;

const std::set<std::string> videoEncodeType {
    "video/h264",
    "video/mpeg4",
    "video/h263",
    "video/mpeg2"
};

const std::set<std::string> audioEncodeType {
    "audio/aac",
    "audio/mp3"
};

const std::map<std::string, std::string> formatToMux {
    {"mp4", "qtmux"},
    {"m4a", "qtmux"}
};

const std::map<std::string, std::set<std::string>> formatToEncode {
    {"mp4", {"video/h264", "video/mpeg4", "video/h263", "video/mpeg2"}},
    {"m4a", {"audio/aac"}}
};

class MuxerEngineGstImpl : public IMuxerEngine {
public:
    MuxerEngineGstImpl();
    ~MuxerEngineGstImpl();

    DISALLOW_COPY_AND_MOVE(MuxerEngineGstImpl);

    int32_t Init() override;
    int32_t SetOutput(const std::string &path, const std::string &format) override;
    int32_t SetLocation(float latitude, float longitude) override;
    int32_t SetOrientationHint(int degrees) override;
    int32_t AddTrack(const MediaDescription &trackDesc, int32_t &trackId) override;
    int32_t Start() override;
    int32_t WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo) override;
    int32_t Stop() override;
private:
    int32_t SetupMsgProcessor();
    void OnNotifyMessage(const InnerMessage &msg);
    void clear();

    GstMuxBin* muxBin_ = nullptr;
    std::set<int32_t> trackIdSet;
    std::set<int32_t> hasCaps;
    std::set<int32_t> hasBuffer;
    std::map<int32_t, bool> needData_; 
    std::map<int32_t, GstCaps*> CapsMat;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool endFlag_ = false;
    bool errHappened_ = false;
    std::unique_ptr<GstMsgProcessor> msgProcessor_;
    uint32_t videoTrackNum = 0;
    uint32_t audioTrackNum = 0;
    std::string format_;
};
}  // namespace Media
}  // namespace OHOS
#endif