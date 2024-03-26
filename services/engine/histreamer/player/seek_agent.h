 /*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef SEEK_AGENT_H
#define SEEK_AGENT_H

#include <memory>
#include <unordered_map>
#include "common/status.h"
#include "avbuffer_queue_define.h"
#include "osal/task/condition_variable.h"
#include "osal/task/mutex.h"
#include "iremote_stub.h"
#include "demuxer_filter.h"

namespace OHOS {
namespace Media {
class HiPlayerImpl;

class SeekAgent : public std::enable_shared_from_this<SeekAgent> {
public:
    explicit SeekAgent(std::shared_ptr<Pipeline::DemuxerFilter> demuxer);
    ~SeekAgent();

    Status Seek(int64_t seekPos);
    Status OnAudioBufferFilled(std::shared_ptr<AVBuffer>& buffer,
        sptr<AVBufferQueueProducer> producer, int32_t trackId);
    Status OnVideoBufferFilled(std::shared_ptr<AVBuffer>& buffer,
        sptr<AVBufferQueueProducer> producer, int32_t trackId);

private:
    Status SetBufferFilledListener();
    Status RemoveBufferFilledListener();

    std::shared_ptr<Pipeline::DemuxerFilter> demuxer_;
    Mutex targetArrivedLock_;
    ConditionVariable targetArrivedCond_;
    bool isAudioTargetArrived_{false};
    bool isVideoTargetArrived_{false};

    int64_t seekTargetPos_{-1};
    std::atomic<bool> isSeeking_{false};
    std::map<uint32_t, sptr<AVBufferQueueProducer>> producerMap_;
    std::map<uint32_t, sptr<IBrokerListener>> listenerMap_;

    static constexpr uint32_t WAIT_MAX_MS = 2000;
    static constexpr uint32_t MS_TO_US = 1000;
};

class AudioBufferFilledListener : public IRemoteStub<IBrokerListener> {
public:
    AudioBufferFilledListener(std::shared_ptr<SeekAgent> seekAgent,
        sptr<AVBufferQueueProducer> producer, int32_t trackId);
    ~AudioBufferFilledListener() = default;

    void OnBufferFilled(std::shared_ptr<AVBuffer>& buffer);

private:
    std::weak_ptr<SeekAgent> seekAgent_;
    sptr<AVBufferQueueProducer> producer_;
    int32_t trackId_;
};

class VideoBufferFilledListener : public IRemoteStub<IBrokerListener> {
public:
    VideoBufferFilledListener(std::shared_ptr<SeekAgent> seekAgent,
        sptr<AVBufferQueueProducer> producer, int32_t trackId);
    ~VideoBufferFilledListener() = default;

    void OnBufferFilled(std::shared_ptr<AVBuffer>& buffer);

private:
    std::weak_ptr<SeekAgent> seekAgent_;
    sptr<AVBufferQueueProducer> producer_;
    int32_t trackId_;
};
} // namespace Media
} // namespace OHOS
#endif // SEEK_AGENT_H
