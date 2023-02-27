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

#ifndef GST_APPSRC_ENGINE_H
#define GST_APPSRC_ENGINE_H

#include <vector>
#include <gst/gst.h>
#include "task_queue.h"
#include "media_data_source.h"
#include "gst/app/gstappsrc.h"
#include "gst_shmemory_wrap_allocator.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
struct AppsrcMemory {
    std::shared_ptr<AVSharedMemory> mem;
    // The position of mem in file
    uint64_t filePos;
    // The start position that can be filled
    uint32_t begin;
    // The end position that can be filled
    uint32_t end;
    // The start position that can be push to appsrc
    uint32_t availableBegin;
    uint64_t pushOffset;
};

class GstAppsrcEngine : public std::enable_shared_from_this<GstAppsrcEngine>, public NoCopyable {
public:
    using AppsrcErrorNotifier = std::function<void(int32_t, std::string)>;
    static std::shared_ptr<GstAppsrcEngine> Create(const std::shared_ptr<IMediaDataSource> &dataSrc);
    GstAppsrcEngine(const std::shared_ptr<IMediaDataSource> &dataSrc, const int64_t size);
    ~GstAppsrcEngine();
    int32_t SetAppsrc(GstElement *appSrc);
    void SetParserParam(GstElement &elem);
    int32_t SetErrorCallback(AppsrcErrorNotifier notifier);
    bool IsLiveMode() const;
    void SetVideoMode();
    int32_t Init();
    int32_t Prepare();
    void Stop();
    void SetPushBufferMode();
private:
    void SetCallBackForAppSrc();
    void ClearAppsrc();
    void ResetMemParam();
    void ResetConfig();
    static void NeedData(const GstElement *appSrc, uint32_t size, gpointer self);
    static gboolean SeekData(const GstElement *appSrc, uint64_t seekPos, gpointer self);
    void NeedDataInner(uint32_t size);
    gboolean SeekDataInner(uint64_t pos);
    int32_t PullBuffer();
    int32_t Pusheos();
    int32_t PushBuffer(uint32_t pushSize);
    int32_t PushBufferWithCopy(uint32_t pushSize);
    void PullTask();
    void PushTask();
    void OnError(int32_t errorCode, std::string message);
    uint32_t GetFreeSize();
    uint32_t GetAvailableSize();
    void PointerMemoryAvailable(uint32_t offset, uint32_t length);
    bool IsConnectTimeout();
    std::shared_ptr<IMediaDataSource> dataSrc_ = nullptr;
    const int64_t size_ = 0;
    std::mutex mutex_;
    std::condition_variable pullCond_;
    std::condition_variable pushCond_;
    GstElement *appSrc_ = nullptr;
    TaskQueue pullTaskQue_;
    TaskQueue pushTaskQue_;
    GstAppStreamType streamType_ = GST_APP_STREAM_TYPE_STREAM;
    AppsrcErrorNotifier notifier_;
    std::vector<gulong> callbackIds_;
    bool atEos_ = false;
    bool needData_ = false;
    uint32_t needDataSize_ = 0;
    bool isExit_ = false;
    uint32_t bufferSize_ = 0;
    std::shared_ptr<AppsrcMemory> appSrcMem_;
    GstShMemoryWrapAllocator *allocator_ = nullptr;
    bool noFreeBuffer_ = false;
    bool noAvailableBuffer_ = true;
    int64_t timer_ = 0;
    bool copyMode_ = false;
    bool isFirstBuffer_ = true;
    bool videoMode_ = false;
};
} // namespace Media
} // namespace OHOS
#endif /* GST_APPSRC_ENGINE_H */
