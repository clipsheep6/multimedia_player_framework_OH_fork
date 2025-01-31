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

#ifndef I_AVCODECLIST_SERVICE_H
#define I_AVCODECLIST_SERVICE_H

#include "avcodec_info.h"
#include "buffer/avsharedmemory.h"

namespace OHOS {
namespace Media {
class IAVCodecListService {
public:
    // AVCodecList
    virtual ~IAVCodecListService() = default;
    virtual std::string FindVideoDecoder(const Format &format) = 0;
    virtual std::string FindVideoEncoder(const Format &format) = 0;
    virtual std::string FindAudioDecoder(const Format &format) = 0;
    virtual std::string FindAudioEncoder(const Format &format) = 0;
    virtual std::vector<CapabilityData> GetCodecCapabilityInfos() = 0;
};
} // namespace Media
} // namespace OHOS
#endif