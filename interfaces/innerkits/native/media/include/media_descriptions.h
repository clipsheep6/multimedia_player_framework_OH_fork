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

#ifndef MEDIA_DESCRIPTIONS_H
#define MEDIA_DESCRIPTIONS_H

#include <string_view>
#include "format.h"

namespace OHOS {
namespace Media {
using MediaDescriptions = Format;

// common
constexpr std::string_view MD_KEY_MIMETYPE = "mime-type";
constexpr std::string_view MD_KEY_DURATION = "duration";
constexpr std::string_view MD_KEY_BITRATE = "bitrate";
constexpr std::string_view MD_KEY_MAX_INPUT_SIZE = "max-input-size";

// video
constexpr std::string_view MD_KEY_WIDTH = "width";
constexpr std::string_view MD_KEY_HEIGHT = "height";
constexpr std::string_view MD_KEY_PIXEL_FORMAT = "pixel-format";
constexpr std::string_view MD_KEY_FRAME_RATE = "frame-rate";
constexpr std::string_view MD_KEY_CAPTURE_RATE = "capture-rate";

// audio
constexpr std::string_view MD_KEY_CHANNEL_COUNT = "channel-count";
constexpr std::string_view MD_KEY_SAMPLE_RATE = "sample-rate";

// container
constexpr std::string_view MD_KEY_TRACK_COUNT = "track-count";
}
}

#endif
