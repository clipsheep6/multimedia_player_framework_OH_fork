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

#include "sink_bytebuffer_impl.h"
#include "gst_shmem_memory.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SinkBytebufferImpl"};
}

namespace OHOS {
namespace Media {
SinkBytebufferImpl::SinkBytebufferImpl()
{
}

SinkBytebufferImpl::~SinkBytebufferImpl()
{
    bufferList_.clear();
    shareMemList_.clear();
    if (element_ != nullptr) {
        gst_object_unref(element_);
        element_ = nullptr;
    }
}

int32_t SinkBytebufferImpl::Init()
{
    element_ = GST_ELEMENT_CAST(gst_object_ref(gst_element_factory_make("sharedmemsink", nullptr)));
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t SinkBytebufferImpl::Flush()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    for (auto it = bufferList_.begin(); it != bufferList_.end(); it++) {
        if ((*it)->owner_ != BufferWrapper::DOWNSTREAM && (*it)->gstBuffer_ != nullptr) {
            gst_buffer_unref((*it)->gstBuffer_);
        }
        (*it)->owner_ = BufferWrapper::DOWNSTREAM;
    }
    return MSERR_OK;
}

std::shared_ptr<AVSharedMemory> SinkBytebufferImpl::GetOutputBuffer(uint32_t index)
{
    MEDIA_LOGD("GetOutputBuffer");
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(index < bufferCount_, nullptr);
    CHECK_AND_RETURN_RET(index <= bufferList_.size() && index <= shareMemList_.size(), nullptr);
    CHECK_AND_RETURN_RET(bufferList_[index]->owner_ == BufferWrapper::SERVER, nullptr);
    CHECK_AND_RETURN_RET(shareMemList_[index] != nullptr, nullptr);

    bufferList_[index]->owner_ = BufferWrapper::APP;
    return shareMemList_[index];
}

int32_t SinkBytebufferImpl::ReleaseOutputBuffer(uint32_t index, bool render)
{
    MEDIA_LOGD("ReleaseOutputBuffer");
    (void)render;
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(index < bufferCount_, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(index <= bufferList_.size(), MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(bufferList_[index]->owner_ == BufferWrapper::APP, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(bufferList_[index]->gstBuffer_ != nullptr, MSERR_UNKNOWN);

    gst_buffer_unref(bufferList_[index]->gstBuffer_);
    bufferList_[index]->owner_ = BufferWrapper::DOWNSTREAM;
    return MSERR_OK;
}

int32_t SinkBytebufferImpl::SetParameter(const Format &format)
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGE("Unsupport");
    return MSERR_OK;
}

int32_t SinkBytebufferImpl::SetCallback(const std::weak_ptr<IAVCodecEngineObs> &obs)
{
    std::unique_lock<std::mutex> lock(mutex_);
    obs_ = obs;
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    GstMemSinkCallbacks callback = { nullptr, nullptr, OutputAvailableCb};
    gst_mem_sink_set_callback((GstMemSink *)element_, &callback, this, nullptr);
    return MSERR_OK;
}

int32_t SinkBytebufferImpl::Configure(std::shared_ptr<ProcessorConfig> config)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    g_object_set(G_OBJECT(element_), "caps", config->caps_, nullptr);
    return MSERR_OK;
}

GstFlowReturn SinkBytebufferImpl::OutputAvailableCb(GstMemSink *sink, GstBuffer *buffer, gpointer userData)
{
    MEDIA_LOGD("OutputAvailableCb");
    CHECK_AND_RETURN_RET(sink != nullptr && buffer != nullptr && userData != nullptr, GST_FLOW_ERROR);
    auto impl = static_cast<SinkBytebufferImpl *>(userData);

    uint32_t index = 0;
    bool findBuffer = false;
    for (auto it = impl->bufferList_.begin(); it != impl->bufferList_.end(); it++) {
        if ((*it) != nullptr && (*it)->gstBuffer_ == buffer) {
            findBuffer = true;
            index = (*it)->index_;
            (*it)->owner_ = BufferWrapper::SERVER;
            break;
        }
    }

    if (!findBuffer) {
        if (ConstructBufferWrapper(impl, buffer, index) != GST_FLOW_OK) {
            gst_buffer_unref(buffer);
            return GST_FLOW_ERROR;
        }
    }

    auto obs = impl->obs_.lock();
    CHECK_AND_RETURN_RET(obs != nullptr, GST_FLOW_ERROR);
    AVCodecBufferInfo info;
    info.presentationTimeUs = GST_BUFFER_PTS(buffer);
    info.offset = GST_BUFFER_OFFSET(buffer);
    info.size = GST_BUFFER_OFFSET_END(buffer) - GST_BUFFER_OFFSET(buffer);
    obs->OnOutputBufferAvailable(index, info, AVCODEC_BUFFER_FLAG_NONE);
    return GST_FLOW_OK;
}

GstFlowReturn SinkBytebufferImpl::ConstructBufferWrapper(SinkBytebufferImpl *impl, GstBuffer *buffer, uint32_t &index)
{
    CHECK_AND_RETURN_RET(impl != nullptr && buffer != nullptr, GST_FLOW_ERROR);

    auto bufWrap = std::make_shared<BufferWrapper>(buffer, impl->bufferList_.size(), BufferWrapper::SERVER);
    CHECK_AND_RETURN_RET_LOG(bufWrap != nullptr, GST_FLOW_ERROR, "No memory");

    bool findShmem = false;
    for (guint i = 0; i < gst_buffer_n_memory(buffer); i++) {
        GstMemory *memory = gst_buffer_peek_memory(buffer, i);
        if (!gst_is_shmem_memory(memory)) {
            continue;
        } else {
            GstShMemMemory *shmem = reinterpret_cast<GstShMemMemory *>(memory);
            CHECK_AND_RETURN_RET(shmem->mem != nullptr, GST_FLOW_ERROR);
            findShmem = true;
            impl->shareMemList_.push_back(shmem->mem);
            break;
        }
    }

    CHECK_AND_RETURN_RET(findShmem == true, GST_FLOW_ERROR);
    impl->bufferList_.push_back(bufWrap);
    index = impl->bufferList_.size();
    impl->bufferCount_++;

    return GST_FLOW_OK;
}
} // Media
} // OHOS
