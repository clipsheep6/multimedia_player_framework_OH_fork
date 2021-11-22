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
#include <gst/base/base.h>
#include "media_errors.h"
#include "scope_guard.h"
#include "buffer_type_meta.h"

#define gst_shmem_pool_src_parent_class parent_class
using namespace OHOS;
namespace {
    constexpr guint DEFAULT_SHMEM_BUF_NUM = 8;
}

GST_DEBUG_CATEGORY_STATIC(gst_shmem_pool_src_debug_category);
#define GST_CAT_DEFAULT gst_shmem_pool_src_debug_category
#define DEBUG_INIT \
    GST_DEBUG_CATEGORY_INIT(gst_shmem_pool_src_debug_category, "shmempoolsrc", 0, \
        "debug category for shmem pool src base class");


#define GST_BUFFER_POOL_LOCK(pool)   (g_mutex_lock(&pool->lock))
#define GST_BUFFER_POOL_UNLOCK(pool) (g_mutex_unlock(&pool->lock))
#define GST_BUFFER_POOL_WAIT(pool, endtime) (g_cond_wait_until(&pool->cond, &pool->lock, endtime))
#define GST_BUFFER_POOL_NOTIFY(pool) (g_cond_signal(&pool->cond))

enum {
    PROP_0,
};

struct _GstShmemPoolSrcPrivate
{
    GRecMutex shmem_lock;
    GstTask *shmem_task;
    gboolean task_start;
    GstBufferPool *pool;
    GMutex priv_lock;
    GCond task_condition;
    GstBuffer *available_buffer;
    gboolean flushing;
    GMutex queue_lock;
    GCond queue_condition;
    GstQueueArray *queue;
};

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(GstShmemPoolSrc, gst_shmem_pool_src, GST_TYPE_BASE_SRC, DEBUG_INIT);

static GstStateChangeReturn gst_shmem_pool_src_change_state(GstElement *element, GstStateChange transition);
static gboolean gst_shmem_pool_src_decide_allocation(GstBaseSrc *basesrc, GstQuery *query);
static gboolean gst_shmem_pool_src_start_task(GstShmemPoolSrc *shmemsrc);
static gboolean gst_shmem_pool_src_pause_task(GstShmemPoolSrc *shmemsrc);
static gboolean gst_shmem_pool_src_stop_task(GstShmemPoolSrc *shmemsrc);
static void gst_mem_pool_src_dispose(GObject *object);
static void gst_shmem_pool_src_finalize(GObject * object);
static void gst_shmem_pool_src_flush_queue(GstShmemPoolSrc *src);
static gboolean gst_shmem_pool_src_send_event(GstElement *element, GstEvent *event);
GstBuffer *gst_shmem_pool_src_pull_buffer(GstMemPoolSrc *memsrc);
GstFlowReturn gst_shmem_pool_src_push_buffer(GstMemPoolSrc *memsrc, GstBuffer *buffer);
static GstFlowReturn gst_shmem_pool_src_create(GstBaseSrc *src, guint64 offset, guint size, GstBuffer **buffer);
static void gst_shmem_pool_src_loop(GstShmemPoolSrc *shmemsrc);

static void gst_shmem_pool_src_class_init(GstShmemPoolSrcClass *klass)
{
    g_return_if_fail(klass != nullptr);
    GObjectClass *gobject_class = reinterpret_cast<GObjectClass *>(klass);
    GstElementClass *gstelement_class = reinterpret_cast<GstElementClass *>(klass);
    GstBaseSrcClass *gstbasesrc_class = reinterpret_cast<GstBaseSrcClass *>(klass);
    GstMemPoolSrcClass *gstmemsrc_class = reinterpret_cast<GstMemPoolSrcClass *>(klass);

    gst_element_class_set_static_metadata(gstelement_class,
        "Shmem pool source", "Source/Shmem",
        "Retrieve frame from shmem buffer queue", "OpenHarmony");

    gobject_class->finalize = gst_shmem_pool_src_finalize;
    gobject_class->dispose = gst_mem_pool_src_dispose;
    gstelement_class->change_state = gst_shmem_pool_src_change_state;
    gstelement_class->send_event = gst_shmem_pool_src_send_event;
    gstbasesrc_class->decide_allocation = gst_shmem_pool_src_decide_allocation;
    gstbasesrc_class->create = gst_shmem_pool_src_create;
    gstmemsrc_class->pull_buffer = gst_shmem_pool_src_pull_buffer;
    gstmemsrc_class->push_buffer = gst_shmem_pool_src_push_buffer;
}

static void gst_shmem_pool_src_init(GstShmemPoolSrc *shmemsrc)
{
    g_return_if_fail(shmemsrc != nullptr);
    auto priv = reinterpret_cast<GstShmemPoolSrcPrivate *>(gst_shmem_pool_src_get_instance_private(shmemsrc));
    g_return_if_fail(priv != nullptr);
    shmemsrc->priv = priv;
    g_rec_mutex_init(&shmemsrc->priv->shmem_lock);
    shmemsrc->priv->shmem_task =
        gst_task_new((GstTaskFunction)gst_shmem_pool_src_loop, shmemsrc, NULL);
    gst_task_set_lock(shmemsrc->priv->shmem_task, &shmemsrc->priv->shmem_lock);
    priv->task_start = FALSE;
    g_mutex_init(&priv->priv_lock);
    g_mutex_init(&priv->queue_lock);
    g_cond_init(&priv->task_condition);
    g_cond_init(&priv->queue_condition);
    priv->available_buffer = nullptr;
    priv->flushing = FALSE;
    priv->queue = gst_queue_array_new(16);
}

static void gst_shmem_pool_src_flush_queue(GstShmemPoolSrc *shmemsrc)
{
    g_return_if_fail(shmemsrc != nullptr && shmemsrc->priv != nullptr);
    GstMiniObject *object = nullptr;
    auto *priv = shmemsrc->priv;

    while (!gst_queue_array_is_empty(priv->queue)) {
        object = reinterpret_cast<GstMiniObject *>(gst_queue_array_pop_head(priv->queue));
        if (object) {
            gst_mini_object_unref(object);
        }
    }
}

static void gst_mem_pool_src_dispose(GObject *object)
{
    GstShmemPoolSrc *shmemsrc = GST_SHMEM_POOL_SRC(object);
    g_return_if_fail(shmemsrc != nullptr);
    auto priv = shmemsrc->priv;
    g_return_if_fail(priv != nullptr);
    g_mutex_lock(&priv->queue_lock);
    gst_shmem_pool_src_flush_queue(shmemsrc);
    g_mutex_unlock(&priv->queue_lock);

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void gst_shmem_pool_src_finalize(GObject *object)
{
    GstShmemPoolSrc *shmemsrc = GST_SHMEM_POOL_SRC(object);
    g_return_if_fail(shmemsrc != nullptr);
    auto priv = shmemsrc->priv;
    g_return_if_fail(priv != nullptr);

    GST_DEBUG_OBJECT(object, "finalize");
    g_object_unref(priv->shmem_task);
    g_rec_mutex_clear(&priv->shmem_lock);
    g_mutex_clear(&priv->priv_lock);
    g_mutex_clear(&priv->queue_lock);
    g_cond_clear(&priv->task_condition);
    g_cond_clear(&priv->queue_condition);
    gst_queue_array_free(priv->queue);
}

static void gst_shmem_pool_src_loop(GstShmemPoolSrc *shmemsrc)
{
    g_return_if_fail(shmemsrc != nullptr && shmemsrc->priv != nullptr);
    GstMemPoolSrc *memsrc = GST_MEM_POOL_SRC(shmemsrc);
    auto priv = shmemsrc->priv;
    g_mutex_lock(&priv->priv_lock);
    while (priv->task_start && (priv->pool == nullptr || priv->available_buffer != nullptr)) {
        g_cond_wait(&priv->task_condition, &priv->priv_lock);
    }
    if (!priv->task_start) {
        g_mutex_unlock(&priv->priv_lock);
        return;
    }
    GstBufferPool *pool = reinterpret_cast<GstBufferPool*>(gst_object_ref(shmemsrc->priv->pool));
    g_mutex_unlock(&priv->priv_lock);

    GstBuffer *buffer = nullptr;
    gboolean ret = gst_buffer_pool_acquire_buffer(pool, &buffer, NULL);
    gst_object_unref(pool);
    g_return_if_fail(ret == TRUE && buffer != nullptr);
    g_mutex_lock(&priv->priv_lock);
    priv->available_buffer = buffer;
    g_mutex_unlock(&priv->priv_lock);

    if (gst_mem_pool_src_buffer_available(memsrc) != GST_FLOW_OK) {
        GST_WARNING_OBJECT(shmemsrc, "buffer available failed");
    }
}

GstBuffer *gst_shmem_pool_src_pull_buffer(GstMemPoolSrc *memsrc)
{
    GstShmemPoolSrc *shmemsrc = GST_SHMEM_POOL_SRC(memsrc);
    g_return_val_if_fail(shmemsrc != nullptr && shmemsrc->priv != nullptr, nullptr);
    auto priv = shmemsrc->priv;
    g_mutex_lock(&priv->priv_lock);
    auto buffer = priv->available_buffer;
    priv->available_buffer = nullptr;
    g_cond_signal(&priv->task_condition);
    g_mutex_unlock(&priv->priv_lock);
    return buffer;
}

GstFlowReturn gst_shmem_pool_src_push_buffer(GstMemPoolSrc *memsrc, GstBuffer *buffer)
{
    GstShmemPoolSrc *shmemsrc = GST_SHMEM_POOL_SRC(memsrc);
    g_return_val_if_fail(shmemsrc != nullptr && shmemsrc->priv != nullptr, GST_FLOW_ERROR);
    auto priv = shmemsrc->priv;
    g_mutex_lock(&priv->queue_lock);
    gst_queue_array_push_tail(priv->queue, buffer);
    g_cond_signal(&priv->queue_condition);
    g_mutex_unlock(&priv->queue_lock);
    return GST_FLOW_OK;
}

static gboolean gst_shmem_pool_src_start_task(GstShmemPoolSrc *shmemsrc)
{
    auto priv = shmemsrc->priv;
    g_mutex_lock(&priv->priv_lock);
    shmemsrc->priv->task_start = TRUE;
    g_mutex_unlock(&priv->priv_lock);
    gboolean ret = gst_task_start(shmemsrc->priv->shmem_task);
    return ret;
}

static gboolean gst_shmem_pool_src_pause_task(GstShmemPoolSrc *shmemsrc)
{
    auto priv = shmemsrc->priv;
    gboolean ret = gst_task_pause(shmemsrc->priv->shmem_task);
    g_mutex_lock(&priv->priv_lock);
    shmemsrc->priv->task_start = FALSE;
    g_cond_signal(&shmemsrc->priv->task_condition);
    g_mutex_unlock(&priv->priv_lock);
    return ret;
}

static gboolean gst_shmem_pool_src_stop_task(GstShmemPoolSrc *shmemsrc)
{
    auto priv = shmemsrc->priv;
    // stop will not failed
    (void)gst_task_stop(shmemsrc->priv->shmem_task);
    g_mutex_lock(&priv->priv_lock);
    shmemsrc->priv->task_start = FALSE;
    g_cond_signal(&shmemsrc->priv->task_condition);
    g_mutex_unlock(&priv->priv_lock);
    gboolean ret = gst_task_join(shmemsrc->priv->shmem_task);
    return ret;
}

static GstStateChangeReturn gst_shmem_pool_src_change_state(GstElement *element, GstStateChange transition)
{
    GstShmemPoolSrc *shmemsrc = GST_SHMEM_POOL_SRC(element);
    g_return_val_if_fail(shmemsrc != nullptr && shmemsrc->priv != nullptr, GST_STATE_CHANGE_FAILURE);
    switch (transition) {
        case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
            gst_shmem_pool_src_start_task(shmemsrc);
            break;
        default:
            break;
    }
    GstStateChangeReturn ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);

    switch (transition) {
        case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
            gst_shmem_pool_src_pause_task(shmemsrc);
            break;
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            gst_shmem_pool_src_stop_task(shmemsrc);
            break;
        default:
            break;
    }

    return ret;
}

static gboolean gst_shmem_pool_src_send_event(GstElement *element, GstEvent *event)
{
    GstShmemPoolSrc *shmemsrc = GST_SHMEM_POOL_SRC(element);
    g_return_val_if_fail(shmemsrc != nullptr && shmemsrc->priv != nullptr, FALSE);
    auto priv = shmemsrc->priv;

    switch (GST_EVENT_TYPE(event)) {
        case GST_EVENT_FLUSH_STOP:
            g_mutex_lock(&priv->queue_lock);
            gst_shmem_pool_src_flush_queue(shmemsrc);
            g_mutex_unlock(&priv->queue_lock);
            break;
        default:
            break;
    }

    return GST_ELEMENT_CLASS(parent_class)->send_event(element, event);
}

static GstFlowReturn gst_shmem_pool_src_create(GstBaseSrc *src, guint64 offset, guint size, GstBuffer **buffer)
{
    (void)offset;
    (void)size;
    GstShmemPoolSrc *shmemsrc = GST_SHMEM_POOL_SRC(src);
    g_return_val_if_fail(shmemsrc != nullptr && shmemsrc->priv != nullptr, GST_FLOW_ERROR);
    auto priv = shmemsrc->priv;
    g_mutex_lock(&priv->queue_lock);
    while (gst_queue_array_is_empty(priv->queue)) {
        g_cond_wait(&priv->queue_condition, &priv->queue_lock);
    }
    if (!gst_queue_array_is_empty(priv->queue)) {
        GstMiniObject *obj = reinterpret_cast<GstMiniObject*>(gst_queue_array_pop_head(priv->queue));
        if (GST_IS_BUFFER(obj)) {
            *buffer = GST_BUFFER(obj);
            gsize buf_size = gst_buffer_get_size(*buffer);
            GST_DEBUG_OBJECT(shmemsrc, "get buffer of size %" G_GSIZE_FORMAT "", buf_size);
        }
    }
    g_mutex_unlock(&priv->queue_lock);
    return GST_FLOW_OK;
}

static gboolean gst_shmem_pool_src_set_pool(GstShmemPoolSrc *shmemsrc, GstBufferPool *pool,
                GstQuery *query, guint minBuf, guint maxBuf)
{
    g_return_val_if_fail(shmemsrc != nullptr && shmemsrc->priv != nullptr, FALSE);
    auto priv = shmemsrc->priv;
    GstCaps *outcaps = nullptr;
    GstMemPoolSrc *memsrc = GST_MEM_POOL_SRC(shmemsrc);
    memsrc->buffer_num = maxBuf;
    // buffer size default is buffer_size
    guint size = memsrc->buffer_size;
    // get caps and save to video info
    gst_query_parse_allocation(query, &outcaps, NULL);
    gboolean is_video = gst_query_find_allocation_meta(query, GST_VIDEO_META_API_TYPE, NULL);
    if (is_video) {
        // when video need update size
        GstVideoInfo info;
        gst_video_info_init(&info);
        gst_video_info_from_caps(&info, outcaps);
        size = info.size;
    }
    g_mutex_lock(&priv->priv_lock);
    if (priv->pool) {
        gst_object_unref(priv->pool);
        priv->pool = nullptr;
    }
    if (pool) {
        priv->pool = pool;
    } else {
        // TODO
        priv->pool = nullptr;
    }
    GstStructure *config = gst_buffer_pool_get_config(priv->pool);
    if (is_video) {
        gst_buffer_pool_config_add_option(config, GST_BUFFER_POOL_OPTION_VIDEO_META);
    }
    gst_buffer_pool_config_set_params(config, outcaps, size, minBuf, maxBuf);
    if (gst_buffer_pool_set_config(priv->pool, config) != TRUE) {
        GST_WARNING_OBJECT(shmemsrc, "set config failed");
    }
    g_cond_signal(&priv->task_condition);
    g_mutex_unlock(&priv->priv_lock);
    return TRUE;
}

static gboolean gst_shmem_pool_src_decide_allocation(GstBaseSrc *basesrc, GstQuery *query)
{
    GstShmemPoolSrc *shmemsrc = GST_SHMEM_POOL_SRC(basesrc);
    g_return_val_if_fail(basesrc != nullptr && query != nullptr, FALSE);
    GstBufferPool *pool = nullptr;
    guint size = 0;
    guint minBuf = DEFAULT_SHMEM_BUF_NUM;
    guint maxBuf = DEFAULT_SHMEM_BUF_NUM;

    // get pool and pool info from down stream
    if (gst_query_get_n_allocation_pools(query) > 0) {
        gst_query_parse_nth_allocation_pool(query, 0, &pool, &size, &minBuf, &maxBuf);
    }
    // when the pool does not have GST_BUFFER_TYPE_META_API_TYPE, we use our own pool
    if (pool != nullptr) {
        gst_query_set_nth_allocation_pool(query, 0, nullptr, 0, 0, 0);
        if (!gst_query_find_allocation_meta(query, GST_BUFFER_TYPE_META_API_TYPE, nullptr)) {
            gst_object_unref(pool);
            pool = nullptr;
        }
    }
    if (minBuf > maxBuf) {
        maxBuf = minBuf;
    } else {
        maxBuf = maxBuf == 0 ? DEFAULT_SHMEM_BUF_NUM : maxBuf;
    }
    if (gst_shmem_pool_src_set_pool(shmemsrc, pool, query, minBuf, maxBuf)) {
        return TRUE;
    }
    return FALSE;
}
