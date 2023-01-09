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

#include "player_server_task.h"
#include <unistd.h>
#include "media_log.h"
#include "media_errors.h"
#include "player_mem_manage.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerServerTask"};
}

namespace OHOS {
namespace Media {
int32_t PlayerServerTask::BaseState::OnMessageReceived(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    std::lock_guard<std::recursive_mutex> lock(serverTask_.recMutex_);
    if (type == INFO_TYPE_STATE_CHANGE) {
        if (extra == PLAYER_PLAYBACK_COMPLETE) {
            HandlePlaybackComplete(extra);
        } else {
            HandleStateChange(extra);
        }
    }

    if (serverTask_.recoveryConfig_.isReleaseMemByManage == false) {
        return MSERR_OK;
    }

    if (serverTask_.recoveryConfig_.curState == serverTask_.InitializedState ||
        serverTask_.recoveryConfig_.curState == serverTask_.StoppedState) {
        if (type == INFO_TYPE_STATE_CHANGE && extra == PLAYER_INITIALIZED) {
            (void)serverTask_.RecoverPlayerNormal();
        }
        return MSERR_OK;
    }

    if (serverTask_.recoveryConfig_.curState == serverTask_.PreparedState ||
        serverTask_.recoveryConfig_.curState == serverTask_.PausedState) {
        if (serverTask_.recoveryConfig_.currentTime != 0) {
            if (type == INFO_TYPE_SEEKDONE) {
                (void)serverTask_.RecoverPlayerNormal();
            } 
        } else if (serverTask_.recoveryConfig_.speedMode != SPEED_FORWARD_1_00_X) {
            if (type == INFO_TYPE_SPEEDDONE) {
                (void)serverTask_.RecoverPlayerNormal();
            }
        } else {
            if (type == INFO_TYPE_STATE_CHANGE && extra == PLAYER_PREPARED) {
                (void)serverTask_.RecoverPlayerNormal();
            }
        }
        return MSERR_OK;
    }

    if (serverTask_.recoveryConfig_.curState == serverTask_.PlaybackCompletedState) {
        if (serverTask_.recoveryConfig_.speedMode != SPEED_FORWARD_1_00_X) {
            if (type == INFO_TYPE_SPEEDDONE) {
                (void)serverTask_.RecoverPlayerNormal();
            }
        } else {
            if (type == INFO_TYPE_STATE_CHANGE && extra == PLAYER_PREPARED) {
                (void)serverTask_.RecoverPlayerNormal();
            }
        }
        return MSERR_OK;
    }

    return MSERR_OK;
}

int32_t PlayerServerTask::IdleState::StateRecover()
{
    MEDIA_LOGE("PlayerServerTask recover state can not be idle");
    return MSERR_INVALID_OPERATION;
}

int32_t PlayerServerTask::InitializedState::StateRecover()
{
    int32_t ret = serverTask_.SetSourceInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "InitializedState failed to SetSource url");

    ret = serverTask_.SetConfigInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "InitializedState failed to SetConfigInternal");
    return MSERR_OK;
}

int32_t PlayerServerTask::InitializedState::HandleStateChange(int32_t newState)
{
    if (newState == PLAYER_PREPARED) {
        serverTask_.ChangeState(serverTask_.preparedState_);
    }
}

int32_t PlayerServerTask::PreparedState::StateRecover()
{
    int32_t ret = serverTask_.SetSourceInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to SetSource url");

    ret = serverTask_.SetConfigInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to SetConfigInternal");

    ret = serverTask_.PrepareAsync();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to PrepareAsync");

    ret = serverTask_.SetBehaviorInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to SetBehaviorInternal");

    return ret;
}

int32_t PlayerServerTask::PreparedState::HandleStateChange(int32_t newState)
{
    if (newState == PLAYER_STARTED) {
        serverTask_.ChangeState(serverTask_.playingState_);
    } else if (newState == PLAYER_STOPPED) {
        serverTask_.ChangeState(serverTask_.stoppedState_);
    }
}

int32_t PlayerServerTask::PlayingState::StateRecover()
{
    MEDIA_LOGE("PlayerServerTask recover state can not be playing");
    return MSERR_INVALID_OPERATION;
}

int32_t PlayerServerTask::PlayingState::HandleStateChange(int32_t newState)
{
    if (newState == PLAYER_PAUSED) {
        serverTask_.ChangeState(serverTask_.pausedState_);
    } else if (newState == PLAYER_STOPPED) {
        serverTask_.ChangeState(serverTask_.stoppedState_);
    }
}

int32_t PlayerServerTask::PlayingState::HandlePlaybackComplete(int32_t extra)
{
    serverTask_.ChangeState(serverTask_.playbackCompletedState_);
}

int32_t PlayerServerTask::PausedState::StateRecover()
{
    int32_t ret = serverTask_.SetSourceInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to SetSource url");

    ret = serverTask_.SetConfigInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to SetConfigInternal");

    ret = serverTask_.PrepareAsync();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to PrepareAsync");

    ret = serverTask_.SetBehaviorInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to SetBehaviorInternal");

    return ret;
}

int32_t PlayerServerTask::PausedState::HandleStateChange(int32_t newState)
{
    if (newState == PLAYER_STARTED) {
        serverTask_.ChangeState(serverTask_.playingState_);
    } else if (newState == PLAYER_STOPPED) {
        serverTask_.ChangeState(serverTask_.stoppedState_);
    }
}

int32_t PlayerServerTask::StoppedState::StateRecover()
{
    int32_t ret = serverTask_.SetSourceInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "InitializedState failed to SetSource url");

    ret = serverTask_.SetConfigInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "InitializedState failed to SetConfigInternal");
    return MSERR_OK;
}

int32_t PlayerServerTask::StoppedState::HandleStateChange(int32_t newState)
{
    if (newState == PLAYER_PREPARED) {
        serverTask_.ChangeState(serverTask_.preparedState_);
    }
}

int32_t PlayerServerTask::PlaybackCompletedState::StateRecover()
{
    int32_t ret = serverTask_.SetSourceInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to SetSource url");

    ret = serverTask_.SetConfigInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to SetConfigInternal");

    ret = serverTask_.PrepareAsync();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to PrepareAsync");

    if (serverTask_.recoveryConfig_->speedMode != SPEED_FORWARD_1_00_X) {
        ret = serverTask_.SetPlaybackSpeed(serverTask_.recoveryConfig_->speedMode);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to SetPlaybackSpeed");
    }

    return ret;
}

int32_t PlayerServerTask::PlaybackCompletedState::HandleStateChange(int32_t newState)
{
    if (newState == PLAYER_STARTED) {
        serverTask_.ChangeState(serverTask_.playingState_);
    } else if (newState == PLAYER_STOPPED) {
        serverTask_.ChangeState(serverTask_.stoppedState_);
    }
}

std::shared_ptr<IPlayerService> PlayerServerTask::Create()
{
    std::shared_ptr<PlayerServerTask> serverTask = std::make_shared<PlayerServerTask>();
    CHECK_AND_RETURN_RET_LOG(serverTask != nullptr, nullptr, "failed to new PlayerServerTask");

    (void)serverTask->Init();
    return serverTask;
}

PlayerServerTask::PlayerServerTask() : taskQue_("PlayerServerTask")
{
    (void)taskQue_.Start();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerServerTask::~PlayerServerTask()
{
    if (playerServer_ != nullptr) {
        auto task = std::make_shared<TaskHandler<void>>([&, this] {
            (void)playerServer_->Release();
            playerServer_ = nullptr;
        });
        (void)taskQue_.EnqueueTask(task);
        (void)task->GetResult();
    }
    (void)taskQue_.Stop();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t PlayerServerTask::Init()
{
    if (playerServer_ == nullptr) {
        playerServer_ = PlayerServer::Create();
    }
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "failed to create PlayerServer");

    std::shared_ptr<IPlayerEngineObs> obs = shared_from_this();
    auto ret = std::static_pointer_cast<PlayerServer>(playerServer_)->SetObs(obs);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetObs Failed!");

    idleState_ = std::make_shared<IdleState>(*this);
    initializedState_ = std::make_shared<InitializedState>(*this);
    preparedState_ = std::make_shared<PreparedState>(*this);
    playingState_ = std::make_shared<PlayingState>(*this);
    pausedState_ = std::make_shared<PausedState>(*this);
    stoppedState_ = std::make_shared<StoppedState>(*this);
    playbackCompletedState_ = std::make_shared<PlaybackCompletedState>(*this);

    PlayerServerStateMachine::Init(idleState_);
    return MSERR_OK;
}

int32_t PlayerServerTask::SetSourceInternal()
{
    int32_t ret;
    switch (recoveryConfig_->sourceType) {
        case SOURCE_TYPE_URL:
            ret = SetSource(recoveryConfig_->url);
            break;

        case SOURCE_TYPE_DATASRC:
            ret = SetSource(recoveryConfig_->dataSrc);
            break;

        case SOURCE_TYPE_FD:
            ret = SetSource(recoveryConfig_->fd, recoveryConfig_->offset, recoveryConfig_->size);
            break;
        
        default:
            ret = MSERR_INVALID_OPERATION;
            break;
    }
    return ret;
}

int32_t PlayerServerTask::SetConfigInternal()
{
    int32_t ret = MSERR_OK;;
    if (recoveryConfig_->leftVolume != 1.0 || recoveryConfig_->rightVolume != 1.0) {
        ret = SetVolume(recoveryConfig_->leftVolume, recoveryConfig_->rightVolume);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SetVolume");
    }

    if (recoveryConfig_->surface != nullptr) {
        ret = SetVideoSurface(recoveryConfig_->surface);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SetVideoSurface");
    }

    if (recoveryConfig_->loop != false) {
        ret = SetLooping(recoveryConfig_->loop);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SetLooping");
    }

    if (recoveryConfig_->isHasSetParam == true) {
        ret = SetParameter(recoveryConfig_->param);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SetParameter");
    }

    if (recoveryConfig_->bitRate != 0) {
        ret = SelectBitRate(recoveryConfig_->bitRate);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SelectBitRate");
    }
    return ret;
}

int32_t PlayerServerTask::SetBehaviorInternal()
{
    int ret = MSERR_OK;
    if (recoveryConfig_->speedMode != SPEED_FORWARD_1_00_X) {
        ret = SetPlaybackSpeed(recoveryConfig_->speedMode);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SetPlaybackSpeed");
    }

    if (recoveryConfig_->currentTime != 0) {
        ret = Seek(recoveryConfig_->currentTime, SEEK_CLOSEST);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to Seek");
    }

    return ret;
}

int32_t PlayerServerTask::RecoverPlayerNormal()
{
    if (recoveryConfig_.callback != nullptr) {
        std::static_pointer_cast<PlayerServer>(playerServer_)->SetPlayerCallbackInner(recoveryConfig_.callback);
    }
    recoveryConfig_.isReleaseMemByManage = false;
    return MSERR_OK;
}

int32_t PlayerServerTask::SetSource(const std::string &url)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    
    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->SetSource(url);
        if (ret == MSERR_OK) {
            recoveryConfig_->url = url;
            recoveryConfig_->sourceType = SOURCE_TYPE_URL;
            ChangeState(initializedState_);
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to SetSource url");
    return result.value;
}

int32_t PlayerServerTask::SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->SetSource(dataSrc);
        if (ret == MSERR_OK) {
            recoveryConfig_->dataSrc = dataSrc;
            recoveryConfig_->sourceType = SOURCE_TYPE_DATASRC;
            ChangeState(initializedState_);
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to SetSource dataSrc");
    return result.value;
}

int32_t PlayerServerTask::SetSource(int32_t fd, int64_t offset, int64_t size)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->SetSource(fd, offset, size);
        if (ret == MSERR_OK) {
            recoveryConfig_->fd = fd;
            recoveryConfig_->offset = offset;
            recoveryConfig_->size = size;
            recoveryConfig_->sourceType = SOURCE_TYPE_FD;
            ChangeState(initializedState_);
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to SetSource dataSrc");
    return result.value;
}

int32_t PlayerServerTask::Play()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->play();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to Play");
    return result.value;
}

int32_t PlayerServerTask::Prepare()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->Prepare();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to Prepare");
    return result.value;
}

int32_t PlayerServerTask::PrepareAsync()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->PrepareAsync();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to PrepareAsync");
    return result.value;
}

int32_t PlayerServerTask::Pause()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->Pause();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to Pause");
    return result.value;
}

int32_t PlayerServerTask::Stop()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->Stop();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to Stop");
    return result.value;
}

int32_t PlayerServerTask::Reset()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->Reset();
        if (ret == MSERR_OK) {
            ChangeState(idleState_);
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to Reset");
    return result.value;
}

int32_t PlayerServerTask::Release()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->Release();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to Release");
    return result.value;
}

int32_t PlayerServerTask::SetVolume(float leftVolume, float rightVolume)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    
    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->SetVolume(leftVolume, rightVolume);
        if (ret == MSERR_OK) {
            recoveryConfig_->leftVolume = leftVolume;
            recoveryConfig_->rightVolume = rightVolume;
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to SetVolume");
    return result.value;
}

int32_t PlayerServerTask::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->Seek(mSeconds, mode);
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to Seek");
    return result.value;
}

int32_t PlayerServerTask::GetCurrentTime(int32_t &currentTime)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->GetCurrentTime(currentTime);
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to GetCurrentTime");
    return result.value;
}

int32_t PlayerServerTask::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->GetVideoTrackInfo(videoTrack);
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to GetVideoTrackInfo");
    return result.value;
}

int32_t PlayerServerTask::GetAudioTrackInfo(std::vector<Format> &audioTrack)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->GetAudioTrackInfo(audioTrack);
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to GetAudioTrackInfo");
    return result.value;
}

int32_t PlayerServerTask::GetVideoWidth()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->GetVideoWidth();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to GetVideoWidth");
    return result.value;
}

int32_t PlayerServerTask::GetVideoHeight()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->GetVideoHeight();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to GetVideoHeight");
    return result.value;
}

int32_t PlayerServerTask::GetDuration()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->GetDuration();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to GetDuration");
    return result.value;
}

int32_t PlayerServerTask::SetPlaybackSpeed(PlaybackRateMode mode)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    
    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->SetPlaybackSpeed();
        if (ret == MSERR_OK) {
            recoveryConfig_->speedMode = mode;
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to SetPlaybackSpeed");
    return result.value;
}

int32_t PlayerServerTask::GetPlaybackSpeed(PlaybackRateMode &mode)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->GetPlaybackSpeed(mode);
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to GetPlaybackSpeed");
    return result.value;
}

#ifdef SUPPORT_VIDEO
int32_t PlayerServerTask::SetVideoSurface(sptr<Surface> surface)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->SetVideoSurface(surface);
        if (ret == MSERR_OK) {
            recoveryConfig_->surface = surface;
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to SetVideoSurface");
    return result.value;
}
#endif

int32_t PlayerServerTask::IsPlaying()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->IsPlaying();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to IsPlaying");
    return result.value;
}

int32_t PlayerServerTask::IsLooping()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->IsLooping();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to IsLooping");
    return result.value;
}

int32_t PlayerServerTask::SetLooping(bool loop)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->SetLooping(loop);
        if (ret == MSERR_OK) {
            recoveryConfig_->loop = loop;
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to SetLooping");
    return result.value;
}

int32_t PlayerServerTask::SetParameter(const Format &param)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->SetParameter(param);
        if (ret == MSERR_OK) {
            recoveryConfig_->isHasSetParam = true;
            recoveryConfig_->param = param;
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to SetParameter");
    return result.value;
}

int32_t PlayerServerTask::SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->SetPlayerCallback(callback);
        if (ret == MSERR_OK) {
            recoveryConfig_->callback = callback;
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to SetPlayerCallback");
    return result.value;
}

int32_t PlayerServerTask::SelectBitRate(bitRate)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);

    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        auto ret = playerServer_->SelectBitRate(bitRate);
        if (ret == MSERR_OK) {
            recoveryConfig_->bitRate = bitRate;
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to SelectBitRate");
    return result.value;
}

void PlayerServerTask::OnError(PlayerErrorType errorType, int32_t errorCode)
{
    (void)errorType;
    (void)errorCode;
}

void PlayerServerTask::OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    (void)HandleMessage(type, extra, infoBody);
}

int32_t PlayerServerTask::DumpInfo(int32_t fd)
{
    auto ret = std::static_pointer_cast<PlayerServer>(playerServer_)->DumpInfo(fd);
    return ret;
}

int32_t PlayerServerTask::ReleaseMemByManage()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    MEDIA_LOGD("Enter ReleaseMemByManage");
    if (recoveryConfig_.isReleaseMemByManage == true) {
        MEDIA_LOGE("Already ReleaseMemByManage");
        return MSERR_INVALID_OPERATION;
    }
    recoveryConfig_.curState = GetCurrState();
    if (recoveryConfig_.curState != idleState_ && recoveryConfig_.curState != playingState_) {
        auto ret = GetCurrentTime(recoveryConfig_.currentTime);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to GetCurrentTime");
    } else {
        return MSERR_OK;
    }
    recoveryConfig_.isReleaseMemByManage = true;
    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        std::static_pointer_cast<PlayerServer>(playerServer_)->SetPlayerCallbackInner(nullptr);
        auto ret = playerServer_->Reset();
        if (ret == MSERR_OK) {
            ChangeState(idleState_);
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to Reset");
    return result.value;
}

int32_t PlayerServerTask::RecoveryMemByUser()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    MEDIA_LOGD("Enter RecoveryMemByUser");
    if (recoveryConfig_.isReleaseMemByManage == true) {
        auto ret = recoveryConfig_.curState->StateRecover();
        return ret;
    }
    return MSERR_INVALID_OPERATION;
}

bool PlayerServerTask::IsReleaseMemByManage()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    return recoveryConfig_.isReleaseMemByManage;
}
}
}