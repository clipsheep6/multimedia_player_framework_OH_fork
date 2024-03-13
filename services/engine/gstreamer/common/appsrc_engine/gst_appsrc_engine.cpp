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

#include "gst_appsrc_engine.h"
#include <algorithm>
#include <sys/time.h>
#include <unistd.h>
#include "avdatasrcmemory.h"
#include "media_log.h"
#include "media_errors.h"
#include "media_dfx.h"
#include "securec.h"
#include "scope_guard.h"
#include "param_wrapper.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstAppsrcEngine"};
    constexpr int32_t AUDIO_DEFAULT_BUFFER_SIZE = 2048000;
    constexpr int32_t VIDEO_DEFAULT_BUFFER_SIZE = 8192000;
    constexpr int32_t MAX_BUFFER_SIZE = 40960000;
    constexpr int32_t PULL_SIZE = 204800;
    constexpr int64_t UNKNOW_FILE_SIZE = -1;
    constexpr int32_t TIME_VAL_MS = 1000;
    constexpr int32_t TIME_VAL_US = 1000000;
    constexpr int32_t PULL_BUFFER_TIME_OUT_MS = 100;
    constexpr int32_t PLAY_TIME_OUT_MS = 15000;
    constexpr int32_t PULL_BUFFER_SLEEP_US = 3000;
}

namespace OHOS {
namespace Media {
std::shared_ptr<GstAppsrcEngine> GstAppsrcEngine::Create(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    MEDIA_LOGD("Create in");
    CHECK_AND_RETURN_RET_LOG(dataSrc != nullptr, nullptr, "input dataSrc is empty!");
    int64_t size = 0;
    int32_t ret = dataSrc->GetSize(size);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "media data source get size failed!");
    CHECK_AND_RETURN_RET_LOG(size >= UNKNOW_FILE_SIZE, nullptr,
        "invalid file size, if unknow file size please set size = -1");
    std::shared_ptr<GstAppsrcEngine> wrap = std::make_shared<GstAppsrcEngine>(dataSrc, size);
    CHECK_AND_RETURN_RET_LOG(wrap->Init() == MSERR_OK, nullptr, "init failed");
    MEDIA_LOGD("Create out");
    return wrap;
}

GstAppsrcEngine::GstAppsrcEngine(const std::shared_ptr<IMediaDataSource> &dataSrc, const int64_t size)
    : dataSrc_(dataSrc),
      size_(size),
      pullTaskQue_("pullbufferTask"),
      pushTaskQue_("pushbufferTask")
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create and size %{public}" PRId64 "", FAKE_POINTER(this), size);
    streamType_ = size == UNKNOW_FILE_SIZE ? GST_APP_STREAM_TYPE_STREAM : GST_APP_STREAM_TYPE_RANDOM_ACCESS;
}

GstAppsrcEngine::~GstAppsrcEngine()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    Stop();
    ClearAppsrc();
    gst_object_unref(allocator_);
}

int32_t GstAppsrcEngine::Init()
{
    MEDIA_LOGD("Init in");
    appSrcMemVec_.push_back(std::make_shared<AppsrcMemory>());
    appSrcMem_ = appSrcMemVec_[curSubscript_];
    allocator_ = gst_shmemory_wrap_allocator_new();
    CHECK_AND_RETURN_RET_LOG(allocator_ != nullptr, MSERR_NO_MEMORY, "Failed to create allocator");
    MEDIA_LOGD("Init out");
    return MSERR_OK;
}

int32_t GstAppsrcEngine::Prepare()
{
    MEDIA_LOGD("Prepare in");
    std::unique_lock<std::mutex> lock(mutex_);
    if (appSrcMem_ && appSrcMem_->GetMem() == nullptr) {
        uint32_t bufferSize = videoMode_ ? VIDEO_DEFAULT_BUFFER_SIZE : AUDIO_DEFAULT_BUFFER_SIZE;
        appSrcMem_->SetBufferSize(bufferSize);
        auto mem = AVDataSrcMemory::CreateFromLocal(
            bufferSize, AVSharedMemory::Flags::FLAGS_READ_WRITE, "appsrc");
        CHECK_AND_RETURN_RET_LOG(mem != nullptr, MSERR_NO_MEMORY, "init AVSharedMemory failed");
        appSrcMem_->SetMem(mem);
    }
    if (decoderSwitch_) {
        RecoverParamFromDecSwitch();
    } else {
        ResetConfig();
    }
    SetPushBufferMode();
    CHECK_AND_RETURN_RET_LOG(pullTaskQue_.Start() == MSERR_OK, MSERR_INVALID_OPERATION, "init task failed");
    CHECK_AND_RETURN_RET_LOG(pushTaskQue_.Start() == MSERR_OK, MSERR_INVALID_OPERATION, "init task failed");
    auto task = std::make_shared<TaskHandler<void>>([this] { PullTask(); });
    CHECK_AND_RETURN_RET_LOG(pullTaskQue_.EnqueueTask(task) == MSERR_OK,
        MSERR_INVALID_OPERATION, "enque task failed");
    task = std::make_shared<TaskHandler<void>>([this] { PushTask(); });
    CHECK_AND_RETURN_RET_LOG(pushTaskQue_.EnqueueTask(task) == MSERR_OK,
        MSERR_INVALID_OPERATION, "enque task failed");
    MEDIA_LOGD("Prepare out");
    return MSERR_OK;
}

void GstAppsrcEngine::RecoverParamFromDecSwitch()
{
    appSrcMem_->RestoreMemParam();
    isExit_ = false;
    decoderSwitch_ = false;
}

void GstAppsrcEngine::Stop()
{
    MEDIA_LOGD("Stop in");
    {
        std::unique_lock<std::mutex> lock(mutex_);
        isExit_ = true;
        pullCond_.notify_all();
        pushCond_.notify_all();
    }
    (void)pullTaskQue_.Stop();
    (void)pushTaskQue_.Stop();
    MEDIA_LOGD("Stop out");
}

int32_t GstAppsrcEngine::SetAppsrc(GstElement *appSrc)
{
    MEDIA_LOGD("SetAppsrc in");
    if (appSrc_) {
        gst_object_unref(appSrc_);
        appSrc_ = nullptr;
    }
    appSrc_ = static_cast<GstElement *>(gst_object_ref(appSrc));
    CHECK_AND_RETURN_RET_LOG(appSrc_ != nullptr, MSERR_INVALID_VAL, "set appsrc failed");
    SetCallBackForAppSrc();
    MEDIA_LOGD("SetAppsrc out");
    return MSERR_OK;
}

int32_t GstAppsrcEngine::SetCallback(AppsrcErrorNotifier notifier)
{
    std::unique_lock<std::mutex> lock(mutex_);
    notifier_ = notifier;
    return MSERR_OK;
}

bool GstAppsrcEngine::IsLiveMode() const
{
    return streamType_ == GST_APP_STREAM_TYPE_STREAM;
}

void GstAppsrcEngine::SetVideoMode()
{
    videoMode_ = true;
}

void GstAppsrcEngine::SetPushBufferMode()
{
    std::string copyMode;
    int32_t res = OHOS::system::GetStringParameter("sys.media.datasrc.set.copymode", copyMode, "");
    if (res == 0 && !copyMode.empty()) {
        if (copyMode == "TRUE") {
            copyMode_ = true;
            MEDIA_LOGD("set copymode to true");
        } else if (copyMode == "FALSE") {
            copyMode_ = false;
            MEDIA_LOGD("set copymode to false");
        }
    }
}

void GstAppsrcEngine::DecoderSwitch()
{
    decoderSwitch_ = true;
}

void GstAppsrcEngine::SetCallBackForAppSrc()
{
    MEDIA_LOGD("SetCallBackForAppSrc in");
    CHECK_AND_RETURN_LOG(appSrc_ != nullptr, "appSrc_ is nullptr");
    g_object_set(appSrc_, "stream-type", streamType_, "format", GST_FORMAT_BYTES, "size", size_, nullptr);
    callbackIds_.push_back(g_signal_connect(appSrc_, "need-data", G_CALLBACK(NeedData), this));
    if (streamType_ == GST_APP_STREAM_TYPE_RANDOM_ACCESS) {
        callbackIds_.push_back(g_signal_connect(appSrc_, "seek-data", G_CALLBACK(SeekData), this));
    }
    MEDIA_LOGD("SetCallBackForAppSrc out, and callbackIds size is %{public}u",
        static_cast<uint32_t>(callbackIds_.size()));
}

void GstAppsrcEngine::ClearAppsrc()
{
    MEDIA_LOGD("ClearAppsrc in");
    if (appSrc_ != nullptr) {
        MEDIA_LOGD("callbackIds size is %{public}u", static_cast<uint32_t>(callbackIds_.size()));
        for (auto &id : callbackIds_) {
            g_signal_handler_disconnect(appSrc_, id);
        }
        callbackIds_.clear();
        gst_object_unref(appSrc_);
        appSrc_ = nullptr;
    }
    MEDIA_LOGD("ClearAppsrc out");
}

void GstAppsrcEngine::ResetConfig()
{
    appSrcMem_->ResetMemParam();
    atEos_ = false;
    needData_ = false;
    needDataSize_ = 0;
    isExit_ = false;
    timer_ = 0;
    copyMode_ = false;
}

void GstAppsrcEngine::NeedData(const GstElement *appSrc, uint32_t size, gpointer self)
{
    MEDIA_LOGD("NeedData in");
    (void)appSrc;
    CHECK_AND_RETURN_LOG(self != nullptr, "self is nullptr");
    auto wrap = static_cast<GstAppsrcEngine *>(self);
    wrap->NeedDataInner(size);
    MEDIA_LOGD("NeedData out");
}

void GstAppsrcEngine::NeedDataInner(uint32_t size)
{
    std::unique_lock<std::mutex> pullLock(pullMutex_);
    MEDIA_LOGD("NeedDataInner in, size %{public}u atEos_ %{public}d, isExit_ %{public}d", size, atEos_, isExit_);
    needDataSize_ = size;
    uint32_t availableSize = appSrcMem_->GetAvailableSize();
    if ((needDataSize_ <= availableSize || atEos_) && !isExit_) {
        if (needDataSize_ > availableSize) {
            needDataSize_ = availableSize;
        }
        bool needcopy = appSrcMem_->IsNeedCopy(needDataSize_);
        MEDIA_LOGD("PushBuffer pushSize is %{public}u", needDataSize_);
        int32_t ret;
        if (availableSize == 0 && atEos_) {
            ret = PushEos();
        } else if (copyMode_ || needcopy) {
            ret = PushBufferWithCopy(needDataSize_);
        } else {
            ret = PushBuffer(needDataSize_);
        }
        if (ret != MSERR_OK) {
            OnError(MSERR_EXT_API9_NO_MEMORY, "GstAppsrcEngine:Push buffer failed.");
        }
    } else {
        std::unique_lock<std::mutex> freeLock(freeMutex_);
        uint32_t freeSize = appSrcMem_->GetFreeSize();
        uint32_t bufferSize = appSrcMem_->GetBufferSize();
        if (needDataSize_ > bufferSize && bufferSize < MAX_BUFFER_SIZE / 2) {
            // 2 Increase to twice the required buffer
            if (AddSrcMem(needDataSize_ * 2) != MSERR_OK) {
                OnError(MSERR_EXT_API9_NO_MEMORY, "GstAppsrcEngine:AddSrcMem failed.");
            }
        } else if (availableSize + (freeSize / PULL_SIZE) * PULL_SIZE < needDataSize_ &&
            bufferSize < MAX_BUFFER_SIZE / 2) {
            // 2 Increase to twice the original buffer
            if (AddSrcMem(bufferSize * 2) != MSERR_OK) {
                OnError(MSERR_EXT_API9_NO_MEMORY, "GstAppsrcEngine:AddSrcMem failed.");
            }
        }
        
        needData_ = true;
        MEDIA_LOGD("needData_ set to true");
    }
    MEDIA_LOGD("NeedDataInner out");
}

gboolean GstAppsrcEngine::SeekData(const GstElement *appSrc, uint64_t seekPos, gpointer self)
{
    MEDIA_LOGD("SeekData in, pos: %{public}" PRIu64 "", seekPos);
    (void)appSrc;
    CHECK_AND_RETURN_RET_LOG(self != nullptr, FALSE, "self is nullptr");
    auto wrap = static_cast<GstAppsrcEngine *>(self);
    return wrap->SeekDataInner(seekPos);
}

gboolean GstAppsrcEngine::SeekDataInner(uint64_t pos)
{
    MEDIA_LOGD("SeekDataInner in");
    if (pos == appSrcMem_->GetPushOffset()) {
        MEDIA_LOGD("Seek to current position");
        return TRUE;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    std::unique_lock<std::mutex> freeLock(freeMutex_);
    appSrcMem_->PrintCurPos();
    appSrcMem_->SeekAndChangePos(pos);
    atEos_ = false;
    needData_ = false;
    appSrcMem_->PrintCurPos();
    pullCond_.notify_all();
    MEDIA_LOGD("SeekDataInner out");

    return TRUE;
}

void GstAppsrcEngine::PullTask()
{
    int32_t ret = PullBuffer();
    if (ret != MSERR_OK) {
        OnError(MSERR_EXT_API9_NO_MEMORY, "GstAppsrcEngine:Pull buffer failed.");
    }
}

int32_t GstAppsrcEngine::PullBuffer()
{
    int32_t ret = MSERR_OK;
    while (ret == MSERR_OK) {
        int32_t readSize;
        std::unique_lock<std::mutex> lock(mutex_);
        CHECK_AND_RETURN_RET_LOG(appSrcMem_ != nullptr, MSERR_NO_MEMORY, "no mem");
        MEDIA_LOGD("PullBuffer loop in");
        pullCond_.wait(lock, [this] { return (!atEos_ && appSrcMem_->GetFreeSize() >= PULL_SIZE) || isExit_; });
        CHECK_AND_BREAK(!isExit_);
        std::unique_lock<std::mutex> pullLock(pullMutex_);
        appSrcMem_->PrintCurPos();
        auto mem = appSrcMem_->GetMem();
        int32_t pullSize = static_cast<int32_t>(appSrcMem_->GetBufferSize() - appSrcMem_->GetBeginPos());
        pullSize = std::min(pullSize, PULL_SIZE);
        MEDIA_LOGD("ReadAt begin, length is %{public}d", pullSize);
        std::static_pointer_cast<AVDataSrcMemory>(mem)->SetOffset(appSrcMem_->GetBeginPos());
        pullLock.unlock();
        if (size_ == UNKNOW_FILE_SIZE) {
            readSize = dataSrc_->ReadAt(mem, pullSize);
        } else {
            readSize = dataSrc_->ReadAt(mem, pullSize, appSrcMem_->GetFilePos());
        }
        pullLock.lock();
        MEDIA_LOGD("ReadAt end, readSize is %{public}d", readSize);
        CHECK_AND_RETURN_RET_LOG(readSize <= pullSize, MSERR_INVALID_VAL,
            "PullBuffer loop end, readSize > length");

        if (readSize < 0) {
            MEDIA_LOGD("no buffer, receive eos!!!");
            atEos_ = true;
            timer_ = 0;
            pushCond_.notify_all();
        } else if (readSize > 0) {
            appSrcMem_->PullBufferAndChangePos(readSize);
            timer_ = 0;
            if (!playState_ && (appSrcMem_->GetAvailableSize() >= PULL_SIZE ||
                static_cast<int64_t>(appSrcMem_->GetAvailableSize() + appSrcMem_->GetFilePos()) >= size_)
                && OnBufferReport(100)) {  // 100 buffering 100%, begin set to play
                playState_ = true;
            }
            appSrcMem_->PrintCurPos();
            pushCond_.notify_all();
        } else if (IsConnectTimeout()) {
            OnError(MSERR_EXT_API9_TIMEOUT, "GstAppsrcEngine:Pull buffer timeout!!!");
        }
        pullLock.unlock();
        lock.unlock();
        usleep(PULL_BUFFER_SLEEP_US);
    }
    return ret;
}

void GstAppsrcEngine::PushTask()
{
    int32_t ret = MSERR_OK;
    while (ret == MSERR_OK) {
        std::unique_lock<std::mutex> lock(mutex_);
        pushCond_.wait(lock, [this] {
            return ((appSrcMem_->GetAvailableSize() >= needDataSize_ || atEos_) && needData_) || isExit_;
        });
        uint32_t availableSize = appSrcMem_->GetAvailableSize();
        if (isExit_) {
            break;
        }
        if (needData_) {
            // pushSize is min(needDataSize_, availableSize, appSrcMem_->bufferSize - appSrcMem_->availableBegin)
            if (needDataSize_ > availableSize) {
                needDataSize_ = availableSize;
            }
            bool needcopy = appSrcMem_->IsNeedCopy(needDataSize_);
            MEDIA_LOGD("PushBuffer pushSize is %{public}d", needDataSize_);
            if (availableSize == 0 && atEos_) {
                ret = PushEos();
            } else if (copyMode_ || needcopy) {
                ret = PushBufferWithCopy(needDataSize_);
            } else {
                ret = PushBuffer(needDataSize_);
            }
        }
    }
    if (ret != MSERR_OK) {
        OnError(MSERR_EXT_API9_NO_MEMORY, "GstAppsrcEngine:Push buffer failed.");
    }
}

int32_t GstAppsrcEngine::PushEos()
{
    MEDIA_LOGD("push eos");
    int32_t ret = gst_app_src_end_of_stream(GST_APP_SRC_CAST(appSrc_));
    CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Push eos failed!");
    needData_ = false;
    return MSERR_OK;
}

int32_t GstAppsrcEngine::PushBuffer(uint32_t pushSize)
{
    MEDIA_LOGD("PushBuffer in");
    CHECK_AND_RETURN_RET_LOG(appSrcMem_ != nullptr && appSrcMem_->GetMem() != nullptr, MSERR_NO_MEMORY, "no mem");
    appSrcMem_->PrintCurPos();

    auto freeMemory = [this](uint32_t offset, uint32_t length, uint32_t curSubscript_) {
        this->FreePointerMemory(offset, length, curSubscript_);
    };
    GstMemory *mem = gst_shmemory_wrap(GST_ALLOCATOR_CAST(allocator_),
        appSrcMem_->GetMem(), appSrcMem_->GetAvailableBeginPos(), pushSize, curSubscript_, freeMemory);

    CHECK_AND_RETURN_RET_LOG(mem != nullptr, MSERR_NO_MEMORY, "Failed to call gst_shmemory_wrap");
    ON_SCOPE_EXIT(0) { gst_memory_unref(mem); };
    GstBuffer *buffer = gst_buffer_new();
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, MSERR_NO_MEMORY, "Failed to call gst_buffer_new");

    gst_buffer_append_memory(buffer, mem);
    GST_BUFFER_OFFSET(buffer) = appSrcMem_->GetPushOffset();
    MEDIA_LOGD("buffer offset is %{public}" PRId64 "", appSrcMem_->GetPushOffset());
    appSrcMem_->PushBufferAndChangePos(pushSize, false);
    if (needDataSize_ == pushSize) {
        needData_ = false;
    }
    needDataSize_ -= pushSize;
    (void)gst_app_src_push_buffer(GST_APP_SRC_CAST(appSrc_), buffer);

    appSrcMem_->PrintCurPos();
    MEDIA_LOGD("PushBuffer out");
    CANCEL_SCOPE_EXIT_GUARD(0);
    return MSERR_OK;
}

int32_t GstAppsrcEngine::PushBufferWithCopy(uint32_t pushSize)
{
    MEDIA_LOGD("PushBufferWithCopy in");
    std::unique_lock<std::mutex> freeLock(freeMutex_);
    CHECK_AND_RETURN_RET_LOG(appSrcMem_ != nullptr && appSrcMem_->GetMem() != nullptr, MSERR_NO_MEMORY, "no mem");
    appSrcMem_->PrintCurPos();

    GstBuffer *buffer = gst_buffer_new_allocate(nullptr, static_cast<gsize>(pushSize), nullptr);
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, MSERR_NO_MEMORY, "no mem");
    ON_SCOPE_EXIT(0) {  gst_buffer_unref(buffer); };
    GstMapInfo info = GST_MAP_INFO_INIT;
    CHECK_AND_RETURN_RET_LOG(gst_buffer_map(buffer, &info, GST_MAP_WRITE) != FALSE,
        MSERR_NO_MEMORY, "map buffer failed");

    errno_t rc;
    guint8 *data = info.data;
    uint8_t *srcBase = std::static_pointer_cast<AVDataSrcMemory>(appSrcMem_->GetMem())->GetInnerBase();
    uint32_t bufferSize = appSrcMem_->GetBufferSize();
    uint32_t copyBegin = appSrcMem_->GetAvailableBeginPos();
    if (!appSrcMem_->IsNeedCopy(pushSize)) {
        rc = memcpy_s(data, pushSize, srcBase + copyBegin, pushSize);
        CHECK_AND_RETURN_RET_LOG(rc == EOK, MSERR_NO_MEMORY, "get mem is nullptr");
    } else {
        uint32_t dataSize = bufferSize - copyBegin;
        rc = memcpy_s(data, dataSize, srcBase + copyBegin, dataSize);
        CHECK_AND_RETURN_RET_LOG(rc == EOK, MSERR_NO_MEMORY, "get mem is failed");
        rc = memcpy_s(data + dataSize, pushSize - dataSize, srcBase, pushSize - dataSize);
        CHECK_AND_RETURN_RET_LOG(rc == EOK, MSERR_NO_MEMORY, "get mem is failed");
    }
    gst_buffer_unmap(buffer, &info);
    GST_BUFFER_OFFSET(buffer) = appSrcMem_->GetPushOffset();
    MEDIA_LOGD("buffer offset is %{public}" PRId64 "", appSrcMem_->GetPushOffset());
    appSrcMem_->PushBufferAndChangePos(pushSize, true);
    
    if (needDataSize_ == pushSize) {
        needData_ = false;
    }
    needDataSize_ -= pushSize;
    appSrcMem_->SetNoFreeBuffer(false);
    (void)gst_app_src_push_buffer(GST_APP_SRC_CAST(appSrc_), buffer);

    appSrcMem_->PrintCurPos();
    pullCond_.notify_all();
    MEDIA_LOGD("PushBufferWithCopy out");
    CANCEL_SCOPE_EXIT_GUARD(0);
    return MSERR_OK;
}

int32_t GstAppsrcEngine::AddSrcMem(uint32_t bufferSize)
{
    MEDIA_LOGD("AddSrcMem in");
    appSrcMemVec_.push_back(std::make_shared<AppsrcMemory>());
    curSubscript_ += 1;
    MEDIA_LOGD("curSubscript_ change to %{public}u", curSubscript_);
    std::shared_ptr<AppsrcMemory> appSrcMemTemp = appSrcMem_;
    appSrcMem_ = appSrcMemVec_[curSubscript_];
    CHECK_AND_RETURN_RET_LOG(appSrcMem_ != nullptr, MSERR_NO_MEMORY, "appSrcMem_ is nullptr");

    appSrcMem_->SetBufferSize(bufferSize);
    auto mem = AVDataSrcMemory::CreateFromLocal(
        bufferSize, AVSharedMemory::Flags::FLAGS_READ_WRITE, "appsrc");
    CHECK_AND_RETURN_RET_LOG(mem != nullptr, MSERR_NO_MEMORY, "init AVSharedMemory failed");
    appSrcMem_->SetMem(mem);
    appSrcMem_->ResetMemParam();

    bool ret = appSrcMem_->CopyBufferAndChangePos(appSrcMemTemp);
    CHECK_AND_RETURN_RET_LOG(ret == true, MSERR_NO_MEMORY, "copy mem is failed");

    pullCond_.notify_all();
    MEDIA_LOGD("AddSrcMem out");
    return MSERR_OK;
}

void GstAppsrcEngine::OnError(int32_t errorCode, const std::string &message)
{
    isExit_ = true;
    pullCond_.notify_all();
    pushCond_.notify_all();
    InnerMessage innerMsg {};
    innerMsg.type = INNER_MSG_ERROR;
    innerMsg.detail1 = errorCode;
    innerMsg.extend = message;
    (void)ReportMessage(innerMsg);
}

bool GstAppsrcEngine::OnBufferReport(int32_t percent)
{
    InnerMessage innerMsg {};
    innerMsg.type = INNER_MSG_BUFFERING;
    innerMsg.detail1 = percent;
    return ReportMessage(innerMsg);
}

bool GstAppsrcEngine::ReportMessage(const InnerMessage &msg)
{
    if (notifier_ != nullptr) {
        return notifier_(msg);
    }
    return false;
}

void GstAppsrcEngine::FreePointerMemory(uint32_t offset, uint32_t length, uint32_t subscript)
{
    MEDIA_LOGD("FreePointerMemory in, offset is %{public}u, length is %{public}u, subscript is %{public}u",
        offset, length, subscript);
    std::unique_lock<std::mutex> freeLock(freeMutex_);
    CHECK_AND_RETURN_LOG(subscript <= appSrcMemVec_.size(), "Check buffer pool subscript  failed");
    std::shared_ptr<AppsrcMemory> mem = appSrcMemVec_[subscript];
    CHECK_AND_RETURN_LOG(mem != nullptr, "Buffer pool has been free");

    mem->PrintCurPos();
    mem->CheckBufferUsage();
    CHECK_AND_RETURN_LOG(mem->FreeBufferAndChangePos(offset, length, copyMode_),
        "Bufferpool checkout failed.");
    mem->PrintCurPos();
    if (subscript == curSubscript_) {
        pullCond_.notify_all();
    } else if (mem->GetFreeSize() == mem->GetBufferSize()) {
        mem->SetMem(nullptr);
        mem = nullptr;
    }
    MEDIA_LOGD("FreePointerMemory out");
}

static int64_t GetTime()
{
    struct timeval time = {};
    int ret = gettimeofday(&time, nullptr);
    CHECK_AND_RETURN_RET_LOG(ret != -1, -1, "Get current time failed!");
    return static_cast<int64_t>(time.tv_sec) * TIME_VAL_MS +
        static_cast<int64_t>(time.tv_usec) * TIME_VAL_MS / TIME_VAL_US;
}

bool GstAppsrcEngine::IsConnectTimeout()
{
    if (!needData_) {
        return false;
    }
    if (timer_ == 0) {
        timer_ = GetTime();
        MEDIA_LOGI("Waiting to receive data");
    } else {
        int64_t curTime = GetTime();
        if (curTime - timer_ > PULL_BUFFER_TIME_OUT_MS && playState_ && OnBufferReport(0)) {
            playState_ = false;
        } else if (curTime - timer_ > PLAY_TIME_OUT_MS) {
            MEDIA_LOGE("No data was received for 15 seconds");
            return true;
        }
    }
    return false;
}
} // namespace Media
} // namespace OHOS
