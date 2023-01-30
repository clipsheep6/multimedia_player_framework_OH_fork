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

#include "player_service_stub_mem.h"
#include "player_mem_manage.h"
#include "ipc_skeleton.h"
#include "media_log.h"
#include "media_errors.h"
#include "media_dfx.h"
#include "mem_mgr_client.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerServiceStubMem"};
}

namespace OHOS {
namespace Media {
constexpr int32_t PER_INSTANCE_NEED_MEMORY_PERCENT = 10;
constexpr int32_t ONE_HUNDRED = 100;
sptr<PlayerServiceStub> PlayerServiceStubMem::Create()
{
    int32_t availableMemory = Memory::MemMgrClient::GetInstance().GetAvailableMemory();
    int32_t totalMemory = Memory::MemMgrClient::GetInstance().GetTotalMemory();
    if (availableMemory != -1 && totalMemory != -1 &&
        availableMemory <= totalMemory / ONE_HUNDRED * PER_INSTANCE_NEED_MEMORY_PERCENT) {
        MEDIA_LOGE("System available memory:%{public}d insufficient, total memory:%{public}d",
            availableMemory, totalMemory);
        return nullptr;
    }

    sptr<PlayerServiceStub> playerStubMem = new(std::nothrow) PlayerServiceStubMem();
    CHECK_AND_RETURN_RET_LOG(playerStubMem != nullptr, nullptr, "failed to new PlayerServiceStubMem");

    int32_t ret = playerStubMem->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to player stubMem init");
    return playerStubMem;
}

PlayerServiceStubMem::PlayerServiceStubMem()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerServiceStubMem::~PlayerServiceStubMem()
{
    if (playerServer_ != nullptr) {
        auto task = std::make_shared<TaskHandler<void>>([&, this] {
            auto memManageRecall = std::bind(&PlayerServiceStubMem::MemManageRecall, this,
                std::placeholders::_1, std::placeholders::_2);
            PlayerMemManage::GetInstance().DeregisterPlayerServer(memManageRecall);
            int32_t id = PlayerXCollie::GetInstance().SetTimer("PlayerServiceStub::~PlayerServiceStub");
            (void)playerServer_->Release();
            PlayerXCollie::GetInstance().CancelTimer(id);
            playerServer_ = nullptr;
        });
        (void)taskQue_.EnqueueTask(task);
        (void)task->GetResult();
    }
}

int32_t PlayerServiceStubMem::init()
{
    if (playerServer_ == nullptr) {
        playerServer_ = PlayerServerMem::Create();
        int32_t appUid = IPCSkeleton::GetCallingUid();
        int32_t appPid = IPCSkeleton::GetCallingPid();
        auto memManageRecall = std::bind(&PlayerServiceStubMem::MemManageRecall, this,
            std::placeholders::_1, std::placeholders::_2);
        PlayerMemManage::GetInstance().RegisterPlayerServer(appUid, appPid, memManageRecall);
    }
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "failed to create PlayerServer");

    SetPlayerFuncs();
    return MSERR_OK;
}

int32_t PlayerServiceStubMem::DestroyStub()
{
    MediaTrace trace("binder::DestroyStub");
    playerCallback_ = nullptr;
    if (playerServer_ != nullptr) {
        auto memManageRecall = std::bind(&PlayerServiceStubMem::MemManageRecall, this,
            std::placeholders::_1, std::placeholders::_2);
        PlayerMemManage::GetInstance().DeregisterPlayerServer(memManageRecall);
        (void)playerServer_->Release();
        playerServer_ = nullptr;
    }

    MediaServerManager::GetInstance().DestroyStubObject(MediaServerManager::PLAYER, AsObject());
    return MSERR_OK;
}

int32_t PlayerServiceStubMem::Release()
{
    MediaTrace trace("binder::Release");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    auto memManageRecall = std::bind(&PlayerServiceStubMem::MemManageRecall, this,
        std::placeholders::_1, std::placeholders::_2);
    PlayerMemManage::GetInstance().DeregisterPlayerServer(memManageRecall);
    return playerServer_->Release();
}

void PlayerServiceStubMem::MemManageRecall(int32_t recallType, int32_t onTrimLevel)
{
    MEDIA_LOGI("Enter MemManageRecall");
    auto task = std::make_shared<TaskHandler<int>>([&, this] {
        int32_t id = PlayerXCollie::GetInstance().SetTimer("MemManageRecall");
        std::static_pointer_cast<PlayerServerMem>(playerServer_)->ResetForMemManage(recallType, onTrimLevel);
        PlayerXCollie::GetInstance().CancelTimer(id);
        return;
    });
    (void)taskQue_.EnqueueTask(task);
    (void)task->GetResult();
}
} // namespace Media
} // namespace OHOS