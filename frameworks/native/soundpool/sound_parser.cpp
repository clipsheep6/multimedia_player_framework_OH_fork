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
#include <fcntl.h>
#include <functional>
#include <cstdio>
#include "isoundpool.h"
#include "sound_parser.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SoundParser"};
    static constexpr int32_t MAX_SOUND_BUFFER_SIZE = 1 * 1024 * 1024;
    static const std::string AUDIO_RAW_MIMETYPE_INFO = "audio/raw";
    static const std::string AUDIO_MPEG_MIMETYPE_INFO = "audio/mpeg";
}

namespace OHOS {
namespace Media {
SoundParser::SoundParser(int32_t soundID, std::string url)
{
    std::shared_ptr<MediaAVCodec::AVSource> source = MediaAVCodec::AVSourceFactory::CreateWithURI(url);
    CHECK_AND_RETURN_LOG(source != nullptr, "Create AVSource failed");
    std::shared_ptr<MediaAVCodec::AVDemuxer> demuxer = MediaAVCodec::AVDemuxerFactory::CreateWithSource(source);
    CHECK_AND_RETURN_LOG(demuxer != nullptr, "Create AVDemuxer failed");
    soundID_ = soundID;
    demuxer_ = demuxer;
    source_ = source;
}

SoundParser::SoundParser(int32_t soundID, int32_t fd, int64_t offset, int64_t length)
{
    fd = fcntl(fd, F_DUPFD_CLOEXEC, MIN_FD); // dup(fd) + close on exec to prevent leaks.
    offset = offset >= INT64_MAX ? INT64_MAX : offset;
    length = length >= INT64_MAX ? INT64_MAX : length;
    std::shared_ptr<MediaAVCodec::AVSource> source = MediaAVCodec::AVSourceFactory::CreateWithFD(fd, offset, length);
    CHECK_AND_RETURN_LOG(source != nullptr, "Create AVSource failed");
    std::shared_ptr<MediaAVCodec::AVDemuxer> demuxer = MediaAVCodec::AVDemuxerFactory::CreateWithSource(source);
    CHECK_AND_RETURN_LOG(demuxer != nullptr, "Create AVDemuxer failed");

    soundID_ = soundID;
    demuxer_ = demuxer;
    source_ = source;
}

SoundParser::~SoundParser()
{
    MEDIA_LOGI("SoundParser Destruction.");
    Release();
}

int32_t SoundParser::DoParser()
{
    MEDIA_LOGE("SoundParser do parser.");
    std::unique_lock<std::mutex> lock(soundParserLock_);
    isParsing_.store(true);
    int32_t result = MSERR_OK;
    result = DoDemuxer(&trackFormat_);
    if (result != MSERR_OK && callback_ != nullptr) {
        callback_->OnError(MSERR_UNSUPPORT_FILE);
    }
    result = DoDecode(trackFormat_);
    if (result != MSERR_OK && callback_ != nullptr) {
        callback_->OnError(MSERR_UNSUPPORT_FILE);
    }
    return MSERR_OK;
}

int32_t SoundParser::DoDemuxer(MediaAVCodec::Format *trackFormat)
{
    MediaAVCodec::Format sourceFormat;
    int32_t sourceTrackCountInfo = 0;
    int64_t sourceDurationInfo = 0;
    CHECK_AND_RETURN_RET_LOG(source_ != nullptr, MSERR_INVALID_VAL, "Failed to obtain av source");
    CHECK_AND_RETURN_RET_LOG(demuxer_ != nullptr, MSERR_INVALID_VAL, "Failed to obtain demuxer");
    CHECK_AND_RETURN_RET_LOG(trackFormat != nullptr, MSERR_INVALID_VAL, "Invalid trackFormat.");
    int32_t ret = source_->GetSourceFormat(sourceFormat);
    if (ret != 0) {
        MEDIA_LOGE("Get source format failed:%{public}d", ret);
    }
    sourceFormat.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_TRACK_COUNT, sourceTrackCountInfo);
    sourceFormat.GetLongValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_DURATION, sourceDurationInfo);

    MEDIA_LOGI("SoundParser sourceTrackCountInfo:%{public}d", sourceTrackCountInfo);
    for (int32_t sourceTrackIndex = 0; sourceTrackIndex < sourceTrackCountInfo; sourceTrackIndex++) {
        int32_t trackType = 0;
        ret = source_->GetTrackFormat(*trackFormat, sourceTrackIndex);
        if (ret != 0) {
            MEDIA_LOGE("Get track format failed:%{public}d", ret);
        }
        trackFormat->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, trackType);
        MEDIA_LOGI("SoundParser trackType:%{public}d", trackType);
        if (trackType == MEDIA_TYPE_AUD) {
            MEDIA_LOGI("SoundParser trackType:%{public}d", trackType);
            demuxer_->SelectTrackByID(sourceTrackIndex);
            int32_t trackBitRateInfo;
            std::string trackMimeTypeInfo;
            trackFormat->GetIntValue(MediaDescriptionKey::MD_KEY_BITRATE, trackBitRateInfo);
            trackFormat->PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, static_cast<int64_t>(trackBitRateInfo));
            trackFormat->GetStringValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_CODEC_MIME, trackMimeTypeInfo);
            if (AUDIO_RAW_MIMETYPE_INFO.compare(trackMimeTypeInfo) != 0) {
                // resample format
                trackFormat->PutIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT,
                    MediaAVCodec::SAMPLE_S16LE);
            } else {
                isRawFile_ = true;
                trackFormat->PutStringValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_CODEC_MIME,
                    AUDIO_MPEG_MIMETYPE_INFO);
            }
            break;
        }
    }
    return MSERR_OK;
}

int32_t SoundParser::DoDecode(MediaAVCodec::Format trackFormat)
{
    int32_t trackTypeInfo;
    trackFormat.GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, trackTypeInfo);
    if (trackTypeInfo == MEDIA_TYPE_AUD) {
        std::string trackMimeTypeInfo;
        trackFormat.GetStringValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_CODEC_MIME, trackMimeTypeInfo);
        MEDIA_LOGI("SoundParser mime type:%{public}s", trackMimeTypeInfo.c_str());
        audioDec_ = MediaAVCodec::AudioDecoderFactory::CreateByMime(trackMimeTypeInfo);
        CHECK_AND_RETURN_RET_LOG(audioDec_ != nullptr, MSERR_INVALID_VAL, "Failed to obtain audioDecorder.");
        int32_t ret = audioDec_->Configure(trackFormat);
        CHECK_AND_RETURN_RET_LOG(ret == 0, MSERR_INVALID_VAL, "Failed to configure audioDecorder.");
        audioDecCb_ = std::make_shared<SoundDecoderCallback>(soundID_, audioDec_, demuxer_, isRawFile_);
        CHECK_AND_RETURN_RET_LOG(audioDecCb_ != nullptr, MSERR_INVALID_VAL, "Failed to obtain decode callback.");
        ret = audioDec_->SetCallback(audioDecCb_);
        CHECK_AND_RETURN_RET_LOG(ret == 0, MSERR_INVALID_VAL, "Failed to setCallback audioDecorder");
        soundParserListener_ = std::make_shared<SoundParserListener>(weak_from_this());
        CHECK_AND_RETURN_RET_LOG(soundParserListener_ != nullptr, MSERR_INVALID_VAL, "Invalid sound parser listener");
        audioDecCb_->SetDecodeCallback(soundParserListener_);
        if (callback_ != nullptr) audioDecCb_->SetCallback(callback_);
        ret = audioDec_->Start();
        CHECK_AND_RETURN_RET_LOG(ret == 0, MSERR_INVALID_VAL, "Failed to Start audioDecorder.");
    }
    return MSERR_OK;
}

int32_t SoundParser::GetSoundData(std::deque<std::shared_ptr<AudioBufferEntry>> &soundData) const
{
    CHECK_AND_RETURN_RET_LOG(soundParserListener_ != nullptr, MSERR_INVALID_VAL, "Invalid sound parser listener");
    return soundParserListener_->GetSoundData(soundData);
}

size_t SoundParser::GetSoundDataTotalSize() const
{
    CHECK_AND_RETURN_RET_LOG(soundParserListener_ != nullptr, MSERR_INVALID_VAL, "Invalid sound parser listener");
    return soundParserListener_->GetSoundDataTotalSize();
}

bool SoundParser::IsSoundParserCompleted() const
{
    CHECK_AND_RETURN_RET_LOG(soundParserListener_ != nullptr, MSERR_INVALID_VAL, "Invalid sound parser listener");
    return soundParserListener_->IsSoundParserCompleted();
}

int32_t SoundParser::SetCallback(const std::shared_ptr<ISoundPoolCallback> &callback)
{
    callback_ = callback;
    return MSERR_OK;
}

int32_t SoundParser::Release()
{
    MEDIA_LOGI("SoundParser Release.");
    std::unique_lock<std::mutex> lock(soundParserLock_);
    isParsing_.store(false);
    int32_t ret = MSERR_OK;
    if (soundParserListener_ != nullptr) soundParserListener_.reset();
    if (audioDecCb_ != nullptr) {
        ret = audioDecCb_->Release();
        audioDecCb_.reset();
    }
    if (audioDec_ != nullptr) {
        ret = audioDec_->Release();
        audioDec_.reset();
    }
    if (demuxer_ != nullptr) demuxer_.reset();
    if (source_ != nullptr) source_.reset();
    if (callback_ != nullptr) callback_.reset();
    return ret;
}

SoundDecoderCallback::SoundDecoderCallback(const int32_t soundID,
    const std::shared_ptr<MediaAVCodec::AVCodecAudioDecoder> &audioDec,
    const std::shared_ptr<MediaAVCodec::AVDemuxer> &demuxer,
    const bool isRawFile) : soundID_(soundID), audioDec_(audioDec),
    demuxer_(demuxer), isRawFile_(isRawFile), eosFlag_(false),
    decodeShouldCompleted_(false), currentSoundBufferSize_(0)
{
    MEDIA_LOGI("Construction SoundDecoderCallback");
}

SoundDecoderCallback::~SoundDecoderCallback()
{
    MEDIA_LOGI("Destruction SoundDecoderCallback");
    Release();
}
void SoundDecoderCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    if (isRawFile_) {
        MEDIA_LOGI("Recive error, errorType:%{public}d,errorCode:%{public}d", errorType, errorCode);
    }
}

void SoundDecoderCallback::OnOutputFormatChanged(const Format &format)
{
    (void)format;
}

void SoundDecoderCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    std::unique_lock<std::mutex> lock(amutex_);
    MediaAVCodec::AVCodecBufferFlag bufferFlag = MediaAVCodec::AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE;
    MediaAVCodec::AVCodecBufferInfo sampleInfo;
    CHECK_AND_RETURN_LOG(demuxer_ != nullptr, "Failed to obtain demuxer");
    CHECK_AND_RETURN_LOG(audioDec_ != nullptr, "Failed to obtain audio decode.");

    if (buffer != nullptr && isRawFile_ && !decodeShouldCompleted_) {
        if (demuxer_->ReadSample(0, buffer, sampleInfo, bufferFlag) != AVCS_ERR_OK) {
            MEDIA_LOGE("SoundDecoderCallback demuxer error.");
            return;
        }
        if (!decodeShouldCompleted_ && (currentSoundBufferSize_ > MAX_SOUND_BUFFER_SIZE ||
                bufferFlag == AVCODEC_BUFFER_FLAG_EOS)) {
            decodeShouldCompleted_ = true;
            CHECK_AND_RETURN_LOG(listener_ != nullptr, "sound decode listener invalid.");
            listener_->OnSoundDecodeCompleted(availableAudioBuffers_);
            listener_->SetSoundBufferTotalSize(static_cast<size_t>(currentSoundBufferSize_));
            CHECK_AND_RETURN_LOG(callback_ != nullptr, "sound decode:soundpool callback invalid.");
            callback_->OnLoadCompleted(soundID_);
            return;
        }
        int32_t size = sampleInfo.size;
        uint8_t *buf = new(std::nothrow) uint8_t[size];
        if (buf != nullptr) {
            if (memcpy_s(buf, size, buffer->GetBase(), size) != EOK) {
                MEDIA_LOGI("audio buffer copy failed:%{public}s", strerror(errno));
            } else {
                availableAudioBuffers_.push_back(std::make_shared<AudioBufferEntry>(buf, size));
                bufferCond_.notify_all();
            }
        }
        currentSoundBufferSize_ += size;
        audioDec_->QueueInputBuffer(index, sampleInfo, bufferFlag);
        return;
    }

    if (buffer != nullptr && !eosFlag_ && !decodeShouldCompleted_) {
        if (demuxer_->ReadSample(0, buffer, sampleInfo, bufferFlag) != AVCS_ERR_OK) {
            MEDIA_LOGE("SoundDecoderCallback demuxer error.");
            return;
        }
        if (bufferFlag == AVCODEC_BUFFER_FLAG_EOS) {
            eosFlag_ = true;
        }
        audioDec_->QueueInputBuffer(index, sampleInfo, bufferFlag);
    }
}

void SoundDecoderCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
    std::shared_ptr<AVSharedMemory> buffer)
{
    std::unique_lock<std::mutex> lock(amutex_);
    if (isRawFile_) {
        MEDIA_LOGI("audio raw data, return.");
        CHECK_AND_RETURN_LOG(audioDec_ != nullptr, "Failed to obtain audio decode.");
        audioDec_->ReleaseOutputBuffer(index);
        return;
    }
    if (buffer != nullptr && !decodeShouldCompleted_) {
        if (currentSoundBufferSize_ > MAX_SOUND_BUFFER_SIZE || flag == AVCODEC_BUFFER_FLAG_EOS) {
            decodeShouldCompleted_ = true;
            CHECK_AND_RETURN_LOG(listener_ != nullptr, "sound decode listener invalid.");
            listener_->OnSoundDecodeCompleted(availableAudioBuffers_);
            listener_->SetSoundBufferTotalSize(static_cast<size_t>(currentSoundBufferSize_));
            CHECK_AND_RETURN_LOG(callback_ != nullptr, "sound decode:soundpool callback invalid.");
            callback_->OnLoadCompleted(soundID_);
            return;
        }
        int32_t size = info.size;
        uint8_t *buf = new(std::nothrow) uint8_t[size];
        if (buf != nullptr) {
            if (memcpy_s(buf, size, buffer->GetBase(), info.size) != EOK) {
                MEDIA_LOGI("audio buffer copy failed:%{public}s", strerror(errno));
            } else {
                availableAudioBuffers_.push_back(std::make_shared<AudioBufferEntry>(buf, size));
                bufferCond_.notify_all();
            }
        }
        currentSoundBufferSize_ += size;
    }
    CHECK_AND_RETURN_LOG(audioDec_ != nullptr, "Failed to obtain audio decode.");
    audioDec_->ReleaseOutputBuffer(index);
}

int32_t SoundDecoderCallback::SetCallback(const std::shared_ptr<ISoundPoolCallback> &callback)
{
    MEDIA_LOGI("SoundDecoderCallback::SetCallback");
    callback_ = callback;
    return MSERR_OK;
}

int32_t SoundDecoderCallback::Release()
{
    int32_t ret = MSERR_OK;
    MEDIA_LOGI("SoundDecoderCallback Release.");
    if (audioDec_ != nullptr) {
        ret = audioDec_->Release();
        audioDec_.reset();
    }
    if (demuxer_ != nullptr) demuxer_.reset();
    if (listener_ != nullptr) listener_.reset();
    if (!availableAudioBuffers_.empty()) availableAudioBuffers_.clear();
    if (callback_ != nullptr) callback_.reset();
    return ret;
}
} // namespace Media
} // namespace OHOS
