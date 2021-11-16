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
    constexpr int32_t DEFAULT_SURFACE_QUEUE_SIZE = 6;
    constexpr int32_t DEFAULT_SURFACE_SIZE = 1024 * 1024;
    constexpr int32_t DEFAULT_VIDEO_WIDTH = 1920;
    constexpr int32_t DEFAULT_VIDEO_HEIGHT = 1080;
}

enum {
    PROP_0,
    PROP_STREAM_TYPE,
    PROP_SURFACE_WIDTH,
    PROP_SURFACE_HEIGHT,
    PROP_SURFACE_BUFFER_NUM,
    PROP_SURFACE,
};

G_DEFINE_ABSTRACT_TYPE(GstSurfacePoolSrc, gst_surface_pool_src, GST_TYPE_BASE_SRC);

static void gst_surface_pool_src_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_surface_pool_src_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static gboolean gst_surface_pool_src_query(GstBaseSrc *src, GstQuery *query);
static GstStateChangeReturn gst_surface_pool_src_change_state(GstElement *element, GstStateChange transition);
static gboolean gst_surface_pool_src_create_surface(GstSurfacePoolSrc *src);
static gboolean gst_surface_pool_src_create_pool(GstSurfacePoolSrc *src);
static void gst_surface_pool_src_destroy_surface(GstSurfacePoolSrc *src);
static void gst_surface_pool_src_destroy_pool(GstSurfacePoolSrc *src);
static void gst_surface_pool_src_init_surface(GstSurfacePoolSrc *src);
static gboolean gst_surface_pool_src_decide_allocation(GstBaseSrc *basesrc, GstQuery *query);
static gboolean gst_surface_pool_src_is_seekable(GstBaseSrc *basesrc);
static GstFlowReturn gst_surface_pool_src_fill(GstBaseSrc *src, guint64 offset, guint size, GstBuffer *buf);

static void gst_surface_pool_src_class_init(GstSurfacePoolSrcClass *klass)
{
    g_return_if_fail(klass != nullptr);
    GObjectClass *gobject_class = reinterpret_cast<GObjectClass *>(klass);
    GstElementClass *gstelement_class = reinterpret_cast<GstElementClass *>(klass);
    GstBaseSrcClass *gstbasesrc_class = reinterpret_cast<GstBaseSrcClass *>(klass);

    gobject_class->set_property = gst_surface_pool_src_set_property;
    gobject_class->get_property = gst_surface_pool_src_get_property;

    g_object_class_install_property(gobject_class, PROP_VIDEO_WIDTH,
        g_param_spec_uint("video-width", "Video width",
            "Video width", 0, G_MAXINT32, 0,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_VIDEO_HEIGHT,
        g_param_spec_uint("video-height", "Video height",
            "Video height", 0, G_MAXINT32, 0,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_BUFFER_NUM,
        g_param_spec_uint("buffer-num", "buffer num",
            "buffer num", 0, G_MAXINT32, 0,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_SURFACE,
        g_param_spec_pointer("surface", "Surface", "Surface for buffer",
            (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    gst_element_class_set_static_metadata(gstelement_class,
        "Surface pool source", "Source/Surface",
        "Retrieve frame from surface buffer queue", "OpenHarmony");

    gstelement_class->change_state = gst_surface_pool_src_change_state;

    gstbasesrc_class->is_seekable = gst_surface_pool_src_is_seekable;

    gstbasesrc_class->query = gst_surface_pool_src_query;
    gstbasesrc_class->fill = gst_surface_pool_src_fill;
    gstbasesrc_class->decide_allocation = gst_surface_pool_src_decide_allocation;
}

static void gst_surface_pool_src_init(GstSurfacePoolSrc *src)
{
    g_return_if_fail(src != nullptr);

    src->videoWidth = DEFAULT_VIDEO_WIDTH;
    src->videoHeight = DEFAULT_VIDEO_HEIGHT;
    src->pool = nullptr;
}

static gboolean gst_surface_pool_src_is_seekable(GstBaseSrc *basesrc)
{
    (void)basesrc;
    return FALSE;
}

static void gst_surface_pool_src_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GstSurfacePoolSrc *src = GST_SURFACE_POOL_SRC(object);
    g_return_if_fail(src != nullptr);
    (void)pspec;
    switch (prop_id) {
        case PROP_VIDEO_WIDTH:
            src->videoWidth = g_value_get_uint(value);
            break;
        case PROP_VIDEO_HEIGHT:
            src->videoHeight = g_value_get_uint(value);
            break;
        default:
            break;
    }
}

static void gst_surface_pool_src_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GstSurfacePoolSrc *src = GST_SURFACE_POOL_SRC(object);
    g_return_if_fail(src != nullptr);
    (void)pspec;
    switch (prop_id) {
        case PROP_VIDEO_WIDTH:
            g_value_set_uint(value, src->videoWidth);
            break;
        case PROP_VIDEO_HEIGHT:
            g_value_set_uint(value, src->videoHeight);
            break;
        case PROP_BUFFER_NUM:
            g_value_set_uint(value, src->videoHeight);
            break;
        case PROP_SURFACE:
            g_value_set_pointer(value, src->producerSurface.GetRefPtr());
            break;
        default:
            break;
    }
}

static gboolean gst_surface_pool_src_query(GstBaseSrc *src, GstQuery *query)
{
    switch (GST_QUERY_TYPE (query)) {
        case GST_QUERY_SCHEDULING: {
            gst_query_set_scheduling(query, GST_SCHEDULING_FLAG_SEQUENTIAL, 1, -1, 0);
            gst_query_add_scheduling_mode(query, GST_PAD_MODE_PUSH);
            return TRUE;
        }
        default: {
            return GST_BASE_SRC_CLASS(parent_class)->query (src, query);
        }
    }
    return TRUE;
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
    GstSurfacePoolSrc *src = GST_SURFACE_POOL_SRC(element);
    g_return_val_if_fail(src != nullptr, GST_STATE_CHANGE_FAILURE);
    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            g_return_val_if_fail(gst_surface_pool_src_create_surface(src) != TRUE, GST_STATE_CHANGE_FAILURE);
            g_return_val_if_fail(gst_surface_pool_src_create_pool(src) != TRUE, GST_STATE_CHANGE_FAILURE);
            break;
        default:
            break;
    }
    GstStateChangeReturn ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);

    switch (transition) {
        case GST_STATE_CHANGE_READY_TO_NULL:
            gst_surface_pool_src_destroy_pool(src);
            gst_surface_pool_src_destroy_surface(src);
            break;
        default:
            break;
    }

    return ret;
}

static gboolean gst_surface_pool_src_create_surface(GstSurfacePoolSrc *src)
{
    // The internal function not need judge whether it is empty
    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer();
    g_return_val_if_fail(consumerSurface != nullptr, FALSE);
    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    g_return_val_if_fail(producer != nullptr, FALSE);
    sptr<Surface> producerSurface = Surface::CreateSurfaceAsProducer(producer);
    g_return_val_if_fail(producerSurface != nullptr, FALSE);
    src->consumerSurface = consumerSurface;
    src->producerSurface = producerSurface;

    gst_surface_pool_src_init_surface(src);
    return TRUE;
}

static gboolean gst_surface_pool_src_create_pool(GstSurfacePoolSrc *src)
{
    GstAllocationParams params {};
    gst_allocation_params_init(&params);
    GstBufferPool *pool = gst_consumer_surface_pool_new();
    g_return_val_if_fail(pool != nullptr, FALSE);
    ON_SCOPE_EXIT(0) { gst_object_unref(pool); };
    GstAllocator *allocator = gst_consumer_surface_allocator_new();
    g_return_val_if_fail(allocator != nullptr, FALSE);
    ON_SCOPE_EXIT(1) { gst_object_unref(allocator); };
    gst_consumer_surface_pool_set_surface(pool, src->consumerSurface);
    gst_consumer_surface_allocator_set_surface(allocator, src->consumerSurface);
    // init pool config
    GstStructure *config = gst_buffer_pool_get_config(pool);
    gst_buffer_pool_config_set_allocator(config, allocator, &params);
    gst_object_unref(allocator);
    CANCEL_SCOPE_EXIT_GUARD(1);
    g_return_val_if_fail(gst_buffer_pool_set_config(pool, config) != TRUE, FALSE);
    src->pool = pool;
    CANCEL_SCOPE_EXIT_GUARD(0);
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
    sptr<Surface> surface = src->consumerSurface;
    SurfaceError ret = surface->SetUserData("video_width", std::to_string(src->videoWidth));
    if (ret != SURFACE_ERROR_OK) {
        GST_WARNING_OBJECT(src, "set video width fail");
    }
    ret = surface->SetUserData("video_height", std::to_string(src->videoHeight));
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
    ret = surface->SetDefaultWidthAndHeight(src->videoWidth, src->videoHeight);
    if (ret != SURFACE_ERROR_OK) {
        GST_WARNING_OBJECT(src, "set surface width and height fail");
    }
}

static gboolean gst_surface_pool_src_get_old_pool(GstSurfacePoolSrc *surfaceSrc, GstQuery *query, GstCaps *outcaps,
        guint size, guint minBuf, guint maxBuf)
{
    g_return_val_if_fail(surfaceSrc != nullptr && query != nullptr && surfaceSrc->consumerSurface != nullptr, FALSE);
    if(surfaceSrc->pool == nullptr) {
        return FALSE;
    }
    SurfaceError ret = surfaceSrc->consumerSurface->SetQueueSize(std::max(minBuf, maxBuf));
    if (ret != SURFACE_ERROR_OK) {
        GST_WARNING_OBJECT(surfaceSrc, "set queue size fail");
    }
    GstStructure *config = gst_buffer_pool_get_config(surfaceSrc->pool);
    gst_buffer_pool_config_set_params(config, outcaps, size, minBuf, maxBuf);
    if (gst_buffer_pool_set_config(surfaceSrc->pool, config) != TRUE) {
        GST_WARNING_OBJECT(surfaceSrc, "set config failed");
    }
    gst_query_set_nth_allocation_pool(query, 0, surfaceSrc->pool, size, minBuf, maxBuf);
    GST_DEBUG_OBJECT(surfaceSrc, "set old surface pool success");
    return TRUE;
}

static gboolean gst_surface_pool_src_decide_allocation(GstBaseSrc *basesrc, GstQuery *query)
{
    GstSurfacePoolSrc *surfaceSrc = GST_SURFACE_POOL_SRC(basesrc);
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
    }
    // we use our own pool
    if (pool != nullptr) {
        GST_DEBUG_OBJECT(basesrc, "get pool from downstream, but it is surface buffer, use surface pool only");
        gst_object_unref(pool);
        pool = nullptr;
    }
    if (gst_surface_pool_src_get_old_pool(surfaceSrc, query, outcaps, size, minBuf, maxBuf)) {
        return TRUE;
    }
    return FALSE;
}
