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
#include "string_ex.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SoundParser"};
    static constexpr int32_t MAX_SOUND_BUFFER_SIZE = 1 * 1024 * 1024;
    static const std::string AUDIO_RAW_MIMETYPE_INFO = "audio/raw";
    static const std::string AUDIO_MPEG_MIMETYPE_INFO = "audio/mpeg";
    static constexpr int64_t WAV_HEADER_SIZE = 42;
}

namespace OHOS {
namespace Media {
SoundParser::SoundParser(int32_t soundID, std::string url)
{
    const std::string fdHead = "fd://";
    int32_t fd = -1;
    StrToInt(url.substr(fdHead.size()), fd);
    int32_t fdFile = fcntl(fd, F_DUPFD, MIN_FD);

    MEDIA_LOGI("SoundParser::SoundParser url::%{public}s, fdFile:%{public}d", url.c_str(), fdFile);
    std::shared_ptr<MediaAVCodec::AVSource> source = MediaAVCodec::AVSourceFactory::CreateWithURI(url);
    CHECK_AND_RETURN_LOG(source != nullptr, "Create AVSource failed");
    std::shared_ptr<MediaAVCodec::AVDemuxer> demuxer = MediaAVCodec::AVDemuxerFactory::CreateWithSource(source);
    CHECK_AND_RETURN_LOG(demuxer != nullptr, "Create AVDemuxer failed");
    soundID_ = soundID;
    demuxer_ = demuxer;
    source_ = source;
    fileReadLength_ = MAX_SOUND_BUFFER_SIZE;
    filePtr_ = fdopen(fdFile, "rb");
    CHECK_AND_RETURN_LOG(filePtr_ != nullptr, "SoundParser::SoundParser filePtr_ is nullptr");
}

SoundParser::SoundParser(int32_t soundID, int32_t fd, int64_t offset, int64_t length)
{
    int32_t fdSource = fcntl(fd, F_DUPFD_CLOEXEC, MIN_FD); // dup(fd) + close on exec to prevent leaks.
    int32_t fdFile = fcntl(fd, F_DUPFD, MIN_FD);
    offset = offset >= INT64_MAX ? INT64_MAX : offset;
    length = length >= INT64_MAX ? INT64_MAX : length;
    MEDIA_LOGI("SoundParser fd:%{public}d, fdSource:%{public}d, fdFile:%{public}d", fd, fdSource, fdFile);
    std::shared_ptr<MediaAVCodec::AVSource> source = 
        MediaAVCodec::AVSourceFactory::CreateWithFD(fdSource, offset, length);
    CHECK_AND_RETURN_LOG(source != nullptr, "Create AVSource failed");
    std::shared_ptr<MediaAVCodec::AVDemuxer> demuxer = MediaAVCodec::AVDemuxerFactory::CreateWithSource(source);
    CHECK_AND_RETURN_LOG(demuxer != nullptr, "Create AVDemuxer failed");

    soundID_ = soundID;
    demuxer_ = demuxer;
    source_ = source;
    fileOffset_ = offset;
    fileReadLength_ = length > MAX_SOUND_BUFFER_SIZE ? MAX_SOUND_BUFFER_SIZE : static_cast<int32_t>(length);
    filePtr_ = fdopen(fdFile, "rb");
    CHECK_AND_RETURN_LOG(filePtr_ != nullptr, "SoundParser::SoundParser filePtr_ is nullptr");
}

SoundParser::~SoundParser()
{
    MEDIA_LOGI("SoundParser Destruction.");
    Release();
}

int32_t SoundParser::DoParser()
{
    MEDIA_LOGE("SoundParser do parser.");
    std::unique_lock<ffrt::mutex> lock(soundParserLock_);
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
    if (result == MSERR_OK && isRawFile_) {
        callback_->OnLoadCompleted(soundID_);
    }
    if (filePtr_ != nullptr) {
        fclose(filePtr_);
        filePtr_ = nullptr;
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
            std::string trackMimeTypeInfo;
            trackFormat->GetStringValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_CODEC_MIME, trackMimeTypeInfo);
            if (AUDIO_RAW_MIMETYPE_INFO.compare(trackMimeTypeInfo) != 0) {
                // resample format
                trackFormat->PutIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT,
                    MediaAVCodec::SAMPLE_S16LE);
            } else {
                isRawFile_ = true;
                return GetAudioBuffers(trackFormat);
            }
            break;
        }
    }
    return MSERR_OK;
}

int32_t SoundParser::GetAudioBuffers(MediaAVCodec::Format *trackFormat)
{
    trackFormat->PutStringValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_CODEC_MIME,
        AUDIO_MPEG_MIMETYPE_INFO);
    int32_t trackSampleRate;
    int32_t trackChannels;
    int32_t trackBitPerSample;
    trackFormat->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, trackSampleRate);
    trackFormat->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, trackChannels);
    trackFormat->GetIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, trackBitPerSample);
    MEDIA_LOGI("SoundParser SampleRate:%{public}d , Channels:%{public}d , BitRate:%{public}d",
        trackSampleRate, trackChannels, trackBitPerSample);
    int32_t bufferLength = trackSampleRate * 20 / 1000 * trackChannels * trackBitPerSample / 8;
    MEDIA_LOGI("SoundParser bufferLength:%{public}d", bufferLength);

    CHECK_AND_RETURN_RET_LOG(filePtr_ != NULL, MSERR_INVALID_VAL, "SoundParser::GetAudioBuffers filePtr_ is null");

    fseek(filePtr_, 0L, SEEK_END);
    int64_t fileSize = static_cast<int64_t>(ftell(filePtr_));
    fseek(filePtr_, 0L, SEEK_SET);

    MEDIA_LOGI("SoundParser fileSize:%{public}ld , WAV_HEADER_SIZE:%{public}ld , fileOffset_:%{public}ld",
        fileSize, WAV_HEADER_SIZE, fileOffset_);

    CHECK_AND_RETURN_RET_LOG(WAV_HEADER_SIZE + fileOffset_ <= fileSize, MSERR_INVALID_VAL,
        "SoundParser::GetAudioBuffers offset too large");
    fseeko64(filePtr_, WAV_HEADER_SIZE + fileOffset_, SEEK_SET);
    while (true)
    {
        if (feof(filePtr_)) {
            MEDIA_LOGI("SoundParser::GetAudioBuffers read WAV end");
            break;
        }
        uint8_t *buf = new(std::nothrow) uint8_t[bufferLength];
        if (buf != nullptr) {
            size_t bytesRead = fread(buf, 1, bufferLength, filePtr_);
            if (bytesRead > 0 && rawSoundBufferTotalSize_ < fileReadLength_) {
                rawAudioBuffers_.push_back(std::make_shared<AudioBufferEntry>(buf, bufferLength));
                rawSoundBufferTotalSize_ += bufferLength;
            } else {
                MEDIA_LOGI("SoundParser::GetAudioBuffers read WAV stop");
                break;
            }
        } else {
            MEDIA_LOGI("SoundParser::GetAudioBuffers buff is null");
            return MSERR_INVALID_VAL;
        }
    }
    rawSoundParserCompleted_.store(true);
    
    return MSERR_OK;
}

int32_t SoundParser::DoDecode(MediaAVCodec::Format trackFormat)
{
    int32_t trackTypeInfo;
    trackFormat.GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, trackTypeInfo);
    if (trackTypeInfo == MEDIA_TYPE_AUD && !isRawFile_) {
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
    MEDIA_LOGI("SoundParser::GetSoundData isRawFile_:%{public}d", isRawFile_);
    if (isRawFile_) {
        soundData = rawAudioBuffers_;
        return MSERR_OK;
    } else {
        CHECK_AND_RETURN_RET_LOG(soundParserListener_ != nullptr, MSERR_INVALID_VAL, "Invalid sound parser listener");
        return soundParserListener_->GetSoundData(soundData);
    }
}

size_t SoundParser::GetSoundDataTotalSize() const
{
    MEDIA_LOGI("SoundParser::GetSoundDataTotalSize isRawFile_:%{public}d", isRawFile_);
    if (isRawFile_) {
        return rawSoundBufferTotalSize_;
    } else {
        CHECK_AND_RETURN_RET_LOG(soundParserListener_ != nullptr, MSERR_INVALID_VAL, "Invalid sound parser listener");
        return soundParserListener_->GetSoundDataTotalSize();
    }
}

bool SoundParser::IsSoundParserCompleted() const
{
    MEDIA_LOGI("SoundParser::IsSoundParserCompleted isRawFile_:%{public}d", isRawFile_);
    if (isRawFile_) {
        return rawSoundParserCompleted_.load();
    } else {
        CHECK_AND_RETURN_RET_LOG(soundParserListener_ != nullptr, MSERR_INVALID_VAL, "Invalid sound parser listener");
        return soundParserListener_->IsSoundParserCompleted();
    }
}

int32_t SoundParser::SetCallback(const std::shared_ptr<ISoundPoolCallback> &callback)
{
    callback_ = callback;
    return MSERR_OK;
}

int32_t SoundParser::Release()
{
    MEDIA_LOGI("SoundParser Release.");
    std::unique_lock<ffrt::mutex> lock(soundParserLock_);
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
