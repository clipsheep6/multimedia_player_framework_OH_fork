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

#include "player_mem_manage.h"
#include "media_log.h"
#include "media_errors.h"
#include "mem_mgr_client.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerMemManage"};
}

namespace OHOS {
namespace Media {
PlayerMemManage& PlayerMemManage::GetInstance()
{
    static PlayerMemManage instance;
    bool ret = Init();
    if (!ret) {
        MEDIA_LOGE("PlayerMemManage GetInstance Init Failed");
    }
    return instance;
}

PlayerMemManage::PlayerMemManage()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerMemManage::~PlayerMemManage()
{
    Memory::MemMgrClient::GetInstance().UnsubscribeAppState(*appStateListener_);
    playerManage_.clear();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool PlayerMemManage::Init()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (isParsered_) {
        return true;
    }
    MEDIA_LOGD("Create PlayerMemManage");
    playerManage_.clear();
    appStateListener_ = std::make_shared<AppStateListener>();
    CHECK_AND_RETURN_RET_LOG(appStateListener_ != nullptr, nullptr, "failed to new AppStateListener");

    Memory::MemMgrClient::GetInstance().SubscribeAppState(*appStateListener_);
    isParsered_ = true;
}

int32_t PlayerMemManage::RegisterPlayerServer(int32_t uid, int32_t pid, std::shared_ptr<PlayerServerTask> playeServerTask)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (playeServerTask == nullptr) {
        MEDIA_LOGE("input param invalid");
        return MSERR_INVALID_VAL;
    }

    MEDIA_LOGD("Register PlayerServerTask uid:%{public}d pid:%{public}d", uid, pid);
    auto objIter = playerManage_.find(uid);
    if (objIter == playerManage_.end()) {
        MEDIA_LOGD("new user in uid:%{public}d", uid);
        auto ret = playerManage_.emplace(uid, PidPlayersInfo {});
        objIter = ret.first;
    }

    auto &pidPlayersInfo = objIter->second;
    auto pidIter = pidPlayersInfo.find(pid);
    if (pidIter == pidPlayersInfo.end()) {
        MEDIA_LOGD("new app in pid:%{public}d", pid);
        auto ret = pidPlayersInfo.emplace(pid, AppPlayerInfo {std::vector<std::shared_ptr<PlayerServerTask>>(), 0, 0, 0});
        pidIter = ret.first;
        Memory::MemMgrClient::GetInstance().RegisterActiveApps(pid, uid);
    }

    auto &appPlayerInfo = pidIter->second;
    appPlayerInfo.playerServerTaskVec.push_back(playeServerTask);

    return MSERR_OK;
}

int32_t PlayerMemManage::DeregisterPlayerServer(std::shared_ptr<PlayerServerTask> playeServerTask)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (playeServerTask == nullptr) {
        MEDIA_LOGE("input param invalid.");
        return MSERR_INVALID_VAL;
    }

    MEDIA_LOGD("Deregister PlayerServerTask");
    bool isFind = false;
    for (auto &[uid, pidPlayersInfo] : playerManage_) {
        for (auto &[pid, appPlayerInfo] : pidPlayersInfo) {
            for (auto iter = appPlayerInfo.playerServerTaskVec.begin(); iter != appPlayerInfo.playerServerTaskVec.end(); iter++) {
                if (*iter == playeServerTask) {
                    MEDIA_LOGD("Remove PlayerServerTask from vector");
                    isFind = true;
                    appPlayerInfo.playerServerTaskVec.erase(iter);
                }
            }
            if (appPlayerInfo.playerServerTaskVec.size() == 0) {
                MEDIA_LOGD("DeregisterActiveApps pid:%{public}d uid:%{public}d", pid, uid);
                Memory::MemMgrClient::GetInstance().DeregisterActiveApps(pid, uid);
                pidPlayersInfo.erase(pid);
            }
        }
        if (playerManage_.count(uid) == 0) {
            MEDIA_LOGD("remove uid:%{public}d", uid);
            playerManage_.erase(uid);
        }
    }

    if (!isFind) {
        MEDIA_LOGE("not find correct playeServerTask.");
        return MSERR_INVALID_OPERATION;
    }

    return MSERR_OK;
}

int32_t PlayerMemManage::HandleForceReclaim(int32_t uid, int32_t pid)
{
    std::lock_guard<std::mutex> lock(mutex_);

    MEDIA_LOGD("Enter ForceReclaim pid:%{public}d uid:%{public}d", pid, uid);
    for (auto &[uid, pidPlayersInfo] : playerManage_) {
        for (auto &[pid, appPlayerInfo] : pidPlayersInfo) {
            for (auto iter = appPlayerInfo.playerServerTaskVec.begin(); iter != appPlayerInfo.playerServerTaskVec.end(); iter++) {
                auto ret = (*iter)->ReleaseMemByManage();
                if (ret != MSERR_OK) {
                    MEDIA_LOGE("playerServerTask ReleaseMemByManage fail.");
                }
            }
        }
    }
    return MSERR_OK;
}

int32_t PlayerMemManage::HandleOnTrim(AppExecFwk:Memory level)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("Enter OnTrim level:%{public}d", level);
    // todo
    return MSERR_OK;
}

int32_t PlayerMemManage::RecordAppState(int32_t uid, int32_t pid, int32_t state)
{
    std::lock_guard<std::mutex> lock(mutex_);

    MEDIA_LOGD("Enter OnAppStateChanged pid:%{public}d uid:%{public}d state:%{public}d", pid, uid, state);
    for (auto &[findUid, pidPlayersInfo] : playerManage_) {
        if (findUid == uid) {
            for (auto &[findPid, appPlayerInfo] : pidPlayersInfo) {
                if (findPid == pid) {
                    appPlayerInfo.appState = state;
                    // todo
                }
            }
        }
    }

    return MSERR_OK;
}
}
}