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

#ifndef AVMUXER_ENGINE_GST_IMPL_H
#define AVMUXER_ENGINE_GST_IMPL_H

#include <set>
#include <map>
#include "nocopyable.h"
#include "i_avmuxer_engine.h"
#include "gst_mux_bin.h"
#include "gst_msg_processor.h"
#include "gst_shmem_wrap_allocator.h"

namespace OHOS {
namespace Media {
constexpr uint32_t MAX_VIDEO_TRACK_NUM = 1;
constexpr uint32_t MAX_AUDIO_TRACK_NUM = 16;

const std::set<std::string> VIDEO_MIME_TYPE {
    "video/x-h264",
    "video/mpeg4",
    "video/x-h263",
    "video/mpeg2"
};

const std::set<std::string> AUDIO_MIME_TYPE {
    "audio/aac",
    "audio/mp3"
};

const std::set<std::string> FORMAT_TYPE {
    "mp4",
    "m4a"
};

const std::map<const std::string, const std::string> MIME_MAP_ENCODE {
    {"video/x-h264", "video/x-h264"},
    {"video/mpeg4", "video/mpeg"},
    {"video/x-h263", "video/x-h263"},
    {"video/mpeg2", "video/mpeg2"},
    {"audio/aac", "audio/mpeg"},
    {"audio/mp3", "audio/mpeg"}
};

const std::map<std::string, std::string> FORMAT_TO_MUX {
    {"mp4", "qtmux"},
    {"m4a", "qtmux"}
};

const std::map<std::string, std::set<std::string>> FORMAT_TO_MIME {
    {"mp4", {"video/x-h264", "video/mpeg4", "video/x-h263", "video/mpeg2", "audio/aac", "audio/mp3"}},
    {"m4a", {"audio/aac"}}
};

class AVMuxerEngineGstImpl : public IAVMuxerEngine {
public:
    AVMuxerEngineGstImpl();
    ~AVMuxerEngineGstImpl();

    DISALLOW_COPY_AND_MOVE(AVMuxerEngineGstImpl);
    std::vector<std::string> GetAVMuxerFormatList() override;
    int32_t Init() override;
    int32_t SetOutput(const std::string &path, const std::string &format) override;
    int32_t SetLocation(float latitude, float longitude) override;
    int32_t SetOrientationHint(int degrees) override;
    int32_t AddTrack(const MediaDescription &trackDesc, int32_t &trackId) override;
    int32_t Start() override;
    int32_t WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo) override;
    int32_t Stop() override;
private:
    int32_t Seth264Caps(const MediaDescription &trackDesc, const std::string &mimeType, int32_t trackId);
    int32_t Seth263Caps(const MediaDescription &trackDesc, const std::string &mimeType, int32_t trackId);
    int32_t SetMPEG4Caps(const MediaDescription &trackDesc, const std::string &mimeType, int32_t trackId);
    int32_t SetaacCaps(const MediaDescription &trackDesc, const std::string &mimeType, int32_t trackId);
    int32_t Setmp3Caps(const MediaDescription &trackDesc, const std::string &mimeType, int32_t trackId);
    int32_t Writeh264CodecData(std::shared_ptr<AVSharedMemory> sampleData,
        const TrackSampleInfo &sampleInfo, GstElement *src);
    int32_t Writeh263CodecData(std::shared_ptr<AVSharedMemory> sampleData,
        const TrackSampleInfo &sampleInfo, GstElement *src);
    int32_t WriteMPEG4CodecData(std::shared_ptr<AVSharedMemory> sampleData,
        const TrackSampleInfo &sampleInfo, GstElement *src);
    int32_t WriteaacCodecData(std::shared_ptr<AVSharedMemory> sampleData,
        const TrackSampleInfo &sampleInfo, GstElement *src);
    int32_t Writemp3CodecData(std::shared_ptr<AVSharedMemory> sampleData,
        const TrackSampleInfo &sampleInfo, GstElement *src);
    int32_t WriteData(std::shared_ptr<AVSharedMemory> sampleData,
        const TrackSampleInfo &sampleInfo, GstElement *src);
    int32_t SetupMsgProcessor();
    void OnNotifyMessage(const InnerMessage &msg);
    void clear();

    GstMuxBin *muxBin_ = nullptr;
    std::set<int32_t> trackIdSet;
    std::map<int32_t, std::string> trackId2EncodeType;
    std::set<int32_t> hasCaps;
    std::set<int32_t> hasBuffer;
    std::map<int32_t, bool> needData_; 
    std::map<int32_t, GstCaps *> CapsMat;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool endFlag_ = false;
    bool errHappened_ = false;
    std::unique_ptr<GstMsgProcessor> msgProcessor_;
    uint32_t videoTrackNum_ = 0;
    uint32_t audioTrackNum_ = 0;
    std::string format_;
    bool isReady_ = false;
    bool isPause_ = false;
    bool isPlay_ = false;
    GstShMemWrapAllocator *allocator_;
};
}  // namespace Media
}  // namespace OHOS
#endif