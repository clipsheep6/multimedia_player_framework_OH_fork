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
    int32_t DoProcess(const Format &format);
    virtual std::shared_ptr<ProcessorConfig> GetInputPortConfig() = 0;
    virtual std::shared_ptr<ProcessorConfig> GetOutputPortConfig() = 0;

protected:
    virtual int32_t ProcessMandatory(const Format &format) = 0;
    virtual int32_t ProcessOptional(const Format &format) = 0;

    InnerCodecMimeType codecName_ = CODEC_MIMIE_TYPE_DEFAULT;
    std::string pluginName_ = "";
    bool isSoftWare_ = false;
    int32_t preferFormat_ = -1;

private:
    int32_t ProcessVendor(const Format &format);
};
} // namespace Media
} // namespace OHOS
#endif // FORMAT_PROCESSOR_BASE_H