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

#ifndef MEDIA_DESCRIPTION_H
#define MEDIA_DESCRIPTION_H

#include <string_view>
#include "format.h"

namespace OHOS {
namespace Media {
using MediaDescription = Format;

/**
 * Key for track index, value type is uint32_t
 */
inline constexpr std::string_view MD_KEY_TRACK_INDEX = "track_index";

/**
 * Key for track type, value type is uint8_t, see {link @MediaTrackType}
 */
inline constexpr std::string_view MD_KEY_TRACK_TYPE = "track_type";

/**
 * Key for codec mime type, value type is string
 */
inline constexpr std::string_view MD_KEY_CODEC_MIME = "codec_mime";

/**
 * Key for duration, value type is int64_t
 */
inline constexpr std::string_view MD_KEY_DURATION = "duration";

/**
 * Key for birtate, value type is uint32_t
 */
inline constexpr std::string_view MD_KEY_BITRATE = "bitrate";

/**
 * Key for max input size, value type is uint32_t
 */
inline constexpr std::string_view MD_KEY_MAX_INPUT_SIZE = "max-input-size";

/**
 * Key for video width, value type is uint32_t
 */
inline constexpr std::string_view MD_KEY_WIDTH = "width";

/**
 * Key for video height, value type is uint32_t
 */
inline constexpr std::string_view MD_KEY_HEIGHT = "height";

/**
 * Key for video pixelformat, value type is int32_t, see {link @MediaPixelFormat}
 */
inline constexpr std::string_view MD_KEY_PIXEL_FORMAT = "pixel-format";

/**
 * Key for video frame rate, value type is uint32_t. The value is 100 times the
 * actual frame rate, i.e. the real frame rate is 23.99, so this value is 2399.
 */
inline constexpr std::string_view MD_KEY_FRAME_RATE = "frame-rate";

/**
 * Key for video capture rate, value type is uint32_t. The value is 100 times the
 * actual frame rate, i.e. the real capture rate is 23.99, so this value is 2399.
 */
inline constexpr std::string_view MD_KEY_CAPTURE_RATE = "capture-rate";

/**
 * Key for the interval of key frame. value type is int32_t, the unit is milliseconds.
 * A negative value means no key frames are requested after the first frame. A zero
 * value means a stream containing all key frames is requested.
 */
inline constexpr std::string_view MD_KEY_I_FRAME_INTERVAL = "i_frame_interval";

/**
 * Key for the request a I-Frame immediately. value type is boolean
 */
inline constexpr std::string_view MD_KEY_REQUEST_I_FRAME = "req_i_frame";

/**
 * Key for audio channel count, value type is uint32_t
 */
inline constexpr std::string_view MD_KEY_CHANNEL_COUNT = "channel-count";

/**
 * Key for audio sample rate, value type is uint32_t
 */
inline constexpr std::string_view MD_KEY_SAMPLE_RATE = "sample-rate";

/**
 * Key for track count in the container, value type is uint32_t
 */
inline constexpr std::string_view MD_KEY_TRACK_COUNT = "track-count";

/**
 * custom key prefix, media service will pass through to HAL.
 */
inline constexpr std::string_view MD_KEY_CUSTOM_PREFIX = "vendor.";
}
}

#endif
