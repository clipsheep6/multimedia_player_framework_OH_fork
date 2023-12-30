/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include <iostream>
#include <sstream>
#include <gst/playback/gstplay-enum.h>
#include "nocopyable.h"
#include "string_ex.h"
#include "media_errors.h"
#include "media_log.h"
#include "player.h"
#include "meta/format.h"
#include "uri_helper.h"
#include "scope_guard.h"
#include "playbin_state.h"
#include "gst_utils.h"
#include "media_dfx.h"
#include "player_xcollie.h"
#include "param_wrapper.h"
#include "playbin_ctrler_base.h"
#ifdef SUPPORT_DRM
#include "key_session_service_proxy.h"
#endif
namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayBinCtrlerBase"};
    constexpr uint64_t RING_BUFFER_MAX_SIZE = 5242880; // 5 * 1024 * 1024
    constexpr int32_t PLAYBIN_QUEUE_MAX_SIZE = 100 * 1024 * 1024; // 100 * 1024 * 1024 Bytes
    constexpr uint64_t BUFFER_DURATION = 15000000000; // 15s
    constexpr int32_t BUFFER_LOW_PERCENT_DEFAULT = 1;
    constexpr int32_t BUFFER_HIGH_PERCENT_DEFAULT = 4;
    constexpr int32_t BUFFER_PERCENT_THRESHOLD = 100;
    constexpr int32_t NANO_SEC_PER_USEC = 1000;
    constexpr int32_t USEC_PER_MSEC = 1000;
    constexpr double DEFAULT_RATE = 1.0;
    constexpr uint32_t INTERRUPT_EVENT_SHIFT = 8;
    constexpr uint64_t CONNECT_SPEED_DEFAULT = 4 * 8 * 1024 * 1024;  // 4Mbps
    constexpr uint32_t MAX_SUBTITLE_TRACK_NUN = 8;
}

namespace OHOS {
namespace Media {

static const std::unordered_map<int32_t, int32_t> SEEK_OPTION_TO_GST_SEEK_FLAGS = {
    {
        IPlayBinCtrler::PlayBinSeekMode::PREV_SYNC,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT | GST_SEEK_FLAG_SNAP_BEFORE,
    },
    {
        IPlayBinCtrler::PlayBinSeekMode::NEXT_SYNC,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT | GST_SEEK_FLAG_SNAP_AFTER,
    },
    {
        IPlayBinCtrler::PlayBinSeekMode::CLOSET_SYNC,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT | GST_SEEK_FLAG_SNAP_NEAREST,
    },
    {
        IPlayBinCtrler::PlayBinSeekMode::CLOSET,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
    }
};

void PlayBinCtrlerBase::ElementSetup(const GstElement *playbin, GstElement *elem, gpointer userData)
{
    (void)playbin;
    CHECK_AND_RETURN_LOG(elem != nullptr, "elem is nullptr");
    CHECK_AND_RETURN_LOG(userData != nullptr, "userData is nullptr");

    auto thizStrong = PlayBinCtrlerWrapper::TakeStrongThiz(userData);
    if (thizStrong != nullptr) {
        return thizStrong->OnElementSetup(*elem);
    }
}

void PlayBinCtrlerBase::ElementUnSetup(const GstElement *playbin, GstElement *subbin,
    GstElement *child, gpointer userData)
{
    (void)playbin;
    (void)subbin;
    CHECK_AND_RETURN_LOG(child != nullptr, "elem is nullptr");
    CHECK_AND_RETURN_LOG(userData != nullptr, "userData is nullptr");

    auto thizStrong = PlayBinCtrlerWrapper::TakeStrongThiz(userData);
    if (thizStrong != nullptr) {
        return thizStrong->OnElementUnSetup(*child);
    }
}

void PlayBinCtrlerBase::SourceSetup(const GstElement *playbin, GstElement *elem, gpointer userData)
{
    CHECK_AND_RETURN_LOG(elem != nullptr, "elem is nullptr");
    CHECK_AND_RETURN_LOG(userData != nullptr, "userData is nullptr");

    auto thizStrong = PlayBinCtrlerWrapper::TakeStrongThiz(userData);
    if (thizStrong != nullptr) {
        return thizStrong->OnSourceSetup(playbin, elem, thizStrong);
    }
}

PlayBinCtrlerBase::PlayBinCtrlerBase(const PlayBinCreateParam &createParam)
    : renderMode_(createParam.renderMode),
    notifier_(createParam.notifier),
    sinkProvider_(createParam.sinkProvider)
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

PlayBinCtrlerBase::~PlayBinCtrlerBase()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
    if (Reset() == MSERR_OK) {
        sinkProvider_ = nullptr;
        notifier_ = nullptr;
    }
}

int32_t PlayBinCtrlerBase::Init()
{
    CHECK_AND_RETURN_RET_LOG(sinkProvider_ != nullptr, MSERR_INVALID_VAL, "sinkprovider is nullptr");

    idleState_ = std::make_shared<IdleState>(*this);
    initializedState_ = std::make_shared<InitializedState>(*this);
    preparingState_ = std::make_shared<PreparingState>(*this);
    preparedState_ = std::make_shared<PreparedState>(*this);
    playingState_ = std::make_shared<PlayingState>(*this);
    pausedState_ = std::make_shared<PausedState>(*this);
    stoppingState_ = std::make_shared<StoppingState>(*this);
    stoppedState_ = std::make_shared<StoppedState>(*this);
    playbackCompletedState_ = std::make_shared<PlaybackCompletedState>(*this);

    rate_ = DEFAULT_RATE;

    ChangeState(idleState_);

    msgQueue_ = std::make_unique<TaskQueue>("PlaybinCtrl");
    int32_t ret = msgQueue_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "msgqueue start failed");

    return ret;
}

bool PlayBinCtrlerBase::IsLiveSource() const
{
    if (appsrcWrap_ != nullptr && appsrcWrap_->IsLiveMode()) {
        return true;
    }
    return false;
}

bool PlayBinCtrlerBase::EnableBufferingBySysParam() const
{
    std::string cmd;
    int32_t ret = OHOS::system::GetStringParameter("sys.media.player.buffering.enable", cmd, "");
    if (ret == 0 && !cmd.empty()) {
        return cmd == "TRUE" ? TRUE : FALSE;
    }
    return FALSE;
}

int32_t PlayBinCtrlerBase::SetSource(const std::string &url)
{
    std::unique_lock<std::mutex> lock(mutex_);
    uri_ = url;
    drmInfo_.clear();
    if (url.find("http") == 0 || url.find("https") == 0 || EnableBufferingBySysParam()) {
        isNetWorkPlay_ = true;
    }

    MEDIA_LOGI("Set source: %{public}s", url.c_str());
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::SetSource(const std::shared_ptr<GstAppsrcEngine> &appsrcWrap)
{
    std::unique_lock<std::mutex> lock(mutex_);
    appsrcWrap_ = appsrcWrap;
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::AddSubSource(const std::string &url)
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);
    ON_SCOPE_EXIT(0) {
        OnAddSubDone();
    };

    CHECK_AND_RETURN_RET(sinkProvider_ != nullptr, MSERR_INVALID_VAL);
    CHECK_AND_RETURN_RET(subtitleTrackNum_ < MAX_SUBTITLE_TRACK_NUN,
        (OnError(MSERR_INVALID_OPERATION, "subtitle tracks exceed the max num limit!"), MSERR_INVALID_OPERATION));

    if (subtitleSink_ == nullptr) {
        subtitleSink_ = sinkProvider_->CreateSubtitleSink();
        g_object_set(playbin_, "text-sink", subtitleSink_, nullptr);
    }

    isAddingSubtitle_ = true;
    g_object_set(playbin_, "add-suburi", url.c_str(), nullptr);
    CANCEL_SCOPE_EXIT_GUARD(0);

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PrepareAsync()
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);
    return PrepareAsyncInternal();
}

int32_t PlayBinCtrlerBase::Play()
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(GetCurrState() != playingState_, MSERR_OK, "already at playing state");

    if (isBuffering_) {
        ChangeState(playingState_);
        return MSERR_OK;
    }

    auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
    int32_t ret = currState->Play();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Play failed");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::Pause()
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(GetCurrState() != pausedState_, MSERR_OK, "already at paused state");
    CHECK_AND_RETURN_RET_LOG(GetCurrState() != preparedState_, MSERR_OK, "already at prepared state");

    auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
    int32_t ret = currState->Pause();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Pause failed");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::Seek(int64_t timeUs, int32_t seekOption)
{
    MEDIA_LOGD("enter");
    CHECK_AND_RETURN_RET_LOG(SEEK_OPTION_TO_GST_SEEK_FLAGS.find(seekOption) !=
        SEEK_OPTION_TO_GST_SEEK_FLAGS.end(), MSERR_INVALID_VAL,
        "unsupported seek option: %{public}d", seekOption);
    std::unique_lock<std::mutex> lock(mutex_);
    std::unique_lock<std::mutex> cacheLock(cacheCtrlMutex_);

    auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
    int32_t ret = currState->Seek(timeUs, seekOption);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Seek failed");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::Stop(bool needWait)
{
    MEDIA_LOGD("enter");
    std::unique_lock<std::mutex> lock(mutex_);
    if (trackParse_ != nullptr) {
        trackParse_->Stop();
    }
    audioIndex_ = -1;

#ifdef SUPPORT_DRM
    // drm condition
    if (GetCurrState() == preparingState_ && isDrmPrepared_ == false) {
        MEDIA_LOGI("Stop and current state is preparing, will trig drmPreparedCond");
        std::unique_lock<std::mutex> drmLock(drmMutex_);
        stopWaitingDrmConfig_ = true;
        drmConfigCond_.notify_all();
    }
#endif

    if (GetCurrState() == preparingState_ && needWait) {
        MEDIA_LOGI("begin wait stop for current status is preparing");
        static constexpr int32_t timeout = 0;  // 0s wait
        preparedCond_.wait_for(lock, std::chrono::seconds(timeout));
        MEDIA_LOGD("end wait stop for current status is preparing");
    }

    if (appsrcWrap_ != nullptr) {
        appsrcWrap_->Stop();
    }

    std::unique_lock<std::mutex> stateChangeLock(stateChangePropertyMutex_);
    stopBuffering_ = true;
    if (videoSink_ == nullptr) {
        g_object_set(playbin_, "state-change", GST_PLAYER_STATUS_READY, nullptr);
    }
    if (seekFuture_.valid()) {
        (void)seekFuture_.get();
    }

    auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
    int stopRes = currState->Stop();
    if (stopRes == MSERR_OK) {
        MEDIA_LOGD("Stop Start");
        if (GetCurrState() != stoppedState_) {
            LISTENER(stoppingCond_.wait(lock), "stoppingCond_.wait", PlayerXCollie::timerTimeout)
        }
        MEDIA_LOGD("Stop End");
    }
    trackParse_ = nullptr;
    CHECK_AND_RETURN_RET_LOG(GetCurrState() == stoppedState_, MSERR_INVALID_STATE, "Stop failed");
    return MSERR_OK;
}

GstSeekFlags PlayBinCtrlerBase::ChooseSetRateFlags(double rate)
{
    (void)rate;
    GstSeekFlags seekFlags = static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH |
        GST_SEEK_FLAG_TRICKMODE | GST_SEEK_FLAG_SNAP_AFTER);
    return seekFlags;
}

int32_t PlayBinCtrlerBase::SetRateInternal(double rate)
{
    MEDIA_LOGD("execute set rate, rate: %{public}lf", rate);

    gint64 position;
    gboolean ret;

    isRating_ = true;
    if (isDuration_.load()) {
        position = duration_ * NANO_SEC_PER_USEC;
    } else {
        ret = gst_element_query_position(GST_ELEMENT_CAST(playbin_), GST_FORMAT_TIME, &position);
        if (!ret) {
            MEDIA_LOGW("query position failed, use lastTime");
            position = lastTime_;
        }
    }

    GstSeekFlags flags = ChooseSetRateFlags(rate);
    int64_t start = rate > 0 ? position : 0;
    int64_t stop = rate > 0 ? static_cast<int64_t>(GST_CLOCK_TIME_NONE) : position;

    GstEvent *event = gst_event_new_seek(rate, GST_FORMAT_TIME, flags,
        GST_SEEK_TYPE_SET, start, GST_SEEK_TYPE_SET, stop);
    CHECK_AND_RETURN_RET_LOG(event != nullptr, MSERR_NO_MEMORY, "set rate failed");

    ret = gst_element_send_event(GST_ELEMENT_CAST(playbin_), event);
    CHECK_AND_RETURN_RET_LOG(ret, MSERR_SEEK_FAILED, "set rate failed");

    rate_ = rate;

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::SetRate(double rate)
{
    MEDIA_LOGD("enter");
    std::unique_lock<std::mutex> lock(mutex_);
    std::unique_lock<std::mutex> cacheLock(cacheCtrlMutex_);

    auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
    int32_t ret = currState->SetRate(rate);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "SetRate failed");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::SetLoop(bool loop)
{
    enableLooping_ = loop;
    return MSERR_OK;
}

void PlayBinCtrlerBase::SetVolume(const float &leftVolume, const float &rightVolume)
{
    (void)rightVolume;
    std::unique_lock<std::mutex> lock(mutex_);
    float volume = leftVolume;
    if (audioSink_ != nullptr) {
        MEDIA_LOGD("SetVolume(%{public}f) to audio sink", volume);
        g_object_set(audioSink_, "volume", volume, nullptr);
    }
}

int32_t PlayBinCtrlerBase::SetAudioRendererInfo(const uint32_t rendererInfo, const int32_t rendererFlag)
{
    std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
    rendererInfo_ = rendererInfo;
    rendererFlag_ = rendererFlag;
    if (audioSink_ != nullptr) {
        MEDIA_LOGI("SetAudioRendererInfo to audio sink");
        g_object_set(audioSink_, "audio-renderer-desc", rendererInfo, nullptr);
        g_object_set(audioSink_, "audio-renderer-flag", rendererFlag, nullptr);
    }
    return MSERR_OK;
}

void PlayBinCtrlerBase::SetAudioInterruptMode(const int32_t interruptMode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (audioSink_ != nullptr) {
        g_object_set(audioSink_, "audio-interrupt-mode", interruptMode, nullptr);
    }
}

int32_t PlayBinCtrlerBase::SetAudioEffectMode(const int32_t effectMode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (audioSink_ != nullptr) {
        g_object_set(audioSink_, "audio-effect-mode", effectMode, nullptr);
        int32_t effectModeNow = -1;
        g_object_get(audioSink_, "audio-effect-mode", &effectModeNow, nullptr);
        CHECK_AND_RETURN_RET_LOG(effectModeNow == effectMode, MSERR_INVALID_VAL, "failed to set audio-effect-mode");
    }
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::SelectBitRate(uint32_t bitRate)
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("enter SelectBitRate, bandwidth: %{public}u", bitRate);
    if (bitRateVec_.empty()) {
        MEDIA_LOGE("BitRate is empty");
        return MSERR_INVALID_OPERATION;
    }
    if (connectSpeed_ == bitRate && !isSelectBitRate_) {
        PlayBinMessage msg = { PLAYBIN_MSG_BITRATEDONE, 0, static_cast<int32_t>(bitRate), {} };
        ReportMessage(msg);
    } else {
        MEDIA_LOGD("set bandwidth to: %{public}u", bitRate);
        isSelectBitRate_ = true;
        g_object_set(playbin_, "connection-speed", static_cast<uint64_t>(bitRate), nullptr);
    }
    return MSERR_OK;
}

#ifdef SUPPORT_DRM
int32_t PlayBinCtrlerBase::SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySessionProxy,
    bool svp)
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " is now the playerCtrlerBase for SetDecryptConfig", FAKE_POINTER(this));
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(keySessionProxy != nullptr, MSERR_INVALID_OPERATION, "keySessionProxy is nullptr");

    keySessionServiceProxy_ = keySessionProxy;
    if (svp) {
        svpMode_ = SVP_TRUE;
    } else {
        svpMode_ = SVP_FALSE;
    }
    keySessionServiceProxy_->CreateMediaDecryptModule(decryptModuleProxy_);
    CHECK_AND_RETURN_RET_LOG(decryptModuleProxy_ != nullptr, MSERR_INVALID_OPERATION, "can not get DRM decryptModule");
    PlayBinMessage msg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_SET_DRM_CONFIG_DONE, 0, {} };
    ReportMessage(msg);

    std::unique_lock<std::mutex> drmLock(drmMutex_);
    MEDIA_LOGI("For Drmcond SetDecryptConfig will trig drmPreparedCond");
    isDrmPrepared_ = true;
    drmConfigCond_.notify_all();

    return MSERR_OK;
}
#endif

int32_t PlayBinCtrlerBase::Reset() noexcept
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);
    {
        std::unique_lock<std::mutex> lk(listenerMutex_);
        elemSetupListener_ = nullptr;
        elemUnSetupListener_ = nullptr;
        autoPlugSortListener_ = nullptr;
    }
    // Do it here before the ChangeState to IdleState, for avoding the deadlock when msg handler
    // try to call the ChangeState.
    ExitInitializedState();
    ChangeState(idleState_);

    if (msgQueue_ != nullptr) {
        (void)msgQueue_->Stop();
    }

    uri_.clear();
    drmInfo_.clear();
    isErrorHappened_ = false;
    enableLooping_ = false;
    {
        std::unique_lock<std::mutex> appsrcLock(appsrcMutex_);
        appsrcWrap_ = nullptr;
    }

    rate_ = DEFAULT_RATE;
    seekPos_ = 0;
    lastTime_ = 0;
    isSeeking_ = false;
    isRating_ = false;
    isAddingSubtitle_ = false;
    isBuffering_ = false;
    isSelectBitRate_ = false;
    cachePercent_ = BUFFER_PERCENT_THRESHOLD;
    isDuration_ = false;
    isUserSetPause_ = false;
    subtitleTrackNum_ = 0;
    isDrmPrepared_ = false;
    stopWaitingDrmConfig_ = false;
    MEDIA_LOGD("exit");
    return MSERR_OK;
}

void PlayBinCtrlerBase::SetElemSetupListener(ElemSetupListener listener)
{
    std::unique_lock<std::mutex> lock(mutex_);
    std::unique_lock<std::mutex> lk(listenerMutex_);
    elemSetupListener_ = listener;
}

void PlayBinCtrlerBase::SetElemUnSetupListener(ElemSetupListener listener)
{
    std::unique_lock<std::mutex> lock(mutex_);
    std::unique_lock<std::mutex> lk(listenerMutex_);
    elemUnSetupListener_ = listener;
}

void PlayBinCtrlerBase::SetAutoPlugSortListener(AutoPlugSortListener listener)
{
    std::unique_lock<std::mutex> lock(mutex_);
    std::unique_lock<std::mutex> lk(listenerMutex_);
    autoPlugSortListener_ = listener;
}

void PlayBinCtrlerBase::DoInitializeForHttp()
{
    if (isNetWorkPlay_) {
        g_object_set(playbin_, "ring-buffer-max-size", RING_BUFFER_MAX_SIZE, nullptr);
        g_object_set(playbin_, "buffering-flags", true, "buffer-size", PLAYBIN_QUEUE_MAX_SIZE,
            "buffer-duration", BUFFER_DURATION, "low-percent", BUFFER_LOW_PERCENT_DEFAULT,
            "high-percent", BUFFER_HIGH_PERCENT_DEFAULT, nullptr);

        std::string autoSelectBitrate;
        int32_t res = OHOS::system::GetStringParameter(
            "sys.media.hls.set.autoSelectBitrate", autoSelectBitrate, "");
        if (res == 0 && !autoSelectBitrate.empty() && autoSelectBitrate == "TRUE") {
            SetAutoSelectBitrate(true);
            MEDIA_LOGD("set autoSelectBitrate to true");
        } else {
            SetAutoSelectBitrate(false);
            MEDIA_LOGD("set autoSelectBitrate to false");
        }

        PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
        CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");

        gulong id = g_signal_connect_data(playbin_, "bitrate-parse-complete",
            G_CALLBACK(&PlayBinCtrlerBase::OnBitRateParseCompleteCb), wrapper,
            (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory, static_cast<GConnectFlags>(0));
        CheckAndAddSignalIds(id, wrapper, GST_ELEMENT_CAST(playbin_));

        PlayBinCtrlerWrapper *wrap = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
        CHECK_AND_RETURN_LOG(wrap != nullptr, "can not create this wrap");

        id = g_signal_connect_data(videoSink_, "bandwidth-change",
            G_CALLBACK(&PlayBinCtrlerBase::OnSelectBitrateDoneCb), wrap,
            (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory, static_cast<GConnectFlags>(0));
        CheckAndAddSignalIds(id, wrap, videoSink_);
    }
}

int32_t PlayBinCtrlerBase::EnterInitializedState()
{
    if (isInitialized_) {
        (void)DoInitializeForDataSource();
        return MSERR_OK;
    }
    MediaTrace("PlayBinCtrlerBase::InitializedState");
    MEDIA_LOGD("EnterInitializedState enter");

    ON_SCOPE_EXIT(0) {
        ExitInitializedState();
        PlayBinMessage msg { PlayBinMsgType::PLAYBIN_MSG_ERROR,
            PlayBinMsgErrorSubType::PLAYBIN_SUB_MSG_ERROR_WITH_MESSAGE,
            MSERR_CREATE_PLAYER_ENGINE_FAILED, std::string("failed to EnterInitializedState") };
        ReportMessage(msg);
        MEDIA_LOGE("enter initialized state failed");
    };

    int32_t ret = OnInit();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
    CHECK_AND_RETURN_RET(playbin_ != nullptr, static_cast<int32_t>(MSERR_CREATE_PLAYER_ENGINE_FAILED));

    ret = DoInitializeForDataSource();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "DoInitializeForDataSource failed!");

    SetupCustomElement();
    SetupSourceSetupSignal();
    ret = SetupSignalMessage();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
    ret = SetupElementUnSetupSignal();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
    SetAudioRendererInfo(rendererInfo_, rendererFlag_);

    uint32_t flags = 0;
    g_object_get(playbin_, "flags", &flags, nullptr);
    if ((renderMode_ & PlayBinRenderMode::DEFAULT_RENDER) != 0) {
        flags &= ~GST_PLAY_FLAG_VIS;
    }
    if ((renderMode_ & PlayBinRenderMode::NATIVE_STREAM) != 0) {
        flags |= GST_PLAY_FLAG_NATIVE_VIDEO | GST_PLAY_FLAG_NATIVE_AUDIO;
        flags &= ~(GST_PLAY_FLAG_SOFT_COLORBALANCE | GST_PLAY_FLAG_SOFT_VOLUME);
    }
    if ((renderMode_ & PlayBinRenderMode::DISABLE_TEXT) != 0) {
        flags &= ~GST_PLAY_FLAG_TEXT;
    }
    g_object_set(playbin_, "flags", flags, nullptr);

    // There may be a risk of data competition, but the uri is unlikely to be reconfigured.
    if (!uri_.empty()) {
        g_object_set(playbin_, "uri", uri_.c_str(), nullptr);
    }

    DoInitializeForHttp();

    isInitialized_ = true;
    ChangeState(initializedState_);

    CANCEL_SCOPE_EXIT_GUARD(0);
    MEDIA_LOGD("EnterInitializedState exit");

    return MSERR_OK;
}

void PlayBinCtrlerBase::ExitInitializedState()
{
    MEDIA_LOGD("ExitInitializedState enter");

    if (!isInitialized_) {
        return;
    }
    isInitialized_ = false;

    mutex_.unlock();
    if (msgProcessor_ != nullptr) {
        msgProcessor_->Reset();
        msgProcessor_ = nullptr;
    }
    mutex_.lock();

    if (sinkProvider_ != nullptr) {
        sinkProvider_->SetMsgNotifier(nullptr);
    }
    for (auto &item : signalIds_) {
        for (auto id : item.second) {
            g_signal_handler_disconnect(item.first, id);
        }
    }
    signalIds_.clear();

    MEDIA_LOGD("unref playbin start");
    if (playbin_ != nullptr) {
        (void)gst_element_set_state(GST_ELEMENT_CAST(playbin_), GST_STATE_NULL);
        gst_object_unref(playbin_);
        playbin_ = nullptr;
    }
    MEDIA_LOGD("unref playbin stop");

    MEDIA_LOGD("ExitInitializedState exit");
}

int32_t PlayBinCtrlerBase::PrepareAsyncInternal()
{
    if ((GetCurrState() == preparingState_) || (GetCurrState() == preparedState_)) {
        MEDIA_LOGI("already at preparing state, skip");
        return MSERR_OK;
    }

    CHECK_AND_RETURN_RET_LOG((!uri_.empty() || appsrcWrap_), MSERR_INVALID_OPERATION, "Set uri firsty!");
    trackParse_ = PlayerTrackParse::Create();
    int32_t ret = EnterInitializedState();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    stopBuffering_ = false;
    auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
    ret = currState->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "PrepareAsyncInternal failed");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::SeekInternal(int64_t timeUs, int32_t seekOption)
{
    MEDIA_LOGI("execute seek, time: %{public}" PRIi64 ", option: %{public}d", timeUs, seekOption);

    int32_t seekFlags = SEEK_OPTION_TO_GST_SEEK_FLAGS.at(seekOption);
    timeUs = timeUs > duration_ ? duration_ : timeUs;
    timeUs = timeUs < 0 ? 0 : timeUs;

    constexpr int32_t usecToNanoSec = 1000;
    int64_t timeNs = timeUs * usecToNanoSec;
    seekPos_ = timeUs;
    isSeeking_ = true;
    if (videoSink_ == nullptr || seekOption == IPlayBinCtrler::PlayBinSeekMode::CLOSET) {
        isClosetSeeking_ = true;
    }
    GstEvent *event = gst_event_new_seek(rate_, GST_FORMAT_TIME, static_cast<GstSeekFlags>(seekFlags),
        GST_SEEK_TYPE_SET, timeNs, GST_SEEK_TYPE_SET, GST_CLOCK_TIME_NONE);
    CHECK_AND_RETURN_RET_LOG(event != nullptr, MSERR_NO_MEMORY, "seek failed");

    if (videoSink_ != nullptr) {
        gboolean ret = gst_element_send_event(GST_ELEMENT_CAST(playbin_), event);
        CHECK_AND_RETURN_RET_LOG(ret, MSERR_SEEK_FAILED, "seek failed");
    } else {
        seekFuture_ = std::async(std::launch::async, [this, event]() -> gboolean {
            gboolean ret = FALSE;
            CHECK_AND_RETURN_RET_LOG(playbin_ != nullptr, FALSE, "playbin is nullptr");
            pthread_setname_np(pthread_self(), "AudioAsyncSeek");
            MEDIA_LOGI("audio seek start");
            ret = gst_element_send_event(GST_ELEMENT_CAST(playbin_), event);
            MEDIA_LOGI("audio seek end");
            return ret;
        });
    }
    return MSERR_OK;
}

void PlayBinCtrlerBase::SetupInterruptEventCb()
{
    PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
    CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");

    gulong id = g_signal_connect_data(audioSink_, "interrupt-event",
        G_CALLBACK(&PlayBinCtrlerBase::OnInterruptEventCb), wrapper,
        (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory, static_cast<GConnectFlags>(0));
    CheckAndAddSignalIds(id, wrapper, audioSink_);
}

void PlayBinCtrlerBase::SetupAudioDeviceEventCb()
{
    PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
    CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");

    gulong id = g_signal_connect_data(audioSink_, "device-change-event",
        G_CALLBACK(&PlayBinCtrlerBase::OnInterruptEventCb), wrapper,
        (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory, static_cast<GConnectFlags>(0));
    CheckAndAddSignalIds(id, wrapper, audioSink_);
}

void PlayBinCtrlerBase::SetupAudioSegmentEventCb()
{
    PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
    CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");

    gulong id = g_signal_connect_data(audioSink_, "segment-updated",
        G_CALLBACK(&PlayBinCtrlerBase::OnAudioSegmentEventCb), wrapper,
        (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory, static_cast<GConnectFlags>(0));
    CheckAndAddSignalIds(id, wrapper, audioSink_);
}

void PlayBinCtrlerBase::SetupAudioDiedEventCb()
{
    PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
    CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");

    gulong id = g_signal_connect_data(audioSink_, "audio-service-died",
        G_CALLBACK(&PlayBinCtrlerBase::OnAudioDiedEventCb), wrapper,
        (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory, static_cast<GConnectFlags>(0));
    CheckAndAddSignalIds(id, wrapper, audioSink_);
}

void PlayBinCtrlerBase::SetupCustomElement()
{
    // There may be a risk of data competition, but the sinkProvider is unlikely to be reconfigured.
    if (sinkProvider_ != nullptr) {
        audioSink_ = sinkProvider_->CreateAudioSink();
        if (audioSink_ != nullptr) {
            g_object_set(playbin_, "audio-sink", audioSink_, nullptr);
            SetupInterruptEventCb();
            SetupAudioDeviceEventCb();
            SetupAudioSegmentEventCb();
            SetupAudioDiedEventCb();
        }
        videoSink_ = sinkProvider_->CreateVideoSink();
        if (videoSink_ != nullptr) {
            g_object_set(playbin_, "video-sink", videoSink_, nullptr);
        } else if (audioSink_ != nullptr) {
            g_object_set(playbin_, "video-sink", audioSink_, nullptr);
        }
        auto msgNotifier = std::bind(&PlayBinCtrlerBase::OnSinkMessageReceived, this, std::placeholders::_1);
        sinkProvider_->SetMsgNotifier(msgNotifier);
    } else {
        MEDIA_LOGD("no sinkprovider, delay the sink selection until the playbin enters pause state.");
    }

    if ((renderMode_ & PlayBinRenderMode::NATIVE_STREAM) == 0) {
        GstElement *audioFilter = gst_element_factory_make("scaletempo", "scaletempo");
        if (audioFilter != nullptr) {
            g_object_set(playbin_, "audio-filter", audioFilter, nullptr);
        } else {
            MEDIA_LOGD("can not create scaletempo, the audio playback speed can not be adjusted");
        }
    }
}

void PlayBinCtrlerBase::SetupSourceSetupSignal()
{
    PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
    CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");

    gulong id = g_signal_connect_data(playbin_, "source-setup",
        G_CALLBACK(&PlayBinCtrlerBase::SourceSetup), wrapper, (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory,
        static_cast<GConnectFlags>(0));
    CheckAndAddSignalIds(id, wrapper, GST_ELEMENT_CAST(playbin_));
}

int32_t PlayBinCtrlerBase::SetupSignalMessage()
{
    MEDIA_LOGD("SetupSignalMessage enter");

    PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
    CHECK_AND_RETURN_RET_LOG(wrapper != nullptr, MSERR_NO_MEMORY, "can not create this wrapper");

    gulong id = g_signal_connect_data(playbin_, "element-setup",
        G_CALLBACK(&PlayBinCtrlerBase::ElementSetup), wrapper, (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory,
        static_cast<GConnectFlags>(0));
    CheckAndAddSignalIds(id, wrapper, GST_ELEMENT_CAST(playbin_));

    PlayBinCtrlerWrapper *wrap = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
    CHECK_AND_RETURN_RET_LOG(wrap != nullptr, MSERR_NO_MEMORY, "can not create this wrapper");
    id = g_signal_connect_data(playbin_, "audio-changed", G_CALLBACK(&PlayBinCtrlerBase::AudioChanged),
        wrap, (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory, static_cast<GConnectFlags>(0));
    CheckAndAddSignalIds(id, wrap, GST_ELEMENT_CAST(playbin_));

    GstBus *bus = gst_pipeline_get_bus(playbin_);
    CHECK_AND_RETURN_RET_LOG(bus != nullptr, MSERR_UNKNOWN, "can not get bus");

    auto msgNotifier = std::bind(&PlayBinCtrlerBase::OnMessageReceived, this, std::placeholders::_1);
    msgProcessor_ = std::make_unique<GstMsgProcessor>(*bus, msgNotifier);

    gst_object_unref(bus);
    bus = nullptr;

    int32_t ret = msgProcessor_->Init();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    // only concern the msg from playbin
    msgProcessor_->AddMsgFilter(ELEM_NAME(GST_ELEMENT_CAST(playbin_)));

    MEDIA_LOGD("SetupSignalMessage exit");
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::SetupElementUnSetupSignal()
{
    MEDIA_LOGD("SetupElementUnSetupSignal enter");

    PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
    CHECK_AND_RETURN_RET_LOG(wrapper != nullptr, MSERR_NO_MEMORY, "can not create this wrapper");

    gulong id = g_signal_connect_data(playbin_, "deep-element-removed",
        G_CALLBACK(&PlayBinCtrlerBase::ElementUnSetup), wrapper, (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory,
        static_cast<GConnectFlags>(0));
    CheckAndAddSignalIds(id, wrapper, GST_ELEMENT_CAST(playbin_));

    return MSERR_OK;
}

void PlayBinCtrlerBase::QueryDuration()
{
    auto state = GetCurrState();
    if (state != preparedState_ && state != playingState_ && state != pausedState_ &&
        state != playbackCompletedState_) {
        MEDIA_LOGE("reuse the last query result: %{public}" PRIi64 " microsecond", duration_);
        return;
    }

    gint64 duration = -1;
    gboolean ret = gst_element_query_duration(GST_ELEMENT_CAST(playbin_), GST_FORMAT_TIME, &duration);
    CHECK_AND_RETURN_LOG(ret, "query duration failed");

    if (duration >= 0) {
        duration_ = duration / NANO_SEC_PER_USEC;
    }
    MEDIA_LOGD("update the duration: %{public}" PRIi64 " microsecond", duration_);
}

int64_t PlayBinCtrlerBase::QueryPosition()
{
    gint64 position = 0;
    gboolean ret = gst_element_query_position(GST_ELEMENT_CAST(playbin_), GST_FORMAT_TIME, &position);
    if (!ret) {
        MEDIA_LOGW("query position failed");
        return lastTime_ / USEC_PER_MSEC;
    }

    int64_t curTime = position / NANO_SEC_PER_USEC;
    if (duration_ >= 0) {
        curTime = std::min(curTime, duration_);
    }
    lastTime_ = curTime;
    MEDIA_LOGD("update the position: %{public}" PRIi64 " microsecond", curTime);
    return curTime / USEC_PER_MSEC;
}

void PlayBinCtrlerBase::ProcessEndOfStream()
{
    MEDIA_LOGD("End of stream");
    isDuration_ = true;

    if (!enableLooping_.load() && !isSeeking_) { // seek duration done->seeking->eos
        ChangeState(playbackCompletedState_);
    }
}

int32_t PlayBinCtrlerBase::DoInitializeForDataSource()
{
    if (appsrcWrap_ != nullptr) {
        (void)appsrcWrap_->Prepare();
        if (isInitialized_) {
            return MSERR_OK;
        }
        auto msgNotifier = std::bind(&PlayBinCtrlerBase::OnAppsrcMessageReceived, this, std::placeholders::_1);
        CHECK_AND_RETURN_RET_LOG(appsrcWrap_->SetCallback(msgNotifier) == MSERR_OK,
            MSERR_INVALID_OPERATION, "set appsrc error callback failed");

        g_object_set(playbin_, "uri", "appsrc://", nullptr);
    }
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(trackParse_ != nullptr, MSERR_INVALID_OPERATION, "trackParse_ is nullptr");
    return trackParse_->GetVideoTrackInfo(videoTrack);
}

int32_t PlayBinCtrlerBase::GetAudioTrackInfo(std::vector<Format> &audioTrack)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(trackParse_ != nullptr, MSERR_INVALID_OPERATION, "trackParse_ is nullptr");
    return trackParse_->GetAudioTrackInfo(audioTrack);
}

int32_t PlayBinCtrlerBase::GetSubtitleTrackInfo(std::vector<Format> &subtitleTrack)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(trackParse_ != nullptr, MSERR_INVALID_OPERATION, "trackParse_ is nullptr");
    return trackParse_->GetSubtitleTrackInfo(subtitleTrack);
}

int32_t PlayBinCtrlerBase::SelectTrack(int32_t index)
{
    std::unique_lock<std::mutex> lock(mutex_);
    ON_SCOPE_EXIT(0) {
        OnTrackDone();
    };

    CHECK_AND_RETURN_RET_LOG(trackParse_ != nullptr, MSERR_INVALID_OPERATION, "trackParse_ is nullptr");
    int32_t trackType = -1;
    int32_t innerIndex = -1;
    int32_t ret = trackParse_->GetTrackInfo(index, innerIndex, trackType);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, (OnError(ret, "Invalid track index!"), ret));
    CHECK_AND_RETURN_RET(innerIndex >= 0,
        (OnError(MSERR_INVALID_OPERATION, "This track is currently not supported!"), MSERR_INVALID_OPERATION));

    if (trackType == MediaType::MEDIA_TYPE_AUD) {
        ret = MSERR_INVALID_OPERATION;
        CHECK_AND_RETURN_RET(GetCurrState() == preparedState_,
            (OnError(ret, "Audio tracks can only be selected in the prepared state!"), ret));
 
        int32_t currentIndex = -1;
        g_object_get(playbin_, "current-audio", &currentIndex, nullptr);
        CHECK_AND_RETURN_RET(innerIndex != currentIndex,
            (OnError(MSERR_OK, "This track has already been selected!"), MSERR_OK));

        lastStartTime_ = gst_element_get_start_time(GST_ELEMENT_CAST(playbin_));
        g_object_set(playbin_, "current-audio", innerIndex, nullptr);
        // The seek operation clears the original audio data and re parses the new track data.
        isTrackChanging_ = true;
        trackChangeType_ = MediaType::MEDIA_TYPE_AUD;
        SeekInternal(seekPos_, IPlayBinCtrler::PlayBinSeekMode::CLOSET_SYNC);
        CANCEL_SCOPE_EXIT_GUARD(0);
    } else if (trackType == MediaType::MEDIA_TYPE_SUBTITLE) {
        int32_t currentIndex = -1;
        g_object_get(playbin_, "current-text", &currentIndex, nullptr);
        CHECK_AND_RETURN_RET((!hasSubtitleTrackSelected_ || innerIndex != currentIndex),
            (OnError(MSERR_OK, "This track has already been selected!"), MSERR_OK));

        MEDIA_LOGI("start select subtitle track %{public}d", index);
        isTrackChanging_ = true;
        trackChangeType_ = MediaType::MEDIA_TYPE_SUBTITLE;
        g_object_set(subtitleSink_, "change-track", true, nullptr);
        lastStartTime_ = gst_element_get_start_time(GST_ELEMENT_CAST(playbin_));
        gst_element_set_start_time(GST_ELEMENT_CAST(playbin_), GST_CLOCK_TIME_NONE);
        g_object_set(playbin_, "current-text", innerIndex, nullptr);
        hasSubtitleTrackSelected_ = true;
        g_object_set(subtitleSink_, "enable-display", hasSubtitleTrackSelected_, nullptr);
        CANCEL_SCOPE_EXIT_GUARD(0);
    } else {
        OnError(MSERR_INVALID_VAL, "The track type does not support this operation!");
        return MSERR_INVALID_OPERATION;
    }
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::DeselectTrack(int32_t index)
{
    std::unique_lock<std::mutex> lock(mutex_);
    ON_SCOPE_EXIT(0) {
        OnTrackDone();
    };

    CHECK_AND_RETURN_RET_LOG(trackParse_ != nullptr, MSERR_INVALID_OPERATION, "trackParse_ is nullptr");
    int32_t trackType = -1;
    int32_t innerIndex = -1;
    int32_t ret = trackParse_->GetTrackInfo(index, innerIndex, trackType);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, (OnError(ret, "Invalid track index!"), ret));
    CHECK_AND_RETURN_RET(innerIndex >= 0,
        (OnError(MSERR_INVALID_OPERATION, "This track has not been selected yet!"), MSERR_INVALID_OPERATION));

    if (trackType == MediaType::MEDIA_TYPE_AUD) {
        ret = MSERR_INVALID_OPERATION;
        CHECK_AND_RETURN_RET(GetCurrState() == preparedState_,
            (OnError(ret, "Audio tracks can only be deselected in the prepared state!"), ret));

        int32_t currentIndex = -1;
        g_object_get(playbin_, "current-audio", &currentIndex, nullptr);
        CHECK_AND_RETURN_RET(innerIndex == currentIndex,
            (OnError(ret, "This track has not been selected yet!"), ret));

        CHECK_AND_RETURN_RET(currentIndex != 0,
            (OnError(MSERR_OK, "The current audio track is already the default track!"), MSERR_OK));

        g_object_set(playbin_, "current-audio", 0, nullptr); // 0 is the default track
        // The seek operation clears the original audio data and re parses the new track data.
        isTrackChanging_ = true;
        trackChangeType_ = MediaType::MEDIA_TYPE_AUD;
        SeekInternal(seekPos_, IPlayBinCtrler::PlayBinSeekMode::CLOSET_SYNC);
        CANCEL_SCOPE_EXIT_GUARD(0);
    } else if (trackType == MediaType::MEDIA_TYPE_SUBTITLE) {
        ret = MSERR_INVALID_OPERATION;
        int32_t currentIndex = -1;
        g_object_get(playbin_, "current-text", &currentIndex, nullptr);
        CHECK_AND_RETURN_RET((hasSubtitleTrackSelected_ && innerIndex == currentIndex),
            (OnError(ret, "This track has not been selected yet!"), ret));

        hasSubtitleTrackSelected_ = false;
        g_object_set(subtitleSink_, "enable-display", hasSubtitleTrackSelected_, nullptr);
        trackChangeType_ = MediaType::MEDIA_TYPE_SUBTITLE;
        ReportTrackChange();
        CANCEL_SCOPE_EXIT_GUARD(0);
    } else {
        OnError(MSERR_INVALID_VAL, "The track type does not support this operation!");
        return MSERR_INVALID_VAL;
    }
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::GetCurrentTrack(int32_t trackType, int32_t &index)
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGI("GetCurrentTrack in");
    CHECK_AND_RETURN_RET(trackParse_ != nullptr, MSERR_INVALID_OPERATION);
    int32_t innerIndex = -1;
    if (trackType == MediaType::MEDIA_TYPE_AUD) {
        if (isTrackChanging_) {
            // During the change track process, return the original value.
            innerIndex = audioIndex_;
        } else {
            g_object_get(playbin_, "current-audio", &innerIndex, nullptr);
        }
    } else if (trackType == MediaType::MEDIA_TYPE_VID) {
        g_object_get(playbin_, "current-video", &innerIndex, nullptr);
    } else {
        if (hasSubtitleTrackSelected_) {
            g_object_get(playbin_, "current-text", &innerIndex, nullptr);
        } else {
            innerIndex = -1;
        }
    }

    if (innerIndex >= 0) {
        return trackParse_->GetTrackIndex(innerIndex, trackType, index);
    } else {
        // There are no tracks currently playing, return to -1.
        index = innerIndex;
        return MSERR_OK;
    }
}

void PlayBinCtrlerBase::HandleCacheCtrl(int32_t percent)
{
    MEDIA_LOGD("HandleCacheCtrl percent is %{public}d", percent);
    if (!isBuffering_) {
        HandleCacheCtrlWhenNoBuffering(percent);
    } else {
        HandleCacheCtrlWhenBuffering(percent);
    }
}

void PlayBinCtrlerBase::HandleCacheCtrlCb(const InnerMessage &msg)
{
    if (isNetWorkPlay_) {
        cachePercent_ = msg.detail1;
        HandleCacheCtrl(cachePercent_);
    }
}

void PlayBinCtrlerBase::HandleCacheCtrlWhenNoBuffering(int32_t percent)
{
    if (isSelectBitRate_) {
        MEDIA_LOGD("switch bitrate, just return");
        return;
    }
    if (percent < static_cast<float>(BUFFER_LOW_PERCENT_DEFAULT) / BUFFER_HIGH_PERCENT_DEFAULT *
        BUFFER_PERCENT_THRESHOLD) {
        // percent<25% buffering start
        PlayBinMessage msg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_BUFFERING_START, 0, {} };
        ReportMessage(msg);

        // percent<25% buffering percent
        PlayBinMessage percentMsg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_BUFFERING_PERCENT, percent, {} };
        ReportMessage(percentMsg);

        isBuffering_ = true;
        if (!SetPlayerState(GST_PLAYER_STATUS_BUFFERING)) {
            MEDIA_LOGD("Stopping, just return");
            return;
        }

        if (GetCurrState() == playingState_ && !isSeeking_ && !isRating_ &&
            !isAddingSubtitle_ && !isUserSetPause_) {
            std::unique_lock<std::mutex> lock(cacheCtrlMutex_);
            MEDIA_LOGI("HandleCacheCtrl percent is %{public}d, begin set to paused", percent);
            GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT_CAST(playbin_), GST_STATE_PAUSED);
            if (ret == GST_STATE_CHANGE_FAILURE) {
                MEDIA_LOGE("Failed to change playbin's state to GST_STATE_PAUSED");
                return;
            }
        }
    }
}

void PlayBinCtrlerBase::HandleCacheCtrlWhenBuffering(int32_t percent)
{
    // 0% < percent < 100% buffering percent
    PlayBinMessage percentMsg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_BUFFERING_PERCENT, percent, {} };
    ReportMessage(percentMsg);

    // percent > 100% buffering end
    if (percent >= BUFFER_PERCENT_THRESHOLD) {
        isBuffering_ = false;
        if (GetCurrState() == playingState_ && !isUserSetPause_) {
            if (!SetPlayerState(GST_PLAYER_STATUS_PLAYING)) {
                MEDIA_LOGD("Stopping, just return");
                return;
            }
            std::unique_lock<std::mutex> lock(cacheCtrlMutex_);
            MEDIA_LOGI("percent is %{public}d, begin set to playing", percent);
            GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT_CAST(playbin_), GST_STATE_PLAYING);
            if (ret == GST_STATE_CHANGE_FAILURE) {
                MEDIA_LOGE("Failed to change playbin's state to GST_STATE_PLAYING");
                return;
            }
        } else {
            if (!SetPlayerState(GST_PLAYER_STATUS_PAUSED)) {
                MEDIA_LOGD("Stopping, just return");
                return;
            }
        }

        PlayBinMessage msg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_BUFFERING_END, 0, {} };
        ReportMessage(msg);
    }
}

void PlayBinCtrlerBase::RemoveGstPlaySinkVideoConvertPlugin()
{
    uint32_t flags = 0;

    CHECK_AND_RETURN_LOG(playbin_ != nullptr, "playbin_ is nullptr");
    g_object_get(playbin_, "flags", &flags, nullptr);
    flags |= (GST_PLAY_FLAG_NATIVE_VIDEO | GST_PLAY_FLAG_HARDWARE_VIDEO);
    flags &= ~GST_PLAY_FLAG_SOFT_COLORBALANCE;
    MEDIA_LOGD("set gstplaysink flags %{public}d", flags);
    // set playsink remove GstPlaySinkVideoConvert, for first-frame performance optimization
    g_object_set(playbin_, "flags", flags, nullptr);
}

GValueArray *PlayBinCtrlerBase::AutoPlugSort(const GstElement *uriDecoder, GstPad *pad, GstCaps *caps,
    GValueArray *factories, gpointer userData)
{
    CHECK_AND_RETURN_RET_LOG(uriDecoder != nullptr, nullptr, "uriDecoder is null");
    CHECK_AND_RETURN_RET_LOG(pad != nullptr, nullptr, "pad is null");
    CHECK_AND_RETURN_RET_LOG(caps != nullptr, nullptr, "caps is null");
    CHECK_AND_RETURN_RET_LOG(factories != nullptr, nullptr, "factories is null");

    auto thizStrong = PlayBinCtrlerWrapper::TakeStrongThiz(userData);
    CHECK_AND_RETURN_RET_LOG(thizStrong != nullptr, nullptr, "thizStrong is null");
    return thizStrong->OnAutoPlugSort(*factories);
}
GValueArray *PlayBinCtrlerBase::OnAutoPlugSort(GValueArray &factories)
{
    MEDIA_LOGD("OnAutoPlugSort");

    decltype(autoPlugSortListener_) listener = nullptr;
    {
        std::unique_lock<std::mutex> lock(listenerMutex_);
        listener = autoPlugSortListener_;
    }

    if (listener != nullptr) {
        return listener(factories);
    }
    return nullptr;
}

void PlayBinCtrlerBase::OnSourceSetup(const GstElement *playbin, GstElement *src,
    const std::shared_ptr<PlayBinCtrlerBase> &playbinCtrl)
{
    (void)playbin;
    CHECK_AND_RETURN_LOG(playbinCtrl != nullptr, "playbinCtrl is null");
    CHECK_AND_RETURN_LOG(src != nullptr, "src is null");

    GstElementFactory *elementFac = gst_element_get_factory(src);
    const gchar *eleTypeName = g_type_name(gst_element_factory_get_element_type(elementFac));
    CHECK_AND_RETURN_LOG(eleTypeName != nullptr, "eleTypeName is nullptr");

    std::unique_lock<std::mutex> appsrcLock(appsrcMutex_);
    if ((strstr(eleTypeName, "GstAppSrc") != nullptr) && (playbinCtrl->appsrcWrap_ != nullptr)) {
        g_object_set(src, "datasrc-mode", true, nullptr);
        (void)playbinCtrl->appsrcWrap_->SetAppsrc(src);
    } else if (strstr(eleTypeName, "GstCurlHttpSrc") != nullptr) {
        g_object_set(src, "ssl-ca-file", "/etc/ssl/certs/cacert.pem", nullptr);
        MEDIA_LOGI("setup curl_http ca_file done");
    }
}

bool PlayBinCtrlerBase::OnVideoDecoderSetup(GstElement &elem)
{
    const gchar *metadata = gst_element_get_metadata(&elem, GST_ELEMENT_METADATA_KLASS);
    if (metadata == nullptr) {
        MEDIA_LOGE("gst_element_get_metadata return nullptr");
        return false;
    }

    std::string metaStr(metadata);
    if (metaStr.find("Decoder/Video") != std::string::npos) {
        return true;
    }

    return false;
}

void PlayBinCtrlerBase::OnIsLiveStream(const GstElement *demux, gboolean isLiveStream, gpointer userData)
{
    (void)demux;
    MEDIA_LOGI("is live stream: %{public}d", isLiveStream);
    auto thizStrong  = PlayBinCtrlerWrapper::TakeStrongThiz(userData);
    if (isLiveStream) {
        PlayBinMessage msg { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_IS_LIVE_STREAM, 0, {} };
        thizStrong->ReportMessage(msg);
    }
}

#ifdef SUPPORT_DRM
int32_t PlayBinCtrlerBase::OnDrmInfoUpdatedSignalReceived(const GstElement *demux, gpointer drmInfoArray,
    uint32_t infoCount, gpointer userData)
{
    MEDIA_LOGI("OnDrmInfoUpdatedSignalReceived is called");
    (void)demux;
    int32_t retCode = DRM_SIGNAL_UNKNOWN;
    CHECK_AND_RETURN_RET_LOG(userData != nullptr, DRM_SIGNAL_INVALID_PARAM, "userData is nullptr");
    auto thizStrong  = PlayBinCtrlerWrapper::TakeStrongThiz(userData);
    CHECK_AND_RETURN_RET_LOG(thizStrong != nullptr, DRM_SIGNAL_INVALID_PARAM, "thizStrong is nullptr");
    CHECK_AND_RETURN_RET_LOG(drmInfoArray != nullptr, DRM_SIGNAL_INVALID_PARAM, "drminfoArray nullptr");
    CHECK_AND_RETURN_RET_LOG(infoCount > 0, DRM_SIGNAL_INVALID_PARAM, "drminfo count less equal to zero");

    // for restore locally
    DrmInfoItem *drmInfos = static_cast<DrmInfoItem*>(drmInfoArray);
    CHECK_AND_RETURN_RET_LOG(drmInfos != nullptr, DRM_SIGNAL_INVALID_PARAM, "drmInfos nullptr");
    std::map<std::string, std::vector<uint8_t>> drmInfoMap;
    for (uint32_t i = 0; i < infoCount; i++) {
        DrmInfoItem temp = drmInfos[i];
        std::stringstream ssConverter;
        std::string uuid;
        for (uint32_t index = 0; index < DrmConstant::DRM_MAX_M3U8_DRM_UUID_LEN; index++) {
            ssConverter << std::hex << static_cast<int32_t>(temp.uuid[index]);
            uuid = ssConverter.str();
        }
        std::vector<uint8_t> pssh(temp.pssh, temp.pssh + temp.psshLen);
        drmInfoMap.insert({ uuid, pssh });
    }
    std::unique_lock<std::mutex> drmInfoLock(thizStrong->drmInfoMutex_);
    thizStrong->drmInfo_ = drmInfoMap;
    drmInfoLock.unlock();

    // for report
    Format format;
    size_t drmInfoSize = static_cast<size_t>(infoCount) * sizeof(DrmInfoItem);
    (void) format.PutBuffer(PlayerKeys::PLAYER_DRM_INFO_ADDR, static_cast<const uint8_t *>(drmInfoArray), drmInfoSize);
    (void) format.PutIntValue(PlayerKeys::PLAYER_DRM_INFO_COUNT, static_cast<int32_t>(infoCount));
    PlayBinMessage msg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_DRM_INFO_UPDATED, 0, format };
    thizStrong->ReportMessage(msg);

    static constexpr int32_t timeout = 10;
    MEDIA_LOGI("Drmcond begin wait for drm prepared");
    std::unique_lock<std::mutex> lock(thizStrong->drmMutex_);
    bool notTimeout = thizStrong->drmConfigCond_.wait_for(lock, std::chrono::seconds(timeout), [thizStrong]() {
        return thizStrong->isDrmPrepared_ || thizStrong->stopWaitingDrmConfig_;
    });
    if (notTimeout) {
        MEDIA_LOGD("Drmcond finish waiting, isDrmPrepared: %{public}d, stopWait: %{public}d",
            thizStrong->isDrmPrepared_, thizStrong->stopWaitingDrmConfig_);
    } else {
        MEDIA_LOGI("Drmcond wait time out!");
    }
    retCode = DRM_SIGNAL_OK;
    return retCode;
}
#endif

#ifdef SUPPORT_DRM
int32_t PlayBinCtrlerBase::OnMediaDecryptSignalReceived(const GstElement *elem, int64_t inputBuffer,
    int64_t outputBuffer, uint32_t length, gpointer keyId, uint32_t keyIdLength, gpointer iv, uint32_t ivLength,
    uint32_t subsampleCount, gpointer subsamples, uint32_t mode, uint32_t svp, uint32_t cryptByteBlock,
    uint32_t skipByteBlock, gpointer userData)
{
    MEDIA_LOGI("OnMediaDecryptSignalReceived is called");
    (void)elem;
    (void)length;
    auto thizStrong = PlayBinCtrlerWrapper::TakeStrongThiz(userData);
    int32_t retCode = DRM_SIGNAL_INVALID_PARAM;

    DrmStandard::IMediaDecryptModuleService::CryptInfo cryptInfo;
    cryptInfo.type = static_cast<DrmStandard::IMediaDecryptModuleService::CryptAlgorithmType>(mode);
    CHECK_AND_RETURN_RET_LOG(keyId != nullptr, retCode, "keyId is nullptr");
    unsigned char* keyIdAddress = static_cast<unsigned char*>(keyId);
    std::vector<uint8_t> keyIdVector(keyIdAddress, keyIdAddress + keyIdLength);
    cryptInfo.keyId = keyIdVector;
    CHECK_AND_RETURN_RET_LOG(iv != nullptr, retCode, "iv is nullptr");
    unsigned char* ivAddress = static_cast<uint8_t*>(iv);
    std::vector<uint8_t> ivVector(ivAddress, ivAddress + ivLength);
    cryptInfo.iv = ivVector;
    cryptInfo.pattern.encryptBlocks = cryptByteBlock;
    cryptInfo.pattern.skipBlocks = skipByteBlock;
    DrmStandard::IMediaDecryptModuleService::SubSample *subSamples =
        static_cast<DrmStandard::IMediaDecryptModuleService::SubSample*>(subsamples);
    for (uint32_t i = 0; i < subsampleCount; i++) {
        DrmStandard::IMediaDecryptModuleService::SubSample temp({ subSamples[i].clearHeaderLen,
            subSamples[i].payLoadLen });
        cryptInfo.subSample.emplace_back(temp);
    }
    uint64_t srcBuffer = static_cast<uint64_t>(inputBuffer);
    uint64_t dstBuffer = static_cast<uint64_t>(outputBuffer);

    if (thizStrong != nullptr && thizStrong->decryptModuleProxy_ != nullptr) {
        retCode = thizStrong->decryptModuleProxy_->DecryptMediaData(svp, cryptInfo, srcBuffer, dstBuffer);
        if (retCode != 0) {
            retCode = DRM_SIGNAL_SERVICE_WRONG;
            MEDIA_LOGE("decryptModuleProxy_ decrypt failed");
        }
    } else {
        retCode = DRM_SIGNAL_INVALID_OPERATOR;
        MEDIA_LOGE("thizStrong or decryptModuleProxy_ is nullptr, media-decrypt signal handle Failed");
    }
    return retCode;
}
#endif

void PlayBinCtrlerBase::OnAdaptiveElementSetup(GstElement &elem)
{
    const gchar *metadata = gst_element_get_metadata(&elem, GST_ELEMENT_METADATA_KLASS);
    if (metadata == nullptr) {
        return;
    }

    std::string metaStr(metadata);
    if (metaStr.find("Demuxer/Adaptive") == std::string::npos) {
        return;
    }
    MEDIA_LOGI("get element_name %{public}s, get metadata %{public}s", GST_ELEMENT_NAME(&elem), metadata);
    PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
    CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");
    gulong id = g_signal_connect_data(&elem, "is-live-scene", G_CALLBACK(&PlayBinCtrlerBase::OnIsLiveStream), wrapper,
        (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory, static_cast<GConnectFlags>(0));
    CheckAndAddSignalIds(id, wrapper, &elem);
}

#ifdef SUPPORT_DRM
void PlayBinCtrlerBase::OnDemuxElementSetup(GstElement &elem)
{
    MEDIA_LOGI("connect signal from %{public}s", GST_ELEMENT_NAME(&elem));
    PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
    CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");
    gulong id = g_signal_connect_data(&elem, "drm-info-updated",
        G_CALLBACK(&PlayBinCtrlerBase::OnDrmInfoUpdatedSignalReceived), wrapper,
        (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory, static_cast<GConnectFlags>(0));
    CheckAndAddSignalIds(id, wrapper, &elem);
}
#endif

#ifdef SUPPORT_DRM
void PlayBinCtrlerBase::OnCodecElementSetup(GstElement &elem)
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " is now the playerCtrlerBase for svp", FAKE_POINTER(this));
    if (svpMode_ == SVP_CLEAR) {
        MEDIA_LOGD("svp is unknown, set codec svp CLEAR!");
    } else if (svpMode_ == SVP_TRUE) {
        MEDIA_LOGD("set the drm svp mode TRUE");
        g_object_set(const_cast<GstElement *>(&elem), "svp-mode", SVP_TRUE, nullptr);
    } else {
        MEDIA_LOGD("set the drm svp mode FALSE");
        g_object_set(const_cast<GstElement *>(&elem), "svp-mode", SVP_FALSE, nullptr);
    }
}
#endif

#ifdef SUPPORT_DRM
void PlayBinCtrlerBase::OnDecryptElementSetup(GstElement &elem)
{
    MEDIA_LOGI("OnDecryptElementSetup element is : %{public}s", ELEM_NAME(&elem));
    PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
    CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");
    gulong id = g_signal_connect_data(&elem, "media-decrypt",
        G_CALLBACK(&PlayBinCtrlerBase::OnMediaDecryptSignalReceived), wrapper,
        (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory, static_cast<GConnectFlags>(0));
    CheckAndAddSignalIds(id, wrapper, &elem);
}
#endif

void PlayBinCtrlerBase::OnElementSetup(GstElement &elem)
{
    MEDIA_LOGD("element setup: %{public}s", ELEM_NAME(&elem));
    // limit to the g-signal, send this notification at this thread, do not change the work thread.
    // otherwise ,the avmetaengine will work improperly.

    if (OnVideoDecoderSetup(elem) || strncmp(ELEM_NAME(&elem), "multiqueue", strlen("multiqueue")) == 0 ||
        strncmp(ELEM_NAME(&elem), "qtdemux", strlen("qtdemux")) == 0) {
        MEDIA_LOGI("add msgfilter element: %{public}s", ELEM_NAME(&elem));
        msgProcessor_->AddMsgFilter(ELEM_NAME(&elem));
    }
#ifdef SUPPORT_DRM
    if (strncmp(ELEM_NAME(&elem), "hlsdemux", strlen("hlsdemux")) == 0 ||
        strncmp(ELEM_NAME(&elem), "tsdemux", strlen("tsdemux")) == 0) {
        OnDemuxElementSetup(elem);
    }
    if (strncmp(ELEM_NAME(&elem), "omx_rk_video_decoder_avc", strlen("omx_rk_video_decoder_avc")) == 0 ||
        strncmp(ELEM_NAME(&elem), "omx_rk_video_decoder_hevc", strlen("omx_rk_video_decoder_hevc")) == 0 ||
        strncmp(ELEM_NAME(&elem), "omx_hisi_video_decoder_hevc", strlen("omx_hisi_video_decoder_hevc")) == 0 ||
        strncmp(ELEM_NAME(&elem), "omx_hisi_video_decoder_avc", strlen("omx_hisi_video_decoder_avc")) == 0) {
        OnCodecElementSetup(elem);
    }
    if (strncmp(ELEM_NAME(&elem), "drmdec", strlen("drmdec")) == 0 ||
        strncmp(ELEM_NAME(&elem), "omx_rk_video_decoder_hevc", strlen("omx_rk_video_decoder_hevc")) == 0 ||
        strncmp(ELEM_NAME(&elem), "omx_rk_video_decoder_avc", strlen("omx_rk_video_decoder_avc")) == 0 ||
        strncmp(ELEM_NAME(&elem), "omx_hisi_video_decoder_hevc", strlen("omx_hisi_video_decoder_hevc")) == 0 ||
        strncmp(ELEM_NAME(&elem), "omx_hisi_video_decoder_avc", strlen("omx_hisi_video_decoder_avc")) == 0) {
        OnDecryptElementSetup(elem);
    }
#endif
    OnAdaptiveElementSetup(elem);
    std::string elementName(GST_ELEMENT_NAME(&elem));
    if (isNetWorkPlay_ == false && elementName.find("uridecodebin") != std::string::npos) {
        PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
        CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");
        gulong id = g_signal_connect_data(&elem, "autoplug-sort",
            G_CALLBACK(&PlayBinCtrlerBase::AutoPlugSort), wrapper,
            (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory, static_cast<GConnectFlags>(0));
        CheckAndAddSignalIds(id, wrapper, &elem);
    }
    
    if (trackParse_ != nullptr) {
        trackParse_->OnElementSetup(elem);
    }

    decltype(elemSetupListener_) listener = nullptr;
    {
        std::unique_lock<std::mutex> lock(listenerMutex_);
        listener = elemSetupListener_;
    }

    if (listener != nullptr) {
        listener(elem);
    }
}

void PlayBinCtrlerBase::OnElementUnSetup(GstElement &elem)
{
    MEDIA_LOGD("element unsetup: %{public}s", ELEM_NAME(&elem));
    if (trackParse_ != nullptr) {
        trackParse_->OnElementUnSetup(elem);
    }

    decltype(elemUnSetupListener_) listener = nullptr;
    {
        std::unique_lock<std::mutex> lock(listenerMutex_);
        listener = elemUnSetupListener_;
    }

    if (listener != nullptr) {
        listener(elem);
    }
    RemoveSignalIds(&elem);
}

void PlayBinCtrlerBase::OnInterruptEventCb(const GstElement *audioSink, const uint32_t eventType,
    const uint32_t forceType, const uint32_t hintType, gpointer userData)
{
    (void)audioSink;
    auto thizStrong = PlayBinCtrlerWrapper::TakeStrongThiz(userData);
    CHECK_AND_RETURN(thizStrong != nullptr);
    uint32_t value = 0;
    value = (((eventType << INTERRUPT_EVENT_SHIFT) | forceType) << INTERRUPT_EVENT_SHIFT) | hintType;
    PlayBinMessage msg { PLAYBIN_MSG_AUDIO_SINK, PLAYBIN_MSG_INTERRUPT_EVENT, 0, value };
    thizStrong->ReportMessage(msg);
}

void PlayBinCtrlerBase::OnDeviceChangeEventCb(const GstElement *audioSink, const AudioStandard::DeviceInfo deviceInfo,
    const AudioStandard::AudioStreamDeviceChangeReason reason, gpointer userData)
{
    (void)audioSink;
    auto thizStrong = PlayBinCtrlerWrapper::TakeStrongThiz(userData);
    CHECK_AND_RETURN(thizStrong != nullptr);
    std::pair<> value = {deviceInfo, reason};
    PlayBinMessage msg { PLAYBIN_MSG_AUDIO_SINK, PLAYBIN_MSG_DEVICE_CHANGE_EVENT, 0, value };
    thizStrong->ReportMessage(msg);
}

void PlayBinCtrlerBase::OnAudioSegmentEventCb(const GstElement *audioSink, gpointer userData)
{
    (void)audioSink;
    auto thizStrong = PlayBinCtrlerWrapper::TakeStrongThiz(userData);
    CHECK_AND_RETURN(thizStrong != nullptr);
    if (thizStrong->subtitleSink_ != nullptr) {
        g_object_set(G_OBJECT(thizStrong->subtitleSink_), "segment-updated", TRUE, nullptr);
    }
}

void PlayBinCtrlerBase::OnAudioDiedEventCb(const GstElement *audioSink, gpointer userData)
{
    (void)audioSink;
    auto thizStrong = PlayBinCtrlerWrapper::TakeStrongThiz(userData);
    CHECK_AND_RETURN(thizStrong != nullptr);
    MEDIA_LOGE("audio service died callback");
    PlayBinMessage msg {
        PLAYBIN_MSG_ERROR,
        PlayBinMsgErrorSubType::PLAYBIN_SUB_MSG_ERROR_WITH_MESSAGE,
        MSERR_AUD_RENDER_FAILED,
        std::string("audio service died!")
    };
    thizStrong->ReportMessage(msg);
}

void PlayBinCtrlerBase::OnBitRateParseCompleteCb(const GstElement *playbin, uint32_t *bitrateInfo,
    uint32_t bitrateNum, gpointer userData)
{
    (void)playbin;
    auto thizStrong = PlayBinCtrlerWrapper::TakeStrongThiz(userData);
    CHECK_AND_RETURN(thizStrong != nullptr);
    MEDIA_LOGD("bitrateNum = %{public}u", bitrateNum);
    for (uint32_t i = 0; i < bitrateNum; i++) {
        MEDIA_LOGD("bitrate = %{public}u", bitrateInfo[i]);
        thizStrong->bitRateVec_.push_back(bitrateInfo[i]);
    }
    Format format;
    (void)format.PutBuffer(std::string(PlayerKeys::PLAYER_BITRATE),
        static_cast<uint8_t *>(static_cast<void *>(bitrateInfo)), bitrateNum * sizeof(uint32_t));
    PlayBinMessage msg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_BITRATE_COLLECT, 0, format };
    thizStrong->ReportMessage(msg);
}

void PlayBinCtrlerBase::AudioChanged(const GstElement *playbin, gpointer userData)
{
    (void)playbin;
    auto thizStrong = PlayBinCtrlerWrapper::TakeStrongThiz(userData);
    CHECK_AND_RETURN(thizStrong != nullptr);
    thizStrong->OnAudioChanged();
}

void PlayBinCtrlerBase::OnAudioChanged()
{
    CHECK_AND_RETURN(playbin_ != nullptr && trackParse_ != nullptr);
    if (!trackParse_->FindTrackInfo()) {
        MEDIA_LOGI("The plugin has been cleared, no need to report it");
        return;
    }

    int32_t audioIndex = -1;
    g_object_get(playbin_, "current-audio", &audioIndex, nullptr);
    MEDIA_LOGI("AudioChanged, current-audio %{public}d", audioIndex);
    if (audioIndex == audioIndex_) {
        MEDIA_LOGI("Audio Not Changed");
        return;
    }

    if (isTrackChanging_) {
        MEDIA_LOGI("Waiting for the seek event to complete, not reporting at this time!");
        return;
    }

    audioIndex_ = audioIndex;
    int32_t index;
    CHECK_AND_RETURN(trackParse_->GetTrackIndex(audioIndex, MediaType::MEDIA_TYPE_AUD, index) == MSERR_OK);

    if (GetCurrState() == preparingState_) {
        MEDIA_LOGI("defaule audio index %{public}d, inner index %{public}d", index, audioIndex);
        Format format;
        (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_TRACK_INDEX), index);
        (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_TRACK_TYPE), MediaType::MEDIA_TYPE_AUD);
        PlayBinMessage msg = { PlayBinMsgType::PLAYBIN_MSG_SUBTYPE,
            PlayBinMsgSubType::PLAYBIN_SUB_MSG_DEFAULE_TRACK, 0, format };
        ReportMessage(msg);
        return;
    }

    Format format;
    (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_TRACK_INDEX), index);
    (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_IS_SELECT), true);
    PlayBinMessage msg = { PlayBinMsgType::PLAYBIN_MSG_SUBTYPE,
        PlayBinMsgSubType::PLAYBIN_SUB_MSG_AUDIO_CHANGED, 0, format };
    ReportMessage(msg);
}

void PlayBinCtrlerBase::OnSubtitleChanged()
{
    CHECK_AND_RETURN(playbin_ != nullptr && trackParse_ != nullptr);
    if (!trackParse_->FindTrackInfo()) {
        MEDIA_LOGI("The plugin has been cleared, no need to report it");
        return;
    }

    int32_t index;
    if (!hasSubtitleTrackSelected_) {
        index = -1;
        MEDIA_LOGI("SubtitleChanged, current text: -1");
    } else {
        int32_t subtitleIndex = -1;
        g_object_get(playbin_, "current-text", &subtitleIndex, nullptr);
        MEDIA_LOGI("SubtitleChanged, current text: %{public}d", subtitleIndex);
        CHECK_AND_RETURN(trackParse_->GetTrackIndex(subtitleIndex, MediaType::MEDIA_TYPE_SUBTITLE, index) == MSERR_OK);
    }

    Format format;
    (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_TRACK_INDEX), index);
    (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_IS_SELECT), true);
    PlayBinMessage msg = { PlayBinMsgType::PLAYBIN_MSG_SUBTYPE,
        PlayBinMsgSubType::PLAYBIN_SUB_MSG_SUBTITLE_CHANGED, 0, format };
    ReportMessage(msg);
}

void PlayBinCtrlerBase::ReportTrackChange()
{
    MEDIA_LOGI("Seek event completed, report track change!");
    if (trackChangeType_ == MediaType::MEDIA_TYPE_AUD) {
        OnAudioChanged();
    } else if (trackChangeType_ == MediaType::MEDIA_TYPE_SUBTITLE) {
        OnSubtitleChanged();
    }
    OnTrackDone();
}

void PlayBinCtrlerBase::OnTrackDone()
{
    PlayBinMessage msg = { PlayBinMsgType::PLAYBIN_MSG_SUBTYPE, PlayBinMsgSubType::PLAYBIN_SUB_MSG_TRACK_DONE, 0, {} };
    ReportMessage(msg);
}

void PlayBinCtrlerBase::OnAddSubDone()
{
    PlayBinMessage msg = { PlayBinMsgType::PLAYBIN_MSG_SUBTYPE,
        PlayBinMsgSubType::PLAYBIN_SUB_MSG_ADD_SUBTITLE_DONE, 0, {} };
    ReportMessage(msg);
}

void PlayBinCtrlerBase::OnError(int32_t errorCode, std::string message)
{
    // There is no limit on the number of reports and the state machine is not changed.
    PlayBinMessage msg = { PlayBinMsgType::PLAYBIN_MSG_SUBTYPE,
        PlayBinMsgSubType::PLAYBIN_SUB_MSG_ONERROR, errorCode, message };
    ReportMessage(msg);
}

void PlayBinCtrlerBase::OnSelectBitrateDoneCb(const GstElement *playbin, uint32_t bandwidth, gpointer userData)
{
    (void)playbin;
    auto thizStrong = PlayBinCtrlerWrapper::TakeStrongThiz(userData);
    MEDIA_LOGD("OnSelectBitrateDoneCb, Get bandwidth is: %{public}u", bandwidth);
    if (thizStrong != nullptr && bandwidth != 0) {
        thizStrong->isSelectBitRate_ = false;
        thizStrong->connectSpeed_ = bandwidth;
        PlayBinMessage msg = { PLAYBIN_MSG_BITRATEDONE, 0, bandwidth, {} };
        thizStrong->ReportMessage(msg);
    }
}

bool PlayBinCtrlerBase::OnAppsrcMessageReceived(const InnerMessage &msg)
{
    MEDIA_LOGI("in OnAppsrcMessageReceived");
    if (msg.type == INNER_MSG_ERROR) {
        PlayBinMessage message { PlayBinMsgType::PLAYBIN_MSG_ERROR,
            PlayBinMsgErrorSubType::PLAYBIN_SUB_MSG_ERROR_WITH_MESSAGE,
            msg.detail1, msg.extend };
        ReportMessage(message);
    } else if (msg.type == INNER_MSG_BUFFERING) {
        if (msg.detail1 < static_cast<float>(BUFFER_LOW_PERCENT_DEFAULT) / BUFFER_HIGH_PERCENT_DEFAULT *
            BUFFER_PERCENT_THRESHOLD) {
            CHECK_AND_RETURN_RET_LOG(GetCurrState() == playingState_ && !isSeeking_ && !isRating_ && !isUserSetPause_,
                false, "ignore change to pause");
            std::unique_lock<std::mutex> lock(cacheCtrlMutex_);
            MEDIA_LOGI("begin set to pause");
            GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT_CAST(playbin_), GST_STATE_PAUSED);
            if (ret == GST_STATE_CHANGE_FAILURE) {
                MEDIA_LOGE("Failed to change playbin's state to GST_STATE_PAUSED");
                return false;
            }
        } else if (msg.detail1 >= BUFFER_PERCENT_THRESHOLD) {
            CHECK_AND_RETURN_RET_LOG(GetCurrState() == playingState_ && !isUserSetPause_,
                false, "ignore change to playing");
            std::unique_lock<std::mutex> lock(cacheCtrlMutex_);
            MEDIA_LOGI("begin set to play");
            GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT_CAST(playbin_), GST_STATE_PLAYING);
            if (ret == GST_STATE_CHANGE_FAILURE) {
                MEDIA_LOGE("Failed to change playbin's state to GST_STATE_PLAYING");
                return false;
            }
        }
    }
    return true;
}

void PlayBinCtrlerBase::OnMessageReceived(const InnerMessage &msg)
{
    HandleMessage(msg);
}

void PlayBinCtrlerBase::OnSinkMessageReceived(const PlayBinMessage &msg)
{
    ReportMessage(msg);
}

void PlayBinCtrlerBase::SetNotifier(PlayBinMsgNotifier notifier)
{
    std::unique_lock<std::mutex> lock(mutex_);
    notifier_ = notifier;
}

void PlayBinCtrlerBase::SetAutoSelectBitrate(bool enable)
{
    if (enable) {
        g_object_set(playbin_, "connection-speed", 0, nullptr);
    } else if (connectSpeed_ == 0) {
        g_object_set(playbin_, "connection-speed", CONNECT_SPEED_DEFAULT, nullptr);
    }
}

void PlayBinCtrlerBase::ReportMessage(const PlayBinMessage &msg)
{
    if (msg.type == PlayBinMsgType::PLAYBIN_MSG_ERROR) {
        MEDIA_LOGE("error happend, error code: %{public}d", msg.code);

        {
            std::unique_lock<std::mutex> lock(mutex_);
            isErrorHappened_ = true;
            preparingCond_.notify_all();
            stoppingCond_.notify_all();
        }
    }

    MEDIA_LOGD("report msg, type: %{public}d", msg.type);

    PlayBinMsgNotifier notifier = notifier_;
    if (notifier != nullptr) {
        auto msgReportHandler = std::make_shared<TaskHandler<void>>([msg, notifier]() {
            LISTENER(notifier(msg), "PlayBinCtrlerBase::ReportMessage", PlayerXCollie::timerTimeout)
        });
        (void)msgQueue_->EnqueueTask(msgReportHandler);
    }

    if (msg.type == PlayBinMsgType::PLAYBIN_MSG_EOS) {
        ProcessEndOfStream();
    }
}

void PlayBinCtrlerBase::CheckAndAddSignalIds(gulong id, PlayBinCtrlerWrapper *wrapper, GstElement *elem)
{
    if (id == 0) {
        delete wrapper;
        MEDIA_LOGW("add signal failed");
    } else {
        AddSignalIds(elem, id);
    }
}

bool PlayBinCtrlerBase::SetPlayerState(GstPlayerStatus status)
{
    std::unique_lock<std::mutex> stateChangeLock(stateChangePropertyMutex_);
    if (stopBuffering_) {
        MEDIA_LOGD("Do not set player state when stopping");
        return false;
    }
    std::unique_lock<std::mutex> lock(cacheCtrlMutex_);
    g_object_set(playbin_, "state-change", status, nullptr);
    return true;
}
} // namespace Media
} // namespace OHOS

