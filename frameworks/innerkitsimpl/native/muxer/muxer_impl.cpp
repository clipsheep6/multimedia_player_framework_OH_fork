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

#include "muxer_impl.h"
#include "i_media_service.h"
#include "media_log.h"
#include "media_errors.h"
#include "avsharedmemorybase.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MuxerImpl"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<Muxer> MuxerFactory::CreateMuxer()
{
    std::shared_ptr<MuxerImpl> impl = std::make_shared<MuxerImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "Failed to create muxer implementation");

    int32_t ret = impl->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "Failed to init muxer implementation");
    return impl;
}

int32_t MuxerImpl::Init()
{
    muxerService_ = MeidaServiceFactory::GetInstance().CreateMuxerService();
    CHECK_AND_RETURN_RET_LOG(muxerService_ != nullptr, MSERR_NO_MEMORY, "Failed to create muxer service");
    return MSERR_OK;
}

MuxerImpl::MuxerImpl()
{
    MEDIA_LOGD("MuxerImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MuxerImpl::~MuxerImpl()
{
    if (muxerService_ != nullptr) {
        (void)MeidaServiceFactory::GetInstance().DestroyMuxerService(muxerService_);
        muxerService_ = nullptr;
    }
    MEDIA_LOGD("MuxerImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

std::vector<std::string> MuxerImpl::GetSupportedFormats()
{
    // CHECK_AND_RETURN_RET_LOG(muxerService_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    return IMuxerService::GetSupportedFormats();
}

int32_t MuxerImpl::SetOutput(const std::string& path, const std::string& format)
{
    CHECK_AND_RETURN_RET_LOG(muxerService_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    CHECK_AND_RETURN_RET_LOG(!path.empty(), MSERR_INVALID_VAL, "Path is empty");
    CHECK_AND_RETURN_RET_LOG(!format.empty(), MSERR_INVALID_VAL, "Format is empty");
    return muxerService_->SetOutput(path, format);
}

int32_t MuxerImpl::SetLocation(float latitude, float longitude)
{
    CHECK_AND_RETURN_RET_LOG(muxerService_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    return muxerService_->SetLocation(latitude, longitude);
}

int32_t MuxerImpl::SetOrientationHint(int degrees)
{
    CHECK_AND_RETURN_RET_LOG(muxerService_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    return muxerService_->SetOrientationHint(degrees);
}

int32_t MuxerImpl::AddTrack(const MediaDescription& trackDesc, int32_t &trackId)
{
    CHECK_AND_RETURN_RET_LOG(muxerService_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    return muxerService_->AddTrack(trackDesc, trackId);
}

int32_t MuxerImpl::Start()
{
    CHECK_AND_RETURN_RET_LOG(muxerService_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    return muxerService_->Start();
}

int32_t MuxerImpl::WriteTrackSample(std::shared_ptr<AVMemory> sampleData, const TrackSampleInfo& info)
{
    CHECK_AND_RETURN_RET_LOG(muxerService_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    // std::shared_ptr<AVSharedMemory> avSharedMem = AVSharedMemoryBase::Create(sampleData->Capacity(), AVSharedMemory::FLAGS_READ_ONLY, "sampleData");
    std::shared_ptr<AVSharedMemory> avSharedMem = std::make_shared<AVSharedMemoryBase>(sampleData->Capacity(), AVSharedMemory::FLAGS_READ_ONLY, "sampleData");
    return muxerService_->WriteTrackSample(avSharedMem, info);
}

int32_t MuxerImpl::Stop()
{
    CHECK_AND_RETURN_RET_LOG(muxerService_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    return muxerService_->Stop();
}

void MuxerImpl::Release()
{
    CHECK_AND_RETURN_LOG(muxerService_ != nullptr, "Muxer Service does not exist");
    (void)muxerService_->Release();
    (void)MeidaServiceFactory::GetInstance().DestroyMuxerService(muxerService_);
    muxerService_ = nullptr;
}
}  // Media
}  // OHOS