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

#include "hdi_buffer_mgr.h"
#include <hdf_base.h>
#include "hdi_codec_util.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HdiBufferMgr"};
}

namespace OHOS {
namespace Media {
HdiBufferMgr::HdiBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

HdiBufferMgr::~HdiBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t HdiBufferMgr::Start()
{
    std::unique_lock<std::mutex> lock(mutex_);
    isStart_ = true;
    isFlushed_ = false;
    return GST_CODEC_OK;
}

void HdiBufferMgr::Init(CodecComponentType *handle, int32_t index)
{
    handle_ = handle;
    mPortIndex_ = index;
    mPortDef_.nPortIndex = (uint32_t)index;
}

std::shared_ptr<OmxCodecBuffer> HdiBufferMgr::GetCodecBuffer(GstBuffer *buffer)
{
    std::shared_ptr<OmxCodecBuffer> codecBuffer = nullptr;
    GstBufferTypeMeta *bufferType = gst_buffer_get_buffer_type_meta(buffer);
    CHECK_AND_RETURN_RET_LOG(bufferType != nullptr, nullptr, "bufferType is nullptr");
    for (auto iter = availableBuffers_.begin(); iter != availableBuffers_.end(); ++iter) {
        if (*iter != nullptr) {
            if ((*iter)->bufferId == bufferType->id) {
                codecBuffer = *iter;
                codingBuffers_.push_back(std::make_pair(codecBuffer, buffer));
                gst_buffer_ref(buffer);
                (void)availableBuffers_.erase(iter);
                UpdateCodecMeta(bufferType, codecBuffer);
                codecBuffer->pts = GST_BUFFER_PTS(buffer);
                break;
            }
        }
    }
    return codecBuffer;
}

void HdiBufferMgr::UpdateCodecMeta(GstBufferTypeMeta *bufferType, std::shared_ptr<OmxCodecBuffer> &codecBuffer)
{
    CHECK_AND_RETURN_LOG(codecBuffer != nullptr, "bufferType is nullptr");
    CHECK_AND_RETURN_LOG(bufferType != nullptr, "bufferType is nullptr");
    codecBuffer->allocLen = bufferType->totalSize;
    codecBuffer->offset = bufferType->offset;
    codecBuffer->filledLen = bufferType->length;
    codecBuffer->fenceFd = bufferType->fenceFd;
    codecBuffer->type = bufferType->memFlag == FLAGS_READ_ONLY ? READ_ONLY_TYPE : READ_WRITE_TYPE;
}

int32_t HdiBufferMgr::UseAshareMems(std::vector<GstBuffer *> &buffers)
{
    auto ret = HdiGetParameter(handle_, OMX_IndexParamPortDefinition, mPortDef_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiGetParameter failed");
    CHECK_AND_RETURN_RET_LOG(buffers.size() == mPortDef_.nBufferCountActual, GST_CODEC_ERROR, "BufferNum error");
    for (auto buffer : buffers) {
        CHECK_AND_RETURN_RET_LOG(buffer != nullptr, GST_CODEC_ERROR, "buffer is nullptr");
        GstBufferTypeMeta *bufferType = gst_buffer_get_buffer_type_meta(buffer);
        CHECK_AND_RETURN_RET_LOG(bufferType != nullptr, GST_CODEC_ERROR, "bufferType is nullptr");
        std::shared_ptr<OmxCodecBuffer> codecBuffer = std::make_shared<OmxCodecBuffer>();
        codecBuffer->size = sizeof(OmxCodecBuffer);
        codecBuffer->version.s.nVersionMajor = 1;
        codecBuffer->bufferType = CodecBufferType::CODEC_BUFFER_TYPE_AVSHARE_MEM_FD;
        codecBuffer->bufferLen = sizeof(bufferType->buf);
        codecBuffer->buffer = reinterpret_cast<uint8_t *>(bufferType->buf);
        codecBuffer->allocLen = bufferType->totalSize;
        codecBuffer->offset = bufferType->offset;
        codecBuffer->filledLen = bufferType->length;
        codecBuffer->fenceFd = bufferType->fenceFd;
        codecBuffer->flag = 0;
        codecBuffer->type = bufferType->memFlag == FLAGS_READ_ONLY ? READ_ONLY_TYPE : READ_WRITE_TYPE;
        auto ret = handle_->UseBuffer(handle_, (uint32_t)mPortIndex_, codecBuffer.get());
        CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "UseBuffer failed");
        bufferType->id = codecBuffer->bufferId;
        availableBuffers_.push_back(codecBuffer);
    }
    return GST_CODEC_OK;
}

void HdiBufferMgr::FreeCodecBuffers()
{
    for (auto codecBuffer : availableBuffers_) {
        auto ret = handle_->FreeBuffer(handle_, mPortIndex_, codecBuffer.get());
        if (ret != HDF_SUCCESS) {
            MEDIA_LOGE("free buffer %{public}u fail", codecBuffer->bufferId);
        }
    }
    EmptyList(availableBuffers_);
}

int32_t HdiBufferMgr::Stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    isStart_ = false;
    bufferCond_.notify_all();
    flushCond_.notify_all();
    return GST_CODEC_OK;
}

int32_t HdiBufferMgr::Flush(bool enable)
{
    MEDIA_LOGD("start flush %{public}d", enable);
    std::unique_lock<std::mutex> lock(mutex_);
    isFlushing_ = enable;
    if (isFlushing_) {
        bufferCond_.notify_all();
        isFlushed_ = true;
    }
    if (!isFlushing_) {
        flushCond_.notify_all();
    }
    return GST_CODEC_OK;
}

void HdiBufferMgr::WaitFlushed()
{
    std::unique_lock<std::mutex> lock(mutex_);
    flushCond_.wait(lock, [this]() { return !isFlushing_ || !isStart_; });
}
}  // namespace Media
}  // namespace OHOS
