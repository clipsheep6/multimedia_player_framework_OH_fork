/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
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
#ifndef PLAYER_MEM_MANAGE_H
#define PLAYER_MEM_MANAGE_H

#include <mutex>
#include <vector>
#include <unordered_map>
#include <chrono>
#include "task_queue.h"
#include "app_state_listener.h"

namespace OHOS {
namespace Media {
using ResetForMemManageRecall = std::function<void()>;
using RecoverByMemManageRecall = std::function<void()>;
struct MemManageRecall {
    ResetForMemManageRecall resetFrontGroundRecall;
    ResetForMemManageRecall resetBackGroundRecall;
    ResetForMemManageRecall resetMemmgrRecall;
    RecoverByMemManageRecall recoverRecall;
    void *signAddr;
};
class PlayerMemManage {
public:
    virtual ~PlayerMemManage();

    static PlayerMemManage& GetInstance();
    int32_t RegisterPlayerServer(int32_t uid, int32_t pid, const MemManageRecall &memRecallStruct);
    int32_t DeregisterPlayerServer(const MemManageRecall &memRecallStruct);
    int32_t HandleForceReclaim(int32_t uid, int32_t pid);
    int32_t HandleOnTrim(Memory::SystemMemoryLevel level);
    int32_t RecordAppState(int32_t uid, int32_t pid, int32_t state);
    void HandleOnConnected();
    void HandleOnDisconnected();
    void HandleOnRemoteDied(const wptr<IRemoteObject> &object);

private:
    struct AppPlayerInfo {
        std::vector<MemManageRecall> memRecallStructVec;
        int32_t appState;
        std::chrono::steady_clock::time_point appEnterFrontTime;
        std::chrono::steady_clock::time_point appEnterBackTime;
    };
    PlayerMemManage();
    bool Init();
    void ProbeTask();
    void HandleOnTrimLevelLow();
    void FindBackGroundPlayerFromVec(AppPlayerInfo &appPlayerInfo);
    void FindFrontGroundPlayerFromVec(AppPlayerInfo &appPlayerInfo);
    void FindProbeTaskPlayer();
    void FindDeregisterPlayerFromVec(bool &isFind, AppPlayerInfo &appPlayerInfo,
        const MemManageRecall &memRecallStruct);
    void AwakeFrontGroundAppMedia(AppPlayerInfo &appPlayerInfo);
    void SetAppPlayerInfo(AppPlayerInfo &appPlayerInfo, int32_t state);
    void RemoteDieAgainRegisterActiveApps();
    static void WritePurgeableEvent(int32_t level, int32_t useTime);

    bool isParsed_ = false;
    std::recursive_mutex recMutex_;
    std::recursive_mutex recTaskMutex_;
    std::shared_ptr<AppStateListener> appStateListener_;
    bool isAppStateListenerConnected_ = false;
    bool isAppStateListenerRemoteDied_ = false;
    using PidPlayersInfo = std::unordered_map<int32_t, AppPlayerInfo>;
    std::unordered_map<int32_t, PidPlayersInfo> playerManage_;
    std::unique_ptr<TaskQueue> probeTaskQueue_;
    bool isProbeTaskCreated_ = false;
};
}
}
#endif // PLAYER_MEM_MANAGE_H
