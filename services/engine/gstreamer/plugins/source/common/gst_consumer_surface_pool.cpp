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

#include "gst_consumer_surface_pool.h"
#include "gst_consumer_surface_allocator.h"
#include "gst_consumer_surface_memory.h"
#include "buffer_type_meta.h"
using namespace OHOS;

#define gst_consumer_surface_pool_parent_class parent_class

GST_DEBUG_CATEGORY_STATIC(gst_consumer_surface_pool_debug_category);
#define GST_CAT_DEFAULT gst_consumer_surface_pool_debug_category
#define DEBUG_INIT \
    GST_DEBUG_CATEGORY_INIT(gst_consumer_surface_pool_debug_category, "mempoolsrc", 0, \
        "debug category for mem pool src base class");

G_DEFINE_TYPE_WITH_CODE(GstConsumerSurfacePool, gst_consumer_surface_pool, GST_TYPE_BUFFER_POOL, DEBUG_INIT);

class ConsumerListenerProxy : public IBufferConsumerListener {
public:
    explicit ConsumerListenerProxy(GstConsumerSurfacePool &owner) : owner_(owner) {}
    ~ConsumerListenerProxy() = default;
    DISALLOW_COPY_AND_MOVE(ConsumerListenerProxy);
    void OnBufferAvailable() override;
private:
    GstConsumerSurfacePool &owner_;
};

struct _GstConsumerSurfacePoolPrivate {
    sptr<Surface> consumer_surface;
    guint available_buf_count;
    std::mutex pool_mutex;
    std::condition_variable buffer_available_con;
    gboolean flushing;
    gboolean start;
};

static void gst_consumer_surface_pool_init(GstConsumerSurfacePool *pool);
static void gst_consumer_surface_pool_buffer_available(GstConsumerSurfacePool *pool);
static GstFlowReturn gst_consumer_surface_pool_acquire_buffer(GstBufferPool *pool, GstBuffer **buffer,
    GstBufferPoolAcquireParams *params);
static void gst_consumer_surface_pool_release_buffer(GstBufferPool *pool, GstBuffer *buffer);
static gboolean gst_consumer_surface_pool_stop(GstBufferPool *pool);
static gboolean gst_consumer_surface_pool_start(GstBufferPool *pool);
static void gst_consumer_surface_pool_flush_start(GstBufferPool *pool);
static void gst_consumer_surface_pool_flush_stop(GstBufferPool *pool);

void ConsumerListenerProxy::OnBufferAvailable()
{
    gst_consumer_surface_pool_buffer_available(&owner_);
}

static const gchar **gst_consumer_surface_pool_get_options(GstBufferPool *pool)
{
    static const gchar *options[] = { GST_BUFFER_POOL_OPTION_VIDEO_META, NULL };
    return options;
}

static gboolean gst_consumer_surface_pool_set_config(GstBufferPool *pool, GstStructure *config)
{
    g_return_val_if_fail(pool != nullptr, FALSE);

    GstAllocator *allocator = nullptr;
    (void)gst_buffer_pool_config_get_allocator(config, &allocator, nullptr);
    if (!(allocator && GST_IS_CONSUMER_SURFACE_ALLOCATOR(allocator))) {
        GST_WARNING_OBJECT(pool, "no valid allocator in pool");
        return FALSE;
    }

    return GST_BUFFER_POOL_CLASS(parent_class)->set_config(pool, config);
}

// before unref must stop(deactive)
static void gst_consumer_surface_pool_finalize(GObject *obj)
{
    g_return_if_fail(obj != nullptr);
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL_CAST(obj);
    g_return_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr);
    if (surfacepool->priv->consumer_surface != nullptr) {
        if (surfacepool->priv->consumer_surface->UnregisterConsumerListener() != SURFACE_ERROR_OK) {
            GST_WARNING_OBJECT(surfacepool, "deregister consumer listener fail");
        }
        surfacepool->priv->consumer_surface = nullptr;
    }
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void gst_consumer_surface_pool_class_init(GstConsumerSurfacePoolClass *klass)
{
    g_return_if_fail(klass != nullptr);
    GstBufferPoolClass *poolClass = GST_BUFFER_POOL_CLASS (klass);
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);

    gobjectClass->finalize = gst_consumer_surface_pool_finalize;
    poolClass->get_options = gst_consumer_surface_pool_get_options;
    poolClass->set_config = gst_consumer_surface_pool_set_config;
    poolClass->release_buffer = gst_consumer_surface_pool_release_buffer;
    poolClass->acquire_buffer = gst_consumer_surface_pool_acquire_buffer;
    poolClass->start = gst_consumer_surface_pool_start;
    poolClass->stop = gst_consumer_surface_pool_stop;
    poolClass->flush_start = gst_consumer_surface_pool_flush_start;
    poolClass->flush_stop = gst_consumer_surface_pool_flush_stop;
}

static void gst_consumer_surface_pool_flush_start(GstBufferPool *pool)
{
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(pool);
    g_return_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr);
    std::unique_lock<std::mutex> lock(surfacepool->priv->pool_mutex);
    surfacepool->priv->flushing = TRUE;
    surfacepool->priv->buffer_available_con.notify_all();
}

static void gst_consumer_surface_pool_flush_stop(GstBufferPool *pool)
{
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(pool);
    g_return_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr);
    std::unique_lock<std::mutex> lock(surfacepool->priv->pool_mutex);
    surfacepool->priv->flushing = FALSE;
}

// Disable pre-caching
static gboolean gst_consumer_surface_pool_start(GstBufferPool *pool)
{
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(pool);
    g_return_val_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr, FALSE);
    std::unique_lock<std::mutex> lock(surfacepool->priv->pool_mutex);
    surfacepool->priv->start = TRUE;
    return TRUE;
}

// Disable release buffers
static gboolean gst_consumer_surface_pool_stop(GstBufferPool *pool)
{
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(pool);
    g_return_val_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr, FALSE);
    std::unique_lock<std::mutex> lock(surfacepool->priv->pool_mutex);
    surfacepool->priv->buffer_available_con.notify_all();
    surfacepool->priv->start = FALSE;
    return TRUE;
}

static void gst_consumer_surface_pool_release_buffer(GstBufferPool *pool, GstBuffer *buffer)
{
    g_return_if_fail(pool != nullptr && buffer != nullptr);
    GstMemory *mem = gst_buffer_peek_memory(buffer, 0);
    if (gst_is_consumer_surface_memory(mem)) {
        GstBufferTypeMeta *meta = gst_buffer_get_buffer_type_meta(buffer);
        if (meta != nullptr) {
            GstConsumerSurfaceMemory *surfacemem = reinterpret_cast<GstConsumerSurfaceMemory *>(mem);
            surfacemem->fencefd = meta->fenceFd;
        }
    }
    // the buffer's pool is remove, the buffer will free by allocator.
    gst_buffer_unref(buffer);
}

static GstFlowReturn gst_consumer_surface_pool_acquire_buffer(GstBufferPool *pool, GstBuffer **buffer,
    GstBufferPoolAcquireParams *params)
{
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(pool);
    g_return_val_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr, GST_FLOW_ERROR);
    GstBufferPoolClass *pclass = GST_BUFFER_POOL_GET_CLASS(pool);
    g_return_val_if_fail(pclass != nullptr, GST_FLOW_ERROR);
    if(!pclass->alloc_buffer) {
        return GST_FLOW_NOT_SUPPORTED;
    }
    std::unique_lock<std::mutex> lock(surfacepool->priv->pool_mutex);
    surfacepool->priv->buffer_available_con.wait(lock,
        [surfacepool]() { return surfacepool->priv->available_buf_count > 0 ||
                surfacepool->priv->flushing || !surfacepool->priv->start; });
    if (surfacepool->priv->flushing || !surfacepool->priv->start) {
        return GST_FLOW_FLUSHING;
    }
    GstFlowReturn result = pclass->alloc_buffer(pool, buffer, params);
    g_return_val_if_fail(result == GST_FLOW_OK && *buffer != nullptr, GST_FLOW_ERROR);
    GstMemory *mem = gst_buffer_peek_memory(*buffer, 0);
    if (gst_is_consumer_surface_memory(mem)) {
        GstConsumerSurfaceMemory *surfacemem = reinterpret_cast<GstConsumerSurfaceMemory *>(mem);
        gst_buffer_add_buffer_handle_meta(*buffer, surfacemem->bufferHandle, surfacemem->fencefd);
        GST_BUFFER_PTS(*buffer) = surfacemem->timeStamp;
    }
    surfacepool->priv->available_buf_count--;
    return result;
}

static void gst_consumer_surface_pool_init(GstConsumerSurfacePool *pool)
{
    g_return_if_fail(pool != nullptr);
    auto priv = reinterpret_cast<GstConsumerSurfacePoolPrivate *>
        (gst_consumer_surface_pool_get_instance_private(pool));
    g_return_if_fail(priv != nullptr);
    pool->priv = priv;
    priv->available_buf_count = 0;
    priv->flushing = FALSE;
    priv->start = FALSE;
}

static void gst_consumer_surface_pool_buffer_available(GstConsumerSurfacePool *pool)
{
    g_return_if_fail(pool != nullptr);
    std::unique_lock<std::mutex> lock(pool->priv->pool_mutex);
    if (pool->priv->available_buf_count == 0) {
        pool->priv->buffer_available_con.notify_all();
    }
    pool->priv->available_buf_count++;
}

void gst_consumer_surface_pool_set_surface(GstBufferPool *pool, sptr<Surface> &consumer_surface)
{
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(pool);
    g_return_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr);
    g_return_if_fail(consumer_surface != nullptr && surfacepool->priv->consumer_surface == nullptr);
    surfacepool->priv->consumer_surface = consumer_surface;
    sptr<IBufferConsumerListener> listenerProxy = new (std::nothrow) ConsumerListenerProxy(*surfacepool);
    g_return_if_fail(listenerProxy != nullptr);

    if (consumer_surface->RegisterConsumerListener(listenerProxy) != SURFACE_ERROR_OK) {
        GST_WARNING_OBJECT(surfacepool, "register consumer listener fail");
    }
}

GstBufferPool *gst_consumer_surface_pool_new()
{
    GstBufferPool *pool = GST_BUFFER_POOL_CAST(g_object_new(
        GST_TYPE_CONSUMER_SURFACE_POOL, "name", "consumer_surfacepool", nullptr));
    (void)gst_object_ref_sink(pool);

    return pool;
}
