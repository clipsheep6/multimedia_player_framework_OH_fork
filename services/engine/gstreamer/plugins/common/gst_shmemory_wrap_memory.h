/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef GST_SHMEMORY_WRAP_MEMORY_H
#define GST_SHMEMORY_WRAP_MEMORY_H

#include <functional>
#include <gst/gst.h>
#include "avsharedmemory.h"

struct _GstShMemoryWrapMemory;
using GstShMemoryWrapMemory = _GstShMemoryWrapMemory;
using FreeMemory = std::function<void(int32_t, int32_t)>;

struct _GstShMemoryWrapMemory {
    GstMemory parent;
    std::shared_ptr<OHOS::Media::AVSharedMemory> shmemory;
    int32_t offset;
    int32_t length;
    FreeMemory free_memory;
};

static const char GST_SHMEMORY_WRAP_MEMORY_TYPE[] = "ShmemWrapMemory";

static inline gboolean gst_is_shmemory_wrap_memory(GstMemory *mem)
{
    return gst_memory_is_type(mem, GST_SHMEMORY_WRAP_MEMORY_TYPE);
}

#endif // GST_SHMEMORY_WRAP_MEMORY_H
