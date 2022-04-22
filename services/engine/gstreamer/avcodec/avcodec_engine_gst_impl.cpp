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
    std::unique_lock<std::mutex> lock(mutex_);
    if (ctrl_ != nullptr) {
        (void)ctrl_->Release();
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVCodecEngineGstImpl::Init(AVCodecType type, bool isMimeType, const std::string &name)
{
    MEDIA_LOGD("Init AVCodecGstEngine: type:%{public}d, %{public}d, name:%{public}s", type, isMimeType, name.c_str());
    std::unique_lock<std::mutex> lock(mutex_);
    type_ = type;

    processor_ = AVCodecEngineFactory::CreateProcessor(type);
    CHECK_AND_RETURN_RET(processor_ != nullptr, MSERR_NO_MEMORY);
    CHECK_AND_RETURN_RET(processor_->Init(type, isMimeType, name) == MSERR_OK, MSERR_UNKNOWN);
    CHECK_AND_RETURN_RET(processor_->GetParameter(useSoftWare_, pluginName_) == MSERR_OK, MSERR_UNKNOWN);

    ctrl_ = std::make_unique<AVCodecEngineCtrl>();
    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_NO_MEMORY);
    ctrl_->SetObs(obs_);
    return ctrl_->Init(type, useSoftWare_, pluginName_);
}

int32_t AVCodecEngineGstImpl::Configure(const Format &format)
{
    MEDIA_LOGD("Enter Configure");
    std::unique_lock<std::mutex> lock(mutex_);
    format_ = format;

    CHECK_AND_RETURN_RET(processor_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(processor_->ProcessParameter(format_) == MSERR_OK, MSERR_UNKNOWN);
    MEDIA_LOGD("Configure success");

    return MSERR_OK;
}

int32_t AVCodecEngineGstImpl::Prepare()
{
    MEDIA_LOGD("Enter Prepare");
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(processor_ != nullptr, MSERR_UNKNOWN);
    auto inputConfig = processor_->GetInputPortConfig();
    CHECK_AND_RETURN_RET(inputConfig != nullptr, MSERR_NO_MEMORY);

    auto outputConfig = processor_->GetOutputPortConfig();
    CHECK_AND_RETURN_RET(outputConfig != nullptr, MSERR_NO_MEMORY);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_UNKNOWN);
    return ctrl_->Prepare(inputConfig, outputConfig);
}

int32_t AVCodecEngineGstImpl::Start()
{
    MEDIA_LOGD("Enter Start");
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_UNKNOWN);
    return ctrl_->Start();
}

int32_t AVCodecEngineGstImpl::Stop()
{
    MEDIA_LOGD("Enter Stop");
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_UNKNOWN);
    return ctrl_->Stop();
}

int32_t AVCodecEngineGstImpl::Flush()
{
    MEDIA_LOGD("Enter Flush");
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_UNKNOWN);
    return ctrl_->Flush();
}

int32_t AVCodecEngineGstImpl::Reset()
{
    MEDIA_LOGD("Enter Reset");
    std::unique_lock<std::mutex> lock(mutex_);
    if (ctrl_ != nullptr) {
        (void)ctrl_->Release();
        ctrl_ = nullptr;
    }
    ctrl_ = std::make_unique<AVCodecEngineCtrl>();
    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_NO_MEMORY);
    CHECK_AND_RETURN_RET(ctrl_->Init(type_, useSoftWare_, pluginName_) == MSERR_OK, MSERR_UNKNOWN);
    ctrl_->SetObs(obs_);

    MEDIA_LOGD("Reset success");
    return MSERR_OK;
}

sptr<Surface> AVCodecEngineGstImpl::CreateInputSurface()
{
    MEDIA_LOGD("Enter CreateInputSurface");
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(processor_ != nullptr, nullptr);
    auto inputConfig = processor_->GetInputPortConfig();
    CHECK_AND_RETURN_RET(inputConfig != nullptr, nullptr);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, nullptr);
    return ctrl_->CreateInputSurface(inputConfig);
}

int32_t AVCodecEngineGstImpl::SetOutputSurface(const sptr<Surface> &surface)
{
    MEDIA_LOGD("Enter SetOutputSurface");
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_NO_MEMORY);
    return ctrl_->SetOutputSurface(surface);
}

std::shared_ptr<AVSharedMemory> AVCodecEngineGstImpl::GetInputBuffer(uint32_t index)
{
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, nullptr);
    return ctrl_->GetInputBuffer(index);
}

int32_t AVCodecEngineGstImpl::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_NO_MEMORY);
    return ctrl_->QueueInputBuffer(index, info, flag);
}

std::shared_ptr<AVSharedMemory> AVCodecEngineGstImpl::GetOutputBuffer(uint32_t index)
{
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, nullptr);
    return ctrl_->GetOutputBuffer(index);
}

int32_t AVCodecEngineGstImpl::GetOutputFormat(Format &format)
{
    format_.PutStringValue("plugin_name", pluginName_);
    format = format_;
    return MSERR_OK;
}

std::shared_ptr<AudioCaps> AVCodecEngineGstImpl::GetAudioCaps()
{
    return nullptr;
}

std::shared_ptr<VideoCaps> AVCodecEngineGstImpl::GetVideoCaps()
{
    return nullptr;
}

int32_t AVCodecEngineGstImpl::ReleaseOutputBuffer(uint32_t index, bool render)
{
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_NO_MEMORY);
    return ctrl_->ReleaseOutputBuffer(index, render);
}

int32_t AVCodecEngineGstImpl::SetParameter(const Format &format)
{
    MEDIA_LOGD("Enter SetParameter");
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_NO_MEMORY);
    return ctrl_->SetParameter(format);
}

int32_t AVCodecEngineGstImpl::SetObs(const std::weak_ptr<IAVCodecEngineObs> &obs)
{
    std::unique_lock<std::mutex> lock(mutex_);
    obs_ = obs;
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
