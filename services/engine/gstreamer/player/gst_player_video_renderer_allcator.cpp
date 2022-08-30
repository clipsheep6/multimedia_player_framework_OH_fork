/*
 * Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED.
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
#include "gst_player_video_renderer_allcator.h"
#include <stdlib.h>
#include <string.h>
#include <gst/gst.h>
#include "display_type.h"
//#include "display_gralloc_private.h"

namespace OHOS {
namespace Media {

#define GST_ALLOCATOR_SURFACEBUF_NAME "GstAllocatorSurfaceBuf"
#define ALLOCTOR_STRIDE_VDP(w) ((((w) % 256) == 0) ? (w) : (((((w) / 256) % 2) == 0) ? \
    ((((w) / 256) + 1) * 256) : ((((w) / 256) + 2) * 256)))
#define ALLOCTOR_ROUND_UP_128(num) (((num)+127)&~127)
#define ALLOCTOR_ROUND_UP_64(num) (((num)+63)&~63)
#define ALLOCTOR_ROUND_UP_16(num) (((num)+15)&~15)

GType gst_allocator_surfacebuf_get_type (void);
G_DEFINE_TYPE (GstAllocatorSurfaceBuf, gst_allocator_surfacebuf, GST_TYPE_ALLOCATOR);

#define parent_class gst_allocator_surfacebuf_parent_class


static inline void _surfacebuf_init (GstAllocatorSurfaceBuf * allocator, GstMemorySurfaceBuf * mem,
    GstMemoryFlags flags,GstMemory * parent, gsize slice_size,
    gpointer data, gsize maxsize, gsize align, gsize offset, gsize size,
    gpointer user_data, GDestroyNotify notify)
{
    gst_memory_init (GST_MEMORY_CAST (mem),
        flags, GST_ALLOCATOR_CAST(allocator), parent, maxsize, align, offset, size);
    mem->slice_size = slice_size;
    mem->data = data;
    mem->user_data = user_data;
    mem->notify = notify;
    GST_INFO("%s:%d.  data:%p, mem->data:%p", __func__, __LINE__, data, mem->data);
}

static gboolean _surfacebuf_alloc_normal(GstAllocatorSurfaceBuf * allocator, GstAllocationParams *params,
    gsize frameSize, PrivateHandle *priHandle)
{
    GstMemInfo *item = nullptr;
    BufferHandle *bufHandle = nullptr;

    item = (GstMemInfo*)g_malloc(sizeof(GstMemInfo));
    if(item == nullptr) {
        GST_ERROR_OBJECT(allocator, "malloc GstMmzInfo failed");
        return FALSE;
    }

    BufferRequestConfig requestConfig;
    uint32_t width;
    uint32_t height;
    allocator->vsink_->GetSurfaceBufferWidthAndHeight(width, height);
    requestConfig.width = ALLOCTOR_STRIDE_VDP(width);
    requestConfig.height = ALLOCTOR_ROUND_UP_64(height);
    requestConfig.strideAlignment = 0x8;  /* surface当前只支持这个值 */
    requestConfig.format = PIXEL_FMT_YCRCB_420_SP;
    requestConfig.timeout = 0;
    requestConfig.usage =  HBM_USE_MEM_DMA | HBM_USE_CPU_READ | HBM_USE_CPU_WRITE;
    int32_t releaseFence;
    sptr<SurfaceBuffer> surfaceBuffer;

    allocator->producerSurface_->RequestBuffer(surfaceBuffer, releaseFence, requestConfig);
    if (surfaceBuffer == nullptr) {
        g_free(item);
        GST_ERROR("producerSurface_ RequestBuffer failed");
        return FALSE;
    }

    bufHandle = surfaceBuffer->GetBufferHandle();
    printf("[%s :%d] zsy##### RequestBuffer bufHandle:%p\n", __func__, __LINE__, bufHandle);
    GST_INFO_OBJECT(allocator, "request buffer succeed, bufHandle is %p, "
    "width is %u, height is %u,", bufHandle, requestConfig.width, requestConfig.height);

    item->bufHdl_ = bufHandle;
    item->surfaceBufffer_ = surfaceBuffer;
    item->fence_ = releaseFence;
    item->bufferIndex_ = allocator->bufNum_;
    item->priHandle_ = bufHandle;
    allocator->vsink_->PushMemBufToQueue(item);

    allocator->bufNum_++;
    *priHandle = (PrivateHandle)bufHandle;
    GST_INFO("%s:%d.  bufHandle:%p", __func__, __LINE__, bufHandle);
    return TRUE;
}

static GstMemorySurfaceBuf *_surfacebuf_new_block (GstAllocatorSurfaceBuf * allocator, GstMemoryFlags flags, GstAllocationParams *params, gsize size)
{
    GstMemorySurfaceBuf * surfaceBufMem = nullptr;
    gsize sliceSize;
    PrivateHandle priHandle = nullptr;//real frame addr and metadata
    gboolean ret;
    gsize align = params->align;
    gsize offset = params->prefix;
    gsize frameSize = size + params->prefix + params->padding;

    GST_INFO("%s:%d.  params->prefix:%u, params->padding:%u, size:%u",
        __func__, __LINE__, params->prefix, params->padding, size);

    sliceSize = sizeof (GstMemorySurfaceBuf) + sizeof(PrivateHandle) + frameSize;
    surfaceBufMem = (GstMemorySurfaceBuf *)g_malloc(sizeof(GstMemorySurfaceBuf));
    if(surfaceBufMem == nullptr) {
        GST_ERROR("malloc surfacebuf_mem failed");
        return nullptr;
    }

    ret = _surfacebuf_alloc_normal(allocator, params, frameSize, &priHandle);
    if(ret == FALSE){
        GST_ERROR_OBJECT(allocator, "_surfacebuf_new_normal failed");
        g_free(surfaceBufMem);
        return nullptr;
    }

    _surfacebuf_init(allocator, surfaceBufMem, flags, nullptr, sliceSize, (gpointer)priHandle, sliceSize, align,
        offset, (unsigned int)(frameSize), nullptr, nullptr);
    GST_INFO("%s:%d.  mem->data:%p", __func__, __LINE__, surfaceBufMem->data);

    return surfaceBufMem;
}

static GstMemory *gst_allocator_surfacebuf_alloc (GstAllocator *allocator, gsize size,
    GstAllocationParams *params)
{
    GST_ERROR("%s:%d enter", __func__, __LINE__);
//    return  (GstMemory *)_surfacebuf_new_block ((GstAllocatorSurfaceBuf *)allocator,
//        (GstMemoryFlags)(GST_MEMORY_FLAG_ZERO_PREFIXED | GST_MEMORY_FLAG_ZERO_PADDED), params, size);
    return  (GstMemory *)_surfacebuf_new_block ((GstAllocatorSurfaceBuf *)allocator,
        (GstMemoryFlags)(0), params, size);

}

void gst_allocator_free_buffer (GstAllocator *allocator, GstMemInfo* item)
{
    GST_INFO("%s:%d enter", __func__, __LINE__);
    if(item == nullptr){
        GST_INFO("item is nullptr, need not to be free!");
        return;
    }

    GstAllocatorSurfaceBuf *allocatorSurfaceBuf = (GstAllocatorSurfaceBuf *)allocator;
    allocatorSurfaceBuf->producerSurface_->CancelBuffer(item->surfaceBufffer_);

    // free p_pri_handle
    g_free(item->priHandle_);
    // we do not free item here
}

static void gst_allocator_surfacebuf_free (GstAllocator * alloc, GstMemory * mem)
{
    GST_INFO("%s:%d enter", __func__, __LINE__);
    GstMemorySurfaceBuf *bufMem = (GstMemorySurfaceBuf *) mem;
    GstAllocatorSurfaceBuf *allocator = (GstAllocatorSurfaceBuf *)alloc;

    if(!bufMem) {
        return;
    }
    if (bufMem->notify) {
        bufMem->notify (bufMem->user_data);
    }

    allocator->vsink_->SetCancelBufferFlag(true);

    allocator->vsink_->PopMemBufFromQueue(mem, alloc);
    g_free(bufMem);
}

static void gst_allocator_surfacebuf_finalize (GObject * obj)
{
  GstAllocatorSurfaceBuf* allocator_surfacebuf = GST_ALLOCATOR_SURFACEBUF(obj);
  gst_object_unref(allocator_surfacebuf->vsink_);
  G_OBJECT_CLASS(parent_class)->finalize (obj);
  return;
}

static void gst_allocator_surfacebuf_class_init (GstAllocatorSurfaceBufClass * klass)
{
    GObjectClass *gobject_class;
    GstAllocatorClass *allocator_class;

    gobject_class = (GObjectClass *) klass;
    allocator_class = (GstAllocatorClass *) klass;
    gobject_class->finalize = gst_allocator_surfacebuf_finalize;
    allocator_class->alloc = gst_allocator_surfacebuf_alloc;
    allocator_class->free = gst_allocator_surfacebuf_free;
}

static GstMemory *gst_allocator_surfacebuf_mem_copy (GstMemory * mem, gssize offset, gssize size)
{
    /* if we not implement mem_copy here, we allocate mem from surface,but call default mem_copy, it lead segment fault*/
    GST_INFO("copy-last-frame not supported now, return nullptr,src_mem is %p, offset is %d, size is %d",
        mem, offset, size);
    return nullptr;
}

static gpointer gst_memory_surfacebuf_map (const GstMemorySurfaceBuf * mem)
{
  return mem->data;
}

static void gst_memory_surfacebuf_unmap (const GstMemorySurfaceBuf * mem)
{
  (void)mem;
  return ;
}

static void gst_allocator_surfacebuf_init  (GstAllocatorSurfaceBuf * allocator)
{
  GstAllocator *alloc = GST_ALLOCATOR_CAST (allocator);

  alloc->mem_type = GST_ALLOCATOR_SURFACEBUF_NAME;
  alloc->mem_map = (GstMemoryMapFunction) gst_memory_surfacebuf_map;
  alloc->mem_unmap = (GstMemoryUnmapFunction) gst_memory_surfacebuf_unmap;
  alloc->mem_copy = (GstMemoryCopyFunction)gst_allocator_surfacebuf_mem_copy;
  alloc->mem_share = nullptr;
  alloc->mem_is_span = nullptr;
  return;
}

GstAllocator* gst_allcator_surfacebuf_new(GstPlayerVideoRendererCtrl *vsink, sptr<Surface> &surface)
{
    GstAllocatorSurfaceBuf *allocatorSurfaceBuffer = nullptr;
    GST_INFO("%s:%d. enter", __func__, __LINE__);
    if(vsink == nullptr) {
        GST_ERROR("vsink is nullptr, gst_allcator_sme_surfacebuf_new failed");
        return nullptr;
    }

    allocatorSurfaceBuffer = (GstAllocatorSurfaceBuf *)g_object_new (gst_allocator_surfacebuf_get_type(), nullptr);
    if(allocatorSurfaceBuffer == nullptr) {
        GST_ERROR_OBJECT(vsink, "new allocator_surfacebuf failed");
        return nullptr;
    }

    allocatorSurfaceBuffer->vsink_ = vsink;
    allocatorSurfaceBuffer->bufNum_ = 0;
    allocatorSurfaceBuffer->producerSurface_ = surface.GetRefPtr();
    GST_INFO("%s:%d. success", __func__, __LINE__);

    return GST_ALLOCATOR(allocatorSurfaceBuffer);
}
}
} // namespace OHOS
