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

#include "processor_venc_impl.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ProcessorVencImpl"};
    const uint32_t MAX_WIDTH = 8000;
    const uint32_t MAX_HEIGHT = 5000;
}

namespace OHOS {
namespace Media {
ProcessorVencImpl::ProcessorVencImpl()
{
}

ProcessorVencImpl::~ProcessorVencImpl()
{
}

int32_t ProcessorVencImpl::ProcessMandatory(const Format &format)
{
    CHECK_AND_RETURN_RET(format.GetIntValue("width", width_) == true, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(format.GetIntValue("height", height_) == true, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(format.GetIntValue("pixel_format", pixelFormat_) == true, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(format.GetIntValue("frame_rate", frameRate_) == true, MSERR_INVALID_VAL);
    MEDIA_LOGD("width:%{public}d, height:%{public}d, pixel:%{public}d, framerate:%{public}d",
        width_, height_, pixelFormat_, frameRate_);

    gstPixelFormat_ = PixelFormatToGst(static_cast<VideoPixelFormat>(pixelFormat_));

    return MSERR_OK;
}

int32_t ProcessorVencImpl::ProcessOptional(const Format &format)
{
    auto formatMap = format.GetFormatMap();
    if (formatMap.find("video_encode_bitrate_mode") != formatMap.end()) {
        (void)format.GetIntValue("video_encode_bitrate_mode", bitrateMode_);
    }

    if (formatMap.find("codec_profile") != formatMap.end()) {
        (void)format.GetIntValue("codec_profile", profile_);
    }

    return MSERR_OK;
}


std::shared_ptr<ProcessorConfig> ProcessorVencImpl::GetInputPortConfig()
{
    CHECK_AND_RETURN_RET(width_ > 0 && width_ < MAX_WIDTH, nullptr);
    CHECK_AND_RETURN_RET(height_ > 0 && height_ < MAX_HEIGHT, nullptr);

    GstCaps *caps = gst_caps_new_simple("video/x-raw",
        "width", G_TYPE_INT, width_,
        "height", G_TYPE_INT, height_,
        "format", G_TYPE_STRING, gstPixelFormat_.c_str(),
        "framerate", G_TYPE_INT, frameRate_, nullptr);
    CHECK_AND_RETURN_RET_LOG(caps != nullptr, nullptr, "No memory");

    auto config = std::make_shared<ProcessorConfig>(caps, true);
    if (config == nullptr) {
        MEDIA_LOGE("No memory");
        gst_caps_unref(caps);
        return nullptr;
    }

    const uint32_t alignment = 16;
    config->bufferSize_ = PixelBufferSize(static_cast<VideoPixelFormat>(pixelFormat_),
        static_cast<uint32_t>(width_), static_cast<uint32_t>(height_), alignment);

    return config;
}

std::shared_ptr<ProcessorConfig> ProcessorVencImpl::GetOutputPortConfig()
{
    CHECK_AND_RETURN_RET(width_ > 0 && width_ < MAX_WIDTH, nullptr);
    CHECK_AND_RETURN_RET(height_ > 0 && height_ < MAX_HEIGHT, nullptr);

    GstCaps *caps = nullptr;
    switch (codecName_) {
        case CODEC_MIMIE_TYPE_VIDEO_MPEG4:
            caps = gst_caps_new_simple("video/mpeg",
                "width", G_TYPE_INT, width_,
                "height", G_TYPE_INT, height_,
                "framerate", G_TYPE_INT, frameRate_,
                "mpegversion", G_TYPE_INT, 1, nullptr);
            break;
        case CODEC_MIMIE_TYPE_VIDEO_AVC:
            caps = gst_caps_new_simple("video/x-h264",
                "width", G_TYPE_INT, width_,
                "height", G_TYPE_INT, height_,
                "framerate", G_TYPE_INT, frameRate_,
                "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
                "level", G_TYPE_STRING, "2",
                "profile", G_TYPE_STRING, "high",
                "alignment", G_TYPE_STRING, "nal",
                "stream-format", G_TYPE_STRING, "byte-stream", nullptr);
            break;
        default:
            break;
    }
    CHECK_AND_RETURN_RET_LOG(caps != nullptr, nullptr, "Unsupported format");

    auto config = std::make_shared<ProcessorConfig>(caps, true);
    if (config == nullptr) {
        MEDIA_LOGE("No memory");
        gst_caps_unref(caps);
        return nullptr;
    }

    config->bufferSize_ = EncodedBufSize(width_, height_);

    return config;
}
} // Media
} // OHOS
