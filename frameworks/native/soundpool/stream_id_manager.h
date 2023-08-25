
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
#ifndef STREAM_ID_MANAGER_H
#define STREAM_ID_MANAGER_H

#include <atomic>
#include <thread>
#include "cache_buffer.h"
#include "isoundpool.h"
#include "sound_parser.h"
#include "thread_pool.h"

namespace OHOS {
namespace Media {
class StreamIDManager {
public:
    StreamIDManager(int32_t maxStreams, AudioStandard::AudioRendererInfo audioRenderInfo);
    ~StreamIDManager();

    int32_t Play(std::shared_ptr<SoundParser> soundParser, PlayParams playParameters);

    std::shared_ptr<CacheBuffer> FindCacheBuffer(const int32_t streamID);

    int32_t SetCallback(const std::shared_ptr<ISoundPoolCallback> &callback);

    AudioStandard::AudioRendererInfo audioRendererInfo_;

private:
    // audio render max concurrency count.
    static constexpr int32_t MAX_STREAMS_TASK_NUMBER = 16;
    static constexpr int32_t MAX_STREAM_ID_QUEUE = 128;

    int32_t InitThreadPool();
    int32_t SetPlay(const int32_t soundID, const int32_t streamID, const PlayParams playParameters);
    int32_t DoPlay(const int32_t streamID, const PlayParams playParameters);
    int32_t GetNewStreamID(const int32_t soundID, PlayParams playParameters);

    std::mutex streamIDManagerLock_;
    std::shared_ptr<ISoundPoolCallback> callback_ = nullptr;
    std::map<int32_t, std::shared_ptr<CacheBuffer>> cacheBuffers_;
    int32_t nextStreamID_ = 0;
    int32_t maxStreams_ = MAX_STREAMS_TASK_NUMBER;

    std::atomic<bool> isStreamPlayingThreadPoolStarted_ = false;
    std::unique_ptr<ThreadPool> streamPlayingThreadPool_;

    std::condition_variable queueSpaceValid_;
    std::condition_variable queueDataValid_;
    std::deque<int32_t> streamIDs_;
    bool quitQueue_ = false;
};
} // namespace Media
} // namespace OHOS
#endif // STREAM_ID_MANAGER_H
