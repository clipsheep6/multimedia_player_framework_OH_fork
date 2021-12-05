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

#include "sink_surface_impl.h"
#include "display_type.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SinkSurfaceImpl"};
    const uint32_t DEFAULT_QUEUE_BUFFER_NUM = 8;
    const uint32_t DEFAULT_BUFFER_COUNT = 5;
    const uint32_t MAX_DEFAULT_TRY_TIMES = 100;
    const uint32_t DEFAULT_WAIT_TIME = 5000;
}

namespace OHOS {
namespace Media {
SinkSurfaceImpl::SinkSurfaceImpl()
{
}

SinkSurfaceImpl::~SinkSurfaceImpl()
{
    g_signal_handler_disconnect(G_OBJECT(element_), signalId_);
    bufferList_.clear();
    if (element_ != nullptr) {
        gst_object_unref(element_);
        element_ = nullptr;
    }
    producerSurface_ = nullptr;
}

int32_t SinkSurfaceImpl::Init()
{
    element_ = GST_ELEMENT_CAST(gst_object_ref(gst_element_factory_make("appsink", "sink")));
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    gst_base_sink_set_async_enabled(GST_BASE_SINK(element_), FALSE);

    bufferCount_ = DEFAULT_BUFFER_COUNT;
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::Configure(std::shared_ptr<ProcessorConfig> config)
{
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    g_object_set(G_OBJECT(element_), "caps", config->caps_, nullptr);

    for (uint32_t i = 0; i < bufferCount_; i++) {
        auto bufWrap = std::make_shared<BufferWrapper>(nullptr, nullptr, bufferList_.size(), BufferWrapper::DOWNSTREAM);
        CHECK_AND_RETURN_RET(bufWrap != nullptr, MSERR_NO_MEMORY);
        bufferList_.push_back(bufWrap);
    }

    signalId_ = g_signal_connect(G_OBJECT(element_), "new_sample",
                                 G_CALLBACK(OutputAvailableCb), reinterpret_cast<gpointer>(this));
    g_object_set(G_OBJECT(element_), "emit-signals", TRUE, nullptr);
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::Flush()
{
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto it = bufferList_.begin(); it != bufferList_.end(); it++) {
        (*it)->owner_ = BufferWrapper::DOWNSTREAM;
    }
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::SetOutputSurface(sptr<Surface> surface)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(surface != nullptr, MSERR_INVALID_VAL);
    g_object_set(G_OBJECT(element_), "surface", static_cast<gpointer>(surface), nullptr);
    producerSurface_ = surface;
    producerSurface_->SetQueueSize(DEFAULT_QUEUE_BUFFER_NUM);
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::ReleaseOutputBuffer(uint32_t index, bool render)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(index < bufferCount_ && index < bufferList_.size(), MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(bufferList_[index]->owner_ == BufferWrapper::APP, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(bufferList_[index]->gstBuffer_ != nullptr, MSERR_UNKNOWN);
    if (render) {
        CHECK_AND_RETURN_RET(UpdateSurfaceBuffer(*(bufferList_[index]->gstBuffer_)) == MSERR_OK, MSERR_UNKNOWN);
    }
    bufferList_[index]->owner_ = BufferWrapper::DOWNSTREAM;
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::SetCallback(const std::weak_ptr<IAVCodecEngineObs> &obs)
{
    std::unique_lock<std::mutex> lock(mutex_);
    obs_ = obs;
    return MSERR_OK;
}

GstFlowReturn SinkSurfaceImpl::OutputAvailableCb(GstElement *sink, gpointer userData)
{
    (void)sink;
    MEDIA_LOGD("OutputAvailableCb");
    auto impl = static_cast<SinkSurfaceImpl *>(userData);
    CHECK_AND_RETURN_RET(impl != nullptr, GST_FLOW_ERROR);
    std::unique_lock<std::mutex> lock(impl->mutex_);
    CHECK_AND_RETURN_RET(impl->HandleOutputCb() == MSERR_OK, GST_FLOW_ERROR);
    return GST_FLOW_OK;
}

int32_t SinkSurfaceImpl::HandleOutputCb()
{
    GstSample *sample = nullptr;
    g_signal_emit_by_name(G_OBJECT(element_), "pull-sample", &sample);
    CHECK_AND_RETURN_RET(sample != nullptr, MSERR_UNKNOWN);

    GstBuffer *buf = gst_sample_get_buffer(sample);
    if (buf == nullptr) {
        MEDIA_LOGE("Failed to gst_sample_get_buffer");
        gst_sample_unref(sample);
        return MSERR_UNKNOWN;
    }

    uint32_t index = 0;
    for (auto it = bufferList_.begin(); it != bufferList_.end(); it++) {
        if ((*it)->owner_ == BufferWrapper::DOWNSTREAM) {
            (*it)->owner_ = BufferWrapper::APP;
            (*it)->gstBuffer_ = buf;
        }
        index++;
    }

    auto obs = obs_.lock();
    CHECK_AND_RETURN_RET(obs != nullptr, MSERR_UNKNOWN);
    AVCodecBufferInfo info;
    obs->OnOutputBufferAvailable(index, info, AVCODEC_BUFFER_FLAG_NONE);

    return MSERR_OK;
}

int32_t SinkSurfaceImpl::UpdateSurfaceBuffer(const GstBuffer &buffer)
{
    CHECK_AND_RETURN_RET(producerSurface_ != nullptr, MSERR_UNKNOWN);

    auto buf = const_cast<GstBuffer *>(&buffer);
    GstVideoMeta *videoMeta = gst_buffer_get_video_meta(buf);
    CHECK_AND_RETURN_RET(videoMeta != nullptr, MSERR_UNKNOWN);

    sptr<SurfaceBuffer> surfaceBuffer = RequestBuffer(videoMeta);
    CHECK_AND_RETURN_RET(surfaceBuffer != nullptr, MSERR_INVALID_OPERATION);
    SurfaceError ret = SURFACE_ERROR_OK;
    bool needFlush = false;
    do {
        auto surfaceBufferAddr = surfaceBuffer->GetVirAddr();
        CHECK_AND_BREAK_LOG(surfaceBufferAddr != nullptr, "Buffer addr is nullptr..");
        gsize size = gst_buffer_get_size(buf);
        CHECK_AND_BREAK_LOG(size > 0, "gst_buffer_get_size failed..");
        gsize sizeCopy = gst_buffer_extract(buf, 0, surfaceBufferAddr, size);
        if (sizeCopy != size) {
            MEDIA_LOGW("extract buffer from size : %" G_GSIZE_FORMAT " to size %" G_GSIZE_FORMAT, size, sizeCopy);
        }
        needFlush = true;
    } while (0);

    if (needFlush) {
        BufferFlushConfig flushConfig = {};
        flushConfig.damage.x = 0;
        flushConfig.damage.y = 0;
        flushConfig.damage.w = videoMeta->width;
        flushConfig.damage.h = videoMeta->height;
        ret = producerSurface_->FlushBuffer(surfaceBuffer, -1, flushConfig);
        CHECK_AND_RETURN_RET_LOG(ret == SURFACE_ERROR_OK, MSERR_INVALID_OPERATION,
            "FlushBuffer failed(ret = %{public}d)..", ret);
    } else {
        (void)producerSurface_->CancelBuffer(surfaceBuffer);
        return MSERR_INVALID_OPERATION;
    }
    return MSERR_OK;
}

sptr<SurfaceBuffer> SinkSurfaceImpl::RequestBuffer(GstVideoMeta *videoMeta)
{
    CHECK_AND_RETURN_RET(videoMeta != nullptr, nullptr);

    BufferRequestConfig config;
    config.width = static_cast<int32_t>(videoMeta->width);
    config.height = static_cast<int32_t>(videoMeta->height);
    constexpr int32_t strideAlignment = 8;
    const std::string surfaceFormat = "SURFACE_FORMAT";
    config.strideAlignment = strideAlignment;
    config.format = PIXEL_FMT_RGBA_8888;
    config.usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA;
    config.timeout = 0;

    sptr<SurfaceBuffer> surfaceBuffer = nullptr;
    int32_t releaseFence = -1;
    SurfaceError ret = SURFACE_ERROR_OK;
    uint32_t count = 0;
    do {
        ret = producerSurface_->RequestBuffer(surfaceBuffer, releaseFence, config);
        if (ret == SURFACE_ERROR_NO_BUFFER) {
            usleep(DEFAULT_WAIT_TIME);
            ++count;
        }
    } while (ret == SURFACE_ERROR_NO_BUFFER && count < MAX_DEFAULT_TRY_TIMES);
    CHECK_AND_RETURN_RET(ret == SURFACE_ERROR_OK, nullptr);
    return surfaceBuffer;
}
} // Media
} // OHOS
