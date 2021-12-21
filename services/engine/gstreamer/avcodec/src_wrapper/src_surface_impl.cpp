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
#include "common_utils.h"
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
    g_object_set(element_, "stream-type", VideoStreamType::VIDEO_STREAM_TYPE_YUV_420, nullptr);
    return MSERR_OK;
}

sptr<Surface> SrcSurfaceImpl::CreateInputSurface(std::shared_ptr<ProcessorConfig> inputConfig)
{
    CHECK_AND_RETURN_RET(inputConfig != nullptr, nullptr);
    CHECK_AND_RETURN_RET(inputConfig->caps_ != nullptr, nullptr);

    GstStructure *structure = gst_caps_get_structure(inputConfig->caps_, 0);;
    CHECK_AND_RETURN_RET(structure != nullptr, nullptr);

    gint width = 0;
    CHECK_AND_RETURN_RET(gst_structure_get(structure, "width", G_TYPE_INT, &width, nullptr) == TRUE, nullptr);
    CHECK_AND_RETURN_RET(width > 0, nullptr);

    gint height = 0;
    CHECK_AND_RETURN_RET(gst_structure_get(structure, "height", G_TYPE_INT, &height, nullptr) == TRUE, nullptr);
    CHECK_AND_RETURN_RET(height > 0, nullptr);

    g_object_set(element_, "surface-width", width, nullptr);
    g_object_set(element_, "surface-height", height, nullptr);

    GValue val = G_VALUE_INIT;
    g_object_get_property((GObject *)element_, "surface", &val);
    gpointer surfaceObj = g_value_get_pointer(&val);
    CHECK_AND_RETURN_RET_LOG(surfaceObj != nullptr, nullptr, "Failed to get surface");
    sptr<Surface> surface = (Surface *)surfaceObj;
    return surface;
}

int32_t SrcSurfaceImpl::SetParameter(const Format &format)
{
    int32_t value = 0;
    if (format.GetIntValue("repeat_frame_after", value) == true) {
        g_object_set(element_, "repeat-frame-after", value, nullptr);
    }

    if (format.GetIntValue("suspend_input_surface", value) == true) {
        g_object_set(element_, "suspend-input-surface", value, nullptr);
    }

    return MSERR_OK;
}
} // Media
} // OHOS
