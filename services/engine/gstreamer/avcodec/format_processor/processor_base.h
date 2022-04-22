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

#ifndef FORMAT_PROCESSOR_BASE_H
#define FORMAT_PROCESSOR_BASE_H

#include "avcodeclist_engine_gst_impl.h"
#include "codec_common.h"
#include "format.h"

namespace OHOS {
namespace Media {
class ProcessorBase {
public:
    virtual ~ProcessorBase() = default;
    int32_t Init(AVCodecType type, bool isMimeType, const std::string &name);
    int32_t GetParameter(bool &isSoftWare, std::string &pluginName);
    int32_t ProcessParameter(const Format &format);
    virtual std::shared_ptr<ProcessorConfig> GetInputPortConfig() = 0;
    virtual std::shared_ptr<ProcessorConfig> GetOutputPortConfig() = 0;

protected:
    virtual int32_t ProcessMandatory(const Format &format) = 0;
    virtual int32_t ProcessOptional(const Format &format) = 0;

    InnerCodecMimeType codecName_ = CODEC_MIMIE_TYPE_DEFAULT;
    std::string pluginName_ = "";
    bool isSoftWare_ = false;
    bool isAudio_ = false;
    CapabilityData data_;

    // video common parameter
    int32_t width_ = 0;
    int32_t height_ = 0;
    int32_t pixelFormat_ = 0;
    int32_t frameRate_ = 0;
    std::string gstPixelFormat_;

    // audio common parameter
    int32_t channels_ = 0;
    int32_t sampleRate_ = 0;
    int32_t audioSampleFormat_ = 0;
    std::string gstRawFormat_ = "";

private:
    int32_t ProcessAudioCommonPara(const Format &format);
    int32_t ProcessVideoCommonPara(const Format &format);
    int32_t ProcessVendorPara(const Format &format);
};
} // namespace Media
} // namespace OHOS
#endif // FORMAT_PROCESSOR_BASE_H