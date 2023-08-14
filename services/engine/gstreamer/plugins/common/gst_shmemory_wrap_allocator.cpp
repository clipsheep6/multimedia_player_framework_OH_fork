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

#include "gst_shmemory_wrap_allocator.h"
#include "securec.h"
#include "media_log.h"
#include "scope_guard.h"
using namespace OHOS;

#define gst_shmemory_wrap_allocator_parent_class parent_class
G_DEFINE_TYPE(GstShMemoryWrapAllocator, gst_shmemory_wrap_allocator, GST_TYPE_ALLOCATOR);

GstShMemoryWrapAllocator *gst_shmemory_wrap_allocator_new(void)
{
    GstShMemoryWrapAllocator *alloc = GST_SHMEMORY_WRAP_ALLOCATOR_CAST(g_object_new(
        GST_TYPE_SHMEMORY_WRAP_ALLOCATOR, "name", "ShmemoryWrapAllocator", nullptr));
    (void)gst_object_ref_sink(alloc);

    return alloc;
}

GstMemory *gst_shmemory_wrap(GstAllocator *allocator, std::shared_ptr<OHOS::Media::AVSharedMemory> shmem,
    int32_t offset, int32_t length, int32_t sub, FreeMemory free_memory)
{
    g_return_val_if_fail(allocator != nullptr, nullptr);
    g_return_val_if_fail(shmem != nullptr, nullptr);

    GstShMemoryWrapMemory *memory =
        reinterpret_cast<GstShMemoryWrapMemory *>(g_slice_alloc0(sizeof(GstShMemoryWrapMemory)));
    g_return_val_if_fail(memory != nullptr, nullptr);

    gst_memory_init(GST_MEMORY_CAST(memory), GST_MEMORY_FLAG_NO_SHARE, allocator,
        nullptr, length, 0, 0, length);

    memory->shmemory = shmem;
    memory->offset = offset;
    memory->length = length;
    memory->sub = sub;
    memory->free_memory = free_memory;
    GST_DEBUG("wrap memory for size: %" PRIu64 ", addr: 0x%06" PRIXPTR "",
        static_cast<uint64_t>(length), FAKE_POINTER(
            std::static_pointer_cast<OHOS::Media::AVDataSrcMemory>(memory->shmemory)->GetInnerBase() + offset));

    return GST_MEMORY_CAST(memory);
}

static GstMemory *gst_shmemory_wrap_allocator_alloc(GstAllocator *allocator, gsize size, GstAllocationParams *params)
{
    (void)allocator;
    (void)size;
    (void)params;
    return nullptr;
}

static void gst_shmemory_wrap_allocator_free(GstAllocator *allocator, GstMemory *memory)
{
    g_return_if_fail(memory != nullptr && allocator != nullptr);
    g_return_if_fail(gst_is_shmemory_wrap_memory(memory));

    GstShMemoryWrapMemory *shWrapMem = reinterpret_cast<GstShMemoryWrapMemory *>(memory);
    GST_DEBUG("free memory for size: %" G_GSIZE_FORMAT ", shWrapMem->offset %d, addr: 0x%06" PRIXPTR "",
    memory->maxsize, shWrapMem->offset, FAKE_POINTER(
        std::static_pointer_cast<OHOS::Media::AVDataSrcMemory>(shWrapMem->shmemory)->GetInnerBase() +
        shWrapMem->offset));

    shWrapMem->shmemory = nullptr;
    if (shWrapMem->free_memory) {
        shWrapMem->free_memory(shWrapMem->offset, shWrapMem->length, shWrapMem->sub);
    }
    g_slice_free(GstShMemoryWrapMemory, shWrapMem);
}

static gpointer gst_shmemory_wrap_allocator_mem_map(GstMemory *mem, gsize maxsize, GstMapFlags flags)
{
    (void)maxsize;
    (void)flags;
    g_return_val_if_fail(mem != nullptr, nullptr);
    g_return_val_if_fail(gst_is_shmemory_wrap_memory(mem), nullptr);

    GstShMemoryWrapMemory *shWrapMem = reinterpret_cast<GstShMemoryWrapMemory *>(mem);
    g_return_val_if_fail(shWrapMem->shmemory != nullptr, nullptr);

    GST_INFO("mem_map, maxsize: %" G_GSIZE_FORMAT ", size: %" G_GSIZE_FORMAT", addr: 0x%06" PRIXPTR "",
        mem->maxsize, mem->size, FAKE_POINTER(
            std::static_pointer_cast<OHOS::Media::AVDataSrcMemory>(shWrapMem->shmemory)->GetInnerBase() +
            shWrapMem->offset));
    
    return std::static_pointer_cast<OHOS::Media::AVDataSrcMemory>(shWrapMem->shmemory)->GetInnerBase() +
        shWrapMem->offset;
}

static void gst_shmemory_wrap_allocator_mem_unmap(GstMemory *mem)
{
    (void)mem;
}

static GstMemory *gst_shmemory_wrap_allocator_mem_share(GstMemory *mem, gssize offset, gssize size)
{
    g_return_val_if_fail(mem != nullptr &&
        reinterpret_cast<GstShMemoryWrapMemory *>(mem)->shmemory != nullptr, nullptr);
    g_return_val_if_fail(offset >= 0 && static_cast<gsize>(offset) < mem->size, nullptr);
    GST_DEBUG("begin gst_shmemory_wrap_allocator_mem_share, offset is %" G_GSSIZE_FORMAT ","
        "size is %" G_GSSIZE_FORMAT "", offset, size);
    GstShMemoryWrapMemory *sub = nullptr;
    GstMemory *parent = nullptr;

    /* find the real parent */
    if ((parent = mem->parent) == NULL) {
        parent = reinterpret_cast<GstMemory *>(mem);
    }
    if (size == -1) {
        size = mem->size - offset;
    }

    sub = g_slice_new0(GstShMemoryWrapMemory);
    g_return_val_if_fail(sub != nullptr, nullptr);
    /* the shared memory is always readonly */
    GST_DEBUG("mem->maxsize is %" G_GSIZE_FORMAT "", mem->maxsize);
    gst_memory_init(
        GST_MEMORY_CAST(sub),
        (GstMemoryFlags)(GST_MINI_OBJECT_FLAGS(parent) | GST_MINI_OBJECT_FLAG_LOCK_READONLY),
        mem->allocator,
        GST_MEMORY_CAST(parent),
        mem->maxsize,
        mem->align,
        mem->offset,
        size);
    
    sub->shmemory = reinterpret_cast<GstShMemoryWrapMemory *>(mem)->shmemory;
    sub->offset = reinterpret_cast<GstShMemoryWrapMemory *>(mem)->offset + offset;
    sub->length = size;
    GST_DEBUG("gst_shmemory_wrap_allocator_mem_share, addr: 0x%06" PRIXPTR "", FAKE_POINTER(
        std::static_pointer_cast<OHOS::Media::AVDataSrcMemory>(sub->shmemory)->GetInnerBase() + sub->offset));
    return GST_MEMORY_CAST(sub);
}

static GstMemory *gst_shmemory_wrap_allocator_mem_copy(GstShMemoryWrapMemory *mem, gssize offset, gssize size)
{
    g_return_val_if_fail(mem != nullptr && mem->shmemory != nullptr, nullptr);
    g_return_val_if_fail(offset >= 0 && offset < mem->length, nullptr);
    GST_DEBUG("in gst_shmemory_wrap_allocator_mem_copy, offset is %" G_GSSIZE_FORMAT ","
        "size is %" G_GSSIZE_FORMAT "", offset, size);

    gssize realOffset = static_cast<gssize>(mem->offset) + offset;
    g_return_val_if_fail(realOffset >= 0, nullptr);
    size = size == -1 ? static_cast<gssize>(mem->length) - offset : size;
    g_return_val_if_fail(size > 0, nullptr);

    GstMemory *copy = gst_allocator_alloc(nullptr, static_cast<gsize>(size), nullptr);
    g_return_val_if_fail(copy != nullptr, nullptr);

    GstMapInfo info = GST_MAP_INFO_INIT;
    ON_SCOPE_EXIT(0) { gst_memory_unref(copy); };
    g_return_val_if_fail(gst_memory_map(copy, &info, GST_MAP_READ), nullptr);
    ON_SCOPE_EXIT(1) { gst_memory_unmap(copy, &info); };

    uint8_t *src = std::static_pointer_cast<OHOS::Media::AVDataSrcMemory>(mem->shmemory)->GetInnerBase() + realOffset;
    errno_t rc = memcpy_s(info.data, info.size, src, static_cast<size_t>(size));
    g_return_val_if_fail(rc == EOK, nullptr);
    GST_DEBUG("realOffset is %" G_GSSIZE_FORMAT ", size is %" G_GSSIZE_FORMAT ", src addr: 0x%06" PRIXPTR "",
        realOffset, size, FAKE_POINTER(
            std::static_pointer_cast<OHOS::Media::AVDataSrcMemory>(mem->shmemory)->GetInnerBase() + realOffset));

    CANCEL_SCOPE_EXIT_GUARD(0);
    return copy;
}

static void gst_shmemory_wrap_allocator_init(GstShMemoryWrapAllocator *allocator)
{
    GstAllocator *bAllocator = GST_ALLOCATOR_CAST(allocator);
    g_return_if_fail(bAllocator != nullptr);

    GST_DEBUG_OBJECT(allocator, "init allocator 0x%06" PRIXPTR "", FAKE_POINTER(allocator));

    bAllocator->mem_type = GST_SHMEMORY_WRAP_MEMORY_TYPE;
    bAllocator->mem_map = (GstMemoryMapFunction)gst_shmemory_wrap_allocator_mem_map;
    bAllocator->mem_unmap = (GstMemoryUnmapFunction)gst_shmemory_wrap_allocator_mem_unmap;
    bAllocator->mem_share = (GstMemoryShareFunction)gst_shmemory_wrap_allocator_mem_share;
    bAllocator->mem_copy = (GstMemoryCopyFunction)gst_shmemory_wrap_allocator_mem_copy;
}

static void gst_shmemory_wrap_allocator_finalize(GObject *obj)
{
    GstShMemoryWrapAllocator *allocator = GST_SHMEMORY_WRAP_ALLOCATOR_CAST(obj);
    g_return_if_fail(allocator != nullptr);

    GST_DEBUG_OBJECT(allocator, "finalize allocator 0x%06" PRIXPTR "", FAKE_POINTER(allocator));
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void gst_shmemory_wrap_allocator_class_init(GstShMemoryWrapAllocatorClass *klass)
{
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    g_return_if_fail(gobjectClass != nullptr);

    gobjectClass->finalize = gst_shmemory_wrap_allocator_finalize;

    GstAllocatorClass *allocatorClass = GST_ALLOCATOR_CLASS(klass);
    g_return_if_fail(allocatorClass != nullptr);

    allocatorClass->alloc = gst_shmemory_wrap_allocator_alloc;
    allocatorClass->free = gst_shmemory_wrap_allocator_free;
}