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

#ifndef GST_COMMON_UTILS_H
#define GST_COMMON_UTILS_H

#include <stdint.h>
#include <gst/gst.h>

struct VideoFrameBuffer {
    uint32_t keyFrameFlag;
    uint64_t timeStamp;
    uint64_t duration;
    uint64_t size;
    GstBuffer *gstBuffer;
};
#endif  // GST_COMMON_UTILS_H
