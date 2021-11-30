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

#include "src_surface_impl.h"
#include "gst_encoder_surface_src.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SrcSurfaceImpl"};
}

namespace OHOS {
namespace Media {
SrcSurfaceImpl::SrcSurfaceImpl()
{
}

SrcSurfaceImpl::~SrcSurfaceImpl()
{
    if (element_ != nullptr) {
        gst_object_unref(element_);
        element_ = nullptr;
    }
}

int32_t SrcSurfaceImpl::Init()
{
    element_ = GST_ELEMENT_CAST(gst_object_ref(gst_element_factory_make("encodersurfacesrc", "encodersurfacesrc")));
    CHECK_AND_RETURN_RET_LOG(element_ != nullptr, MSERR_UNKNOWN, "Failed to gst_element_factory_make");
    return MSERR_OK;
}

int32_t SrcSurfaceImpl::Configure(std::shared_ptr<ProcessorConfig> config)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    g_object_set(G_OBJECT(element_), "caps", config->caps_, nullptr);
    return MSERR_OK;
}

int32_t SrcSurfaceImpl::Flush()
{
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
    MEDIA_LOGE("Unsupport");
    return MSERR_OK;
}

sptr<Surface> SrcSurfaceImpl::CreateInputSurface()
{
    GValue val = G_VALUE_INIT;
    g_object_get_property((GObject *)element_, "surface", &val);
    gpointer surfaceObj = g_value_get_pointer(&val);
    CHECK_AND_RETURN_RET_LOG(surfaceObj != nullptr, nullptr, "Failed to get surface");
    sptr<Surface> surface = (Surface *)surfaceObj;
    return surface;
}

int32_t SrcSurfaceImpl::SetParameter(const Format &format)
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGE("Unsupport");
    return MSERR_OK;
}

int32_t SrcSurfaceImpl::SetCallback(const std::weak_ptr<IAVCodecEngineObs> &obs)
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("SetCallback success");
    return MSERR_OK;
}
} // Media
} // OHOS
