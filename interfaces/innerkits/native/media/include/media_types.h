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

#ifndef MEDIA_TYPES_H
#define MEDIA_TYPES_H

#include <cstdint>

namespace OHOS {
namespace Media {
enum SampleType : uint16_t {
    SYNC_FRAME = 1,
    PARTIAL_FRAME = 2,
    CODEC_DATA = 3,
};

enum SampleFlags : uint16_t {
    EOS_FRAME = 1,
};

struct SampleInfo {
    int64_t timeUs;
    int32_t size;
    int32_t offset;
    SampleType type;
    SampleFlags flags;
};

struct TrackSampleInfo : public SampleInfo {
    int32_t trackId;
};
}
}
#endif
