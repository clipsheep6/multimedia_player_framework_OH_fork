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
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SinkSurfaceImpl"};
}

namespace OHOS {
namespace Media {
SinkSurfaceImpl::SinkSurfaceImpl()
{
}

SinkSurfaceImpl::~SinkSurfaceImpl()
{
    bufferList_.clear();
    shareMemList_.clear();
    if (element_ != nullptr) {
        g_signal_handler_disconnect(G_OBJECT(element_), signalId_);
        gst_object_unref(element_);
        element_ = nullptr;
    }
}

int32_t SinkSurfaceImpl::AllocateBuffer()
{
    CHECK_AND_RETURN_RET(bufferCount_ > 0, MSERR_UNKNOWN);
    int32_t ret = MSERR_UNKNOWN;
    for (uint32_t i = 0; i < bufferCount_; i++) {
        GstSample *sample = nullptr;
        g_signal_emit_by_name(G_OBJECT(element_), "pull-sample", &sample);
        CHECK_AND_BREAK_LOG(sample != nullptr, "Failed to pull sample");
        GstBuffer *buf = gst_sample_get_buffer(sample);
        if (buf == nullptr) {
            MEDIA_LOGE("Failed to gst_sample_get_buffer");
            gst_sample_unref(sample);
            break;
        }
        auto bufferWrapper = std::make_shared<BufferWrapper>(buf, i);
        if (bufferWrapper == nullptr) {
            MEDIA_LOGE("No memory");
            gst_sample_unref(sample);
            break;
        }
        bufferList_.push_back(bufferWrapper);
        // todo construct share memory by gstBuffer_
        // auto shareMem = xxx
        // if (shareMem == nullptr) {
        //     MEDIA_LOGE("No memory");
        //     gst_sample_unref(sample);
        //     break;
        // }
        // shareMemList_.push_back(shareMem);
        gst_sample_unref(sample);
        ret = (i == (bufferCount_ - 1)) ? MSERR_OK : MSERR_UNKNOWN;
    }
    CHECK_AND_RETURN_RET(ret == MSERR_OK, MSERR_UNKNOWN);
    for (auto it = bufferList_.begin(); it != bufferList_.end(); it++) {
        if ((*it)->gstBuffer_ != nullptr) {
            gst_buffer_unref((*it)->gstBuffer_);
        }
    }
    return ret;
}

int32_t SinkSurfaceImpl::Init()
{
    element_ = gst_element_factory_make("todo", nullptr);
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::Flush()
{
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    for (auto it = bufferList_.begin(); it != bufferList_.end(); it++) {
        (*it)->owner_ = BufferWrapper::DOWNSTREAM;
        if ((*it)->gstBuffer_ != nullptr) {
            gst_buffer_unref((*it)->gstBuffer_);
        }
    }
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::SetOutputSurface(sptr<Surface> surface)
{
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    g_object_set(element_, "surface", static_cast<gpointer>(surface), nullptr);
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::ReleaseOutputBuffer(uint32_t index, bool render)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(index < bufferCount_, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(index <= bufferList_.size(), MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(bufferList_[index]->owner_ == BufferWrapper::APP, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(bufferList_[index]->gstBuffer_ != nullptr, MSERR_UNKNOWN);
    bufferList_[index]->owner_ = BufferWrapper::DOWNSTREAM;
    g_signal_emit_by_name(G_OBJECT(element_), "render", &(bufferList_[index]->gstBuffer_));
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::SetParameter(const Format &format)
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("Unsupport");
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::SetCallback(const std::weak_ptr<IAVCodecEngineObs> &obs)
{
    std::unique_lock<std::mutex> lock(mutex_);
    obs_ = obs;
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    signalId_ = g_signal_connect(element_, "need-data", G_CALLBACK(OutputAvailableCb), this);
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::Configure(std::shared_ptr<ProcessorConfig> config)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    //g_object_set(G_OBJECT(element_), "caps", caps, nullptr);
    return MSERR_OK;
}

void SinkSurfaceImpl::OutputAvailableCb(const GstElement *sink, uint32_t size, gpointer self)
{
    (void)sink;
    CHECK_AND_RETURN_LOG(self != nullptr, "Null pointer exception");
    auto impl = static_cast<SinkSurfaceImpl *>(self);

    GstSample *sample = nullptr;
    g_signal_emit_by_name(G_OBJECT(impl->element_), "pull-sample", &sample);
    CHECK_AND_RETURN_LOG(sample != nullptr, "Failed to pull sample");
    GstBuffer *buf = gst_sample_get_buffer(sample);
    if (buf == nullptr) {
        MEDIA_LOGE("Failed to gst_sample_get_buffer");
        gst_sample_unref(sample);
        return;
    }

    uint32_t index = 0;
    for (auto it = impl->bufferList_.begin(); it != impl->bufferList_.end(); it++) {
        if ((*it)->gstBuffer_ != nullptr && (*it)->gstBuffer_ == buf) {
            index = (*it)->index_;
            (*it)->owner_ = BufferWrapper::SERVER;
            break;
        }
    }
    gst_sample_unref(sample);
    auto obs = impl->obs_.lock();
    CHECK_AND_RETURN_LOG(obs != nullptr, "Null pointer exception");
    // todo
    AVCodecBufferInfo info;
    obs->OnOutputBufferAvailable(index, info, AVCODEC_BUFFER_FLAG_NONE);
}
} // Media
} // OHOS
