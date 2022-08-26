/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "freezer_client.h"

#include "system_ability_definition.h"
#include "iservice_registry.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "FreezerClient"};
}

namespace OHOS {
namespace Media {
FreezerClient::FreezerClient() { }

FreezerClient::~FreezerClient() { }

bool FreezerClient::GetFreezerProxy()
{
    if (freezerProxy_) {
        MEDIA_LOGD("freezerProxy_ is already isExist");
        return true;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (!samgr) {
        MEDIA_LOGE("samgr init fail");
        return false;
    }
    sptr<IRemoteObject> object = samgr->GetSystemAbility(OHOS::PLAYER_DISTRIBUTED_SERVICE_ID);
    if (!object) {
        MEDIA_LOGE("object init fail");
        return false;
    }

    freezerProxy_ = iface_cast<IStandardFreezerService>(object);
    if (!freezerProxy_) {
        MEDIA_LOGE("freezerProxy_ init fail");
        return false;
    }
    return true;
}

int32_t FreezerClient::ProxyApp(const std::unordered_set<int32_t>& pidSet, const bool isFreeze)
{
    if (!freezerProxy_ && GetFreezerProxy()) {
        return MSERR_UNKNOWN;
    }
    return freezerProxy_->ProxyApp(pidSet, isFreeze);
}

int32_t FreezerClient::ResetAll()
{
    if (!freezerProxy_ && GetFreezerProxy()) {
        return MSERR_UNKNOWN;
    }
    return freezerProxy_->ResetAll();
}
} // namespace Media
} // namespace OHOS
