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
#define SUPPORT_VIDEO

#include "hiplayer_impl.h"

#include "audio_info.h"
#include "common/log.h"
#include "common/media_source.h"
#include "filter/filter_factory.h"
#include "media_errors.h"
#include "osal/utils/dump_buffer.h"
#include "common/plugin_time.h"
#include "media_utils.h"

namespace {
const float MAX_MEDIA_VOLUME = 1.0f; // standard interface volume is between 0 to 1.
}

namespace OHOS {
namespace Media {
using namespace Pipeline;
constexpr double EPSINON = 0.0001;

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
    syncManager_ = std::make_shared<MediaSyncManager>();
    callbackLooper_.SetPlayEngine(this);
}

HiPlayerImpl::~HiPlayerImpl()
{
    MEDIA_LOG_I("dtor called.");
    pipeline_->Stop();
    audioSink_.reset();
#ifdef SUPPORT_VIDEO
    videoDecoder_.reset();
#endif
    syncManager_.reset();
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
        MEDIA_LOG_I("Prepare pipeline prepare ret " PUBLIC_LOG_D32, ret);
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
        MEDIA_LOG_I("Prepare not ready");
        ret = Status::ERROR_UNKNOWN;
    }
    MEDIA_LOG_I("Prepare End");
    return TransStatus(ret);
}

int HiPlayerImpl::PrepareAsync()
{
    MEDIA_LOG_I("PrepareAsync Start");
    if (!(pipelineStates_ == PlayerStates::PLAYER_INITIALIZED || pipelineStates_ == PlayerStates::PLAYER_STOPPED)) {
        return MSERR_INVALID_OPERATION;
    }
    if (pipelineStates_ == PlayerStates::PLAYER_STOPPED) {
        SetSource(url_);
    }
    NotifyBufferingUpdate(PlayerKeys::PLAYER_BUFFERING_START, 0);
    MEDIA_LOG_I("PrepareAsync entered, current pipeline state: " PUBLIC_LOG_S,
        StringnessPlayerState(pipelineStates_).c_str());
    OnStateChanged(PlayerStateId::PREPARING);
    auto ret = pipeline_->Prepare();
    if (ret != Status::OK) {
        MEDIA_LOG_E("PrepareAsync failed with error " PUBLIC_LOG_D32, ret);
        return TransStatus(ret);
    }
    NotifyBufferingUpdate(PlayerKeys::PLAYER_BUFFERING_END, 0);
    int32_t durationMs = 0;
    GetDuration(durationMs);
    NotifyDurationUpdate(PlayerKeys::PLAYER_CACHED_DURATION, 0);
    OnStateChanged(PlayerStateId::READY);
    MEDIA_LOG_I("PrepareAsync End, resource duration " PUBLIC_LOG_D32, durationMs);
    return TransStatus(ret);
}

int32_t HiPlayerImpl::Play()
{
    MEDIA_LOG_I("Play entered.");
    int32_t ret = MSERR_INVALID_VAL;
    callbackLooper_.StartReportMediaProgress(100); // 100 ms
    if (pipelineStates_ == PlayerStates::PLAYER_PLAYBACK_COMPLETE) {
        ret = Seek(0, PlayerSeekMode::SEEK_PREVIOUS_SYNC);
    } else if (pipelineStates_ == PlayerStates::PLAYER_PAUSED) {
        ret = TransStatus(Resume());
    } else {
        syncManager_->Resume();
        ret = TransStatus(pipeline_->Start());
        if (ret != MSERR_OK) {
            UpdateStateNoLock(PlayerStates::PLAYER_STATE_ERROR);
        }
    }
    if (ret == MSERR_OK) {
        OnStateChanged(PlayerStateId::PLAYING);
    }
    return ret;
}

int32_t HiPlayerImpl::Pause()
{
    MEDIA_LOG_I("Pause entered.");
    auto ret = pipeline_->Pause();
    syncManager_->Pause();
    if (ret != Status::OK) {
        UpdateStateNoLock(PlayerStates::PLAYER_STATE_ERROR);
    }
    callbackLooper_.StopReportMediaProgress();
    callbackLooper_.ManualReportMediaProgressOnce();
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
    auto ret = Status::ERROR_UNKNOWN;
    if (pipeline_ != nullptr) {
        ret = pipeline_->Stop();
    }
    OnStateChanged(PlayerStateId::STOPPED);
    return TransStatus(ret);
}

int32_t HiPlayerImpl::Reset()
{
    if (pipelineStates_ == PlayerStates::PLAYER_STOPPED) {
        return TransStatus(Status::OK);
    }
    singleLoop_ = false;
    auto ret = Stop();
    syncManager_->Reset();
    OnStateChanged(PlayerStateId::STOPPED);
    return ret;
}

int32_t HiPlayerImpl::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    MEDIA_LOG_I("Seek entered. mSeconds : " PUBLIC_LOG_D32 ", seekMode : " PUBLIC_LOG_D32,
                mSeconds, static_cast<int32_t>(mode));
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
    int64_t seekPos = mSeconds;
    auto seekMode = Transform2SeekMode(mode);
    auto rtv = seekPos >= 0 ? Status::OK : Status::ERROR_INVALID_PARAMETER;
    if (rtv == Status::OK) {
        if (pipelineStates_ == PlayerStates::PLAYER_STARTED) {
            pipeline_->Pause();
            pipeline_->Flush();
        }
        MEDIA_LOG_I("Do seek ...");
        int64_t realSeekTime = seekPos;
        rtv = demuxer_->SeekTo(seekPos, seekMode, realSeekTime);
        if (rtv == Status::OK) {
            syncManager_->Seek(realSeekTime);
        }
        if (pipelineStates_ == PLAYER_STARTED) {
            pipeline_->Start();
        }
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
    if (ret != Status::OK) {
        MEDIA_LOG_E("SetVolume failed with error " PUBLIC_LOG_D32, static_cast<int>(ret));
    }
    return TransStatus(ret);
}

int32_t HiPlayerImpl::SetVideoSurface(sptr<Surface> surface)
{
    MEDIA_LOG_D("SetVideoSurface entered.");
#ifdef SUPPORT_VIDEO
    FALSE_RETURN_V_MSG_E(surface != nullptr, (int32_t)(Status::ERROR_INVALID_PARAMETER),
                         "Set video surface failed, surface == nullptr");
    if (videoDecoder_ != nullptr) {
        return TransStatus(videoDecoder_->SetVideoSurface(surface));
    }
    surface_ = surface;
#endif
    return TransStatus(Status::OK);
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
#ifdef SUPPORT_VIDEO
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

int32_t HiPlayerImpl::SetObsForHst(const std::shared_ptr<IPlayerEngineObs>& obs)
{
    MEDIA_LOG_I("SetObs entered.");
    callbackLooper_.StartWithPlayerEngineObs(obs);
    return TransStatus(Status::OK);
}

int32_t HiPlayerImpl::SetObs(const std::weak_ptr<IPlayerEngineObs>& obs)
{
    return 0;
}

int32_t HiPlayerImpl::GetCurrentTime(int32_t& currentPositionMs)
{
    FALSE_RETURN_V(syncManager_ != nullptr, TransStatus(Status::ERROR_NULL_POINTER));
    currentPositionMs = Plugins::HstTime2Us(syncManager_->GetMediaTimeNow());
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
    if (found) {
        duration_ = Plugins::HstTime2Us(duration);
    }
    int64_t tmp = 0;
    duration = std::max(duration, tmp);
    durationMs = duration_;
    MEDIA_LOG_W("Get media duration " PUBLIC_LOG_D32, durationMs);
    return TransStatus(Status::OK);
}

int32_t HiPlayerImpl::SetPlaybackSpeed(PlaybackRateMode mode)
{
    MEDIA_LOG_I("SetPlaybackSpeed entered.");
    double playbackSpeed = ChangeModeToSpeed(mode);
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

bool HiPlayerImpl::IsVideoMime(const std::string& mime)
{
    return mime.find("video/") == 0;
}

int32_t HiPlayerImpl::GetVideoTrackInfo(std::vector<Format>& videoTrack)
{
    MEDIA_LOG_I("GetVideoTrackInfo entered.");
#ifdef SUPPORT_VIDEO
    std::string mime;
    std::vector<std::shared_ptr<Meta>> metaInfo = demuxer_->GetStreamMetaInfo();
    for (const auto& trackInfo : metaInfo) {
        if (!(trackInfo->GetData(Tag::MIME_TYPE, mime))) {
            MEDIA_LOG_W("Get MIME fail");
            continue;
        }
        if (IsVideoMime(mime)) {
            Format videoTrackInfo {};
            videoTrackInfo.PutStringValue("codec_mime", mime);
            videoTrackInfo.PutIntValue("track_type", static_cast<int32_t>(MediaType::VIDEO));
            uint32_t trackIndex;
            trackInfo->GetData(Tag::REGULAR_TRACK_ID, trackIndex);
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
            videoTrack.emplace_back(std::move(videoTrackInfo));
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
    for (size_t trackIndex = 0; trackIndex < metaInfo.size(); trackIndex++) {
        auto trackInfo = metaInfo[trackIndex];
        if (!(trackInfo->GetData(Tag::MIME_TYPE, mime))) {
            MEDIA_LOG_W("Get MIME fail");
            continue;
        }
        if (mime.find("audio/") == 0) {
            Format audioTrackInfo {};
            audioTrackInfo.PutStringValue("codec_mime", mime);
            audioTrackInfo.PutIntValue("track_type", static_cast<int32_t>(MediaType::AUDIO));
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
            audioTrack.emplace_back(std::move(audioTrackInfo));
        }
    }
    return TransStatus(Status::OK);
}

int32_t HiPlayerImpl::GetVideoWidth()
{
#ifdef SUPPORT_VIDEO
    std::vector<std::shared_ptr<Meta>> metaInfo = demuxer_->GetStreamMetaInfo();
    for (const auto& trackInfo : metaInfo) {
        std::string mime;
        if (!trackInfo->GetData(Tag::MIME_TYPE, mime)) {
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
#ifdef SUPPORT_VIDEO
    std::vector<std::shared_ptr<Meta>> metaInfo = demuxer_->GetStreamMetaInfo();
    for (const auto& trackInfo : metaInfo) {
        std::string mime;
        if (!trackInfo->GetData(Tag::MIME_TYPE, mime)) {
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
#ifdef SUPPORT_VIDEO
    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::VIDEO_SCALE_TYPE>(static_cast<int32_t>(videoScaleType));
    videoDecoder_->SetParameter(meta);
    return TransStatus(Status::OK);
#else
    return TransStatus(Status::OK);
#endif
}

int32_t HiPlayerImpl::SetAudioRendererInfo(const int32_t contentType, const int32_t streamUsage,
                                           const int32_t rendererFlag)
{
    MEDIA_LOG_I("SetAudioRendererInfo entered.");
    Plugins::AudioRenderInfo audioRenderInfo {contentType, streamUsage, rendererFlag};
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
            HandleCompleteEvent(event);
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
    syncManager_->Resume();
    auto ret = pipeline_->Resume();
    if (ret != Status::OK) {
        UpdateStateNoLock(PlayerStates::PLAYER_STATE_ERROR);
    }
    return ret;
}

void HiPlayerImpl::HandleCompleteEvent(const Event& event)
{
    MEDIA_LOG_I("OnComplete looping: " PUBLIC_LOG_D32 ".", singleLoop_.load());
    Format format;
    if (singleLoop_.load()) { // PlayerServer will judge and seek
        callbackLooper_.OnInfo(INFO_TYPE_EOS, static_cast<int32_t>(singleLoop_.load()), format);
    } else {
        OnStateChanged(PlayerStateId::EOS);
        callbackLooper_.StopReportMediaProgress();
        callbackLooper_.ManualReportMediaProgressOnce();
    }
}

void HiPlayerImpl::UpdateStateNoLock(PlayerStates newState, bool notifyUpward)
{
    if (pipelineStates_ == newState) {
        return;
    }
    pipelineStates_ = newState;
    if (pipelineStates_ == PlayerStates::PLAYER_IDLE || pipelineStates_ == PlayerStates::PLAYER_PREPARING) {
        MEDIA_LOG_W("do not report idle and preparing since av player doesn't need report idle and preparing");
        return;
    }
    if (notifyUpward) {
        if (callbackLooper_.IsStarted()) {
            Format format;
            while (!pendingStates_.empty()) {
                auto pendingState = pendingStates_.front();
                pendingStates_.pop();
                MEDIA_LOG_I("sending pending state change: " PUBLIC_LOG_S, StringnessPlayerState(pendingState).c_str());
                callbackLooper_.OnInfo(INFO_TYPE_STATE_CHANGE, pendingState, format);
            }
            MEDIA_LOG_I("sending newest state change: " PUBLIC_LOG_S,
                    StringnessPlayerState(pipelineStates_.load()).c_str());
            callbackLooper_.OnInfo(INFO_TYPE_STATE_CHANGE, pipelineStates_, format);
        } else {
            pendingStates_.push(newState);
        }
    }
}

double HiPlayerImpl::ChangeModeToSpeed(const PlaybackRateMode &mode) const
{
    switch (mode) {
        case SPEED_FORWARD_0_75_X:
            return SPEED_0_75_X;
        case SPEED_FORWARD_1_00_X:
            return SPEED_1_00_X;
        case SPEED_FORWARD_1_25_X:
            return SPEED_1_25_X;
        case SPEED_FORWARD_1_75_X:
            return SPEED_1_75_X;
        case SPEED_FORWARD_2_00_X:
            return SPEED_2_00_X;
        default:
            MEDIA_LOG_W("unknown mode: " PUBLIC_LOG_D32 ", return default speed(SPEED_1_00_X)", mode);
    }

    return SPEED_1_00_X;
}

void HiPlayerImpl::NotifyBufferingUpdate(const std::string_view& type, int32_t param)
{
    Format format;
    format.PutIntValue(std::string(type), param);
    callbackLooper_.OnInfo(INFO_TYPE_BUFFERING_UPDATE, duration_, format);
}

void HiPlayerImpl::NotifyDurationUpdate(const std::string_view& type, int32_t param)
{
    Format format;
    format.PutIntValue(std::string(type), param);
    callbackLooper_.OnInfo(INFO_TYPE_DURATION_UPDATE, duration_, format);
}

void HiPlayerImpl::NotifySeekDone(int32_t status)
{
    Format format;
    callbackLooper_.OnInfo(INFO_TYPE_SEEKDONE, status, format);
}

void HiPlayerImpl::OnStateChanged(PlayerStateId state)
{
    curState_ = state;
    MEDIA_LOG_I("OnStateChanged from " PUBLIC_LOG_D32 " to " PUBLIC_LOG_D32, pipelineStates_.load(),
            TransStateId2PlayerState(state));
    UpdateStateNoLock(TransStateId2PlayerState(state));
    {
        AutoLock lock(stateMutex_);
        cond_.NotifyOne();
    }
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
#ifdef SUPPORT_VIDEO
            case StreamType::STREAMTYPE_RAW_VIDEO:
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
        audioDecoder_ = FilterFactory::Instance().CreateFilter<AudioDecoderFilter>("player.audiodecoder",
            FilterType::FILTERTYPE_ADEC);
        FALSE_RETURN_V(audioDecoder_ != nullptr, Status::ERROR_NULL_POINTER);
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
        FALSE_RETURN_V(audioSink_ != nullptr, Status::ERROR_NULL_POINTER);
        audioSink_->Init(playerEventReceiver_, playerFilterCallback_);
        audioSink_->SetSyncCenter(syncManager_);
    }
    return pipeline_->LinkFilters(preFilter, {audioSink_}, type);
}
#ifdef SUPPORT_VIDEO
Status HiPlayerImpl::LinkVideoDecoderFilter(const std::shared_ptr<Filter>& preFilter, StreamType type)
{
    MEDIA_LOG_I("HiPlayerImpl::LinkVideoDecoderFilter");
    if (videoDecoder_ == nullptr) {
        videoDecoder_ = FilterFactory::Instance().CreateFilter<DecoderSurfaceFilter>("player.videodecoder",
            FilterType::FILTERTYPE_VDEC);
        FALSE_RETURN_V(videoDecoder_ != nullptr, Status::ERROR_NULL_POINTER);
        videoDecoder_->Init(playerEventReceiver_, playerFilterCallback_);
        videoDecoder_->SetSyncCenter(syncManager_);
        if (surface_ != nullptr) {
            videoDecoder_->SetVideoSurface(surface_);
        }
    }
    return pipeline_->LinkFilters(preFilter, {videoDecoder_}, type);
}
#endif
}  // namespace Media
}  // namespace OHOS