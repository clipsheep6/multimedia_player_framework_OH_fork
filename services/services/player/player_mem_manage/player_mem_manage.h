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
using MemManageRecall = std::function<void()>;
using MemManageRecallPair = std::pair<MemManageRecall, void *>;
class PlayerMemManage {
public:
    virtual ~PlayerMemManage();

    static PlayerMemManage& GetInstance();
    int32_t RegisterPlayerServer(int32_t uid, int32_t pid, MemManageRecallPair memRecallPair);
    int32_t DeregisterPlayerServer(MemManageRecallPair memRecallPair);
    int32_t HandleForceReclaim(int32_t uid, int32_t pid);
    int32_t HandleOnTrim(Memory::SystemMemoryLevel level);
    int32_t RecordAppState(int32_t uid, int32_t pid, int32_t state);
    void HandleOnConnected();
    void HandleOnDisconnected();
    void HandleOnRemoteDied(const wptr<IRemoteObject> &object);

private:
    struct AppPlayerInfo {
        std::vector<MemManageRecallPair> memRecallPairVec;
        int32_t appState;
        bool isReserve;
        std::chrono::steady_clock::time_point appEnterFrontTime;
        std::chrono::steady_clock::time_point appEnterBackTime;
    };
    PlayerMemManage();
    bool Init();
    void ProbeTask();
    void HandleOnTrimLevelLow();
    void FindProbeTaskPlayerFromVec(AppPlayerInfo &appPlayerInfo);
    void FindProbeTaskPlayer();
    void FindDeregisterPlayerFromVec(bool &isFind, AppPlayerInfo &appPlayerInfo, MemManageRecallPair memRecallPair);
    void SetAppPlayerInfo(AppPlayerInfo &appPlayerInfo, int32_t state);
    void SetLastestExitBackGroundApp();
    static bool BackGroundTimeGreaterSort(AppPlayerInfo *a, AppPlayerInfo *b);
    void RemoteDieAgainRegisterActiveApps();

    bool isParsered_ = false;
    std::recursive_mutex recMutex_;
    std::shared_ptr<AppStateListener> appStateListener_;
    bool appStateListenerIsConnected_ = false;
    bool appStateListenerRomoteDied_ = false;
    using PidPlayersInfo = std::unordered_map<int32_t, AppPlayerInfo>;
    std::unordered_map<int32_t, PidPlayersInfo> playerManage_;
    TaskQueue probeTaskQueue_;
    bool existTask_ = false;
};
}
}
#endif // PLAYER_MEM_MANAGE_H
