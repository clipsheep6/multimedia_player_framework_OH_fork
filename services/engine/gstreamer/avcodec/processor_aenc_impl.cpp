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

#include "processor_aenc_impl.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ProcessorAencImpl"};
    constexpr int32_t MAX_CHANNELS = 2;
}

namespace OHOS {
namespace Media {
ProcessorAencImpl::ProcessorAencImpl()
{
}

ProcessorAencImpl::~ProcessorAencImpl()
{
}

int32_t ProcessorAencImpl::ProcessMandatory(const Format &format)
{
    MEDIA_LOGD("ProcessMandatory");
    (void)channels_;
    (void)sampleRate_;
    (void)pcmFormat_;
    //CHECK_AND_RETURN_RET(Format::GetIntValue("channels", channels_) == true, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET_LOG(channels_ <= MAX_CHANNELS, MSERR_INVALID_VAL, "Invalid channel cout");
    //CHECK_AND_RETURN_RET(Format::GetIntValue("sampleRate", sampleRate_) == true, MSERR_INVALID_VAL);
    //CHECK_AND_RETURN_RET(Format::GetIntValue("pcmFormat", pcmFormat_) == true, MSERR_INVALID_VAL);
    return MSERR_OK;
}

int32_t ProcessorAencImpl::ProcessOptional(const Format &format)
{
    (void)profile_;
    //(void)Format::GetIntValue("profile", profile_);
    // switch (mimeType_) {
    //     case aac:
    //         break;
    //     default:
    //         MEDIA_LOGE("Unknown type");
    //         break;
    // }
    return MSERR_OK;
}

std::shared_ptr<ProcessorConfig> ProcessorAencImpl::GetInputPortConfig()
{
    guint64 channelMask = 0;
    if (!gst_audio_channel_positions_to_mask(CHANNEL_POSITION[channels_], channels_, FALSE, &channelMask)) {
        MEDIA_LOGE("Invalid channel positions");
        return nullptr;
    }

    GstCaps *caps = gst_caps_new_simple("audio/x-raw",
        "rate", G_TYPE_INT, sampleRate_,
        "channels", G_TYPE_INT, channels_,
        "format", G_TYPE_STRING, "S16LE",
        "channel-mask", GST_TYPE_BITMASK, channelMask,
        "layout", G_TYPE_STRING, "interleaved", nullptr);
    auto config = std::make_shared<ProcessorConfig>(caps);
    return config;
}

std::shared_ptr<ProcessorConfig> ProcessorAencImpl::GetOutputPortConfig()
{
    return nullptr;
}
} // Media
} // OHOS
