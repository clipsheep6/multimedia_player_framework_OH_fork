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
    MEDIA_LOGD("ProcessMandatory");
    (void)width_;
    (void)height_;
    (void)pixelFormat_;
    (void)frameRate_;
    //CHECK_AND_RETURN_RET(Format::GetIntValue("width", width_) == true, MSERR_INVALID_VAL);
    //CHECK_AND_RETURN_RET(Format::GetIntValue("height", height_) == true, MSERR_INVALID_VAL);
    //CHECK_AND_RETURN_RET(Format::GetIntValue("pixelFormat", pixelFormat_) == true, MSERR_INVALID_VAL);
    //CHECK_AND_RETURN_RET(Format::GetIntValue("frameRate", frameRate_) == true, MSERR_INVALID_VAL);
    return MSERR_OK;
}

int32_t ProcessorVencImpl::ProcessOptional(const Format &format)
{
    (void)bitrate_;
    (void)iFrameInterval_;
    (void)profile_;
    (void)quality_;
    (void)level_;
    // (void)Format::GetIntValue("bitrate", bitrate_);
    // (void)Format::GetIntValue("i_frame_interval", iFrameInterval_);
    // (void)Format::GetIntValue("profile", profile_);
    // (void)Format::GetIntValue("quality", quality_);
    // (void)Format::GetIntValue("level", level_);
    // switch (mimeType_) {
    //     case 264:
    //         break;
    //     case 265:
    //         break;
    //     case mpeg:
    //         break;
    //     default:
    //         MEDIA_LOGE("Unknown type");
    //         break;
    // }
    return MSERR_OK;
}

std::shared_ptr<ProcessorConfig> ProcessorVencImpl::GetInputPortConfig()
{
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
        "width", G_TYPE_INT, width_,
        "height", G_TYPE_INT, height_,
        "format", G_TYPE_STRING, "NV12",
        "framerate", G_TYPE_INT, frameRate_, nullptr);
    auto config = std::make_shared<ProcessorConfig>(caps);
    return config;
}

std::shared_ptr<ProcessorConfig> ProcessorVencImpl::GetOutputPortConfig()
{
    GstCaps *caps = gst_caps_new_simple("video/x-h264",
        "width", G_TYPE_INT, width_,
        "height", G_TYPE_INT, height_,
        "alignment", G_TYPE_STRING, "au",
        "framerate", G_TYPE_INT, frameRate_,
        "stream-format", G_TYPE_STRING, "byte-stream", nullptr);
    // GstCaps *caps = gst_caps_new_simple("video/x-h265",
    //     "width", G_TYPE_INT, width_,
    //     "height", G_TYPE_INT, height_,
    //     "alignment", G_TYPE_STRING, "au",
    //     "framerate", G_TYPE_INT, frameRate_,
    //     "stream-format", G_TYPE_STRING, "byte-stream", nullptr);
    auto config = std::make_shared<ProcessorConfig>(caps);
    return config;
}
} // Media
} // OHOS
