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
            // use this format to avoid raw audio/video frame format convert
            preferFormat_ = (*it).format[0];
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

int32_t ProcessorBase::DoProcess(const Format &format)
{
    CHECK_AND_RETURN_RET(ProcessMandatory(format) == MSERR_OK, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(ProcessOptional(format) == MSERR_OK, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(ProcessVendor(format) == MSERR_OK, MSERR_INVALID_VAL);
    return MSERR_OK;
}

int32_t ProcessorBase::ProcessVendor(const Format &format)
{
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
