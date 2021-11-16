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

#include "gst_shmem_pool_src.h"
#include <gst/video/video.h>
#include "media_errors.h"
#include "scope_guard.h"

#define gst_shmem_pool_src_parent_class parent_class
using namespace OHOS;
namespace {
    constexpr int32_t DEFAULT_SHMEM_QUEUE_SIZE = 6;
    constexpr int32_t DEFAULT_SHMEM_SIZE = 1024 * 1024;
    constexpr int32_t DEFAULT_VIDEO_WIDTH = 1920;
    constexpr int32_t DEFAULT_VIDEO_HEIGHT = 1080;
}

enum {
    PROP_0,
    PROP_STREAM_TYPE,
    PROP_SHMEM_WIDTH,
    PROP_SHMEM_HEIGHT,
    PROP_SHMEM_BUFFER_NUM,
    PROP_SHMEM,
};

G_DEFINE_ABSTRACT_TYPE(GstShmemPoolSrc, gst_shmem_pool_src, GST_TYPE_BASE_SRC);

static void gst_shmem_pool_src_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_shmem_pool_src_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static gboolean gst_shmem_pool_src_query(GstBaseSrc *src, GstQuery *query);
static GstStateChangeReturn gst_shmem_pool_src_change_state(GstElement *element, GstStateChange transition);
static gboolean gst_shmem_pool_src_create_shmem(GstShmemPoolSrc *src);
static gboolean gst_shmem_pool_src_create_pool(GstShmemPoolSrc *src);
static void gst_shmem_pool_src_destroy_shmem(GstShmemPoolSrc *src);
static void gst_shmem_pool_src_destroy_pool(GstShmemPoolSrc *src);
static void gst_shmem_pool_src_init_shmem(GstShmemPoolSrc *src);
static gboolean gst_shmem_pool_src_decide_allocation(GstBaseSrc *basesrc, GstQuery *query);
static gboolean gst_shmem_pool_src_is_seekable(GstBaseSrc *basesrc);
static GstFlowReturn gst_shmem_pool_src_fill(GstBaseSrc *src, guint64 offset, guint size, GstBuffer *buf);

static void gst_shmem_pool_src_class_init(GstShmemPoolSrcClass *klass)
{
    g_return_if_fail(klass != nullptr);
    GObjectClass *gobject_class = reinterpret_cast<GObjectClass *>(klass);
    GstElementClass *gstelement_class = reinterpret_cast<GstElementClass *>(klass);
    GstBaseSrcClass *gstbasesrc_class = reinterpret_cast<GstBaseSrcClass *>(klass);

    gobject_class->set_property = gst_shmem_pool_src_set_property;
    gobject_class->get_property = gst_shmem_pool_src_get_property;

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

    gst_element_class_set_static_metadata(gstelement_class,
        "Shmem pool source", "Source/Shmem",
        "Retrieve frame from shmem buffer queue", "OpenHarmony");

    gstelement_class->change_state = gst_shmem_pool_src_change_state;

    gstbasesrc_class->is_seekable = gst_shmem_pool_src_is_seekable;

    gstbasesrc_class->query = gst_shmem_pool_src_query;
    gstbasesrc_class->decide_allocation = gst_shmem_pool_src_decide_allocation;
    gstbasesrc_class->create = gst_shmem_pool_src_create;
}

static void gst_shmem_pool_src_init(GstShmemPoolSrc *src)
{
    g_return_if_fail(src != nullptr);

    src->videoWidth = DEFAULT_VIDEO_WIDTH;
    src->videoHeight = DEFAULT_VIDEO_HEIGHT;
    src->pool = nullptr;
}

static gboolean gst_shmem_pool_src_is_seekable(GstBaseSrc *basesrc)
{
    (void)basesrc;
    return FALSE;
}

static void gst_shmem_pool_src_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GstShmemPoolSrc *src = GST_SHMEM_POOL_SRC(object);
    g_return_if_fail(src != nullptr);
    (void)pspec;
    switch (prop_id) {
        case PROP_SHMEM_WIDTH:
            src->videoWidth = g_value_get_uint(value);
            break;
        case PROP_SHMEM_HEIGHT:
            src->videoHeight = g_value_get_uint(value);
            break;
        default:
            break;
    }
}

static void gst_shmem_pool_src_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GstShmemPoolSrc *src = GST_SHMEM_POOL_SRC(object);
    g_return_if_fail(src != nullptr);
    (void)pspec;
    switch (prop_id) {
        case PROP_SHMEM_WIDTH:
            g_value_set_uint(value, src->videoWidth);
            break;
        case PROP_SHMEM_HEIGHT:
            g_value_set_uint(value, src->videoHeight);
            break;
        case PROP_SHMEM_BUFFER_NUM:
            g_value_set_uint(value, src->videoHeight);
            break;
        case PROP_SHMEM:
            g_value_set_pointer(value, src->producerShmem.GetRefPtr());
            break;
        default:
            break;
    }
}

static gboolean gst_shmem_pool_src_query(GstBaseSrc *src, GstQuery *query)
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

static GstFlowReturn gst_shmem_pool_src_fill(GstBaseSrc *src, guint64 offset, guint size, GstBuffer *buf)
{
    (void)src;
    (void)offset;
    (void)size;
    (void)buf;
    return GST_FLOW_OK;
}

static GstStateChangeReturn gst_shmem_pool_src_change_state(GstElement *element, GstStateChange transition)
{
    GstShmemPoolSrc *src = GST_SHMEM_POOL_SRC(element);
    g_return_val_if_fail(src != nullptr, GST_STATE_CHANGE_FAILURE);
    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            break;
        default:
            break;
    }
    GstStateChangeReturn ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);

    switch (transition) {
        case GST_STATE_CHANGE_READY_TO_NULL:
            break;
        default:
            break;
    }

    return ret;
}

static gboolean gst_shmem_pool_src_create_pool(GstShmemPoolSrc *src)
{
    return TRUE;
}

static gboolean gst_shmem_pool_src_decide_allocation(GstBaseSrc *basesrc, GstQuery *query)
{
    GstShmemPoolSrc *shmemSrc = GST_SHMEM_POOL_SRC(basesrc);
    g_return_val_if_fail(basesrc != nullptr && query != nullptr, FALSE);
    GstCaps *outcaps = nullptr;
    GstBufferPool *pool = nullptr;
    guint size = 0;
    guint minBuf = DEFAULT_SHMEM_QUEUE_SIZE;
    guint maxBuf = DEFAULT_SHMEM_QUEUE_SIZE;

    // get caps and save to video info
    gst_query_parse_allocation(query, &outcaps, NULL);
    // get pool and pool info from down stream
    if (gst_query_get_n_allocation_pools(query) > 0) {
        gst_query_parse_nth_allocation_pool(query, 0, &pool, &size, &minBuf, &maxBuf);
    }
    // we use our own pool
    if (pool != nullptr) {
        GST_DEBUG_OBJECT(basesrc, "get pool from downstream, but it is shmem buffer, use shmem pool only");
        gst_object_unref(pool);
        pool = nullptr;
    }
    if (gst_shmem_pool_src_get_old_pool(shmemSrc, query, outcaps, size, minBuf, maxBuf)) {
        return TRUE;
    }
    return FALSE;
}
