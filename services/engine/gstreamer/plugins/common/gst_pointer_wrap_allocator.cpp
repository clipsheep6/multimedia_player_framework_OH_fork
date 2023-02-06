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

#include "securec.h"
#include "gst_pointer_wrap_allocator.h"
#include "media_log.h"

#define gst_pointer_wrap_allocator_parent_class parent_class
G_DEFINE_TYPE(GstPointerWrapAllocator, gst_pointer_wrap_allocator, GST_TYPE_ALLOCATOR);

GstPointerWrapAllocator *gst_pointer_wrap_allocator_new(void)
{
    GstPointerWrapAllocator *alloc = GST_POINTER_WRAP_ALLOCATOR_CAST(g_object_new(
        GST_TYPE_POINTER_WRAP_ALLOCATOR, "name", "PointerWrapAllocator", nullptr));
    (void)gst_object_ref_sink(alloc);

    return alloc;
}

GstMemory *gst_pointer_wrap(GstAllocator *allocator, std::shared_ptr<OHOS::Media::AVSharedMemory> shmem,
    int32_t offset, int32_t length, FreeMemory free_memory)
{
    g_return_val_if_fail(allocator != nullptr, nullptr);
    g_return_val_if_fail(shmem != nullptr, nullptr);

    GstPointerWrapMemory *memory = reinterpret_cast<GstPointerWrapMemory *>(g_slice_alloc0(sizeof(GstPointerWrapMemory)));
    g_return_val_if_fail(memory != nullptr, nullptr);

    gst_memory_init(GST_MEMORY_CAST(memory), GST_MEMORY_FLAG_NO_SHARE, allocator,
        nullptr, length, 0, 0, length);

    memory->pointer = shmem->GetBase();
    memory->offset = offset;
    memory->length = length;
    memory->free_memory = free_memory;
    GST_DEBUG("wrap memory for size: %" PRIu64 ", addr: 0x%06" PRIXPTR "",
        static_cast<uint64_t>(length), FAKE_POINTER(shmem->GetBase() + offset));

    return GST_MEMORY_CAST(memory);
}

static GstMemory *gst_pointer_wrap_allocator_alloc(GstAllocator *allocator, gsize size, GstAllocationParams *params)
{
    (void)allocator;
    (void)size;
    (void)params;
    return nullptr;
}

static void gst_pointer_wrap_allocator_free(GstAllocator *allocator, GstMemory *memory)
{
    g_return_if_fail(memory != nullptr && allocator != nullptr);
    g_return_if_fail(gst_is_pointer_wrap_memory(memory));

    GstPointerWrapMemory *pointerMem = reinterpret_cast<GstPointerWrapMemory *>(memory);
    GST_DEBUG("free memory for size: %" G_GSIZE_FORMAT ", pointerMem->offset %d, addr: 0x%06" PRIXPTR "",
        memory->maxsize, pointerMem->offset, FAKE_POINTER(pointerMem->pointer + pointerMem->offset));

    pointerMem->pointer = nullptr;
    if (pointerMem->free_memory) {
        pointerMem->free_memory(pointerMem->offset, pointerMem->length);
    }
    g_slice_free(GstPointerWrapMemory, pointerMem);
}

static gpointer gst_pointer_wrap_allocator_mem_map(GstMemory *mem, gsize maxsize, GstMapFlags flags)
{
    (void)maxsize;
    (void)flags;
    g_return_val_if_fail(mem != nullptr, nullptr);
    g_return_val_if_fail(gst_is_pointer_wrap_memory(mem), nullptr);

    GstPointerWrapMemory *pointerMem = reinterpret_cast<GstPointerWrapMemory *>(mem);
    g_return_val_if_fail(pointerMem->pointer != nullptr, nullptr);

    GST_INFO("mem_map, maxsize: %" G_GSIZE_FORMAT ", size: %" G_GSIZE_FORMAT", addr: 0x%06" PRIXPTR "",
        mem->maxsize, mem->size, FAKE_POINTER(pointerMem->pointer + pointerMem->offset));
    
    return pointerMem->pointer + pointerMem->offset;
}

static void gst_pointer_wrap_allocator_mem_unmap(GstMemory *mem)
{
    (void)mem;
}

static GstMemory *gst_pointer_wrap_allocator_mem_share(GstMemory *mem, gssize offset, gssize size)
{
    g_return_val_if_fail(mem != nullptr && reinterpret_cast<GstPointerWrapMemory *>(mem)->pointer != nullptr, nullptr);
    g_return_val_if_fail(offset >= 0 && static_cast<gsize>(offset) < mem->size, nullptr);
    GST_DEBUG("begin gst_pointer_wrap_allocator_mem_share, offset is %" G_GSSIZE_FORMAT ","
        "size is %" G_GSSIZE_FORMAT "", offset, size);
    GstPointerWrapMemory *sub = nullptr;
    GstMemory *parent = nullptr;

    /* find the real parent */
    if ((parent = mem->parent) == NULL) {
        parent = (GstMemory *)mem;
    }
    if (size == -1) {
        size = mem->size - offset;
    }

    sub = g_slice_new0(GstPointerWrapMemory);
    g_return_val_if_fail(sub != nullptr, nullptr);
    /* the shared memory is always readonly */
    GST_DEBUG("mem->maxsize is %u", mem->maxsize);
    gst_memory_init(
        GST_MEMORY_CAST(sub),
        (GstMemoryFlags)(GST_MINI_OBJECT_FLAGS(parent) | GST_MINI_OBJECT_FLAG_LOCK_READONLY),
        mem->allocator,
        GST_MEMORY_CAST(parent),
        mem->maxsize,
        mem->align,
        mem->offset,
        size);
    
    sub->pointer = reinterpret_cast<GstPointerWrapMemory *>(mem)->pointer;
    sub->offset = reinterpret_cast<GstPointerWrapMemory *>(mem)->offset + offset;
    sub->length = size;
    GST_DEBUG("gst_pointer_wrap_allocator_mem_share, addr: 0x%06" PRIXPTR "",
        FAKE_POINTER(sub->pointer + sub->offset));
    return GST_MEMORY_CAST(sub);
}

static GstMemory *gst_pointer_wrap_allocator_mem_copy(GstPointerWrapMemory *mem, gssize offset, gssize size)
{
    g_return_val_if_fail(mem != nullptr && mem->pointer != nullptr, nullptr);
    g_return_val_if_fail(offset >= 0 && offset < mem->length, nullptr);
    GST_DEBUG("in gst_pointer_wrap_allocator_mem_copy, offset is %" G_GSSIZE_FORMAT ", size is %" G_GSSIZE_FORMAT "", offset, size);

    gssize realOffset = static_cast<gssize>(mem->offset) + offset;
    g_return_val_if_fail(realOffset >= 0, nullptr);
    if (size == -1) {
        size = static_cast<gssize>(mem->length) - offset;
    }
    g_return_val_if_fail(size > 0, nullptr);

    GstMemory *copy = gst_allocator_alloc(nullptr, static_cast<gsize>(size), nullptr);
    g_return_val_if_fail(copy != nullptr, nullptr);

    GstMapInfo info = GST_MAP_INFO_INIT;
    if (!gst_memory_map(copy, &info, GST_MAP_READ)) {
        gst_memory_unref(copy);
        GST_ERROR("map failed");
        return nullptr;
    }

    uint8_t *src = mem->pointer + realOffset;
    errno_t rc = memcpy_s(info.data, info.size, src, static_cast<size_t>(size));
    if (rc != EOK) {
        GST_ERROR("memcpy failed");
        gst_memory_unmap(copy, &info);
        gst_memory_unref(copy);
        return nullptr;
    }
    GST_DEBUG("realOffset is %" G_GSSIZE_FORMAT ", size is %" G_GSSIZE_FORMAT ", src addr: 0x%06" PRIXPTR "",
        realOffset, size, FAKE_POINTER(mem->pointer + realOffset));

    gst_memory_unmap(copy, &info);

    return copy;
}

static void gst_pointer_wrap_allocator_init(GstPointerWrapAllocator *allocator)
{
    GstAllocator *bAllocator = GST_ALLOCATOR_CAST(allocator);
    g_return_if_fail(bAllocator != nullptr);

    GST_DEBUG_OBJECT(allocator, "init allocator 0x%06" PRIXPTR "", FAKE_POINTER(allocator));

    bAllocator->mem_type = GST_POINTER_WRAP_MEMORY_TYPE;
    bAllocator->mem_map = (GstMemoryMapFunction)gst_pointer_wrap_allocator_mem_map;
    bAllocator->mem_unmap = (GstMemoryUnmapFunction)gst_pointer_wrap_allocator_mem_unmap;
    bAllocator->mem_share = (GstMemoryShareFunction)gst_pointer_wrap_allocator_mem_share;
    bAllocator->mem_copy = (GstMemoryCopyFunction)gst_pointer_wrap_allocator_mem_copy;
}

static void gst_pointer_wrap_allocator_finalize(GObject *obj)
{
    GstPointerWrapAllocator *allocator = GST_POINTER_WRAP_ALLOCATOR_CAST(obj);
    g_return_if_fail(allocator != nullptr);

    GST_DEBUG_OBJECT(allocator, "finalize allocator 0x%06" PRIXPTR "", FAKE_POINTER(allocator));
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void gst_pointer_wrap_allocator_class_init(GstPointerWrapAllocatorClass *klass)
{
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    g_return_if_fail(gobjectClass != nullptr);

    gobjectClass->finalize = gst_pointer_wrap_allocator_finalize;

    GstAllocatorClass *allocatorClass = GST_ALLOCATOR_CLASS(klass);
    g_return_if_fail(allocatorClass != nullptr);

    allocatorClass->alloc = gst_pointer_wrap_allocator_alloc;
    allocatorClass->free = gst_pointer_wrap_allocator_free;
}