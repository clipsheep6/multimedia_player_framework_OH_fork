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

#include "gst_appsrc_engine.h"
#include <algorithm>
#include <sys/time.h>
#include "avdatasrcmemory.h"
#include "media_log.h"
#include "media_errors.h"
#include "securec.h"
#include "scope_guard.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstAppsrcEngine"};
    constexpr int32_t DEFAULT_BUFFER_SIZE = 409600;
    constexpr int32_t PULL_SIZE = 81920;
    constexpr int64_t UNKNOW_FILE_SIZE = -1;
    constexpr int32_t TIME_VAL_MS = 1000;
    constexpr int32_t TIME_VAL_US = 1000000;
    constexpr int32_t TIME_OUT_MS = 15000;
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
      pushTaskQue_("pushbufferTask"),
      bufferSize_(DEFAULT_BUFFER_SIZE)
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
    appSrcMem_ = std::make_shared<AppsrcMemory>();
    CHECK_AND_RETURN_RET_LOG(appSrcMem_ != nullptr, MSERR_NO_MEMORY, "init AppsrcMemory failed");
    appSrcMem_->mem = AVDataSrcMemory::CreateFromLocal(
        bufferSize_, AVSharedMemory::Flags::FLAGS_READ_WRITE, "appsrc");
    CHECK_AND_RETURN_RET_LOG(appSrcMem_->mem != nullptr, MSERR_NO_MEMORY, "init AVSharedMemory failed");
    ResetMemParam();
    allocator_ = gst_shmemory_wrap_allocator_new();
    CHECK_AND_RETURN_RET_LOG(allocator_ != nullptr, MSERR_NO_MEMORY, "Failed to create allocator");
    MEDIA_LOGD("Init out");
    return MSERR_OK;
}

int32_t GstAppsrcEngine::Prepare()
{
    MEDIA_LOGD("Prepare in");
    std::unique_lock<std::mutex> lock(mutex_);
    ResetConfig();
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

void GstAppsrcEngine::ResetMemParam()
{
    appSrcMem_->filePos = 0;
    appSrcMem_->begin = 0;
    appSrcMem_->end = bufferSize_ - 1;
    appSrcMem_->availableBegin = 0;
    appSrcMem_->pushOffset = 0;
}

void GstAppsrcEngine::ResetConfig()
{
    ResetMemParam();
    atEos_ = false;
    needData_ = false;
    needDataSize_ = 0;
    isExit_ = false;
    noFreeBuffer_ = false;
    noAvailableBuffer_ = true;
    timer_ = 0;
    copyMode_ = false;
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

int32_t GstAppsrcEngine::SetAppsrc(GstElement *appSrc)
{
    MEDIA_LOGD("SetAppsrc in");
    appSrc_ = static_cast<GstElement *>(gst_object_ref(appSrc));
    CHECK_AND_RETURN_RET_LOG(appSrc_ != nullptr, MSERR_INVALID_VAL, "set appsrc failed");
    SetCallBackForAppSrc();
    MEDIA_LOGD("SetAppsrc out");
    return MSERR_OK;
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

bool GstAppsrcEngine::IsLiveMode() const
{
    return streamType_ == GST_APP_STREAM_TYPE_STREAM;
}

void GstAppsrcEngine::SetPushBufferMode(bool isCopy)
{
    MEDIA_LOGD("Set copyMode_ to %{public}d", isCopy);
    copyMode_ = isCopy;
}

int32_t GstAppsrcEngine::SetErrorCallback(AppsrcErrorNotifier notifier)
{
    std::unique_lock<std::mutex> lock(mutex_);
    notifier_ = notifier;
    return MSERR_OK;
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
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("NeedDataInner in, and size is: %{public}u", size);
    needDataSize_ = size;
    uint32_t availableSize = GetAvailableSize();
    MEDIA_LOGD("atEos_ is %{public}d, isExit_ is %{public}d", atEos_, isExit_);
    if ((needDataSize_ <= availableSize || atEos_) && !isExit_) {
        needData_ = true;
        uint32_t pushSize = needDataSize_ > availableSize ? availableSize : needDataSize_;
        pushSize = pushSize > (bufferSize_ - appSrcMem_->availableBegin) ?
            bufferSize_ - appSrcMem_->availableBegin : pushSize;
        int32_t ret;
        if (pushSize == 0) {
            ret = Pusheos();
        } else if (copyMode_) {
            ret = PushBufferWithCopy(pushSize);
        } else {
            ret = PushBuffer(pushSize);
        }
        if (ret != MSERR_OK) {
            OnError(ret);
        }
    } else {
        needData_ = true;
        MEDIA_LOGD("needData_ set to true");
    }
    MEDIA_LOGD("NeedDataInner out");
}

void GstAppsrcEngine::PullTask()
{
    int32_t ret = PullBuffer();
    if (ret != MSERR_OK) {
        OnError(ret);
    }
}

void GstAppsrcEngine::PushTask()
{
    int32_t ret = MSERR_OK;
    while (ret == MSERR_OK) {
        std::unique_lock<std::mutex> lock(mutex_);
        pushCond_.wait(lock, [this] {
            return ((GetAvailableSize() || atEos_) && needData_) || isExit_;
        });
        if (isExit_) {
            break;
        }
        uint32_t availableSize = GetAvailableSize();
        // pushSize is min(needDataSize_, availableSize, bufferSize_ - appSrcMem_->availableBegin)
        uint32_t pushSize = needDataSize_ > availableSize ? availableSize : needDataSize_;
        pushSize = pushSize > (bufferSize_ - appSrcMem_->availableBegin) ?
            bufferSize_ - appSrcMem_->availableBegin : pushSize;
        if (pushSize == 0) {
            ret = Pusheos();
        }
        if (copyMode_) {
            ret = PushBufferWithCopy(pushSize);
        } else {
            ret = PushBuffer(pushSize);
        }
    }
    if (ret != MSERR_OK) {
        OnError(ret);
    }
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
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("SeekAndFreeBuffers in");
    appSrcMem_->filePos = pos;
    appSrcMem_->availableBegin = appSrcMem_->begin;
    appSrcMem_->end = appSrcMem_->availableBegin == 0 ? bufferSize_ - 1 : appSrcMem_->availableBegin - 1;
    appSrcMem_->pushOffset = pos;
    noFreeBuffer_ = false;
    noAvailableBuffer_ = true;
    atEos_ = false;
    MEDIA_LOGD("SeekAndFreeBuffers end, free mem begin is: %{public}u, free mem end is: %{public}u,"
        "available mem begin is: %{public}u, filePos is: %{public}" PRIu64 "",
        appSrcMem_->begin, appSrcMem_->end, appSrcMem_->availableBegin, appSrcMem_->filePos);
    pullCond_.notify_all();
    return TRUE;
}

int32_t GstAppsrcEngine::PullBuffer()
{
    int32_t ret = MSERR_OK;
    while (ret == MSERR_OK) {
        MEDIA_LOGD("PullBuffer loop");
        int32_t readSize;
        std::unique_lock<std::mutex> lock(mutex_);
        MEDIA_LOGD("free mem begin is: %{public}u, free mem end is: %{public}u", appSrcMem_->begin, appSrcMem_->end);
        pullCond_.wait(lock, [this] { return (!atEos_ && GetFreeSize() >= PULL_SIZE) || isExit_; });
        if (isExit_) {
            break;
        }
        CHECK_AND_RETURN_RET_LOG(appSrcMem_ != nullptr && appSrcMem_->mem != nullptr, MSERR_NO_MEMORY, "no mem");
        int32_t pullSize = bufferSize_ - appSrcMem_->begin;
        pullSize = std::min(pullSize, PULL_SIZE);
        std::static_pointer_cast<AVDataSrcMemory>(appSrcMem_->mem)->SetOffset(appSrcMem_->begin);
        if (size_ == UNKNOW_FILE_SIZE) {
            MEDIA_LOGD("ReadAt begin, offset is %{public}u, length is %{public}d", appSrcMem_->begin, pullSize);
            readSize = dataSrc_->ReadAt(appSrcMem_->mem, pullSize);
        } else {
            MEDIA_LOGD("ReadAt begin, offset is %{public}u, length is %{public}d, pos is %{public}" PRIu64 "",
                appSrcMem_->begin, pullSize, appSrcMem_->filePos);
            readSize = dataSrc_->ReadAt(appSrcMem_->mem, pullSize, appSrcMem_->filePos);
        }
        MEDIA_LOGD("ReadAt end");
        if (readSize > pullSize) {
            MEDIA_LOGE("PullBuffer loop end, readSize > length");
            ret = MSERR_INVALID_VAL;
        }

        if (readSize < 0) {
            MEDIA_LOGD("no buffer, receive eos!!!");
            atEos_ = true;
            timer_ = 0;
        } else if (readSize > 0) {
            appSrcMem_->begin += readSize;
            appSrcMem_->begin %= bufferSize_;
            if (appSrcMem_->begin == (appSrcMem_->end + 1) % bufferSize_) {
                noFreeBuffer_ = true;
                MEDIA_LOGD("noFreeBuffer_ set to true");
            }
            noAvailableBuffer_ = false;
            appSrcMem_->filePos += readSize;
            timer_ = 0;
            MEDIA_LOGD("free mem begin update to %{public}u, filePos update to %{public}" PRIu64 "",
                appSrcMem_->begin, appSrcMem_->filePos);
            pushCond_.notify_all();
        } else if (IsConnectTimeout()) {
            OnError(MSERR_DATA_SOURCE_IO_ERROR);
        }
    }
    return ret;
}

int32_t GstAppsrcEngine::Pusheos()
{
    MEDIA_LOGD("push eos");
    int32_t ret = gst_app_src_end_of_stream(GST_APP_SRC_CAST(appSrc_));
    CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Push eos failed!");
    needData_ = false;
    return MSERR_OK;
}

int32_t GstAppsrcEngine::PushBuffer(uint32_t pushSize)
{
    MEDIA_LOGD("PushBuffer in, pushSize is %{public}d, free mem begin is: %{public}u,"
        "free mem end is: %{public}u,available begin is: %{public}u",
        pushSize, appSrcMem_->begin, appSrcMem_->end, appSrcMem_->availableBegin);
    CHECK_AND_RETURN_RET_LOG(appSrcMem_ != nullptr && appSrcMem_->mem != nullptr, MSERR_NO_MEMORY, "no mem");

    if (appSrcMem_->availableBegin + pushSize <= bufferSize_) {
        auto freeMemory = [this](uint32_t offset, uint32_t length) {
            this->PointerMemoryAvailable(offset, length);
        };
        GstMemory *mem = gst_shmemory_wrap(GST_ALLOCATOR_CAST(allocator_),
            appSrcMem_->mem, appSrcMem_->availableBegin, pushSize, freeMemory);
        CHECK_AND_RETURN_RET_LOG(mem != nullptr, MSERR_NO_MEMORY, "Failed to call gst_shmemory_wrap");
        GstBuffer *buffer = gst_buffer_new();
        if (buffer == nullptr) {
            gst_memory_unref(mem);
            MEDIA_LOGE("Failed to call gst_buffer_new");
            return MSERR_NO_MEMORY;
        }

        gst_buffer_append_memory(buffer, mem);
        GST_BUFFER_OFFSET(buffer) = appSrcMem_->pushOffset;
        appSrcMem_->pushOffset += pushSize;
        if (gst_app_src_push_buffer(GST_APP_SRC_CAST(appSrc_), buffer) != GST_FLOW_OK) {
            gst_buffer_unref(buffer);
            MEDIA_LOGE("Push buffer failed!");
            return MSERR_INVALID_OPERATION;
        }
        appSrcMem_->availableBegin = (appSrcMem_->availableBegin + pushSize) % bufferSize_;
        MEDIA_LOGD("free mem begin is: %{public}u, free mem end is: %{public}u, available begin is: %{public}u",
            appSrcMem_->begin, appSrcMem_->end, appSrcMem_->availableBegin);
        if (appSrcMem_->availableBegin == appSrcMem_->begin) {
            noAvailableBuffer_ = true;
        }
    } else {
        MEDIA_LOGE("appSrcMem_->availableBegin + pushSize > bufferSize_");
        return MSERR_INVALID_OPERATION;
    }
    if (needDataSize_ == pushSize) {
        needData_ = false;
    }
    needDataSize_ -= pushSize;
    MEDIA_LOGD("PushBuffer out");
    return MSERR_OK;
}

int32_t GstAppsrcEngine::PushBufferWithCopy(uint32_t pushSize)
{
    MEDIA_LOGD("PushBufferWithCopy in, pushSize is %{public}d, free mem begin is: %{public}u,"
        "free mem end is: %{public}u, available begin is: %{public}u",
        pushSize, appSrcMem_->begin, appSrcMem_->end, appSrcMem_->availableBegin);
    CHECK_AND_RETURN_RET_LOG(appSrcMem_ != nullptr && appSrcMem_->mem != nullptr, MSERR_NO_MEMORY, "no mem");

    if (appSrcMem_->availableBegin + pushSize <= bufferSize_) {
        GstBuffer *buffer = gst_buffer_new_allocate(nullptr, static_cast<gsize>(pushSize), nullptr);
        CHECK_AND_RETURN_RET_LOG(buffer != nullptr, MSERR_NO_MEMORY, "no mem");

        GstMapInfo info = GST_MAP_INFO_INIT;
        if (gst_buffer_map(buffer, &info, GST_MAP_WRITE) == FALSE) {
            gst_buffer_unref(buffer);
            MEDIA_LOGE("map buffer failed");
            return MSERR_NO_MEMORY;
        }

        guint8 *data = info.data;
        CHECK_AND_RETURN_RET_LOG(memcpy_s(data, pushSize,
            std::static_pointer_cast<AVDataSrcMemory>(appSrcMem_->mem)->GetInnerBase() + appSrcMem_->availableBegin,
            pushSize) == EOK, MSERR_NO_MEMORY, "get mem is nullptr");
        gst_buffer_unmap(buffer, &info);
        GST_BUFFER_OFFSET(buffer) = appSrcMem_->pushOffset;
        appSrcMem_->pushOffset += pushSize;
        if (gst_app_src_push_buffer(GST_APP_SRC_CAST(appSrc_), buffer) != GST_FLOW_OK) {
            gst_buffer_unref(buffer);
            MEDIA_LOGE("Push buffer failed!");
            return MSERR_INVALID_OPERATION;
        }
        appSrcMem_->availableBegin = (appSrcMem_->availableBegin + pushSize) % bufferSize_;
        appSrcMem_->end = (appSrcMem_->end + pushSize) % bufferSize_;
        MEDIA_LOGD("free mem begin is: %{public}u, free mem end is: %{public}u, available begin is: %{public}u",
            appSrcMem_->begin, appSrcMem_->end, appSrcMem_->availableBegin);
        if (appSrcMem_->availableBegin == appSrcMem_->begin) {
            noAvailableBuffer_ = true;
        }
        if (noFreeBuffer_) {
            noFreeBuffer_ = false;
            MEDIA_LOGD("noFreeBuffer_ set to false");
        }
    } else {
        MEDIA_LOGE("appSrcMem_->availableBegin + pushSize > bufferSize_");
        return MSERR_INVALID_OPERATION;
    }
    if (needDataSize_ == pushSize) {
        needData_ = false;
    }
    needDataSize_ -= pushSize;
    pullCond_.notify_all();
    MEDIA_LOGD("PushBufferWithCopy out");
    return MSERR_OK;
}

void GstAppsrcEngine::OnError(int32_t errorCode)
{
    isExit_ = true;
    pullCond_.notify_all();
    pushCond_.notify_all();
    if (notifier_ != nullptr) {
        notifier_(errorCode);
    }
}

uint32_t GstAppsrcEngine::GetFreeSize()
{
    MEDIA_LOGD("free mem begin is: %{public}u, free mem end is: %{public}u, available begin is: %{public}u",
        appSrcMem_->begin, appSrcMem_->end, appSrcMem_->availableBegin);
    uint32_t freeSize;
    if (noFreeBuffer_) {
        freeSize = 0;
    } else {
        freeSize = appSrcMem_->begin <= appSrcMem_->end ?
            appSrcMem_->end - appSrcMem_->begin + 1 :
            bufferSize_ - appSrcMem_->begin + appSrcMem_->end + 1;
    }
    MEDIA_LOGD("GetFreeSize is: %{public}u", freeSize);
    return freeSize;
}

uint32_t GstAppsrcEngine::GetAvailableSize()
{
    MEDIA_LOGD("free mem begin is: %{public}u, free mem end is: %{public}u, available begin is: %{public}u",
        appSrcMem_->begin, appSrcMem_->end, appSrcMem_->availableBegin);
    uint32_t availableSize;
    if (appSrcMem_->availableBegin == appSrcMem_->begin && noAvailableBuffer_) {
        availableSize = 0;
    } else {
        availableSize = appSrcMem_->availableBegin < appSrcMem_->begin ?
            appSrcMem_->begin - appSrcMem_->availableBegin :
            bufferSize_ - appSrcMem_->availableBegin + appSrcMem_->begin;
    }
    MEDIA_LOGD("GetAvailableSize is: %{public}u", availableSize);
    return availableSize;
}

void GstAppsrcEngine::PointerMemoryAvailable(uint32_t offset, uint32_t length)
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("PointerMemoryAvailable in, offset is %{public}u, length is %{public}u, free mem end is %{public}u",
        offset, length, appSrcMem_->end);
    if ((appSrcMem_->end + 1) % bufferSize_ != offset && !copyMode_) {
        MEDIA_LOGE("mempool error, wrap->appSrcMem_->end is %{public}u offset is %{public}u",
            appSrcMem_->end, offset);
        OnError(MSERR_INVALID_OPERATION);
    }
    appSrcMem_->end = offset + length - 1;
    if (noFreeBuffer_) {
        noFreeBuffer_ = false;
        MEDIA_LOGD("noFreeBuffer_ set to false");
    }
    pullCond_.notify_all();
}

static int64_t GetTime()
{
    struct timeval time = {};
    int ret = gettimeofday(&time, nullptr);
    CHECK_AND_RETURN_RET_LOG(ret != -1, -1, "Get current time failed!");
    if ((static_cast<int64_t>(time.tv_sec) < (LLONG_MAX / TIME_VAL_MS)) &&
        (static_cast<int64_t>(time.tv_usec) <= TIME_VAL_US)) {
        return static_cast<int64_t>(time.tv_sec) * TIME_VAL_MS +
            static_cast<int64_t>(time.tv_usec) * TIME_VAL_MS / TIME_VAL_US;
    } else {
        MEDIA_LOGW("time overflow");
    }
    return -1;
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
        if (curTime - timer_ > TIME_OUT_MS) {
            MEDIA_LOGE("No data was received for 15 seconds");
            return true;
        }
    }
    return false;
}
} // namespace Media
} // namespace OHOS
