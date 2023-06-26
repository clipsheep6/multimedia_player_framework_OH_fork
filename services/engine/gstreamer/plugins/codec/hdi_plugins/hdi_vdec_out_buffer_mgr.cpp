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

#include "hdi_vdec_out_buffer_mgr.h"
#include <hdf_base.h>
#include "media_log.h"
#include "media_errors.h"
#include "hdi_codec_util.h"
#include "buffer_type_meta.h"
#include "scope_guard.h"
#include "gst_producer_surface_pool.h"
#include "media_dfx.h"
#include "gst_utils.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HdiVdecOutBufferMgr"};
}

namespace OHOS {
namespace Media {
HdiVdecOutBufferMgr::HdiVdecOutBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

HdiVdecOutBufferMgr::~HdiVdecOutBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    ClearPreBuffers();
}

int32_t HdiVdecOutBufferMgr::UseHandleMems(std::vector<GstBuffer *> &buffers)
{
    MEDIA_LOGD("Enter UseHandleMems");
    CHECK_AND_RETURN_RET_LOG(buffers.size() == mPortDef_.nBufferCountActual, GST_CODEC_ERROR, "BufferNum error");
    for (auto buffer : buffers) {
        GstBufferTypeMeta *bufferType = gst_buffer_get_buffer_type_meta(buffer);
        CHECK_AND_RETURN_RET_LOG(bufferType != nullptr, GST_CODEC_ERROR, "bufferType is nullptr");
        std::shared_ptr<HdiBufferWrap> codecBuffer = std::make_shared<HdiBufferWrap>();
        codecBuffer->buf = bufferType->buf;
        codecBuffer->hdiBuffer.size = sizeof(OmxCodecBuffer);
        codecBuffer->hdiBuffer.version = verInfo_.compVersion;
        codecBuffer->hdiBuffer.bufferType = CodecBufferType::CODEC_BUFFER_TYPE_HANDLE;
        codecBuffer->hdiBuffer.buffer = reinterpret_cast<uint8_t *>(bufferType->buf);
        codecBuffer->hdiBuffer.bufferLen = bufferType->bufLen;
        codecBuffer->hdiBuffer.allocLen = bufferType->bufLen;
        int32_t ret = GST_CODEC_OK;
        LISTENER(ret = handle_->UseBuffer(handle_, (uint32_t)mPortIndex_, &codecBuffer->hdiBuffer),
            "Hdi::UseBuffer", PlayerXCollie::timerTimeout)
        CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "UseBuffer failed");
        availableBuffers_.push_back(codecBuffer);
        GstBufferWrap bufferWarp = {};
        bufferWarp.gstBuffer = buffer;
        mBuffers.push_back(bufferWarp);
        gst_buffer_ref(buffer);
    }
    return GST_CODEC_OK;
}

int32_t HdiVdecOutBufferMgr::UseBuffers(std::vector<GstBuffer *> buffers)
{
    MEDIA_LOGD("Enter UseBuffers");
    ON_SCOPE_EXIT(0) {
        std::for_each(buffers.begin(), buffers.end(), [&](GstBuffer *buffer) { gst_buffer_unref(buffer); });
    };
    if (isCallBackMode_) {
        MEDIA_LOGD("Enter callback mode");
        auto ret = HdiGetParameter(handle_, OMX_IndexParamPortDefinition, mPortDef_);
        CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiGetParameter failed");
        {
            std::unique_lock<std::mutex> lock(mutex_);
            static constexpr int32_t timeout = 2;
            preBufferCond_.wait_for(lock, std::chrono::seconds(timeout),
                [this]() { return preBuffers_.size() == mPortDef_.nBufferCountActual || bufferReleased_; });
            buffers.swap(preBuffers_);
        }
        enableNativeBuffer_ = true;
        (void)UseHandleMems(buffers);
        return GST_CODEC_OK;
    }
    CHECK_AND_RETURN_RET_LOG(!buffers.empty(), GST_CODEC_ERROR, "buffers is empty");
    CHECK_AND_RETURN_RET_LOG(buffers[0] != nullptr, GST_CODEC_ERROR, "first buffer is empty");
    GstBufferTypeMeta *bufferType = gst_buffer_get_buffer_type_meta(buffers[0]);
    CHECK_AND_RETURN_RET_LOG(bufferType != nullptr, GST_CODEC_ERROR, "bufferType is nullptr");
    enableNativeBuffer_ = bufferType->type == GstBufferType::BUFFER_TYPE_HANDLE ? true : false;
    if (!enableNativeBuffer_) {
        auto omxBuffers = PreUseAshareMems(buffers);
        (void)UseHdiBuffers(omxBuffers);
        for (auto buffer : buffers) {
            GstBufferWrap bufferWarp = {};
            bufferWarp.gstBuffer = buffer;
            mBuffers.push_back(bufferWarp);
            gst_buffer_ref(buffer);
        }
    }
    MEDIA_LOGD("UseBuffer end");
    return GST_CODEC_OK;
}

void HdiVdecOutBufferMgr::UpdateCodecMeta(GstBufferTypeMeta *bufferType, std::shared_ptr<HdiBufferWrap> &codecBuffer)
{
    MEDIA_LOGD("Enter UpdateCodecMeta");
    CHECK_AND_RETURN_LOG(codecBuffer != nullptr, "bufferType is nullptr");
    CHECK_AND_RETURN_LOG(bufferType != nullptr, "bufferType is nullptr");
    if (enableNativeBuffer_) {
        codecBuffer->hdiBuffer.fenceFd = bufferType->fenceFd;
    } else {
        HdiBufferMgr::UpdateCodecMeta(bufferType, codecBuffer);
    }
}

using HdiVdecOutBufferMgrWrapper = ThizWrapper<HdiVdecOutBufferMgr>;

GstFlowReturn HdiVdecOutBufferMgr::NewBuffer(GstBuffer *buffer, gpointer userData)
{
    MediaTrace trace("HdiVdecOutBufferMgr::NewBuffer");
    ON_SCOPE_EXIT(0) {
        gst_buffer_unref(buffer);
    };

    auto thizStrong = HdiVdecOutBufferMgrWrapper::TakeStrongThiz(userData);
    CHECK_AND_RETURN_RET_LOG(thizStrong != nullptr, GST_FLOW_ERROR, "thizStrong is null");
    CANCEL_SCOPE_EXIT_GUARD(0);
    if (thizStrong->OnNewBuffer(buffer) == GST_CODEC_ERROR) {
        MEDIA_LOGE("new buffer done failed");
        return GST_FLOW_ERROR;
    }
    return GST_FLOW_OK;
}

int32_t HdiVdecOutBufferMgr::FreeBuffers()
{
    HdiOutBufferMgr::FreeBuffers();
    {
        std::unique_lock<std::mutex> lock(mutex_);
        ClearPreBuffers();
    }
    return GST_CODEC_OK;
}

int32_t HdiVdecOutBufferMgr::OnNewBuffer(GstBuffer *buffer)
{
    MEDIA_LOGD("OnNewBuffer");
    if (isFlushed_) {
        std::unique_lock<std::mutex> lock(mutex_);
        GstBufferWrap bufferWarp = {false, buffer};
        mBuffers.push_back(bufferWarp);
    } else if (isStart_) {
        auto ret = HdiOutBufferMgr::PushBuffer(buffer);
        // PushBuffer will not deal buffer ref count, we need unref after push.
        gst_buffer_unref(buffer);
        return ret;
    } else {
        std::unique_lock<std::mutex> lock(mutex_);
        // preBuffers get the ref count.
        preBuffers_.push_back(buffer);
        preBufferCond_.notify_all();
    }
    return GST_CODEC_OK;
}

void HdiVdecOutBufferMgr::SetOutputPool(GstBufferPool *pool)
{
    MEDIA_LOGI("SetOutputPool");
    pool_ = pool;
    HdiVdecOutBufferMgrWrapper *wrapper = new(std::nothrow) HdiVdecOutBufferMgrWrapper(shared_from_this());
    CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");
    gst_producer_surface_pool_set_callback(pool, HdiVdecOutBufferMgr::NewBuffer, wrapper,
        &HdiVdecOutBufferMgrWrapper::OnDestory);
    isCallBackMode_ = true;
}

void HdiVdecOutBufferMgr::ClearPreBuffers()
{
    std::for_each(preBuffers_.begin(), preBuffers_.end(), [&](auto buffer) { gst_buffer_unref(buffer); });
    std::vector<GstBuffer *> temp;
    swap(temp, preBuffers_);
}
}  // namespace Media
}  // namespace OHOS
