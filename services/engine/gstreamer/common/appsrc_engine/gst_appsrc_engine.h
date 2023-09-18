/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
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
#include <vector>
#include "task_queue.h"
#include "media_data_source.h"
#include "appsrc_memory.h"
#include "inner_msg_define.h"
#include "gst/app/gstappsrc.h"
#include "gst_shmemory_wrap_allocator.h"

namespace OHOS {
namespace Media {
class GstAppsrcEngine : public std::enable_shared_from_this<GstAppsrcEngine>, public NoCopyable {
public:
    using AppsrcErrorNotifier = std::function<bool(const InnerMessage &msg)>;
    static std::shared_ptr<GstAppsrcEngine> Create(const std::shared_ptr<IMediaDataSource> &dataSrc);
    GstAppsrcEngine(const std::shared_ptr<IMediaDataSource> &dataSrc, const int64_t size);
    ~GstAppsrcEngine();
    int32_t Init();
    int32_t Prepare();
    void Stop();
    int32_t SetAppsrc(GstElement *appSrc);
    int32_t SetCallback(AppsrcErrorNotifier notifier);
    bool IsLiveMode() const;
    void SetVideoMode();
    void SetPushBufferMode();
    void DecoderSwitch();
    void RecoverParamFromDecSwitch();
private:
    void SetCallBackForAppSrc();
    void ClearAppsrc();
    void ResetConfig();
    static void NeedData(const GstElement *appSrc, uint32_t size, gpointer self);
    void NeedDataInner(uint32_t size);
    static gboolean SeekData(const GstElement *appSrc, uint64_t seekPos, gpointer self);
    gboolean SeekDataInner(uint64_t pos);
    void PullTask();
    int32_t PullBuffer();
    void PushTask();
    int32_t PushEos();
    int32_t PushBuffer(uint32_t pushSize);
    int32_t PushBufferWithCopy(uint32_t pushSize);
    int32_t AddSrcMem(uint32_t bufferSize);
    void OnError(int32_t errorCode, const std::string &message);
    bool OnBufferReport(int32_t percent);
    bool ReportMessage(const InnerMessage &msg);
    void FreePointerMemory(uint32_t offset, uint32_t length, uint32_t subscript);
    bool IsConnectTimeout();
    std::shared_ptr<IMediaDataSource> dataSrc_ = nullptr;
    const int64_t size_ = 0;
    std::mutex mutex_;
    std::mutex freeMutex_;
    std::mutex pullMutex_;
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
    std::vector<std::shared_ptr<AppsrcMemory>> appSrcMemVec_;
    std::shared_ptr<AppsrcMemory> appSrcMem_;
    GstShMemoryWrapAllocator *allocator_ = nullptr;
    int64_t timer_ = 0;
    bool copyMode_ = false;
    bool videoMode_ = false;
    uint32_t curSubscript_ = 0;
    bool decoderSwitch_ = false;
    bool playState_ = true;
};
} // namespace Media
} // namespace OHOS
#endif /* GST_APPSRC_ENGINE_H */
