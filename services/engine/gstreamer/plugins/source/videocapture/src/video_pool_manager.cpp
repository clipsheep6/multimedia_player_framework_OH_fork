/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "video_pool_manager.h"
#include "media_dfx.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoPoolManager"};
}
namespace OHOS {
namespace Media {
VideoPoolManager::~VideoPoolManager()
{
    FreeBufferQue();
}

bool VideoPoolManager::IsBufferQueFull()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (bufferQue_.size() >= maxQueSize_) {
        return true;
    }

    return false;
}

bool VideoPoolManager::IsBufferQueEmpty()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return bufferQue_.empty();
}

uint32_t VideoPoolManager::GetBufferQueSize()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return bufferQue_.size();
}

int32_t VideoPoolManager::PushBuffer(GstBuffer *buf)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(bufferQue_.size() < maxQueSize_, MSERR_NO_MEMORY,
        "The queue length has reached the maximum.");

    bufferQue_.push(buf);
    MEDIA_LOGI("PushBuffer end, size: %{public}zu", bufferQue_.size());
    return MSERR_OK;
}

GstBuffer *VideoPoolManager::PopBuffer()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(!bufferQue_.empty(), nullptr, "The queue is empty.");

    GstBuffer *buf = bufferQue_.front();
    bufferQue_.pop();
    MEDIA_LOGI("PopBuffer end, size: %{public}zu", bufferQue_.size());
    return buf;
}

void VideoPoolManager::FreeBufferQue()
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGI("FreeBufferQue size: %{public}zu", bufferQue_.size());
    while (bufferQue_.size() > 0) {
        GstBuffer *buf = bufferQue_.front();
        bufferQue_.pop();
        if (buf) {
            gst_buffer_unref(buf);
        }
    }
}
} // namespace Media
} // namespace OHOS