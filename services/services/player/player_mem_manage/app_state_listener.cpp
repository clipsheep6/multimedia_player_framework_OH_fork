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
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AppStateListener::~AppStateListener()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

AppStateListener::OnConnected()
{
    MEDIA_LOGD("enter");
}

AppStateListener::OnDisconnecter()
{
    MEDIA_LOGD("enter");
}

AppStateListener::OnRemoteDied()
{
    MEDIA_LOGD("enter");
}

AppStateListener::ForceReclaim(int32_t pid, int32_t uid)
{
    MEDIA_LOGD("enter");
    PlayerMemManage::GetInstance().HandleForceReclaim(uid, pid);
}

AppStateListener::OnTrim(AppExecFwk::MemoryLevel level)
{
    MEDIA_LOGD("enter");
    PlayerMemManage::GetInstance().HandleOnTrim(level);
}

AppStateListener::OnAppStateChanged(int32_t pid, int32_t uid, int32_t state)
{
    MEDIA_LOGD("enter");
    PlayerMemManage::GetInstance().RecordAppState(uid, pid, state);
}
}
}
