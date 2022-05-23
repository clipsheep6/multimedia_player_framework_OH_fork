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

#include "hdi_venc_in_buffer_mgr.h"
#include <hdf_base.h>
#include <algorithm>
#include "media_log.h"
#include "media_errors.h"
#include "hdi_codec_util.h"
#include "buffer_type_meta.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "HdiVencInBufferMgr"};
}

namespace OHOS {
namespace Media {
HdiVencInBufferMgr::HdiVencInBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

HdiVencInBufferMgr::~HdiVencInBufferMgr()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t HdiVencInBufferMgr::UseHandleMems(std::vector<GstBuffer *> &buffers)
{
    auto ret = HdiGetParameter(handle_, OMX_IndexParamPortDefinition, mPortDef_);
    CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "HdiGetParameter failed");
    CHECK_AND_RETURN_RET_LOG(buffers.size() == mPortDef_.nBufferCountActual, GST_CODEC_ERROR, "BufferNum error");
    for (int i = 0; i < buffers.size(); ++i) {
        std::shared_ptr<OmxCodecBuffer> codecBuffer = std::make_shared<OmxCodecBuffer>();
        codecBuffer->size = sizeof(OmxCodecBuffer);
        codecBuffer->version.s.nVersionMajor = 1;
        codecBuffer->bufferType = CodecBufferType::CODEC_BUFFER_TYPE_DYNAMIC_HANDLE;
        auto ret = handle_->UseBuffer(handle_, (uint32_t)mPortIndex_, codecBuffer.get());
        CHECK_AND_RETURN_RET_LOG(ret == HDF_SUCCESS, GST_CODEC_ERROR, "UseBuffer failed");
        availableBuffers_.push_back(codecBuffer);
    }
    return GST_CODEC_OK;
}

int32_t HdiVencInBufferMgr::Preprocessing()
{
    int32_t ret = GST_CODEC_OK;
    if (enableNativeBuffer_) {
        ret = UseHandleMems(preBuffers_);
        std::for_each(preBuffers_.begin(), preBuffers_.end(), [&](GstBuffer *buffer){ gst_buffer_unref(buffer); });
    } else {
        ret = UseAshareMems(preBuffers_);
    }
    EmptyList(preBuffers_);
    return ret;
}

int32_t HdiVencInBufferMgr::UseBuffers(std::vector<GstBuffer *> buffers)
{
    CHECK_AND_RETURN_RET_LOG(!buffers.empty(), GST_CODEC_ERROR, "buffer num error");
    if (buffers[0] == nullptr) {
        enableNativeBuffer_ = true;
        preBuffers_ = buffers;
    } else {
        enableNativeBuffer_ = false;
        std::for_each(buffers.begin(), buffers.end(), [&](GstBuffer *buffer){ gst_buffer_ref(buffer); });
        preBuffers_ = buffers;
    }
    return GST_CODEC_OK;
}

void HdiVencInBufferMgr::UpdateCodecMeta(GstBufferTypeMeta *bufferType, std::shared_ptr<OmxCodecBuffer> &codecBuffer)
{
    CHECK_AND_RETURN_LOG(codecBuffer != nullptr, "bufferType is nullptr");
    CHECK_AND_RETURN_LOG(bufferType != nullptr, "bufferType is nullptr");
    if (enableNativeBuffer_) {
        codecBuffer->fenceFd = bufferType->fenceFd;
        codecBuffer->buffer = reinterpret_cast<uint8_t *>(bufferType->buf);
    } else {
        HdiBufferMgr::UpdateCodecMeta(bufferType, codecBuffer);
    }
}
}  // namespace Media
}  // namespace OHOS
