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

#include "avmuxer_impl.h"
#include "i_media_service.h"
#include "media_log.h"
#include "media_errors.h"
#include "avsharedmemorybase.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMuxerImpl"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<AVMuxer> AVMuxerFactory::CreateAVMuxer()
{
    std::shared_ptr<AVMuxerImpl> impl = std::make_shared<AVMuxerImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "Failed to create avmuxer implementation");

    int32_t ret = impl->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "Failed to init avmuxer implementation");
    return impl;
}

int32_t AVMuxerImpl::Init()
{
    avmuxerService_ = MeidaServiceFactory::GetInstance().CreateAVMuxerService();
    CHECK_AND_RETURN_RET_LOG(avmuxerService_ != nullptr, MSERR_NO_MEMORY, "Failed to create avmuxer service");
    return MSERR_OK;
}

AVMuxerImpl::AVMuxerImpl()
{
    MEDIA_LOGD("AVMuxerImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVMuxerImpl::~AVMuxerImpl()
{
    if (avmuxerService_ != nullptr) {
        (void)MeidaServiceFactory::GetInstance().DestroyAVMuxerService(avmuxerService_);
        avmuxerService_ = nullptr;
    }
    MEDIA_LOGD("AVMuxerImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

std::vector<std::string> AVMuxerImpl::GetSupportedFormats()
{
    // CHECK_AND_RETURN_RET_LOG(avmuxerService_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    return IAVMuxerService::GetSupportedFormats();
}

int32_t AVMuxerImpl::SetOutput(const std::string& path, const std::string& format)
{
    CHECK_AND_RETURN_RET_LOG(avmuxerService_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    CHECK_AND_RETURN_RET_LOG(!path.empty(), MSERR_INVALID_VAL, "Path is empty");
    CHECK_AND_RETURN_RET_LOG(!format.empty(), MSERR_INVALID_VAL, "Format is empty");
    return avmuxerService_->SetOutput(path, format);
}

int32_t AVMuxerImpl::SetLocation(float latitude, float longitude)
{
    CHECK_AND_RETURN_RET_LOG(avmuxerService_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    return avmuxerService_->SetLocation(latitude, longitude);
}

int32_t AVMuxerImpl::SetOrientationHint(int degrees)
{
    CHECK_AND_RETURN_RET_LOG(avmuxerService_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    return avmuxerService_->SetOrientationHint(degrees);
}

int32_t AVMuxerImpl::AddTrack(const MediaDescription& trackDesc, int32_t &trackId)
{
    CHECK_AND_RETURN_RET_LOG(avmuxerService_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    return avmuxerService_->AddTrack(trackDesc, trackId);
}

int32_t AVMuxerImpl::Start()
{
    CHECK_AND_RETURN_RET_LOG(avmuxerService_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    return avmuxerService_->Start();
}

int32_t AVMuxerImpl::WriteTrackSample(std::shared_ptr<AVMemory> sampleData, const TrackSampleInfo& info)
{
    CHECK_AND_RETURN_RET_LOG(avmuxerService_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    MEDIA_LOGD("sampleData->Capacity() is: %{public}u", sampleData->Capacity());
    // std::shared_ptr<AVSharedMemory> avSharedMem = AVSharedMemoryBase::Create(sampleData->Capacity(), AVSharedMemory::FLAGS_READ_ONLY, "sampleData");
    std::shared_ptr<AVSharedMemoryBase> avSharedMem = std::make_shared<AVSharedMemoryBase>(sampleData->Capacity(), AVSharedMemory::FLAGS_READ_ONLY, "sampleData");
    avSharedMem->Init();
    return avmuxerService_->WriteTrackSample(avSharedMem, info);
}

int32_t AVMuxerImpl::Stop()
{
    CHECK_AND_RETURN_RET_LOG(avmuxerService_ != nullptr, MSERR_NO_MEMORY, "AVMuxer Service does not exist");
    return avmuxerService_->Stop();
}

void AVMuxerImpl::Release()
{
    CHECK_AND_RETURN_LOG(avmuxerService_ != nullptr, "AVMuxer Service does not exist");
    (void)avmuxerService_->Release();
    (void)MeidaServiceFactory::GetInstance().DestroyAVMuxerService(avmuxerService_);
    avmuxerService_ = nullptr;
}
}  // Media
}  // OHOS