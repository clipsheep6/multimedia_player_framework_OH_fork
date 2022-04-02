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
#include <gst/audio/audio.h>
#include <gst/gst.h>
#include "avcodec_info.h"
#include "avcodec_common.h"
#include "audio_info.h"
#include "avsharedmemory.h"
#include "format.h"
#include "surface.h"

namespace OHOS {
namespace Media {
enum InnerCodecMimeType : int32_t {
    CODEC_MIMIE_TYPE_DEFAULT = -1,
    /** H263 */
    CODEC_MIMIE_TYPE_VIDEO_H263,
    /** H264 */
    CODEC_MIMIE_TYPE_VIDEO_AVC,
    /** MPEG2 */
    CODEC_MIMIE_TYPE_VIDEO_MPEG2,
    /** HEVC */
    CODEC_MIMIE_TYPE_VIDEO_HEVC,
    /** MPEG4 */
    CODEC_MIMIE_TYPE_VIDEO_MPEG4,
    /** MP3 */
    CODEC_MIMIE_TYPE_AUDIO_MPEG,
    /** AAC */
    CODEC_MIMIE_TYPE_AUDIO_AAC,
    /** VORBIS */
    CODEC_MIMIE_TYPE_AUDIO_VORBIS,
    /** FLAC */
    CODEC_MIMIE_TYPE_AUDIO_FLAC,
};

enum VideoEncoderBitrateMode : int32_t {
    VIDEO_ENCODER_BITRATE_MODE_CBR = 0,
    VIDEO_ENCODER_BITRATE_MODE_VBR,
    VIDEO_ENCODER_BITRATE_MODE_CQ,
};

enum MPEG4SamplingFrequencies : int32_t {
    MPEG4_SAMPLING_FREQUENCY_0 = 0,
    MPEG4_SAMPLING_FREQUENCY_1,
    MPEG4_SAMPLING_FREQUENCY_2,
    MPEG4_SAMPLING_FREQUENCY_3,
    MPEG4_SAMPLING_FREQUENCY_4,
    MPEG4_SAMPLING_FREQUENCY_5,
    MPEG4_SAMPLING_FREQUENCY_6,
    MPEG4_SAMPLING_FREQUENCY_7,
    MPEG4_SAMPLING_FREQUENCY_8,
    MPEG4_SAMPLING_FREQUENCY_9,
    MPEG4_SAMPLING_FREQUENCY_10,
    MPEG4_SAMPLING_FREQUENCY_11,
    MPEG4_SAMPLING_FREQUENCY_12,
    MPEG4_SAMPLING_FREQUENCY_13,
};

const std::map<int32_t, MPEG4SamplingFrequencies> MPEG4_SAMPLING_FREQUENCIES = {
    { 96000, MPEG4_SAMPLING_FREQUENCY_0 },
    { 88200, MPEG4_SAMPLING_FREQUENCY_1 },
    { 64000, MPEG4_SAMPLING_FREQUENCY_2 },
    { 48000, MPEG4_SAMPLING_FREQUENCY_3 },
    { 44100, MPEG4_SAMPLING_FREQUENCY_4 },
    { 32000, MPEG4_SAMPLING_FREQUENCY_5 },
    { 24000, MPEG4_SAMPLING_FREQUENCY_6 },
    { 22050, MPEG4_SAMPLING_FREQUENCY_7 },
    { 16000, MPEG4_SAMPLING_FREQUENCY_8 },
    { 12000, MPEG4_SAMPLING_FREQUENCY_9 },
    { 11025, MPEG4_SAMPLING_FREQUENCY_10 },
    { 8000, MPEG4_SAMPLING_FREQUENCY_11 },
    { 7350, MPEG4_SAMPLING_FREQUENCY_12 },
    { -1, MPEG4_SAMPLING_FREQUENCY_13 },
};

struct AdtsFixedHeader {
    int32_t version = 0; // MPEG-4
    int32_t layer = 0;
    int32_t protectAbsent = 1; // no CRC
    int32_t profile = 1; // AAC_LC
    int32_t samplingFrequencyIndex = 0;
    int32_t channelConfiguration = 0;
};

struct BufferWrapper {
    enum Owner : int32_t {
        APP = 0,
        SERVER,
        DOWNSTREAM
    };
    explicit BufferWrapper(Owner owner)
        : owner_(owner)
    {
    }

    ~BufferWrapper()
    {
        if (gstBuffer_ != nullptr) {
            gst_buffer_unref(gstBuffer_);
        }
    }

    GstBuffer *gstBuffer_ = nullptr;
    AVSharedMemory *mem_ = nullptr;
    sptr<SurfaceBuffer> surfaceBuffer_ = nullptr;
    Owner owner_ = DOWNSTREAM;
};

struct ProcessorConfig {
    ProcessorConfig(GstCaps *caps, bool isEncoder)
        : caps_(caps),
          isEncoder_(isEncoder)
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
    bool needParser_ = false;
    bool needAdtsTransform_ = false;
    bool isEncoder_ = false;
    uint32_t bufferSize_ = 0;
    AdtsFixedHeader head_;
};

std::string PixelFormatToGst(VideoPixelFormat pixel);
std::string MPEG4ProfileToGst(MPEG4Profile profile);
std::string AVCProfileToGst(AVCProfile profile);
std::string RawAudioFormatToGst(AudioStandard::AudioSampleFormat format);
int32_t MapCodecMime(const std::string &mime, InnerCodecMimeType &name);
int32_t CapsToFormat(GstCaps *caps, Format &format);
uint32_t PixelBufferSize(VideoPixelFormat pixel, uint32_t width, uint32_t height, uint32_t alignment);
uint32_t CompressedBufSize(uint32_t width, uint32_t height, bool isEncoder, InnerCodecMimeType type);
} // namespace Media
} // namespace OHOS
#endif // CODEC_COMMON_H
