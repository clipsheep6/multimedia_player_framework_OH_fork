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

#ifndef MEDIA_COMMON_H
#define MEDIA_COMMON_H

#include <cstdint>
#include <string>
#include "recorder.h"

namespace OHOS {
namespace Media {
enum CodecMimeType : int32_t {
    CODEC_MIMIE_TYPE_DEFAULT = -1,
    CODEC_MIMIE_TYPE_VIDEO_H263 = 0,
    CODEC_MIMIE_TYPE_VIDEO_AVC,
    CODEC_MIMIE_TYPE_VIDEO_MPEG2,
    CODEC_MIMIE_TYPE_VIDEO_HEVC,
    CODEC_MIMIE_TYPE_VIDEO_MPEG4,
    CODEC_MIMIE_TYPE_VIDEO_VP8,
    CODEC_MIMIE_TYPE_VIDEO_VP9,
    CODEC_MIMIE_TYPE_AUDIO_AMR_NB,
    CODEC_MIMIE_TYPE_AUDIO_AMR_WB,
    CODEC_MIMIE_TYPE_AUDIO_MPEG,
    CODEC_MIMIE_TYPE_AUDIO_AAC,
    CODEC_MIMIE_TYPE_AUDIO_VORBIS,
    CODEC_MIMIE_TYPE_AUDIO_OPUS,
    CODEC_MIMIE_TYPE_AUDIO_FLAC,
    CODEC_MIMIE_TYPE_AUDIO_RAW,
};

enum ContainerFormatType : int32_t {
    CFT_MPEG_4 = 0,
    CFT_MPEG_TS,
    CFT_MKV,
    CFT_WEBM,
    CFT_MPEG_4A,
    CFT_OGG,
    CFT_WAV,
    CFT_AAC,
    CFT_FLAC,
};

__attribute__((visibility("default"))) int32_t MapCodecMime(const std::string &mime, CodecMimeType &name);
__attribute__((visibility("default"))) int32_t MapContainerFormat(const std::string &format, ContainerFormatType &cft);
__attribute__((visibility("default"))) int32_t MapOutputFormatType(const ContainerFormatType &cft, 
    OutputFormatType &opf);
__attribute__((visibility("default"))) int32_t MapAudioCodec(const CodecMimeType &mime, AudioCodecFormat &audio);
__attribute__((visibility("default"))) int32_t MapVideoCodec(const CodecMimeType &mime, VideoCodecFormat &video);
} // namespace Media
} // namespace OHOS
#endif // MEDIA_COMMON_H
