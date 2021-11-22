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

#include "gst_shared_mem_sink.h"
#include <cinttypes>
#include "gst_shmem_pool.h"
#include "avsharedmemorypool.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr guint32 DEFAULT_PROP_MEM_SIZE = 0; // 0 is meanless
    constexpr guint32 DEFAULT_PROP_MEM_PREFIX_SIZE = 0;
    constexpr gboolean DEFAULT_PROP_REMOTE_REFCOUNT = FALSE;
}

struct _GstSharedMemSinkPrivate {
    guint memSize;
    guint memPrefixSize;
    gboolean enableRemoteRefCount;
    GstShMemPool *pool;
    GstShMemAllocator *allocator;
    GstAllocationParams allocParams;
    std::shared_ptr<OHOS::Media::AVSharedMemoryPool> avShmemPool;
};

enum {
    PROP_0,
    PROP_MEM_SIZE,
    PROP_MEM_PREFIX_SIZE,
    PROP_ENABLE_REMOTE_REFCOUNT,
};

static void gst_shared_mem_sink_dispose(GObject *obj);
static void gst_shared_mem_sink_finalize(GObject *object);
static void gst_shared_mem_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_shared_mem_sink_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static gboolean gst_shared_mem_sink_do_propose_allocation(GstMemSink *memsink, GstQuery *query);
static GstFlowReturn gst_shared_mem_sink_do_stream_render(GstMemSink *memsink, GstBuffer **buffer);
static GstFlowReturn gst_shared_mem_sink_do_app_render(GstMemSink *memsink, GstBuffer *buffer);

#define gst_shared_mem_sink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstSharedMemSink, gst_shared_mem_sink,
                        GST_TYPE_MEM_SINK, G_ADD_PRIVATE(GstSharedMemSink));

static void gst_shared_mem_sink_class_init(GstSharedMemSinkClass *klass)
{
    g_return_if_fail(klass != nullptr);

    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    GstMemSinkClass *memSinkClass = GST_MEM_SINK_CLASS(klass);
    GstElementClass *elementClass = GST_ELEMENT_CLASS(klass);

    gst_element_class_set_static_metadata(elementClass,
        "ShMemSink", "Sink/Generic",
        "Output to multi-process shared memory and allow the application to get access to the shared memory",
        "OpenHarmony");

    g_object_class_install_property(gobjectClass, PROP_MEM_SIZE,
        g_param_spec_uint("mem-size", "Memory Size",
            "Allocate the memory with required length (in bytes)",
            0, G_MAXUINT, DEFAULT_PROP_MEM_SIZE,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobjectClass, PROP_MEM_PREFIX_SIZE,
        g_param_spec_uint("mem-prefix-size", "Memory Prefix Size",
            "Allocate the memory with required length's prefix (in bytes)",
            0, G_MAXUINT, DEFAULT_PROP_MEM_PREFIX_SIZE,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobjectClass, PROP_ENABLE_REMOTE_REFCOUNT,
        g_param_spec_boolean ("enable-remote-refcount", "Enable Remote RefCount",
          "Enable the remote refcount at the allocated memory", DEFAULT_PROP_REMOTE_REFCOUNT,
          (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    gobjectClass->dispose = gst_shared_mem_sink_dispose;
    gobjectClass->finalize = gst_shared_mem_sink_finalize;
    gobjectClass->set_property = gst_shared_mem_sink_set_property;
    gobjectClass->get_property = gst_shared_mem_sink_get_property;

    memSinkClass->do_propose_allocation = gst_shared_mem_sink_do_propose_allocation;
    memSinkClass->do_stream_render = gst_shared_mem_sink_do_stream_render;
    memSinkClass->do_app_render = gst_shared_mem_sink_do_app_render;
}

static void gst_shared_mem_sink_init(GstSharedMemSink *sink)
{
    g_return_if_fail(sink != nullptr);

    auto priv = reinterpret_cast<GstSharedMemSinkPrivate *>(gst_shared_mem_sink_get_instance_private(sink));
    g_return_if_fail(priv != nullptr);
    sink->priv = priv;
    priv->memSize = DEFAULT_PROP_MEM_SIZE;
    priv->memPrefixSize = DEFAULT_PROP_MEM_PREFIX_SIZE;
    priv->enableRemoteRefCount = DEFAULT_PROP_REMOTE_REFCOUNT;
    gst_allocation_params_init(&priv->allocParams);
    priv->allocator = gst_shmem_allocator_new();
}

static void gst_shared_mem_sink_dispose(GObject *obj)
{
    g_return_if_fail(obj != nullptr);

    GstSharedMemSink *shmemSink = GST_SHARED_MEM_SINK_CAST(obj);
    GstSharedMemSinkPrivate *priv = shmemSink->priv;
    g_return_if_fail(priv != nullptr);

    GST_OBJECT_LOCK(shmemSink);
    if (priv->allocator) {
        gst_object_unref(priv->allocator);
        priv->allocator = nullptr;
    }
    if (priv->pool != nullptr) {
        gst_object_unref(priv->pool);
        priv->pool = nullptr;
    }
    GST_OBJECT_UNLOCK(shmemSink);

    G_OBJECT_CLASS(parent_class)->dispose(obj);
}

static void gst_shared_mem_sink_finalize(GObject *obj)
{
    g_return_if_fail(obj != nullptr);
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void gst_shared_mem_sink_set_property(GObject *object, guint propId, const GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr && value != nullptr);

    GstSharedMemSink *shmemSink = GST_SHARED_MEM_SINK_CAST(object);
    GstSharedMemSinkPrivate *priv = shmemSink->priv;
    g_return_if_fail(priv != nullptr);

    switch (propId) {
        case PROP_MEM_SIZE: {
            GST_OBJECT_LOCK(shmemSink);
            priv->memSize = g_value_get_uint(value);
            GST_DEBUG_OBJECT(shmemSink, "memory size: %u", priv->memSize);
            GST_OBJECT_UNLOCK(shmemSink);
            break;
        }
        case PROP_MEM_PREFIX_SIZE: {
            GST_OBJECT_LOCK(shmemSink);
            priv->memPrefixSize = g_value_get_uint(value);
            priv->allocParams.prefix = priv->memPrefixSize;
            GST_DEBUG_OBJECT(shmemSink, "memory prefix size: %u", priv->memPrefixSize);
            GST_OBJECT_UNLOCK(shmemSink);
            break;
        }
        case PROP_ENABLE_REMOTE_REFCOUNT: {
            GST_OBJECT_LOCK(shmemSink);
            priv->enableRemoteRefCount = g_value_get_boolean(value);
            GST_DEBUG_OBJECT(shmemSink, "enable remote refcount: %d", priv->enableRemoteRefCount);
            GST_OBJECT_UNLOCK(shmemSink);
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
            break;
    }
}

static void gst_shared_mem_sink_get_property(GObject *object, guint propId, GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr);

    GstSharedMemSink *shmemSink = GST_SHARED_MEM_SINK_CAST(object);
    GstSharedMemSinkPrivate *priv = shmemSink->priv;
    g_return_if_fail(priv != nullptr);

    switch (propId) {
        case PROP_MEM_SIZE: {
            GST_OBJECT_LOCK(shmemSink);
            g_value_set_uint(value, priv->memSize);
            GST_OBJECT_UNLOCK(shmemSink);
            break;
        }
        case PROP_MEM_PREFIX_SIZE: {
            GST_OBJECT_LOCK(shmemSink);
            g_value_set_uint(value, priv->memPrefixSize);
            GST_OBJECT_UNLOCK(shmemSink);
            break;
        }
        case PROP_ENABLE_REMOTE_REFCOUNT: {
            GST_OBJECT_LOCK(shmemSink);
            g_value_set_boolean(value, priv->enableRemoteRefCount);
            GST_OBJECT_UNLOCK(shmemSink);
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
            break;
    }
}

static gboolean set_pool_for_allocator(GstSharedMemSink *shmemSink, guint minBufs, guint maxBufs, guint size)
{
    GstSharedMemSinkPrivate *priv = shmemSink->priv;

    if (priv->allocator == nullptr) {
        return FALSE;
    }

    if (priv->avShmemPool != nullptr) {
        return TRUE;
    }

    if (size == 0 || maxBufs == 0) {
        GST_ERROR_OBJECT(shmemSink, "need copy buffer, but the dest buf's size or pool capacity is not provided");
        return FALSE;
    }

    if (priv->avShmemPool == nullptr) {
        priv->avShmemPool = std::make_shared<OHOS::Media::AVSharedMemoryPool>();
        OHOS::Media::AVSharedMemoryPool::InitializeOption option = {
            .preAllocMemCnt = minBufs,
            .memSize = size,
            .maxMemCnt = maxBufs,
            .enableRemoteRefCnt = priv->enableRemoteRefCount
        };
        int32_t ret = priv->avShmemPool->Init(option);
        g_return_val_if_fail(ret == OHOS::Media::MSERR_OK, GST_FLOW_ERROR);
    }

    gst_shmem_allocator_set_pool(priv->allocator, priv->avShmemPool);
    return TRUE;
}

static GstFlowReturn do_copy_buffer(GstSharedMemSink *shmemSink, GstBuffer *inBuf, GstBuffer **outBuf)
{
    GstSharedMemSinkPrivate *priv = shmemSink->priv;
    GstMemSink *memsink = GST_MEM_SINK_CAST(shmemSink);

    gboolean ret = set_pool_for_allocator(shmemSink, 1, memsink->maxPoolCapacity, priv->memSize);
    g_return_val_if_fail(ret, GST_FLOW_ERROR);

    *outBuf = gst_buffer_new();
    g_return_val_if_fail(*outBuf != nullptr, GST_FLOW_ERROR);

    // blocking allocate memory
    GstMemory *memory = gst_allocator_alloc(GST_ALLOCATOR_CAST(priv->allocator), priv->memSize, &priv->allocParams);
    g_return_val_if_fail(memory != nullptr, GST_FLOW_ERROR);
    gst_buffer_append_memory(*outBuf, memory);

    do {
        ret = gst_buffer_copy_into(*outBuf, inBuf, GST_BUFFER_COPY_METADATA, 0, -1);
        if (!ret) {
            GST_ERROR_OBJECT(shmemSink, "copy metadata from inbuf failed");
            break;
        }

        // add buffer type meta here.

        GstMapInfo info;
        if (!gst_buffer_map(*outBuf, &info, GST_MAP_WRITE)) {
            GST_ERROR_OBJECT(shmemSink, "map buffer failed");
            ret = FALSE;
            break;
        }

        gsize size = gst_buffer_get_size(inBuf);
        gsize writesize = gst_buffer_extract(inBuf, 0, info.data, size);
        if (writesize != size) {
            GST_ERROR_OBJECT(shmemSink, "extract buffer failed");
            ret = FALSE;
            break;
        }
    } while (0);

    if (!ret) {
        gst_buffer_unref(*outBuf);
        *outBuf = nullptr;
        return GST_FLOW_ERROR;
    }

    return GST_FLOW_OK;
}

static gboolean check_need_copy(GstSharedMemSink *shmemSink, GstBuffer *buffer)
{
    GstSharedMemSinkPrivate *priv = shmemSink->priv;

    if (gst_buffer_n_memory(buffer) != 1) {
        GST_ERROR_OBJECT(shmemSink, "buffer's memory chunks is not 1 !");
        return FALSE;
    }

    if (priv->pool != nullptr && buffer->pool == GST_BUFFER_POOL_CAST(priv->pool)) {
        return FALSE;
    }

    if (priv->pool == nullptr && priv->allocator != nullptr) {
        GstMemory *memory = gst_buffer_peek_memory(buffer, 0);
        if (memory == nullptr) {
            return FALSE;
        }
        if (memory->allocator == GST_ALLOCATOR_CAST(priv->allocator)) {
            return FALSE;
        }
    }

    return TRUE;
}

static GstFlowReturn gst_shared_mem_sink_do_stream_render(GstMemSink *memsink, GstBuffer **buffer)
{
    GstSharedMemSink *shmemSink = GST_SHARED_MEM_SINK_CAST(memsink);
    GstSharedMemSinkPrivate *priv = shmemSink->priv;
    g_return_val_if_fail(priv != nullptr, GST_FLOW_ERROR);
    GstBuffer *origBuf = *buffer;
    GstBuffer *outBuf = nullptr;

    if (!check_need_copy(shmemSink, origBuf)) {
        return GST_FLOW_OK;
    }

    GstFlowReturn ret = do_copy_buffer(shmemSink, origBuf, &outBuf);
    g_return_val_if_fail(ret != GST_FLOW_OK, ret);

    *buffer = outBuf;
    gst_buffer_unref(origBuf);
    return GST_FLOW_OK;
}

static GstFlowReturn gst_shared_mem_sink_do_app_render(GstMemSink *memsink, GstBuffer *buffer)
{
    /**
     * the buffer is not used by this sharedmemsink. But this sharedmemsink need this call to
     * trigger the underlying pool to waken up when the remote process ends up the access to
     * the memory.
     */
    (void)buffer;

    GstSharedMemSink *shmemSink = GST_SHARED_MEM_SINK_CAST(memsink);
    GstSharedMemSinkPrivate *priv = shmemSink->priv;
    g_return_val_if_fail(priv != nullptr, GST_FLOW_ERROR);
    g_return_val_if_fail(priv->avShmemPool == nullptr, GST_FLOW_ERROR);

    priv->avShmemPool->SignalMemoryReleased();
    return GST_FLOW_OK;
}

static gboolean set_pool_for_propose_allocation(GstSharedMemSink *shmemSink, GstQuery *query, GstCaps *caps)
{
    GstMemSink *memsink = GST_MEM_SINK_CAST(shmemSink);
    GstSharedMemSinkPrivate *priv = shmemSink->priv;

    guint size = 0;
    guint minBuffers = 0;
    guint maxBuffers = 0;
    gst_query_parse_nth_allocation_pool(query, 0, nullptr, &size, &minBuffers, &maxBuffers);
    if (maxBuffers > memsink->maxPoolCapacity && memsink->maxPoolCapacity != 0) {
        GST_INFO_OBJECT(shmemSink, "correct the maxbuffer from %u to %u", maxBuffers, memsink->maxPoolCapacity);
        maxBuffers = memsink->maxPoolCapacity;
    }

    priv->pool = gst_shmem_pool_new();
    g_return_val_if_fail(priv->pool != nullptr, FALSE);
    GstShMemPool *pool = priv->pool;
    gst_query_add_allocation_pool(query, GST_BUFFER_POOL_CAST(pool), size, minBuffers, maxBuffers);
    // add buffer type meta here.

    priv->avShmemPool = std::make_shared<OHOS::Media::AVSharedMemoryPool>();

    GstStructure *config = gst_buffer_pool_get_config(GST_BUFFER_POOL_CAST(pool));
    if (config != nullptr) {
        return FALSE;
    }

    gst_buffer_pool_config_set_params(config, caps, size, minBuffers, maxBuffers);
    gst_buffer_pool_config_set_allocator(config, GST_ALLOCATOR_CAST(priv->allocator), &priv->allocParams);

    return gst_buffer_pool_set_config(GST_BUFFER_POOL_CAST(pool), config);
}

static gboolean gst_shared_mem_sink_do_propose_allocation(GstMemSink *memsink, GstQuery *query)
{
    GstSharedMemSink *shmemSink = GST_SHARED_MEM_SINK_CAST(memsink);
    GstSharedMemSinkPrivate *priv = shmemSink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);
    g_return_val_if_fail(priv->allocator != nullptr, FALSE);

    GstCaps *caps = nullptr;
    gboolean needPool = FALSE;
    gst_query_parse_allocation(query, &caps, &needPool);
    GST_INFO_OBJECT(shmemSink, "process allocation query, caps: %" GST_PTR_FORMAT ", need pool: %d", caps, needPool);

    GST_OBJECT_LOCK(shmemSink);

    gst_query_add_allocation_param(query, GST_ALLOCATOR_CAST(priv->allocator), &priv->allocParams);
    gboolean ret = TRUE;
    if (needPool) {
        ret = set_pool_for_propose_allocation(shmemSink, query, caps);
        if (!ret) {
            GST_ERROR_OBJECT(shmemSink, "config pool failed");
        }
    }

    GST_OBJECT_LOCK(shmemSink);
    return ret;
}
