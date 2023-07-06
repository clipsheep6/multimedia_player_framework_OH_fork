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

#include "playbin_ctrler_base.h"
#include <gst/playback/gstplay-enum.h>
#include "nocopyable.h"
#include "string_ex.h"
#include "media_errors.h"
#include "media_log.h"
#include "player.h"
#include "format.h"
#include "uri_helper.h"
#include "scope_guard.h"
#include "playbin_state.h"
#include "gst_utils.h"
#include "media_dfx.h"
#include "player_xcollie.h"
#include "param_wrapper.h"

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

using PlayBinCtrlerWrapper = ThizWrapper<PlayBinCtrlerBase>;

void PlayBinCtrlerBase::ElementSetup(const GstElement *playbin, GstElement *elem, gpointer userData)
{
    (void)playbin;
    if (elem == nullptr || userData == nullptr) {
        return;
    }

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
    if (child == nullptr || userData == nullptr) {
        return;
    }

    auto thizStrong = PlayBinCtrlerWrapper::TakeStrongThiz(userData);
    if (thizStrong != nullptr) {
        return thizStrong->OnElementUnSetup(*child);
    }
}

void PlayBinCtrlerBase::SourceSetup(const GstElement *playbin, GstElement *elem, gpointer userData)
{
    if (elem == nullptr || userData == nullptr) {
        return;
    }

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

int32_t PlayBinCtrlerBase::SetSource(const std::string &url)
{
    std::unique_lock<std::mutex> lock(mutex_);
    uri_ = url;
    if (url.find("http") == 0 || url.find("https") == 0) {
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
    CHECK_AND_RETURN_RET(sinkProvider_ != nullptr, MSERR_INVALID_VAL);
    if (subtitleSink_ == nullptr) {
        subtitleSink_ = sinkProvider_->CreateSubtitleSink();
        g_object_set(playbin_, "text-sink", subtitleSink_, nullptr);
    }

    isAddingSubtitle_ = true;
    g_object_set(playbin_, "add-suburi", url.c_str(), nullptr);

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::Prepare()
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);

    int32_t ret = PrepareAsyncInternal();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    {
        MEDIA_LOGD("Prepare Start");
        preparingCond_.wait(lock);
        MEDIA_LOGD("Prepare End");
    }

    if (isErrorHappened_) {
        MEDIA_LOGE("Prepare failed");
        GstStateChangeReturn gstRet = gst_element_set_state(GST_ELEMENT_CAST(playbin_), GST_STATE_READY);
        if (gstRet == GST_STATE_CHANGE_FAILURE) {
            MEDIA_LOGE("Failed to change playbin's state to %{public}s", gst_element_state_get_name(GST_STATE_READY));
        }
        return MSERR_INVALID_STATE;
    }

    MEDIA_LOGD("exit");
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

    if (GetCurrState() == playingState_) {
        MEDIA_LOGI("already at playing state, skip");
        return MSERR_OK;
    }

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

    if (GetCurrState() == pausedState_ || GetCurrState() == preparedState_) {
        MEDIA_LOGI("already at paused state, skip");
        return MSERR_OK;
    }

    auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
    int32_t ret = currState->Pause();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Pause failed");

    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::Seek(int64_t timeUs, int32_t seekOption)
{
    MEDIA_LOGD("enter");

    if (SEEK_OPTION_TO_GST_SEEK_FLAGS.find(seekOption) == SEEK_OPTION_TO_GST_SEEK_FLAGS.end()) {
        MEDIA_LOGE("unsupported seek option: %{public}d", seekOption);
        return MSERR_INVALID_VAL;
    }

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

    if (GetCurrState() == preparingState_ && needWait) {
        MEDIA_LOGD("begin wait stop for current status is preparing");
        static constexpr int32_t timeout = 2;
        preparedCond_.wait_for(lock, std::chrono::seconds(timeout));
        MEDIA_LOGD("end wait stop for current status is preparing");
    }

    if (appsrcWrap_ != nullptr) {
        appsrcWrap_->Stop();
    }

    auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
    (void)currState->Stop();

    {
        MEDIA_LOGD("Stop Start");
        if (GetCurrState() != stoppedState_) {
            LISTENER(stoppingCond_.wait(lock), "stoppingCond_.wait", PlayerXCollie::timerTimeout)
        }
        MEDIA_LOGD("Stop End");
    }

    if (GetCurrState() != stoppedState_) {
        MEDIA_LOGE("Stop failed");
        return MSERR_INVALID_STATE;
    }
    return MSERR_OK;
}

GstSeekFlags PlayBinCtrlerBase::ChooseSetRateFlags(double rate)
{
    GstSeekFlags seekFlags;

    if (rate > 0.0) {
        seekFlags = static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH |
            GST_SEEK_FLAG_TRICKMODE | GST_SEEK_FLAG_SNAP_AFTER);
    } else {
        seekFlags = static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH |
            GST_SEEK_FLAG_TRICKMODE | GST_SEEK_FLAG_SNAP_BEFORE);
    }

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

double PlayBinCtrlerBase::GetRate()
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGI("get rate=%{public}lf", rate_);
    return rate_;
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
        MEDIA_LOGI("SetVolume(%{public}f) to audio sink", volume);
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
    if (bitRateVec_.empty()) {
        MEDIA_LOGE("BitRate is empty");
        return MSERR_INVALID_OPERATION;
    }
    if (connectSpeed_ == bitRate) {
        PlayBinMessage msg = { PLAYBIN_MSG_BITRATEDONE, 0, static_cast<int32_t>(bitRate), {} };
        ReportMessage(msg);
    } else {
        connectSpeed_ = bitRate;
        g_object_set(playbin_, "connection-speed", static_cast<uint64_t>(bitRate), nullptr);
    }
    return MSERR_OK;
}

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
    cachePercent_ = BUFFER_PERCENT_THRESHOLD;
    isDuration_ = false;
    isUserSetPause_ = false;

    MEDIA_LOGD("exit");
    return MSERR_OK;
}

void PlayBinCtrlerBase::SetElemSetupListener(ElemSetupListener listener)
{
    std::unique_lock<std::mutex> lock(mutex_);
    std::unique_lock<std::mutex> lk(listenerMutex_);
    elemSetupListener_ = listener;
    
    if (trackParse_ == nullptr) {
        trackParse_ = PlayerTrackParse::Create();
    }
    CHECK_AND_RETURN_LOG(trackParse_ != nullptr, "creat track parse failed")
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
        AddSignalIds(GST_ELEMENT_CAST(playbin_), id);

        PlayBinCtrlerWrapper *wrap = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
        CHECK_AND_RETURN_LOG(wrap != nullptr, "can not create this wrap");

        id = g_signal_connect_data(playbin_, "video-changed",
            G_CALLBACK(&PlayBinCtrlerBase::OnSelectBitrateDoneCb), wrap,
            (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory, static_cast<GConnectFlags>(0));
        AddSignalIds(GST_ELEMENT_CAST(playbin_), id);
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

    int32_t ret = EnterInitializedState();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

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

    gboolean ret = gst_element_send_event(GST_ELEMENT_CAST(playbin_), event);
    CHECK_AND_RETURN_RET_LOG(ret, MSERR_SEEK_FAILED, "seek failed");

    return MSERR_OK;
}

void PlayBinCtrlerBase::SetupInterruptEventCb()
{
    PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
    CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");

    gulong id = g_signal_connect_data(audioSink_, "interrupt-event",
        G_CALLBACK(&PlayBinCtrlerBase::OnInterruptEventCb), wrapper,
        (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory, static_cast<GConnectFlags>(0));
    AddSignalIds(GST_ELEMENT_CAST(audioSink_), id);
}

void PlayBinCtrlerBase::SetupAudioStateEventCb()
{
    PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
    CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");

    gulong id = g_signal_connect_data(audioSink_, "audio-state-event",
        G_CALLBACK(&PlayBinCtrlerBase::OnAudioStateEventCb), wrapper,
        (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory, static_cast<GConnectFlags>(0));
    AddSignalIds(GST_ELEMENT_CAST(audioSink_), id);
}

void PlayBinCtrlerBase::SetupAudioSegmentEventCb()
{
    PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
    CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");

    gulong id = g_signal_connect_data(audioSink_, "segment-updated",
        G_CALLBACK(&PlayBinCtrlerBase::OnAudioSegmentEventCb), wrapper,
        (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory, static_cast<GConnectFlags>(0));
    AddSignalIds(GST_ELEMENT_CAST(audioSink_), id);
}

void PlayBinCtrlerBase::SetupCustomElement()
{
    // There may be a risk of data competition, but the sinkProvider is unlikely to be reconfigured.
    if (sinkProvider_ != nullptr) {
        audioSink_ = sinkProvider_->CreateAudioSink();
        if (audioSink_ != nullptr) {
            g_object_set(playbin_, "audio-sink", audioSink_, nullptr);
            SetupInterruptEventCb();
            SetupAudioStateEventCb();
            SetupAudioSegmentEventCb();
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
    AddSignalIds(GST_ELEMENT_CAST(playbin_), id);
}

int32_t PlayBinCtrlerBase::SetupSignalMessage()
{
    MEDIA_LOGD("SetupSignalMessage enter");

    PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
    CHECK_AND_RETURN_RET_LOG(wrapper != nullptr, MSERR_NO_MEMORY, "can not create this wrapper");

    gulong id = g_signal_connect_data(playbin_, "element-setup",
        G_CALLBACK(&PlayBinCtrlerBase::ElementSetup), wrapper, (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory,
        static_cast<GConnectFlags>(0));
    AddSignalIds(GST_ELEMENT_CAST(playbin_), id);

    PlayBinCtrlerWrapper *wrap = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
    CHECK_AND_RETURN_RET_LOG(wrap != nullptr, MSERR_NO_MEMORY, "can not create this wrapper");
    id = g_signal_connect_data(playbin_, "audio-changed", G_CALLBACK(&PlayBinCtrlerBase::AudioChanged),
        wrap, (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory, static_cast<GConnectFlags>(0));
    AddSignalIds(GST_ELEMENT_CAST(playbin_), id);

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
    AddSignalIds(GST_ELEMENT_CAST(playbin_), id);

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
    MEDIA_LOGI("update the duration: %{public}" PRIi64 " microsecond", duration_);
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
    MEDIA_LOGI("update the position: %{public}" PRIi64 " microsecond", curTime);
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

        g_object_set(playbin_, "current-audio", innerIndex, nullptr);
        // The seek operation clears the original audio data and re parses the new track data.
        isTrackChanging_ = true;
        trackChangeType_ = MediaType::MEDIA_TYPE_AUD;
        SeekInternal(seekPos_, IPlayBinCtrler::PlayBinSeekMode::CLOSET_SYNC);
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
        g_object_get(playbin_, "current-text", &innerIndex, nullptr);
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
    MEDIA_LOGI("HandleCacheCtrl percent is %{public}d", percent);
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
    if (percent < static_cast<float>(BUFFER_LOW_PERCENT_DEFAULT) / BUFFER_HIGH_PERCENT_DEFAULT *
        BUFFER_PERCENT_THRESHOLD) {
        // percent<25% buffering start
        PlayBinMessage msg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_BUFFERING_START, 0, {} };
        ReportMessage(msg);

        // percent<25% buffering percent
        PlayBinMessage percentMsg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_BUFFERING_PERCENT, percent, {} };
        ReportMessage(percentMsg);

        isBuffering_ = true;
        {
            std::unique_lock<std::mutex> lock(cacheCtrlMutex_);
            g_object_set(playbin_, "state-change", GST_PLAYER_STATUS_BUFFERING, nullptr);
        }

        if (GetCurrState() == playingState_ && !isSeeking_ && !isRating_ && !isAddingSubtitle_ && !isUserSetPause_) {
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
            {
                std::unique_lock<std::mutex> lock(cacheCtrlMutex_);
                g_object_set(playbin_, "state-change", GST_PLAYER_STATUS_PLAYING, nullptr);
            }
            std::unique_lock<std::mutex> lock(cacheCtrlMutex_);
            MEDIA_LOGI("percent is %{public}d, begin set to playing", percent);
            GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT_CAST(playbin_), GST_STATE_PLAYING);
            if (ret == GST_STATE_CHANGE_FAILURE) {
                MEDIA_LOGE("Failed to change playbin's state to GST_STATE_PLAYING");
                return;
            }
        } else {
            std::unique_lock<std::mutex> lock(cacheCtrlMutex_);
            g_object_set(playbin_, "state-change", GST_PLAYER_STATUS_PAUSED, nullptr);
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
    AddSignalIds(&elem, id);
}

void PlayBinCtrlerBase::OnElementSetup(GstElement &elem)
{
    MEDIA_LOGI("element setup: %{public}s", ELEM_NAME(&elem));
    // limit to the g-signal, send this notification at this thread, do not change the work thread.
    // otherwise ,the avmetaengine will work improperly.

    if (OnVideoDecoderSetup(elem) || strncmp(ELEM_NAME(&elem), "multiqueue", strlen("multiqueue")) == 0 ||
        strncmp(ELEM_NAME(&elem), "qtdemux", strlen("qtdemux")) == 0) {
        MEDIA_LOGI("add msgfilter element: %{public}s", ELEM_NAME(&elem));
        msgProcessor_->AddMsgFilter(ELEM_NAME(&elem));
    }

    OnAdaptiveElementSetup(elem);
    std::string elementName(GST_ELEMENT_NAME(&elem));
    if (isNetWorkPlay_ == false && elementName.find("uridecodebin") != std::string::npos) {
        PlayBinCtrlerWrapper *wrapper = new(std::nothrow) PlayBinCtrlerWrapper(shared_from_this());
        CHECK_AND_RETURN_LOG(wrapper != nullptr, "can not create this wrapper");
        gulong id = g_signal_connect_data(&elem, "autoplug-sort",
            G_CALLBACK(&PlayBinCtrlerBase::AutoPlugSort), wrapper,
            (GClosureNotify)&PlayBinCtrlerWrapper::OnDestory, static_cast<GConnectFlags>(0));
        AddSignalIds(&elem, id);
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
    MEDIA_LOGI("element unsetup: %{public}s", ELEM_NAME(&elem));
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

void PlayBinCtrlerBase::OnAudioStateEventCb(const GstElement *audioSink, const uint32_t audioState, gpointer userData)
{
    (void)audioSink;
    auto thizStrong = PlayBinCtrlerWrapper::TakeStrongThiz(userData);
    CHECK_AND_RETURN(thizStrong != nullptr);
    int32_t value = static_cast<int32_t>(audioState);
    PlayBinMessage msg { PLAYBIN_MSG_AUDIO_SINK, PLAYBIN_MSG_AUDIO_STATE_EVENT, 0, value };
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

void PlayBinCtrlerBase::ReportTrackChange()
{
    MEDIA_LOGI("Seek event completed, report track change!");
    if (trackChangeType_ == MediaType::MEDIA_TYPE_AUD) {
        OnAudioChanged();
    }
    OnTrackDone();
}

void PlayBinCtrlerBase::OnTrackDone()
{
    PlayBinMessage msg = { PlayBinMsgType::PLAYBIN_MSG_SUBTYPE, PlayBinMsgSubType::PLAYBIN_SUB_MSG_TRACK_DONE, 0, {} };
    ReportMessage(msg);
}

void PlayBinCtrlerBase::OnError(int32_t errorCode, std::string message)
{
    // There is no limit on the number of reports and the state machine is not changed.
    PlayBinMessage msg = { PlayBinMsgType::PLAYBIN_MSG_SUBTYPE,
        PlayBinMsgSubType::PLAYBIN_SUB_MSG_ONERROR, errorCode, message };
    ReportMessage(msg);
}

void PlayBinCtrlerBase::OnSelectBitrateDoneCb(const GstElement *playbin, const char *streamId, gpointer userData)
{
    (void)playbin;
    auto thizStrong = PlayBinCtrlerWrapper::TakeStrongThiz(userData);
    MEDIA_LOGD("Get streamId is: %{public}s", streamId);
    if (thizStrong != nullptr && strcmp(streamId, "") != 0) {
        int32_t bandwidth = thizStrong->trackParse_->GetHLSStreamBandwidth(streamId);
        PlayBinMessage msg = { PLAYBIN_MSG_BITRATEDONE, 0, bandwidth, {} };
        thizStrong->ReportMessage(msg);
    }
}

void PlayBinCtrlerBase::OnAppsrcMessageReceived(const InnerMessage &msg)
{
    MEDIA_LOGI("in OnAppsrcMessageReceived");
    if (msg.type == INNER_MSG_ERROR) {
        PlayBinMessage message { PlayBinMsgType::PLAYBIN_MSG_ERROR,
            PlayBinMsgErrorSubType::PLAYBIN_SUB_MSG_ERROR_WITH_MESSAGE,
            msg.detail1, msg.extend };
        ReportMessage(message);
    } else if (msg.type == INNER_MSG_BUFFERING) {
        if (msg.detail1 < static_cast<float>(BUFFER_LOW_PERCENT_DEFAULT) / BUFFER_HIGH_PERCENT_DEFAULT *
            BUFFER_PERCENT_THRESHOLD && GetCurrState() == playingState_ &&
            !isSeeking_ && !isRating_ && !isUserSetPause_) {
            std::unique_lock<std::mutex> lock(cacheCtrlMutex_);
            MEDIA_LOGI("begin set to pause");
            GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT_CAST(playbin_), GST_STATE_PAUSED);
            if (ret == GST_STATE_CHANGE_FAILURE) {
                MEDIA_LOGE("Failed to change playbin's state to GST_STATE_PAUSED");
                return;
            }
        } else if (msg.detail1 >= BUFFER_PERCENT_THRESHOLD && GetCurrState() == playingState_ && !isUserSetPause_) {
            std::unique_lock<std::mutex> lock(cacheCtrlMutex_);
            MEDIA_LOGI("begin set to play");
            GstStateChangeReturn ret = gst_element_set_state(GST_ELEMENT_CAST(playbin_), GST_STATE_PLAYING);
            if (ret == GST_STATE_CHANGE_FAILURE) {
                MEDIA_LOGE("Failed to change playbin's state to GST_STATE_PLAYING");
                return;
            }
        }
    }
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
        int32_t ret = msgQueue_->EnqueueTask(msgReportHandler);
        if (ret != MSERR_OK) {
            MEDIA_LOGE("async report msg failed, type: %{public}d, subType: %{public}d, code: %{public}d",
                msg.type, msg.subType, msg.code);
        };
    }

    if (msg.type == PlayBinMsgType::PLAYBIN_MSG_EOS) {
        ProcessEndOfStream();
    }
}
} // namespace Media
} // namespace OHOS

