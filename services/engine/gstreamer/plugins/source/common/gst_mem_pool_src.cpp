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

#include "gst_mem_pool_src.h"
#include "gst_consumer_mem_pool.h"
#include "gst_consumer_mem_allocator.h"
#include <gst/video/video.h>
#include "media_errors.h"
#include "mem_buffer.h"
#include "scope_guard.h"

#define gst_mem_pool_src_parent_class parent_class
using namespace OHOS;
namespace {
    constexpr int32_t DEFAULT_VIDEO_WIDTH = 1920;
    constexpr int32_t DEFAULT_VIDEO_HEIGHT = 1080;
    constexpr int32_t DEFAULT_BUFFER_SIZE = 41920;
    constexpr int32_t DEFAULT_BUFFER_NUM = 8;
}

enum {
    PROP_0,
    PROP_VIDEO_WIDTH,
    PROP_VIDEO_HEIGHT,
    PROP_BUFFER_NUM,
};

G_DEFINE_ABSTRACT_TYPE(GstMemPoolSrc, gst_mem_pool_src, GST_TYPE_BASE_SRC);

static void gst_mem_pool_src_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_mem_pool_src_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static gboolean gst_mem_pool_src_query(GstBaseSrc *src, GstQuery *query);
static GstStateChangeReturn gst_mem_pool_src_change_state(GstElement *element, GstStateChange transition);
static gboolean gst_mem_pool_src_create_mem(GstMemPoolSrc *src);
static gboolean gst_mem_pool_src_create_pool(GstMemPoolSrc *src);
static void gst_mem_pool_src_destroy_mem(GstMemPoolSrc *src);
static void gst_mem_pool_src_destroy_pool(GstMemPoolSrc *src);
static void gst_mem_pool_src_init_mem(GstMemPoolSrc *src);
static gboolean gst_mem_pool_src_decide_allocation(GstBaseSrc *basesrc, GstQuery *query);
static gboolean gst_mem_pool_src_is_seekable(GstBaseSrc *basesrc);
static GstFlowReturn gst_mem_pool_src_fill(GstBaseSrc *src, guint64 offset, guint size, GstBuffer *buf);

static void gst_mem_pool_src_class_init(GstMemPoolSrcClass *klass)
{
    g_return_if_fail(klass != nullptr);
    GObjectClass *gobject_class = reinterpret_cast<GObjectClass *>(klass);
    GstElementClass *gstelement_class = reinterpret_cast<GstElementClass *>(klass);
    GstBaseSrcClass *gstbasesrc_class = reinterpret_cast<GstBaseSrcClass *>(klass);

    gobject_class->set_property = gst_mem_pool_src_set_property;
    gobject_class->get_property = gst_mem_pool_src_get_property;

    g_object_class_install_property(gobject_class, PROP_VIDEO_WIDTH,
        g_param_spec_uint("video-width", "Video width",
            "Video width", 0, G_MAXINT32, 0,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_VIDEO_HEIGHT,
        g_param_spec_uint("video-height", "Video height",
            "Video height", 0, G_MAXINT32, 0,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_BUFFER_SIZE,
        g_param_spec_uint("buffer-size", "buffer size",
            "buffer size", 0, G_MAXINT32, 0,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_BUFFER_NUM,
        g_param_spec_uint("buffer-num", "buffer num",
            "buffer num", 0, G_MAXINT32, 0,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    gst_element_class_set_static_metadata(gstelement_class,
        "Mem pool source", "Source/Mem",
        "Retrieve frame from mem buffer queue", "OpenHarmony");

    gstelement_class->change_state = gst_mem_pool_src_change_state;

    gstbasesrc_class->is_seekable = gst_mem_pool_src_is_seekable;

    gstbasesrc_class->query = gst_mem_pool_src_query;
}

static void gst_mem_pool_src_init(GstMemPoolSrc *src)
{
    g_return_if_fail(src != nullptr);

    src->videoWidth = DEFAULT_VIDEO_WIDTH;
    src->videoHeight = DEFAULT_VIDEO_HEIGHT;
    src->bufferSize = DEFAULT_BUFFER_SIZE;
    src->bufferNum = DEFAULT_BUFFER_NUM;
    src->pool = nullptr;
}

static gboolean gst_mem_pool_src_is_seekable(GstBaseSrc *basesrc)
{
    (void)basesrc;
    return FALSE;
}

static void gst_mem_pool_src_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GstMemPoolSrc *src = GST_MEM_POOL_SRC(object);
    g_return_if_fail(src != nullptr);
    (void)pspec;
    switch (prop_id) {
        case PROP_VIDEO_WIDTH:
            src->videoWidth = g_value_get_uint(value);
            break;
        case PROP_VIDEO_HEIGHT:
            src->videoHeight = g_value_get_uint(value);
            break;
        case PROP_BUFFER_SIZE:
            src->bufferSize = g_value_get_uint(value);
            break;
        default:
            break;
    }
}

static void gst_mem_pool_src_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GstMemPoolSrc *src = GST_MEM_POOL_SRC(object);
    g_return_if_fail(src != nullptr);
    (void)pspec;
    switch (prop_id) {
        case PROP_VIDEO_WIDTH:
            g_value_set_uint(value, src->videoWidth);
            break;
        case PROP_VIDEO_HEIGHT:
            g_value_set_uint(value, src->videoHeight);
            break;
        case PROP_BUFFER_SIZE:
            g_value_set_uint(value, src->bufferSize);
            break;
        case PROP_BUFFER_NUM:
            g_value_set_uint(value, src->bufferNum);
            break;
        default:
            break;
    }
}

static gboolean gst_mem_pool_src_query(GstBaseSrc *src, GstQuery *query)
{
    switch (GST_QUERY_TYPE (query)) {
        case GST_QUERY_SCHEDULING: {
            gst_query_set_scheduling(query, GST_SCHEDULING_FLAG_SEQUENTIAL, 1, -1, 0);
            gst_query_add_scheduling_mode(query, GST_PAD_MODE_PUSH);
            return TRUE;
        }
        default: {
            return GST_BASE_SRC_CLASS(parent_class)->query(src, query);
        }
    }
    return TRUE;
}

void gst_shmem_pool_src_set_callback(GstMemPoolSrc *poolsrc, BufferAvailable callback,
                                        gpointer user_data, GDestroyNotify notify);