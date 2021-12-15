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

#ifndef CONVERT_CODEC_DATA_H
#define CONVERT_CODEC_DATA_H

#include <vector>
#include <gst/gst.h>
#include "nocopyable.h"
#include "avsharedmemory.h"

namespace OHOS {
namespace Media {
class ConvertCodecData {
public:
    ConvertCodecData();
    ~ConvertCodecData();
    GstBuffer* GetCodecBuffer(std::shared_ptr<AVSharedMemory> sampleData);
    DISALLOW_COPY_AND_MOVE(ConvertCodecData);
private:
    const uint8_t *FindNextNal(const uint8_t *start, const uint8_t *end);
    void GetCodecData(const uint8_t *data, int32_t len, std::vector<uint8_t> &sps, std::vector<uint8_t> &pps,
            std::vector<uint8_t> &sei);
    void AVCDecoderConfiguration(std::vector<uint8_t> &sps, std::vector<uint8_t> &pps);
    uint32_t nalSize_ = 0;
    GstBuffer* configBuffer_ = nullptr;
};
}  // namespace Media
}  // namespace OHOS
#endif  // CONVERT_CODEC_DATA_H