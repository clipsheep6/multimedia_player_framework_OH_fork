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
sptr<Surface> SrcSurfaceImpl::CreateInputSurface()
{
    MEDIA_LOGD("CreateInputSurface");
    return nullptr;
}

int32_t SrcSurfaceImpl::SetParameter(const Format &format)
{
    return MSERR_OK;
}

int32_t SrcSurfaceImpl::SetCallback(const std::shared_ptr<SrcCallback> &callback)
{
    return MSERR_OK;
}

int32_t SrcSurfaceImpl::SetCaps(GstCaps *caps)
{
    return MSERR_OK;
}
} // Media
} // OHOS
