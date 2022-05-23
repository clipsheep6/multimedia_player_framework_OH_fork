/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "hdi_in_buffer_mgr.h"
#include <algorithm>
#include <hdf_base.h>
#include "media_log.h"
#include "media_errors.h"
#include "hdi_codec_util.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HdiInBufferMgr"};
}

namespace OHOS {
namespace Media {
HdiInBufferMgr::HdiInBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

HdiInBufferMgr::~HdiInBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    std::for_each(preBuffers_.begin(), preBuffers_.end(), [&](GstBuffer *buffer){ gst_buffer_unref(buffer); });
    EmptyList(preBuffers_);
}

int32_t HdiInBufferMgr::UseBuffers(std::vector<GstBuffer *> buffers)
{
    std::for_each(buffers.begin(), buffers.end(), [&](GstBuffer *buffer){ gst_buffer_ref(buffer); });
    preBuffers_ = buffers;
    return GST_CODEC_OK;
}

std::shared_ptr<OmxCodecBuffer> HdiInBufferMgr::GetHdiEosBuffer()
{
    std::shared_ptr<OmxCodecBuffer> codecBuffer = nullptr;
    if (!availableBuffers_.empty()) {
        MEDIA_LOGD("Init eos buffer");
        codecBuffer = availableBuffers_.front();
        availableBuffers_.pop_front();
        codecBuffer->filledLen = 0;
        codecBuffer->flag |= OMX_BUFFERFLAG_EOS;
    }
    return codecBuffer;
}

int32_t HdiInBufferMgr::PushBuffer(GstBuffer *buffer)
{
    MEDIA_LOGD("PushBuffer start");
    std::unique_lock<std::mutex> lock(mutex_);
    bufferCond_.wait(lock, [this]() {return !availableBuffers_.empty() || isFlushed_ || !isStart_;});
    if (isFlushed_ || !isStart_) {
        return GST_CODEC_FLUSH;
    }
    std::shared_ptr<OmxCodecBuffer> codecBuffer = nullptr;
    if (buffer == nullptr) {
        codecBuffer = GetHdiEosBuffer();
    } else {
        codecBuffer = GetCodecBuffer(buffer);
    }
    CHECK_AND_RETURN_RET_LOG(codecBuffer != nullptr, GST_CODEC_ERROR, "Push buffer failed");
    auto ret = handle_->EmptyThisBuffer(handle_, codecBuffer.get());
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "EmptyThisBuffer failed");
    MEDIA_LOGD("PushBuffer end");
    return GST_CODEC_OK;
}

int32_t HdiInBufferMgr::FreeBuffers()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (availableBuffers_.size() != mPortDef_.nBufferCountActual) {
        MEDIA_LOGE("Erorr buffer number");
    }
    FreeCodecBuffers();
    std::for_each(preBuffers_.begin(), preBuffers_.end(), [&](GstBuffer *buffer){ gst_buffer_unref(buffer); });
    EmptyList(preBuffers_);
    return GST_CODEC_OK;
}

int32_t HdiInBufferMgr::CodecBufferAvailable(const OmxCodecBuffer *buffer)
{
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, GST_CODEC_ERROR, "EmptyBufferDone failed");
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto iter = codingBuffers_.begin(); iter != codingBuffers_.end(); ++iter) {
        if (iter->first != nullptr && iter->first->bufferId == buffer->bufferId) {
            availableBuffers_.push_back(iter->first);
            gst_buffer_unref(iter->second);
            (void)codingBuffers_.erase(iter);
            break;
        }
    }
    bufferCond_.notify_all();
    return GST_CODEC_OK;
}
}  // namespace Media
}  // namespace OHOS
