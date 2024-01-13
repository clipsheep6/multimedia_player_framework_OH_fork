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
#include "common/log.h"
#include "common/media_source.h"
#include "filter/filter_factory.h"
#include "media_errors.h"
#include "osal/utils/dump_buffer.h"
#include "common/plugin_time.h"
#include "media_utils.h"
#include "meta/media_types.h"

namespace {
constexpr uint32_t INTERRUPT_EVENT_SHIFT = 8;
const float MAX_MEDIA_VOLUME = 1.0f; // standard interface volume is between 0 to 1.
const float MIN_MEDIA_VOLUME = 0.0f; // standard interface volume is between 0 to 1.
const int32_t FADE_OUT_LATENCY = 0; // fade out latency ms
const int32_t FRAME_RATE_UNIT_MULTIPLE = 100; // the unit of frame rate is frames per 100s
}

namespace OHOS {
namespace Media {
using namespace Pipeline;

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
    if (videoDecoder_) {
        videoDecoder_.reset();
    }
#endif
    syncManager_.reset();
    if (demuxer_) {
        pipeline_->RemoveHeadFilter(demuxer_);
    }
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
        if (url_.find("http") == 0 || url_.find("https") == 0) {
            isNetWorkPlay_ = true;
        }
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
    NotifyDurationUpdate(PlayerKeys::PLAYER_CACHED_DURATION, durationMs);
    NotifyResolutionChange();
    NotifyPositionUpdate();
    DoInitializeForHttp();
    OnStateChanged(PlayerStateId::READY);
    MEDIA_LOG_I("PrepareAsync End, resource duration " PUBLIC_LOG_D32, durationMs);
    return TransStatus(ret);
}

int32_t HiPlayerImpl::SelectBitRate(uint32_t bitRate)
{
    if (demuxer_ == nullptr) {
        MEDIA_LOG_E("SelectBitRate failed, demuxer_ is null");
    }
    Status ret = demuxer_->SelectBitRate(bitRate);
    if (ret == Status::OK) {
        Format bitRateFormat;
        callbackLooper_.OnInfo(INFO_TYPE_BITRATEDONE, bitRate, bitRateFormat);
        MEDIA_LOG_I("SelectBitRate success");
        return MSERR_OK;
    }
    MEDIA_LOG_I("SelectBitRate failed");
    return MSERR_INVALID_OPERATION;
}

void HiPlayerImpl::DoInitializeForHttp()
{
    if (!isNetWorkPlay_) {
        return;
    }
    std::vector<uint32_t> vBitRates;
    MEDIA_LOG_I("DoInitializeForHttp");
    auto ret = demuxer_->GetBitRates(vBitRates);
    if (ret == Status::OK && vBitRates.size() > 0) {
        int mSize = vBitRates.size();
        const int size = mSize;
        uint32_t* bitrates = vBitRates.data();
        Format bitRateFormat;
        (void)bitRateFormat.PutBuffer(std::string(PlayerKeys::PLAYER_BITRATE),
            static_cast<uint8_t *>(static_cast<void *>(bitrates)), size * sizeof(uint32_t));
        callbackLooper_.OnInfo(INFO_TYPE_BITRATE_COLLECT, 0, bitRateFormat);
        MEDIA_LOG_I("OnInfo INFO_TYPE_BITRATE_COLLEC");
    } else {
        MEDIA_LOG_E("GetBitRates failed, ret %{public}d", ret);
    }
}

int32_t HiPlayerImpl::Play()
{
    MEDIA_LOG_I("Play entered.");
    int32_t ret = MSERR_INVALID_VAL;
    callbackLooper_.StartReportMediaProgress(100); // 100 ms
    if (pipelineStates_ == PlayerStates::PLAYER_PLAYBACK_COMPLETE) {
        isStreaming_ = true;
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
    if (audioSink_ != nullptr) {
        audioSink_->SetVolumeWithRamp(MIN_MEDIA_VOLUME, FADE_OUT_LATENCY);
    }
    auto ret = pipeline_->Pause();
    syncManager_->Pause();
    if (audioSink_ != nullptr) {
        audioSink_->Pause();
    }
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
    if (audioSink_ != nullptr) {
        audioSink_->SetVolumeWithRamp(MIN_MEDIA_VOLUME, FADE_OUT_LATENCY);
    }
    // close demuxer first to avoid concurrent problem
    if (demuxer_ != nullptr) {
        demuxer_->Stop();
    }
    auto ret = Status::ERROR_UNKNOWN;
    if (pipeline_ != nullptr) {
        ret = pipeline_->Stop();
    }
    if (audioDecoder_ != nullptr) {
        audioDecoder_->Flush();
    }
    if (audioSink_ != nullptr) {
        audioSink_->Flush();
    }

    // triger drm waiting condition
    if (isDrmProtected_) {
        std::unique_lock<std::mutex> drmLock(drmMutex_);
        stopWaitingDrmConfig_ = true;
        drmConfigCond_.notify_all();
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

Status HiPlayerImpl::SeekInner(int64_t seekPos, PlayerSeekMode mode)
{
    auto seekMode = Transform2SeekMode(mode);
    if (pipelineStates_ == PlayerStates::PLAYER_STARTED) {
        if (audioSink_ != nullptr) {
            audioSink_->SetVolumeWithRamp(MIN_MEDIA_VOLUME, FADE_OUT_LATENCY);
        }
        pipeline_->Pause();
        if (audioDecoder_ != nullptr) {
            audioDecoder_->Flush();
            audioDecoder_->Start();
        }
        if (audioSink_ != nullptr) {
            audioSink_->Pause();
            audioSink_->Flush();
        }
    } else if (pipelineStates_ == PlayerStates::PLAYER_PLAYBACK_COMPLETE) {
        pipeline_->Pause();
        if (audioSink_ != nullptr) {
            audioSink_->Pause();
            audioSink_->Flush();
        }
    } else if (pipelineStates_ == PlayerStates::PLAYER_PAUSED) {
        if (audioDecoder_ != nullptr) {
            audioDecoder_->Flush();
        }
        if (audioSink_ != nullptr) {
            audioSink_->Flush();
        }
    }
    MEDIA_LOG_I("Do seek ...");
    int64_t realSeekTime = seekPos;
    auto rtv = demuxer_->SeekTo(seekPos, seekMode, realSeekTime);
    if (rtv == Status::OK) {
        syncManager_->Seek(Plugins::HstTime2Us(realSeekTime));
    }
    std::promise<bool> videoSeekSuccess;
    std::future<bool> videoSeekFuture = videoSeekSuccess.get_future();
    if (audioDecoder_) {
        audioDecoder_->SeekTo(Plugins::HstTime2Us(realSeekTime), std::move(videoSeekFuture));
    }
    if (videoDecoder_) {
        videoDecoder_->SeekTo(Plugins::HstTime2Us(realSeekTime), std::move(videoSeekSuccess));
    } else {
        videoSeekSuccess.set_value(true);
    }
    if (pipelineStates_ == PlayerStates::PLAYER_STARTED) {
        pipeline_->Resume();
        if (audioSink_ != nullptr) {
            audioSink_->Resume();
        }
    }
    if (pipelineStates_ == PlayerStates::PLAYER_PLAYBACK_COMPLETE && isStreaming_) {
        pipeline_->Resume();
    } else if (pipelineStates_ == PlayerStates::PLAYER_PLAYBACK_COMPLETE && !isStreaming_) {
        callbackLooper_.StopReportMediaProgress();
        callbackLooper_.ManualReportMediaProgressOnce();
        OnStateChanged(PlayerStateId::PAUSE);
    }
    return rtv;
}

int32_t HiPlayerImpl::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    MEDIA_LOG_I("Seek entered. mSeconds : " PUBLIC_LOG_D32 ", seekMode : " PUBLIC_LOG_D32,
                mSeconds, static_cast<int32_t>(mode));
    isSeek_ = true;
    int32_t durationMs = 0;
    GetDuration(durationMs);
    FALSE_RETURN_V_MSG_E(durationMs > 0, (int32_t) Status::ERROR_INVALID_PARAMETER,
        "Seek, invalid operation, source is unseekable or invalid");
    MEDIA_LOG_D("Seek durationMs : " PUBLIC_LOG_D32, durationMs);
    if (mSeconds >= durationMs) { // if exceeds change to duration
        mSeconds = durationMs;
    }
    mSeconds = mSeconds < 0 ? 0 : mSeconds;
    int64_t seekPos = mSeconds;
    auto rtv = seekPos >= 0 ? Status::OK : Status::ERROR_INVALID_PARAMETER;
    if (rtv == Status::OK) {
        rtv = SeekInner(seekPos, mode);
    }
    NotifySeekDone(seekPos);
    if (rtv != Status::OK) {
        MEDIA_LOG_E("Seek done, seek error.");
    }
    isSeek_ = false;
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

int32_t HiPlayerImpl::SetDecryptConfig(const sptr<OHOS::DrmStandard::IMediaKeySessionService> &keySessionProxy,
    bool svp)
{
    MEDIA_LOG_I("SetDecryptConfig entered.");
#ifdef SUPPORT_DRM
    FALSE_RETURN_V_MSG_E(keySessionProxy != nullptr, (int32_t)(Status::ERROR_INVALID_PARAMETER),
        "SetDecryptConfig failed, keySessionProxy == nullptr");
    keySessionServiceProxy_ = keySessionProxy;
    if (svp) {
        svpMode_ = HiplayerSvpMode::SVP_TRUE;
    } else {
        svpMode_ = HiplayerSvpMode::SVP_FALSE;
    }

    std::unique_lock<std::mutex> drmLock(drmMutex_);
    MEDIA_LOG_I("For Drmcond SetDecryptConfig will trig drmPreparedCond");
    isDrmPrepared_ = true;
    drmConfigCond_.notify_all();
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

int32_t HiPlayerImpl::SetObs(const std::weak_ptr<IPlayerEngineObs>& obs)
{
    MEDIA_LOG_I("SetObs entered.");
    callbackLooper_.StartWithPlayerEngineObs(obs);
    return TransStatus(Status::OK);
}

int32_t HiPlayerImpl::GetCurrentTime(int32_t& currentPositionMs)
{
    if (curState_ == PlayerStateId::EOS) {
        GetDuration(currentPositionMs);
        return TransStatus(Status::OK);
    }
    if (isSeek_.load()) {
        return TransStatus(Status::ERROR_UNKNOWN);
    }
    FALSE_RETURN_V(syncManager_ != nullptr, TransStatus(Status::ERROR_NULL_POINTER));
    currentPositionMs = Plugins::HstTime2Us(syncManager_->GetMediaTimeNow());
    if (currentPositionMs < 0) {
        currentPositionMs = 0;
    }
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
    if (found && duration > 0 && duration != duration_) {
        duration_ = Plugins::HstTime2Us(duration);
    }
    int64_t tmp = 0;
    durationMs = std::max(duration_, tmp);
    MEDIA_LOG_W("Get media duration " PUBLIC_LOG_D32, durationMs);
    return TransStatus(Status::OK);
}

int32_t HiPlayerImpl::SetPlaybackSpeed(PlaybackRateMode mode)
{
    MEDIA_LOG_I("SetPlaybackSpeed entered.");
    if (mode == playbackRateMode_.load()) {
        MEDIA_LOG_I("SetPlaybackSpeed new mode same as the old.");
        return MSERR_OK;
    }
    Status res = Status::OK;
    float speed = TransformPlayRate2Float(mode);
    if (audioSink_ != nullptr) {
        res = audioSink_->SetSpeed(speed);
    }
    if (res != Status::OK) {
        MEDIA_LOG_E("SetPlaybackSpeed audioSink set speed  error.");
        return MSERR_UNKNOWN;
    }
    if (syncManager_ != nullptr) {
        res = syncManager_->SetPlaybackRate(speed);
    }
    if (res != Status::OK) {
        MEDIA_LOG_E("SetPlaybackSpeed syncManager set audio speed error.");
        return MSERR_UNKNOWN;
    }
    playbackRateMode_ = mode;
    Format format;
    callbackLooper_.OnInfo(INFO_TYPE_SPEEDDONE, mode, format);
    MEDIA_LOG_I("SetPlaybackSpeed entered end.");
    return MSERR_OK;
}

int32_t HiPlayerImpl::GetPlaybackSpeed(PlaybackRateMode& mode)
{
    MEDIA_LOG_I("GetPlaybackSpeed entered.");
    mode = playbackRateMode_.load();
    MEDIA_LOG_I("GetPlaybackSpeed end, mode is " PUBLIC_LOG_D32, mode);
    return MSERR_OK;
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
            videoTrackInfo.PutIntValue("track_type", static_cast<int32_t>(OHOS::Media::Plugins::MediaType::VIDEO));
            int32_t trackIndex;
            trackInfo->GetData(Tag::REGULAR_TRACK_ID, trackIndex);
            videoTrackInfo.PutIntValue("track_index", static_cast<int32_t>(trackIndex));
            int64_t bitRate;
            trackInfo->GetData(Tag::MEDIA_BITRATE, bitRate);
            videoTrackInfo.PutIntValue("bitrate", static_cast<int32_t>(bitRate));
            double frameRate;
            trackInfo->GetData(Tag::VIDEO_FRAME_RATE, frameRate);
            videoTrackInfo.PutIntValue("frame_rate", static_cast<int32_t>(frameRate * FRAME_RATE_UNIT_MULTIPLE));
            int32_t height;
            trackInfo->GetData(Tag::VIDEO_HEIGHT, height);
            videoTrackInfo.PutIntValue("height", static_cast<int32_t>(height));
            int32_t width;
            trackInfo->GetData(Tag::VIDEO_WIDTH, width);
            videoTrackInfo.PutIntValue("width", static_cast<int32_t>(width));
            Plugins::VideoRotation rotation;
            trackInfo->Get<Tag::VIDEO_ROTATION>(rotation);
            videoTrackInfo.PutIntValue(Tag::VIDEO_ROTATION, rotation);
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
            audioTrackInfo.PutIntValue("track_type", static_cast<int32_t>(OHOS::Media::Plugins::MediaType::AUDIO));
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
    if (videoDecoder_) {
        videoDecoder_->SetParameter(meta);
    }
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
    audioRenderInfo_ = std::make_shared<Meta>();
    audioRenderInfo_->SetData(Tag::AUDIO_RENDER_INFO, audioRenderInfo);
    if (audioSink_ != nullptr) {
        audioSink_->SetParameter(audioRenderInfo_);
    }
    return TransStatus(Status::OK);
}

int32_t HiPlayerImpl::SetAudioInterruptMode(const int32_t interruptMode)
{
    MEDIA_LOG_I("SetAudioInterruptMode entered.");
    audioInterruptMode_ = std::make_shared<Meta>();
    audioInterruptMode_->SetData(Tag::AUDIO_INTERRUPT_MODE, interruptMode);
    if (audioSink_ != nullptr) {
        audioSink_->SetParameter(audioInterruptMode_);
    }
    return TransStatus(Status::OK);
}

void HiPlayerImpl::OnEvent(const Event &event)
{
    switch (event.type) {
        case EventType::EVENT_IS_LIVE_STREAM: {
            HandleIsLiveStreamEvent(AnyCast<bool>(event.param));
            break;
        }
        case EventType::EVENT_ERROR: {
            OnStateChanged(PlayerStateId::ERROR);
            HandleErrorEvent(AnyCast<int32_t>(event.param));
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
        case EventType::EVENT_AUDIO_INTERRUPT: {
            NotifyAudioInterrupt(event);
            break;
        }
        case EventType::EVENT_AUDIO_FIRST_FRAME: {
            NotifyAudioFirstFrame(event);
            break;
        }
        case EventType::EVENT_DRM_INFO_UPDATED: {
            HandleDrmInfoUpdatedEvent(event);
            break;
        }
        case EventType::EVENT_VIDEO_RENDERING_START: {
            Format format;
            callbackLooper_.OnInfo(INFO_TYPE_MESSAGE, PlayerMessageType::PLAYER_INFO_VIDEO_RENDERING_START, format);
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
    if (audioSink_ != nullptr) {
        audioSink_->Resume();
    }
    if (ret != Status::OK) {
        UpdateStateNoLock(PlayerStates::PLAYER_STATE_ERROR);
    }
    return ret;
}

void HiPlayerImpl::HandleIsLiveStreamEvent(bool isLiveStream)
{
    Format format;
    callbackLooper_.OnInfo(INFO_TYPE_IS_LIVE_STREAM, isLiveStream, format);
}

void HiPlayerImpl::HandleErrorEvent(int32_t errorCode)
{
    Format format;
    callbackLooper_.OnInfo(INFO_TYPE_ERROR_MSG, errorCode, format);
}

void HiPlayerImpl::HandleCompleteEvent(const Event& event)
{
    bool isSingleLoop = singleLoop_.load();
    MEDIA_LOG_I("OnComplete looping: " PUBLIC_LOG_D32 ".", isSingleLoop);
    isStreaming_ = false;
    Format format;

    if (!isSingleLoop) {
        OnStateChanged(PlayerStateId::EOS);
        callbackLooper_.StopReportMediaProgress();
    }
    callbackLooper_.DoReportCompletedTime();
    callbackLooper_.OnInfo(INFO_TYPE_EOS, static_cast<int32_t>(isSingleLoop), format);
}

void HiPlayerImpl::HandleDrmInfoUpdatedEvent(const Event& event)
{
    MEDIA_LOG_I("HandleDrmInfoUpdatedEvent");

    std::multimap<std::string, std::vector<uint8_t>> drmInfo =
        AnyCast<std::multimap<std::string, std::vector<uint8_t>>>(event.param);
    int32_t infoCount = drmInfo.size();
    if (infoCount > DrmConstant::DRM_MAX_DRM_INFO_COUNT || infoCount <= 0) {
        MEDIA_LOG_E("HandleDrmInfoUpdatedEvent info count is invalid");
        return;
    }
    DrmInfoItem *drmInfoArray = new DrmInfoItem[infoCount];
    if (drmInfoArray == nullptr) {
        MEDIA_LOG_E("HandleDrmInfoUpdatedEvent new drm info failed");
        return;
    }
    int32_t i = 0;
    for (auto item : drmInfo) {
        uint32_t step = 2;
        for (uint32_t j = 0; j < item.first.size(); j += step) {
            std::string byteString = item.first.substr(j, step);
            unsigned char byte = (unsigned char)strtol(byteString.c_str(), NULL, 16);
            drmInfoArray[i].uuid[j / step] = byte;
        }

        errno_t ret = memcpy_s(drmInfoArray[i].pssh, sizeof(drmInfoArray[i].pssh),
            item.second.data(), item.second.size());
        if (ret != EOK) {
            MEDIA_LOG_E("HandleDrmInfoUpdatedEvent memcpy drm info pssh failed");
            delete []drmInfoArray;
            return;
        }
        drmInfoArray[i].psshLen = item.second.size();
        i++;
    }

    // report event
    Format format;
    size_t drmInfoSize = static_cast<size_t>(infoCount) * sizeof(DrmInfoItem);
    (void) format.PutBuffer(PlayerKeys::PLAYER_DRM_INFO_ADDR,
        reinterpret_cast<const uint8_t *>(drmInfoArray), drmInfoSize);
    (void) format.PutIntValue(PlayerKeys::PLAYER_DRM_INFO_COUNT, static_cast<int32_t>(infoCount));
    callbackLooper_.OnInfo(INFO_TYPE_DRM_INFO_UPDATED, static_cast<int32_t>(singleLoop_.load()), format);

    // triger waiting
    MEDIA_LOG_I("HiPlayerImpl has received drminfo event, and this is drm protected");
    std::unique_lock<std::mutex> lock(drmMutex_);
    isDrmProtected_ = true;
    lock.unlock();

    delete []drmInfoArray;
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
    MEDIA_LOG_I("NotifyDurationUpdate duration_ " PUBLIC_LOG_D64 " param " PUBLIC_LOG_D32, duration_, param);
    callbackLooper_.OnInfo(INFO_TYPE_DURATION_UPDATE, duration_, format);
}

void HiPlayerImpl::NotifySeekDone(int32_t seekPos)
{
    Format format;
    callbackLooper_.OnInfo(INFO_TYPE_SEEKDONE, seekPos, format);
    callbackLooper_.OnInfo(INFO_TYPE_POSITION_UPDATE, seekPos, format);
}

void HiPlayerImpl::NotifyAudioInterrupt(const Event& event)
{
    Format format;
    uint32_t value = AnyCast<uint32_t>(event.param);
    int32_t hintType = value & 0x000000FF;
    int32_t forceType = (value >> INTERRUPT_EVENT_SHIFT) & 0x000000FF;
    int32_t eventType = value >> (INTERRUPT_EVENT_SHIFT * 2);
    (void)format.PutIntValue(PlayerKeys::AUDIO_INTERRUPT_TYPE, eventType);
    (void)format.PutIntValue(PlayerKeys::AUDIO_INTERRUPT_FORCE, forceType);
    (void)format.PutIntValue(PlayerKeys::AUDIO_INTERRUPT_HINT, hintType);
    callbackLooper_.OnInfo(INFO_TYPE_INTERRUPT_EVENT, 0, format);
}

void HiPlayerImpl::NotifyAudioFirstFrame(const Event& event)
{
    uint64_t latency = AnyCast<uint64_t>(event.param);
    MEDIA_LOG_I("Audio first frame event in latency " PUBLIC_LOG_U64, latency);
    Format format;
    (void)format.PutLongValue(PlayerKeys::AUDIO_FIRST_FRAME, latency);
    callbackLooper_.OnInfo(INFO_TYPE_AUDIO_FIRST_FRAME, 0, format);
}

void HiPlayerImpl::NotifyResolutionChange()
{
    std::vector<Format> videoTrackInfo;
    GetVideoTrackInfo(videoTrackInfo);
    if (videoTrackInfo.size() == 0) {
        return;
    }
    for (auto& videoTrack : videoTrackInfo) {
        int32_t height;
        videoTrack.GetIntValue("height", height);
        int32_t width;
        videoTrack.GetIntValue("width", width);
        if (height <= 0 && width <= 0) {
            continue;
        }
        int32_t rotation = 0;
        bool needSwapWH = videoTrack.GetIntValue(Tag::VIDEO_ROTATION, rotation)
            && (rotation == rotation90 || rotation == rotation270);
        MEDIA_LOG_D("rotation %{public}d", rotation);
        Format format;
        (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_WIDTH), !needSwapWH ? width : height);
        (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_HEIGHT), !needSwapWH ? height : width);
        MEDIA_LOG_I("video size changed, width = %{public}d, height = %{public}d", width, height);
        callbackLooper_.OnInfo(INFO_TYPE_RESOLUTION_CHANGE, 0, format);
        break;
    }
}

void HiPlayerImpl::NotifyPositionUpdate()
{
    int32_t currentPosMs = 0;
    GetCurrentTime(currentPosMs);
    Format format;
    callbackLooper_.OnInfo(INFO_TYPE_POSITION_UPDATE, currentPosMs, format);
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
        if (audioRenderInfo_ != nullptr) {
            audioSink_->SetParameter(audioRenderInfo_);
        }
        if (audioInterruptMode_ != nullptr) {
            audioSink_->SetParameter(audioInterruptMode_);
        }
        std::shared_ptr<Meta> globalMeta = std::make_shared<Meta>();
        if (demuxer_ != nullptr) {
            globalMeta = demuxer_->GetGlobalMetaInfo();
        }
        if (globalMeta != nullptr) {
            globalMeta->SetData(Tag::APP_PID, appPid_);
            globalMeta->SetData(Tag::APP_UID, appUid_);
            audioSink_->SetParameter(globalMeta);
        }
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

        // set decrypt config for drm videos
        if (isDrmProtected_) {
            std::unique_lock<std::mutex> lock(drmMutex_);
            static constexpr int32_t timeout = 5;
            bool notTimeout = drmConfigCond_.wait_for(lock, std::chrono::seconds(timeout), [this]() {
                return this->isDrmPrepared_ || this->stopWaitingDrmConfig_;
            });
            if (notTimeout && isDrmPrepared_) {
                MEDIA_LOG_I("HiPlayerImpl::LinkVideoDecoderFilter will SetDecryptConfig");
                bool svpFlag = svpMode_ == HiplayerSvpMode::SVP_TRUE ? true : false;
                videoDecoder_->SetDecryptConfig(keySessionServiceProxy_, svpFlag);
            } else {
                MEDIA_LOG_E("HiPlayerImpl Drmcond wait timeout or has been stopped! Play drm protected video failed!");
                return Status::ERROR_INVALID_OPERATION;
            }
        }
    }
    return pipeline_->LinkFilters(preFilter, {videoDecoder_}, type);
}
#endif
}  // namespace Media
}  // namespace OHOS
