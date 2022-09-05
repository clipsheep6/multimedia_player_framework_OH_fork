/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "freezer_server.h"
#include "media_server_manager.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "FreezerServer"};
}
namespace OHOS {
namespace Media {
FreezerServer::FreezerServer() {}

FreezerServer::~FreezerServer() {}

int32_t FreezerServer::ProxyApp(const std::unordered_set<int32_t>& pidSet, const bool isFreeze)
{
    MEDIA_LOGD("FreezerServer: ProxyApp, pidset size is %{public}u, isFreeze is %{public}d", pidSet.size(), isFreeze);
    std::lock_guard<std::mutex> lock(freezeMutex_);
    for (const auto& pid : pidSet) {
        if (isFreeze && !freezePids.count(pid)) {
            freezePids.emplace(pid);
            std::vector<sptr<IStandardPlayerService>> playerStubVec;
            MediaServerManager::GetInstance().GetPlayerStubByPid(pid, playerStubVec);
            MEDIA_LOGD("playerStubVec size is : %{public}u", playerStubVec.size());
            for (const auto& playerStub : playerStubVec) {
                CHECK_AND_RETURN_RET_LOG(playerStub != nullptr, MSERR_INVALID_VAL, "failed to get playerStub");
                playerStub->SetFreezerState(false);
                playerStub->Pause();
            }
        } else if (!isFreeze && freezePids.count(pid)) {
            freezePids.erase(pid);
            std::vector<sptr<IStandardPlayerService>> playerStubVec;
            MediaServerManager::GetInstance().GetPlayerStubByPid(pid, playerStubVec);
            MEDIA_LOGD("playerStubVec size is : %{public}u", playerStubVec.size());
            for (const auto& playerStub : playerStubVec) {
                CHECK_AND_RETURN_RET_LOG(playerStub != nullptr, MSERR_INVALID_VAL, "failed to get playerStub");
                playerStub->Play();
                playerStub->SetFreezerState(true);
            }
        }
    }
    return MSERR_OK;
}

int32_t FreezerServer::ResetAll()
{
    MEDIA_LOGD("FreezerServer: ResetAll");
    for (const auto& pid : freezePids) {
        MEDIA_LOGD("resume pid is : %{public}d", pid);
        std::vector<sptr<IStandardPlayerService>> playerStubVec;
        MediaServerManager::GetInstance().GetPlayerStubByPid(pid, playerStubVec);
        MEDIA_LOGD("playerStubVec size is : %{public}u", playerStubVec.size());
        for (const auto& playerStub : playerStubVec) {
            CHECK_AND_RETURN_RET_LOG(playerStub != nullptr, MSERR_INVALID_VAL, "failed to get playerStub");
            playerStub->Play();
            playerStub->SetFreezerState(true);
        }
    }
    freezePids.clear();
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
