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

#include "app_state_listener.h"
#include "player_mem_manage.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AppStateListener"};
}

namespace OHOS {
namespace Media {
AppStateListener::AppStateListener()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AppStateListener::~AppStateListener()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void AppStateListener::OnConnected()
{
    MEDIA_LOGI("enter");
    PlayerMemManage::GetInstance().HandleOnConnected();
}

void AppStateListener::OnDisconnected()
{
    MEDIA_LOGI("enter");
    PlayerMemManage::GetInstance().HandleOnDisconnected();
}

void AppStateListener::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    (void)object;
    MEDIA_LOGI("enter");
    PlayerMemManage::GetInstance().HandleOnRemoteDied(object);
}

void AppStateListener::ForceReclaim(int32_t pid, int32_t uid)
{
    MEDIA_LOGI("enter");
    PlayerMemManage::GetInstance().HandleForceReclaim(uid, pid);
}

void AppStateListener::OnTrim(Memory::SystemMemoryLevel level)
{
    MEDIA_LOGI("enter level:%{public}d", level);
    PlayerMemManage::GetInstance().HandleOnTrim(level);
}

void AppStateListener::OnAppStateChanged(int32_t pid, int32_t uid, int32_t state)
{
    MEDIA_LOGI("enter pid:%{public}d, uid:%{public}d, state:%{public}d", pid, uid, state);
    PlayerMemManage::GetInstance().RecordAppState(uid, pid, state);
}
}
}
