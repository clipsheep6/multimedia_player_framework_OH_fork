/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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
#ifndef RECORDER_UTILS_H
#define RECORDER_UTILS_H

#include <cstdint>

namespace OHOS {
namespace Media {
constexpr uint8_t VIDEO_SOURCE_MAX_COUNT = 1;
constexpr uint8_t AUDIO_SOURCE_MAX_COUNT = 1;

constexpr int32_t INVALID_SOURCE_ID = -1;

struct SourceIdGenerator {
    static const uint32_t SOURCE_MASK = 0xF00;
    static const uint32_t VIDEO_MASK = 0x100;
    static const uint32_t AUDIO_MASK = 0x200;
    static const uint32_t INDEX_MASK = 0xFF;

    static int32_t GenerateAudioSourceId(uint32_t index)
    {
        return static_cast<int32_t>(AUDIO_MASK + (INDEX_MASK & index));
    }

    static int32_t GenerateVideoSourceId(uint32_t index)
    {
        return static_cast<int32_t>(VIDEO_MASK + (INDEX_MASK & index));
    }

    static int32_t IsAudio(int32_t sourceId)
    {
        return ((sourceId > 0) &&
                ((static_cast<uint32_t>(sourceId) & SOURCE_MASK) == AUDIO_MASK));
    }

    static int32_t IsVideo(int32_t sourceId)
    {
        return ((sourceId > 0) &&
                ((static_cast<uint32_t>(sourceId) & SOURCE_MASK) == VIDEO_MASK));
    }
};
#endif  // RECORDER_UTILS_H
} // Media
} // OHOS