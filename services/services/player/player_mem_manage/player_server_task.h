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

#ifndef PLAYER_SERVER_TASK_H
#define PLAYER_SERVER_TASK_H

#include <vector>
#include "i_player_service.h"
#include "player_server.h"

namespace OHOS {
namespace Media {
enum PlayerSourceType : int32_t {
    SOURCE_TYPE_NULL,
    SOURCE_TYPE_URL,
    SOURCE_TYPE_DATASRC,
    SOURCE_TYPE_FD,
};

class PlayerServerTask
    : public IPlayerService,
      public IPlayerEngineObs,
      public NoCopyable,
      public PlayerServerStateMachine {
public:
    static std::shared_ptr<IPlayerService> Create();
    PlayerServerTask();
    ~PlayerServerTask();

    int32_t SetSource(const std::string &url) override;
    int32_t SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc) override;
    int32_t SetSource(int32_t fd, int64_t offset, int64_t size) override;
    int32_t Play() override;
    int32_t Prepare() override;
    int32_t PrepareAsync() override;
    int32_t Pause() override;
    int32_t Stop() override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t SetVolume(float leftVolume, float rightVolume) override;
    int32_t Seek(int32_t mSeconds, PlayerSeekMode mode) override;
    int32_t GetCurrentTime(int32_t &currentTime) override;
    int32_t GetVideoTrackInfo(std::vector<Format> &videoTrack) override;
    int32_t GetAudioTrackInfo(std::vector<Format> &audioTrack) override;
    int32_t GetVideoWidth() override;
    int32_t GetVideoHeight() override;
    int32_t GetDuration(int32_t &duration) override;
    int32_t SetPlaybackSpeed(PlaybackRateMode mode) override;
    int32_t GetPlaybackSpeed(PlaybackRateMode &mode) override;
#ifdef SUPPORT_VIDEO
    int32_t SetVideoSurface(sptr<Surface> surface) override;
#endif
    bool IsPlaying() override;
    bool IsLooping() override;
    int32_t SetLooping(bool loop) override;
    int32_t SetParameter(const Format &param) override;
    int32_t SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback) override;
    int32_t SelectBitRate(uint32_t bitRate) override;

    // IPlayerEngineObs override
    void OnError(PlayerErrorType errorType, int32_t errorCode) override;
    void OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody = {}) override;

    int32_t DumpInfo(int32_t fd);
    int32_t ReleaseMemByManage();
    int32_t RecoveryMemByUser();
    bool IsReleaseMemByManage();

private:
    struct RecoveryConfigInfo {
        bool isReleaseMemByManage = false;
        std::shared_ptr<PlayerServerState> curState = nullptr;
        int32_t sourceType = SOURCE_TYPE_NULL;
        std::string url = nullptr;
        std::shared_ptr<IMediaDataSource> dataSrc = nullptr;
        int32_t fd = 0;
        int64_t offset = 0;
        int64_t size = 0;
        float leftVolume = 1.0f;
        float rightVolume = 1.0f;
        PlaybackRateMode speedMode = SPEED_FORWARD_1_00_X;
        sptr<Surface> surface = nullptr;
        bool loop = false;
        bool isHasSetParam = false;
        Format param;
        std::shared_ptr<PlayerCallback> callback = nullptr;
        uint32_t bitRate = 0;
        int32_t currentTime = 0;
    } recoveryConfig_;
    std::recursive_mutex recMutex_;
    std::shared_ptr<IPlayerService> playerServer_ = nullptr;
    TaskQueue taskQue_;

    int32_t Init();
    int32_t SetSourceInternal();
    int32_t SetConfigInternal();
    int32_t SetBehaviorInternal();
    int32_t RecoverPlayerNormal();

    class BaseState;
    class IdleState;
    class InitializedState;
    class PreparedState;
    class PlayingState;
    class PausedState;
    class StoppedState;
    class PlaybackCompletedState;
    std::shared_ptr<IdleState> idleState_;
    std::shared_ptr<InitializedState> initializedState_;
    std::shared_ptr<PreparedState> preparedState_;
    std::shared_ptr<PlayingState> playingState_;
    std::shared_ptr<PausedState> pausedState_;
    std::shared_ptr<StoppedState> stoppedState_;
    std::shared_ptr<PlaybackCompletedState> playbackCompletedState_;
};

class PlayerServerTask::BaseState : public PlayerServerState {
public:
    BaseState(PlayerServerTask &serverTask, const std::string &name) : PlayerServerState(name), serverTask_(serverTask) {}
    virtual ~BaseState() = default;

    virtual int32_t StateRecover() = 0;

protected:
    int32_t OnMessageReceived(PlayerOnInfoType type, int32_t extra, const Format &infoBody) final;
    virtual void HandleStateChange(int32_t newState)
    {
        (void)newState;
    }
    virtual void HandlePlaybackComplete(int32_t extra)
    {
        (void)extra;
    }
    PlayerServerTask &serverTask_;
};

class PlayerServerTask::IdleState : public PlayerServerTask::BaseState {
public:
    explicit IdleState(PlayerServerTask &serverTask) : BaseState(serverTask, "idle_state") {}
    ~IdleState() = default;

    int32_t StateRecover() override;
};

class PlayerServerTask::InitializedState : public PlayerServerTask::BaseState {
public:
    explicit InitializedState(PlayerServerTask &serverTask) : BaseState(serverTask, "inited_state") {}
    ~InitializedState() = default;

    int32_t StateRecover() override;

protected:
    void HandleStateChange(int32_t newState) override;
};

class PlayerServerTask::PreparedState : public PlayerServerTask::BaseState {
public:
    explicit PreparedState(PlayerServerTask &serverTask) : BaseState(serverTask, "prepared_state") {}
    ~PreparedState() = default;

    int32_t StateRecover() override;

protected:
    void HandleStateChange(int32_t newState) override;
};

class PlayerServerTask::PlayingState : public PlayerServerTask::BaseState {
public:
    explicit PlayingState(PlayerServerTask &serverTask) : BaseState(serverTask, "playing_state") {}
    ~PlayingState() = default;

    int32_t StateRecover() override;

protected:
    void HandleStateChange(int32_t newState) override;
    void HandlePlaybackComplete(int32_t extra) override;
};

class PlayerServerTask::PausedState : public PlayerServerTask::BaseState {
public:
    explicit PausedState(PlayerServerTask &serverTask) : BaseState(serverTask, "paused_state") {}
    ~PausedState() = default;

    int32_t StateRecover() override;

protected:
    void HandleStateChange(int32_t newState) override;
};

class PlayerServerTask::StoppedState : public PlayerServerTask::BaseState {
public:
    explicit StoppedState(PlayerServerTask &serverTask) : BaseState(serverTask, "stopped_state") {}
    ~StoppedState() = default;

    int32_t StateRecover() override;

protected:
    void HandleStateChange(int32_t newState) override;
};

class PlayerServerTask::PlaybackCompletedState : public PlayerServerTask::BaseState {
public:
    explicit PlaybackCompletedState(PlayerServerTask &serverTask) : BaseState(serverTask, "playbackCompleted_state") {}
    ~PlaybackCompletedState() = default;

    int32_t StateRecover() override;

protected:
    void HandleStateChange(int32_t newState) override;
};
}
}
#endif // PLAYER_SERVER_TASK_H
