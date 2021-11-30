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

#include "src_bytebuffer_impl.h"
#include "gst_shmem_memory.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SrcBytebufferImpl"};
}

namespace OHOS {
namespace Media {
SrcBytebufferImpl::SrcBytebufferImpl()
{
}

SrcBytebufferImpl::~SrcBytebufferImpl()
{
    bufferList_.clear();
    shareMemList_.clear();
    if (element_ != nullptr) {
        gst_object_unref(element_);
        element_ = nullptr;
    }
}

int32_t SrcBytebufferImpl::Init()
{
    element_ = GST_ELEMENT_CAST(gst_object_ref(gst_element_factory_make("codecshmemsrc", "codecshmemsrc")));
    CHECK_AND_RETURN_RET_LOG(element_ != nullptr, MSERR_UNKNOWN, "Failed to gst_element_factory_make");
    // todo calculate
    g_object_set(element_, "buffer-size", 8000, nullptr);
    return MSERR_OK;
}

int32_t SrcBytebufferImpl::Flush()
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

std::shared_ptr<AVSharedMemory> SrcBytebufferImpl::GetInputBuffer(uint32_t index)
{
    MEDIA_LOGD("GetInputBuffer");
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(index < bufferCount_, nullptr);
    CHECK_AND_RETURN_RET(index <= bufferList_.size() && index <= shareMemList_.size(), nullptr);
    CHECK_AND_RETURN_RET(bufferList_[index]->owner_ == BufferWrapper::SERVER, nullptr);
    CHECK_AND_RETURN_RET(shareMemList_[index] != nullptr, nullptr);

    bufferList_[index]->owner_ = BufferWrapper::APP;
    return shareMemList_[index];
}

int32_t SrcBytebufferImpl::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    MEDIA_LOGD("QueueInputBuffer");
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(index < bufferCount_, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(index <= bufferList_.size(), MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(bufferList_[index]->owner_ == BufferWrapper::APP, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(bufferList_[index]->gstBuffer_ != nullptr, MSERR_UNKNOWN);

    GST_BUFFER_PTS(bufferList_[index]->gstBuffer_) = info.presentationTimeUs;
    GST_BUFFER_OFFSET(bufferList_[index]->gstBuffer_) = info.offset;
    GST_BUFFER_OFFSET_END(bufferList_[index]->gstBuffer_) = info.offset + info.size;

    if (static_cast<int32_t>(flag) | static_cast<int32_t>(AVCODEC_BUFFER_FLAG_SYNC_FRAME)) {
        GST_BUFFER_FLAG_SET(bufferList_[index]->gstBuffer_, GST_BUFFER_FLAG_RESYNC);
    }

    CHECK_AND_RETURN_RET(gst_mem_pool_src_push_buffer((GstMemPoolSrc *)element_,
        bufferList_[index]->gstBuffer_) == GST_FLOW_OK, MSERR_UNKNOWN);
    bufferList_[index]->owner_ = BufferWrapper::DOWNSTREAM;
    return MSERR_OK;
}

int32_t SrcBytebufferImpl::SetParameter(const Format &format)
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGE("Unsupport");
    return MSERR_OK;
}

int32_t SrcBytebufferImpl::SetCallback(const std::weak_ptr<IAVCodecEngineObs> &obs)
{
    std::unique_lock<std::mutex> lock(mutex_);
    obs_ = obs;
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    gst_mem_pool_src_set_callback(GST_MEM_POOL_SRC(element_), NeedDataCb, this, nullptr);
    MEDIA_LOGD("SetCallback success");
    return MSERR_OK;
}

int32_t SrcBytebufferImpl::Configure(std::shared_ptr<ProcessorConfig> config)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    g_object_set(G_OBJECT(element_), "caps", config->caps_, nullptr);
    return MSERR_OK;
}

GstFlowReturn SrcBytebufferImpl::NeedDataCb(GstMemPoolSrc *src, gpointer userData)
{
    MEDIA_LOGD("NeedDataCb");
    CHECK_AND_RETURN_RET(src != nullptr && userData != nullptr, GST_FLOW_ERROR);
    auto impl = static_cast<SrcBytebufferImpl *>(userData);

    GstBuffer *buffer = gst_mem_pool_src_pull_buffer(src);
    CHECK_AND_RETURN_RET(buffer != nullptr, GST_FLOW_ERROR);

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
    obs->OnInputBufferAvailable(index);
    return GST_FLOW_OK;
}

GstFlowReturn SrcBytebufferImpl::ConstructBufferWrapper(SrcBytebufferImpl *impl, GstBuffer *buffer, uint32_t &index)
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
