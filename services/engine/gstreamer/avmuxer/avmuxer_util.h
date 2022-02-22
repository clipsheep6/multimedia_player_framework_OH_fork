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

#ifndef AVMUXER_UTIL_H
#define AVMUXER_UTIL_H

#include <functional>
#include <set>
#include <map>
#include <gst/gst.h>
#include "i_avmuxer_engine.h"
#include "nocopyable.h"
#include "gst_shmem_wrap_allocator.h"
#include "gst_mux_bin.h"

namespace OHOS {
namespace Media {

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

enum MimeType {
    MUX_H264,
    MUX_H263,
    MUX_MPEG2,
    MUX_MPEG4,
    MUX_AAC,
    MUX_MP3,
};

const std::map<const std::string, MimeType> MIME_MAP_TYPE {
    {"video/x-h264", MUX_H264},
    {"video/mpeg4", MUX_MPEG4},
    {"video/x-h263", MUX_H263},
    {"video/mpeg2", MUX_MPEG2},
    {"audio/aac", MUX_AAC},
    {"audio/mp3", MUX_MP3}
};

class MyType {
public:
    bool hasCodecData_ = false;
    bool hasBuffer_ = false;
    bool needData_ = false;
    GstCaps *caps_ = nullptr;
    MimeType type_ ;
};

class FormatParam {
public:
    int32_t width;
    int32_t height;
    int32_t frameRate;
    int32_t channels;
    int32_t rate;
}

class AVMuxerUtil {
public:
    AVMuxerUtil() = delete;
    ~AVMuxerUtil() = delete;
    DISALLOW_COPY_AND_MOVE(AVMuxerUtil);

    static int32_t Seth264Caps(const MediaDescription &trackDesc, const std::string &mimeType,
        int32_t trackId, GstCaps *src_caps);
    static int32_t Seth263Caps(const MediaDescription &trackDesc, const std::string &mimeType,
        int32_t trackId, GstCaps *src_caps);
    static int32_t SetMPEG4Caps(const MediaDescription &trackDesc, const std::string &mimeType,
        int32_t trackId, GstCaps *src_caps);
    static int32_t SetaacCaps(const MediaDescription &trackDesc, const std::string &mimeType,
        int32_t trackId, GstCaps *src_caps);
    static int32_t Setmp3Caps(const MediaDescription &trackDesc, const std::string &mimeType,
        int32_t trackId, GstCaps *src_caps);

    static int32_t Writeh264CodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
        GstElement *src, GstMuxBin *muxBin, std::map<int, MyType>& trackInfo, GstShMemWrapAllocator *allocator);
    static int32_t Writeh263CodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
        GstElement *src, GstMuxBin *muxBin, std::map<int, MyType>& trackInfo, GstShMemWrapAllocator *allocator);
    static int32_t WriteMPEG4CodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
        GstElement *src, GstMuxBin *muxBin, std::map<int, MyType>& trackInfo, GstShMemWrapAllocator *allocator);
    static int32_t WriteaacCodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
        GstElement *src, GstMuxBin *muxBin, std::map<int, MyType>& trackInfo, GstShMemWrapAllocator *allocator);
    static int32_t Writemp3CodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
        GstElement *src, GstMuxBin *muxBin, std::map<int, MyType>& trackInfo, GstShMemWrapAllocator *allocator);
};
}
}
#endif