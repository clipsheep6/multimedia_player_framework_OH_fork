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
#include "directory_ex.h"
#include "filter/filter_factory.h"
#include "media_errors.h"
#include "osal/task/jobutils.h"
#include "osal/task/pipeline_threadpool.h"
#include "osal/task/task.h"
#include "osal/utils/dump_buffer.h"
#include "plugin/plugin_time.h"
#include "media_dfx.h"
#include "media_utils.h"
#include "meta_utils.h"
#include "meta/media_types.h"

namespace {
const float MAX_MEDIA_VOLUME = 1.0f; // standard interface volume is between 0 to 1.
const float MIN_MEDIA_VOLUME = 0.0f; // standard interface volume is between 0 to 1.
const int32_t FADE_OUT_LATENCY = 40; // fade out latency ms
const int32_t AUDIO_SINK_MAX_LATENCY = 400; // audio sink write latency ms
const int32_t FRAME_RATE_UNIT_MULTIPLE = 100; // the unit of frame rate is frames per 100s
const int32_t PLAYING_SEEK_WAIT_TIME = 200; // wait up to 200 ms for new frame after seek in playing.
}

namespace OHOS {
namespace Media {
using namespace Pipeline;
using namespace OHOS::Media::Plugins;
class PlayerEventReceiver : public EventReceiver {
public:
    explicit PlayerEventReceiver(HiPlayerImpl* hiPlayerImpl, std::string playerId)
    {
        MEDIA_LOG_I("PlayerEventReceiver ctor called.");
        hiPlayerImpl_ = hiPlayerImpl;
        task_ = std::make_unique<Task>("PlayerEventReceiver", playerId, TaskType::GLOBAL,
            OHOS::Media::TaskPriority::HIGH, false);
    }

    void OnEvent(const Event &event)
    {
        MEDIA_LOG_I("PlayerEventReceiver OnEvent.");
        task_->SubmitJobOnce([this, event] { hiPlayerImpl_->OnEvent(event); });
    }

private:
    HiPlayerImpl* hiPlayerImpl_;
    std::unique_ptr<Task> task_;
};

class PlayerFilterCallback : public FilterCallback {
public:
    explicit PlayerFilterCallback(HiPlayerImpl* hiPlayerImpl)
    {
        MEDIA_LOG_I("PlayerFilterCallback ctor called.");
        hiPlayerImpl_ = hiPlayerImpl;
    }

    Status OnCallback(const std::shared_ptr<Filter>& filter, FilterCallBackCommand cmd, StreamType outType)
    {
        MEDIA_LOG_I("PlayerFilterCallback OnCallback.");
        return hiPlayerImpl_->OnCallback(filter, cmd, outType);
    }

private:
    HiPlayerImpl* hiPlayerImpl_;
};

HiPlayerImpl::HiPlayerImpl(int32_t appUid, int32_t appPid, uint32_t appTokenId, uint64_t appFullTokenId)
    : appUid_(appUid), appPid_(appPid), appTokenId_(appTokenId), appFullTokenId_(appFullTokenId)
{
    MEDIA_LOG_I("hiPlayerImpl ctor appUid " PUBLIC_LOG_D32 " appPid " PUBLIC_LOG_D32 " appTokenId " PUBLIC_LOG_D32
        " appFullTokenId " PUBLIC_LOG_D64, appUid_, appPid_, appTokenId_, appFullTokenId_);
    playerId_ = std::string("HiPlayer_") + std::to_string(OHOS::Media::Pipeline::Pipeline::GetNextPipelineId());
    pipeline_ = std::make_shared<OHOS::Media::Pipeline::Pipeline>();
    syncManager_ = std::make_shared<MediaSyncManager>();
    callbackLooper_.SetPlayEngine(this, playerId_);
    bundleName_ = GetClientBundleName(appUid);
}

HiPlayerImpl::~HiPlayerImpl()
{
    MEDIA_LOG_I("~HiPlayerImpl dtor called.");
    if (demuxer_) {
        pipeline_->RemoveHeadFilter(demuxer_);
    }
    PipeLineThreadPool::GetInstance().DestroyThread(playerId_);
}

void HiPlayerImpl::ReleaseInner()
{
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
    MediaTrace trace("HiPlayerImpl::Init");
    MEDIA_LOG_I("Init entered.");
    std::shared_ptr<EventReceiver> playerEventReceiver = std::make_shared<PlayerEventReceiver>(this, playerId_);
    playerEventReceiver_ = playerEventReceiver;
    std::shared_ptr<FilterCallback> playerFilterCallback = std::make_shared<PlayerFilterCallback>(this);
    playerFilterCallback_ = playerFilterCallback;
    MEDIA_LOG_I("pipeline init start");
    pipeline_->Init(playerEventReceiver, playerFilterCallback, playerId_);
    MEDIA_LOG_I("Init End.");
    for (std::pair<std::string, bool>& item: completeState_) {
        item.second = false;
    }
    SetDefaultAudioRenderInfo();
    return Status::OK;
}

void HiPlayerImpl::SetDefaultAudioRenderInfo()
{
    MEDIA_LOG_I("SetDefaultAudioRenderInfo");
    Plugins::AudioRenderInfo audioRenderInfo {AudioStandard::CONTENT_TYPE_MUSIC,
        AudioStandard::STREAM_USAGE_MEDIA, 0};
    if (audioRenderInfo_ == nullptr) {
        audioRenderInfo_ = std::make_shared<Meta>();
        audioRenderInfo_->SetData(Tag::AUDIO_RENDER_INFO, audioRenderInfo);
    }
}

int32_t HiPlayerImpl::GetRealPath(const std::string &url, std::string &realUrlPath) const
{
    std::string fileHead = "file://";
    std::string tempUrlPath;

    if (url.find(fileHead) == 0 && url.size() > fileHead.size()) {
        tempUrlPath = url.substr(fileHead.size());
    } else {
        tempUrlPath = url;
    }
    if (tempUrlPath.find("..") != std::string::npos) {
        MEDIA_LOG_E("invalid url. The Url (%{public}s) path may be invalid.", tempUrlPath.c_str());
        return MSERR_FILE_ACCESS_FAILED;
    }
    bool ret = PathToRealPath(tempUrlPath, realUrlPath);
    if (!ret) {
        MEDIA_LOG_E("invalid url. The Url (%{public}s) path may be invalid.", url.c_str());
        return MSERR_OPEN_FILE_FAILED;
    }
    if (access(realUrlPath.c_str(), R_OK) != 0) {
        return MSERR_FILE_ACCESS_FAILED;
    }
    return MSERR_OK;
}

bool HiPlayerImpl::IsFileUrl(const std::string &url) const
{
    return url.find("://") == std::string::npos || url.find("file://") == 0;
}

int32_t HiPlayerImpl::SetSource(const std::string& uri)
{
    MediaTrace trace("HiPlayerImpl::SetSource uri");
    MEDIA_LOG_I("SetSource entered source uri: " PUBLIC_LOG_S, uri.c_str());
    url_ = uri;
    if (IsFileUrl(uri)) {
        std::string realUriPath;
        int32_t result = GetRealPath(uri, realUriPath);
        if (result != MSERR_OK) {
            MEDIA_LOG_E("SetSource error: GetRealPath error");
            return result;
        }
        url_ = "file://" + realUriPath;
    }
    if (url_.find("http") == 0 || url_.find("https") == 0) {
        isNetWorkPlay_ = true;
    }
    pipelineStates_ = PlayerStates::PLAYER_INITIALIZED;
    return TransStatus(Status::OK);
}

int32_t HiPlayerImpl::SetMediaSource(const std::shared_ptr<AVMediaSource> &mediaSource, AVPlayStrategy strategy)
{
    MediaTrace trace("HiPlayerImpl::SetMediaSource.");
    MEDIA_LOG_I("SetMediaSource entered media source stream");
    if (mediaSource == nullptr) {
        return MSERR_INVALID_VAL;
    }
    header_ = mediaSource->header;
    url_ = mediaSource->url;
    preferedWidth_ = strategy.preferredWidth;
    preferedHeight_ = strategy.preferredHeight;
    bufferDuration_ = strategy.preferredBufferDuration;
    preferHDR_ = strategy.preferredHdr;
    return MSERR_OK;
}

int32_t HiPlayerImpl::SetSource(const std::shared_ptr<IMediaDataSource>& dataSrc)
{
    MediaTrace trace("HiPlayerImpl::SetSource dataSrc");
    MEDIA_LOG_I("SetSource entered source stream");
    if (dataSrc == nullptr) {
        MEDIA_LOG_E("SetSource error: dataSrc is null");
    }
    dataSrc_ = dataSrc;
    pipelineStates_ = PlayerStates::PLAYER_INITIALIZED;
    return TransStatus(Status::OK);
}

void HiPlayerImpl::ResetIfSourceExisted()
{
    FALSE_RETURN_MSG(demuxer_ != nullptr, "Source not exist, no need reset.");
    MEDIA_LOG_I("Source is existed, reset the relatived objects.");
    ReleaseInner();
    if (pipeline_ != nullptr) {
        pipeline_.reset();
    }
    if (audioDecoder_ != nullptr) {
        audioDecoder_.reset();
    }

    pipeline_ = std::make_shared<OHOS::Media::Pipeline::Pipeline>();
    syncManager_ = std::make_shared<MediaSyncManager>();
    MEDIA_LOG_I("Reset the relatived objects end.");
}

int32_t HiPlayerImpl::Prepare()
{
    return TransStatus(Status::OK);
}

int32_t HiPlayerImpl::SetRenderFirstFrame(bool display)
{
    MEDIA_LOG_I("SetRenderFirstFrame entered, display: " PUBLIC_LOG_D32, display);
    renderFirstFrame_ = display;
    return TransStatus(Status::OK);
}

int32_t HiPlayerImpl::PrepareAsync()
{
    MediaTrace trace("HiPlayerImpl::PrepareAsync");
    MEDIA_LOG_I("PrepareAsync Start");
    if (!(pipelineStates_ == PlayerStates::PLAYER_INITIALIZED || pipelineStates_ == PlayerStates::PLAYER_STOPPED)) {
        return MSERR_INVALID_OPERATION;
    }
    auto ret = Init();
    if (ret != Status::OK || isInterruptNeeded_.load()) {
        MEDIA_LOG_E("PrepareAsync error: init error");
        return TransStatus(Status::ERROR_UNSUPPORTED_FORMAT);
    }
    if (dataSrc_ != nullptr) {
        ret = DoSetSource(std::make_shared<MediaSource>(dataSrc_));
    } else {
        if (!header_.empty()) {
            MEDIA_LOG_I("DoSetSource header");
            ret = DoSetSource(std::make_shared<MediaSource>(url_, header_));
        } else {
            MEDIA_LOG_I("DoSetSource url");
            ret = DoSetSource(std::make_shared<MediaSource>(url_));
        }
    }
    if (ret != Status::OK) {
        MEDIA_LOG_E("PrepareAsync error: DoSetSource error");
        OnEvent({"engine", EventType::EVENT_ERROR, MSERR_UNSUPPORT_CONTAINER_TYPE});
        return TransStatus(Status::ERROR_UNSUPPORTED_FORMAT);
    }
    FALSE_RETURN_V(!BreakIfInterruptted(), TransStatus(Status::OK));
    NotifyBufferingUpdate(PlayerKeys::PLAYER_BUFFERING_START, 0);
    MEDIA_LOG_I("PrepareAsync entered, current pipeline state: " PUBLIC_LOG_S,
        StringnessPlayerState(pipelineStates_).c_str());
    OnStateChanged(PlayerStateId::PREPARING);
    ret = pipeline_->Prepare();
    if (ret != Status::OK) {
        MEDIA_LOG_E("PrepareAsync failed with error " PUBLIC_LOG_D32, ret);
        return TransStatus(ret);
    }
    ret = pipeline_->PrepareFrame(renderFirstFrame_);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, TransStatus(ret), "PrepareFrame failed.");
    NotifyBufferingUpdate(PlayerKeys::PLAYER_BUFFERING_END, 0);
    InitDuration();
    NotifyDurationUpdate(PlayerKeys::PLAYER_CACHED_DURATION, durationMs_.load());
    InitVideoWidthAndHeight();
    NotifyResolutionChange();
    NotifyPositionUpdate();
    DoInitializeForHttp();
    OnStateChanged(PlayerStateId::READY);
    MEDIA_LOG_I("PrepareAsync End, resource duration " PUBLIC_LOG_D32, durationMs_.load());
    return TransStatus(ret);
}

bool HiPlayerImpl::BreakIfInterruptted()
{
    if (isInterruptNeeded_.load()) {
        OnStateChanged(PlayerStateId::READY);
        return true;
    }
    return false;
}

void HiPlayerImpl::SetInterruptState(bool isInterruptNeeded)
{
    isInterruptNeeded_ = isInterruptNeeded;
    if (demuxer_ != nullptr) {
        demuxer_->SetInterruptState(isInterruptNeeded);
    }
}

int32_t HiPlayerImpl::SelectBitRate(uint32_t bitRate)
{
    if (demuxer_ == nullptr) {
        MEDIA_LOG_E("SelectBitRate failed, demuxer_ is null");
        return MSERR_INVALID_OPERATION;
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
        MEDIA_LOG_E("DoInitializeForHttp failed, not network play");
        return;
    }
    std::vector<uint32_t> vBitRates;
    MEDIA_LOG_I("DoInitializeForHttp");
    auto ret = demuxer_->GetBitRates(vBitRates);
    if (ret == Status::OK && vBitRates.size() > 0) {
        int mSize = static_cast<int>(vBitRates.size());
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
    MediaTrace trace("HiPlayerImpl::Play");
    MEDIA_LOG_I("Play entered.");
    int32_t ret = MSERR_INVALID_VAL;
    isInCompleted_ = false;
    callbackLooper_.StartReportMediaProgress(100); // 100 ms
    if (pipelineStates_ == PlayerStates::PLAYER_PLAYBACK_COMPLETE || pipelineStates_ == PlayerStates::PLAYER_STOPPED) {
        isStreaming_ = true;
        ret = TransStatus(Seek(0, PlayerSeekMode::SEEK_PREVIOUS_SYNC, false));
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
        if (!isInitialPlay_) {
            OnStateChanged(PlayerStateId::PLAYING);
        } else {
            MEDIA_LOG_I("InitialPlay, pending to change state of playing.");
        }
    }
    return ret;
}

int32_t HiPlayerImpl::Pause()
{
    MediaTrace trace("HiPlayerImpl::Pause");
    MEDIA_LOG_I("Pause entered.");
    if (pipelineStates_ == PlayerStates::PLAYER_PLAYBACK_COMPLETE) {
        MEDIA_LOG_E("completed not allow pause");
        return TransStatus(Status::OK);
    }
    if (audioSink_ != nullptr) {
        audioSink_->SetVolumeWithRamp(MIN_MEDIA_VOLUME, FADE_OUT_LATENCY);
    }
    Status ret = Status::OK;
    syncManager_->Pause();
    ret = pipeline_->Pause();
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
    MediaTrace trace("HiPlayerImpl::Stop");
    MEDIA_LOG_I("Stop entered.");
    callbackLooper_.StopReportMediaProgress();
    // close demuxer first to avoid concurrent problem
    auto ret = Status::ERROR_UNKNOWN;
    if (pipeline_ != nullptr) {
        ret = pipeline_->Stop();
    }
    syncManager_->Stop();
    if (audioDecoder_ != nullptr) {
        audioDecoder_->Flush();
    }
    #ifdef SUPPORT_VIDEO
        if (videoDecoder_) {
            videoDecoder_->Stop();
            videoDecoder_->Flush();
        }
    #endif
    if (audioSink_ != nullptr) {
        audioSink_->Flush();
    }
    for (std::pair<std::string, bool>& item: completeState_) {
        item.second = false;
    }

    // triger drm waiting condition
    if (isDrmProtected_) {
        std::unique_lock<std::mutex> drmLock(drmMutex_);
        stopWaitingDrmConfig_ = true;
        drmConfigCond_.notify_all();
    }
    isInCompleted_ = false;
    OnStateChanged(PlayerStateId::STOPPED);
    return TransStatus(ret);
}

int32_t HiPlayerImpl::Reset()
{
    MediaTrace trace("HiPlayerImpl::Reset");
    if (pipelineStates_ == PlayerStates::PLAYER_STOPPED) {
        return TransStatus(Status::OK);
    }
    singleLoop_ = false;
    auto ret = Stop();
    syncManager_->Reset();
    OnStateChanged(PlayerStateId::STOPPED);
    return ret;
}

int32_t HiPlayerImpl::SeekToCurrentTime(int32_t mSeconds, PlayerSeekMode mode)
{
    MEDIA_LOG_I("SeekToCurrentTime entered. mSeconds : " PUBLIC_LOG_D32 ", seekMode : " PUBLIC_LOG_D32,
                mSeconds, static_cast<int32_t>(mode));
    return Seek(mSeconds, mode);
}

Status HiPlayerImpl::Seek(int64_t mSeconds, PlayerSeekMode mode, bool notifySeekDone)
{
    MediaTrace trace("HiPlayerImpl::Seek");
    MEDIA_LOG_I("Seek entered. mSeconds : " PUBLIC_LOG_D64 ", seekMode : " PUBLIC_LOG_D32,
                mSeconds, static_cast<int32_t>(mode));
    if (audioSink_ != nullptr) {
        audioSink_->SetIsTransitent(true);
    }
    FALSE_RETURN_V_MSG_E(durationMs_.load() > 0, Status::ERROR_INVALID_PARAMETER,
        "Seek, invalid operation, source is unseekable or invalid");
    isSeek_ = true;
    int64_t seekPos = std::max(static_cast<int64_t>(0), std::min(mSeconds, static_cast<int64_t>(durationMs_.load())));
    auto rtv = seekPos >= 0 ? Status::OK : Status::ERROR_INVALID_PARAMETER;
    if (rtv == Status::OK) {
        switch (pipelineStates_) {
            case PlayerStates::PLAYER_STARTED: {
                rtv = doStartedSeek(seekPos, mode);
                break;
            }
            case PlayerStates::PLAYER_PAUSED: {
                rtv = doPausedSeek(seekPos, mode);
                break;
            }
            case PlayerStates::PLAYER_PLAYBACK_COMPLETE: {
                rtv = doCompletedSeek(seekPos, mode);
                break;
            }
            case PlayerStates::PLAYER_PREPARED: {
                rtv = doPreparedSeek(seekPos, mode);
                break;
            }
            default:
                MEDIA_LOG_I("Seek in error pipelineStates: " PUBLIC_LOG_D32, static_cast<int32_t>(pipelineStates_));
                rtv = Status::ERROR_WRONG_STATE;
                break;
        }
    }
    NotifySeek(rtv, notifySeekDone, seekPos);
    if (audioSink_ != nullptr) {
        audioSink_->SetIsTransitent(false);
    }
    isSeek_ = false;
    return rtv;
}

void HiPlayerImpl::NotifySeek(Status rtv, bool flag, int64_t seekPos)
{
    if (rtv != Status::OK) {
        MEDIA_LOG_E("Seek done, seek error.");
        // change player state to PLAYER_STATE_ERROR when seek error.
        UpdateStateNoLock(PlayerStates::PLAYER_STATE_ERROR);
    }  else if (flag) {
        // only notify seekDone for external call.
        NotifySeekDone(seekPos);
    }
}

int32_t HiPlayerImpl::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    MediaTrace trace("HiPlayerImpl::Seek.");
    MEDIA_LOG_I("Seek.");
    return TransStatus(Seek(mSeconds, mode, true));
}

Status HiPlayerImpl::doPreparedSeek(int64_t seekPos, PlayerSeekMode mode)
{
    MEDIA_LOG_I("doPreparedSeek.");
    int32_t curPosMs = 0;
    GetCurrentTime(curPosMs);
    if (seekPos == static_cast<int64_t>(curPosMs)) {
        MEDIA_LOG_I("doPreparedSeek return and already at curPosMs: " PUBLIC_LOG_D32, curPosMs);
        return Status::OK;
    }
    pipeline_ -> Flush();
    auto rtv = doSeek(seekPos, mode);
    return rtv;
}

Status HiPlayerImpl::doStartedSeek(int64_t seekPos, PlayerSeekMode mode)
{
    MEDIA_LOG_I("doStartedSeek.");
    // audio fade in and out
    if (audioSink_ != nullptr) {
        audioSink_->SetVolumeWithRamp(MIN_MEDIA_VOLUME, FADE_OUT_LATENCY);
    }
    pipeline_ -> Pause();
    pipeline_ -> Flush();
    auto rtv = doSeek(seekPos, mode);
    pipeline_ -> Resume();
    return rtv;
}

Status HiPlayerImpl::doPausedSeek(int64_t seekPos, PlayerSeekMode mode)
{
    MEDIA_LOG_I("doPausedSeek.");
    int32_t curPosMs = 0;
    GetCurrentTime(curPosMs);
    if (seekPos == static_cast<int64_t>(curPosMs)) {
        MEDIA_LOG_I("doPausedSeek return and already at curPosMs: " PUBLIC_LOG_D32, curPosMs);
        return Status::OK;
    }
    pipeline_ -> Pause();
    pipeline_ -> Flush();
    auto rtv = doSeek(seekPos, mode);
    return rtv;
}

Status HiPlayerImpl::doCompletedSeek(int64_t seekPos, PlayerSeekMode mode)
{
    MEDIA_LOG_I("doCompletedSeek.");
    pipeline_ -> Flush();
    auto rtv = doSeek(seekPos, mode);
    if (isStreaming_) {
        MEDIA_LOG_D("doCompletedSeek isStreaming_ is true");
        pipeline_->Resume();
        syncManager_->Resume();
    } else {
        callbackLooper_.StopReportMediaProgress();
        callbackLooper_.ManualReportMediaProgressOnce();
        OnStateChanged(PlayerStateId::PAUSE);
    }
    return rtv;
}

Status HiPlayerImpl::doSeek(int64_t seekPos, PlayerSeekMode mode)
{
    MEDIA_LOG_I("doSeek.");
    int64_t seekTimeUs = 0;
    if (!Plugins::Us2HstTime(seekPos, seekTimeUs)) { // ms to us
        MEDIA_LOG_E("Invalid seekPos: %{public}" PRId64, seekPos);
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (mode == PlayerSeekMode::SEEK_CLOSEST) {
        MEDIA_LOG_I("doSeek SEEK_CLOSEST.");
        if (videoDecoder_ != nullptr) {
            videoDecoder_->SetSeekTime(seekTimeUs);
        }
        seekAgent_ = std::make_shared<SeekAgent>(demuxer_);
        auto res = seekAgent_->Seek(seekPos);
        MEDIA_LOG_I("seekAgent_ Seek end");
        if (res != Status::OK) {
            MEDIA_LOG_E("Seek closest failed.");
        } else {
            syncManager_->Seek(seekTimeUs);
        }
        seekAgent_.reset();
        return res;
    }
    int64_t realSeekTime = seekPos;
    auto seekMode = Transform2SeekMode(mode);
    auto rtv = demuxer_->SeekTo(seekPos, seekMode, realSeekTime);
    // if it has no next key frames, seek previous.
    if (rtv != Status::OK && mode == PlayerSeekMode::SEEK_NEXT_SYNC) {
        seekMode = Transform2SeekMode(PlayerSeekMode::SEEK_PREVIOUS_SYNC);
        rtv = demuxer_->SeekTo(seekPos, seekMode, realSeekTime);
    }
    if (rtv == Status::OK) {
        syncManager_->Seek(seekTimeUs);
    }
    return rtv;
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
    FALSE_RETURN_V_MSG_E(audioSink_ != nullptr, (int32_t)TransStatus(Status::ERROR_INVALID_OPERATION),
        "Set volume failed, audio sink is nullptr");
    MEDIA_LOG_I("Sink SetVolume");
    Status ret = audioSink_->SetVolume(volume);
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
    surface_ = surface;
    if (videoDecoder_ != nullptr &&
        pipelineStates_ != PlayerStates::PLAYER_STOPPED &&
        pipelineStates_ != PlayerStates::PLAYER_STATE_ERROR) {
        return TransStatus(videoDecoder_->SetVideoSurface(surface));
    }
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
    MediaTrace trace("HiPlayerImpl::SetParameter");
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
    if (curState_ == PlayerStateId::EOS && isInCompleted_) {
        currentPositionMs = durationMs_.load();
        return TransStatus(Status::OK);
    }
    if (isSeek_.load()) {
        return TransStatus(Status::ERROR_UNKNOWN);
    }
    FALSE_RETURN_V(syncManager_ != nullptr, TransStatus(Status::ERROR_NULL_POINTER));
    currentPositionMs = Plugins::HstTime2Us32(syncManager_->GetMediaTimeNow());
    if (currentPositionMs < 0) {
        currentPositionMs = 0;
    }
    if (durationMs_.load() > 0 && currentPositionMs > durationMs_.load()) {
        currentPositionMs = durationMs_.load();
    }
    return TransStatus(Status::OK);
}

int32_t HiPlayerImpl::GetDuration(int32_t& durationMs)
{
    durationMs = durationMs_.load();
    MEDIA_LOG_I("Get media duration in GetDuration: " PUBLIC_LOG_D32, durationMs);
    return TransStatus(Status::OK);
}

int32_t HiPlayerImpl::InitDuration()
{
    if (demuxer_ == nullptr) {
        MEDIA_LOG_W("Get media duration failed, demuxer is not ready.");
        return TransStatus(Status::ERROR_WRONG_STATE);
    }
    int64_t duration = 0;
    bool found = false;
    if (demuxer_->GetDuration(duration)) {
        found = true;
    } else {
        MEDIA_LOG_W("Get media duration failed.");
    }
    if (found && duration > 0 && duration != durationMs_.load()) {
        durationMs_ = Plugins::HstTime2Us(duration);
    }
    durationMs_ = std::max(durationMs_.load(), 0);
    MEDIA_LOG_I("Get media duration in InitDuration: " PUBLIC_LOG_D32, durationMs_.load());
    return TransStatus(Status::OK);
}

void HiPlayerImpl::SetBundleName(std::string bundleName)
{
    if (!bundleName.empty()) {
        MEDIA_LOG_I("SetBundleName bundleName: " PUBLIC_LOG_S, bundleName.c_str());
        demuxer_->SetBundleName(bundleName);
    } else {
        MEDIA_LOG_I("SetBundleName failed.");
    }
}

int32_t HiPlayerImpl::InitVideoWidthAndHeight()
{
#ifdef SUPPORT_VIDEO
    std::vector<Format> videoTrackInfo;
    GetVideoTrackInfo(videoTrackInfo);
    if (videoTrackInfo.size() == 0) {
        MEDIA_LOG_E("InitVideoWidthAndHeight failed, as videoTrackInfo is empty!");
        return TransStatus(Status::ERROR_INVALID_OPERATION);
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
        needSwapWH_ = videoTrack.GetIntValue(Tag::VIDEO_ROTATION, rotation)
            && (rotation == rotation90 || rotation == rotation270);
        MEDIA_LOG_I("rotation %{public}d", rotation);
        videoWidth_ = !needSwapWH_.load() ? width : height;
        videoHeight_ = !needSwapWH_.load() ? height : width;
        MEDIA_LOG_I("InitVideoWidthAndHeight, width = %{public}d, height = %{public}d",
            videoWidth_.load(), videoHeight_.load());
        break;
    }
#endif
    return TransStatus(Status::OK);
}

void HiPlayerImpl::InitAudioDefaultTrackIndex()
{
    std::vector<std::shared_ptr<Meta>> metaInfo = demuxer_->GetStreamMetaInfo();
    std::string mime;
    for (size_t trackIndex = 0; trackIndex < metaInfo.size(); trackIndex++) {
        auto trackInfo = metaInfo[trackIndex];
        if (!(trackInfo->GetData(Tag::MIME_TYPE, mime))) {
            MEDIA_LOG_W("Get MIME fail");
            continue;
        }
        if (mime.find("audio/") == 0) {
            defaultAudioTrackId_ = trackIndex;
            break;
        }
    }
    currentAudioTrackId_ = defaultAudioTrackId_;
}

int32_t HiPlayerImpl::SetAudioEffectMode(int32_t effectMode)
{
    MEDIA_LOG_I("SetAudioEffectMode entered.");
    Status res = Status::OK;
    if (audioSink_ != nullptr) {
        res = audioSink_->SetAudioEffectMode(effectMode);
    }
    if (res != Status::OK) {
        MEDIA_LOG_E("audioSink set AudioEffectMode error.");
        return MSERR_UNKNOWN;
    }
    return MSERR_OK;
}

int32_t HiPlayerImpl::GetAudioEffectMode(int32_t &effectMode)
{
    MEDIA_LOG_I("GetAudioEffectMode entered.");
    Status res = Status::OK;
    if (audioSink_ != nullptr) {
        res = audioSink_->GetAudioEffectMode(effectMode);
    }
    if (res != Status::OK) {
        MEDIA_LOG_E("audioSink get AudioEffectMode error.");
        return MSERR_UNKNOWN;
    }
    return MSERR_OK;
}

int32_t HiPlayerImpl::SetPlaybackSpeed(PlaybackRateMode mode)
{
    MEDIA_LOG_I("SetPlaybackSpeed entered, mode=%{public}d", mode);
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
    if (demuxer_ != nullptr) {
        demuxer_->SetSpeed(speed);
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

int32_t HiPlayerImpl::GetCurrentTrack(int32_t trackType, int32_t &index)
{
    FALSE_RETURN_V_MSG_W(trackType >= OHOS::Media::MediaType::MEDIA_TYPE_AUD &&
        trackType <= OHOS::Media::MediaType::MEDIA_TYPE_SUBTITLE,
        MSERR_INVALID_VAL, "Invalid trackType %{public}d", trackType);
    if (trackType == OHOS::Media::MediaType::MEDIA_TYPE_AUD) {
        if (currentAudioTrackId_ < 0) {
            InitAudioDefaultTrackIndex();
        }
        index = currentAudioTrackId_;
    } else {
        (void)index;
    }

    return MSERR_OK;
}

int32_t HiPlayerImpl::SelectTrack(int32_t trackId)
{
    MEDIA_LOG_I("SelectTrack begin trackId is " PUBLIC_LOG_D32, trackId);
    std::vector<std::shared_ptr<Meta>> metaInfo = demuxer_->GetStreamMetaInfo();
    std::string mime;
    if (currentAudioTrackId_ < 0) {
        InitAudioDefaultTrackIndex();
    }
    FALSE_RETURN_V_MSG_W(trackId != currentAudioTrackId_ && trackId >= 0 && trackId < metaInfo.size(),
        MSERR_INVALID_VAL, "DeselectTrack trackId invalid");
    if (!(metaInfo[trackId]->GetData(Tag::MIME_TYPE, mime))) {
        MEDIA_LOG_E("SelectTrack trackId " PUBLIC_LOG_D32 "get mime error", trackId);
        return MSERR_INVALID_VAL;
    }
    if (mime.find("audio/") != 0) {
        MEDIA_LOG_E("SelectTrack trackId " PUBLIC_LOG_D32 " not support", trackId);
        return MSERR_INVALID_VAL;
    }
    if (Status::OK != demuxer_->SelectTrack(trackId)) {
        MEDIA_LOG_E("SelectTrack error. trackId is " PUBLIC_LOG_D32, trackId);
        return MSERR_UNKNOWN;
    }
    if (Status::OK != audioDecoder_->ChangePlugin(metaInfo[trackId])) {
        MEDIA_LOG_E("SelectTrack audioDecoder change plugin error");
        return MSERR_UNKNOWN;
    }
    if (Status::OK != audioSink_->ChangeTrack(metaInfo[trackId])) {
        MEDIA_LOG_E("SelectTrack audioSink change track error");
        return MSERR_UNKNOWN;
    }
    if (Status::OK != demuxer_->StartAudioTask()) {
        MEDIA_LOG_E("SelectTrack error. trackId is " PUBLIC_LOG_D32, trackId);
        return MSERR_UNKNOWN;
    }
    Format audioTrackInfo {};
    audioTrackInfo.PutIntValue("track_index", static_cast<int32_t>(trackId));
    audioTrackInfo.PutIntValue("track_is_select", static_cast<int32_t>(trackId));
    callbackLooper_.OnInfo(INFO_TYPE_TRACKCHANGE, 0, audioTrackInfo);
    currentAudioTrackId_ = trackId;
    return MSERR_OK;
}

int32_t HiPlayerImpl::DeselectTrack(int32_t trackId)
{
    MEDIA_LOG_I("DeselectTrack trackId is " PUBLIC_LOG_D32, trackId);
    if (currentAudioTrackId_ < 0) {
        InitAudioDefaultTrackIndex();
    }
    FALSE_RETURN_V_MSG_W(trackId == currentAudioTrackId_ && currentAudioTrackId_ >= 0,
        MSERR_INVALID_VAL, "DeselectTrack trackId invalid");
    return SelectTrack(defaultAudioTrackId_);
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
            videoTrackInfo.PutIntValue("track_index", trackIndex);
            int64_t bitRate;
            trackInfo->GetData(Tag::MEDIA_BITRATE, bitRate);
            videoTrackInfo.PutLongValue("bitrate", bitRate);
            double frameRate;
            trackInfo->GetData(Tag::VIDEO_FRAME_RATE, frameRate);
            videoTrackInfo.PutDoubleValue("frame_rate", frameRate * FRAME_RATE_UNIT_MULTIPLE);
            int32_t height;
            trackInfo->GetData(Tag::VIDEO_HEIGHT, height);
            videoTrackInfo.PutIntValue("height", height);
            int32_t width;
            trackInfo->GetData(Tag::VIDEO_WIDTH, width);
            videoTrackInfo.PutIntValue("width", width);
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
            audioTrackInfo.PutLongValue("bitrate", bitRate);
            int32_t audioChannels;
            trackInfo->GetData(Tag::AUDIO_CHANNEL_COUNT, audioChannels);
            audioTrackInfo.PutIntValue("channel_count", audioChannels);
            int32_t audioSampleRate;
            trackInfo->GetData(Tag::AUDIO_SAMPLE_RATE, audioSampleRate);
            audioTrackInfo.PutIntValue("sample_rate", audioSampleRate);
            int32_t sampleDepth;
            trackInfo->GetData(Tag::AUDIO_BITS_PER_CODED_SAMPLE, sampleDepth);
            audioTrackInfo.PutIntValue("sample_depth", sampleDepth);
            audioTrack.emplace_back(std::move(audioTrackInfo));
        }
    }
    return TransStatus(Status::OK);
}

int32_t HiPlayerImpl::GetVideoWidth()
{
#ifdef SUPPORT_VIDEO
    MEDIA_LOG_I("GetVideoWidth entered. video width: " PUBLIC_LOG_D32, videoWidth_.load());
#endif
    return videoWidth_.load();
}

int32_t HiPlayerImpl::GetVideoHeight()
{
#ifdef SUPPORT_VIDEO
    MEDIA_LOG_I("GetVideoHeight entered. video height: " PUBLIC_LOG_D32, videoHeight_.load());
#endif
    return videoHeight_.load();
}

int32_t HiPlayerImpl::SetVideoScaleType(OHOS::Media::VideoScaleType videoScaleType)
{
    MEDIA_LOG_I("SetVideoScaleType entered. VIDEO_SCALE_TYPE: " PUBLIC_LOG_D32, videoScaleType);
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
    MEDIA_LOG_I("SetAudioRendererInfo entered, coutentType: " PUBLIC_LOG_D32 ", streamUsage: " PUBLIC_LOG_D32
        ", rendererFlag: " PUBLIC_LOG_D32, contentType, streamUsage, rendererFlag);
    Plugins::AudioRenderInfo audioRenderInfo {contentType, streamUsage, rendererFlag};
    if (audioRenderInfo_ == nullptr) {
        audioRenderInfo_ = std::make_shared<Meta>();
    }
    audioRenderInfo_->SetData(Tag::AUDIO_RENDER_SET_FLAG, true);
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
    MEDIA_LOG_I("OnEvent entered, event type is: %{public}d", event.type);
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
            MEDIA_LOG_I("audio first frame reneder received");
            NotifyAudioFirstFrame(event);
            HandleInitialPlayingStateChange(event.type);
            break;
        }
        case EventType::EVENT_DRM_INFO_UPDATED: {
            HandleDrmInfoUpdatedEvent(event);
            break;
        }
        case EventType::EVENT_VIDEO_RENDERING_START: {
            MEDIA_LOG_I("video first frame reneder received");
            Format format;
            callbackLooper_.OnInfo(INFO_TYPE_MESSAGE, PlayerMessageType::PLAYER_INFO_VIDEO_RENDERING_START, format);
            HandleInitialPlayingStateChange(event.type);
            break;
        }
        case EventType::EVENT_RESOLUTION_CHANGE: {
            MEDIA_LOG_I("resolution change event received");
            HandleResolutionChangeEvent(event);
            break;
        }
        default:
            break;
    }
    OnEventSub(event);
}

void HiPlayerImpl::OnEventSub(const Event &event)
{
    MEDIA_LOG_I("OnEvent entered, event type is: %{public}d", event.type);
    switch (event.type) {
        case EventType::EVENT_AUDIO_DEVICE_CHANGE : {
            NotifyAudioDeviceChange(event);
            break;
        }
        case EventType::EVENT_AUDIO_SERVICE_DIED : {
            NotifyAudioServiceDied();
            break;
        }
        case EventType::BUFFERING_END : {
            MEDIA_LOG_I("HiPlayerImpl::BUFFERING_END PLAYING");
            NotifyBufferingEnd(AnyCast<int32_t>(event.param));
            break;
        }
        case EventType::BUFFERING_START : {
            MEDIA_LOG_I("HiPlayerImpl::BUFFERING_START PAUSE");
            NotifyBufferingStart(AnyCast<int32_t>(event.param));
            break;
        }
        default:
            break;
    }
}

void HiPlayerImpl::HandleInitialPlayingStateChange(const EventType& eventType)
{
    MEDIA_LOG_I("HandleInitialPlayingStateChange");
    if (!isInitialPlay_) {
        return;
    }
    for (std::pair<EventType, bool>& item : initialAVStates_) {
        if (item.first == eventType) {
            item.second = true;
        }
    }
    for (auto item : initialAVStates_) {
        if (item.second == false) {
            return;
        }
    }

    MEDIA_LOG_I("av first frame reneder all received");

    isInitialPlay_ = false;
    OnStateChanged(PlayerStateId::PLAYING);
}

Status HiPlayerImpl::DoSetSource(const std::shared_ptr<MediaSource> source)
{
    MediaTrace trace("HiPlayerImpl::DoSetSource");
    ResetIfSourceExisted();
    demuxer_ = FilterFactory::Instance().CreateFilter<DemuxerFilter>("builtin.player.demuxer",
        FilterType::FILTERTYPE_DEMUXER);
    pipeline_->AddHeadFilters({demuxer_});
    demuxer_->Init(playerEventReceiver_, playerFilterCallback_);

    PlayStrategy* playStrategy = new PlayStrategy;
    playStrategy->width = preferedWidth_;
    playStrategy->height = preferedHeight_;
    playStrategy->duration = bufferDuration_;
    playStrategy->preferHDR = preferHDR_;
    source->SetPlayStrategy(playStrategy);

    auto ret = demuxer_->SetDataSource(source);
    if (ret == Status::OK && !MetaUtils::CheckFileType(demuxer_->GetGlobalMetaInfo())) {
        MEDIA_LOG_W("0x%{public}06 " PRIXPTR "SetSource unsupport", FAKE_POINTER(this));
        ret = Status::ERROR_INVALID_DATA;
    }
    if (ret != Status::OK) {
        return ret;
    }

    std::unique_lock<std::mutex> lock(drmMutex_);
    isDrmProtected_ = demuxer_->IsDrmProtected();
    MEDIA_LOG_I("Is the source drm-protected : %{public}d", isDrmProtected_);
    lock.unlock();

    SetBundleName(bundleName_);
    demuxer_->OptimizeDecodeSlow(IsEnableOptimizeDecode());
    return ret;
}

Status HiPlayerImpl::Resume()
{
    MediaTrace trace("HiPlayerImpl::Resume");
    Status ret = Status::OK;
    ret = pipeline_->Resume();
    syncManager_->Resume();
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
    callbackLooper_.OnError(PLAYER_ERROR, errorCode);
}

void HiPlayerImpl::NotifyBufferingStart(int32_t param)
{
    Format format;
    (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_BUFFERING_START), 1);
    callbackLooper_.OnInfo(INFO_TYPE_BUFFERING_UPDATE, param, format);
}

void HiPlayerImpl::NotifyBufferingEnd(int32_t param)
{
    MEDIA_LOG_I("NotifyBufferingEnd");
    Format format;
    (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_BUFFERING_END), 1);
    callbackLooper_.OnInfo(INFO_TYPE_BUFFERING_UPDATE, param, format);
}

void HiPlayerImpl::HandleCompleteEvent(const Event& event)
{
    MEDIA_LOG_I("HandleCompleteEvent");
    for (std::pair<std::string, bool>& item: completeState_) {
        if (item.first == event.srcFilter) {
            MEDIA_LOG_I("one eos event received " PUBLIC_LOG_S, item.first.c_str());
            item.second = true;
        }
    }
    for (auto item : completeState_) {
        if (item.second == false) {
            MEDIA_LOG_I("expect receive eos event " PUBLIC_LOG_S, item.first.c_str());
            return;
        }
    }
    MEDIA_LOG_I("OnComplete looping: " PUBLIC_LOG_D32 ".", singleLoop_.load());
    isStreaming_ = false;
    Format format;
    int32_t curPosMs = 0;
    GetCurrentTime(curPosMs);
    if (durationMs_.load() > curPosMs && abs(durationMs_.load() - curPosMs) < AUDIO_SINK_MAX_LATENCY) {
        MEDIA_LOG_I("OnComplete durationMs - curPosMs: " PUBLIC_LOG_D32, durationMs_.load() - curPosMs);
        OHOS::Media::SleepInJob(durationMs_.load() - curPosMs);
    }
    if (!singleLoop_.load()) {
        callbackLooper_.StopReportMediaProgress();
    }
    pipeline_->Pause();
    callbackLooper_.DoReportCompletedTime();
    if (!singleLoop_.load()) {
        OnStateChanged(PlayerStateId::EOS);
        syncManager_->ResetTimeAnchorNoLock();
        isInCompleted_ = true;
    }
    callbackLooper_.OnInfo(INFO_TYPE_EOS, static_cast<int32_t>(singleLoop_.load()), format);
    for (std::pair<std::string, bool>& item: completeState_) {
        item.second = false;
    }
}

void HiPlayerImpl::HandleDrmInfoUpdatedEvent(const Event& event)
{
    MEDIA_LOG_I("HandleDrmInfoUpdatedEvent");

    std::multimap<std::string, std::vector<uint8_t>> drmInfo =
        AnyCast<std::multimap<std::string, std::vector<uint8_t>>>(event.param);
    uint32_t infoCount = drmInfo.size();
    if (infoCount > DrmConstant::DRM_MAX_DRM_INFO_COUNT || infoCount == 0) {
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

    delete []drmInfoArray;
}

void HiPlayerImpl::HandleResolutionChangeEvent(const Event& event)
{
#ifdef SUPPORT_VIDEO
    // update new video size
    std::pair<int32_t, int32_t> videoSize = AnyCast<std::pair<int32_t, int32_t>>(event.param);
    int32_t width = videoSize.first;
    int32_t height = videoSize.second;
    videoWidth_ = !needSwapWH_.load() ? width : height;
    videoHeight_ = !needSwapWH_.load() ? height : width;
    MEDIA_LOG_I("HandleResolutionChangeEvent, width = %{public}d, height = %{public}d",
        videoWidth_.load(), videoHeight_.load());
    // notify size change
    NotifyResolutionChange();
#endif
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

void HiPlayerImpl::NotifyBufferingUpdate(const std::string_view& type, int32_t param)
{
    Format format;
    format.PutIntValue(std::string(type), param);
    MEDIA_LOG_D("NotifyBufferingUpdate param " PUBLIC_LOG_D32, param);
    callbackLooper_.OnInfo(INFO_TYPE_BUFFERING_UPDATE, durationMs_.load(), format);
}

void HiPlayerImpl::NotifyDurationUpdate(const std::string_view& type, int32_t param)
{
    Format format;
    format.PutIntValue(std::string(type), param);
    MEDIA_LOG_I("NotifyDurationUpdate durationMs_ " PUBLIC_LOG_D64 " param " PUBLIC_LOG_D32, durationMs_.load(), param);
    callbackLooper_.OnInfo(INFO_TYPE_DURATION_UPDATE, durationMs_.load(), format);
}

void HiPlayerImpl::NotifySeekDone(int32_t seekPos)
{
    Format format;
    // Report position firstly to make sure that client can get real position when seek done in playing state.
    if (curState_ == PlayerStateId::PLAYING) {
        std::unique_lock<std::mutex> lock(seekMutex_);
        syncManager_->seekCond_.wait_for(
            lock,
            std::chrono::milliseconds(PLAYING_SEEK_WAIT_TIME),
            [this]() {
                return !syncManager_->InSeeking();
            });
    }
    MEDIA_LOG_D("NotifySeekDone seekPos: %{public}d", seekPos);
    callbackLooper_.OnInfo(INFO_TYPE_POSITION_UPDATE, seekPos, format);
    callbackLooper_.OnInfo(INFO_TYPE_SEEKDONE, seekPos, format);
}

void HiPlayerImpl::NotifyAudioInterrupt(const Event& event)
{
    MEDIA_LOG_I("NotifyAudioInterrupt");
    Format format;
    auto interruptEvent = AnyCast<AudioStandard::InterruptEvent>(event.param);
    int32_t hintType = interruptEvent.hintType;
    int32_t forceType = interruptEvent.forceType;
    int32_t eventType = interruptEvent.eventType;
    if (forceType == OHOS::AudioStandard::INTERRUPT_FORCE) {
        if (hintType == OHOS::AudioStandard::INTERRUPT_HINT_PAUSE
            || hintType == OHOS::AudioStandard::INTERRUPT_HINT_STOP) {
            Status ret = Status::OK;
            syncManager_->Pause();
            ret = pipeline_->Pause();
            if (audioSink_ != nullptr) {
                audioSink_->Pause();
            }
            if (ret != Status::OK) {
                UpdateStateNoLock(PlayerStates::PLAYER_STATE_ERROR);
            }
            callbackLooper_.StopReportMediaProgress();
        }
    }
    (void)format.PutIntValue(PlayerKeys::AUDIO_INTERRUPT_TYPE, eventType);
    (void)format.PutIntValue(PlayerKeys::AUDIO_INTERRUPT_FORCE, forceType);
    (void)format.PutIntValue(PlayerKeys::AUDIO_INTERRUPT_HINT, hintType);
    callbackLooper_.OnInfo(INFO_TYPE_INTERRUPT_EVENT, 0, format);
}

void HiPlayerImpl::NotifyAudioDeviceChange(const Event& event)
{
    MEDIA_LOG_I("NotifyAudioDeviceChange");
    auto [deviceInfo, reason] = AnyCast<std::pair<AudioStandard::DeviceInfo,
        AudioStandard::AudioStreamDeviceChangeReason>>(event.param);
    Format format;
    Parcel parcel;
    deviceInfo.Marshalling(parcel);
    auto parcelSize = parcel.GetReadableBytes();
    (void)format.PutBuffer(PlayerKeys::AUDIO_DEVICE_CHANGE,
        parcel.ReadBuffer(parcelSize), parcelSize);
    format.PutIntValue(PlayerKeys::AUDIO_DEVICE_CHANGE_REASON, static_cast<int32_t>(reason));
    callbackLooper_.OnInfo(INFO_TYPE_AUDIO_DEVICE_CHANGE, static_cast<int32_t>(reason), format);
}

void HiPlayerImpl::NotifyAudioServiceDied()
{
    Format format;
    callbackLooper_.OnInfo(INFO_TYPE_ERROR_MSG, MSERR_EXT_API9_IO, format);
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
#ifdef SUPPORT_VIDEO
    Format format;
    int32_t width = videoWidth_.load();
    int32_t height = videoHeight_.load();
    (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_WIDTH), width);
    (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_HEIGHT), height);
    MEDIA_LOG_I("video size changed, width = %{public}d, height = %{public}d", width, height);
    callbackLooper_.OnInfo(INFO_TYPE_RESOLUTION_CHANGE, 0, format);
#endif
}

void HiPlayerImpl::NotifyPositionUpdate()
{
    int32_t currentPosMs = 0;
    GetCurrentTime(currentPosMs);
    MEDIA_LOG_D("NotifyPositionUpdate currentPosMs: %{public}d", currentPosMs);
    Format format;
    callbackLooper_.OnInfo(INFO_TYPE_POSITION_UPDATE, currentPosMs, format);
}

void __attribute__((no_sanitize("cfi"))) HiPlayerImpl::OnStateChanged(PlayerStateId state)
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

Status HiPlayerImpl::OnCallback(std::shared_ptr<Filter> filter, const FilterCallBackCommand cmd, StreamType outType)
{
    MEDIA_LOG_I("HiPlayerImpl::OnCallback filter, outType: %{public}d", outType);
    if (cmd == FilterCallBackCommand::NEXT_FILTER_NEEDED) {
        switch (outType) {
            case StreamType::STREAMTYPE_RAW_AUDIO:
                return LinkAudioSinkFilter(filter, outType);
            case StreamType::STREAMTYPE_ENCODED_AUDIO:
                return LinkAudioDecoderFilter(filter, outType);
#ifdef SUPPORT_VIDEO
            case StreamType::STREAMTYPE_RAW_VIDEO:
                break;
            case StreamType::STREAMTYPE_ENCODED_VIDEO:
                return LinkVideoDecoderFilter(filter, outType);
#endif
            default:
                break;
        }
    }
    return Status::OK;
}

void HiPlayerImpl::OnDumpInfo(int32_t fd)
{
    MEDIA_LOG_D("HiPlayerImpl::OnDumpInfo called.");
    audioDecoder_->OnDumpInfo(fd);
    demuxer_->OnDumpInfo(fd);
#ifdef SUPPORT_VIDEO
    videoDecoder_->OnDumpInfo(fd);
#endif
}

Status HiPlayerImpl::LinkAudioDecoderFilter(const std::shared_ptr<Filter>& preFilter, StreamType type)
{
    MediaTrace trace("HiPlayerImpl::LinkAudioDecoderFilter");
    MEDIA_LOG_I("HiPlayerImpl::LinkAudioDecoderFilter");
    FALSE_RETURN_V(audioDecoder_ == nullptr, Status::OK);
    audioDecoder_ = FilterFactory::Instance().CreateFilter<AudioDecoderFilter>("player.audiodecoder",
        FilterType::FILTERTYPE_ADEC);
    FALSE_RETURN_V(audioDecoder_ != nullptr, Status::ERROR_NULL_POINTER);
    audioDecoder_->Init(playerEventReceiver_, playerFilterCallback_);

    // set decrypt config for drm audios
    if (isDrmProtected_) {
        MEDIA_LOG_D("HiPlayerImpl::LinkAudioDecoderFilter will SetDecryptConfig");
        std::unique_lock<std::mutex> lock(drmMutex_);
        static constexpr int32_t timeout = 5;
        bool notTimeout = drmConfigCond_.wait_for(lock, std::chrono::seconds(timeout), [this]() {
            return this->isDrmPrepared_ || this->stopWaitingDrmConfig_;
        });
        if (notTimeout && isDrmPrepared_) {
            MEDIA_LOG_I("HiPlayerImpl::LinkAudioDecoderFilter will SetDecryptConfig");
            bool svpFlag = svpMode_ == HiplayerSvpMode::SVP_TRUE ? true : false;
            audioDecoder_->SetDecryptionConfig(keySessionServiceProxy_, svpFlag);
        } else {
            MEDIA_LOG_E("HiPlayerImpl Drmcond wait timeout or has been stopped! Play drm protected audio failed!");
            return Status::ERROR_INVALID_OPERATION;
        }
    } else {
        MEDIA_LOG_D("HiPlayerImpl::LinkAudioDecoderFilter, and it's not drm-protected.");
    }
    return pipeline_->LinkFilters(preFilter, {audioDecoder_}, type);
}

Status HiPlayerImpl::LinkAudioSinkFilter(const std::shared_ptr<Filter>& preFilter, StreamType type)
{
    MediaTrace trace("HiPlayerImpl::LinkAudioSinkFilter");
    MEDIA_LOG_I("HiPlayerImpl::LinkAudioSinkFilter");
    FALSE_RETURN_V(audioSink_ == nullptr, Status::OK);
    audioSink_ = FilterFactory::Instance().CreateFilter<AudioSinkFilter>("player.audiosink",
        FilterType::FILTERTYPE_ASINK);
    FALSE_RETURN_V(audioSink_ != nullptr, Status::ERROR_NULL_POINTER);
    audioSink_->Init(playerEventReceiver_, playerFilterCallback_);
    std::shared_ptr<Meta> globalMeta = std::make_shared<Meta>();
    if (demuxer_ != nullptr) {
        globalMeta = demuxer_->GetGlobalMetaInfo();
    }
    if (globalMeta != nullptr) {
        globalMeta->SetData(Tag::APP_PID, appPid_);
        globalMeta->SetData(Tag::APP_UID, appUid_);
        if (audioRenderInfo_ != nullptr) {
            for (MapIt iter = audioRenderInfo_->begin(); iter != audioRenderInfo_->end(); iter++) {
                globalMeta->SetData(iter->first, iter->second);
            }
        }
        if (audioInterruptMode_ != nullptr) {
            for (MapIt iter = audioInterruptMode_->begin(); iter != audioInterruptMode_->end(); iter++) {
                globalMeta->SetData(iter->first, iter->second);
            }
        }
        audioSink_->SetParameter(globalMeta);
    }
    audioSink_->SetSyncCenter(syncManager_);
    completeState_.emplace_back(std::make_pair("AudioSink", false));
    initialAVStates_.emplace_back(std::make_pair(EventType::EVENT_AUDIO_FIRST_FRAME, false));
    return pipeline_->LinkFilters(preFilter, {audioSink_}, type);
}

#ifdef SUPPORT_VIDEO
Status HiPlayerImpl::LinkVideoDecoderFilter(const std::shared_ptr<Filter>& preFilter, StreamType type)
{
    MediaTrace trace("HiPlayerImpl::LinkVideoDecoderFilter");
    MEDIA_LOG_I("HiPlayerImpl::LinkVideoDecoderFilter");
    if (videoDecoder_ == nullptr) {
        videoDecoder_ = FilterFactory::Instance().CreateFilter<DecoderSurfaceFilter>("player.videodecoder",
            FilterType::FILTERTYPE_VDEC);
        FALSE_RETURN_V(videoDecoder_ != nullptr, Status::ERROR_NULL_POINTER);
        videoDecoder_->Init(playerEventReceiver_, playerFilterCallback_);
        videoDecoder_->SetSyncCenter(syncManager_);
        videoDecoder_->SetCallingInfo(appUid_, appPid_, bundleName_);
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
        } else {
            MEDIA_LOG_D("HiPlayerImpl::LinkVideoDecoderFilter, and it's not drm-protected.");
        }
    }
    completeState_.emplace_back(std::make_pair("VideoSink", false));
    initialAVStates_.emplace_back(std::make_pair(EventType::EVENT_VIDEO_RENDERING_START, false));
    return pipeline_->LinkFilters(preFilter, {videoDecoder_}, type);
}
#endif
}  // namespace Media
}  // namespace OHOS
