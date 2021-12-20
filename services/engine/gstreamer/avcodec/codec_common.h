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

#ifndef CODEC_COMMON_H
#define CODEC_COMMON_H

#include <string>
#include "avsharedmemory.h"
#include <gst/audio/audio.h>
#include <gst/gst.h>

namespace OHOS {
namespace Media {
static const GstAudioChannelPosition CHANNEL_POSITION[6][6] = {
    {
        GST_AUDIO_CHANNEL_POSITION_MONO
    },
    {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT
    },
    {
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT
    },
    {
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_CENTER
    },
    {
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT
    },
    {
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_LFE1
    },
};

enum CodecMimeType : int32_t {
    CODEC_MIMIE_TYPE_VIDEO_H263 = 0,
    CODEC_MIMIE_TYPE_VIDEO_AVC,
    CODEC_MIMIE_TYPE_VIDEO_HEVC,
    CODEC_MIMIE_TYPE_VIDEO_MPEG,
    CODEC_MIMIE_TYPE_VIDEO_MPEG2,
    CODEC_MIMIE_TYPE_VIDEO_MPEG4,
    CODEC_MIMIE_TYPE_AUDIO_VORBIS,
    CODEC_MIMIE_TYPE_AUDIO_MP3,
    CODEC_MIMIE_TYPE_AUDIO_AAC,
    CODEC_MIMIE_TYPE_AUDIO_FLAC,
};

enum VideoEncoderBitrateMode : int32_t {
    VIDEO_ENCODER_BITRATE_MODE_CBR = 0,
    VIDEO_ENCODER_BITRATE_MODE_VBR,
    VIDEO_ENCODER_BITRATE_MODE_CQ,
};

enum VideoPixelFormat : int32_t {
    VIDEO_PIXEL_FORMAT_YUVI420 = 0,
    VIDEO_PIXEL_FORMAT_NV12 = 1,
    VIDEO_PIXEL_FORMAT_NV21 = 2,
};

enum AudioRawFormat : int32_t {
    AUDIO_PCM_S8 = 1,
    AUDIO_PCM_8,
    AUDIO_PCM_S16_BE,
    AUDIO_PCM_S16_LE,
    AUDIO_PCM_16_BE,
    AUDIO_PCM_16_LE,
    AUDIO_PCM_S24_BE,
    AUDIO_PCM_S24_LE,
    AUDIO_PCM_24_BE,
    AUDIO_PCM_24_LE,
    AUDIO_PCM_S32_BE,
    AUDIO_PCM_S32_LE,
    AUDIO_PCM_32_BE,
    AUDIO_PCM_32_LE,
    AUDIO_PCM_F32_BE,
    AUDIO_PCM_F32_LE,
};

enum AACProfile : int32_t {
    AAC_PROFILE_LC = 0,
    AAC_PROFILE_ELD,
    AAC_PROFILE_ERLC,
    AAC_PROFILE_HE,
    AAC_PROFILE_HE_V2,
    AAC_PROFILE_LD,
    AAC_PROFILE_MAIN,
};

enum AVCProfile : int32_t {
    AVC_PROFILE_BASELINE = 0,
    AVC_PROFILE_CONSTRAINED_BASELINE,
    AVC_PROFILE_CONSTRAINED_HIGH,
    AVC_PROFILE_EXTENDED,
    AVC_PROFILE_HIGH,
    AVC_PROFILE_HIGH_10,
    AVC_PROFILE_HIGH_422,
    AVC_PROFILE_HIGH_444,
    AVC_PROFILE_MAIN,
};

enum HEVCProfile : int32_t {
    HEVC_PROFILE_MAIN = 0,
    HEVC_PROFILE_MAIN_10,
    HEVC_PROFILE_MAIN_10_HDR10,
    HEVC_PROFILE_MAIN_10_HDR10_PLUS,
    HEVC_PROFILE_MAIN_STILL,
};

enum MPEG2Profile : int32_t {
    MPEG2_PROFILE_422 = 0,
    MPEG2_PROFILE_HIGH,
    MPEG2_PROFILE_MAIN,
    MPEG2_PROFILE_SNR,
    MPEG2_PROFILE_SIMPLE,
    MPEG2_PROFILE_SPATIAL,
};

enum MPEG4Profile : int32_t {
    MPEG4_PROFILE_ADVANCED_CODING = 0,
    MPEG4_PROFILE_ADVANCED_CORE,
    MPEG4_PROFILE_ADVANCED_REAL_TIME,
    MPEG4_PROFILE_ADVANCED_SCALABLE,
    MPEG4_PROFILE_ADVANCED_SIMPLE,
    MPEG4_PROFILE_BASIC_ANIMATED,
    MPEG4_PROFILE_CORE,
    MPEG4_PROFILE_CORE_SCALABLE,
    MPEG4_PROFILE_HYBRID,
    MPEG4_PROFILE_MAIN,
    MPEG4_PROFILE_NBIT,
    MPEG4_PROFILE_SCALABLE_TEXXTURE,
    MPEG4_PROFILE_SIMPLE,
    MPEG4_PROFILE_SIMPLE_FBA,
    MPEG4_PROFILE_SIMPLE_FACE,
    MPEG4_PROFILE_SIMPLE_SCALABLE,
};

enum VP8Profile : int32_t {
    VP8_PROFILE_MAIN = 0,
};

enum VP9Profile : int32_t {
    VP9_PROFILE_0 = 0,
    VP9_PROFILE_1,
    VP9_PROFILE_2,
    VP9_PROFILE_2_HDR,
    VP9_PROFILE_2_HDR10_PLUS,
    VP9_PROFILE_3,
    VP9_PROFILE_3_HDR,
    VP9_PROFILE_3_HDR10_PLUS,
};

struct BufferWrapper {
    enum Owner : int32_t {
        APP = 0,
        SERVER,
        DOWNSTREAM
    };
    BufferWrapper(std::shared_ptr<AVSharedMemory> mem, GstBuffer *gstBuffer, uint32_t index, Owner owner)
    : mem_(mem), gstBuffer_(gstBuffer), index_(index), owner_(owner)
    {
    }
    ~BufferWrapper()
    {
        if (gstBuffer_ != nullptr) {
            gst_buffer_unref(gstBuffer_);
        }
    }
    std::shared_ptr<AVSharedMemory> mem_ = nullptr;
    GstBuffer *gstBuffer_ = nullptr;
    uint32_t index_ = 0;
    Owner owner_ = DOWNSTREAM;
    GstSample *sample_ = nullptr;
};

struct ProcessorConfig {
    ProcessorConfig(GstCaps *caps)
    : caps_(caps)
    {
    }
    ~ProcessorConfig()
    {
        if (caps_ != nullptr) {
            gst_caps_unref(caps_);
        }
    }
    GstCaps *caps_ = nullptr;
    bool needCodecData_ = false;
};

__attribute__((visibility("default"))) int32_t MapVideoPixelFormat(int32_t number, VideoPixelFormat &pixel);
__attribute__((visibility("default"))) std::string PixelFormatToString(VideoPixelFormat pixel);
__attribute__((visibility("default"))) int32_t MapPCMFormat(int32_t number, AudioRawFormat &format);
__attribute__((visibility("default"))) std::string PCMFormatToString(AudioRawFormat format);
__attribute__((visibility("default"))) int32_t MapBitrateMode(int32_t number, VideoEncoderBitrateMode &mode);
__attribute__((visibility("default"))) int32_t MapCodecMime(const std::string &mime, CodecMimeType &name);
__attribute__((visibility("default"))) int32_t MapProfile(int32_t number, AVCProfile &profile);
} // Media
} // OHOS
#endif // CODEC_COMMON_H
