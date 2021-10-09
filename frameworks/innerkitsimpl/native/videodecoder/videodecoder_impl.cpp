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

#include "videodecoder_impl.h"
#include "i_media_service.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoDecoderImpl"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<VideoDecoder> VideoDecoderFactory::CreateVideoDecoder(const std::string &name)
{
    std::shared_ptr<VideoDecoderImpl> impl = std::make_shared<VideoDecoderImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "Failed to new VideoDecoderImpl");

    int32_t ret = impl->Init(name);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "Failed to init VideoDecoderImpl");
    return impl;
}

int32_t VideoDecoderImpl::Init(const std::string &name)
{
    videoDecoderService_ = MeidaServiceFactory::GetInstance().CreateVideoDecoderService();
    CHECK_AND_RETURN_RET_LOG(videoDecoderService_ != nullptr, MSERR_NO_MEMORY, "Failed to init video decoder server");
    return MSERR_OK;
}

VideoDecoderImpl::VideoDecoderImpl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

VideoDecoderImpl::~VideoDecoderImpl()
{
    videoDecoderService_ = nullptr;
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t VideoDecoderImpl::Configure(const std::vector<Format> &format, sptr<Surface> surface)
{
    CHECK_AND_RETURN_RET_LOG(videoDecoderService_ != nullptr, MSERR_INVALID_OPERATION, "Service died");
    return videoDecoderService_->Configure(format, surface);
}

int32_t VideoDecoderImpl::Start()
{
    CHECK_AND_RETURN_RET_LOG(videoDecoderService_ != nullptr, MSERR_INVALID_OPERATION, "Service died");
    return videoDecoderService_->Start();
}

int32_t VideoDecoderImpl::Stop()
{
    CHECK_AND_RETURN_RET_LOG(videoDecoderService_ != nullptr, MSERR_INVALID_OPERATION, "Service died");
    return videoDecoderService_->Stop();
}

int32_t VideoDecoderImpl::Flush()
{
    CHECK_AND_RETURN_RET_LOG(videoDecoderService_ != nullptr, MSERR_INVALID_OPERATION, "Service died");
    return videoDecoderService_->Flush();
}

int32_t VideoDecoderImpl::Reset()
{
    CHECK_AND_RETURN_RET_LOG(videoDecoderService_ != nullptr, MSERR_INVALID_OPERATION, "Service died");
    return videoDecoderService_->Reset();
}

int32_t VideoDecoderImpl::Release()
{
    CHECK_AND_RETURN_RET_LOG(videoDecoderService_ != nullptr, MSERR_INVALID_OPERATION, "Service died");
    (void)videoDecoderService_->Release();
    videoDecoderService_ = nullptr;
    return MSERR_OK;
}

std::shared_ptr<AVSharedMemory> VideoDecoderImpl::GetInputBuffer(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(videoDecoderService_ != nullptr, nullptr, "Service died");
    return videoDecoderService_->GetInputBuffer(index);
}

int32_t VideoDecoderImpl::PushInputBuffer(uint32_t index, uint32_t offset, uint32_t size, int64_t presentationTimeUs,
                                          VideoDecoderInputBufferFlag flags)
{
    CHECK_AND_RETURN_RET_LOG(videoDecoderService_ != nullptr, MSERR_INVALID_OPERATION, "Service died");
    return videoDecoderService_->PushInputBuffer(index, offset, size, presentationTimeUs, flags);
}

int32_t VideoDecoderImpl::ReleaseOutputBuffer(uint32_t index, bool render)
{
    CHECK_AND_RETURN_RET_LOG(videoDecoderService_ != nullptr, MSERR_INVALID_OPERATION, "Service died");
    return videoDecoderService_->ReleaseOutputBuffer(index, render);
}

int32_t VideoDecoderImpl::SetCallback(const std::shared_ptr<VideoDecoderCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_INVALID_VAL, "Input callback is nullptr");
    CHECK_AND_RETURN_RET_LOG(videoDecoderService_ != nullptr, MSERR_INVALID_OPERATION, "Service died");
    return videoDecoderService_->SetCallback(callback);
}
} // nmamespace Media
} // namespace OHOS
