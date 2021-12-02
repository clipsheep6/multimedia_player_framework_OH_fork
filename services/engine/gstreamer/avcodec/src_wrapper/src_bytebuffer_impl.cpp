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
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SrcBytebufferImpl"};
    const uint32_t DEFAULT_BUFFER_COUNT = 5;
    const uint32_t DEFAULT_BUFFER_SIZE = 30000;
}

namespace OHOS {
namespace Media {
SrcBytebufferImpl::SrcBytebufferImpl()
{
}

SrcBytebufferImpl::~SrcBytebufferImpl()
{
    bufferList_.clear();
    if (element_ != nullptr) {
        gst_object_unref(element_);
        element_ = nullptr;
    }
}

int32_t SrcBytebufferImpl::Init()
{
    element_ = GST_ELEMENT_CAST(gst_object_ref(gst_element_factory_make("appsrc", "src")));
    CHECK_AND_RETURN_RET_LOG(element_ != nullptr, MSERR_UNKNOWN, "Failed to gst_element_factory_make");

    bufferCount_ = DEFAULT_BUFFER_COUNT;
    bufferSize_ = DEFAULT_BUFFER_SIZE;

    return MSERR_OK;
}

int32_t SrcBytebufferImpl::Configure(std::shared_ptr<ProcessorConfig> config)
{
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    g_object_set(G_OBJECT(element_), "caps", config->caps_, nullptr);

    for (uint32_t i = 0; i < bufferCount_; i++) {
        auto mem = AVSharedMemory::Create(bufferSize_, AVSharedMemory::Flags::FLAGS_READ_WRITE, "appsrc");
        CHECK_AND_RETURN_RET(mem != nullptr, MSERR_NO_MEMORY);

        GstBuffer *buffer = gst_buffer_new_allocate(nullptr, static_cast<gsize>(DEFAULT_BUFFER_SIZE), nullptr);
        CHECK_AND_RETURN_RET(buffer != nullptr, MSERR_NO_MEMORY);

        auto bufWrap = std::make_shared<BufferWrapper>(mem, buffer, bufferList_.size(), BufferWrapper::SERVER);
        CHECK_AND_RETURN_RET(bufWrap != nullptr, MSERR_NO_MEMORY);
        bufferList_.push_back(bufWrap);
    }

    return MSERR_OK;
}

int32_t SrcBytebufferImpl::Flush()
{
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto it = bufferList_.begin(); it != bufferList_.end(); it++) {
        (*it)->owner_ = BufferWrapper::SERVER;
    }
    return MSERR_OK;
}

std::shared_ptr<AVSharedMemory> SrcBytebufferImpl::GetInputBuffer(uint32_t index)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(index < bufferCount_ && index <= bufferList_.size(), nullptr);
    CHECK_AND_RETURN_RET(bufferList_[index]->owner_ == BufferWrapper::SERVER, nullptr);

    bufferList_[index]->owner_ = BufferWrapper::APP;
    return bufferList_[index]->mem_;
}

int32_t SrcBytebufferImpl::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(index < bufferCount_ && index <= bufferList_.size(), MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(bufferList_[index]->owner_ == BufferWrapper::SERVER, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(bufferList_[index]->mem_ != nullptr, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(bufferList_[index]->gstBuffer_ != nullptr, MSERR_UNKNOWN);

    uint8_t *address = bufferList_[index]->mem_->GetBase();
    CHECK_AND_RETURN_RET((info.offset + info.size) <= bufferList_[index]->mem_->GetSize(), MSERR_INVALID_VAL);

    gsize size = gst_buffer_fill(bufferList_[index]->gstBuffer_, 0, (char *)address + info.offset, info.size);
    CHECK_AND_RETURN_RET(size == static_cast<gsize>(info.size), MSERR_UNKNOWN);

    GST_BUFFER_PTS(bufferList_[index]->gstBuffer_) = info.presentationTimeUs;
    GST_BUFFER_OFFSET(bufferList_[index]->gstBuffer_) = 0;
    GST_BUFFER_OFFSET_END(bufferList_[index]->gstBuffer_) = info.size;

    int32_t ret = GST_FLOW_OK;
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    g_signal_emit_by_name(element_, "push-buffer", bufferList_[index]->gstBuffer_, &ret);

    auto obs = obs_.lock();
    CHECK_AND_RETURN_RET(obs != nullptr, MSERR_UNKNOWN);
    obs->OnInputBufferAvailable(index);

    return MSERR_OK;
}

int32_t SrcBytebufferImpl::SetCallback(const std::weak_ptr<IAVCodecEngineObs> &obs)
{
    std::unique_lock<std::mutex> lock(mutex_);
    obs_ = obs;
    return MSERR_OK;
}
} // Media
} // OHOS
