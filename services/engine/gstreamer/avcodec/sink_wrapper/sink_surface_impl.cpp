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

#include "sink_surface_impl.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SinkSurfaceImpl"};
}

namespace OHOS {
namespace Media {
SinkSurfaceImpl::SinkSurfaceImpl()
{
}

SinkSurfaceImpl::~SinkSurfaceImpl()
{
    bufferList_.clear();
    if (element_ != nullptr) {
        gst_object_unref(element_);
        element_ = nullptr;
    }
}

int32_t SinkSurfaceImpl::Init()
{
    element_ = GST_ELEMENT_CAST(gst_object_ref(gst_element_factory_make("appsink", "sink")));
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::Flush()
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

int32_t SinkSurfaceImpl::SetOutputSurface(sptr<Surface> surface)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    g_object_set(G_OBJECT(element_), "surface", static_cast<gpointer>(surface), nullptr);
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::ReleaseOutputBuffer(uint32_t index, bool render)
{
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::SetParameter(const Format &format)
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGE("Unsupport");
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::SetCallback(const std::weak_ptr<IAVCodecEngineObs> &obs)
{
    return MSERR_OK;
}

int32_t SinkSurfaceImpl::Configure(std::shared_ptr<ProcessorConfig> config)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    g_object_set(G_OBJECT(element_), "caps", config->caps_, nullptr);
    return MSERR_OK;
}
} // Media
} // OHOS
