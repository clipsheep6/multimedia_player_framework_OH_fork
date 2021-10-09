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

#include "vdec_engine_ff_impl.h"
#include "display_type.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VdecEngineFFImpl"};
}

namespace OHOS {
namespace Media {
VdecEngineFFImpl::VdecEngineFFImpl()
{
    MEDIA_LOGD("enter ctor");
}

VdecEngineFFImpl::~VdecEngineFFImpl()
{
    (void)Stop();
    vdecCtrl_ = nullptr;
}

int32_t VdecEngineFFImpl::Init()
{
    vdecCtrl_ = std::make_shared<VdecController>();
    CHECK_AND_RETURN_RET_LOG(vdecCtrl_ != nullptr, MSERR_NO_MEMORY, "No memory");
    return MSERR_OK;
}

int32_t VdecEngineFFImpl::Configure(const std::vector<Format> &format, sptr<Surface> surface)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, MSERR_INVALID_VAL, "Surface is nullptr");

    CHECK_AND_RETURN_RET(vdecCtrl_->Init() == MSERR_OK, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(vdecCtrl_->Configure(surface) == MSERR_OK, MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t VdecEngineFFImpl::Start()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return vdecCtrl_->Start();
}

int32_t VdecEngineFFImpl::Stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return MSERR_OK;
}

int32_t VdecEngineFFImpl::Flush()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return MSERR_OK;
}

int32_t VdecEngineFFImpl::Reset()
{
    std::unique_lock<std::mutex> lock(mutex_);
    vdecCtrl_ = nullptr;
    return MSERR_OK;
}

std::shared_ptr<AVSharedMemory> VdecEngineFFImpl::GetInputBuffer(uint32_t index)
{
    std::unique_lock<std::mutex> lock(mutex_);
    return vdecCtrl_->GetInputBuffer(index);
}

int32_t VdecEngineFFImpl::PushInputBuffer(uint32_t index, uint32_t offset, uint32_t size,
    int64_t presentationTimeUs, VideoDecoderInputBufferFlag flags)
{
    std::unique_lock<std::mutex> lock(mutex_);
    return vdecCtrl_->PushInputBuffer(index, offset, size, presentationTimeUs, flags);
}

int32_t VdecEngineFFImpl::ReleaseOutputBuffer(uint32_t index, bool render)
{
    std::unique_lock<std::mutex> lock(mutex_);
    return vdecCtrl_->ReleaseOutputBuffer(index, render);
}

int32_t VdecEngineFFImpl::SetObs(const std::weak_ptr<IVideoDecoderEngineObs> &obs)
{
    std::unique_lock<std::mutex> lock(mutex_);
    vdecCtrl_->SetObs(obs);
    return MSERR_OK;
}
} // nmamespace Media
} // namespace OHOS
