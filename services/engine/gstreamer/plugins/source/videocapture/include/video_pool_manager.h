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

#ifndef VIDEO_POOL_MANAGER_H
#define VIDEO_POOL_MANAGER_H

#include <mutex>
#include <queue>
#include <gst/gst.h>
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class VideoPoolManager : public OHOS::NoCopyable {
public:
    VideoPoolManager() = default;
    explicit VideoPoolManager(uint32_t size) : maxQueSize_(size) {}
    ~VideoPoolManager();

    bool IsBufferQueFull();
    bool IsBufferQueEmpty();
    uint32_t GetBufferQueSize();
    int32_t PushBuffer(GstBuffer *buf);
    GstBuffer *PopBuffer();
    void FreeBufferQue();

private:
    std::queue<GstBuffer *> bufferQue_;
    uint32_t maxQueSize_ = 0;
    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS

#endif // VIDEO_POOL_MANAGER_H
