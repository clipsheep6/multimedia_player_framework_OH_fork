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
#include "scope_guard.h"
#include "securec.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SrcBytebufferImpl"};
    const uint32_t DEFAULT_BUFFER_COUNT = 5;
    const uint32_t DEFAULT_BUFFER_SIZE = 8000;
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
    MEDIA_LOGD("GetInputBuffer, index:%{public}d", index);
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
    return MSERR_OK;
}

int32_t SrcBytebufferImpl::SetParameter(const Format &format)
{
    MEDIA_LOGE("Unsupport");
    return MSERR_OK;
}

int32_t SrcBytebufferImpl::SetCallback(const std::weak_ptr<IAVCodecEngineObs> &obs)
{
    return MSERR_OK;
}
} // Media
} // OHOS
