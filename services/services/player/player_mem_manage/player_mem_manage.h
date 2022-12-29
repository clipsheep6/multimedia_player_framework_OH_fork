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
#include "i_player_service.h"
#include "app_state_listener.h"
#include "player_server_task.h"

namespace OHOS {
namespace Media {

class PlayerMemManage {
public:
    ~PlayerMemManage();

    static PlayerMemManage& GetInstance();
    int32_t RegisterPlayerServer(int32_t uid, int32_t pid, std::shared_ptr<PlayerServerTask> playeServerTask);
    int32_t DeregisterPlayerServer(std::shared_ptr<PlayerServerTask> playeServerTask);
    int32_t HandleForceReclaim(int32_t uid, int32_t pid);
    int32_t HandleOnTrim(AppExecFwk:Memory level);
    int32_t RecordAppState(int32_t uid, int32_t pid, int32_t state);

private:
    PlayerMemManage();
    bool isParsered_ = false;
    std::mutex mutex_;
    std::shared_ptr<AppStateListener> appStateListener_;
    struct AppPlayerInfo {
        std::vector<std::shared_ptr<PlayerServerTask>> playerServerTaskVec;
        int32_t appState;
        time_t appEnterFrontTime;
        time_t appEnterBackTime;
    };
    using PidPlayersInfo = std::unordered_map<int32_t, AppPlayerInfo>;
    std::unordered_map<int32_t, PidPlayersInfo> playerManage_;

    bool Init();
};
}
}
#endif // PLAYER_MEM_MANAGE_H
