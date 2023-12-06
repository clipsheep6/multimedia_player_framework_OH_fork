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

#ifndef RECORDERSTOP_H
#define RECORDERSTOP_H

#include <mutex>
#include <functional>

namespace OHOS {
namespace Media {
class StopCallback {
public:
    virtual ~StopCallback() = default;

    virtual void OnStop(int64_t stopTime) = 0;
};

class StopDoneCallback {
public:
    virtual ~StopDoneCallback() = default;

    virtual void OnStopDone() = 0;
};

/**
 * @brief A simple pool implementation for shared memory.
 *
 * This pool support multi configuration:
 * @preAllocMemCnt: The number of memory blocks allocated when the pool is initialized.
 * @memSize: the size of the preallocated memory blocks.
 * @maxMemCnt: the total number of memory blocks in the pool.
 * @flags: the shared memory access property, refer to {@AVSharedMemory::Flags}.
 * @enableFixedSize: if true, the pool will allocate all memory block using the memSize. If the acquired
 *                  size is larger than the memSize, the acquire will failed. If false, the pool will
 *                  free the smallest idle memory block when there is no idle memory block that can
 *                  satisfy the acqiured size and reallocate a new memory block with the acquired size.
 * @notifier: the callback will be called to notify there are any available memory. It will be useful for
 *            non-blocking memory acquisition.
 */
class __attribute__((visibility("default"))) RecorderStop {
public:
    static RecorderStop* GetInstance();
    void RegisterStopCallback(std::shared_ptr<StopCallback> stopCallback);
    void NotifyStop(int64_t stopTime);
    void RegisterStopDoneCallback(std::shared_ptr<StopDoneCallback> stopDoneCallback);
    void NotifyStopDone();
    void SetEncoder(bool isEncoder);
    bool GetEncoder();
    void Release();

private:
    RecorderStop();
    ~RecorderStop();
    static RecorderStop* instance_;
    static std::mutex mutex_;
    std::vector<std::shared_ptr<StopCallback>> stopCallbacks_;
    std::vector<std::shared_ptr<StopDoneCallback>> stopDoneCallbacks_;
    int32_t stopCallbackNum_;
    bool isEncoder_;
};
} // namespace Media
} // namespace OHOS

#endif