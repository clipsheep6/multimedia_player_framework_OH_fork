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

#include "codec_common.h"
#include "media_errors.h"

namespace OHOS {
namespace Media {
const std::map<int32_t, VideoPixelFormat> NUMBER_TO_PIEXEL = {
    {0, VIDEO_PIXEL_FORMAT_YUVI420},
    {1, VIDEO_PIXEL_FORMAT_NV12},
    {2, VIDEO_PIXEL_FORMAT_NV21},
};

const std::map<VideoPixelFormat, std::string> PIEXEL_TO_STRING = {
    {VIDEO_PIXEL_FORMAT_YUVI420, "I420"},
    {VIDEO_PIXEL_FORMAT_NV12, "NV12"},
    {VIDEO_PIXEL_FORMAT_NV21, "NV21"},
};

const std::map<int32_t, AudioRawFormat> NUMBER_TO_PCM = {
    {0, AUDIO_PCM_S8},
    {1, AUDIO_PCM_8},
    {2, AUDIO_PCM_S16_BE},
    {3, AUDIO_PCM_S16_LE},
    {4, AUDIO_PCM_16_BE},
    {5, AUDIO_PCM_16_LE},
    {6, AUDIO_PCM_S24_BE},
    {7, AUDIO_PCM_S24_LE},
    {8, AUDIO_PCM_24_BE},
    {9, AUDIO_PCM_24_LE},
    {10, AUDIO_PCM_S32_BE},
    {11, AUDIO_PCM_S32_LE},
    {12, AUDIO_PCM_32_BE},
    {13, AUDIO_PCM_32_LE},
    {14, AUDIO_PCM_F32_BE},
    {15, AUDIO_PCM_F32_LE},
};

const std::map<AudioRawFormat, std::string> PCM_TO_STRING = {
    {AUDIO_PCM_S8, "S8"},
    {AUDIO_PCM_8, "U8"},
    {AUDIO_PCM_S16_BE, "S16BE"},
    {AUDIO_PCM_S16_LE, "S16LE"},
    {AUDIO_PCM_16_BE, "U16BE"},
    {AUDIO_PCM_16_LE, "U16LE"},
    {AUDIO_PCM_S24_BE, "S24BE"},
    {AUDIO_PCM_S24_LE, "S24LE"},
    {AUDIO_PCM_24_BE, "U24BE"},
    {AUDIO_PCM_24_LE, "U24LE"},
    {AUDIO_PCM_S32_BE, "S32BE"},
    {AUDIO_PCM_S32_LE, "S32LE"},
    {AUDIO_PCM_32_BE, "U32BE"},
    {AUDIO_PCM_32_LE, "U32LE"},
    {AUDIO_PCM_F32_BE, "F32BE"},
    {AUDIO_PCM_F32_LE, "F32LE"},
};

const std::map<int32_t, VideoEncoderBitrateMode> NUMBER_TO_BITRATE_MODE = {
    {0, VIDEO_ENCODER_BITRATE_MODE_CBR},
    {1, VIDEO_ENCODER_BITRATE_MODE_VBR},
    {2, VIDEO_ENCODER_BITRATE_MODE_CQ},
};

int32_t MapVideoPixelFormat(int32_t number, VideoPixelFormat &pixel)
{
    if (NUMBER_TO_PIEXEL.count(number) != 0) {
        pixel =  NUMBER_TO_PIEXEL.at(number);
        return MSERR_OK;
    }
    return MSERR_INVALID_VAL;
}

std::string PixelFormatToString(VideoPixelFormat pixel)
{
    if (PIEXEL_TO_STRING.count(pixel) != 0) {
        return PIEXEL_TO_STRING.at(pixel);
    }
    return "Invalid";
}

int32_t MapPCMFormat(int32_t number, AudioRawFormat &format)
{
    if (NUMBER_TO_PCM.count(number) != 0) {
        format =  NUMBER_TO_PCM.at(number);
        return MSERR_OK;
    }
    return MSERR_INVALID_VAL;
}

std::string PCMFormatToString(AudioRawFormat format)
{
    if (PCM_TO_STRING.count(format) != 0) {
        return PCM_TO_STRING.at(format);
    }
    return "Invalid";
}

int32_t MapBitrateMode(int32_t number, VideoEncoderBitrateMode &mode)
{
    if (NUMBER_TO_BITRATE_MODE.count(number) != 0) {
        mode =  NUMBER_TO_BITRATE_MODE.at(number);
        return MSERR_OK;
    }
    return MSERR_INVALID_VAL;
}
} // namespace Media
} // namespace OHOS