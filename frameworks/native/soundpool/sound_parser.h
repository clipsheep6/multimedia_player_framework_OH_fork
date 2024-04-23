
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
#ifndef SOUND_PARSER_H
#define SOUND_PARSER_H

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include "ashmem.h"
#include "avcodec_audio_decoder.h"
#include "avcodec_errors.h"
#include "avdemuxer.h"
#include "avsource.h"
#include "buffer/avsharedmemory.h"
#include "avcodec_codec_name.h"
#include "cache_buffer.h"
#include "isoundpool.h"
#include "media_description.h"
#include "media_errors.h"
#include "media_log.h"
#include "securec.h"

namespace OHOS {
namespace Media {
using namespace MediaAVCodec;

class SoundDecoderCallback : public AVCodecCallback, public NoCopyable {
public:
    class SoundDecodeListener {
    public:
        SoundDecodeListener()
        {
            (void)HILOG_IMPL(LOG_CORE, LOG_INFO, LOG_DOMAIN, "SoundDecodeListener",
                "Construction SoundDecodeListener");
        }
        virtual ~SoundDecodeListener()
        {
            (void)HILOG_IMPL(LOG_CORE, LOG_INFO, LOG_DOMAIN, "SoundDecodeListener",
                "Destruction SoundDecodeListener");
        }
        virtual void OnSoundDecodeCompleted(const std::deque<std::shared_ptr<AudioBufferEntry>>
            &availableAudioBuffers) = 0;
        virtual void SetSoundBufferTotalSize(const size_t soundBufferTotalSize) = 0;
    };

    SoundDecoderCallback(const int32_t soundID, const std::shared_ptr<MediaAVCodec::AVCodecAudioDecoder> &audioDec,
        const std::shared_ptr<MediaAVCodec::AVDemuxer> &demuxer, const bool isRawFile);
    ~SoundDecoderCallback();
    int32_t SetDecodeCallback(const std::shared_ptr<SoundDecodeListener> &listener)
    {
        (void)HILOG_IMPL(LOG_CORE, LOG_INFO, LOG_DOMAIN, "SoundDecoderCallback",
            "%{public}s:%{public}d", __func__, __LINE__);
        if (listener == nullptr) {
            (void)HILOG_IMPL(LOG_CORE, LOG_INFO, LOG_DOMAIN, "SoundDecodeListener", "Invalid listener");
            return MSERR_INVALID_VAL;
        }
        listener_ = listener;
        return MSERR_OK;
    }
    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
        std::shared_ptr<AVSharedMemory> buffer) override;

    int32_t SetCallback(const std::shared_ptr<ISoundPoolCallback> &callback);
    int32_t Release();

private:
    const int32_t soundID_;
    std::shared_ptr<MediaAVCodec::AVCodecAudioDecoder> audioDec_;
    std::shared_ptr<MediaAVCodec::AVDemuxer> demuxer_;
    std::shared_ptr<SoundDecodeListener> listener_;
    std::string trackMimeTypeInfo_;
    bool isRawFile_ = false;
    bool eosFlag_;
    std::deque<std::shared_ptr<AudioBufferEntry>> availableAudioBuffers_;
    bool decodeShouldCompleted_;
    int32_t currentSoundBufferSize_;
    std::condition_variable bufferCond_;
    std::shared_ptr<ISoundPoolCallback> callback_ = nullptr;
    std::mutex amutex_;
};

class SoundParser : public std::enable_shared_from_this<SoundParser> {
public:
    SoundParser(int32_t soundID, std::string url);
    SoundParser(int32_t soundID, int32_t fd, int64_t offset, int64_t length);
    ~SoundParser();
    int32_t DoParser();
    int32_t GetSoundID() const
    {
        return soundID_;
    }
    int32_t GetSoundData(std::deque<std::shared_ptr<AudioBufferEntry>> &soundData) const;
    size_t GetSoundDataTotalSize() const;
    MediaAVCodec::Format GetSoundTrackFormat() const
    {
        return trackFormat_;
    }
    bool IsSoundParserCompleted() const;

    int32_t SetCallback(const std::shared_ptr<ISoundPoolCallback> &callback);
    int32_t Release();

private:
    class SoundParserListener : public SoundDecoderCallback::SoundDecodeListener {
    public:
        explicit SoundParserListener(const std::weak_ptr<SoundParser> soundParser) : soundParserInner_(soundParser) {}

        void OnSoundDecodeCompleted(const std::deque<std::shared_ptr<AudioBufferEntry>> &availableAudioBuffers) override
        {
            if (!soundParserInner_.expired()) {
                while (!soundParserInner_.lock()->soundParserLock_.try_lock()) {
                    if (!soundParserInner_.lock()->isParsing_.load()) {
                        return;
                    }
                }
                soundData_ = availableAudioBuffers;
                isSoundParserCompleted_.store(true);
                soundParserInner_.lock()->soundParserLock_.unlock();
            }
        }
        void SetSoundBufferTotalSize(const size_t soundBufferTotalSize) override
        {
            if (!soundParserInner_.expired()) {
                while (!soundParserInner_.lock()->soundParserLock_.try_lock()) {
                    if (!soundParserInner_.lock()->isParsing_.load()) {
                        return;
                    }
                }
                soundBufferTotalSize_ = soundBufferTotalSize;
                soundParserInner_.lock()->soundParserLock_.unlock();
            }
        }
        int32_t GetSoundData(std::deque<std::shared_ptr<AudioBufferEntry>> &soundData) const
        {
            std::unique_lock<std::mutex> lock(soundParserInner_.lock()->soundParserLock_);
            soundData = soundData_;
            return MSERR_OK;
        }

        size_t GetSoundDataTotalSize() const
        {
            std::unique_lock<std::mutex> lock(soundParserInner_.lock()->soundParserLock_);
            return soundBufferTotalSize_;
        }
        bool IsSoundParserCompleted() const
        {
            std::unique_lock<std::mutex> lock(soundParserInner_.lock()->soundParserLock_);
            return isSoundParserCompleted_.load();
        }

    private:
        std::weak_ptr<SoundParser> soundParserInner_;
        std::deque<std::shared_ptr<AudioBufferEntry>> soundData_;
        size_t soundBufferTotalSize_ = 0;
        std::atomic<bool> isSoundParserCompleted_ = false;
    };

    int32_t DoDemuxer(MediaAVCodec::Format *trackFormat);
    int32_t DoDecode(MediaAVCodec::Format trackFormat);
    int32_t soundID_;
    std::shared_ptr<MediaAVCodec::AVDemuxer> demuxer_;
    std::shared_ptr<MediaAVCodec::AVSource> source_;
    std::shared_ptr<MediaAVCodec::AVCodecAudioDecoder> audioDec_;
    std::shared_ptr<SoundDecoderCallback> audioDecCb_;
    std::mutex soundParserLock_;
    std::shared_ptr<SoundParserListener> soundParserListener_;
    std::shared_ptr<ISoundPoolCallback> callback_ = nullptr;
    bool isRawFile_ = false;
    std::atomic<bool> isParsing_ = false;

    MediaAVCodec::Format trackFormat_;

    static constexpr int32_t AUDIO_SOURCE_TRACK_COUNT = 1;
    static constexpr int32_t AUDIO_SOURCE_TRACK_INDEX = 0;
    static constexpr int64_t MIN_FD = 0;
};
} // namespace Media
} // namespace OHOS
#endif // SOUND_PARSER_H
