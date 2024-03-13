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

#include "gst_surface_allocator.h"
#include <sync_fence.h>
#include "media_log.h"
#include "media_dfx.h"
#include "player_xcollie.h"

GST_DEBUG_CATEGORY_STATIC(gst_surface_allocator_debug_category);
#define GST_CAT_DEFAULT gst_surface_allocator_debug_category

#define gst_surface_allocator_parent_class parent_class
G_DEFINE_TYPE(GstSurfaceAllocator, gst_surface_allocator, GST_TYPE_ALLOCATOR);

#define GST_SURFACE_ALLOCATOR_LOCK(allocator) (g_mutex_lock(&(allocator)->lock))
#define GST_SURFACE_ALLOCATOR_UNLOCK(allocator) (g_mutex_unlock(&(allocator)->lock))
#define GST_SURFACE_ALLOCATOR_WAIT(allocator) (g_cond_wait(&(allocator)->cond, &(allocator)->lock))
#define GST_SURFACE_ALLOCATOR_NOTIFY(allocator) (g_cond_signal(&(allocator)->cond))

using namespace OHOS;
using namespace std;
using namespace OHOS::Media;

enum class VideoScaleType {
    VIDEO_SCALE_TYPE_FIT,
    VIDEO_SCALE_TYPE_FIT_CROP,
    VIDEO_SCALE_TYPE_LAST,
};

namespace {
    const std::unordered_map<VideoScaleType, OHOS::ScalingMode> SCALEMODE_MAP = {
        { VideoScaleType::VIDEO_SCALE_TYPE_FIT, OHOS::SCALING_MODE_SCALE_TO_WINDOW },
        { VideoScaleType::VIDEO_SCALE_TYPE_FIT_CROP, OHOS::SCALING_MODE_SCALE_CROP},
    };
}

#define SURFACE_BUFFER_LOG_INFO(allocator, id) \
    GST_DEBUG_OBJECT(allocator, \
    "buffer %u requestBuffer %d flushBuffer %d cacheBuffer %d totalBuffer %d", \
    id, allocator->requestBufferNum, allocator->flushBufferNum, \
    allocator->cacheBufferNum, allocator->totalBufferNum)

static void gst_surface_allocator_buffer_release(GstSurfaceAllocator *allocator, sptr<SurfaceBuffer> &buffer)
{
    GST_DEBUG_OBJECT(allocator, "buffer released");
    (void)buffer;
    GST_SURFACE_ALLOCATOR_LOCK(allocator);
    allocator->flushBufferNum--;
    allocator->cacheBufferNum++;
    SURFACE_BUFFER_LOG_INFO(allocator, 0);
    GST_SURFACE_ALLOCATOR_NOTIFY(allocator);
    GST_SURFACE_ALLOCATOR_UNLOCK(allocator);
    MediaTrace::CounterTrace("flushBufferNum", allocator->flushBufferNum);
    MediaTrace::CounterTrace("cacheBufferNum", allocator->cacheBufferNum);
}

GSError AllocatorWrap::OnBufferReleased(sptr<SurfaceBuffer> &buffer)
{
    gst_surface_allocator_buffer_release(&owner_, buffer);
    return OHOS::SurfaceError::SURFACE_ERROR_OK;
}

gboolean gst_surface_allocator_set_surface(GstSurfaceAllocator *allocator, OHOS::sptr<OHOS::Surface> surface)
{
    if (allocator == nullptr) {
        GST_ERROR("allocator is nullptr");
        return FALSE;
    }
    if (surface == nullptr) {
        GST_ERROR("surface is nullptr");
        return FALSE;
    }
    allocator->surface = surface;
    if (allocator->allocatorWrap) {
        delete allocator->allocatorWrap;
        allocator->allocatorWrap = nullptr;
    }
    allocator->allocatorWrap = new AllocatorWrap(*allocator);
    allocator->clean = FALSE;
    GST_SURFACE_ALLOCATOR_LOCK(allocator);
    allocator->requestBufferNum = 0;
    allocator->totalBufferNum = 0;
    allocator->cacheBufferNum = 0;
    allocator->flushBufferNum = 0;
    GST_SURFACE_ALLOCATOR_UNLOCK(allocator);
    auto bufferReleased = std::bind(&AllocatorWrap::OnBufferReleased, allocator->allocatorWrap, std::placeholders::_1);
    GSError ret = OHOS::SurfaceError::SURFACE_ERROR_OK;
    LISTENER(ret = surface->RegisterReleaseListener(bufferReleased),
        "surface::RegisterReleaseListener", PlayerXCollie::timerTimeout)

    if (ret != OHOS::SurfaceError::SURFACE_ERROR_OK) {
        allocator->isCallbackMode = FALSE;
        GST_ERROR("Register Release Listener failed");
    }
    return TRUE;
}

static OHOS::ScalingMode gst_surface_allocator_get_scale_type(GstSurfaceAllocParam param)
{
    if (SCALEMODE_MAP.find(static_cast<VideoScaleType>(param.scale_type)) == SCALEMODE_MAP.end()) {
        return OHOS::SCALING_MODE_SCALE_TO_WINDOW;
    }
    return SCALEMODE_MAP.at(static_cast<VideoScaleType>(param.scale_type));
}

static void gst_surface_cancel_buffer(GstSurfaceAllocator *allocator, OHOS::sptr<OHOS::SurfaceBuffer> &buffer)
{
    MediaTrace scaleTrace("Surface::CancelBuffer");
    LISTENER(allocator->surface->CancelBuffer(buffer),
        "surface::CancelBuffer", PlayerXCollie::timerTimeout)
    GST_SURFACE_ALLOCATOR_LOCK(allocator);
    allocator->requestBufferNum--;
    allocator->cacheBufferNum++;
    SURFACE_BUFFER_LOG_INFO(allocator, buffer->GetSeqNum());
    GST_SURFACE_ALLOCATOR_UNLOCK(allocator);
    MediaTrace::CounterTrace("requestBufferNum", allocator->requestBufferNum);
    MediaTrace::CounterTrace("cacheBufferNum", allocator->cacheBufferNum);
}

static bool gst_surface_scale_buffer(GstSurfaceAllocator *allocator, GstSurfaceAllocParam param,
    OHOS::sptr<OHOS::SurfaceBuffer> &buffer)
{
    MediaTrace scaleTrace("Surface::SetScalingMode");
    auto scaleType = gst_surface_allocator_get_scale_type(param);
    OHOS::SurfaceError ret = OHOS::SurfaceError::SURFACE_ERROR_OK;
    LISTENER(ret = allocator->surface->SetScalingMode(buffer->GetSeqNum(), scaleType),
        "surface::SetScalingMode", PlayerXCollie::timerTimeout)
    if (ret != OHOS::SurfaceError::SURFACE_ERROR_OK) {
        GST_ERROR("surface buffer set scaling mode failed");
        gst_surface_cancel_buffer(allocator, buffer);
        return false;
    }
    return true;
}

static bool gst_surface_map_buffer(GstSurfaceAllocator *allocator, OHOS::sptr<OHOS::SurfaceBuffer> &buffer)
{
    MediaTrace mapTrace("Surface::Map");
    if (buffer->Map() != OHOS::SurfaceError::SURFACE_ERROR_OK) {
        GST_ERROR("surface buffer Map failed");
        gst_surface_cancel_buffer(allocator, buffer);
        return false;
    }
    return true;
}

static bool gst_surface_request_buffer(GstSurfaceAllocator *allocator, GstSurfaceAllocParam param,
    OHOS::sptr<OHOS::SurfaceBuffer> &buffer)
{
    GST_SURFACE_ALLOCATOR_LOCK(allocator);
    while (allocator->cacheBufferNum == 0 && allocator->clean == FALSE && allocator->isCallbackMode) {
        GST_SURFACE_ALLOCATOR_WAIT(allocator);
    }
    GST_SURFACE_ALLOCATOR_UNLOCK(allocator);
    if (allocator->clean == TRUE) {
        return FALSE;
    }

    MediaTrace trace("Surface::RequestBuffer");
    static constexpr int32_t stride_alignment = 8;
    int32_t wait_time = (param.dont_wait && allocator->isCallbackMode) ? 0 : INT_MAX; // wait forever or no wait.
    OHOS::BufferRequestConfig request_config = {
        param.width, param.height, stride_alignment, param.format,
        param.usage, wait_time
    };
    int32_t release_fence = -1;
    OHOS::SurfaceError ret = OHOS::SurfaceError::SURFACE_ERROR_OK;
    if (wait_time == 0) {
        LISTENER(ret = allocator->surface->RequestBuffer(buffer, release_fence, request_config),
            "surface::RequestBuffer", PlayerXCollie::timerTimeout)
    } else {
        ret = allocator->surface->RequestBuffer(buffer, release_fence, request_config);
    }
    if (ret != OHOS::SurfaceError::SURFACE_ERROR_OK || buffer == nullptr) {
        GST_ERROR("there is no more surface buffer");
        return false;
    }

    GST_SURFACE_ALLOCATOR_LOCK(allocator);
    allocator->requestBufferNum++;
    allocator->cacheBufferNum--;
    SURFACE_BUFFER_LOG_INFO(allocator, buffer->GetSeqNum());
    GST_SURFACE_ALLOCATOR_UNLOCK(allocator);
    MediaTrace::CounterTrace("requestBufferNum", allocator->requestBufferNum);
    MediaTrace::CounterTrace("cacheBufferNum", allocator->cacheBufferNum);

    g_return_val_if_fail(gst_surface_map_buffer(allocator, buffer), false);
    OHOS::sptr<OHOS::SyncFence> autoFence = new(std::nothrow) OHOS::SyncFence(release_fence);
    if (autoFence != nullptr) {
        autoFence->Wait(100); // 100ms
    }

    g_return_val_if_fail(gst_surface_scale_buffer(allocator, param, buffer), false);
    return true;
}

GstSurfaceMemory *gst_surface_allocator_alloc(GstSurfaceAllocator *allocator, GstSurfaceAllocParam param)
{
    g_return_val_if_fail(allocator != nullptr && allocator->surface != nullptr, nullptr);

    OHOS::sptr<OHOS::SurfaceBuffer> buffer = nullptr;
    if (!gst_surface_request_buffer(allocator, param, buffer) || buffer == nullptr) {
        GST_ERROR_OBJECT(allocator, "0x%06" PRIXPTR " failed to request surface buffer", FAKE_POINTER(allocator));
        return nullptr;
    }

    GstSurfaceMemory *memory = reinterpret_cast<GstSurfaceMemory *>(g_slice_alloc0(sizeof(GstSurfaceMemory)));
    if (memory == nullptr) {
        GST_ERROR("alloc GstSurfaceMemory slice failed");
        LISTENER(allocator->surface->CancelBuffer(buffer),
            "surface::CancelBuffer", PlayerXCollie::timerTimeout)
        return nullptr;
    }

    gst_memory_init(GST_MEMORY_CAST(memory), (GstMemoryFlags)0, GST_ALLOCATOR_CAST(allocator), nullptr,
        buffer->GetSize(), 0, 0, buffer->GetSize());

    memory->buf = buffer;
    memory->fence = -1;
    memory->need_render = FALSE;
    GST_DEBUG("alloc surface buffer for width: %d, height: %d, format: %d, size: %u",
        param.width, param.height, param.format, buffer->GetSize());

    return memory;
}

static void gst_surface_allocator_free(GstAllocator *baseAllocator, GstMemory *baseMemory)
{
    GstSurfaceAllocator *allocator = reinterpret_cast<GstSurfaceAllocator*>(baseAllocator);
    GstSurfaceMemory *memory = reinterpret_cast<GstSurfaceMemory*>(baseMemory);
    g_return_if_fail(memory != nullptr && allocator != nullptr && allocator->surface != nullptr);

    GST_DEBUG("free surface buffer %u for width: %d, height: %d, format: %d, size: %u, need_render: %d, fence: %d",
        memory->buf->GetSeqNum(), memory->buf->GetWidth(), memory->buf->GetHeight(), memory->buf->GetFormat(),
        memory->buf->GetSize(), memory->need_render, memory->fence);

    if (!memory->need_render) {
        MediaTrace trace("Surface::CancelBuffer");
        OHOS::SurfaceError ret = OHOS::SurfaceError::SURFACE_ERROR_OK;
        LISTENER(ret = allocator->surface->CancelBuffer(memory->buf),
            "surface::CancelBuffer", PlayerXCollie::timerTimeout)
        if (ret != OHOS::SurfaceError::SURFACE_ERROR_OK) {
            GST_DEBUG("cancel buffer to surface failed, %d", ret);
        } else {
            GST_SURFACE_ALLOCATOR_LOCK(allocator);
            allocator->requestBufferNum--;
            allocator->cacheBufferNum++;
            SURFACE_BUFFER_LOG_INFO(allocator, memory->buf->GetSeqNum());
            GST_SURFACE_ALLOCATOR_NOTIFY(allocator);
            GST_SURFACE_ALLOCATOR_UNLOCK(allocator);
            MediaTrace::CounterTrace("requestBufferNum", allocator->requestBufferNum);
            MediaTrace::CounterTrace("cacheBufferNum", allocator->cacheBufferNum);
        }
    }

    memory->buf = nullptr;
    g_slice_free(GstSurfaceMemory, memory);
}

static GstMemory *gst_surface_allocator_alloc_dummy(GstAllocator *allocator, gsize size, GstAllocationParams *params)
{
    (void)allocator;
    (void)size;
    (void)params;
    return nullptr;
}

static gpointer gst_surface_allocator_mem_map(GstMemory *mem, gsize maxsize, GstMapFlags flags)
{
    (void)maxsize;
    (void)flags;
    g_return_val_if_fail(mem != nullptr, nullptr);
    g_return_val_if_fail(gst_is_surface_memory(mem), nullptr);

    GstSurfaceMemory *sf_mem = reinterpret_cast<GstSurfaceMemory *>(mem);
    g_return_val_if_fail(sf_mem->buf != nullptr, nullptr);

    return sf_mem->buf->GetVirAddr();
}

static void gst_surface_allocator_mem_unmap(GstMemory *mem)
{
    (void)mem;
}

gboolean gst_surface_allocator_flush_buffer(GstSurfaceAllocator *allocator, sptr<SurfaceBuffer>& buffer,
    int32_t fence, BufferFlushConfig &config)
{
    if (allocator->surface) {
        GST_DEBUG_OBJECT(allocator, "FlushBuffer");
        MediaTrace trace("Surface::FlushBuffer");
        OHOS::SurfaceError ret = OHOS::SurfaceError::SURFACE_ERROR_OK;
        LISTENER(ret = allocator->surface->FlushBuffer(buffer, fence, config),
            "surface::FlushBuffer", PlayerXCollie::timerTimeout)
        if (ret != OHOS::SurfaceError::SURFACE_ERROR_OK) {
            GST_ERROR_OBJECT(allocator, "flush buffer %d to surface failed, %d", buffer->GetSeqNum(), ret);
            return FALSE;
        }
    }
    {
        GST_SURFACE_ALLOCATOR_LOCK(allocator);
        allocator->flushBufferNum++;
        allocator->requestBufferNum--;
        SURFACE_BUFFER_LOG_INFO(allocator, buffer->GetSeqNum());
        GST_SURFACE_ALLOCATOR_UNLOCK(allocator);
        MediaTrace::CounterTrace("flushBufferNum", allocator->flushBufferNum);
        MediaTrace::CounterTrace("requestBufferNum", allocator->requestBufferNum);
    }
    return TRUE;
}

gboolean gst_surface_allocator_set_queue_size(GstSurfaceAllocator *allocator, int32_t size)
{
    if (allocator->surface) {
        GST_DEBUG_OBJECT(allocator, "set queue size %d", size);
        MediaTrace trace("Surface::SetQueueSize");
        OHOS::SurfaceError err = OHOS::SurfaceError::SURFACE_ERROR_OK;
        LISTENER(err = allocator->surface->SetQueueSize(size),
            "surface::SetQueueSize", PlayerXCollie::timerTimeout)
        if (err != OHOS::SurfaceError::SURFACE_ERROR_OK) {
            GST_ERROR_OBJECT(allocator, "set queue size failed, %d", err);
            return FALSE;
        }
    }
    {
        GST_SURFACE_ALLOCATOR_LOCK(allocator);
        allocator->cacheBufferNum += (size - allocator->totalBufferNum);
        allocator->totalBufferNum = size;
        SURFACE_BUFFER_LOG_INFO(allocator, 0);
        GST_SURFACE_ALLOCATOR_NOTIFY(allocator);
        GST_SURFACE_ALLOCATOR_UNLOCK(allocator);
        MediaTrace::CounterTrace("cacheBufferNum", allocator->cacheBufferNum);
        MediaTrace::CounterTrace("totalBufferNum", allocator->totalBufferNum);
    }
    return TRUE;
}

void gst_surface_allocator_clean_cache(GstSurfaceAllocator *allocator)
{
    GST_INFO_OBJECT(allocator, "clean cache");
    if (allocator->surface) {
        OHOS::SurfaceError err = OHOS::SurfaceError::SURFACE_ERROR_OK;
        MediaTrace trace("Surface::CleanCache");
        LISTENER(err = allocator->surface->CleanCache(),
            "surface::CleanCache", PlayerXCollie::timerTimeout)
        if (err != OHOS::SurfaceError::SURFACE_ERROR_OK) {
            GST_ERROR_OBJECT(allocator, "clean cache failed, %d", err);
        }
    }
    allocator->clean = TRUE;
    GST_SURFACE_ALLOCATOR_NOTIFY(allocator);
}

static void gst_surface_allocator_init(GstSurfaceAllocator *allocator)
{
    GstAllocator *base_allocator = GST_ALLOCATOR_CAST(allocator);
    g_return_if_fail(base_allocator != nullptr);

    GST_DEBUG_OBJECT(allocator, "init allocator 0x%06" PRIXPTR "", FAKE_POINTER(allocator));

    base_allocator->mem_type = GST_SURFACE_MEMORY_TYPE;
    base_allocator->mem_map = (GstMemoryMapFunction)gst_surface_allocator_mem_map;
    base_allocator->mem_unmap = (GstMemoryUnmapFunction)gst_surface_allocator_mem_unmap;
    allocator->allocatorWrap = nullptr;
    allocator->isCallbackMode = TRUE;
    g_mutex_init(&allocator->lock);
    g_cond_init(&allocator->cond);
}

static void gst_surface_allocator_finalize(GObject *obj)
{
    GstSurfaceAllocator *allocator = GST_SURFACE_ALLOCATOR(obj);
    g_return_if_fail(allocator != nullptr);

    if (allocator->surface != nullptr) {
        allocator->surface->UnRegisterReleaseListener();
        allocator->surface = nullptr;
    }
    if (allocator->allocatorWrap) {
        delete allocator->allocatorWrap;
        allocator->allocatorWrap = nullptr;
    }
    g_mutex_clear(&allocator->lock);
    g_cond_clear(&allocator->cond);
    GST_DEBUG_OBJECT(allocator, "finalize allocator 0x%06" PRIXPTR "", FAKE_POINTER(allocator));
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void gst_surface_allocator_class_init(GstSurfaceAllocatorClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    g_return_if_fail(gobject_class != nullptr);

    gobject_class->finalize = gst_surface_allocator_finalize;

    GstAllocatorClass *allocatorClass = GST_ALLOCATOR_CLASS(klass);
    g_return_if_fail(allocatorClass != nullptr);

    allocatorClass->alloc = gst_surface_allocator_alloc_dummy;
    allocatorClass->free = gst_surface_allocator_free;
    GST_DEBUG_CATEGORY_INIT(gst_surface_allocator_debug_category, "prosurallocator", 0, "surface allocator");
}

GstSurfaceAllocator *gst_surface_allocator_new()
{
    GstSurfaceAllocator *alloc = GST_SURFACE_ALLOCATOR(g_object_new(
        GST_TYPE_SURFACE_ALLOCATOR, "name", "SurfaceAllocator", nullptr));
    (void)gst_object_ref_sink(alloc);

    return alloc;
}
