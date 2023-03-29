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

#include "player_server_state.h"
#include "media_errors.h"
#include "media_log.h"
#include "media_dfx.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerServerState"};
}

namespace OHOS {
namespace Media {
void PlayerServer::BaseState::ReportInvalidOperation() const
{
    MEDIA_LOGE("invalid operation for %{public}s", GetStateName().c_str());
    (void)server_.taskMgr_.MarkTaskDone("ReportInvalidOperation");
}

int32_t PlayerServer::BaseState::Prepare()
{
    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayerServer::BaseState::Play()
{
    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayerServer::BaseState::Pause()
{
    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayerServer::BaseState::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    (void)mSeconds;
    (void)mode;

    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayerServer::BaseState::Stop()
{
    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayerServer::BaseState::SetPlaybackSpeed(PlaybackRateMode mode)
{
    (void)mode;

    ReportInvalidOperation();
    return MSERR_INVALID_STATE;
}

int32_t PlayerServer::BaseState::OnMessageReceived(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    MEDIA_LOGD("message received, type = %{public}d, extra = %{public}d", type, extra);
    (void)infoBody;

    if (type == INFO_TYPE_SEEKDONE) {
        int32_t ret = MSERR_OK;
        (void)server_.taskMgr_.MarkTaskDone("seek done");
        MediaTrace::TraceEnd("PlayerServer::Seek", FAKE_POINTER(&server_));
        if (server_.disableNextSeekDone_ && extra == 0) {
            ret = MSERR_UNSUPPORT;
        }
        server_.disableNextSeekDone_ = false;
        return ret;
    }
        
    if (type == INFO_TYPE_SPEEDDONE) {
        (void)server_.taskMgr_.MarkTaskDone("speed done");
        MediaTrace::TraceEnd("PlayerServer::SetPlaybackSpeed", FAKE_POINTER(&server_));
        return MSERR_OK;
    }

    if (type == INFO_TYPE_EOS) {
        HandleEos();
        return MSERR_OK;
    }

    if (type == INFO_TYPE_STATE_CHANGE) {
        if (extra == PLAYER_PLAYBACK_COMPLETE) {
            HandlePlaybackComplete(extra);
        } else {
            HandleStateChange(extra);
            BehaviorEventWrite(server_.GetStatusDescription(extra).c_str(), "Player");
            MEDIA_LOGI("Callback State change, currentState is %{public}s",
                server_.GetStatusDescription(extra).c_str());
        }

        if (extra == PLAYER_STOPPED && server_.disableStoppedCb_) {
            server_.disableStoppedCb_ = false;
            return MSERR_UNSUPPORT;
        }
    }
    return MSERR_OK;
}

void PlayerServer::IdleState::StateEnter()
{
    (void)server_.HandleReset();
}

int32_t PlayerServer::InitializedState::Prepare()
{
    server_.ChangeState(server_.preparingState_);
    return MSERR_OK;
}

void PlayerServer::PreparingState::StateEnter()
{
    (void)server_.HandlePrepare();
    MEDIA_LOGD("PlayerServer::PreparingState::StateEnter finished");
}

int32_t PlayerServer::PreparingState::Stop()
{
    (void)server_.HandleStop();
    server_.ChangeState(server_.stoppedState_);
    return MSERR_OK;
}

void PlayerServer::PreparingState::HandleStateChange(int32_t newState)
{
    if (newState == PLAYER_PREPARED || newState == PLAYER_STATE_ERROR) {
        MediaTrace::TraceEnd("PlayerServer::PrepareAsync", FAKE_POINTER(&server_));
        if (newState == PLAYER_STATE_ERROR) {
            server_.lastOpStatus_ = PLAYER_STATE_ERROR;
            server_.ChangeState(server_.initializedState_);
        } else {
            server_.ChangeState(server_.preparedState_);
        }
        (void)server_.taskMgr_.MarkTaskDone("preparing->prepared done");
    }
}

int32_t PlayerServer::PreparedState::Prepare()
{
    (void)server_.taskMgr_.MarkTaskDone("double prepare");
    return MSERR_OK;
}

int32_t PlayerServer::PreparedState::Play()
{
    return server_.HandlePlay();
}

int32_t PlayerServer::PreparedState::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    return server_.HandleSeek(mSeconds, mode);
}

int32_t PlayerServer::PreparedState::Stop()
{
    return server_.HandleStop();
}

int32_t PlayerServer::PreparedState::SetPlaybackSpeed(PlaybackRateMode mode)
{
    return server_.HandleSetPlaybackSpeed(mode);
}

void PlayerServer::PreparedState::HandleStateChange(int32_t newState)
{
    if (newState == PLAYER_STARTED) {
        MediaTrace::TraceEnd("PlayerServer::Play", FAKE_POINTER(&server_));
        server_.ChangeState(server_.playingState_);
        (void)server_.taskMgr_.MarkTaskDone("prepared->started done");
    } else if (newState == PLAYER_STOPPED) {
        MediaTrace::TraceEnd("PlayerServer::Stop", FAKE_POINTER(&server_));
        server_.ChangeState(server_.stoppedState_);
        (void)server_.taskMgr_.MarkTaskDone("prepared->stopped done");
    }
}

int32_t PlayerServer::PlayingState::Play()
{
    (void)server_.taskMgr_.MarkTaskDone("double play");
    return MSERR_OK;
}

int32_t PlayerServer::PlayingState::Pause()
{
    return server_.HandlePause();
}

int32_t PlayerServer::PlayingState::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    return server_.HandleSeek(mSeconds, mode);
}

int32_t PlayerServer::PlayingState::Stop()
{
    return server_.HandleStop();
}

int32_t PlayerServer::PlayingState::SetPlaybackSpeed(PlaybackRateMode mode)
{
    return server_.HandleSetPlaybackSpeed(mode);
}

void PlayerServer::PlayingState::HandleStateChange(int32_t newState)
{
    if (newState == PLAYER_PAUSED) {
        MediaTrace::TraceEnd("PlayerServer::Pause", FAKE_POINTER(&server_));
        server_.ChangeState(server_.pausedState_);
        (void)server_.taskMgr_.MarkTaskDone("started->paused done");
    } else if (newState == PLAYER_STOPPED) {
        MediaTrace::TraceEnd("PlayerServer::Stop", FAKE_POINTER(&server_));
        server_.ChangeState(server_.stoppedState_);
        (void)server_.taskMgr_.MarkTaskDone("started->stopped done");
    }
}

void PlayerServer::PlayingState::HandlePlaybackComplete(int32_t extra)
{
    (void)extra;
    server_.lastOpStatus_ = PLAYER_PLAYBACK_COMPLETE;
    server_.ChangeState(server_.playbackCompletedState_);
    (void)server_.taskMgr_.MarkTaskDone("playing->completed done");
}

void PlayerServer::PlayingState::HandleEos()
{
    server_.HandleEos();
}

int32_t PlayerServer::PausedState::Play()
{
    return server_.HandlePlay();
}

int32_t PlayerServer::PausedState::Pause()
{
    (void)server_.taskMgr_.MarkTaskDone("double pause");
    return MSERR_OK;
}

int32_t PlayerServer::PausedState::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    return server_.HandleSeek(mSeconds, mode);
}

int32_t PlayerServer::PausedState::Stop()
{
    return server_.HandleStop();
}

int32_t PlayerServer::PausedState::SetPlaybackSpeed(PlaybackRateMode mode)
{
    return server_.HandleSetPlaybackSpeed(mode);
}

void PlayerServer::PausedState::HandleStateChange(int32_t newState)
{
    if (newState == PLAYER_STARTED) {
        MediaTrace::TraceEnd("PlayerServer::Play", FAKE_POINTER(&server_));
        server_.ChangeState(server_.playingState_);
        (void)server_.taskMgr_.MarkTaskDone("paused->started done");
    } else if (newState == PLAYER_STOPPED) {
        MediaTrace::TraceEnd("PlayerServer::Stop", FAKE_POINTER(&server_));
        server_.ChangeState(server_.stoppedState_);
        (void)server_.taskMgr_.MarkTaskDone("paused->stopped done");
    }
}

int32_t PlayerServer::StoppedState::Prepare()
{
    server_.ChangeState(server_.preparingState_);
    return MSERR_OK;
}

int32_t PlayerServer::StoppedState::Stop()
{
    (void)server_.taskMgr_.MarkTaskDone("double stop");
    return MSERR_OK;
}

int32_t PlayerServer::PlaybackCompletedState::Play()
{
    return server_.HandlePlay();
}

int32_t PlayerServer::PlaybackCompletedState::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    return server_.HandleSeek(mSeconds, mode);
}

int32_t PlayerServer::PlaybackCompletedState::Stop()
{
    return server_.HandleStop();
}

void PlayerServer::PlaybackCompletedState::HandleStateChange(int32_t newState)
{
    if (newState == PLAYER_STARTED) {
        MediaTrace::TraceEnd("PlayerServer::Play", FAKE_POINTER(&server_));
        server_.ChangeState(server_.playingState_);
        (void)server_.taskMgr_.MarkTaskDone("completed->started done");
    } else if (newState == PLAYER_STOPPED) {
        MediaTrace::TraceEnd("PlayerServer::Stop", FAKE_POINTER(&server_));
        server_.ChangeState(server_.stoppedState_);
        (void)server_.taskMgr_.MarkTaskDone("completed->stopped done");
    }
}

int32_t PlayerServer::PlaybackCompletedState::SetPlaybackSpeed(PlaybackRateMode mode)
{
    return server_.HandleSetPlaybackSpeed(mode);
}
}
}
