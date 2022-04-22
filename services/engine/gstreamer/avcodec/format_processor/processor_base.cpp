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

#include "processor_base.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ProcessorBase"};
}

namespace OHOS {
namespace Media {
int32_t ProcessorBase::Init(AVCodecType type, bool isMimeType, const std::string &name)
{
    isAudio_ = (type == AVCODEC_TYPE_AUDIO_ENCODER) || (type == AVCODEC_TYPE_AUDIO_DECODER);

    auto codecList = std::make_unique<AVCodecListEngineGstImpl>();
    CHECK_AND_RETURN_RET(codecList != nullptr, MSERR_NO_MEMORY);

    if (isMimeType) {
        CHECK_AND_RETURN_RET(MapCodecMime(name, codecName_) == MSERR_OK, MSERR_UNKNOWN);
        Format format;
        format.PutStringValue("codec_mime", name);
        switch (type) {
            case AVCODEC_TYPE_VIDEO_ENCODER:
                pluginName_ = codecList->FindVideoEncoder(format);
                break;
            case AVCODEC_TYPE_VIDEO_DECODER:
                pluginName_ = codecList->FindVideoDecoder(format);
                break;
            case AVCODEC_TYPE_AUDIO_ENCODER:
                pluginName_ = codecList->FindAudioEncoder(format);
                break;
            case AVCODEC_TYPE_AUDIO_DECODER:
                pluginName_ = codecList->FindAudioDecoder(format);
                break;
            default:
                break;
        }
    }

    std::vector<CapabilityData> data = codecList->GetCodecCapabilityInfos();
    bool pluginExist = false;
    for (auto it = data.begin(); it != data.end(); it++) {
        if ((*it).codecName == pluginName_) {
            isSoftWare_ = !(*it).isVendor;
            if (!isMimeType) {
                (void)MapCodecMime((*it).mimeType, codecName_);
            }
            pluginExist = true;
            data_ = *it;
            break;
        }
    }
    CHECK_AND_RETURN_RET(pluginExist == true, MSERR_INVALID_VAL);

    return MSERR_OK;
}

int32_t ProcessorBase::GetParameter(bool &isSoftWare, std::string &pluginName)
{
    isSoftWare = isSoftWare_;
    pluginName = pluginName_;
    return MSERR_OK;
}

int32_t ProcessorBase::ProcessParameter(const Format &format)
{
    if (isAudio_) {
        int32_t ret = ProcessAudioCommonPara(format);
        CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
    } else {
        int32_t ret = ProcessVideoCommonPara(format);
        CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
    }
    CHECK_AND_RETURN_RET(ProcessMandatory(format) == MSERR_OK, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(ProcessOptional(format) == MSERR_OK, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(ProcessVendorPara(format) == MSERR_OK, MSERR_INVALID_VAL);
    return MSERR_OK;
}

int32_t ProcessorBase::ProcessAudioCommonPara(const Format &format)
{
    CHECK_AND_RETURN_RET(format.GetIntValue("channel_count", channels_) == true, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(format.GetIntValue("sample_rate", sampleRate_) == true, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(format.GetIntValue("audio_sample_format", audioSampleFormat_) == true, MSERR_INVALID_VAL);
    MEDIA_LOGD("channels:%{public}d, sampleRate:%{public}d, pcm:%{public}d",
        channels_, sampleRate_, audioSampleFormat_);

    if (channels_ < data_.channels.minVal || channels_ > data_.channels.maxVal) {
        MEDIA_LOGE("Unsupported channels");
        return MSERR_UNSUPPORT_AUD_CHANNEL_NUM;
    }
    CHECK_AND_RETURN_RET(data_.sampleRate.size() > 0, MSERR_UNKNOWN);
    std::sort(data_.sampleRate.begin(), data_.sampleRate.end());
    if (sampleRate_ < data_.sampleRate[0] || sampleRate_ > data_.sampleRate.back()) {
        MEDIA_LOGE("Unsupported sample rate");
        return MSERR_UNSUPPORT_AUD_SAMPLE_RATE;
    }
    if (std::find(data_.format.begin(), data_.format.end(), audioSampleFormat_) == data_.format.end()) {
        MEDIA_LOGE("Unsupported audio raw format");
        return MSERR_UNSUPPORT_AUD_PARAMS;
    }

    gstRawFormat_ = RawAudioFormatToGst(static_cast<AudioStandard::AudioSampleFormat>(audioSampleFormat_));
    return MSERR_OK;
}

int32_t ProcessorBase::ProcessVideoCommonPara(const Format &format)
{
    CHECK_AND_RETURN_RET(format.GetIntValue("width", width_) == true, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(format.GetIntValue("height", height_) == true, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(format.GetIntValue("pixel_format", pixelFormat_) == true, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(format.GetIntValue("frame_rate", frameRate_) == true, MSERR_INVALID_VAL);

    if (pixelFormat_ == SURFACE_FORMAT) {
        pixelFormat_ = data_.format[0];
    }

    MEDIA_LOGD("width:%{public}d, height:%{public}d, pixel:%{public}d, frameRate:%{public}d",
        width_, height_, pixelFormat_, frameRate_);

    if (width_ < data_.width.minVal || width_ > data_.width.maxVal) {
        MEDIA_LOGE("Unsupported width");
        return MSERR_UNSUPPORT_VID_PARAMS;
    }
    if (height_ < data_.height.minVal || height_ > data_.height.maxVal) {
        MEDIA_LOGE("Unsupported height");
        return MSERR_UNSUPPORT_VID_PARAMS;
    }
    if (std::find(data_.format.begin(), data_.format.end(), pixelFormat_) == data_.format.end()) {
        MEDIA_LOGE("Unsupported pixel format");
        return MSERR_UNSUPPORT_VID_PARAMS;
    }
    if (frameRate_ < data_.frameRate.minVal || frameRate_ > data_.frameRate.maxVal) {
        MEDIA_LOGE("Unsupported frame rate");
        return MSERR_UNSUPPORT_VID_PARAMS;
    }

    gstPixelFormat_ = PixelFormatToGst(static_cast<VideoPixelFormat>(pixelFormat_));
    return MSERR_OK;
}

int32_t ProcessorBase::ProcessVendorPara(const Format &format)
{
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
