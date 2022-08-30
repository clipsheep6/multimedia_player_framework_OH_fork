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

#ifndef GST_PLAYER_VIDEO_RENDERER_ALLCATOR_H
#define GST_PLAYER_VIDEO_RENDERER_ALLCATOR_H

#include <gst/gstallocator.h>
#include "gst_player_video_renderer_hisi.h"

namespace OHOS {
namespace Media {
typedef struct {
  GstMemory mem;
  gsize slice_size;//sizeof(GstMemoryHiVOMMZ) + sizeof(private_handle) + frame size + metadata size
  gpointer data;//private_handle
  gpointer user_data;
  GDestroyNotify notify;
} GstMemorySurfaceBuf;

#define GST_TYPE_ALLOCATOR_SURFACEBUF            (gst_allocator_surfacebuf_get_type())
#define GST_IS_ALLOCATOR_SURFACEBUF(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_ALLOCATOR_SURFACEBUF))
#define GST_IS_ALLOCATOR_SURFACEBUF_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_ALLOCATOR_SURFACEBUF))
#define GST_ALLOCATOR_SURFACEBUF_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_ALLOCATOR_SURFACEBUF, GstAllocatorSurfaceBufClass))
#define GST_ALLOCATOR_SURFACEBUF(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_ALLOCATOR_SURFACEBUF, GstAllocatorSurfaceBuf))
#define GST_ALLOCATOR_SURFACEBUF_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_ALLOCATOR_SURFACEBUF, GstAllocatorSurfaceBufClass))
#define GST_ALLOCATOR_SURFACEBUF_CAST(obj)       ((GstAllocatorSurfaceBuf*)(obj))


typedef struct _GstAllocatorSurfaceBuf GstAllocatorSurfaceBuf;
typedef struct _GstAllocatorSurfaceBufClass GstAllocatorSurfaceBufClass;

typedef struct {
    BufferHandle *bufHandle_;
    SurfaceBuffer *bufffer_;
    int32_t bufferIndex_;
    PrivateHandle *priHandle_;
} SurfaceBufferMng;

struct _GstAllocatorSurfaceBuf {
  GstAllocator parent_;
  GstPlayerVideoRendererCtrl *vsink_;//vsink element指针
  guint32 bufNum_;
  Surface *producerSurface_;
};

struct _GstAllocatorSurfaceBufClass {
  GstAllocatorClass p_class;
};

//@vsink:video sink element 指针
//return: GstAllocatorSurfaceBuf对象指针
GstAllocator* gst_allcator_surfacebuf_new(GstPlayerVideoRendererCtrl *vsink, sptr<Surface> &surface);
void gst_allocator_free_buffer (GstAllocator *allocator, GstMemInfo* item);
} // Media
} // OHOS
#endif