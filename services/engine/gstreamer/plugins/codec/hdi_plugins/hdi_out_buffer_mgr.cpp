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

#include "hdi_out_buffer_mgr.h"
#include <algorithm>
#include <hdf_base.h>
#include "media_log.h"
#include "media_errors.h"
#include "hdi_codec_util.h"
#include "buffer_type_meta.h"
#include "scope_guard.h"
#include "media_dfx.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HdiOutBufferMgr"};
}

namespace OHOS {
namespace Media {
HdiOutBufferMgr::HdiOutBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

HdiOutBufferMgr::~HdiOutBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    ClearmBuffers();
}

int32_t HdiOutBufferMgr::Start()
{
    MEDIA_LOGD("Enter Start mBuffers size %{public}zu", mBuffers.size());
    std::unique_lock<std::mutex> lock(mutex_);
    isStart_ = true;
    isFlushed_ = false;
    isFormatChange_ = false;
    while (!mBuffers.empty()) {
        GstBufferWrap buffer = mBuffers.front();
        std::shared_ptr<HdiBufferWrap> codecBuffer = GetCodecBuffer(buffer.gstBuffer);
        CHECK_AND_RETURN_RET_LOG(codecBuffer != nullptr, GST_CODEC_ERROR, "Push buffer failed");
        auto ret = HdiFillThisBuffer(handle_, &codecBuffer->hdiBuffer);
        CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "FillThisBuffer failed");
        mBuffers.pop_front();
        gst_buffer_unref(buffer.gstBuffer);
    }
    MEDIA_LOGD("Quit Start");
    return GST_CODEC_OK;
}

int32_t HdiOutBufferMgr::PushBuffer(GstBuffer *buffer)
{
    MEDIA_LOGD("mBuffers %{public}zu, available %{public}zu, codingBuffers %{public}zu",
        mBuffers.size(), availableBuffers_.size(), codingBuffers_.size());
    MediaTrace::CounterTrace("AvailableBuffers", availableBuffers_.size());
    std::unique_lock<std::mutex> lock(mutex_);
    if (isFormatChange_) {
        MEDIA_LOGD("It is formatchange");
        return GST_CODEC_OK;
    }
    if (isFlushed_ || !isStart_) {
        MEDIA_LOGD("isFlush %{public}d isStart %{public}d", isFlushed_.load(), isStart_);
        return GST_CODEC_FLUSH;
    }
    std::shared_ptr<HdiBufferWrap> codecBuffer = nullptr;
    codecBuffer = GetCodecBuffer(buffer);
    CHECK_AND_RETURN_RET_LOG(codecBuffer != nullptr, GST_CODEC_ERROR, "Push buffer failed");
    {
        MediaTrace trace("HdiOutBufferMgr::FillThisBuffer");
        auto ret = HdiFillThisBuffer(handle_, &codecBuffer->hdiBuffer);
        CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "FillThisBuffer failed");
    }
    return GST_CODEC_OK;
}

int32_t HdiOutBufferMgr::PullBuffer(GstBuffer **buffer)
{
    MEDIA_LOGD("Enter PullBuffer");
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, GST_CODEC_ERROR, "buffer is nullptr");
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("mBuffers.size %{public}zu, isFlush %{public}d, isStart %{public}d, isFormatChange_ %{public}d",
               mBuffers.size(), isFlushed_.load(), isStart_, isFormatChange_);
    bufferCond_.wait(lock, [this]() { return !mBuffers.empty() || isFlushed_ || !isStart_ || isFormatChange_; });
    if (isFormatChange_ && !mBuffers.empty()) {
        MEDIA_LOGD("format change, waiting mBuffers empty");
    } else if (isFlushed_ || !isStart_) {
        MEDIA_LOGD("0x%{public}06" PRIXPTR " isFlush %{public}d isStart %{public}d",
            FAKE_POINTER(this), isFlushed_.load(), isStart_);
        return GST_CODEC_FLUSH;
    }
    if (!mBuffers.empty()) {
        MEDIA_LOGD("mBuffers %{public}zu, available %{public}zu", mBuffers.size(), availableBuffers_.size());
        MediaTrace::CounterTrace("WaitingForDisplayBuffers", mBuffers.size());
        GstBufferWrap bufferWarp = mBuffers.front();
        mBuffers.pop_front();
        if (bufferWarp.isEos) {
            gst_buffer_unref(bufferWarp.gstBuffer);
            MEDIA_LOGD("buffer is in eos");
            return GST_CODEC_EOS;
        }
        (*buffer) = bufferWarp.gstBuffer;
        return GST_CODEC_OK;
    }
    return GST_CODEC_ERROR;
}

int32_t HdiOutBufferMgr::FreeBuffers()
{
    MEDIA_LOGD("FreeBuffers");
    std::unique_lock<std::mutex> lock(mutex_);
    static constexpr int32_t timeout = 2;
    MEDIA_LOGD("availableBuffers_.size() == mPortDef_.nBufferCountActual %{public}d, bufferReleased_ %{public}d",
               availableBuffers_.size() == mPortDef_.nBufferCountActual, bufferReleased_ == true);
    freeCond_.wait_for(lock, std::chrono::seconds(timeout),
        [this]() { return availableBuffers_.size() == mPortDef_.nBufferCountActual || bufferReleased_; });
    FreeCodecBuffers();
    MEDIA_LOGI("0x%{public}06" PRIXPTR " unref mBuffers %{public}zu", FAKE_POINTER(this), mBuffers.size());
    std::for_each(mBuffers.begin(), mBuffers.end(), [&](GstBufferWrap buffer) { gst_buffer_unref(buffer.gstBuffer); });
    EmptyList(mBuffers);
    return GST_CODEC_OK;
}

int32_t HdiOutBufferMgr::CodecBufferAvailable(const OmxCodecBuffer *buffer)
{
    MEDIA_LOGD("codecBufferAvailable");
    MediaTrace::CounterTrace("WaitingForDisplayBuffers", mBuffers.size());
    MediaTrace::CounterTrace("AvailableBuffers", availableBuffers_.size());
    MediaTrace::CounterTrace("CodingBuffers", codingBuffers_.size());
    MediaTrace trace("HdiOutBufferMgr::CodecBufferAvailable");
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, GST_CODEC_ERROR, "FillBufferDone failed");
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("mBuffers %{public}zu, available %{public}zu codingBuffers %{public}zu",
        mBuffers.size(), availableBuffers_.size(), codingBuffers_.size());
    for (auto iter = codingBuffers_.begin(); iter != codingBuffers_.end(); ++iter) {
        if (iter->first != nullptr && iter->first->hdiBuffer.bufferId == buffer->bufferId) {
            availableBuffers_.push_back(iter->first);
            GstBufferWrap bufferWarp = {};
            if (buffer->flag & OMX_BUFFERFLAG_EOS) {
                MEDIA_LOGD("Bufferavailable, but buffer is eos");
                bufferWarp.isEos = true;
            } else {
                gst_buffer_resize(iter->second, buffer->offset, buffer->filledLen);
            }
            bufferWarp.gstBuffer = iter->second;
            if (bufferWarp.gstBuffer != nullptr) {
                GST_BUFFER_PTS(bufferWarp.gstBuffer) = buffer->pts;
                MEDIA_LOGD("get from hdi, pts = %{public}" PRId64 "", buffer->pts);
            }
            if (!firstFramePrinted && buffer->pts != 0) {
                MEDIA_LOGI("first output buffer, pts = %{public}" PRId64 "", buffer->pts);
                firstFramePrinted = true;
            }
            SetFlagToBuffer(bufferWarp.gstBuffer, buffer->flag);
            mBuffers.push_back(bufferWarp);
            (void)codingBuffers_.erase(iter);
            break;
        }
    }
    NotifyAvailable();
    bufferCond_.notify_all();
    return GST_CODEC_OK;
}

void HdiOutBufferMgr::ClearmBuffers()
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGI("0x%{public}06" PRIXPTR " unref mBuffers %{public}zu", FAKE_POINTER(this), mBuffers.size());
    while (!mBuffers.empty()) {
        GstBufferWrap bufferWarp = mBuffers.front();
        mBuffers.pop_front();
        gst_buffer_unref(bufferWarp.gstBuffer);
    }
}
}  // namespace Media
}  // namespace OHOS
