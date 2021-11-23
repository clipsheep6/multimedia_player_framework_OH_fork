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

#include "gst_surface_pool_src.h"
#include "gst_consumer_surface_pool.h"
#include "gst_consumer_surface_allocator.h"
#include <gst/video/video.h>
#include "media_errors.h"
#include "surface_buffer.h"
#include "scope_guard.h"

#define gst_surface_pool_src_parent_class parent_class
using namespace OHOS;
namespace {
    constexpr guint MAX_SURFACE_QUEUE_SIZE = 12;
    constexpr guint DEFAULT_SURFACE_QUEUE_SIZE = 8;
    constexpr int32_t DEFAULT_SURFACE_SIZE = 1024 * 1024;
    constexpr int32_t DEFAULT_VIDEO_WIDTH = 1920;
    constexpr int32_t DEFAULT_VIDEO_HEIGHT = 1080;
}

GST_DEBUG_CATEGORY_STATIC(gst_surface_pool_src_debug_category);
#define GST_CAT_DEFAULT gst_surface_pool_src_debug_category
#define DEBUG_INIT \
    GST_DEBUG_CATEGORY_INIT(gst_surface_pool_src_debug_category, "surfacepoolsrc", 0, \
        "debug category for surface pool src base class");

enum {
    PROP_0,
    PROP_SURFACE,
};

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(GstSurfacePoolSrc, gst_surface_pool_src, GST_TYPE_MEM_POOL_SRC, DEBUG_INIT);

static void gst_surface_pool_src_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static GstStateChangeReturn gst_surface_pool_src_change_state(GstElement *element, GstStateChange transition);
static gboolean gst_surface_pool_src_create_surface(GstSurfacePoolSrc *src);
static gboolean gst_surface_pool_src_create_pool(GstSurfacePoolSrc *src);
static void gst_surface_pool_src_destroy_surface(GstSurfacePoolSrc *src);
static void gst_surface_pool_src_destroy_pool(GstSurfacePoolSrc *src);
static gboolean gst_surface_pool_src_decide_allocation(GstBaseSrc *basesrc, GstQuery *query);
static GstFlowReturn gst_surface_pool_src_fill(GstBaseSrc *src, guint64 offset, guint size, GstBuffer *buf);
static void gst_surface_pool_src_init_surface(GstSurfacePoolSrc *src);

static void gst_surface_pool_src_class_init(GstSurfacePoolSrcClass *klass)
{
    g_return_if_fail(klass != nullptr);
    GObjectClass *gobject_class = reinterpret_cast<GObjectClass *>(klass);
    GstElementClass *gstelement_class = reinterpret_cast<GstElementClass *>(klass);
    GstBaseSrcClass *gstbasesrc_class = reinterpret_cast<GstBaseSrcClass *>(klass);

    gobject_class->get_property = gst_surface_pool_src_get_property;

    g_object_class_install_property(gobject_class, PROP_SURFACE,
        g_param_spec_pointer("surface", "Surface", "Surface for buffer",
            (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    gst_element_class_set_static_metadata(gstelement_class,
        "Surface pool source", "Source/Surface",
        "Retrieve frame from surface buffer queue", "OpenHarmony");

    gstelement_class->change_state = gst_surface_pool_src_change_state;
    gstbasesrc_class->fill = gst_surface_pool_src_fill;
    gstbasesrc_class->decide_allocation = gst_surface_pool_src_decide_allocation;
}

static void gst_surface_pool_src_init(GstSurfacePoolSrc *surfacesrc)
{
    g_return_if_fail(surfacesrc != nullptr);
    surfacesrc->pool = nullptr;
}

static void gst_surface_pool_src_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GstSurfacePoolSrc *src = GST_SURFACE_POOL_SRC(object);
    g_return_if_fail(src != nullptr);
    (void)pspec;
    switch (prop_id) {
        case PROP_SURFACE:
            g_value_set_pointer(value, src->producerSurface.GetRefPtr());
            break;
        default:
            break;
    }
}

static GstFlowReturn gst_surface_pool_src_fill(GstBaseSrc *src, guint64 offset, guint size, GstBuffer *buf)
{
    (void)src;
    (void)offset;
    (void)size;
    (void)buf;
    return GST_FLOW_OK;
}

static GstStateChangeReturn gst_surface_pool_src_change_state(GstElement *element, GstStateChange transition)
{
    GstSurfacePoolSrc *surfacesrc = GST_SURFACE_POOL_SRC(element);
    g_return_val_if_fail(surfacesrc != nullptr, GST_STATE_CHANGE_FAILURE);
    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            g_return_val_if_fail(gst_surface_pool_src_create_surface(surfacesrc) != TRUE, GST_STATE_CHANGE_FAILURE);
            g_return_val_if_fail(gst_surface_pool_src_create_pool(surfacesrc) != TRUE, GST_STATE_CHANGE_FAILURE);
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            gst_surface_pool_src_init_surface(surfacesrc);
            break;
        default:
            break;
    }
    GstStateChangeReturn ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);

    switch (transition) {
        case GST_STATE_CHANGE_READY_TO_NULL:
            gst_surface_pool_src_destroy_pool(surfacesrc);
            gst_surface_pool_src_destroy_surface(surfacesrc);
            break;
        default:
            break;
    }

    return ret;
}

static gboolean gst_surface_pool_src_create_surface(GstSurfacePoolSrc *surfacesrc)
{
    // The internal function not need judge whether it is empty
    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer();
    g_return_val_if_fail(consumerSurface != nullptr, FALSE);
    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    g_return_val_if_fail(producer != nullptr, FALSE);
    sptr<Surface> producerSurface = Surface::CreateSurfaceAsProducer(producer);
    g_return_val_if_fail(producerSurface != nullptr, FALSE);
    surfacesrc->consumerSurface = consumerSurface;
    surfacesrc->producerSurface = producerSurface;

    GST_DEBUG_OBJECT(surfacesrc, "create surface");
    return TRUE;
}

static gboolean gst_surface_pool_src_create_pool(GstSurfacePoolSrc *surfacesrc)
{
    GstAllocationParams params {};
    gst_allocation_params_init(&params);
    GstBufferPool *pool = gst_consumer_surface_pool_new();
    g_return_val_if_fail(pool != nullptr, FALSE);
    ON_SCOPE_EXIT(0) { gst_object_unref(pool); };
    GstAllocator *allocator = gst_consumer_surface_allocator_new();
    g_return_val_if_fail(allocator != nullptr, FALSE);
    ON_SCOPE_EXIT(1) { gst_object_unref(allocator); };
    gst_consumer_surface_pool_set_surface(pool, surfacesrc->consumerSurface);
    gst_consumer_surface_allocator_set_surface(allocator, surfacesrc->consumerSurface);
    // init pool config
    GstStructure *config = gst_buffer_pool_get_config(pool);
    gst_buffer_pool_config_set_allocator(config, allocator, &params);
    gst_object_unref(allocator);
    CANCEL_SCOPE_EXIT_GUARD(1);
    g_return_val_if_fail(gst_buffer_pool_set_config(pool, config) != TRUE, FALSE);
    surfacesrc->pool = pool;
    CANCEL_SCOPE_EXIT_GUARD(0);
    GST_DEBUG_OBJECT(surfacesrc, "create surface pool");
    return TRUE;
}

static void gst_surface_pool_src_destroy_pool(GstSurfacePoolSrc *src)
{
    gst_object_unref(src->pool);
    src->pool = nullptr;
}

static void gst_surface_pool_src_destroy_surface(GstSurfacePoolSrc *src)
{
    src->consumerSurface = nullptr;
    src->producerSurface = nullptr;
}

static void gst_surface_pool_src_init_surface(GstSurfacePoolSrc *src)
{
    // The internal function do not need judge whether it is empty
    GstMemPoolSrc *memsrc = GST_MEM_POOL_SRC(src);
    sptr<Surface> surface = src->consumerSurface;
    guint width = DEFAULT_VIDEO_WIDTH;
    guint height = DEFAULT_VIDEO_HEIGHT;
    GST_OBJECT_LOCK(memsrc);
    if (memsrc->caps != nullptr) {
        GstVideoInfo info;
        gst_video_info_init(&info);
        gst_video_info_from_caps(&info, memsrc->caps);
        width = info.width;
        height = info.height;
    }
    GST_OBJECT_UNLOCK(memsrc);
    SurfaceError ret = surface->SetUserData("video_width", std::to_string(width));
    if (ret != SURFACE_ERROR_OK) {
        GST_WARNING_OBJECT(src, "set video width fail");
    }
    ret = surface->SetUserData("video_height", std::to_string(height));
    if (ret != SURFACE_ERROR_OK) {
        GST_WARNING_OBJECT(src, "set video height fail");
    }
    ret = surface->SetQueueSize(DEFAULT_SURFACE_QUEUE_SIZE);
    if (ret != SURFACE_ERROR_OK) {
        GST_WARNING_OBJECT(src, "set queue size fail");
    }
    ret = surface->SetUserData("surface_size", std::to_string(DEFAULT_SURFACE_SIZE));
    if (ret != SURFACE_ERROR_OK) {
        GST_WARNING_OBJECT(src, "set surface size fail");
    }
    ret = surface->SetDefaultWidthAndHeight(width, height);
    if (ret != SURFACE_ERROR_OK) {
        GST_WARNING_OBJECT(src, "set surface width and height fail");
    }
}

static gboolean gst_surface_pool_src_get_pool(GstSurfacePoolSrc *surfacesrc, GstQuery *query, GstCaps *outcaps,
        guint minBuf, guint maxBuf)
{
    g_return_val_if_fail(surfacesrc != nullptr && query != nullptr && surfacesrc->consumerSurface != nullptr, FALSE);
    if(surfacesrc->pool == nullptr) {
        return FALSE;
    }
    GstMemPoolSrc *memsrc = GST_MEM_POOL_SRC(surfacesrc);
    memsrc->buffer_num = maxBuf;
    gboolean is_video = gst_query_find_allocation_meta(query, GST_VIDEO_META_API_TYPE, NULL);
    if (is_video) {
        // when video need update size
        GstVideoInfo info;
        gst_video_info_init(&info);
        gst_video_info_from_caps(&info, outcaps);
        memsrc->buffer_size = info.size;
    }
    GST_INFO_OBJECT(surfacesrc, "update buffer num %u", memsrc->buffer_num);
    SurfaceError ret = surfacesrc->consumerSurface->SetQueueSize(memsrc->buffer_num);
    if (ret != SURFACE_ERROR_OK) {
        GST_WARNING_OBJECT(surfacesrc, "set queue size fail");
    }
    GstStructure *config = gst_buffer_pool_get_config(surfacesrc->pool);
    if (is_video) {
        gst_buffer_pool_config_add_option(config, GST_BUFFER_POOL_OPTION_VIDEO_META);
    }
    gst_buffer_pool_config_set_params(config, outcaps, memsrc->buffer_size, minBuf, maxBuf);
    if (gst_buffer_pool_set_config(surfacesrc->pool, config) != TRUE) {
        GST_WARNING_OBJECT(surfacesrc, "set config failed");
    }
    gst_query_set_nth_allocation_pool(query, 0, surfacesrc->pool, memsrc->buffer_num, minBuf, maxBuf);
    GST_DEBUG_OBJECT(surfacesrc, "set surface pool success");
    return TRUE;
}

static gboolean gst_surface_pool_src_decide_allocation(GstBaseSrc *basesrc, GstQuery *query)
{
    GstSurfacePoolSrc *surfacesrc = GST_SURFACE_POOL_SRC(basesrc);
    g_return_val_if_fail(basesrc != nullptr && query != nullptr, FALSE);
    GstCaps *outcaps = nullptr;
    GstBufferPool *pool = nullptr;
    guint size = 0;
    guint minBuf = DEFAULT_SURFACE_QUEUE_SIZE;
    guint maxBuf = DEFAULT_SURFACE_QUEUE_SIZE;

    // get caps and save to video info
    gst_query_parse_allocation(query, &outcaps, NULL);
    // get pool and pool info from down stream
    if (gst_query_get_n_allocation_pools(query) > 0) {
        gst_query_parse_nth_allocation_pool(query, 0, &pool, &size, &minBuf, &maxBuf);
    } else {
        GstVideoInfo info;
        gst_video_info_init(&info);
        gst_video_info_from_caps(&info, outcaps);
        size = info.size;
    }
    // we use our own pool
    if (pool != nullptr) {
        GST_DEBUG_OBJECT(basesrc, "get pool from downstream, but it is surface buffer, use surface pool only");
        gst_object_unref(pool);
        pool = nullptr;
    }
    if (minBuf > MAX_SURFACE_QUEUE_SIZE || maxBuf > MAX_SURFACE_QUEUE_SIZE) {
        minBuf = MAX_SURFACE_QUEUE_SIZE;
        maxBuf = MAX_SURFACE_QUEUE_SIZE;
    } else if (minBuf > maxBuf) {
        maxBuf = minBuf;
    } else {
        maxBuf = maxBuf == 0 ? DEFAULT_SURFACE_QUEUE_SIZE : maxBuf;
    }
    if (gst_surface_pool_src_get_pool(surfacesrc, query, outcaps, minBuf, maxBuf)) {
        return TRUE;
    }
    return FALSE;
}
