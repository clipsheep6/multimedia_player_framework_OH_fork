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

#include "videodecoder_server.h"
#include "media_log.h"
#include "media_errors.h"
#include "engine_factory_repo.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoDecoderServer"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<IVideoDecoderService> VideoDecoderServer::Create()
{
    std::shared_ptr<VideoDecoderServer> server = std::make_shared<VideoDecoderServer>();
    int32_t ret = server->Init();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Failed to init VideoDecoderServer");
        return nullptr;
    }
    return server;
}

VideoDecoderServer::VideoDecoderServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

VideoDecoderServer::~VideoDecoderServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t VideoDecoderServer::Init()
{
    auto engineFactory = EngineFactoryRepo::Instance().GetEngineFactory(IEngineFactory::Scene::SCENE_VIDEODECODER);
    CHECK_AND_RETURN_RET_LOG(engineFactory != nullptr, MSERR_CREATE_REC_ENGINE_FAILED, "Failed to get factory");
    videoDecoderEngine_ = engineFactory->CreateVideoDecoderEngine();
    CHECK_AND_RETURN_RET_LOG(videoDecoderEngine_ != nullptr, MSERR_CREATE_VIDEODECODER_ENGINE_FAILED,
        "Failed to create video decoder engine");
    CHECK_AND_RETURN_RET_LOG(videoDecoderEngine_->Init() == MSERR_OK, MSERR_UNKNOWN, "Failed to init engine");
    status_ = VDEC_UNINITIALIZED;
    return MSERR_OK;
}

void VideoDecoderServer::OnError(int32_t errorType, int32_t errorCode)
{
    std::lock_guard<std::mutex> lock(cbMutex_);
    if (videoDecoderCb_ != nullptr) {
        videoDecoderCb_->OnError(errorType, errorCode);
    }
}

void VideoDecoderServer::OnOutputFormatChanged(std::vector<Format> format)
{
    std::lock_guard<std::mutex> lock(cbMutex_);
    if (videoDecoderCb_ != nullptr) {
        videoDecoderCb_->OnOutputFormatChanged(format);
    }
}

void VideoDecoderServer::OnOutputBufferAvailable(uint32_t index, VideoDecoderBufferInfo info)
{
    std::lock_guard<std::mutex> lock(cbMutex_);
    if (videoDecoderCb_ != nullptr) {
        videoDecoderCb_->OnOutputBufferAvailable(index, info);
    }
}

void VideoDecoderServer::OnInputBufferAvailable(uint32_t index)
{
    std::lock_guard<std::mutex> lock(cbMutex_);
    if (videoDecoderCb_ != nullptr) {
        videoDecoderCb_->OnInputBufferAvailable(index);
    }
}

int32_t VideoDecoderServer::SetCallback(const std::shared_ptr<VideoDecoderCallback> &callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == VDEC_UNINITIALIZED, MSERR_INVALID_OPERATION, "Invalid state");
    {
        std::lock_guard<std::mutex> cbLock(cbMutex_);
        videoDecoderCb_ = callback;
    }
    CHECK_AND_RETURN_RET_LOG(videoDecoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    std::shared_ptr<IVideoDecoderEngineObs> obs = shared_from_this();
    (void)videoDecoderEngine_->SetObs(obs);
    return MSERR_OK;
}

int32_t VideoDecoderServer::Configure(const std::vector<Format> &format, sptr<Surface> surface)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == VDEC_UNINITIALIZED, MSERR_INVALID_OPERATION, "Invalid state");
    CHECK_AND_RETURN_RET_LOG(videoDecoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = videoDecoderEngine_->Configure(format, surface);
    status_ = (ret == MSERR_OK ? VDEC_CONFIGURED : VDEC_ERROR);
    return ret;
}

int32_t VideoDecoderServer::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == VDEC_CONFIGURED , MSERR_INVALID_OPERATION, "Invalid state");
    CHECK_AND_RETURN_RET_LOG(videoDecoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = videoDecoderEngine_->Start();
    status_ = (ret == MSERR_OK ? VDEC_RUNNING : VDEC_ERROR);
    return ret;
}

int32_t VideoDecoderServer::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == VDEC_RUNNING || status_ == VDEC_EOS, MSERR_INVALID_OPERATION, "Invalid state");
    CHECK_AND_RETURN_RET_LOG(videoDecoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = videoDecoderEngine_->Stop();
    status_ = (ret == MSERR_OK ? VDEC_UNINITIALIZED : VDEC_ERROR);
    return ret;
}

int32_t VideoDecoderServer::Flush()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == VDEC_EOS, MSERR_INVALID_OPERATION, "Invalid state");
    CHECK_AND_RETURN_RET_LOG(videoDecoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = videoDecoderEngine_->Flush();
    status_ = (ret == MSERR_OK ? VDEC_RUNNING : VDEC_ERROR);
    return ret;
}

int32_t VideoDecoderServer::Reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(videoDecoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = videoDecoderEngine_->Reset();
    status_ = (ret == MSERR_OK ? VDEC_UNINITIALIZED : VDEC_ERROR);
    return ret;
}

int32_t VideoDecoderServer::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(videoDecoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    videoDecoderEngine_ = nullptr;
    return MSERR_OK;
}

std::shared_ptr<AVSharedMemory> VideoDecoderServer::GetInputBuffer(uint32_t index)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(videoDecoderEngine_ != nullptr, nullptr, "engine is nullptr");
    return videoDecoderEngine_->GetInputBuffer(index);
}

int32_t VideoDecoderServer::PushInputBuffer(uint32_t index, uint32_t offset, uint32_t size,
                                            int64_t presentationTimeUs, VideoDecoderInputBufferFlag flags)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == VDEC_RUNNING, MSERR_INVALID_OPERATION, "Invalid state");
    CHECK_AND_RETURN_RET_LOG(videoDecoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = videoDecoderEngine_->PushInputBuffer(index, offset, size, presentationTimeUs, flags);
    if (ret != MSERR_OK) {
        status_ = VDEC_ERROR;
    } else if (flags == BUFFER_FLAG_END_OF_STREAM) {
        status_ = VDEC_EOS;
    }
    return ret;
}

int32_t VideoDecoderServer::ReleaseOutputBuffer(uint32_t index, bool render)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == VDEC_RUNNING || status_ == VDEC_EOS, MSERR_INVALID_OPERATION, "Invalid state");
    CHECK_AND_RETURN_RET_LOG(videoDecoderEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = videoDecoderEngine_->ReleaseOutputBuffer(index, render);
    if (ret != MSERR_OK) {
        status_ = VDEC_ERROR;
    }
    return ret;
}
}
}
