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
#include "player_server_mem.h"
#include "ipc_skeleton.h"
#include "media_log.h"
#include "media_errors.h"
#include "media_dfx.h"
#include "media_server_manager.h"
#include "mem_mgr_client.h"
#include "player_xcollie.h"

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
    if (availableMemory > 0 && totalMemory > 0 &&
        availableMemory <= totalMemory / ONE_HUNDRED * PER_INSTANCE_NEED_MEMORY_PERCENT) {
        MEDIA_LOGE("System available memory:%{public}d insufficient, total memory:%{public}d",
            availableMemory, totalMemory);
        return nullptr;
    }

    sptr<PlayerServiceStubMem> playerStubMem = new(std::nothrow) PlayerServiceStubMem();
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
            PlayerMemManage::GetInstance().DeregisterPlayerServer(memRecallStruct_);
            int32_t id = PlayerXCollie::GetInstance().SetTimer("PlayerServiceStubMem::~PlayerServiceStubMem");
            (void)playerServer_->Release();
            PlayerXCollie::GetInstance().CancelTimer(id);
            playerServer_ = nullptr;
        });
        (void)taskQue_.EnqueueTask(task);
        (void)task->GetResult();
    }
}

int32_t PlayerServiceStubMem::Init()
{
    if (playerServer_ == nullptr) {
        playerServer_ = PlayerServerMem::Create();
        int32_t appUid = IPCSkeleton::GetCallingUid();
        int32_t appPid = IPCSkeleton::GetCallingPid();
        memRecallStruct_ = {std::bind(&PlayerServiceStubMem::ResetFrontGroundForMemManageRecall, this),
            std::bind(&PlayerServiceStubMem::ResetBackGroundForMemManageRecall, this),
            std::bind(&PlayerServiceStubMem::ResetMemmgrForMemManageRecall, this),
            std::bind(&PlayerServiceStubMem::RecoverByMemManageRecall, this), &playerServer_};
        PlayerMemManage::GetInstance().RegisterPlayerServer(appUid, appPid, memRecallStruct_);
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
        PlayerMemManage::GetInstance().DeregisterPlayerServer(memRecallStruct_);
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
    PlayerMemManage::GetInstance().DeregisterPlayerServer(memRecallStruct_);
    return playerServer_->Release();
}

void PlayerServiceStubMem::ResetFrontGroundForMemManageRecall()
{
    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        int32_t id = PlayerXCollie::GetInstance().SetTimer("ResetFrontGroundForMemManageRecall");
        std::static_pointer_cast<PlayerServerMem>(playerServer_)->ResetFrontGroundForMemManage();
        PlayerXCollie::GetInstance().CancelTimer(id);
        return;
    });
    (void)taskQue_.EnqueueTask(task);
    (void)task->GetResult();
}

void PlayerServiceStubMem::ResetBackGroundForMemManageRecall()
{
    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        int32_t id = PlayerXCollie::GetInstance().SetTimer("ResetBackGroundForMemManageRecall");
        std::static_pointer_cast<PlayerServerMem>(playerServer_)->ResetBackGroundForMemManage();
        PlayerXCollie::GetInstance().CancelTimer(id);
        return;
    });
    (void)taskQue_.EnqueueTask(task);
    (void)task->GetResult();
}

void PlayerServiceStubMem::ResetMemmgrForMemManageRecall()
{
    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        int32_t id = PlayerXCollie::GetInstance().SetTimer("ResetMemmgrForMemManageRecall");
        std::static_pointer_cast<PlayerServerMem>(playerServer_)->ResetMemmgrForMemManage();
        PlayerXCollie::GetInstance().CancelTimer(id);
        return;
    });
    (void)taskQue_.EnqueueTask(task);
    (void)task->GetResult();
}

void PlayerServiceStubMem::RecoverByMemManageRecall()
{
    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        int32_t id = PlayerXCollie::GetInstance().SetTimer("RecoverByMemManage");
        std::static_pointer_cast<PlayerServerMem>(playerServer_)->RecoverByMemManage();
        PlayerXCollie::GetInstance().CancelTimer(id);
        return;
    });
    (void)taskQue_.EnqueueTask(task);
    (void)task->GetResult();
}
} // namespace Media
} // namespace OHOS