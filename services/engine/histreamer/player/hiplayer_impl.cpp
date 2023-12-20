/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "HiPlayerImpl"

#include "hiplayer_impl.h"

#include "audio_info.h"
#include "av_common.h"
#include "common/log.h"
#include "common/media_source.h"
#include "filter/filter_factory.h"
#include "media_errors.h"
#include "osal/utils/dump_buffer.h"

namespace {
const float MAX_MEDIA_VOLUME = 1.0f; // standard interface volume is between 0 to 1.
}

namespace OHOS {
namespace Media {
using namespace Pipeline;
constexpr double EPSINON = 0.0001;
constexpr double SPEED_0_75_X = 0.75;
constexpr double SPEED_1_00_X = 1.00;
constexpr double SPEED_1_25_X = 1.25;
constexpr double SPEED_1_75_X = 1.75;
constexpr double SPEED_2_00_X = 2.00;
const std::pair<Status, int> g_statusPair[] = {
    {Status::OK, MSERR_OK},
    {Status::ERROR_UNKNOWN, MSERR_UNKNOWN},
    {Status::ERROR_AGAIN, MSERR_UNKNOWN},
    {Status::ERROR_UNIMPLEMENTED, MSERR_UNSUPPORT},
    {Status::ERROR_INVALID_PARAMETER, MSERR_INVALID_VAL},
    {Status::ERROR_INVALID_OPERATION, MSERR_INVALID_OPERATION},
    {Status::ERROR_UNSUPPORTED_FORMAT, MSERR_UNSUPPORT_CONTAINER_TYPE},
    {Status::ERROR_NOT_EXISTED, MSERR_OPEN_FILE_FAILED},
    {Status::ERROR_TIMED_OUT, MSERR_EXT_TIMEOUT},
    {Status::ERROR_NO_MEMORY, MSERR_EXT_NO_MEMORY},
    {Status::ERROR_INVALID_STATE, MSERR_INVALID_STATE},
};

class PlayerEventReceiver : public EventReceiver {
public:
    explicit PlayerEventReceiver(HiPlayerImpl* hiPlayerImpl)
    {
        hiPlayerImpl_ = hiPlayerImpl;
    }

    void OnEvent(const Event &event)
    {
        hiPlayerImpl_->OnEvent(event);
    }

private:
    HiPlayerImpl* hiPlayerImpl_;
};

class PlayerFilterCallback : public FilterCallback {
public:
    explicit PlayerFilterCallback(HiPlayerImpl* hiPlayerImpl)
    {
        hiPlayerImpl_ = hiPlayerImpl;
    }

    void OnCallback(const std::shared_ptr<Filter>& filter, FilterCallBackCommand cmd, StreamType outType)
    {
        hiPlayerImpl_->OnCallback(filter, cmd, outType);
    }

private:
    HiPlayerImpl* hiPlayerImpl_;
};

HiPlayerImpl::HiPlayerImpl(int32_t appUid, int32_t appPid, uint32_t appTokenId, uint64_t appFullTokenId)
    : appUid_(appUid), appPid_(appPid), appTokenId_(appTokenId), appFullTokenId_(appFullTokenId)
{
    MEDIA_LOG_I("hiPlayerImpl ctor appUid " PUBLIC_LOG_D32 " appPid " PUBLIC_LOG_D32 " appTokenId " PUBLIC_LOG_D32
        " appFullTokenId " PUBLIC_LOG_D64, appUid_, appPid_, appTokenId_, appFullTokenId_);
    pipeline_ = std::make_shared<OHOS::Media::Pipeline::Pipeline>();
}

HiPlayerImpl::~HiPlayerImpl()
{
    MEDIA_LOG_I("dtor called.");
    pipeline_->Stop();
    audioSink_.reset();
#ifdef VIDEO_SUPPORT
    videoSink_.reset();
#endif
}

Status HiPlayerImpl::Init()
{
    MEDIA_LOG_I("Init entered.");
    std::shared_ptr<EventReceiver> playerEventReceiver = std::make_shared<PlayerEventReceiver>(this);
    playerEventReceiver_ = playerEventReceiver;
    std::shared_ptr<FilterCallback> playerFilterCallback = std::make_shared<PlayerFilterCallback>(this);
    playerFilterCallback_ = playerFilterCallback;
    MEDIA_LOG_I("pipeline init start");
    pipeline_->Init(playerEventReceiver, playerFilterCallback);
    MEDIA_LOG_I("Init End.");
    return Status::OK;
}

int32_t HiPlayerImpl::SetSource(const std::string& uri)
{
    MEDIA_LOG_I("SetSource entered source uri: " PUBLIC_LOG_S, uri.c_str());
    auto ret = Init();
    if (ret == Status::OK) {
        url_ = uri;
        ret = DoSetSource(std::make_shared<MediaSource>(url_));
    }
    if (ret != Status::OK) {
        MEDIA_LOG_E("SetSource error: " PUBLIC_LOG_D32, ret);
    } else {
        OnStateChanged(PlayerStateId::INIT);
    }
    return TransStatus(ret);
}

int32_t HiPlayerImpl::SetSource(const std::shared_ptr<IMediaDataSource>& dataSrc)
{
    MEDIA_LOG_I("SetSource entered source stream");
    auto ret = Init();
    if (ret == Status::OK) {
        ret = DoSetSource(std::make_shared<MediaSource>(dataSrc));
    }
    if (ret != Status::OK) {
        MEDIA_LOG_E("SetSource error: " PUBLIC_LOG_D32, ret);
    } else {
        OnStateChanged(PlayerStateId::INIT);
    }
    return TransStatus(ret);
}

int32_t HiPlayerImpl::Prepare()
{
    MEDIA_LOG_I("Prepare start");
    Status ret = pipeline_->Prepare();
    if (ret != Status::OK) {
        return TransStatus(ret);
    }
    AutoLock lock(stateMutex_);
    if (curState_ == PlayerStateId::PREPARING) { // Wait state change to ready
        cond_.Wait(lock, [this] { return curState_ != PlayerStateId::READY; });
    }
    if (curState_ == PlayerStateId::READY) {
        ret = Status::OK;
        Format format;
        callbackLooper_.OnInfo(INFO_TYPE_STATE_CHANGE, PlayerStates::PLAYER_PREPARED, format);
    } else {
        ret = Status::ERROR_UNKNOWN;
    }
    MEDIA_LOG_I("Prepare End");
    return TransStatus(ret);
}

int HiPlayerImpl::PrepareAsync()
{
    MEDIA_LOG_I("PrepareAsync Start");
    auto ret = pipeline_->Prepare();
    if (ret != Status::OK) {
        MEDIA_LOG_E("Prepare async failed with error " PUBLIC_LOG_D32, ret);
    }
    MEDIA_LOG_I("PrepareAsync End");
    OnStateChanged(PlayerStateId::READY);
    Format format;
    callbackLooper_.OnInfo(INFO_TYPE_STATE_CHANGE, PlayerStates::PLAYER_PREPARED, format);
    return TransStatus(ret);
}

int32_t HiPlayerImpl::Play()
{
    MEDIA_LOG_I("Play entered.");
    auto ret {Status::OK};
    if (curState_ == PlayerStateId::READY) {
        ret = pipeline_->Start();
    }
    if (ret == Status::OK) {
        OnStateChanged(PlayerStateId::PLAYING);
    }
    return TransStatus(ret);
}

int32_t HiPlayerImpl::Pause()
{
    MEDIA_LOG_I("Pause entered.");
    auto ret = pipeline_->Pause();
    OnStateChanged(PlayerStateId::PAUSE);
    return TransStatus(ret);
}

int32_t HiPlayerImpl::Stop()
{
    MEDIA_LOG_I("Stop entered.");
    // close demuxer first to avoid concurrent problem
    if (demuxer_ != nullptr) {
        demuxer_->Stop();
    }
    auto ret = Status::OK;
    if (pipeline_ != nullptr) {
        pipeline_->Stop();
    }
    OnStateChanged(PlayerStateId::STOPPED);
    return TransStatus(ret);
}

int32_t HiPlayerImpl::Reset()
{
    MEDIA_LOG_I("Reset entered.");
    Stop();
    auto ret = Resume();
    OnStateChanged(PlayerStateId::STOPPED);
    return TransStatus(ret);
}

int32_t HiPlayerImpl::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    MEDIA_LOG_I("Seek entered. mSeconds : " PUBLIC_LOG_D32 ", seekMode : " PUBLIC_LOG_D32,
        mSeconds, static_cast<int32_t>(mode));
    int64_t hstTime = 0;
    int32_t durationMs;
    GetDuration(durationMs);
    if (durationMs <= 0) {
        MEDIA_LOG_E("Seek, invalid operation, source is unseekable or invalid");
        return (int32_t)Status::ERROR_INVALID_PARAMETER;
    }
    MEDIA_LOG_D("Seek durationMs : " PUBLIC_LOG_D32, durationMs);
    if (mSeconds >= durationMs) { // if exceeds change to duration
        mSeconds = durationMs;
    }
    mSeconds = mSeconds < 0 ? 0 : mSeconds;
    int64_t seekPos = hstTime;
    SeekMode seekMode;
    switch (mode) {
        case PlayerSeekMode::SEEK_NEXT_SYNC:
            seekMode = Plugin::SeekMode::SEEK_NEXT_SYNC;
        case PlayerSeekMode::SEEK_PREVIOUS_SYNC:
            seekMode = Plugin::SeekMode::SEEK_PREVIOUS_SYNC;
        case PlayerSeekMode::SEEK_CLOSEST_SYNC:
            seekMode = Plugin::SeekMode::SEEK_CLOSEST_SYNC;
        case PlayerSeekMode::SEEK_CLOSEST:
        default:
            seekMode = Plugin::SeekMode::SEEK_CLOSEST;
    }
    auto rtv = seekPos >= 0 ? Status::OK : Status::ERROR_INVALID_PARAMETER;
    if (rtv == Status::OK) {
        pipeline_->Flush();
        MEDIA_LOG_I("Do seek ...");
        int64_t realSeekTime = seekPos;
        rtv = demuxer_->SeekTo(seekPos, seekMode, realSeekTime);
    }
    if (rtv != Status::OK) {
        MEDIA_LOG_E("Seek done, seek error.");
    }
    return TransStatus(rtv);
}

int32_t HiPlayerImpl::SetVolume(float leftVolume, float rightVolume)
{
    MEDIA_LOG_I("SetVolume entered.");
    if (leftVolume < 0 || leftVolume > MAX_MEDIA_VOLUME || rightVolume < 0 || rightVolume > MAX_MEDIA_VOLUME) {
        MEDIA_LOG_E("volume not valid, should be in range [0,100]");
        return (int32_t)Status::ERROR_INVALID_PARAMETER;
    }
    float volume = 0.0f;
    if (leftVolume < 1e-6 && rightVolume >= 1e-6) {  // 1e-6
        volume = rightVolume;
    } else if (rightVolume < 1e-6 && leftVolume >= 1e-6) {  // 1e-6
        volume = leftVolume;
    } else {
        volume = (leftVolume + rightVolume) / 2;  // 2
    }
    volume /= MAX_MEDIA_VOLUME;  // normalize to 0~1
    Status ret = Status::OK;
    ret = audioSink_->SetVolume(volume);
    if (ret != Status::OK) {
        MEDIA_LOG_E("SetVolume failed with error " PUBLIC_LOG_D32, static_cast<int>(ret));
    }
    return TransStatus(ret);
}

int32_t HiPlayerImpl::SetVideoSurface(sptr<Surface> surface)
{
    MEDIA_LOG_D("SetVideoSurface entered.");
#ifdef VIDEO_SUPPORT
    FALSE_RETURN_V_MSG_E(surface != nullptr, (int32_t)(Status::ERROR_INVALID_PARAMETER),
                         "Set video surface failed, surface == nullptr");
    return TransStatus(videoSink_->SetVideoSurface(surface));
#else
    return TransStatus(Status::OK);
#endif
}

int32_t HiPlayerImpl::SetLooping(bool loop)
{
    MEDIA_LOG_I("SetLooping entered, loop: " PUBLIC_LOG_D32, loop);
    singleLoop_ = loop;
    return TransStatus(Status::OK);
}

int32_t HiPlayerImpl::SetParameter(const Format& params)
{
    MEDIA_LOG_I("SetParameter entered.");
#ifdef VIDEO_SUPPORT
    if (params.ContainKey(PlayerKeys::VIDEO_SCALE_TYPE)) {
        int32_t videoScaleType = 0;
        params.GetIntValue(PlayerKeys::VIDEO_SCALE_TYPE, videoScaleType);
        return SetVideoScaleType(VideoScaleType(videoScaleType));
    }
#endif
    if (params.ContainKey(PlayerKeys::CONTENT_TYPE) && params.ContainKey(PlayerKeys::STREAM_USAGE)) {
        int32_t contentType;
        int32_t streamUsage;
        int32_t rendererFlag;
        params.GetIntValue(PlayerKeys::CONTENT_TYPE, contentType);
        params.GetIntValue(PlayerKeys::STREAM_USAGE, streamUsage);
        params.GetIntValue(PlayerKeys::RENDERER_FLAG, rendererFlag);
        return SetAudioRendererInfo(contentType, streamUsage, rendererFlag);
    }
    if (params.ContainKey(PlayerKeys::AUDIO_INTERRUPT_MODE)) {
        int32_t interruptMode = 0;
        params.GetIntValue(PlayerKeys::AUDIO_INTERRUPT_MODE, interruptMode);
        return SetAudioInterruptMode(interruptMode);
    }
    return TransStatus(Status::OK);
}

int32_t HiPlayerImpl::SetObs(const std::weak_ptr<IPlayerEngineObs>& obs)
{
    MEDIA_LOG_I("SetObs entered.");
    callbackLooper_.StartWithPlayerEngineObs(obs);
    return TransStatus(Status::OK);
}

int32_t HiPlayerImpl::GetCurrentTime(int32_t& currentPositionMs)
{
    return TransStatus(Status::OK);
}

int32_t HiPlayerImpl::GetDuration(int32_t& durationMs)
{
    auto tmpMeta = demuxer_->GetGlobalMetaInfo();
    sourceMeta_ = tmpMeta;
    int64_t duration = 0;
    bool found = false;
    if (tmpMeta->GetData(Tag::MEDIA_DURATION, duration)) {
        found = true;
    } else {
        MEDIA_LOG_W("Get media duration failed.");
    }
    streamMeta_.clear();
    int64_t tmp = 0;
    for (auto& streamMeta : demuxer_->GetStreamMetaInfo()) {
        streamMeta_.push_back(streamMeta);
        if (streamMeta->GetData(Tag::MEDIA_DURATION, tmp)) {
            duration = std::max(duration, tmp);
            found = true;
        } else {
            MEDIA_LOG_W("Get media duration failed.");
        }
    }
    if (found) {
        duration_ = duration;
    }
    durationMs = duration_;
    return TransStatus(Status::OK);
}

int32_t HiPlayerImpl::SetPlaybackSpeed(PlaybackRateMode mode)
{
    MEDIA_LOG_I("SetPlaybackSpeed entered.");
    double playbackSpeed = SPEED_1_00_X;
    switch (mode) {
        case SPEED_FORWARD_0_75_X:
            playbackSpeed = SPEED_0_75_X;
        case SPEED_FORWARD_1_00_X:
            playbackSpeed = SPEED_1_00_X;
        case SPEED_FORWARD_1_25_X:
            playbackSpeed = SPEED_1_25_X;
        case SPEED_FORWARD_1_75_X:
            playbackSpeed = SPEED_1_75_X;
        case SPEED_FORWARD_2_00_X:
            playbackSpeed = SPEED_2_00_X;
        default:
            MEDIA_LOG_I("unknown mode:" PUBLIC_LOG_D32 ", return default speed(SPEED_1_00_X)", mode);
    }
    auto meta = std::make_shared<Meta>();
    meta->SetData(Tag::MEDIA_PLAYBACK_SPEED, playbackSpeed);
    demuxer_->SetParameter(meta);
    Format format;
    callbackLooper_.OnInfo(INFO_TYPE_SPEEDDONE, 0, format);

    int32_t currentPosMs = 0;
    int32_t durationMs = 0;
    NZERO_RETURN(GetDuration(durationMs));
    NZERO_RETURN(GetCurrentTime(currentPosMs));
    currentPosMs = std::min(currentPosMs, durationMs);
    currentPosMs = currentPosMs < 0 ? 0 : currentPosMs;
    callbackLooper_.OnInfo(INFO_TYPE_POSITION_UPDATE, currentPosMs, format);
    MEDIA_LOG_D("SetPlaybackSpeed entered end.");
    return MSERR_OK;
}
int32_t HiPlayerImpl::GetPlaybackSpeed(PlaybackRateMode& mode)
{
    MEDIA_LOG_I("GetPlaybackSpeed entered.");
    double rate;
    auto meta = std::make_shared<Meta>();
    demuxer_->GetParameter(meta);
    meta->GetData(Tag::MEDIA_PLAYBACK_SPEED, rate);
    if (abs(rate - SPEED_0_75_X) < EPSINON) {
        return SPEED_FORWARD_0_75_X;
    }
    if (abs(rate - SPEED_1_00_X) < EPSINON) {
        return SPEED_FORWARD_1_00_X;
    }
    if (abs(rate - SPEED_1_25_X) < EPSINON) {
        return SPEED_FORWARD_1_25_X;
    }
    if (abs(rate - SPEED_1_75_X) < EPSINON) {
        return SPEED_FORWARD_1_75_X;
    }
    if (abs(rate - SPEED_2_00_X) < EPSINON) {
        return SPEED_FORWARD_2_00_X;
    }
    return SPEED_FORWARD_1_00_X;
}

int32_t HiPlayerImpl::GetVideoTrackInfo(std::vector<Format>& videoTrack)
{
    MEDIA_LOG_I("GetVideoTrackInfo entered.");
#ifdef VIDEO_SUPPORT
    std::string mime;
    std::vector<std::shared_ptr<Meta>> metaInfo = demuxer_->GetStreamMetaInfo();
    for (const auto& trackInfo : metaInfo) {
        if (!(trackInfo->GetData(Tag::MIME_TYPE, mime))) {
            MEDIA_LOG_W("Get MIME fail");
        }
        if (IsVideoMime(mime)) {
            Format videoTrackInfo {};
            videoTrackInfo.PutStringValue("codec_mime", mime);
            videoTrackInfo.PutIntValue("track_type", MediaType::MEDIA_TYPE_VID);
            uint32_t trackIndex;
            trackInfo->GetData(Tag::TRACK_ID, trackIndex);
            videoTrackInfo.PutIntValue("track_index", static_cast<int32_t>(trackIndex));
            int64_t bitRate;
            trackInfo->GetData(Tag::MEDIA_BITRATE, bitRate);
            videoTrackInfo.PutIntValue("bitrate", static_cast<int32_t>(bitRate));
            uint32_t frameRate;
            trackInfo->GetData(Tag::VIDEO_FRAME_RATE, frameRate);
            videoTrackInfo.PutIntValue("frame_rate", static_cast<int32_t>(frameRate));
            uint32_t height;
            trackInfo->GetData(Tag::VIDEO_HEIGHT, height);
            videoTrackInfo.PutIntValue("height", static_cast<int32_t>(height));
            uint32_t width;
            trackInfo->GetData(Tag::VIDEO_WIDTH, width);
            videoTrackInfo.PutIntValue("width", static_cast<int32_t>(width));
            videoTrack.push_back(videoTrackInfo);
        }
    }
#endif
    return TransStatus(Status::OK);
}

int32_t HiPlayerImpl::GetAudioTrackInfo(std::vector<Format>& audioTrack)
{
    MEDIA_LOG_I("GetAudioTrackInfo entered.");
    std::string mime;
    std::vector<std::shared_ptr<Meta>> metaInfo = demuxer_->GetStreamMetaInfo();
    for (int trackIndex = 0; trackIndex < metaInfo.size(); trackIndex++) {
        auto trackInfo = metaInfo[trackIndex];
        if (!(trackInfo->GetData(Tag::MIME_TYPE, mime))) {
            MEDIA_LOG_W("Get MIME fail");
        }
        if (mime.find("audio/") == 0) {
            Format audioTrackInfo {};
            audioTrackInfo.PutStringValue("codec_mime", mime);
            audioTrackInfo.PutIntValue("track_type", OHOS::Media::MediaType::MEDIA_TYPE_AUD);
            audioTrackInfo.PutIntValue("track_index", static_cast<int32_t>(trackIndex));
            int64_t bitRate;
            trackInfo->GetData(Tag::MEDIA_BITRATE, bitRate);
            audioTrackInfo.PutIntValue("bitrate", static_cast<int32_t>(bitRate));
            int32_t audioChannels;
            trackInfo->GetData(Tag::AUDIO_CHANNEL_COUNT, audioChannels);
            audioTrackInfo.PutIntValue("channel_count", static_cast<int32_t>(audioChannels));
            int32_t audioSampleRate;
            trackInfo->GetData(Tag::AUDIO_SAMPLE_RATE, audioSampleRate);
            audioTrackInfo.PutIntValue("sample_rate", static_cast<int32_t>(audioSampleRate));
            audioTrack.push_back(audioTrackInfo);
        }
    }
    return TransStatus(Status::OK);
}

int32_t HiPlayerImpl::GetVideoWidth()
{
#ifdef VIDEO_SUPPORT
    std::vector<std::shared_ptr<Meta>> metaInfo = demuxer_->GetStreamMetaInfo();
    for (const auto& trackInfo : metaInfo) {
        if !(trackInfo->GetData(Tag::MIME, mime)) {
            MEDIA_LOG_W("Get MIME fail");
        }
        if (IsVideoMime(mime)) {
            uint32_t width;
            trackInfo->GetData(Tag::VIDEO_WIDTH, width);
            videoWidth_ = width;
        }
    }
    MEDIA_LOG_I("GetVideoWidth entered. video width: " PUBLIC_LOG_D32, videoWidth_);
#endif
    return videoWidth_;
}

int32_t HiPlayerImpl::GetVideoHeight()
{
#ifdef VIDEO_SUPPORT
    std::vector<std::shared_ptr<Meta>> metaInfo = demuxer_->GetStreamMetaInfo();
    for (const auto& trackInfo : metaInfo) {
        if !(trackInfo->GetData(Tag::MIME, mime)) {
            MEDIA_LOG_W("Get MIME fail");
        }
        if (IsVideoMime(mime)) {
            uint32_t height;
            trackInfo->GetData(Tag::VIDEO_HEIGHT, height);
            videoHeight_ = height;
        }
    }
    MEDIA_LOG_I("GetVideoHeight entered. video height: " PUBLIC_LOG_D32, videoHeight_);
#endif
    return videoHeight_;
}

int32_t HiPlayerImpl::SetVideoScaleType(OHOS::Media::VideoScaleType videoScaleType)
{
    MEDIA_LOG_I("SetVideoScaleType entered.");
#ifdef VIDEO_SUPPORT
    auto ret = videoSink_->SetParameter(static_cast<int32_t>(Tag::VIDEO_SCALE_TYPE),
        static_cast<Plugin::VideoScaleType>(static_cast<uint32_t>(videoScaleType)));
    return TransStatus(ret);
#else
    return TransStatus(Status::OK);
#endif
}

int32_t HiPlayerImpl::SetAudioRendererInfo(const int32_t contentType, const int32_t streamUsage,
                                           const int32_t rendererFlag)
{
    MEDIA_LOG_I("SetAudioRendererInfo entered.");
    Plugin::AudioRenderInfo audioRenderInfo {contentType, streamUsage, rendererFlag};
    auto meta = std::make_shared<Meta>();
    meta->SetData(Tag::AUDIO_RENDER_INFO, audioRenderInfo);
    audioSink_->SetParameter(meta);
    return TransStatus(Status::OK);
}

int32_t HiPlayerImpl::SetAudioInterruptMode(const int32_t interruptMode)
{
    MEDIA_LOG_I("SetAudioInterruptMode entered.");
    auto meta = std::make_shared<Meta>();
    meta->SetData(Tag::AUDIO_INTERRUPT_MODE, interruptMode);
    audioSink_->SetParameter(meta);
    return TransStatus(Status::OK);
}

int HiPlayerImpl::TransStatus(Status status)
{
    for (const auto& errPair : g_statusPair) {
        if (errPair.first == status) {
            return errPair.second;
        }
    }
    return MSERR_UNKNOWN;
}

void HiPlayerImpl::OnEvent(const Event &event)
{
    switch (event.type) {
        case EventType::EVENT_ERROR: {
            OnStateChanged(PlayerStateId::ERROR);
            break;
        }
        case EventType::EVENT_READY: {
            OnStateChanged(PlayerStateId::READY);
            break;
        }
        case EventType::EVENT_COMPLETE: {
            break;
        }
        default:
            break;
    }
}

Status HiPlayerImpl::DoSetSource(const std::shared_ptr<MediaSource> source)
{
    demuxer_ = FilterFactory::Instance().CreateFilter<DemuxerFilter>("builtin.player.demuxer",
        FilterType::FILTERTYPE_DEMUXER);
    demuxer_->Init(playerEventReceiver_, playerFilterCallback_);
    auto ret = demuxer_->SetDataSource(source);
    pipeline_->AddHeadFilters({demuxer_});
    return ret;
}

Status HiPlayerImpl::Resume()
{
    auto ret = pipeline_->Resume();
    return ret;
}

void HiPlayerImpl::OnStateChanged(PlayerStateId state)
{
    curState_ = state;
}

void HiPlayerImpl::OnCallback(std::shared_ptr<Filter> filter, const FilterCallBackCommand cmd, StreamType outType)
{
    MEDIA_LOG_I("HiPlayerImpl::OnCallback filter, ");
    if (cmd == FilterCallBackCommand::NEXT_FILTER_NEEDED) {
        switch (outType) {
            case StreamType::STREAMTYPE_RAW_AUDIO:
                LinkAudioSinkFilter(filter, outType);
                break;
            case StreamType::STREAMTYPE_ENCODED_AUDIO:
                LinkAudioDecoderFilter(filter, outType);
                break;
#ifdef VIDEO_SUPPORT
            case StreamType::STREAMTYPE_RAW_VIDEO:
                LinkVideoSinkFilter(filter, outType);
                break;
            case StreamType::STREAMTYPE_ENCODED_VIDEO:
                LinkVideoDecoderFilter(filter, outType);
                break;
#endif
            default:
                break;
        }
    }
}

Status HiPlayerImpl::LinkAudioDecoderFilter(const std::shared_ptr<Filter>& preFilter, StreamType type)
{
    MEDIA_LOG_I("HiPlayerImpl::LinkAudioDecoderFilter");
    if (audioDecoder_ == nullptr) {
        audioDecoder_ = FilterFactory::Instance().CreateFilter<CodecFilter>("player.audiodecoder",
            FilterType::FILTERTYPE_ADEC);
        audioDecoder_->Init(playerEventReceiver_, playerFilterCallback_);
    }
    return pipeline_->LinkFilters(preFilter, {audioDecoder_}, type);
}

Status HiPlayerImpl::LinkAudioSinkFilter(const std::shared_ptr<Filter>& preFilter, StreamType type)
{
    MEDIA_LOG_I("HiPlayerImpl::LinkAudioSinkFilter");
    if (audioSink_ == nullptr) {
        audioSink_ = FilterFactory::Instance().CreateFilter<AudioSinkFilter>("player.audiosink",
            FilterType::FILTERTYPE_ASINK);
        audioSink_->Init(playerEventReceiver_, playerFilterCallback_);
    }
    return pipeline_->LinkFilters(preFilter, {audioSink_}, type);
}
#ifdef VIDEO_SUPPORT
Status HiPlayerImpl::LinkVideoDecoderFilter(const std::shared_ptr<Filter>& preFilter, StreamType type)
{
    MEDIA_LOG_I("HiPlayerImpl::LinkVideoDecoderFilter");
    if (videoDecoder_ == nullptr) {
        videoDecoder_ = FilterFactory::Instance().CreateFilter<CodecFilter>("player.videodecoder",
            FilterType::FILTERTYPE_VDEC);
        videoDecoder_->Init(playerEventReceiver_, playerFilterCallback_);
    }
    return pipeline_->LinkFilters(preFilter, {videoDecoder_}, type);
}

Status HiPlayerImpl::LinkVideoSinkFilter(const std::shared_ptr<Filter>& preFilter, StreamType type)
{
    MEDIA_LOG_I("HiPlayerImpl::LinkVideoSinkFilter");
    if (videoSink_ == nullptr) {
        videoSink_ = FilterFactory::Instance().CreateFilter<VideoSinkFilter>("player.videosink",
            FilterType::FILTERTYPE_VSINK);
        videoSink_->Init(playerEventReceiver_, playerFilterCallback_);
    }
    return pipeline_->LinkFilters(preFilter, {videoSink_}, type);
}
#endif
}  // namespace Media
}  // namespace OHOS