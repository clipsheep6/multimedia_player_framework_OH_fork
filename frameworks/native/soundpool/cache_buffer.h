
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
#ifndef CACHE_BUFFER_H
#define CACHE_BUFFER_H

#include <deque>
#include "audio_renderer.h"
#include "audio_info.h"
#include "isoundpool.h"
#include "media_description.h"

namespace OHOS {
namespace Media {
using namespace MediaAVCodec;

struct AudioBufferEntry {
    AudioBufferEntry(uint8_t *buf, int32_t length) : buffer(std::move(buf)), size(length) {}
    ~AudioBufferEntry()
    {
        if (buffer != nullptr) {
            free(buffer);
            buffer = nullptr;
        }
    }
    uint8_t *buffer;
    int32_t size;
};

class CacheBuffer {
public:
    CacheBuffer(MediaAVCodec::Format trackFormat,
            std::deque<std::shared_ptr<AudioBufferEntry>> cacheData,
            int32_t soundID, int32_t streamID);
    ~CacheBuffer();
    int32_t DoPlay(const int32_t streamID, const AudioStandard::AudioRendererInfo audioRendererInfo,
        const PlayParams playParams);
    int32_t Stop(const int32_t streamID);
    int32_t SetVolume(const int32_t streamID, const float leftVolume, const float rightVolume);
    int32_t SetRate(const int32_t streamID, const AudioStandard::AudioRendererRate renderRate);
    int32_t SetPriority(const int32_t streamID, const int32_t priority);
    int32_t SetLoop(const int32_t streamID, const int32_t loop);
    int32_t SetParallelPlayFlag(const int32_t streamID, const bool parallelPlayFlag);
    int32_t SetCallback(const std::shared_ptr<ISoundPoolCallback> &callback);

    bool IsRunning() const
    {
        return isRunning_;
    }
    int32_t GetSoundID() const
    {
        return soundID_;
    }
    int32_t GetStreamID() const
    {
        return streamID_;
    }

private:
    int32_t PreparePlay(const int32_t streamID, const AudioStandard::AudioRendererInfo audioRendererInfo,
        const PlayParams playParams);
    std::unique_ptr<AudioStandard::AudioRenderer> CreateAudioRenderer(const int32_t streamID,
        const AudioStandard::AudioRendererInfo audioRendererInfo, const PlayParams playParams);
    void AudioRendererRunning();
    int32_t DealPlayParamsBeforePaly(const int32_t streamID, const PlayParams playParams);

    MediaAVCodec::Format trackFormat_;
    std::deque<std::shared_ptr<AudioBufferEntry>> cacheData_;
    int32_t soundID_;
    int32_t streamID_;

    std::mutex cachebufferLock_;
    // use for save audiobuffer
    std::unique_ptr<AudioStandard::AudioRenderer> audioRenderer_;
    std::condition_variable bufferCond_;
    std::mutex amutex_;
    std::atomic<bool> isRunning_ = false;
    std::shared_ptr<ISoundPoolCallback> callback_ = nullptr;

    int32_t loop_ = 0;
    int32_t priority_ = 0;
    bool parallelPlayFlag_ = false;
};
} // namespace Media
} // namespace OHOS
#endif // CACHE_BUFFER_H
