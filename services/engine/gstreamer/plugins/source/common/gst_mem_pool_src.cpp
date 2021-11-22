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

using namespace OHOS;
namespace {
    constexpr int32_t DEFAULT_VIDEO_WIDTH = 1920;
    constexpr int32_t DEFAULT_VIDEO_HEIGHT = 1080;
    constexpr int32_t DEFAULT_BUFFER_SIZE = 41920;
    constexpr int32_t DEFAULT_BUFFER_NUM = 8;
}

#define gst_mem_pool_src_parent_class parent_class

GST_DEBUG_CATEGORY_STATIC(gst_mem_pool_src_debug_category);
#define GST_CAT_DEFAULT gst_mem_pool_src_debug_category
#define DEBUG_INIT \
    GST_DEBUG_CATEGORY_INIT(gst_mem_pool_src_debug_category, "mempoolsrc", 0, \
        "debug category for mem pool src base class");

struct _GstMemPoolSrcPrivate {
    BufferAvailable buffer_available;
    gboolean emit_signals;
    std::mutex emit_mutex;
    GDestroyNotify notify;
    gpointer user_data;
};

enum
{
  /* signals */
  SIGNAL_BUFFER_AVAILABLE,
  /* actions */
  SIGNAL_PULL_BUFFER,
  SIGNAL_PUSH_BUFFER,
  LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_VIDEO_WIDTH,
    PROP_VIDEO_HEIGHT,
    PROP_BUFFER_NUM,
};

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(GstMemPoolSrc, gst_mem_pool_src, GST_TYPE_BASE_SRC, DEBUG_INIT);

static guint gst_mem_pool_src_signals[LAST_SIGNAL] = { 0 };
static void gst_mem_pool_src_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_mem_pool_src_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static gboolean gst_mem_pool_src_query(GstBaseSrc *src, GstQuery *query);
static gboolean gst_mem_pool_src_is_seekable(GstBaseSrc *basesrc);

static void gst_mem_pool_src_class_init(GstMemPoolSrcClass *klass)
{
    g_return_if_fail(klass != nullptr);
    GObjectClass *gobject_class = reinterpret_cast<GObjectClass *>(klass);
    GstElementClass *gstelement_class = reinterpret_cast<GstElementClass *>(klass);
    GstBaseSrcClass *gstbasesrc_class = reinterpret_cast<GstBaseSrcClass *>(klass);

    gobject_class->set_property = gst_mem_pool_src_set_property;
    gobject_class->get_property = gst_mem_pool_src_get_property;
    gobject_class->dispose = gst_mem_pool_dispose;

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

    gst_mem_pool_src_signals[SIGNAL_BUFFER_AVAILABLE] =
        g_signal_new("buffer-available", G_TYPE_FROM_CLASS(gobject_class), G_SIGNAL_RUN_LAST,
            0, NULL, NULL, NULL, GST_TYPE_FLOW_RETURN, 0, G_TYPE_NONE);

    gst_mem_pool_src_signals[SIGNAL_PULL_BUFFER] =
        g_signal_new("pull-buffer", G_TYPE_FROM_CLASS(gobject_class), G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET(GstMemPoolSrcClass, pull_buffer), NULL, NULL, NULL, GST_TYPE_BUFFER, 0, G_TYPE_NONE);

    gst_mem_pool_src_signals[SIGNAL_PUSH_BUFFER] =
        g_signal_new("push-buffer", G_TYPE_FROM_CLASS(gobject_class), G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET(GstMemPoolSrcClass, push_buffer), NULL, NULL, NULL,
            GST_TYPE_FLOW_RETURN, 1, GST_TYPE_BUFFER);

    gst_element_class_set_static_metadata(gstelement_class,
        "Mem pool source", "Source/Mem",
        "Retrieve frame from mem buffer queue", "OpenHarmony");

    gstbasesrc_class->is_seekable = gst_mem_pool_src_is_seekable;
    gstbasesrc_class->query = gst_mem_pool_src_query;
    klass->buffer_available = gst_mem_pool_src_buffer_available;
}

static void gst_mem_pool_src_init(GstMemPoolSrc *memsrc)
{
    g_return_if_fail(memsrc != nullptr);
    auto priv = reinterpret_cast<GstMemPoolSrcPrivate *>(gst_mem_pool_src_get_instance_private(memsrc));
    g_return_if_fail(priv != nullptr);
    GST_OBJECT_LOCK(memsrc);
    memsrc->video_width = DEFAULT_VIDEO_WIDTH;
    memsrc->video_height = DEFAULT_VIDEO_HEIGHT;
    memsrc->buffer_size = DEFAULT_BUFFER_SIZE;
    memsrc->buffer_num = DEFAULT_BUFFER_NUM;
    memsrc->priv = priv;
    priv->buffer_available = nullptr;
    priv->user_data = nullptr;
    priv->notify = nullptr;
    GST_OBJECT_UNLOCK(memsrc);
}

static GstFlowReturn gst_mem_pool_src_buffer_available(GstMemPoolSrc *memsrc)
{
    g_return_val_if_fail(memsrc != nullptr && memsrc->priv != nullptr, FALSE);
    GstFlowReturn ret = GST_FLOW_OK;
    auto priv = memsrc->priv;
    gboolean emit = FALSE; 
    if (priv->buffer_available) {
        ret = priv->buffer_available(memsrc);
    } else {
        {
            std::unique_lock<std::mutex> lock(priv->emit_mutex);
            emit = priv->emit_signals;
        }
        if (emit) {
            g_signal_emit(memsrc, gst_app_src_signals[SIGNAL_BUFFER_AVAILABLE], 0, &ret);
        }
    }
    return ret;
}

static gboolean gst_mem_pool_src_is_seekable(GstBaseSrc *basesrc)
{
    (void)basesrc;
    return FALSE;
}

static void gst_mem_pool_src_dispose(GObject *obj)
{
    GstMemPoolSrc *memsrc = GST_MEM_POOL_SRC(object);
    g_return_if_fail(memsrc != nullptr && memsrc->priv != nullptr);
    auto priv = memsrc->priv;

    GST_OBJECT_LOCK(memsrc);
    if (priv->notify) {
        priv->notify(priv->user_data);
    }
    priv->user_data = nullptr;
    priv->notify = nullptr;
    GST_OBJECT_UNLOCK(memsrc);

    G_OBJECT_CLASS(parent_class)->dispose(obj);
}

static void gst_mem_pool_src_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GstMemPoolSrc *memsrc = GST_MEM_POOL_SRC(object);
    g_return_if_fail(memsrc != nullptr);
    (void)pspec;
    switch (prop_id) {
        case PROP_VIDEO_WIDTH:
            memsrc->video_width = g_value_get_uint(value);
            break;
        case PROP_VIDEO_HEIGHT:
            memsrc->video_height = g_value_get_uint(value);
            break;
        case PROP_BUFFER_SIZE:
            memsrc->buffer_size = g_value_get_uint(value);
            break;
        default:
            break;
    }
}

static void gst_mem_pool_src_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GstMemPoolSrc *memsrc = GST_MEM_POOL_SRC(object);
    g_return_if_fail(memsrc != nullptr);
    (void)pspec;
    switch (prop_id) {
        case PROP_VIDEO_WIDTH:
            g_value_set_uint(value, memsrc->video_width);
            break;
        case PROP_VIDEO_HEIGHT:
            g_value_set_uint(value, memsrc->video_height);
            break;
        case PROP_BUFFER_SIZE:
            g_value_set_uint(value, memsrc->buffer_size);
            break;
        case PROP_BUFFER_NUM:
            g_value_set_uint(value, memsrc->buffer_num);
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

void gst_shmem_pool_src_set_callback(GstMemPoolSrc *memsrc, BufferAvailable callback,
                                        gpointer user_data, GDestroyNotify notify)
{
    g_return_if_fail(memsrc != nullptr && memsrc->priv != nullptr);
    auto priv = memsrc->priv;
    GST_OBJECT_LOCK(memsrc);
    priv->user_data = user_data;
    priv->notify = notify;
    priv->buffer_available = callback;
    GST_OBJECT_UNLOCK(memsrc);
}