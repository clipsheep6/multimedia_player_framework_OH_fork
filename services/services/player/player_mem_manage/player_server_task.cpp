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
#include "mem_mgr_client.h"
#include "param_wrapper.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerServerTask"};
}

namespace OHOS {
namespace Media {
constexpr int32_t PER_INSTANCE_NEED_MEMORY_PERCENT = 10;
constexpr int32_t ONE_HUNDRED = 100;
int32_t PlayerServerTask::BaseState::OnMessageReceived(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    (void)infoBody;
    MEDIA_LOGI("Message Received type:%{public}d extra:%{public}d stateName:%{public}s",
        type, extra, GetStateName().c_str());
    if (type == INFO_TYPE_STATE_CHANGE) {
        if (extra == PLAYER_PLAYBACK_COMPLETE) {
            HandlePlaybackComplete(extra);
        } else {
            HandleStateChange(extra);
        }
    }

    serverTask_.CheckHasRecover(type, extra);

    return MSERR_OK;
}

int32_t PlayerServerTask::IdleState::StateRecover()
{
    MEDIA_LOGE("PlayerServerTask recover state can not be idle");
    return MSERR_INVALID_OPERATION;
}

int32_t PlayerServerTask::InitializedState::StateRecover()
{
    MEDIA_LOGI("InitializedState recover");
    int32_t ret = serverTask_.SetSourceInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "InitializedState failed to SetSource url");

    ret = serverTask_.SetConfigInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "InitializedState failed to SetConfigInternal");
    return MSERR_OK;
}

void PlayerServerTask::InitializedState::HandleStateChange(int32_t newState)
{
    MEDIA_LOGI("InitializedState HandleStateChange newState:%{public}d", newState);
    if (newState == PLAYER_PREPARED) {
        serverTask_.ChangeState(serverTask_.preparedState_);
    }
}

int32_t PlayerServerTask::PreparedState::StateRecover()
{
    MEDIA_LOGI("PreparedState recover");
    int32_t ret = serverTask_.SetSourceInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to SetSource url");

    ret = serverTask_.SetConfigInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to SetConfigInternal");

    ret = serverTask_.playerServer_->PrepareAsync();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to PrepareAsync");

    ret = serverTask_.SetBehaviorInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to SetBehaviorInternal");

    return ret;
}

void PlayerServerTask::PreparedState::HandleStateChange(int32_t newState)
{
    MEDIA_LOGI("PreparedState HandleStateChange newState:%{public}d", newState);
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

void PlayerServerTask::PlayingState::HandleStateChange(int32_t newState)
{
    MEDIA_LOGI("PlayingState HandleStateChange newState:%{public}d", newState);
    if (newState == PLAYER_PAUSED) {
        serverTask_.ChangeState(serverTask_.pausedState_);
    } else if (newState == PLAYER_STOPPED) {
        serverTask_.ChangeState(serverTask_.stoppedState_);
    }
}

void PlayerServerTask::PlayingState::HandlePlaybackComplete(int32_t extra)
{
    (void)extra;
    MEDIA_LOGI("PlayingState HandlePlaybackComplete extra:%{public}d", extra);
    serverTask_.ChangeState(serverTask_.playbackCompletedState_);
}

int32_t PlayerServerTask::PausedState::StateRecover()
{
    MEDIA_LOGI("PausedState recover");
    int32_t ret = serverTask_.SetSourceInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to SetSource url");

    ret = serverTask_.SetConfigInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to SetConfigInternal");

    ret = serverTask_.playerServer_->PrepareAsync();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to PrepareAsync");

    ret = serverTask_.SetBehaviorInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to SetBehaviorInternal");

    return ret;
}

void PlayerServerTask::PausedState::HandleStateChange(int32_t newState)
{
    MEDIA_LOGI("PausedState HandleStateChange newState:%{public}d", newState);
    if (newState == PLAYER_STARTED) {
        serverTask_.ChangeState(serverTask_.playingState_);
    } else if (newState == PLAYER_STOPPED) {
        serverTask_.ChangeState(serverTask_.stoppedState_);
    }
}

int32_t PlayerServerTask::StoppedState::StateRecover()
{
    MEDIA_LOGI("StoppedState recover");
    int32_t ret = serverTask_.SetSourceInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "InitializedState failed to SetSource url");

    ret = serverTask_.SetConfigInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "InitializedState failed to SetConfigInternal");
    return MSERR_OK;
}

void PlayerServerTask::StoppedState::HandleStateChange(int32_t newState)
{
    MEDIA_LOGI("StoppedState HandleStateChange newState:%{public}d", newState);
    if (newState == PLAYER_PREPARED) {
        serverTask_.ChangeState(serverTask_.preparedState_);
    }
}

int32_t PlayerServerTask::PlaybackCompletedState::StateRecover()
{
    MEDIA_LOGI("PlaybackCompletedState recover");
    int32_t ret = serverTask_.SetSourceInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to SetSource url");

    ret = serverTask_.SetConfigInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to SetConfigInternal");

    ret = serverTask_.playerServer_->PrepareAsync();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to PrepareAsync");

    if (serverTask_.recoverConfig_.speedMode != SPEED_FORWARD_1_00_X) {
        ret = serverTask_.playerServer_->SetPlaybackSpeed(serverTask_.recoverConfig_.speedMode);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to SetPlaybackSpeed");
    }

    return ret;
}

void PlayerServerTask::PlaybackCompletedState::HandleStateChange(int32_t newState)
{
    MEDIA_LOGI("PlaybackCompletedState HandleStateChange newState:%{public}d", newState);
    if (newState == PLAYER_STARTED) {
        serverTask_.ChangeState(serverTask_.playingState_);
    } else if (newState == PLAYER_STOPPED) {
        serverTask_.ChangeState(serverTask_.stoppedState_);
    }
}

std::shared_ptr<IPlayerService> PlayerServerTask::Create()
{
    MEDIA_LOGI("Create new PlayerServerTask");
    if (Memory::MemMgrClient::GetInstance().GetAvailableMemory() <=
        Memory::MemMgrClient::GetInstance().GetTotalMemory() / ONE_HUNDRED * PER_INSTANCE_NEED_MEMORY_PERCENT) {
        MEDIA_LOGE("System available memory:%{public}d insufficient, total memory:%{public}d",
            Memory::MemMgrClient::GetInstance().GetAvailableMemory(),
            Memory::MemMgrClient::GetInstance().GetTotalMemory());
        return nullptr;
    }
    std::shared_ptr<PlayerServerTask> serverTask = std::make_shared<PlayerServerTask>();
    CHECK_AND_RETURN_RET_LOG(serverTask != nullptr, nullptr, "failed to new PlayerServerTask");

    (void)serverTask->Init();
    return serverTask;
}

PlayerServerTask::PlayerServerTask() : taskQue_("PlayerServerTask")
{
    (void)taskQue_.Start();
    std::string isDisable;
    int32_t ret = OHOS::system::GetStringParameter("sys.media.memory.recycle.disable", isDisable, "");
    if (ret != 0 || isDisable.empty()) {
        MEDIA_LOGI("sys.media.memory.recycle.disable ret:%{public}d", ret);
        return;
    }
    MEDIA_LOGI("sys.media.memory.recycle.disable = %{public}s", isDisable.c_str());

    if (isDisable == "true") {
        isCloseMemoryRecycle_ = true;
    } else if (isDisable == "false") {
        isCloseMemoryRecycle_ = false;
    }
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
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
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t PlayerServerTask::Init()
{
    MEDIA_LOGI("PlayerServerTask Init");
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
    MEDIA_LOGI("PlayerServerTask SetSourceInternal");
    switch (recoverConfig_.sourceType) {
        case static_cast<int32_t>(PlayerSourceType::SOURCE_TYPE_URL):
            ret = playerServer_->SetSource(recoverConfig_.url);
            break;

        case static_cast<int32_t>(PlayerSourceType::SOURCE_TYPE_DATASRC):
            ret = playerServer_->SetSource(recoverConfig_.dataSrc);
            break;

        case static_cast<int32_t>(PlayerSourceType::SOURCE_TYPE_FD):
            ret = playerServer_->SetSource(recoverConfig_.fd, recoverConfig_.offset, recoverConfig_.size);
            break;
        
        default:
            ret = MSERR_INVALID_OPERATION;
            break;
    }
    if (ret == MSERR_OK) {
        ChangeState(initializedState_);
    }
    return ret;
}

int32_t PlayerServerTask::SetConfigInternal()
{
    int32_t ret = MSERR_OK;
    MEDIA_LOGI("PlayerServerTask SetConfigInternal");
    if (recoverConfig_.leftVolume != 1.0f || recoverConfig_.rightVolume != 1.0f) {
        ret = playerServer_->SetVolume(recoverConfig_.leftVolume, recoverConfig_.rightVolume);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SetVolume");
    }

    if (recoverConfig_.surface != nullptr) {
        ret = playerServer_->SetVideoSurface(recoverConfig_.surface);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SetVideoSurface");
    }

    if (recoverConfig_.loop != false) {
        ret = playerServer_->SetLooping(recoverConfig_.loop);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SetLooping");
    }

    if (recoverConfig_.isHasSetParam == true) {
        ret = playerServer_->SetParameter(recoverConfig_.param);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SetParameter");
    }

    if (recoverConfig_.bitRate != 0) {
        ret = playerServer_->SelectBitRate(recoverConfig_.bitRate);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SelectBitRate");
    }
    return ret;
}

int32_t PlayerServerTask::SetBehaviorInternal()
{
    int ret = MSERR_OK;
    MEDIA_LOGI("PlayerServerTask SetBehaviorInternal");
    if (recoverConfig_.speedMode != SPEED_FORWARD_1_00_X) {
        ret = playerServer_->SetPlaybackSpeed(recoverConfig_.speedMode);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SetPlaybackSpeed");
    }

    if (recoverConfig_.currentTime != 0) {
        ret = playerServer_->Seek(recoverConfig_.currentTime, SEEK_CLOSEST);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to Seek");
    }

    return ret;
}

int32_t PlayerServerTask::SetSource(const std::string &url)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call SetSource");
        if (RecoverMemByUser() != MSERR_OK) {
            MEDIA_LOGE("RecoverMemByUser fail");
            return MSERR_INVALID_OPERATION;
        }
    }
    MEDIA_LOGI("SetSource enter url:%{public}s", url.c_str());
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->SetSource(url);
        if (ret == MSERR_OK) {
            recoverConfig_.url = url;
            recoverConfig_.sourceType = static_cast<int32_t>(PlayerSourceType::SOURCE_TYPE_URL);
            ChangeState(initializedState_);
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to SetSource url");
    return result.Value();
}

int32_t PlayerServerTask::SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call SetSource");
        if (RecoverMemByUser() != MSERR_OK) {
            MEDIA_LOGE("RecoverMemByUser fail");
            return MSERR_INVALID_OPERATION;
        }
    }
    MEDIA_LOGI("SetSource enter datasrc");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->SetSource(dataSrc);
        if (ret == MSERR_OK) {
            recoverConfig_.dataSrc = dataSrc;
            recoverConfig_.sourceType = static_cast<int32_t>(PlayerSourceType::SOURCE_TYPE_DATASRC);
            ChangeState(initializedState_);
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to SetSource dataSrc");
    return result.Value();
}

int32_t PlayerServerTask::SetSource(int32_t fd, int64_t offset, int64_t size)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call SetSource");
        if (RecoverMemByUser() != MSERR_OK) {
            MEDIA_LOGE("RecoverMemByUser fail");
            return MSERR_INVALID_OPERATION;
        }
    }
    MEDIA_LOGI("SetSource enter fd:%{public}d, offset:%{public}ld, size:%{public}ld", fd, offset, size);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->SetSource(fd, offset, size);
        if (ret == MSERR_OK) {
            recoverConfig_.fd = fd;
            recoverConfig_.offset = offset;
            recoverConfig_.size = size;
            recoverConfig_.sourceType = static_cast<int32_t>(PlayerSourceType::SOURCE_TYPE_FD);
            ChangeState(initializedState_);
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to SetSource dataSrc");
    return result.Value();
}

int32_t PlayerServerTask::Play()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call Play");
        if (RecoverMemByUser() != MSERR_OK) {
            MEDIA_LOGE("RecoverMemByUser fail");
            return MSERR_INVALID_OPERATION;
        }
    }
    MEDIA_LOGI("Play enter");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->Play();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to Play");
    return result.Value();
}

int32_t PlayerServerTask::Prepare()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call Prepare");
        if (RecoverMemByUser() != MSERR_OK) {
            MEDIA_LOGE("RecoverMemByUser fail");
            return MSERR_INVALID_OPERATION;
        }
    }
    MEDIA_LOGI("Prepare enter");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->Prepare();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to Prepare");
    return result.Value();
}

int32_t PlayerServerTask::PrepareAsync()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call PrepareAsync");
        if (RecoverMemByUser() != MSERR_OK) {
            MEDIA_LOGE("RecoverMemByUser fail");
            return MSERR_INVALID_OPERATION;
        }
    }
    MEDIA_LOGI("PrepareAsync enter");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->PrepareAsync();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to PrepareAsync");
    return result.Value();
}

int32_t PlayerServerTask::Pause()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call Pause");
        if (RecoverMemByUser() != MSERR_OK) {
            MEDIA_LOGE("RecoverMemByUser fail");
            return MSERR_INVALID_OPERATION;
        }
    }
    MEDIA_LOGI("Pause enter");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->Pause();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to Pause");
    return result.Value();
}

int32_t PlayerServerTask::Stop()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call Stop");
        if (RecoverMemByUser() != MSERR_OK) {
            MEDIA_LOGE("RecoverMemByUser fail");
            return MSERR_INVALID_OPERATION;
        }
    }
    MEDIA_LOGI("Stop enter");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->Stop();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to Stop");
    return result.Value();
}

int32_t PlayerServerTask::Reset()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call Reset");
        if (RecoverMemByUser() != MSERR_OK) {
            MEDIA_LOGE("RecoverMemByUser fail");
            return MSERR_INVALID_OPERATION;
        }
    }
    MEDIA_LOGI("Reset enter");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->Reset();
        if (ret == MSERR_OK) {
            ChangeState(idleState_);
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to Reset");
    return result.Value();
}

int32_t PlayerServerTask::Release()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call Release");
        SetReleaseMemByManage(false);
    }
    MEDIA_LOGI("Release enter");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->Release();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to Release");
    return result.Value();
}

int32_t PlayerServerTask::ReleaseSync()
{
    return MSERR_OK;
}

int32_t PlayerServerTask::SetVolume(float leftVolume, float rightVolume)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call SetVolume");
        if (RecoverMemByUser() != MSERR_OK) {
            MEDIA_LOGE("RecoverMemByUser fail");
            return MSERR_INVALID_OPERATION;
        }
    }
    MEDIA_LOGI("SetVolume enter leftVolume:%{public}f rightVolume:%{public}f", leftVolume, rightVolume);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->SetVolume(leftVolume, rightVolume);
        if (ret == MSERR_OK) {
            recoverConfig_.leftVolume = leftVolume;
            recoverConfig_.rightVolume = rightVolume;
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to SetVolume");
    return result.Value();
}

int32_t PlayerServerTask::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call Seek, going to recover");
        if (RecoverMemByUser() != MSERR_OK) {
            MEDIA_LOGE("RecoverMemByUser fail");
            return MSERR_INVALID_OPERATION;
        }
    }
    MEDIA_LOGI("Seek enter mSeconds:%{public}d mode:%{public}d", mSeconds, mode);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->Seek(mSeconds, mode);
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to Seek");
    return result.Value();
}

int32_t PlayerServerTask::GetCurrentTime(int32_t &currentTime)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call GetCurrentTime:%{public}d", recoverConfig_.currentTime);
        currentTime = recoverConfig_.currentTime;
        return MSERR_OK;
    }
    MEDIA_LOGI("GetCurrentTime enter");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->GetCurrentTime(currentTime);
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to GetCurrentTime");
    return result.Value();
}

int32_t PlayerServerTask::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call GetVideoTrackInfo");
        videoTrack = recoverConfig_.videoTrack;
        return MSERR_OK;
    }

    MEDIA_LOGI("GetVideoTrackInfo enter");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->GetVideoTrackInfo(videoTrack);
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to GetVideoTrackInfo");
    return result.Value();
}

int32_t PlayerServerTask::GetAudioTrackInfo(std::vector<Format> &audioTrack)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call GetAudioTrackInfo");
        audioTrack = recoverConfig_.audioTrack;
        return MSERR_OK;
    }

    MEDIA_LOGI("GetAudioTrackInfo enter");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->GetAudioTrackInfo(audioTrack);
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to GetAudioTrackInfo");
    return result.Value();
}

int32_t PlayerServerTask::GetVideoWidth()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call GetVideoWidth:%{public}d", recoverConfig_.videoWidth);
        return recoverConfig_.videoWidth;
    }

    MEDIA_LOGI("GetVideoWidth enter");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->GetVideoWidth();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to GetVideoWidth");
    return result.Value();
}

int32_t PlayerServerTask::GetVideoHeight()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call GetVideoHeight:%{public}d", recoverConfig_.videoHeight);
        return recoverConfig_.videoHeight;
    }
    MEDIA_LOGI("GetVideoHeight enter");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->GetVideoHeight();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to GetVideoHeight");
    return result.Value();
}

int32_t PlayerServerTask::GetDuration(int32_t &duration)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call GetDuration:%{public}d", recoverConfig_.duration);
        duration = recoverConfig_.duration;
        return MSERR_OK;
    }
    MEDIA_LOGI("GetDuration enter");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->GetDuration(duration);
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to GetDuration");
    return result.Value();
}

int32_t PlayerServerTask::SetPlaybackSpeed(PlaybackRateMode mode)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call SetPlaybackSpeed");
        if (RecoverMemByUser() != MSERR_OK) {
            MEDIA_LOGE("RecoverMemByUser fail");
            return MSERR_INVALID_OPERATION;
        }
    }
    MEDIA_LOGI("SetPlaybackSpeed enter mode:%{public}d", mode);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->SetPlaybackSpeed(mode);
        if (ret == MSERR_OK) {
            recoverConfig_.speedMode = mode;
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to SetPlaybackSpeed");
    return result.Value();
}

int32_t PlayerServerTask::GetPlaybackSpeed(PlaybackRateMode &mode)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call GetPlaybackSpeed:%{public}d", recoverConfig_.speedMode);
        mode = recoverConfig_.speedMode;
        return MSERR_OK;
    }
    MEDIA_LOGI("GetPlaybackSpeed enter");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->GetPlaybackSpeed(mode);
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to GetPlaybackSpeed");
    return result.Value();
}

#ifdef SUPPORT_VIDEO
int32_t PlayerServerTask::SetVideoSurface(sptr<Surface> surface)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    isAudioPlayer_ = false;
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call SetVideoSurface");
        if (RecoverMemByUser() != MSERR_OK) {
            MEDIA_LOGE("RecoverMemByUser fail");
            return MSERR_INVALID_OPERATION;
        }
    }
    MEDIA_LOGI("SetVidoSurface enter");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->SetVideoSurface(surface);
        if (ret == MSERR_OK) {
            recoverConfig_.surface = surface;
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to SetVideoSurface");
    return result.Value();
}
#endif

bool PlayerServerTask::IsPlaying()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call IsPlaying:%{public}d", recoverConfig_.isPlaying);
        return recoverConfig_.isPlaying;
    }
    MEDIA_LOGI("IsPlaying enter");
    auto task = std::make_shared<TaskHandler<bool>>([&, this] {
        auto ret = playerServer_->IsPlaying();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to IsPlaying");
    return result.Value();
}

bool PlayerServerTask::IsLooping()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call IsLooping:%{public}d", recoverConfig_.loop);
        return recoverConfig_.loop;
    }
    MEDIA_LOGI("IsLooping enter");
    auto task = std::make_shared<TaskHandler<bool>>([&, this] {
        auto ret = playerServer_->IsLooping();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to IsLooping");
    return result.Value();
}

int32_t PlayerServerTask::SetLooping(bool loop)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call SetLooping");
        if (RecoverMemByUser() != MSERR_OK) {
            MEDIA_LOGE("RecoverMemByUser fail");
            return MSERR_INVALID_OPERATION;
        }
    }
    MEDIA_LOGI("SetLooping enter loop:%{public}d", loop);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->SetLooping(loop);
        if (ret == MSERR_OK) {
            recoverConfig_.loop = loop;
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to SetLooping");
    return result.Value();
}

int32_t PlayerServerTask::SetParameter(const Format &param)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call SetParameter");
        if (RecoverMemByUser() != MSERR_OK) {
            MEDIA_LOGE("RecoverMemByUser fail");
            return MSERR_INVALID_OPERATION;
        }
    }
    MEDIA_LOGI("SetParameter enter");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->SetParameter(param);
        if (ret == MSERR_OK) {
            recoverConfig_.isHasSetParam = true;
            recoverConfig_.param = param;
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to SetParameter");
    return result.Value();
}

int32_t PlayerServerTask::SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call SetPlayerCallback");
        if (RecoverMemByUser() != MSERR_OK) {
            MEDIA_LOGE("RecoverMemByUser fail");
            return MSERR_INVALID_OPERATION;
        }
    }
    MEDIA_LOGI("SetPlayerCallback enter");
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->SetPlayerCallback(callback);
        if (ret == MSERR_OK) {
            recoverConfig_.callback = callback;
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to SetPlayerCallback");
    return result.Value();
}

int32_t PlayerServerTask::SelectBitRate(uint32_t bitRate)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (GetReleaseMemByManage()) {
        MEDIA_LOGI("User call SelectBitRate");
        if (RecoverMemByUser() != MSERR_OK) {
            MEDIA_LOGE("RecoverMemByUser fail");
            return MSERR_INVALID_OPERATION;
        }
    }
    MEDIA_LOGI("SelectBitRate enter bitRate:%{public}u", bitRate);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->SelectBitRate(bitRate);
        if (ret == MSERR_OK) {
            recoverConfig_.bitRate = bitRate;
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to SelectBitRate");
    return result.Value();
}

void PlayerServerTask::OnError(PlayerErrorType errorType, int32_t errorCode)
{
    (void)errorType;
    (void)errorCode;
}

void PlayerServerTask::OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    MEDIA_LOGI("OnInfo enter");
    (void)HandleMessage(type, extra, infoBody);
}

int32_t PlayerServerTask::DumpInfo(int32_t fd)
{
    MEDIA_LOGI("DumpInfo enter");
    auto ret = std::static_pointer_cast<PlayerServer>(playerServer_)->DumpInfo(fd);

    std::string dumpString;
    std::string isReleaseMemByManage = "false";
    std::string isRecoverMemByUser = "false";
    if (GetReleaseMemByManage()) {
        isReleaseMemByManage = "true";
    }
    if (GetRecoverMemByUser()) {
        isRecoverMemByUser = "true";
    }
    dumpString += "PlayerServerTask IsReleaseMemByManage: " +
        isReleaseMemByManage + ", IsRecoverMemByUser: " + isRecoverMemByUser + "\n";
    write(fd, dumpString.c_str(), dumpString.size());
    return ret;
}

int32_t PlayerServerTask::ReleaseMemByManage()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    MEDIA_LOGI("Enter ReleaseMemByManage");
    if (isCloseMemoryRecycle_) {
        MEDIA_LOGI("isCloseMemoryRecycle true");
        return MSERR_OK;
    }
    if (GetReleaseMemByManage() || GetRecoverMemByUser()) {
        MEDIA_LOGE("ReleaseMemByManage %{public}d, RecoverMemByUser %{public}d",
            GetReleaseMemByManage(), GetRecoverMemByUser());
        return MSERR_OK;
    }

    recoverConfig_.curState = GetCurrState();
    if (recoverConfig_.curState == idleState_ || recoverConfig_.curState == playingState_) {
        MEDIA_LOGI("CurState is Idle or Playing");
        return MSERR_OK;
    }
    SetReleaseMemByManage(true);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = playerServer_->GetCurrentTime(recoverConfig_.currentTime);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "failed to GetCurrentTime");
        if (recoverConfig_.curState != initializedState_) {
            ret = playerServer_->GetVideoTrackInfo(recoverConfig_.videoTrack);
            CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "failed to GetVideoTrack");
            ret = playerServer_->GetAudioTrackInfo(recoverConfig_.audioTrack);
            CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "failed to GetAudioTrack");
            recoverConfig_.videoWidth = playerServer_->GetVideoWidth();
            recoverConfig_.videoHeight = playerServer_->GetVideoHeight();
            ret = playerServer_->GetDuration(recoverConfig_.duration);
            CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "failed to GetDuration");
            recoverConfig_.isPlaying = playerServer_->IsPlaying();
        }
        std::static_pointer_cast<PlayerServer>(playerServer_)->SetPlayerCallbackInner(nullptr);
        ret = playerServer_->Reset();
        if (ret == MSERR_OK) {
            ChangeState(idleState_);
        }
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to Reset");
    if (result.Value() != MSERR_OK) {
        SetReleaseMemByManage(false);
        MEDIA_LOGE("ReleaseMemByManage fail");
    } else {
        MEDIA_LOGI("ReleaseMemByManage success");
    }

    return result.Value();
}

int32_t PlayerServerTask::RecoverMemByUser()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    MEDIA_LOGI("Enter RecoverMemByUser");
    if (isCloseMemoryRecycle_) {
        MEDIA_LOGI("isCloseMemoryRecycle true");
        return MSERR_OK;
    }
    if (!GetReleaseMemByManage() || GetRecoverMemByUser()) {
        MEDIA_LOGE("ReleaseMemByManage %{public}d, RecoverMemByUser %{public}d",
            GetReleaseMemByManage(), GetRecoverMemByUser());
        return MSERR_OK;
    }

    SetReleaseMemByManage(false);
    SetRecoverMemByUser(true);
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto ret = std::static_pointer_cast<PlayerServerTask::BaseState>(recoverConfig_.curState)->StateRecover();
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    auto result = task->GetResult();
    CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION, "failed to Reset");
    if (result.Value() != MSERR_OK) {
        SetReleaseMemByManage(true);
        SetRecoverMemByUser(false);
        MEDIA_LOGE("RecoverMemByUser fail");
    } else {
        MEDIA_LOGI("RecoverMemByUser success");
    }

    return result.Value();
}

bool PlayerServerTask::IsAudioPlayer()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    return isAudioPlayer_;
}

void PlayerServerTask::ContinuousNotPlayingCntAdd()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    continuousNotPlayingCnt_++;
}

void PlayerServerTask::SetContinuousNotPlayingCntZero()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    continuousNotPlayingCnt_ = 0;
}

int32_t PlayerServerTask::GetContinuousNotPlayingCnt()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    return continuousNotPlayingCnt_;
}

bool PlayerServerTask::GetReleaseMemByManage()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    return recoverConfig_.isReleaseMemByManage;
}

void PlayerServerTask::SetReleaseMemByManage(bool value)
{
    recoverConfig_.isReleaseMemByManage = value;
}

bool PlayerServerTask::GetRecoverMemByUser()
{
    return recoverConfig_.isRecoverMemByUser;
}

void PlayerServerTask::SetRecoverMemByUser(bool value)
{
    recoverConfig_.isRecoverMemByUser = value;
}

int32_t PlayerServerTask::RecoverPlayerNormal()
{
    MEDIA_LOGI("PlayerServerTask RecoverPlayerNormal");
    if (recoverConfig_.callback != nullptr) {
        std::static_pointer_cast<PlayerServer>(playerServer_)->SetPlayerCallbackInner(recoverConfig_.callback);
    }
    SetRecoverMemByUser(false);
    return MSERR_OK;
}

void PlayerServerTask::CheckHasRecover(PlayerOnInfoType type, int32_t extra)
{
    MEDIA_LOGI("CheckHasRecover enter, type:%{public}d, extra:%{public}d, isRecoverMemByUser:%{public}d",
        type, extra, GetRecoverMemByUser());
    if (!GetRecoverMemByUser()) {
        return;
    }

    if (recoverConfig_.curState == initializedState_ || recoverConfig_.curState == stoppedState_) {
        if (type == INFO_TYPE_STATE_CHANGE && extra == PLAYER_INITIALIZED) {
            (void)RecoverPlayerNormal();
        }
        return;
    }

    if (recoverConfig_.curState == preparedState_ || recoverConfig_.curState == pausedState_) {
        if (recoverConfig_.currentTime != 0) {
            if (type == INFO_TYPE_SEEKDONE) {
                (void)RecoverPlayerNormal();
            }
        } else if (recoverConfig_.speedMode != SPEED_FORWARD_1_00_X) {
            if (type == INFO_TYPE_SPEEDDONE) {
                (void)RecoverPlayerNormal();
            }
        } else if (type == INFO_TYPE_STATE_CHANGE && extra == PLAYER_PREPARED) {
            (void)RecoverPlayerNormal();
        }
        return;
    }

    if (recoverConfig_.curState == playbackCompletedState_) {
        if (recoverConfig_.speedMode != SPEED_FORWARD_1_00_X) {
            if (type == INFO_TYPE_SPEEDDONE) {
                (void)RecoverPlayerNormal();
            }
        } else if (type == INFO_TYPE_STATE_CHANGE && extra == PLAYER_PREPARED) {
            (void)RecoverPlayerNormal();
        }
        return;
    }
}
}
}