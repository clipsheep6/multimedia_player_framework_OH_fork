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
#ifndef PLAYER_SERVER_MEM_STATE_H
#define PLAYER_SERVER_MEM_STATE_H

#include "player_server_mem.h"

namespace OHOS {
namespace Media {
class PlayerServerMem::MemBaseState {
public:
    MemBaseState(PlayerServerMem &playerServerMem, const std::string &name) :
        playerServerMem_(playerServerMem), name_(name) {}
    virtual ~MemBaseState() = default;
    std::string GetStateName() const;
    virtual int32_t MemStateRecover();
    virtual int32_t MemStateRelease();
    virtual int32_t MemPlayerCbRecover(PlayerOnInfoType type, int32_t extra);

protected:
    int32_t MemRecoverToInitialized();
    int32_t MemRecoverToPrepared();
    int32_t MemRecoverToCompleted();
    PlayerServerMem &playerServerMem_;
    std::string name_;
};

class PlayerServerMem::MemIdleState : public PlayerServerMem::MemBaseState {
public:
    MemIdleState(PlayerServerMem &playerServerMem) : MemBaseState(playerServerMem, "mem_idle_state") {}
    virtual ~MemIdleState() = default;
};

class PlayerServerMem::MemInitializedState : public PlayerServerMem::MemBaseState {
public:
    MemInitializedState(PlayerServerMem &playerServerMem) : MemBaseState(playerServerMem, "mem_initialize_state") {}
    virtual ~MemInitializedState() = default;
    int32_t MemStateRecover() override;
    int32_t MemStateRelease() override;
    int32_t MemPlayerCbRecover(PlayerOnInfoType type, int32_t extra) override;
};

class PlayerServerMem::MemPreparingState : public PlayerServerMem::MemBaseState {
public:
    MemPreparingState(PlayerServerMem &playerServerMem) : MemBaseState(playerServerMem, "mem_preparing_state") {}
    virtual ~MemPreparingState() = default;
    int32_t MemStateRecover() override;
    int32_t MemStateRelease() override;
    int32_t MemPlayerCbRecover(PlayerOnInfoType type, int32_t extra) override;
};

class PlayerServerMem::MemPreparedState : public PlayerServerMem::MemBaseState {
public:
    MemPreparedState(PlayerServerMem &playerServerMem) : MemBaseState(playerServerMem, "mem_prepared_state") {}
    virtual ~MemPreparedState() = default;
    int32_t MemStateRecover() override;
    int32_t MemStateRelease() override;
    int32_t MemPlayerCbRecover(PlayerOnInfoType type, int32_t extra) override;
};

class PlayerServerMem::MemPlayingState : public PlayerServerMem::MemBaseState {
public:
    MemPlayingState(PlayerServerMem &playerServerMem) : MemBaseState(playerServerMem, "mem_playing_state") {}
    virtual ~MemPlayingState() = default;
};

class PlayerServerMem::MemPausedState : public PlayerServerMem::MemBaseState {
public:
    MemPausedState(PlayerServerMem &playerServerMem) : MemBaseState(playerServerMem, "mem_paused_state") {}
    virtual ~MemPausedState() = default;
    int32_t MemStateRecover() override;
    int32_t MemStateRelease() override;
    int32_t MemPlayerCbRecover(PlayerOnInfoType type, int32_t extra) override;
};

class PlayerServerMem::MemStoppedState : public PlayerServerMem::MemBaseState {
public:
    MemStoppedState(PlayerServerMem &playerServerMem) : MemBaseState(playerServerMem, "mem_stopped_state") {}
    virtual ~MemStoppedState() = default;
    int32_t MemStateRecover() override;
    int32_t MemStateRelease() override;
    int32_t MemPlayerCbRecover(PlayerOnInfoType type, int32_t extra) override;
};

class PlayerServerMem::MemPlaybackCompletedState : public PlayerServerMem::MemBaseState {
public:
    MemPlaybackCompletedState(PlayerServerMem &playerServerMem)
        : MemBaseState(playerServerMem, "mem_playbackCompleted_state") {}
    virtual ~MemPlaybackCompletedState() = default;
    int32_t MemStateRecover() override;
    int32_t MemStateRelease() override;
    int32_t MemPlayerCbRecover(PlayerOnInfoType type, int32_t extra) override;
};
}
}
#endif