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

#include "processor_vdec_impl.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ProcessorVdecImpl"};
}

namespace OHOS {
namespace Media {
ProcessorVdecImpl::ProcessorVdecImpl()
{
}

ProcessorVdecImpl::~ProcessorVdecImpl()
{
}

int32_t ProcessorVdecImpl::ProcessMandatory(const Format &format)
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

int32_t ProcessorVdecImpl::ProcessOptional(const Format &format)
{
    (void)pushBlankFrame_;
    //(void)Format::GetIntValue("push_blank_frame_after_stop", pushBlankFrame_);
    return MSERR_OK;
}

std::shared_ptr<ProcessorConfig> ProcessorVdecImpl::GetInputPortConfig()
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

std::shared_ptr<ProcessorConfig> ProcessorVdecImpl::GetOutputPortConfig()
{
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
        "width", G_TYPE_INT, width_,
        "height", G_TYPE_INT, height_,
        "format", G_TYPE_STRING, "NV12",
        "framerate", G_TYPE_INT, frameRate_, nullptr);
    auto config = std::make_shared<ProcessorConfig>(caps);
    return config;
}
} // Media
} // OHOS
