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

#include "avsession_background.h"
#include <unistd.h>
#include "player.h"
#include "player_server.h"
#include "media_dfx.h"
#include "media_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVSessionBackGround"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<AVsessionBackground> AVsessionBackground::instance_;
std::once_flag AVsessionBackground::onceFlag_;

AVsessionBackground &AVsessionBackground::Instance()
{
    std::call_once(onceFlag_, [] {
        instance_ = std::make_shared<AVsessionBackground>();
    });
    return *instance_;
}

AVsessionBackground::AVsessionBackground()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVsessionBackground::~AVsessionBackground()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void AVsessionBackground::Init()
{
    if (!init_) {
        MEDIA_LOGI("AVSession::AVSessionManager::RegisterSessionListener");
        int32_t ret = AVSession::AVSessionManager::GetInstance().RegisterSessionListener(shared_from_this());
        CHECK_AND_RETURN_LOG(ret == 0, "failed to AVSessionManager::RegisterSessionListener");
        init_ = (ret == 0) ? true : false;
    }
}

void AVsessionBackground::AddListener(std::weak_ptr<IPlayerService> player, int32_t uid)
{
    Init();
    std::lock_guard<std::mutex> lock(mutex_);
    // clean map/list from listener list/map
    for (auto it1 = playerMap_.begin(); it1 != playerMap_.end();) {
        std::list<std::weak_ptr<IPlayerService>> &playerList = it1->second;
        for (auto it2 = playerList.begin(); it2 != playerList.end();) {
            std::weak_ptr<IPlayerService> &temp = *it2;
            if (temp.lock() == nullptr) {
                it2 = playerList.erase(it2);
            } else {
                ++it2;
            }
        }
        if (playerList.empty()) {
            it1 = playerMap_.erase(it1);
        } else {
            ++it1;
        }
    }
    MEDIA_LOGI("DelListener uid num %{public}zu", playerMap_.size());

    // add player to listener list/map
    auto it = playerMap_.find(uid);
    if (it != playerMap_.end()) {
        std::list<std::weak_ptr<IPlayerService>> &playerList = it->second;
        playerList.push_back(player);
        MEDIA_LOGI("AddListener uid %{public}d, player num %{public}zu", uid, playerList.size());
    } else {
        std::list<std::weak_ptr<IPlayerService>> playerList;
        playerList.push_back(player);
        playerMap_[uid] = playerList;
        MEDIA_LOGI("AddListener uid %{public}d, player num %{public}zu", uid, playerList.size());
    }
}

void AVsessionBackground::OnAudioSessionChecked(const int32_t uid)
{
    std::list<std::weak_ptr<IPlayerService>> playerList;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it1 = playerMap_.find(uid);
        if (it1 != playerMap_.end()) {
            playerList = it1->second; // copylist and unlock mutex, Avoid locking the addlist
        }
    }

    MEDIA_LOGW("The application uid %{public}d has not registered avsession", uid);
    for (auto &it2 : playerList) {
        auto player = it2.lock();
        if (player != nullptr && player->IsPlaying()) {
            MediaTrace trace("AVsessionBackground::Pause");
            auto playerServer = std::static_pointer_cast<PlayerServer>(player);
            (void)playerServer->BackGroundChangeState(PlayerStates::PLAYER_PAUSED, true);
            MEDIA_LOGW("The application has been logged back to the background");
        }
    }
}
} // namespace Media
} // namespace OHOS