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

#include "transcoder_impl.h"
#include <map>
#include "i_media_service.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "TransCoderImpl"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<TransCoder> TransCoderFactory::CreateTransCoder()
{
    std::shared_ptr<TransCoderImpl> impl = std::make_shared<TransCoderImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "failed to new TransCoderImpl");

    int32_t ret = impl->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to init TransCoderImpl");
    return impl;
}

int32_t TransCoderImpl::Init()
{
    transCoderService_ = MediaServiceFactory::GetInstance().CreateTransCoderService();
    CHECK_AND_RETURN_RET_LOG(transCoderService_ != nullptr, MSERR_NO_MEMORY, "failed to create transcoder service");
    return MSERR_OK;
}

TransCoderImpl::TransCoderImpl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

TransCoderImpl::~TransCoderImpl()
{
    if (transCoderService_ != nullptr) {
        (void)MediaServiceFactory::GetInstance().DestroyTransCoderService(transCoderService_);
        transCoderService_ = nullptr;
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t TransCoderImpl::SetVideoEncoder(VideoCodecFormat encoder)
{
    CHECK_AND_RETURN_RET_LOG(transCoderService_ != nullptr, MSERR_INVALID_OPERATION, "transcoder service does not exist..");
    return transCoderService_->SetVideoEncoder(encoder);
}

int32_t TransCoderImpl::SetVideoSize(int32_t width, int32_t height)
{
    CHECK_AND_RETURN_RET_LOG(transCoderService_ != nullptr, MSERR_INVALID_OPERATION, "transcoder service does not exist..");
    return transCoderService_->SetVideoSize(width, height);
}

int32_t TransCoderImpl::SetVideoEncodingBitRate(int32_t rate)
{
    CHECK_AND_RETURN_RET_LOG(transCoderService_ != nullptr, MSERR_INVALID_OPERATION, "transcoder service does not exist..");
    return transCoderService_->SetVideoEncodingBitRate(rate);
}

int32_t TransCoderImpl::SetAudioEncoder(AudioCodecFormat encoder)
{
    CHECK_AND_RETURN_RET_LOG(transCoderService_ != nullptr, MSERR_INVALID_OPERATION, "transcoder service does not exist..");
    return transCoderService_->SetAudioEncoder(encoder);
}

int32_t TransCoderImpl::SetAudioEncodingBitRate(int32_t bitRate)
{
    CHECK_AND_RETURN_RET_LOG(transCoderService_ != nullptr, MSERR_INVALID_OPERATION, "transcoder service does not exist..");
    return transCoderService_->SetAudioEncodingBitRate(bitRate);
}

int32_t TransCoderImpl::SetOutputFormat(OutputFormatType format)
{
    CHECK_AND_RETURN_RET_LOG(transCoderService_ != nullptr, MSERR_INVALID_OPERATION, "transcoder service does not exist..");
    return transCoderService_->SetOutputFormat(format);
}

int32_t TransCoderImpl::SetInputFile(int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(transCoderService_ != nullptr, MSERR_INVALID_OPERATION, "transcoder service does not exist..");
    return transCoderService_->SetOutputFile(fd);
}

int32_t TransCoderImpl::SetOutputFile(int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(transCoderService_ != nullptr, MSERR_INVALID_OPERATION, "transcoder service does not exist..");
    return transCoderService_->SetOutputFile(fd);
}

int32_t TransCoderImpl::SetTransCoderCallback(const std::shared_ptr<TransCoderCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_INVALID_VAL, "input callback is nullptr.");
    CHECK_AND_RETURN_RET_LOG(transCoderService_ != nullptr, MSERR_INVALID_OPERATION, "transcoder service does not exist..");
    return transCoderService_->SetTransCoderCallback(callback);
}

int32_t TransCoderImpl::Prepare()
{
    CHECK_AND_RETURN_RET_LOG(transCoderService_ != nullptr, MSERR_INVALID_OPERATION, "transcoder service does not exist..");
    return transCoderService_->Prepare();
}

int32_t TransCoderImpl::Start()
{
    CHECK_AND_RETURN_RET_LOG(transCoderService_ != nullptr, MSERR_INVALID_OPERATION, "transcoder service does not exist..");
    return transCoderService_->Start();
}

int32_t TransCoderImpl::Pause()
{
    CHECK_AND_RETURN_RET_LOG(transCoderService_ != nullptr, MSERR_INVALID_OPERATION, "transcoder service does not exist..");
    return transCoderService_->Pause();
}

int32_t TransCoderImpl::Resume()
{
    CHECK_AND_RETURN_RET_LOG(transCoderService_ != nullptr, MSERR_INVALID_OPERATION, "transcoder service does not exist..");
    return transCoderService_->Resume();
}

int32_t TransCoderImpl::Cancel()
{
    CHECK_AND_RETURN_RET_LOG(transCoderService_ != nullptr, MSERR_INVALID_OPERATION, "transcoder service does not exist..");
    return transCoderService_->Cancel();
}

int32_t TransCoderImpl::Release()
{
    CHECK_AND_RETURN_RET_LOG(transCoderService_ != nullptr, MSERR_INVALID_OPERATION, "transcoder service does not exist..");
    (void)transCoderService_->Release();
    (void)MediaServiceFactory::GetInstance().DestroyTransCoderService(transCoderService_);
    transCoderService_ = nullptr;
    return MSERR_OK;
}

} // namespace Media
} // namespace OHOS
