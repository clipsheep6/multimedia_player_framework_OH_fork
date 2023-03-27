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

#include "playbin_state.h"
#include <gst/gst.h>
#include <gst/playback/gstplay-enum.h>
#include "media_errors.h"
#include "media_log.h"
#include "dumper.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayBinState"};
    constexpr int32_t USEC_PER_MSEC = 1000;
    constexpr uint32_t DEFAULT_POSITION_UPDATE_INTERVAL_MS = 100; // 100 ms
}

namespace OHOS {
namespace Media {
void PlayBinCtrlerBase::BaseState::ReportInvalidOperation()
{
    MEDIA_LOGE("invalid operation for %{public}s", GetStateName().c_str());

    /*
     * The ReportInvalidOperation function is locked before calling it. However,
     * the ReportMessage function is also locked. Therefore, the ReportMessage function
     * needs to be unlocked before using it.
     */
    ctrler_.mutex_.unlock();
    PlayBinMessage msg { PlayBinMsgType::PLAYBIN_MSG_ERROR,
        PlayBinMsgErrorSubType::PLAYBIN_SUB_MSG_ERROR_NO_MESSAGE,
        MSERR_INVALID_STATE, "PlayBinCtrlerBase::BaseState::ReportInvalidOperation" };
    ctrler_.ReportMessage(msg);
    ctrler_.mutex_.lock();
}

int32_t PlayBinCtrlerBase::BaseState::Prepare()
{
    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayBinCtrlerBase::BaseState::Play()
{
    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayBinCtrlerBase::BaseState::Pause()
{
    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayBinCtrlerBase::BaseState::Seek(int64_t timeUs, int32_t option)
{
    (void)timeUs;
    (void)option;

    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayBinCtrlerBase::BaseState::Stop()
{
    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayBinCtrlerBase::BaseState::SetRate(double rate)
{
    (void)rate;

    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayBinCtrlerBase::BaseState::ChangePlayBinState(GstState targetState, GstStateChangeReturn &ret)
{
    if (targetState < GST_STATE_PLAYING) {
        int64_t position = ctrler_.QueryPosition();
        int32_t tickType = INNER_MSG_POSITION_UPDATE;
        ctrler_.msgProcessor_->RemoveTickSourceByType(tickType);
        PlayBinMessage posUpdateMsg { PLAYBIN_MSG_POSITION_UPDATE, PLAYBIN_SUB_MSG_POSITION_UPDATE_FORCE,
            static_cast<int32_t>(position), static_cast<int32_t>(ctrler_.duration_ / USEC_PER_MSEC) };
        ctrler_.ReportMessage(posUpdateMsg);
    }

    ret = gst_element_set_state(GST_ELEMENT_CAST(ctrler_.playbin_), targetState);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        MEDIA_LOGE("Failed to change playbin's state to %{public}s", gst_element_state_get_name(targetState));
        return MSERR_INVALID_OPERATION;
    }

    return MSERR_OK;
}

void PlayBinCtrlerBase::BaseState::HandleStateChange(const InnerMessage &msg)
{
    MEDIA_LOGI("BaseState::HandleStateChange");
    if (msg.extend.has_value() && std::any_cast<GstPipeline *>(msg.extend) == ctrler_.playbin_) {
        GstState targetState = static_cast<GstState>(msg.detail2);
        MEDIA_LOGI("state changed from %{public}s to %{public}s",
            gst_element_state_get_name(static_cast<GstState>(msg.detail1)),
            gst_element_state_get_name(targetState));
        if (targetState == GST_STATE_PLAYING) {
            int32_t tickType = INNER_MSG_POSITION_UPDATE;
            uint32_t interval = DEFAULT_POSITION_UPDATE_INTERVAL_MS;
            ctrler_.msgProcessor_->AddTickSource(tickType, interval);
        } else if (targetState == GST_STATE_PAUSED) {
            int32_t tickType = INNER_MSG_POSITION_UPDATE;
            ctrler_.msgProcessor_->RemoveTickSourceByType(tickType);
            if (!ctrler_.isSeeking_ && !ctrler_.isRating_) {
                int64_t position = ctrler_.QueryPosition();
                PlayBinMessage posUpdateMsg { PLAYBIN_MSG_POSITION_UPDATE, PLAYBIN_SUB_MSG_POSITION_UPDATE_FORCE,
                    static_cast<int32_t>(position), static_cast<int32_t>(ctrler_.duration_ / USEC_PER_MSEC) };
                ctrler_.ReportMessage(posUpdateMsg);
            }
        }

        Dumper::DumpDotGraph(*ctrler_.playbin_, msg.detail1, msg.detail2);

        ProcessStateChange(msg);
        if ((msg.detail1 == GST_STATE_PAUSED && msg.detail2 == GST_STATE_PLAYING) && ctrler_.isNetWorkPlay_) {
            ctrler_.HandleCacheCtrl(ctrler_.cachePercent_);
        }
    }
}

void PlayBinCtrlerBase::BaseState::HandleDurationChange()
{
    MEDIA_LOGI("received duration change msg, update duration");
    ctrler_.QueryDuration();
    int64_t position = ctrler_.QueryPosition();
    PlayBinMessage posUpdateMsg { PLAYBIN_MSG_POSITION_UPDATE, PLAYBIN_SUB_MSG_POSITION_UPDATE_FORCE,
        static_cast<int32_t>(position), static_cast<int32_t>(ctrler_.duration_ / USEC_PER_MSEC) };
    ctrler_.ReportMessage(posUpdateMsg);
}

void PlayBinCtrlerBase::BaseState::HandleResolutionChange(const InnerMessage &msg)
{
    std::pair<int32_t, int32_t> resolution;
    guint rotation = 0;
    if (ctrler_.videoSink_) {
        GValue val = G_VALUE_INIT;
        g_object_get_property(G_OBJECT(ctrler_.videoSink_), "video-rotation", &val);
        rotation = g_value_get_uint(&val);
    }
    if (rotation == 90 || rotation == 270) {  // angle of rotation 90, 270
        resolution.first = msg.detail2;
        resolution.second = msg.detail1;
    } else {
        resolution.first = msg.detail1;
        resolution.second = msg.detail2;
    }
    PlayBinMessage playBinMsg { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_VIDEO_SIZE_CHANGED, 0, resolution };
    ctrler_.ReportMessage(playBinMsg);
}

void PlayBinCtrlerBase::BaseState::HandleAsyncDone(const InnerMessage &msg)
{
    MEDIA_LOGI("BaseState::HandleAsyncDone");
    if (std::any_cast<GstPipeline *>(msg.extend) == ctrler_.playbin_) {
        GstState state = GST_STATE_NULL;
        GstStateChangeReturn stateRet = gst_element_get_state(GST_ELEMENT_CAST(ctrler_.playbin_), &state,
            nullptr, static_cast<GstClockTime>(0));
        MEDIA_LOGI("BaseState::HandleAsyncDone %{public}d, %{public}d",
            static_cast<int32_t>(stateRet), static_cast<int32_t>(state));
        if ((stateRet == GST_STATE_CHANGE_SUCCESS) && (state >= GST_STATE_PAUSED)) {
            if (ctrler_.isSeeking_) {
                int64_t position = ctrler_.seekPos_ / USEC_PER_MSEC;
                ctrler_.isSeeking_ = false;
                ctrler_.isDuration_ = (position == ctrler_.duration_ / USEC_PER_MSEC) ? true : false;
                MEDIA_LOGI("asyncdone after seek done, pos = %{public}" PRIi64 "ms", position);
                PlayBinMessage posUpdateMsg { PLAYBIN_MSG_POSITION_UPDATE, PLAYBIN_SUB_MSG_POSITION_UPDATE_FORCE,
                    static_cast<int32_t>(ctrler_.QueryPosition()),
                    static_cast<int32_t>(ctrler_.duration_ / USEC_PER_MSEC) };
                ctrler_.ReportMessage(posUpdateMsg);

                PlayBinMessage playBinMsg { PLAYBIN_MSG_SEEKDONE, 0, static_cast<int32_t>(position), {} };
                ctrler_.ReportMessage(playBinMsg);
            } else if (ctrler_.isRating_) {
                ctrler_.isRating_ = false;
                MEDIA_LOGI("asyncdone after setRate done, rate = %{public}lf", ctrler_.rate_);
                PlayBinMessage posUpdateMsg { PLAYBIN_MSG_POSITION_UPDATE, PLAYBIN_SUB_MSG_POSITION_UPDATE_FORCE,
                    static_cast<int32_t>(ctrler_.QueryPosition()),
                    static_cast<int32_t>(ctrler_.duration_ / USEC_PER_MSEC) };
                ctrler_.ReportMessage(posUpdateMsg);

                PlayBinMessage playBinMsg { PLAYBIN_MSG_SPEEDDONE, 0, 0, ctrler_.rate_ };
                ctrler_.ReportMessage(playBinMsg);
            } else {
                MEDIA_LOGD("Async done, not seeking or rating!");
                PlayBinMessage playBinMsg { PLAYBIN_MSG_ASYNC_DONE, 0, 0, {} };
                ctrler_.ReportMessage(playBinMsg);
            }
        }
    }
}

void PlayBinCtrlerBase::BaseState::HandleError(const InnerMessage &msg)
{
    PlayBinMessage playbinMsg { PLAYBIN_MSG_ERROR, PLAYBIN_SUB_MSG_ERROR_WITH_MESSAGE, msg.detail1, msg.extend };
    ctrler_.ReportMessage(playbinMsg);
}

void PlayBinCtrlerBase::BaseState::HandleEos()
{
    int64_t position = ctrler_.duration_ / USEC_PER_MSEC;
    if (ctrler_.IsLiveSource()) {
        position = ctrler_.QueryPosition();
    }
    int32_t tickType = INNER_MSG_POSITION_UPDATE;
    ctrler_.msgProcessor_->RemoveTickSourceByType(tickType);
    PlayBinMessage posUpdateMsg { PLAYBIN_MSG_POSITION_UPDATE, PLAYBIN_SUB_MSG_POSITION_UPDATE_FORCE,
        static_cast<int32_t>(position), static_cast<int32_t>(ctrler_.duration_ / USEC_PER_MSEC) };
    ctrler_.ReportMessage(posUpdateMsg);

    PlayBinMessage playBinMsg = { PLAYBIN_MSG_EOS, 0, static_cast<int32_t>(ctrler_.enableLooping_.load()), {} };
    ctrler_.ReportMessage(playBinMsg);
}

void PlayBinCtrlerBase::BaseState::HandleBuffering(const InnerMessage &msg)
{
    ctrler_.HandleCacheCtrlCb(msg);
}

void PlayBinCtrlerBase::BaseState::HandleBufferingTime(const InnerMessage &msg)
{
    std::pair<uint32_t, int64_t> bufferingTimePair;
    bufferingTimePair.first = static_cast<uint32_t>(msg.detail1);
    bufferingTimePair.second = std::any_cast<int64_t>(msg.extend);
    PlayBinMessage playBinMsg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_BUFFERING_TIME, 0, bufferingTimePair };
    ctrler_.ReportMessage(playBinMsg);
}

void PlayBinCtrlerBase::BaseState::HandleUsedMqNum(const InnerMessage &msg)
{
    uint32_t usedMqNum = static_cast<uint32_t>(msg.detail1);
    MEDIA_LOGI("HandleUsedMqNum usedMqNum %{public}u", usedMqNum);
    PlayBinMessage playBinMsg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_BUFFERING_USED_MQ_NUM, 0, usedMqNum };
    ctrler_.ReportMessage(playBinMsg);
}

void PlayBinCtrlerBase::BaseState::HandleVideoRotation(const InnerMessage &msg)
{
    if (ctrler_.videoSink_) {
        g_object_set(ctrler_.videoSink_, "video-rotation", msg.detail1, nullptr);
    } else {
        MEDIA_LOGE("Failed to set video-rotation, videoSink is nullptr");
    }
}

void PlayBinCtrlerBase::BaseState::OnMessageReceived(const InnerMessage &msg)
{
    switch (msg.type) {
        case INNER_MSG_STATE_CHANGED:
            HandleStateChange(msg);
            break;
        case INNER_MSG_DURATION_CHANGED:
            HandleDurationChange();
            break;
        case INNER_MSG_RESOLUTION_CHANGED:
            HandleResolutionChange(msg);
            break;
        case INNER_MSG_ASYNC_DONE:
            HandleAsyncDone(msg);
            break;
        case INNER_MSG_ERROR:
            HandleError(msg);
            break;
        case INNER_MSG_EOS:
            HandleEos();
            break;
        case INNER_MSG_BUFFERING:
            HandleBuffering(msg);
            break;
        case INNER_MSG_BUFFERING_TIME:
            HandleBufferingTime(msg);
            break;
        case INNER_MSG_BUFFERING_USED_MQ_NUM:
            HandleUsedMqNum(msg);
            break;
        case INNER_MSG_POSITION_UPDATE:
            HandlePositionUpdate();
            break;
        case INNER_MSG_VIDEO_ROTATION:
            HandleVideoRotation(msg);
            break;
        default:
            break;
    }
}

void PlayBinCtrlerBase::IdleState::StateEnter()
{
    ctrler_.ExitInitializedState();
}

int32_t PlayBinCtrlerBase::InitializedState::Prepare()
{
    ctrler_.ChangeState(ctrler_.preparingState_);
    return MSERR_OK;
}

void PlayBinCtrlerBase::PreparingState::StateEnter()
{
    PlayBinMessage msg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_BUFFERING_START, 0, {} };
    ctrler_.ReportMessage(msg);

    GstStateChangeReturn ret;
    (void)ChangePlayBinState(GST_STATE_PAUSED, ret);

    MEDIA_LOGD("PreparingState::StateEnter finished");
}

int32_t PlayBinCtrlerBase::PreparingState::Stop()
{
    // change to stop always success
    ctrler_.ChangeState(ctrler_.stoppingState_);
    return MSERR_OK;
}

void PlayBinCtrlerBase::PreparingState::ProcessStateChange(const InnerMessage &msg)
{
    MEDIA_LOGI("PreparingState::ProcessStateChange");
    if ((msg.detail1 == GST_STATE_READY) && (msg.detail2 == GST_STATE_PAUSED)) {
        std::unique_lock<std::mutex> lock(ctrler_.mutex_);
        ctrler_.ChangeState(ctrler_.preparedState_);
        ctrler_.preparingCond_.notify_one(); // awake the prepaingCond_'s waiter in Prepare()
        ctrler_.preparedCond_.notify_one(); // awake the preparedCond_'s waiter in Prepare()
        MEDIA_LOGI("preparingCond_.notify_one, preparing->prepard");
    }
}

void PlayBinCtrlerBase::PreparedState::StateEnter()
{
    PlayBinMessage msg = { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_BUFFERING_END, 0, {} };
    ctrler_.ReportMessage(msg);

    ctrler_.QueryDuration();
    PlayBinMessage posUpdateMsg { PLAYBIN_MSG_POSITION_UPDATE, PLAYBIN_SUB_MSG_POSITION_UPDATE_FORCE,
        0, static_cast<int32_t>(ctrler_.duration_ / USEC_PER_MSEC) };
    ctrler_.ReportMessage(posUpdateMsg);

    msg = { PLAYBIN_MSG_STATE_CHANGE, 0, PLAYBIN_STATE_PREPARED, {} };
    ctrler_.ReportMessage(msg);
}

int32_t PlayBinCtrlerBase::PreparedState::Prepare()
{
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PreparedState::Play()
{
    ctrler_.isUserSetPlay_ = true;
    GstStateChangeReturn ret;
    return ChangePlayBinState(GST_STATE_PLAYING, ret);
}

int32_t PlayBinCtrlerBase::PreparedState::Seek(int64_t timeUs, int32_t option)
{
    return ctrler_.SeekInternal(timeUs, option);
}

int32_t PlayBinCtrlerBase::PreparedState::Stop()
{
    // change to stop always success
    ctrler_.ChangeState(ctrler_.stoppingState_);
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PreparedState::SetRate(double rate)
{
    return ctrler_.SetRateInternal(rate);
}

void PlayBinCtrlerBase::PreparedState::ProcessStateChange(const InnerMessage &msg)
{
    MEDIA_LOGI("PreparingState::ProcessStateChange");
    if ((msg.detail1 == GST_STATE_PAUSED) && (msg.detail2 == GST_STATE_PLAYING) && ctrler_.isUserSetPlay_) {
        ctrler_.isUserSetPlay_ = false;
        ctrler_.ChangeState(ctrler_.playingState_);
        MEDIA_LOGI("prepared->playing");
        return;
    }
}

void PlayBinCtrlerBase::PlayingState::StateEnter()
{
    PlayBinMessage msg = { PLAYBIN_MSG_STATE_CHANGE, 0, PLAYBIN_STATE_PLAYING, {} };
    ctrler_.ReportMessage(msg);

    msg.type = PLAYBIN_MSG_SUBTYPE;
    msg.subType = PLAYBIN_SUB_MSG_VIDEO_RENDING_START;
    ctrler_.ReportMessage(msg);

    // add tick handler to periodically query the current location
    // if the state = eos, should seek to 0 position
}

int32_t PlayBinCtrlerBase::PlayingState::Play()
{
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PlayingState::Pause()
{
    GstState state = GST_STATE_NULL;
    gst_element_get_state(GST_ELEMENT_CAST(ctrler_.playbin_), &state, nullptr, static_cast<GstClockTime>(0));
    if (state == GST_STATE_PAUSED) {
        MEDIA_LOGI("playbin already paused");
        ctrler_.ChangeState(ctrler_.pausedState_);
        return MSERR_OK;
    }
    ctrler_.isUserSetPause_ = true;
    GstStateChangeReturn ret;
    MEDIA_LOGI("playing->pause start");
    return ChangePlayBinState(GST_STATE_PAUSED, ret);
}

int32_t PlayBinCtrlerBase::PlayingState::Seek(int64_t timeUs, int32_t option)
{
    MEDIA_LOGI("playing->seek start");
    return ctrler_.SeekInternal(timeUs, option);
}

int32_t PlayBinCtrlerBase::PlayingState::Stop()
{
    // change to stop always success
    MEDIA_LOGI("playing->stopping start");
    ctrler_.ChangeState(ctrler_.stoppingState_);
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PlayingState::SetRate(double rate)
{
    MEDIA_LOGI("playing->speed start");
    return ctrler_.SetRateInternal(rate);
}

void PlayBinCtrlerBase::PlayingState::HandleAsyncDone(const InnerMessage &msg)
{
    MEDIA_LOGI("PreparingState::HandleAsyncDone");
    BaseState::HandleAsyncDone(msg);
}

void PlayBinCtrlerBase::PlayingState::ProcessStateChange(const InnerMessage &msg)
{
    MEDIA_LOGI("PreparingState::ProcessStateChange");
    if (msg.detail1 == GST_STATE_PLAYING && msg.detail2 == GST_STATE_PAUSED && ctrler_.isUserSetPause_) {
        ctrler_.ChangeState(ctrler_.pausedState_);
        ctrler_.isUserSetPause_ = false;
        MEDIA_LOGI("playing->paused end");
        return;
    }

    if (msg.detail2 == GST_STATE_PLAYING) {
        GstState state = GST_STATE_NULL;
        GstStateChangeReturn stateRet = gst_element_get_state(GST_ELEMENT_CAST(ctrler_.playbin_), &state,
            nullptr, static_cast<GstClockTime>(0));
        if ((stateRet == GST_STATE_CHANGE_SUCCESS) && (state == GST_STATE_PLAYING)) {
            if (ctrler_.isSeeking_) {
                int64_t position = ctrler_.seekPos_ / USEC_PER_MSEC;
                ctrler_.isSeeking_ = false;
                ctrler_.isDuration_ = (position == ctrler_.duration_ / USEC_PER_MSEC) ? true : false;
                MEDIA_LOGI("playing after seek done, pos = %{public}" PRIi64 "ms", position);
                PlayBinMessage posUpdateMsg { PLAYBIN_MSG_POSITION_UPDATE, PLAYBIN_SUB_MSG_POSITION_UPDATE_FORCE,
                    static_cast<int32_t>(ctrler_.QueryPosition()),
                    static_cast<int32_t>(ctrler_.duration_ / USEC_PER_MSEC) };
                ctrler_.ReportMessage(posUpdateMsg);

                PlayBinMessage playBinMsg { PLAYBIN_MSG_SEEKDONE, 0, static_cast<int32_t>(position), {} };
                ctrler_.ReportMessage(playBinMsg);
            } else if (ctrler_.isRating_) {
                ctrler_.isRating_ = false;
                MEDIA_LOGI("playing after setRate done, rate = %{public}lf", ctrler_.rate_);
                PlayBinMessage posUpdateMsg { PLAYBIN_MSG_POSITION_UPDATE, PLAYBIN_SUB_MSG_POSITION_UPDATE_FORCE,
                    static_cast<int32_t>(ctrler_.QueryPosition()),
                    static_cast<int32_t>(ctrler_.duration_ / USEC_PER_MSEC) };
                ctrler_.ReportMessage(posUpdateMsg);

                PlayBinMessage playBinMsg { PLAYBIN_MSG_SPEEDDONE, 0, 0, ctrler_.rate_ };
                ctrler_.ReportMessage(playBinMsg);
            } else {
                MEDIA_LOGD("playing, not seeking or rating!");
            }
        }
    }
}

void PlayBinCtrlerBase::PlayingState::HandlePositionUpdate()
{
    ctrler_.QueryDuration();
    int64_t position = ctrler_.QueryPosition();
    PlayBinMessage posUpdateMsg { PLAYBIN_MSG_POSITION_UPDATE, PLAYBIN_SUB_MSG_POSITION_UPDATE_UNFORCE,
        static_cast<int32_t>(position), static_cast<int32_t>(ctrler_.duration_ / USEC_PER_MSEC)};
    ctrler_.ReportMessage(posUpdateMsg);
}

void PlayBinCtrlerBase::PausedState::StateEnter()
{
    PlayBinMessage msg = { PLAYBIN_MSG_STATE_CHANGE, 0, PLAYBIN_STATE_PAUSED, {} };
    ctrler_.ReportMessage(msg);
}

int32_t PlayBinCtrlerBase::PausedState::Play()
{
    MEDIA_LOGI("paused->play start");
    ctrler_.isUserSetPlay_ = true;
    GstStateChangeReturn ret;
    return ChangePlayBinState(GST_STATE_PLAYING, ret);
}

int32_t PlayBinCtrlerBase::PausedState::Pause()
{
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PausedState::Seek(int64_t timeUs, int32_t option)
{
    MEDIA_LOGI("paused->seek start");
    return ctrler_.SeekInternal(timeUs, option);
}

int32_t PlayBinCtrlerBase::PausedState::Stop()
{
    MEDIA_LOGI("paused->stopping start");
    // change to stop always success
    ctrler_.ChangeState(ctrler_.stoppingState_);
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PausedState::SetRate(double rate)
{
    MEDIA_LOGI("paused->speed start");
    return ctrler_.SetRateInternal(rate);
}

void PlayBinCtrlerBase::PausedState::HandleAsyncDone(const InnerMessage &msg)
{
    MEDIA_LOGI("PausedState::HandleAsyncDone");
    BaseState::HandleAsyncDone(msg);
}

void PlayBinCtrlerBase::PausedState::ProcessStateChange(const InnerMessage &msg)
{
    MEDIA_LOGI("PausedState::ProcessStateChange");
    if ((msg.detail1 == GST_STATE_PAUSED) && (msg.detail2 == GST_STATE_PLAYING) && ctrler_.isUserSetPlay_) {
        ctrler_.isUserSetPlay_ = false;
        ctrler_.ChangeState(ctrler_.playingState_);
    }
}

void PlayBinCtrlerBase::StoppingState::StateEnter()
{
    // maybe need the deferred task to change state from ready to null, refer to gstplayer.
    GstStateChangeReturn ret;
    (void)ChangePlayBinState(GST_STATE_READY, ret);
    if (ret == GST_STATE_CHANGE_SUCCESS) {
        ctrler_.ChangeState(ctrler_.stoppedState_);
    }
    ctrler_.isDuration_ = false;

    MEDIA_LOGD("StoppingState::StateEnter finished");
}

int32_t PlayBinCtrlerBase::StoppingState::Prepare()
{
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::StoppingState::Stop()
{
    return MSERR_OK;
}

void PlayBinCtrlerBase::StoppingState::ProcessStateChange(const InnerMessage &msg)
{
    MEDIA_LOGI("StoppingState::ProcessStateChange");
    if (msg.detail2 == GST_STATE_READY) {
        std::unique_lock<std::mutex> lock(ctrler_.mutex_);
        ctrler_.ChangeState(ctrler_.stoppedState_);
        ctrler_.stoppingCond_.notify_one(); // awake the stoppingCond_'s waiter in Stop()
        MEDIA_LOGI("stoppingCond_.notify_one stopping->stopped");
    }
}

void PlayBinCtrlerBase::StoppedState::StateEnter()
{
    PlayBinMessage playBinMsg = { PLAYBIN_MSG_STATE_CHANGE, 0, PLAYBIN_STATE_STOPPED, {} };
    ctrler_.ReportMessage(playBinMsg);

    MEDIA_LOGD("StoppedState::StateEnter finished");
}

int32_t PlayBinCtrlerBase::StoppedState::Prepare()
{
    MEDIA_LOGI("stopped->preparing start");
    ctrler_.ChangeState(ctrler_.preparingState_);
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::StoppedState::Stop()
{
    return MSERR_OK;
}

void PlayBinCtrlerBase::StoppedState::ProcessStateChange(const InnerMessage &msg)
{
    MEDIA_LOGI("StoppedState::ProcessStateChange");
    (void)msg;
}

void PlayBinCtrlerBase::PlaybackCompletedState::StateEnter()
{
    PlayBinMessage msg = { PLAYBIN_MSG_STATE_CHANGE, 0, PLAYBIN_STATE_PLAYBACK_COMPLETE, {} };
    ctrler_.ReportMessage(msg);
}

int32_t PlayBinCtrlerBase::PlaybackCompletedState::Play()
{
    ctrler_.isUserSetPlay_ = true;
    ctrler_.isDuration_ = false;
    PlayBinMessage posUpdateMsg { PLAYBIN_MSG_POSITION_UPDATE, PLAYBIN_SUB_MSG_POSITION_UPDATE_FORCE,
        0, static_cast<int32_t>(ctrler_.duration_ / USEC_PER_MSEC) };
    ctrler_.ReportMessage(posUpdateMsg);

    GstState state = GST_STATE_NULL;
    gst_element_get_state(GST_ELEMENT_CAST(ctrler_.playbin_), &state, nullptr, static_cast<GstClockTime>(0));
    if (state == GST_STATE_PAUSED) { // completed + seek + play
        GstStateChangeReturn ret;
        MEDIA_LOGI("completed->seek->play");
        return ChangePlayBinState(GST_STATE_PLAYING, ret);
    } else { // completed + play
        ctrler_.isReplay_ = true;
        MEDIA_LOGI("completed->play");
        return ctrler_.SeekInternal(0, IPlayBinCtrler::PlayBinSeekMode::PREV_SYNC);
    }
}

int32_t PlayBinCtrlerBase::PlaybackCompletedState::Pause()
{
    MEDIA_LOGI("completed state does not support pause");
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PlaybackCompletedState::Stop()
{
    MEDIA_LOGI("completed->stopping");
    ctrler_.ChangeState(ctrler_.stoppingState_);
    return MSERR_OK;
}

int32_t PlayBinCtrlerBase::PlaybackCompletedState::Seek(int64_t timeUs, int32_t option)
{
    MEDIA_LOGI("completed->seek");
    GstState state = GST_STATE_NULL;
    gst_element_get_state(GST_ELEMENT_CAST(ctrler_.playbin_), &state, nullptr, static_cast<GstClockTime>(0));
    if (state == GST_STATE_PLAYING) {
        GstStateChangeReturn ret;
        MEDIA_LOGI("completed GST_STATE_PLAYING->GST_STATE_PAUSED");
        (void)ChangePlayBinState(GST_STATE_PAUSED, ret);
    }
    return ctrler_.SeekInternal(timeUs, option);
}

int32_t PlayBinCtrlerBase::PlaybackCompletedState::SetRate(double rate)
{
    MEDIA_LOGI("completed->speed");
    GstState state = GST_STATE_NULL;
    gst_element_get_state(GST_ELEMENT_CAST(ctrler_.playbin_), &state, nullptr, static_cast<GstClockTime>(0));
    if (state == GST_STATE_PAUSED) { // completed + seek + speed
        ctrler_.SetRateInternal(rate);
    } else { // completed + speed
        ctrler_.rate_ = rate;
        PlayBinMessage msg = { PLAYBIN_MSG_SPEEDDONE, 0, 0, rate };
        ctrler_.ReportMessage(msg);
    }
    return MSERR_OK;
}

void PlayBinCtrlerBase::PlaybackCompletedState::ProcessStateChange(const InnerMessage &msg)
{
    MEDIA_LOGI("PlaybackCompletedState::ProcessStateChange");
    if (msg.detail2 == GST_STATE_PLAYING && ctrler_.isUserSetPlay_) {
        ctrler_.isUserSetPlay_ = false;
        ctrler_.ChangeState(ctrler_.playingState_);
    }
}

void PlayBinCtrlerBase::PlaybackCompletedState::HandleAsyncDone(const InnerMessage &msg)
{
    MEDIA_LOGI("PlaybackCompletedState::HandleAsyncDone");
    if (ctrler_.isReplay_) {
        ctrler_.isSeeking_ = false;
        ctrler_.isReplay_ = false;
    } else {
        BaseState::HandleAsyncDone(msg);
    }
}
} // namespace Media
} // namespace OHOS