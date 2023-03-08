/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "gst_video_capture_pool.h"
#include <gst/gst.h>
#include "buffer_type_meta.h"
#include "surface.h"
#include "scope_guard.h"
#include "securec.h"
#include "media_log.h"
#include "media_dfx.h"
#include "media_errors.h"
using namespace OHOS;

#define gst_video_capture_pool_parent_class parent_class

GST_DEBUG_CATEGORY_STATIC(gst_video_capture_pool_debug_category);
#define GST_CAT_DEFAULT gst_video_capture_pool_debug_category

enum {
    PROP_0,
    PROP_CACHED_DATA,
};

G_DEFINE_TYPE(GstVideoCapturePool, gst_video_capture_pool, GST_TYPE_CONSUMER_SURFACE_POOL);

static void gst_video_capture_pool_set_property(GObject *object, guint id, const GValue *value, GParamSpec *pspec);
static GstFlowReturn gst_video_capture_pool_buffer_available(GstConsumerSurfacePool *surfacepool, bool *releasebuffer);
static GstFlowReturn gst_video_capture_pool_find_buffer(GstBufferPool *gstpool, GstBuffer **buffer, bool *found);
static GstFlowReturn gst_video_capture_pool_get_buffer(GstConsumerSurfacePool *surfacepool,
    GstBuffer **buffer, bool *releasebuffer);
static GstFlowReturn gst_video_capture_pool_release_buffer(GstConsumerSurfacePool *surfacepool, bool *releasebuffer);

static void gst_video_capture_pool_class_init(GstVideoCapturePoolClass *klass)
{
    g_return_if_fail(klass != nullptr);
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    GST_DEBUG_CATEGORY_INIT(gst_video_capture_pool_debug_category, "videocapturepool", 0,
        "video capture pool base class");

    gobjectClass->set_property = gst_video_capture_pool_set_property;
   
    g_object_class_install_property(gobjectClass, PROP_CACHED_DATA,
        g_param_spec_boolean("cached-data", "es pause", "es pause",
            FALSE, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));
}

static void gst_video_capture_pool_init(GstVideoCapturePool *pool)
{
    g_return_if_fail(pool != nullptr);
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(pool);
    surfacepool->buffer_available = gst_video_capture_pool_buffer_available;
    surfacepool->find_buffer = gst_video_capture_pool_find_buffer;

    pool->cached_data = false;
    pool->poolMgr = nullptr;
}

GstBufferPool *gst_video_capture_pool_new()
{
    GstBufferPool *pool = GST_BUFFER_POOL_CAST(g_object_new(
        GST_TYPE_VIDEO_CAPTURE_POOL, "name", "video_capture_pool", nullptr));
    (void)gst_object_ref_sink(pool);

    return pool;
}

static void gst_video_capture_pool_set_property(GObject *object, guint id, const GValue *value, GParamSpec *pspec)
{
    (void)pspec;
    GstVideoCapturePool *pool = GST_VIDEO_CAPTURE_POOL(object);
    g_return_if_fail(pool != nullptr && value != nullptr);

    g_mutex_lock(&pool->pool_lock);
    ON_SCOPE_EXIT(0) {
        g_mutex_unlock(&pool->pool_lock);
    };
    switch (id) {
        case PROP_CACHED_DATA:
            if (g_value_get_boolean(value) == true) {
                if (pool->poolMgr == nullptr) {
                    const uint32_t size = 6; // Save up to 6 buffer data.
                    pool->poolMgr = std::make_shared<OHOS::Media::VideoPoolManager>(size);
                    g_return_if_fail(pool->poolMgr != nullptr);
                }

                // If the queue is not empty, it is not supported to cache the suspended data again.
                if (pool->poolMgr->IsBufferQueEmpty()) {
                    pool->cached_data = true;
                }
            } else {
                pool->cached_data = false;
                if (pool->poolMgr != nullptr) {
                    GST_DEBUG_OBJECT(pool, "Received %d frames of data during pause.",
                        pool->poolMgr->GetBufferQueSize());
                }
            }
            break;
        default:
            break;
    }
}

static GstFlowReturn gst_video_capture_pool_buffer_available(GstConsumerSurfacePool *surfacepool, bool *releasebuffer)
{
    g_return_val_if_fail(surfacepool != nullptr && releasebuffer != nullptr, GST_FLOW_ERROR);
    GstVideoCapturePool *pool = GST_VIDEO_CAPTURE_POOL(surfacepool);

    g_mutex_lock(&pool->pool_lock);
    ON_SCOPE_EXIT(0) {
        g_mutex_unlock(&pool->pool_lock);
    };

    *releasebuffer = false;
    if (pool->cached_data) {
        if (pool->poolMgr != nullptr && pool->poolMgr->IsBufferQueFull() == false) {
            GstBuffer *buf;
            if (gst_video_capture_pool_get_buffer(surfacepool, &buf, releasebuffer) == GST_FLOW_OK) {
                (void)pool->poolMgr->PushBuffer(buf);
                *releasebuffer = false;
            } else {
                GST_WARNING_OBJECT(surfacepool, "video capture pool get buffer failed");
            }
        }
    } else {
        if (gst_video_capture_pool_release_buffer(surfacepool, releasebuffer) != GST_FLOW_OK) {
            GST_WARNING_OBJECT(surfacepool, "video capture pool release buffer failed");
        }
    }

    return GST_FLOW_OK;
}

static GstFlowReturn gst_video_capture_pool_find_buffer(GstBufferPool *gstpool, GstBuffer **buffer, bool *found)
{
    GstVideoCapturePool *pool = GST_VIDEO_CAPTURE_POOL(gstpool);
    g_return_val_if_fail(pool != nullptr && buffer != nullptr && found != nullptr, GST_FLOW_ERROR);
    
    g_mutex_lock(&pool->pool_lock);
    ON_SCOPE_EXIT(0) {
        g_mutex_unlock(&pool->pool_lock);
    };
    
    *found = false;
    if (pool->poolMgr != nullptr && pool->poolMgr->GetBufferQueSize() > 0) {
        *buffer = pool->poolMgr->PopBuffer();
        *found = true;
    }

    return GST_FLOW_OK;
}

static GstFlowReturn gst_video_capture_pool_get_buffer(GstConsumerSurfacePool *surfacepool,
    GstBuffer **buffer, bool *releasebuffer)
{
    g_return_val_if_fail(surfacepool != nullptr && buffer != nullptr && surfacepool->get_surface_buffer != nullptr &&
        surfacepool->release_surface_buffer != nullptr && releasebuffer != nullptr, GST_FLOW_ERROR);

    // Get buffer
    OHOS::sptr<OHOS::SurfaceBuffer> surfacebuffer = nullptr;
    gint32 fencefd = -1;
    GstFlowReturn ret = surfacepool->get_surface_buffer(surfacepool, surfacebuffer, fencefd);
    g_return_val_if_fail(ret == GST_FLOW_OK && surfacebuffer != nullptr, GST_FLOW_ERROR);
    *releasebuffer = true;
    ON_SCOPE_EXIT(0) {
        surfacepool->release_surface_buffer(surfacepool, surfacebuffer, fencefd);
    };

    // Get buffer data.
    gint64 timestamp = 0;
    gint32 data_size = 0;
    gboolean end_of_stream = false;
    const OHOS::sptr<OHOS::BufferExtraData>& extraData = surfacebuffer->GetExtraData();
    g_return_val_if_fail(extraData != nullptr, GST_FLOW_ERROR);
    (void)extraData->ExtraGet("timeStamp", timestamp);
    (void)extraData->ExtraGet("dataSize", data_size);
    g_return_val_if_fail(static_cast<gint32>(surfacebuffer->GetSize()) >= data_size, GST_FLOW_ERROR);
    (void)extraData->ExtraGet("endOfStream", end_of_stream);

    // copy data
    GstBuffer *dts_buffer = gst_buffer_new_allocate(nullptr, data_size, nullptr);
    g_return_val_if_fail(dts_buffer != nullptr, GST_FLOW_ERROR);
    ON_SCOPE_EXIT(1) {
        gst_buffer_unref(dts_buffer);
    };

    GstMapInfo info = GST_MAP_INFO_INIT;
    g_return_val_if_fail(gst_buffer_map(dts_buffer, &info, GST_MAP_WRITE) == TRUE, GST_FLOW_ERROR);
    ON_SCOPE_EXIT(2) { // ON_SCOPE_EXIT 2
        gst_buffer_unmap(dts_buffer, &info);
    };

    uint8_t *src = static_cast<uint8_t *>(surfacebuffer->GetVirAddr());
    errno_t rc = memcpy_s(info.data, info.size, src, static_cast<size_t>(data_size));
    g_return_val_if_fail(rc == EOK, GST_FLOW_ERROR);

    // Meta
    GstBufferTypeMeta *meta = (GstBufferTypeMeta *)gst_buffer_add_meta(dts_buffer, GST_BUFFER_TYPE_META_INFO, NULL);
    g_return_val_if_fail(meta != NULL, GST_FLOW_ERROR);

    meta->type = BUFFER_TYPE_HANDLE;
    meta->bufLen = static_cast<uint32_t>(data_size);
    meta->length = static_cast<uint32_t>(data_size);
    meta->bufferFlag = end_of_stream ? BUFFER_FLAG_EOS : 0;
    meta->pixelFormat = surfacebuffer->GetFormat();
    meta->width = static_cast<uint32_t>(surfacebuffer->GetWidth());
    meta->height = static_cast<uint32_t>(surfacebuffer->GetHeight());
    meta->invalidpts = TRUE;

    GST_BUFFER_PTS(dts_buffer) = static_cast<uint64_t>(timestamp);
    GST_DEBUG_OBJECT(surfacepool, "BufferQue video capture buffer size is: %" G_GSIZE_FORMAT ", pts: %"
        G_GUINT64_FORMAT, gst_buffer_get_size(dts_buffer), timestamp);

    CANCEL_SCOPE_EXIT_GUARD(1);
    *buffer = dts_buffer;
    return GST_FLOW_OK;
}

static GstFlowReturn gst_video_capture_pool_release_buffer(GstConsumerSurfacePool *surfacepool, bool *releasebuffer)
{
    g_return_val_if_fail(surfacepool != nullptr && surfacepool->get_surface_buffer != nullptr &&
        surfacepool->release_surface_buffer != nullptr && releasebuffer != nullptr, GST_FLOW_ERROR);

    // Get buffer
    OHOS::sptr<OHOS::SurfaceBuffer> surfacebuffer = nullptr;
    gint32 fencefd = -1;
    GstFlowReturn ret = surfacepool->get_surface_buffer(surfacepool, surfacebuffer, fencefd);
    g_return_val_if_fail(ret == GST_FLOW_OK && surfacebuffer != nullptr, GST_FLOW_ERROR);
    *releasebuffer = true;
    surfacepool->release_surface_buffer(surfacepool, surfacebuffer, fencefd);
    
    return GST_FLOW_OK;
}