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
#include "scope_guard.h"
#include "media_dfx.h"
#include "media_log.h"
#include "watchdog.h"
#include "param_wrapper.h"
using namespace OHOS;

#define gst_consumer_surface_pool_parent_class parent_class

GST_DEBUG_CATEGORY_STATIC(gst_consumer_surface_pool_debug_category);
#define GST_CAT_DEFAULT gst_consumer_surface_pool_debug_category

class PoolManager : public OHOS::Media::WatchDog, public NoCopyable {
public:
    explicit PoolManager(GstConsumerSurfacePool &owner, uint32_t timeoutMs) : WatchDog(timeoutMs), owner_(owner) {}
    ~PoolManager() = default;

    void Alarm() override;
private:
    GstConsumerSurfacePool &owner_;
};

struct _GstConsumerSurfacePoolPrivate {
    sptr<Surface> consumer_surface;
    guint available_buf_count;
    GMutex pool_lock;
    GCond buffer_available_con;
    gboolean flushing;
    gboolean start;
    gboolean suspend;
    gboolean is_first_buffer;
    guint32 repeat_interval;
    guint32 max_frame_rate;
    guint64 pre_timestamp;
    GstBuffer *cache_buffer;
    gboolean need_eos_buffer;
    gboolean is_first_buffer_in_for_trace;
    gboolean pause_data;
    FILE *dump_file;
    GstElement *src;
    std::shared_ptr<PoolManager> poolMgr;
};

enum {
    PROP_0,
    PROP_SUSPEND,
    PROP_REPEAT,
    PROP_MAX_FRAME_RATE,
    PROP_NOTIFY_EOS,
    PROP_PAUSE_DATA,
    PROP_SRC,
    PROP_INPUT_DETECTION,
};

G_DEFINE_TYPE_WITH_PRIVATE(GstConsumerSurfacePool, gst_consumer_surface_pool, GST_TYPE_VIDEO_BUFFER_POOL);

class ConsumerListenerProxy : public IBufferConsumerListener, public NoCopyable {
public:
    explicit ConsumerListenerProxy(GstConsumerSurfacePool &owner) : owner_(owner) {}
    ~ConsumerListenerProxy() = default;
    void OnBufferAvailable() override;
private:
    GstConsumerSurfacePool &owner_;
};

static void gst_consumer_surface_pool_set_property(GObject *object, guint id, const GValue *value, GParamSpec *pspec);
static void gst_consumer_surface_pool_init(GstConsumerSurfacePool *pool);
static void gst_consumer_surface_pool_buffer_available(GstConsumerSurfacePool *pool);
static void gst_consumer_surface_pool_notify_timeout(GstConsumerSurfacePool *pool);
static void gst_consumer_surface_pool_set_input_detection(GObject *object, bool enable);
static GstFlowReturn gst_consumer_surface_pool_acquire_buffer(GstBufferPool *pool, GstBuffer **buffer,
    GstBufferPoolAcquireParams *params);
static void gst_consumer_surface_pool_release_buffer(GstBufferPool *pool, GstBuffer *buffer);
static gboolean gst_consumer_surface_pool_stop(GstBufferPool *pool);
static gboolean gst_consumer_surface_pool_start(GstBufferPool *pool);
static void gst_consumer_surface_pool_flush_start(GstBufferPool *pool);
static void gst_consumer_surface_pool_flush_stop(GstBufferPool *pool);
static void add_buffer_info(GstConsumerSurfacePool *pool, GstConsumerSurfaceMemory *mem, GstBuffer *buffer);
static void cache_frame_if_necessary(GstConsumerSurfacePool *pool, GstConsumerSurfaceMemory *mem, GstBuffer *buffer);
static gboolean drop_this_frame(GstConsumerSurfacePool *pool, guint64 new_timestamp,
    guint64 old_timestamp, guint32 frame_rate);
static GstFlowReturn gst_consumer_surface_pool_get_surface_buffer(GstConsumerSurfacePool *pool,
    sptr<SurfaceBuffer> &surface_buffer, gint32 &fencefd);
static void gst_consumer_surface_pool_release_surface_buffer(GstConsumerSurfacePool *pool,
    sptr<SurfaceBuffer> &surface_buffer, gint32 &fencefd);
static void gst_consumer_surface_pool_get_dump_file(GstConsumerSurfacePool *pool);
static void gst_consumer_surface_pool_dump_surfacebuffer(GstConsumerSurfacePool *pool, sptr<SurfaceBuffer> &buffer);
static void gst_consumer_surface_pool_dump_gstbuffer(GstConsumerSurfacePool *pool, GstBuffer *buf);

void ConsumerListenerProxy::OnBufferAvailable()
{
    gst_consumer_surface_pool_buffer_available(&owner_);
}

void PoolManager::Alarm()
{
    gst_consumer_surface_pool_notify_timeout(&owner_);
}

static const gchar **gst_consumer_surface_pool_get_options(GstBufferPool *pool)
{
    (void)pool;
    static const gchar *options[] = { GST_BUFFER_POOL_OPTION_VIDEO_META, nullptr };
    return options;
}

static gboolean gst_consumer_surface_pool_set_config(GstBufferPool *pool, GstStructure *config)
{
    g_return_val_if_fail(pool != nullptr, FALSE);
    g_return_val_if_fail(config != nullptr, FALSE);

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
    auto priv = surfacepool->priv;
    priv->poolMgr = nullptr;
    if (priv->consumer_surface != nullptr) {
        if (priv->consumer_surface->UnregisterConsumerListener() != SURFACE_ERROR_OK) {
            GST_WARNING_OBJECT(surfacepool, "deregister consumer listener fail");
        }
        priv->consumer_surface = nullptr;
    }
    g_mutex_clear(&priv->pool_lock);
    g_cond_clear(&priv->buffer_available_con);
    if (priv->dump_file) {
        (void)fclose(priv->dump_file);
    }
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void gst_consumer_surface_pool_class_init(GstConsumerSurfacePoolClass *klass)
{
    g_return_if_fail(klass != nullptr);
    GstBufferPoolClass *poolClass = GST_BUFFER_POOL_CLASS(klass);
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    GST_DEBUG_CATEGORY_INIT(gst_consumer_surface_pool_debug_category, "surfacepool", 0, "surface pool");
    gobjectClass->set_property = gst_consumer_surface_pool_set_property;
    gobjectClass->finalize = gst_consumer_surface_pool_finalize;

    g_object_class_install_property(gobjectClass, PROP_SUSPEND,
        g_param_spec_boolean("suspend", "Suspend surface", "Suspend surface",
            FALSE, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobjectClass, PROP_REPEAT,
        g_param_spec_uint("repeat", "Repeat frame", "Repeat previous frame after given milliseconds",
            0, G_MAXUINT32, 0, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobjectClass, PROP_MAX_FRAME_RATE,
        g_param_spec_uint("max-framerate", "Max frame rate", "Max frame rate",
            0, G_MAXUINT32, 0, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobjectClass, PROP_NOTIFY_EOS,
        g_param_spec_boolean("notify-eos", "notify eos", "Need notify eos",
            FALSE, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobjectClass, PROP_PAUSE_DATA,
        g_param_spec_boolean("pause-data", "pause data", "pause data",
            FALSE, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobjectClass, PROP_SRC,
        g_param_spec_pointer("src", "Src plugin-in", "Src plugin-in",
            (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobjectClass, PROP_INPUT_DETECTION,
        g_param_spec_boolean("input-detection", "input-detection", "input-detection",
            FALSE, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));
            
    poolClass->get_options = gst_consumer_surface_pool_get_options;
    poolClass->set_config = gst_consumer_surface_pool_set_config;
    poolClass->release_buffer = gst_consumer_surface_pool_release_buffer;
    poolClass->acquire_buffer = gst_consumer_surface_pool_acquire_buffer;
    poolClass->start = gst_consumer_surface_pool_start;
    poolClass->stop = gst_consumer_surface_pool_stop;
    poolClass->flush_start = gst_consumer_surface_pool_flush_start;
    poolClass->flush_stop = gst_consumer_surface_pool_flush_stop;
}

static void gst_consumer_surface_pool_set_property(GObject *object, guint id, const GValue *value, GParamSpec *pspec)
{
    (void)pspec;
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(object);
    g_return_if_fail(surfacepool != nullptr && value != nullptr);
    auto priv = surfacepool->priv;

    g_mutex_lock(&priv->pool_lock);
    ON_SCOPE_EXIT(0) { g_mutex_unlock(&priv->pool_lock); };

    switch (id) {
        case PROP_SUSPEND:
            priv->suspend = g_value_get_boolean(value);
            break;
        case PROP_REPEAT:
            if (g_value_get_uint(value) == 0 && priv->cache_buffer != nullptr) {
                gst_buffer_unref(priv->cache_buffer);
                priv->cache_buffer = nullptr;
            }
            priv->repeat_interval = g_value_get_uint(value) * 1000; // ms * 1000 = us
            break;
        case PROP_MAX_FRAME_RATE:
            priv->max_frame_rate = g_value_get_uint(value);
            break;
        case PROP_NOTIFY_EOS:
            priv->need_eos_buffer = g_value_get_boolean(value);
            g_cond_signal(&priv->buffer_available_con);
            break;
        case PROP_PAUSE_DATA:
            priv->pause_data = g_value_get_boolean(value);
            g_cond_signal(&priv->buffer_available_con);
            break;
        case PROP_SRC:
            priv->src = static_cast<GstElement *>(g_value_get_pointer(value));
            break;
        case PROP_INPUT_DETECTION:
            gst_consumer_surface_pool_set_input_detection(object, g_value_get_boolean(value));
            break;
        default:
            break;
    }
}

static void gst_consumer_surface_pool_flush_start(GstBufferPool *pool)
{
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(pool);
    g_return_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr);
    auto priv = surfacepool->priv;
    g_mutex_lock(&priv->pool_lock);
    if (priv->cache_buffer != nullptr) {
        gst_buffer_unref(priv->cache_buffer);
        priv->cache_buffer = nullptr;
    }

    // clear cache buffer
    while (priv->available_buf_count > 0) {
        sptr<SurfaceBuffer> buffer = nullptr;
        gint32 fencefd = -1;
        gint64 timestamp = 0;
        Rect damage = {0, 0, 0, 0};
        if (priv->consumer_surface->AcquireBuffer(buffer, fencefd, timestamp, damage) == SURFACE_ERROR_OK) {
            gst_consumer_surface_pool_dump_surfacebuffer(surfacepool, buffer);
            (void)priv->consumer_surface->ReleaseBuffer(buffer, fencefd);
        }
        priv->available_buf_count--;
        GST_DEBUG_OBJECT(pool, "Release buffer on flush. Available buffer count %u", priv->available_buf_count);
    }

    surfacepool->priv->flushing = TRUE;
    g_cond_signal(&priv->buffer_available_con);
    g_mutex_unlock(&priv->pool_lock);
}

static void gst_consumer_surface_pool_flush_stop(GstBufferPool *pool)
{
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(pool);
    g_return_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr);
    auto priv = surfacepool->priv;
    g_mutex_lock(&priv->pool_lock);
    surfacepool->priv->flushing = FALSE;
    surfacepool->priv->is_first_buffer = TRUE;
    surfacepool->priv->is_first_buffer_in_for_trace = TRUE;
    g_mutex_unlock(&priv->pool_lock);
}

// Disable pre-caching
static gboolean gst_consumer_surface_pool_start(GstBufferPool *pool)
{
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(pool);
    g_return_val_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr, FALSE);
    auto priv = surfacepool->priv;
    g_mutex_lock(&priv->pool_lock);
    surfacepool->priv->start = TRUE;
    g_mutex_unlock(&priv->pool_lock);
    return TRUE;
}

// Disable release buffers
static gboolean gst_consumer_surface_pool_stop(GstBufferPool *pool)
{
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(pool);
    g_return_val_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr, FALSE);
    auto priv = surfacepool->priv;
    g_mutex_lock(&priv->pool_lock);
    surfacepool->priv->poolMgr = nullptr;
    surfacepool->priv->start = FALSE;
    g_cond_signal(&priv->buffer_available_con);
    g_mutex_unlock(&priv->pool_lock);
    return TRUE;
}

static void gst_consumer_surface_pool_release_buffer(GstBufferPool *pool, GstBuffer *buffer)
{
    g_return_if_fail(pool != nullptr && buffer != nullptr);
    GstMemory *mem = gst_buffer_peek_memory(buffer, 0);
    g_return_if_fail(mem != nullptr);
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

static GstFlowReturn gst_consumer_surface_pool_get_eos_buffer(GstConsumerSurfacePool *surfacepool, GstBuffer **buffer)
{
    g_return_val_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr, GST_FLOW_ERROR);
    g_return_val_if_fail(buffer != nullptr, GST_FLOW_ERROR);
    *buffer = gst_buffer_new();
    g_return_val_if_fail(*buffer != nullptr, GST_FLOW_ERROR);
    uint32_t bufferFlag = BUFFER_FLAG_EOS;
    GstBufferHandleConfig config = { 0, -1, bufferFlag, 0, 0, 0, 0 };
    gst_buffer_add_buffer_handle_meta(*buffer, 0, config);

    surfacepool->priv->need_eos_buffer = FALSE;
    return GST_FLOW_OK;
}

static void gst_consumer_surface_pool_get_repeat_buffer(GstConsumerSurfacePool *surfacepool, GstBuffer **buffer)
{
    auto priv = surfacepool->priv;
    *buffer = priv->cache_buffer;
    gst_buffer_ref(priv->cache_buffer);
    GST_BUFFER_PTS(*buffer) = priv->pre_timestamp + priv->repeat_interval;
    priv->pre_timestamp = GST_BUFFER_PTS(*buffer);
}

static GstFlowReturn gst_consumer_surface_pool_alloc_buffer(GstBufferPool *pool, GstBuffer **buffer,
    GstBufferPoolAcquireParams *params, GstConsumerSurfaceMemory **surfacemem)
{
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(pool);
    g_return_val_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr, GST_FLOW_ERROR);
    GstBufferPoolClass *pclass = GST_BUFFER_POOL_GET_CLASS(pool);
    g_return_val_if_fail(pclass != nullptr && pclass->alloc_buffer != nullptr, GST_FLOW_NOT_SUPPORTED);
    g_return_val_if_fail(surfacemem != nullptr, GST_FLOW_ERROR);

    if (surfacepool->find_buffer) {
        bool found = false;
        g_return_val_if_fail(surfacepool->find_buffer(pool, buffer, &found) == GST_FLOW_OK,
            GST_FLOW_ERROR);
        if (found) {
            gst_consumer_surface_pool_dump_gstbuffer(surfacepool, *buffer);
            return GST_FLOW_OK;
        }
    }

    GstFlowReturn result = pclass->alloc_buffer(pool, buffer, params);
    g_return_val_if_fail(result == GST_FLOW_OK && *buffer != nullptr, GST_FLOW_ERROR);
    GstMemory *mem = gst_buffer_peek_memory(*buffer, 0);
    g_return_val_if_fail(mem != nullptr, GST_FLOW_ERROR);
    if (gst_is_consumer_surface_memory(mem)) {
        *surfacemem = reinterpret_cast<GstConsumerSurfaceMemory*>(mem);
        add_buffer_info(surfacepool, *surfacemem, *buffer);
    }
    gst_consumer_surface_pool_dump_gstbuffer(surfacepool, *buffer);
    return GST_FLOW_OK;
}

static GstFlowReturn gst_consumer_surface_pool_acquire_buffer(GstBufferPool *pool, GstBuffer **buffer,
    GstBufferPoolAcquireParams *params)
{
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(pool);
    g_return_val_if_fail(surfacepool != nullptr && surfacepool->priv != nullptr, GST_FLOW_ERROR);
    GstBufferPoolClass *pclass = GST_BUFFER_POOL_GET_CLASS(pool);
    g_return_val_if_fail(pclass != nullptr && pclass->alloc_buffer != nullptr, GST_FLOW_NOT_SUPPORTED);
    auto priv = surfacepool->priv;
    g_mutex_lock(&priv->pool_lock);
    ON_SCOPE_EXIT(0) { g_mutex_unlock(&priv->pool_lock); };

    while (true) {
        gboolean repeat = FALSE;
        while (priv->available_buf_count == 0 && !priv->flushing && priv->start && !priv->need_eos_buffer) {
            if (priv->repeat_interval == 0 || priv->cache_buffer == nullptr) {
                g_cond_wait(&priv->buffer_available_con, &priv->pool_lock);
            } else if (g_cond_wait_until(&priv->buffer_available_con, &priv->pool_lock, priv->repeat_interval)) {
                repeat = TRUE;
                break;
            }
        }

        if (priv->pause_data && priv->start) {
            g_cond_wait(&priv->buffer_available_con, &priv->pool_lock);
            continue;
        }

        if (priv->flushing || !priv->start) {
            return GST_FLOW_FLUSHING;
        }
        if (priv->available_buf_count == 0 && priv->need_eos_buffer) {
            return gst_consumer_surface_pool_get_eos_buffer(surfacepool, buffer);
        }

        if (repeat && priv->cache_buffer != nullptr) {
            gst_consumer_surface_pool_get_repeat_buffer(surfacepool, buffer);
            break;
        }

        GstConsumerSurfaceMemory *surfacemem = nullptr;
        GstFlowReturn result = gst_consumer_surface_pool_alloc_buffer(pool, buffer, params, &surfacemem);
        g_return_val_if_fail(result == GST_FLOW_OK, result);

        if (!repeat) {
            priv->available_buf_count--;
        }

        // check whether needs to drop frame to ensure the maximum frame rate
        if (surfacemem != nullptr && priv->max_frame_rate > 0 && !priv->is_first_buffer &&
            drop_this_frame(surfacepool, surfacemem->timestamp, priv->pre_timestamp, priv->max_frame_rate)) {
            (void)priv->consumer_surface->ReleaseBuffer(surfacemem->surface_buffer, surfacemem->fencefd);
            if (!priv->flushing && priv->start) {
                continue;
            }
        }
        cache_frame_if_necessary(surfacepool, surfacemem, *buffer);
        break;
    };

    return GST_FLOW_OK;
}

static void gst_consumer_surface_pool_init(GstConsumerSurfacePool *pool)
{
    g_return_if_fail(pool != nullptr);
    auto priv = reinterpret_cast<GstConsumerSurfacePoolPrivate *>
        (gst_consumer_surface_pool_get_instance_private(pool));
    g_return_if_fail(priv != nullptr);
    pool->priv = priv;
    pool->buffer_available = nullptr;
    pool->find_buffer = nullptr;
    pool->get_surface_buffer = gst_consumer_surface_pool_get_surface_buffer;
    pool->release_surface_buffer = gst_consumer_surface_pool_release_surface_buffer;
    priv->available_buf_count = 0;
    priv->flushing = FALSE;
    priv->start = FALSE;
    priv->suspend = FALSE;
    priv->pause_data = FALSE;
    priv->is_first_buffer = TRUE;
    priv->is_first_buffer_in_for_trace = TRUE;
    priv->repeat_interval = 0;
    priv->max_frame_rate = 0;
    priv->pre_timestamp = 0;
    priv->cache_buffer = nullptr;
    priv->need_eos_buffer = FALSE;
    g_mutex_init(&priv->pool_lock);
    g_cond_init(&priv->buffer_available_con);
    priv->src = nullptr;
    priv->poolMgr = nullptr;
    priv->dump_file = nullptr;
    gst_consumer_surface_pool_get_dump_file(pool);
}

static void gst_consumer_surface_pool_buffer_available(GstConsumerSurfacePool *pool)
{
    g_return_if_fail(pool != nullptr && pool->priv != nullptr);
    auto priv = pool->priv;
    g_mutex_lock(&priv->pool_lock);
    ON_SCOPE_EXIT(0) { g_mutex_unlock(&priv->pool_lock); };

    if (priv->poolMgr) {
        priv->poolMgr->Notify();
    }

    if (priv->suspend) {
        if (pool->buffer_available) {
            bool releasebuffer = false;
            if (pool->buffer_available(pool, &releasebuffer) != GST_FLOW_OK) {
                GST_WARNING_OBJECT(pool, "Cache buffer failed.");
            }

            if (releasebuffer) {
                GST_INFO_OBJECT(pool, "release buffer. Available buffer count %u", priv->available_buf_count);
                return;
            }
        } else {
            sptr<SurfaceBuffer> buffer = nullptr;
            gint32 fencefd = -1;
            gint64 timestamp = 0;
            Rect damage = {0, 0, 0, 0};
            if (priv->consumer_surface->AcquireBuffer(buffer, fencefd, timestamp, damage) == SURFACE_ERROR_OK) {
                GST_INFO_OBJECT(pool, "Surface is suspended, release buffer. Available buffer count %u",
                    priv->available_buf_count);
                gst_consumer_surface_pool_dump_surfacebuffer(pool, buffer);
                (void)priv->consumer_surface->ReleaseBuffer(buffer, fencefd);
                return;
            }
        }
    }

    if (priv->available_buf_count == 0) {
        g_cond_signal(&priv->buffer_available_con);
    }

    if (priv->is_first_buffer_in_for_trace) {
        OHOS::Media::MediaTrace::TraceBegin("AVCodecServer::FirstFrame",
            FAKE_POINTER(priv->consumer_surface.GetRefPtr()));
        priv->is_first_buffer_in_for_trace = FALSE;
    }

    pool->priv->available_buf_count++;
    GST_DEBUG_OBJECT(pool, "Available buffer count %u", pool->priv->available_buf_count);
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

static void add_buffer_info(GstConsumerSurfacePool *pool, GstConsumerSurfaceMemory *mem, GstBuffer *buffer)
{
    g_return_if_fail(pool != nullptr && mem != nullptr && buffer != nullptr);
    uint32_t bufferFlag = 0;
    if (mem->is_eos_frame) {
        bufferFlag = BUFFER_FLAG_EOS;
    }
    GstBufferHandleConfig config = { sizeof(*(mem->buffer_handle)), mem->fencefd,
        bufferFlag, mem->data_size, mem->pixel_format, mem->width, mem->height };
    gst_buffer_add_buffer_handle_meta(buffer, reinterpret_cast<intptr_t>(mem->buffer_handle), config);

    if (mem->timestamp < 0) {
        GST_WARNING_OBJECT(pool, "Invalid timestamp: < 0");
        GST_BUFFER_PTS(buffer) = 0;
    } else {
        GST_BUFFER_PTS(buffer) = static_cast<uint64_t>(mem->timestamp);
    }
}

static void cache_frame_if_necessary(GstConsumerSurfacePool *pool, GstConsumerSurfaceMemory *mem, GstBuffer *buffer)
{
    g_return_if_fail(pool != nullptr && pool->priv != nullptr && mem != nullptr && buffer != nullptr);
    auto priv = pool->priv;
    priv->pre_timestamp = static_cast<uint64_t>(mem->timestamp);
    if (priv->is_first_buffer) {
        priv->is_first_buffer = FALSE;
    } else if (priv->repeat_interval > 0) {
        if (priv->cache_buffer != nullptr) {
            gst_buffer_unref(priv->cache_buffer);
        }
        priv->cache_buffer = buffer;
        gst_buffer_ref(priv->cache_buffer);
    }
}

static gboolean drop_this_frame(GstConsumerSurfacePool *pool, guint64 new_timestamp,
    guint64 old_timestamp, guint32 frame_rate)
{
    if (new_timestamp <= old_timestamp) {
        GST_WARNING_OBJECT(pool, "Invalid timestamp: not increased");
        return TRUE;
    }

    if (frame_rate == 0) {
        GST_WARNING_OBJECT(pool, "Invalid frame rate: 0");
        return FALSE;
    }
    guint64 min_interval = 1000000000 / frame_rate; // 1s = 1000000000ns
    if ((UINT64_MAX - min_interval) < old_timestamp) {
        GST_WARNING_OBJECT(pool, "Invalid timestamp: too big");
        return TRUE;
    }

    const guint64 deviations = 3000000; // 3ms
    if (new_timestamp < (old_timestamp - deviations + min_interval)) {
        GST_INFO_OBJECT(pool, "Drop this frame to make sure maximum frame rate");
        return TRUE;
    }
    return FALSE;
}

GstBufferPool *gst_consumer_surface_pool_new()
{
    GstBufferPool *pool = GST_BUFFER_POOL_CAST(g_object_new(
        GST_TYPE_CONSUMER_SURFACE_POOL, "name", "consumer_surfacepool", nullptr));
    (void)gst_object_ref_sink(pool);

    return pool;
}

static void gst_consumer_surface_pool_set_input_detection(GObject *object, bool enable)
{
    GST_DEBUG_OBJECT(object, "set_input_detection enable = %d.", enable);
    GstConsumerSurfacePool *surfacepool = GST_CONSUMER_SURFACE_POOL(object);
    g_return_if_fail(surfacepool != nullptr);
    auto priv = surfacepool->priv;
    g_return_if_fail(priv != nullptr);

    if (enable) {
        if (priv->poolMgr == nullptr) {
            const guint32 timeoutMs = 3000; // Error will be reported if there is no data input in 3000ms by default.
            priv->poolMgr = std::make_shared<PoolManager>(*surfacepool, timeoutMs);
            g_return_if_fail(priv->poolMgr != nullptr);
        }
        
        priv->poolMgr->EnableWatchDog();
    } else {
        if (priv->poolMgr) {
            priv->poolMgr->DisableWatchDog();
        }
    }
}

static void gst_consumer_surface_pool_notify_timeout(GstConsumerSurfacePool *pool)
{
    GST_DEBUG_OBJECT(pool, "Input stream timeout.");
    g_return_if_fail(pool != nullptr && pool->priv != nullptr);
    auto priv = pool->priv;

    if (priv->src) {
        GST_ELEMENT_ERROR (priv->src, RESOURCE, NOT_FOUND,
            ("Input stream timeout, please confirm whether the input is normal."),
            ("Input stream timeout, please confirm whether the input is normal."));
    }
}

static GstFlowReturn gst_consumer_surface_pool_get_surface_buffer(GstConsumerSurfacePool *pool,
    sptr<SurfaceBuffer> &surface_buffer, gint32 &fencefd)
{
    g_return_val_if_fail(pool != nullptr && pool->priv != nullptr, GST_FLOW_ERROR);
    auto priv = pool->priv;

    gint64 timestamp = 0;
    Rect damage = {0, 0, 0, 0};
    if (priv->consumer_surface->AcquireBuffer(surface_buffer, fencefd, timestamp, damage) == SURFACE_ERROR_OK) {
        return GST_FLOW_OK;
    }

    return GST_FLOW_ERROR;
}

static void gst_consumer_surface_pool_release_surface_buffer(GstConsumerSurfacePool *pool,
    sptr<SurfaceBuffer> &surface_buffer, gint32 &fencefd)
{
    g_return_if_fail(pool != nullptr && pool->priv != nullptr);
    auto priv = pool->priv;

    (void)priv->consumer_surface->ReleaseBuffer(surface_buffer, fencefd);
}

static void gst_consumer_surface_pool_get_dump_file(GstConsumerSurfacePool *pool)
{
    g_return_if_fail(pool != nullptr && pool->priv != nullptr);
    auto priv = pool->priv;

    std::string dump_enable;
    int32_t res = OHOS::system::GetStringParameter("sys.media.dump.surfacesrc.enable", dump_enable, "");
    if (res != 0 || dump_enable.empty()) {
        GST_DEBUG_OBJECT(pool, "sys.media.dump.surfacesrc.enable");
        return;
    }
    GST_DEBUG_OBJECT(pool, "sys.media.dump.surfacesrc.enable=%s", dump_enable.c_str());

    if (dump_enable == "true") {
        std::string input_dump_file = "/data/media/surface_in" +
            std::to_string(static_cast<int32_t>(FAKE_POINTER(pool))) + ".es_yuv";

        priv->dump_file = fopen(input_dump_file.c_str(), "ab+");
        if (priv->dump_file == nullptr) {
            GST_ERROR_OBJECT(pool, "open file failed");
            return;
        }
    }
}

static void gst_consumer_surface_pool_dump_data(FILE *dump_file, const void *addr,
    gint32 size, gint32 width, gint32 height)
{
    g_return_if_fail(dump_file != nullptr && addr != nullptr);
    gint32 data_size = size;
    
    if (width != 0 && height != 0) {
        // The size of non-es streams needs to be adjusted, only dump video data
        gint32 rgbaSize = width * height * 4;   // rgba = w * h * 4
        gint32 yuvSize = width * height * 3 / 2; // yuv = w * h * 3 / 2
        if (size > rgbaSize) {
            data_size = rgbaSize;
        } else if (size > yuvSize) {
            data_size = yuvSize;
        }
    }

    (void)fwrite(addr, data_size, 1, dump_file);
    (void)fflush(dump_file);
    return;
}

static void gst_consumer_surface_pool_dump_surfacebuffer(GstConsumerSurfacePool *pool, sptr<SurfaceBuffer> &buffer)
{
    g_return_if_fail(pool != nullptr && pool->priv != nullptr && buffer != nullptr);

    if (pool->priv->dump_file == nullptr) {
        return;
    }

    gint32 data_size = 0;
    const sptr<OHOS::BufferExtraData>& extraData = buffer->GetExtraData();
    if (extraData != nullptr) {
        (void)extraData->ExtraGet("dataSize", data_size);
    }

    gst_consumer_surface_pool_dump_data(pool->priv->dump_file, buffer->GetVirAddr(),
        data_size, buffer->GetWidth(), buffer->GetHeight());
    return;
}

static void gst_consumer_surface_pool_dump_gstbuffer(GstConsumerSurfacePool *pool, GstBuffer *buf)
{
    g_return_if_fail(pool != nullptr && pool->priv != nullptr && buf != nullptr);

    if (pool->priv->dump_file == nullptr) {
        return;
    }

    GstBufferTypeMeta *meta = gst_buffer_get_buffer_type_meta(buf);
    g_return_if_fail(meta != nullptr);
    GstMapInfo info = GST_MAP_INFO_INIT;
    gst_buffer_map(buf, &info, GST_MAP_READ);
    gst_consumer_surface_pool_dump_data(pool->priv->dump_file, info.data,
        static_cast<gint32>(meta->length), static_cast<gint32>(meta->width), static_cast<gint32>(meta->height));
    gst_buffer_unmap(buf, &info);
    return;
}