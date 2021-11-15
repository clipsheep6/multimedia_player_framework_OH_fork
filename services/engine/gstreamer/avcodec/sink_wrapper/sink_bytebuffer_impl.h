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

#ifndef SINK_BYTEBUFFER_IMPL_H
#define SINK_BYTEBUFFER_IMPL_H

#include "sink_base.h"

namespace OHOS {
namespace Media {
class SinkBytebufferImpl : public SinkBase {
public:
    virtual ~SinkBytebufferImpl() = default;

    std::shared_ptr<AVSharedMemory> GetOutputBuffer(uint32_t index) override;
    int32_t ReleaseOutputBuffer(uint32_t index, bool render) override;
    int32_t SetParameter(const Format &format) override;
    int32_t SetCallback(const std::shared_ptr<SinkCallback> &callback) override;
    int32_t SetCaps(GstCaps *caps) override;
};
} // Media
} // OHOS
#endif // SINK_BYTEBUFFER_IMPL_H