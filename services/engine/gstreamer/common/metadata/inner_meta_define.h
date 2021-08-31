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

#ifndef INNER_META_DEFINE_H
#define INNER_META_DEFINE_H

#include <cstdint>
#include <string_view>

namespace OHOS {
namespace Media {
enum InnerMetaKey : int32_t {
    INNER_META_KEY_ALBUM = 0,
    INNER_META_KEY_ALBUMARTIST,
    INNER_META_KEY_ARTIST,
    INNER_META_KEY_AUTHOR,
    INNER_META_KEY_COMPOSER,
    INNER_META_KEY_DURATION,
    INNER_META_KEY_GENRE,
    INNER_META_KEY_HAS_AUDIO,
    INNER_META_KEY_HAS_VIDEO,
    INNER_META_KEY_MIMETYPE,
    INNER_META_KEY_NUM_TRACKS,
    INNER_META_KEY_SAMPLERATE,
    INNER_META_KEY_TITLE,
    INNER_META_KEY_VIDEO_HEIGHT,
    INNER_META_KEY_VIDEO_WIDTH,
    INNER_META_KEY_BUTT,
};
}
}

#endif