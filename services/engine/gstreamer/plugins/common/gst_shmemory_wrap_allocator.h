/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
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

#ifndef GST_SHMEMORY_WRAP_ALLOCATOR_H
#define GST_SHMEMORY_WRAP_ALLOCATOR_H

#include <gst/gst.h>
#include "avdatasrcmemory.h"
#include "gst_shmemory_wrap_memory.h"

G_BEGIN_DECLS

#define GST_TYPE_SHMEMORY_WRAP_ALLOCATOR (gst_shmemory_wrap_allocator_get_type())
#define GST_SHMEMORY_WRAP_ALLOCATOR(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SHMEMORY_WRAP_ALLOCATOR, GstShMemoryWrapAllocator))
#define GST_SHMEMORY_WRAP_ALLOCATOR_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_SHMEMORY_WRAP_ALLOCATOR, GstShMemoryWrapAllocatorClass))
#define GST_IS_SHMEMORY_WRAP_ALLOCATOR(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SHMEMORY_WRAP_ALLOCATOR))
#define GST_IS_SHMEMORY_WRAP_ALLOCATOR_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_SHMEMORY_WRAP_ALLOCATOR))
#define GST_SHMEMORY_WRAP_ALLOCATOR_CAST(obj) ((GstShMemoryWrapAllocator*)(obj))

using GstShMemoryWrapAllocator = struct _GstShmemoryWrapAllocator;
using GstShMemoryWrapAllocatorClass = struct _GstShmemoryWrapAllocatorClass;

struct _GstShmemoryWrapAllocator {
    GstAllocator parent;
};

struct _GstShmemoryWrapAllocatorClass {
    GstAllocatorClass parent;
};

GType gst_shmemory_wrap_allocator_get_type(void);

__attribute__((visibility("default"))) GstShMemoryWrapAllocator *gst_shmemory_wrap_allocator_new(void);
__attribute__((visibility("default"))) GstMemory *gst_shmemory_wrap(GstAllocator *allocator,
    std::shared_ptr<OHOS::Media::AVSharedMemory> shmem, int32_t offset,
    int32_t length, int32_t sub, FreeMemory free_memory);

G_END_DECLS

#endif // GST_SHMEMORY_WRAP_ALLOCATOR_H