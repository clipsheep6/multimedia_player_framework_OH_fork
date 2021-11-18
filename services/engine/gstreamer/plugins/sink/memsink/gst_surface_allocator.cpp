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

#include "gst_surface_allocator.h"
#include "media_log.h"

#define gst_surface_allocator_parent_class parent_class
G_DEFINE_TYPE(GstSurfaceAllocator, gst_surface_allocator, GST_TYPE_ALLOCATOR);

gboolean gst_surface_allocator_set_surface(GstSurfaceAllocator *allocator, OHOS::sptr<OHOS::Surface> surface)
{
    g_return_val_if_fail(allocator != nullptr && surface != nullptr, FALSE);
    allocator->surface = surface;
    return TRUE;
}

GstSurfaceMemory *gst_surface_allocator_alloc(GstSurfaceAllocator *allocator,
    gint width, gint height, PixelFormat format)
{
    g_return_val_if_fail(allocator != nullptr && allocator->surface != nullptr, nullptr);

    static constexpr int32_t strideAlignment = 8;
    OHOS::BufferRequestConfig requestConfig = {
        width, height, strideAlignment, format, HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA, 0
    };
    int32_t releaseFence = -1;
    OHOS::sptr<OHOS::SurfaceBuffer> surfaceBuffer = nullptr;
    OHOS::SurfaceError ret = allocator->surface->RequestBuffer(surfaceBuffer, releaseFence, requestConfig);
    if (ret == OHOS::SurfaceError::SURFACE_ERROR_NO_BUFFER) {
        GST_INFO("there is no more buffers");
    }
    if (ret != OHOS::SurfaceError::SURFACE_ERROR_OK) {
        return nullptr;
    }

    GstSurfaceMemory *memory = reinterpret_cast<GstSurfaceMemory *>(g_slice_alloc0(sizeof(GstSurfaceMemory)));
    g_return_val_if_fail(memory != nullptr, nullptr);

    gst_memory_init(GST_MEMORY_CAST(memory), (GstMemoryFlags)0, GST_ALLOCATOR_CAST(allocator), nullptr,
        surfaceBuffer->GetSize(), 0, 0, surfaceBuffer->GetSize());

    memory->buf = surfaceBuffer;
    memory->fence = releaseFence;
    memory->needRender = FALSE;
    GST_DEBUG("alloc surface buffer for width: %d, height: %d, format: %d, size: %u",
        width, height, format, surfaceBuffer->GetSize());

    return memory;
}

void gst_surface_allocator_free(GstSurfaceAllocator *allocator, GstSurfaceMemory *memory)
{
    g_return_if_fail(memory != nullptr && allocator != nullptr && allocator->surface != nullptr);

    GST_DEBUG("free surface buffer for width: %d, height: %d, format: %d, size: %u, needRender: %d, fence: %d",
        memory->buf->GetWidth(), memory->buf->GetHeight(), memory->buf->GetFormat(), memory->buf->GetSize(),
        memory->needRender, memory->fence);

    if (!memory->needRender) {
        OHOS::SurfaceError ret = allocator->surface->CancelBuffer(memory->buf);
        if (ret != OHOS::SurfaceError::SURFACE_ERROR_OK) {
            GST_ERROR("cancel buffer to surface failed, %d", ret);
        }
    }

    memory->buf = nullptr;
    g_slice_free(GstSurfaceMemory, memory);
}

static GstMemory *gst_surface_allocator_alloc_dummy(GstAllocator *allocator, gsize size, GstAllocationParams *params)
{
    GST_ERROR("using the wrong version to alloc memory");
    (void)allocator;
    (void)size;
    (void)params;
    return nullptr;
}

static void gst_surface_allocator_free_dummy(GstAllocator *allocator, GstMemory *memory)
{
    GST_ERROR("using the wrong version to free memory");

    (void)allocator;
    (void)memory;
}

static gpointer gst_surface_allocator_mem_map(GstMemory *mem, gsize maxsize, GstMapFlags flags)
{
    g_return_val_if_fail(mem != nullptr, nullptr);
    g_return_val_if_fail(gst_is_surface_memory(mem), nullptr);

    GstSurfaceMemory *surfaceMem = reinterpret_cast<GstSurfaceMemory *>(mem);
    g_return_val_if_fail(surfaceMem->buf != nullptr, nullptr);

    return surfaceMem->buf->GetVirAddr();
}

static void gst_surface_allocator_mem_unmap(GstMemory *mem)
{
    (void)mem;
}

static void gst_surface_allocator_init(GstSurfaceAllocator *allocator)
{
    GstAllocator *bAllocator = GST_ALLOCATOR_CAST(allocator);
    g_return_if_fail(bAllocator != nullptr);

    GST_DEBUG_OBJECT(allocator, "init allocator 0x%06" PRIXPTR "", FAKE_POINTER(allocator));

    bAllocator->mem_type = GST_SURFACE_MEMORY_TYPE;
    bAllocator->mem_map = (GstMemoryMapFunction)gst_surface_allocator_mem_map;
    bAllocator->mem_unmap = (GstMemoryUnmapFunction)gst_surface_allocator_mem_unmap;
}

static void gst_surface_allocator_finalize(GObject *obj)
{
    GstSurfaceAllocator *allocator = GST_SURFACE_ALLOCATOR_CAST(obj);
    g_return_if_fail(allocator != nullptr);

    GST_DEBUG_OBJECT(allocator, "finalize allocator 0x%06" PRIXPTR "", FAKE_POINTER(allocator));
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void gst_surface_allocator_class_init(GstSurfaceAllocatorClass *klass)
{
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    g_return_if_fail(gobjectClass != nullptr);

    gobjectClass->finalize = gst_surface_allocator_finalize;

    GstAllocatorClass *allocatorClass = GST_ALLOCATOR_CLASS(klass);
    g_return_if_fail(allocatorClass != nullptr);

    allocatorClass->alloc = gst_surface_allocator_alloc_dummy;
    allocatorClass->free = gst_surface_allocator_free_dummy;
}

GstSurfaceAllocator *gst_surface_allocator_new()
{
    GstSurfaceAllocator *alloc = GST_SURFACE_ALLOCATOR_CAST(g_object_new(
        GST_TYPE_SURFACE_ALLOCATOR, "name", "SurfaceAllocator", nullptr));
    (void)gst_object_ref_sink(alloc);

    return alloc;
}
