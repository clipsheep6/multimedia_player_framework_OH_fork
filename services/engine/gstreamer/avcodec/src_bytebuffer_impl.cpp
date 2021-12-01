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
#include "scope_guard.h"
#include "securec.h"

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
    MEDIA_LOGD("QueueInputBuffer, index:%{public}d", index);
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(index < bufferCount_, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(index <= bufferList_.size() && index <= shareMemList_.size(), MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(bufferList_[index]->owner_ == BufferWrapper::APP, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(bufferList_[index]->gstBuffer_ != nullptr, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(shareMemList_[index] != nullptr, MSERR_UNKNOWN);

    if (static_cast<int32_t>(flag) | static_cast<int32_t>(AVCODEC_BUFFER_FLAG_CODEDC_DATA)) {
        MEDIA_LOGD("Handle codec data, index:%{public}d, bufferSize:%{public}d", index, info.size);
        uint8_t *address = shareMemList_[index]->GetBase();
        CHECK_AND_RETURN_RET(address != nullptr, MSERR_UNKNOWN);

        std::vector<uint8_t> sps;
        std::vector<uint8_t> pps;
        GetCodecData(address, info.size, sps, pps);
        MEDIA_LOGD("nalSize:%{public}d, sps size:%{public}d, pps size:%{public}d", nalSize_, sps.size(), pps.size());
        CHECK_AND_RETURN_RET(nalSize_ > 0 && sps.size() > 0 && pps.size() > 0, MSERR_VID_DEC_FAILED);

        GstBuffer *codecBuffer = AVCDecoderConfiguration(sps, pps);
        CHECK_AND_RETURN_RET(codecBuffer != nullptr, MSERR_UNKNOWN);
        int32_t ret = SetCodecData(codecBuffer);
        gst_buffer_unref(codecBuffer);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "Failed to set codec data");

        auto obs = obs_.lock();
        CHECK_AND_RETURN_RET(obs != nullptr, GST_FLOW_ERROR);
        obs->OnInputBufferAvailable(index);
        return ret;
    }

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

int32_t SrcBytebufferImpl::SetCodecData(GstBuffer *codecBuffer)
{
    g_object_set(G_OBJECT(element_), "codec-data", static_cast<gpointer>(codecBuffer), nullptr);
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
            MEDIA_LOGE("Failed to wrap buffer");
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

    CHECK_AND_RETURN_RET_LOG(findShmem == true, GST_FLOW_ERROR, "Illegal buffer");
    index = impl->bufferList_.size();
    impl->bufferList_.push_back(bufWrap);
    impl->bufferCount_++;
    MEDIA_LOGI("Counstrucet buffer wrapper, index:%{public}d, bufferCount %{public}d", index, impl->bufferCount_);
    return GST_FLOW_OK;
}

const uint8_t *SrcBytebufferImpl::FindNextNal(const uint8_t *start, const uint8_t *end)
{
    CHECK_AND_RETURN_RET(start != nullptr && end != nullptr, nullptr);
    // there is two kind of nal head. four byte 0x00000001 or three byte 0x000001
    while (start <= end - 4) {
        if (start[0] == 0x00 && start[1] == 0x00 && start[2] == 0x01) {
            nalSize_ = 3; // 0x000001 Nal
            return start;
        }
        if (start[0] == 0x00 && start[1] == 0x00 && start[2] == 0x00 && start[3] == 0x01) {
            nalSize_ = 4; // 0x00000001 Nal
            return start;
        }
        start++;
    }
    return end;
}

void SrcBytebufferImpl::GetCodecData(const uint8_t *data, int32_t len,
    std::vector<uint8_t> &sps, std::vector<uint8_t> &pps)
{
    CHECK_AND_RETURN(data != nullptr);
    const uint8_t *end = data + len - 1;
    const uint8_t *pBegin = data;
    const uint8_t *pEnd = nullptr;
    while (pBegin < end) {
        pBegin = FindNextNal(pBegin, end);
        if (pBegin == end) {
            break;
        }
        pBegin += nalSize_;
        pEnd = FindNextNal(pBegin, end);
        if (((*pBegin) & 0x1F) == 0x07) { // sps
            sps.assign(pBegin, pBegin + static_cast<int>(pEnd - pBegin));
        }
        if (((*pBegin) & 0x1F) == 0x08) { // pps
            pps.assign(pBegin, pBegin + static_cast<int>(pEnd - pBegin));
        }
        pBegin = pEnd;
    }
}

GstBuffer *SrcBytebufferImpl::AVCDecoderConfiguration(std::vector<uint8_t> &sps,
    std::vector<uint8_t> &pps)
{
    // 11 is the length of AVCDecoderConfigurationRecord field except sps and pps
    uint32_t codecBufferSize = sps.size() + pps.size() + 11;
    GstBuffer *codec = gst_buffer_new_allocate(nullptr, codecBufferSize, nullptr);
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "no memory");
    ON_SCOPE_EXIT(0) {
        gst_buffer_unref(codec);
    };
    GstMapInfo map = GST_MAP_INFO_INIT;
    CHECK_AND_RETURN_RET_LOG(gst_buffer_map(codec, &map, GST_MAP_READ) == TRUE, nullptr, "gst_buffer_map fail");

    ON_SCOPE_EXIT(1) {
        gst_buffer_unmap(codec, &map);
    };

    uint32_t offset = 0;
    map.data[offset++] = 0x01; // configurationVersion
    map.data[offset++] = sps[1]; // AVCProfileIndication
    map.data[offset++] = sps[2]; // profileCompatibility
    map.data[offset++] = sps[3]; // AVCLevelIndication
    map.data[offset++] = 0xff; // lengthSizeMinusOne

    map.data[offset++] = 0xe0 | 0x01; // numOfSequenceParameterSets
    map.data[offset++] = (sps.size() >> 8) & 0xff; // sequenceParameterSetLength high 8 bits
    map.data[offset++] = sps.size() & 0xff; // sequenceParameterSetLength low 8 bits
    // sequenceParameterSetNALUnit
    CHECK_AND_RETURN_RET(memcpy_s(map.data + offset, codecBufferSize - offset, &sps[0], sps.size()) == EOK, nullptr);
    offset += sps.size();

    map.data[offset++] = 0x01; // numOfPictureParameterSets
    map.data[offset++] = (pps.size() >> 8) & 0xff; // pictureParameterSetLength  high 8 bits
    map.data[offset++] = pps.size() & 0xff; // pictureParameterSetLength  low 8 bits
    // pictureParameterSetNALUnit
    CHECK_AND_RETURN_RET(memcpy_s(map.data + offset, codecBufferSize - offset, &pps[0], pps.size()) == EOK, nullptr);
    CANCEL_SCOPE_EXIT_GUARD(0);

    return codec;
}
} // Media
} // OHOS
