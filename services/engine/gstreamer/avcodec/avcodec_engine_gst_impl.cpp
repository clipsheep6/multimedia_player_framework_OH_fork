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

#include "avcodec_engine_gst_impl.h"
#include "media_errors.h"
#include "media_log.h"
#include "format_processor/processor_adec_impl.h"
#include "format_processor/processor_aenc_impl.h"
#include "format_processor/processor_vdec_impl.h"
#include "format_processor/processor_venc_impl.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecEngineGstImpl"};
}

namespace OHOS {
namespace Media {
AVCodecEngineGstImpl::AVCodecEngineGstImpl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecEngineGstImpl::~AVCodecEngineGstImpl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVCodecEngineGstImpl::Init(AVCodecType type, bool useSoftware)
{
    switch (type) {
        case AVCODEC_TYPE_VIDEO_ENCODER:
            processor_ = std::make_unique<ProcessorVencImpl>();
            break;
        case AVCODEC_TYPE_VIDEO_DECODER:
            processor_ = std::make_unique<ProcessorVdecImpl>();
            break;
        case AVCODEC_TYPE_AUDIO_ENCODER:
            processor_ = std::make_unique<ProcessorAencImpl>();
            break;
        case AVCODEC_TYPE_AUDIO_DECODER:
            processor_ = std::make_unique<ProcessorAdecImpl>();
            break;
        default:
            MEDIA_LOGE("Unknown codec type");
            break;
    }
    CHECK_AND_RETURN_RET(processor_ != nullptr, MSERR_NO_MEMORY);

    ctrl_ = std::make_unique<AVCodecEngineCtrl>();
    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_NO_MEMORY);
    int32_t ret = ctrl_->Init(type, useSoftware);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t AVCodecEngineGstImpl::Configure(const Format &format)
{
    CHECK_AND_RETURN_RET(processor_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(processor_->Init(format) == MSERR_OK, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(processor_->DoProcess() == MSERR_OK, MSERR_UNKNOWN);
    return MSERR_OK;
}

int32_t AVCodecEngineGstImpl::Prepare()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return ctrl_->Prepare();
}

int32_t AVCodecEngineGstImpl::Start()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return ctrl_->Start();
}

int32_t AVCodecEngineGstImpl::Stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return ctrl_->Stop();
}

int32_t AVCodecEngineGstImpl::Flush()
{
    return MSERR_OK;
}

int32_t AVCodecEngineGstImpl::Reset()
{
    return MSERR_OK;
}

int32_t AVCodecEngineGstImpl::Release()
{
    return MSERR_OK;
}

sptr<Surface> AVCodecEngineGstImpl::CreateInputSurface()
{
    return nullptr;
}

int32_t AVCodecEngineGstImpl::SetOutputSurface(sptr<Surface> surface)
{
    return MSERR_OK;
}

std::shared_ptr<AVSharedMemory> AVCodecEngineGstImpl::GetInputBuffer(uint32_t index)
{
    return nullptr;
}

int32_t AVCodecEngineGstImpl::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info,
                                               AVCodecBufferType type, AVCodecBufferFlag flag)
{
    return MSERR_OK;
}

std::shared_ptr<AVSharedMemory> AVCodecEngineGstImpl::GetOutputBuffer(uint32_t index)
{
    return nullptr;
}

int32_t AVCodecEngineGstImpl::ReleaseOutputBuffer(uint32_t index, bool render)
{
    return MSERR_OK;
}

int32_t AVCodecEngineGstImpl::SetParameter(const Format &format)
{
    return MSERR_OK;
}

int32_t AVCodecEngineGstImpl::SetObs(const std::weak_ptr<IAVCodecEngineObs> &obs)
{
    return MSERR_OK;
}
} // Media
} // OHOS
