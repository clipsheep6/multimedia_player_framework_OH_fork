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

#include "video_capture_sf_impl.h"
#include <map>
#include "media_log.h"
#include "media_errors.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <securec.h>

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoCaptureSfmpl"};
}

namespace OHOS {
namespace Media {
constexpr int32_t DEFAULT_SURFACE_QUEUE_SIZE = 6;
constexpr int32_t DEFAULT_SURFACE_SIZE = 1024 * 1024;

const std::map<uint32_t, uint32_t> SURFACE_RESOLUTION_MAP = {
    { 1920, 1080 },
    { 1280, 720 },
    { 720, 480 },
};

VideoCaptureSfImpl::VideoCaptureSfImpl()
    : surfaceWidth_(1920),
      surfaceHeight_(1080),
      fence_(-1),
      bufferAvailableCount_(0),
      timestamp_(0),
      damage_ {0},
      surfaceBuffer_(nullptr),
      started_(false),
      paused_(false),
      streamType_(VIDEO_STREAM_TYPE_UNKNOWN),
      streamTypeUnknown_(true),
      dataConSurface_(nullptr),
      producerSurface_(nullptr)
{
}

VideoCaptureSfImpl::~VideoCaptureSfImpl()
{
    Stop();
}

int32_t VideoCaptureSfImpl::Prepare()
{
    auto iter = SURFACE_RESOLUTION_MAP.find(surfaceWidth_);
    CHECK_AND_RETURN_RET_LOG(iter != SURFACE_RESOLUTION_MAP.end(), MSERR_INVALID_VAL, "illegal surface width");
    CHECK_AND_RETURN_RET_LOG(surfaceHeight_ == iter->second, MSERR_INVALID_VAL, "illegal surface height");

    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer();
    CHECK_AND_RETURN_RET_LOG(consumerSurface != nullptr, MSERR_NO_MEMORY, "create surface fail");

    sptr<IBufferConsumerListener> listenerProxy = new (std::nothrow) ConsumerListenerProxy(*this);
    CHECK_AND_RETURN_RET_LOG(listenerProxy != nullptr, MSERR_NO_MEMORY, "create consumer listener fail");
    consumerSurface->RegisterConsumerListener(listenerProxy);

    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    CHECK_AND_RETURN_RET_LOG(producer != nullptr, MSERR_NO_MEMORY, "get producer fail");
    sptr<Surface> producerSurface = Surface::CreateSurfaceAsProducer(producer);
    CHECK_AND_RETURN_RET_LOG(producerSurface != nullptr, MSERR_NO_MEMORY, "get producer fail");

    dataConSurface_ = consumerSurface;
    producerSurface_ = producerSurface;

    dataConSurface_->SetUserData("surface_width", std::to_string(surfaceWidth_));
    dataConSurface_->SetUserData("surface_height", std::to_string(surfaceHeight_));
    dataConSurface_->SetQueueSize(DEFAULT_SURFACE_QUEUE_SIZE);
    dataConSurface_->SetUserData("surface_size", std::to_string(DEFAULT_SURFACE_SIZE));

    return MSERR_OK;
}

int32_t VideoCaptureSfImpl::Start()
{
    std::unique_lock<std::mutex> lock(mutex_);
    started_.store(true);
    startedCondition_.notify_all();
    return MSERR_OK;
}

int32_t VideoCaptureSfImpl::Pause()
{
    return MSERR_OK;
}

int32_t VideoCaptureSfImpl::Resume()
{
    return MSERR_OK;
}

int32_t VideoCaptureSfImpl::Stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    started_.store(false);
    if (bufferAvailableCount_ == 0) {
        bufferAvailableCount_++;
    }
    bufferAvailableCondition_.notify_all();
    if (dataConSurface_ != nullptr) {
        dataConSurface_->UnregisterConsumerListener();
        dataConSurface_ = nullptr;
        producerSurface_ = nullptr;
    }
    return MSERR_OK;
}

int32_t VideoCaptureSfImpl::SetSurfaceWidth(uint32_t width)
{
    surfaceWidth_ = width;
    return MSERR_OK;
}

int32_t VideoCaptureSfImpl::SetSurfaceHeight(uint32_t height)
{
    surfaceHeight_ = height;
    return MSERR_OK;
}

sptr<Surface> VideoCaptureSfImpl::GetSurface()
{
    CHECK_AND_RETURN_RET_LOG(producerSurface_ != nullptr, nullptr, "surface not created");
    return producerSurface_;
}

std::shared_ptr<EsAvcCodecBuffer> VideoCaptureSfImpl::GetCodecBuffer()
{
    if (AcquireSurfaceBuffer() == MSERR_OK) {
        if (streamTypeUnknown_) {
            ProbeStreamType();
        }
        return DoGetCodecBuffer();
    }

    return nullptr;
}

std::shared_ptr<VideoFrameBuffer> VideoCaptureSfImpl::GetFrameBufferInner()
{
    if (streamTypeUnknown_) {
        ProbeStreamType();
    }
    bufferNumber_++;
    return DoGetFrameBuffer();
}

std::shared_ptr<VideoFrameBuffer> VideoCaptureSfImpl::GetFrameBuffer()
{
    if (bufferNumber_  == 0) {
        return GetFrameBufferInner();
    } else {
        if (AcquireSurfaceBuffer() == MSERR_OK) {
            return GetFrameBufferInner();
        }
    }
    return nullptr;
}

uint64_t GetCurrentLocalTimeStamp1()
{
    std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp =
        std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
    return tmp.count();
}

int32_t VideoCaptureSfImpl::SaveYUV(void* buffer, int32_t size)
{
    char path[PATH_MAX] = {0};
    system("mkdir -p /mnt/record");
    (void) sprintf_s(path, sizeof(path) / sizeof(path[0]),
                    "/mnt/record/%s_%lld.mpg", "record", GetCurrentLocalTimeStamp1());
    MEDIA_LOGE("%s, saving file to %{public}s", __FUNCTION__, path);
    int imgFd = open(path, O_RDWR | O_CREAT, 00766);
    if (imgFd == -1) {
        MEDIA_LOGE("%s, open file failed, errno = %{public}s.", __FUNCTION__, strerror(errno));
        return -1;
    }
    int ret = write(imgFd, buffer, size);
    if (ret == -1) {
        MEDIA_LOGE("%s, write file failed, error = %{public}s", __FUNCTION__, strerror(errno));
        close(imgFd);
        return -1;
    }
    close(imgFd);
    return 0;
}

int32_t VideoCaptureSfImpl::AcquireSurfaceBuffer()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (!started_ || (dataConSurface_ == nullptr)) {
        return MSERR_INVALID_OPERATION;
    }
    bufferAvailableCondition_.wait(lock, [this]() { return bufferAvailableCount_ > 0; });
    if (!started_ || (dataConSurface_ == nullptr)) {
        return MSERR_INVALID_OPERATION;
    }
    if (dataConSurface_->AcquireBuffer(surfaceBuffer_, fence_, timestamp_, damage_) != SURFACE_ERROR_OK) {
        return MSERR_UNKNOWN;
    }

    void *addr = surfaceBuffer_->GetVirAddr();
    int32_t size = surfaceBuffer_->GetSize();
    SaveYUV(addr, size);
    MEDIA_LOGE("Calling AcquireSurfaceBuffer::SaveYUV SIZE %{public}d", size);

    surfaceBuffer_->ExtraGet("dataSize", dataSize_);
    surfaceBuffer_->ExtraGet("pts", pts_);
    surfaceBuffer_->ExtraGet("duration", duration_);
    bufferAvailableCount_--;
    return MSERR_OK;
}

void VideoCaptureSfImpl::ConsumerListenerProxy::OnBufferAvailable()
{
    return owner_.OnBufferAvailable();
}

void VideoCaptureSfImpl::OnBufferAvailable()
{
    if (dataConSurface_ == nullptr) {
        return;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    if (bufferAvailableCount_ == 0) {
        bufferAvailableCondition_.notify_all();
    }
    bufferAvailableCount_++;
}

void VideoCaptureSfImpl::ProbeStreamType()
{
    streamTypeUnknown_ = false;
    streamType_ = VIDEO_STREAM_TYPE_ES_AVC;
}
}  // namespace Media
}  // namespace OHOS
