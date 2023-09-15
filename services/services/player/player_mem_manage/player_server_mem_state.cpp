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

#include "player_server_mem_state.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerServerMemState"};
}

namespace OHOS {
namespace Media {
std::string PlayerServerMem::MemBaseState::GetStateName() const
{
    return name_;
}

int32_t PlayerServerMem::MemBaseState::MemStateRecover()
{
    MEDIA_LOGE("invalid operation for %{public}s", GetStateName().c_str());
    return MSERR_INVALID_STATE;
}

int32_t PlayerServerMem::MemBaseState::MemStateRelease()
{
    return MSERR_INVALID_STATE;
}

int32_t PlayerServerMem::MemBaseState::MemPlayerCbRecover(PlayerOnInfoType type, int32_t extra)
{
    (void)type;
    (void)extra;
    MEDIA_LOGE("invalid operation for %{public}s", GetStateName().c_str());
    return MSERR_INVALID_STATE;
}

int32_t PlayerServerMem::MemBaseState::MemRecoverToInitialized()
{
    playerServerMem_.SetPlayerServerConfig();
    auto ret = playerServerMem_.SetSourceInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION,
        "curState:%{public}s failed to SetSource url", GetStateName().c_str());
    ret = playerServerMem_.SetConfigInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION,
        "curState:%{public}s failed to SetConfigInternal", GetStateName().c_str());
    
    return MSERR_OK;
}

int32_t PlayerServerMem::MemBaseState::MemRecoverToPrepared()
{
    playerServerMem_.SetPlayerServerConfig();
    auto res = playerServerMem_.SetSourceInternal();
    CHECK_AND_RETURN_RET_LOG(res == MSERR_OK, MSERR_INVALID_OPERATION,
        "curState:%{public}s failed to SetSource url.", GetStateName().c_str());
    res = playerServerMem_.SetConfigInternal();
    CHECK_AND_RETURN_RET_LOG(res == MSERR_OK, MSERR_INVALID_OPERATION,
        "curState:%{public}s failed to SetConfigInternal.", GetStateName().c_str());
    res = playerServerMem_.PrepareAsyncInner();
    CHECK_AND_RETURN_RET_LOG(res == MSERR_OK, MSERR_INVALID_OPERATION,
        "curState:%{public}s failed to PrepareAsyncInner.", GetStateName().c_str());
    res = playerServerMem_.SetBehaviorInternal();
    CHECK_AND_RETURN_RET_LOG(res == MSERR_OK, MSERR_INVALID_OPERATION,
        "curState:%{public}s failed to SetBehaviorInternal.", GetStateName().c_str());

    return MSERR_OK;
}

int32_t PlayerServerMem::MemBaseState::MemRecoverToCompleted()
{
    playerServerMem_.SetPlayerServerConfig();
    auto ret = playerServerMem_.SetSourceInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION,
        "curState:%{public}s failed to SetSource url", GetStateName().c_str());
    ret = playerServerMem_.SetConfigInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION,
        "curState:%{public}s failed to SetConfigInternal", GetStateName().c_str());
    ret = playerServerMem_.PrepareAsyncInner();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION,
        "curState:%{public}s failed to PrepareAsyncInner", GetStateName().c_str());
    ret = playerServerMem_.SetPlaybackSpeedInternal();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION,
        "curState:%{public}s failed to SetPlaybackSpeedInternal", GetStateName().c_str());
    
    return MSERR_OK;
}

int32_t PlayerServerMem::MemInitializedState::MemStateRecover()
{
    return MemRecoverToInitialized();
}

int32_t PlayerServerMem::MemInitializedState::MemStateRelease()
{
    playerServerMem_.GetPlayerServerConfig();
    return MSERR_OK;
}

int32_t PlayerServerMem::MemInitializedState::MemPlayerCbRecover(PlayerOnInfoType type, int32_t extra)
{
    playerServerMem_.RecoverToInitialized(type, extra);
    return MSERR_OK;
}

int32_t PlayerServerMem::MemPreparingState::MemStateRecover()
{
    return MemRecoverToPrepared();
}

int32_t PlayerServerMem::MemPreparingState::MemStateRelease()
{
    return playerServerMem_.GetInformationBeforeMemReset();
}

int32_t PlayerServerMem::MemPreparingState::MemPlayerCbRecover(PlayerOnInfoType type, int32_t extra)
{
    playerServerMem_.RecoverToPrepared(type, extra);
    return MSERR_OK;
}

int32_t PlayerServerMem::MemPreparedState::MemStateRecover()
{
    return MemRecoverToPrepared();
}

int32_t PlayerServerMem::MemPreparedState::MemStateRelease()
{
    return playerServerMem_.GetInformationBeforeMemReset();
}

int32_t PlayerServerMem::MemPreparedState::MemPlayerCbRecover(PlayerOnInfoType type, int32_t extra)
{
    playerServerMem_.RecoverToPrepared(type, extra);
    return MSERR_OK;
}

int32_t PlayerServerMem::MemPausedState::MemStateRecover()
{
    return MemRecoverToPrepared();
}

int32_t PlayerServerMem::MemPausedState::MemStateRelease()
{
    return playerServerMem_.GetInformationBeforeMemReset();
}

int32_t PlayerServerMem::MemPausedState::MemPlayerCbRecover(PlayerOnInfoType type, int32_t extra)
{
    playerServerMem_.RecoverToPrepared(type, extra);
    return MSERR_OK;
}

int32_t PlayerServerMem::MemPlaybackCompletedState::MemStateRecover()
{
    return MemRecoverToCompleted();
}

int32_t PlayerServerMem::MemPlaybackCompletedState::MemStateRelease()
{
    return playerServerMem_.GetInformationBeforeMemReset();
}

int32_t PlayerServerMem::MemPlaybackCompletedState::MemPlayerCbRecover(PlayerOnInfoType type, int32_t extra)
{
    playerServerMem_.RecoverToCompleted(type, extra);
    return MSERR_OK;
}
}
}