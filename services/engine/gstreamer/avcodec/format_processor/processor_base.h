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

#include "format.h"
#include "../sink_wrapper/sink_base.h"
#include "../src_wrapper/src_base.h"

namespace OHOS {
namespace Media {
class ProcessorBase {
public:
    virtual ~ProcessorBase() = default;
    int32_t Init(const Format &format);
    int32_t DoProcess();

protected:
    virtual int32_t ProcessMandatory() = 0;
    virtual int32_t ProcessOptional() = 0;
    virtual int32_t FillSrcCaps(const std::shared_ptr<SrcBase> &src) = 0;
    virtual int32_t FillSinkCaps(const std::shared_ptr<SinkBase> &sink) = 0;

private:
    int32_t ProcessVendor();
};
} // Media
} // OHOS
#endif // FORMAT_PROCESSOR_BASE_H