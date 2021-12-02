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
    element_ = GST_ELEMENT_CAST(gst_object_ref(gst_element_factory_make("surfacevideosrc", "src")));
    CHECK_AND_RETURN_RET_LOG(element_ != nullptr, MSERR_UNKNOWN, "Failed to make gstsurfacevideosrc");
    return MSERR_OK;
}

int32_t SrcSurfaceImpl::Configure(std::shared_ptr<ProcessorConfig> config)
{
    CHECK_AND_RETURN_RET(element_ != nullptr, MSERR_UNKNOWN);
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
} // Media
} // OHOS
