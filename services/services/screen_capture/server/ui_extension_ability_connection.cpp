/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "ui_extension_ability_connection.h"
#include "ability_connect_callback_interface.h"
#include "ability_manager_client.h"
#include "media_log.h"

constexpr int32_t SIGNAL_NUM = 3;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "UIExtensionAbilityConnection"};
}

namespace OHOS {
namespace Media {

void UIExtensionAbilityConnection::OnAbilityConnectDone(const AppExecFwk::ElementName &element,
    const sptr<IRemoteObject> &remoteObject, int32_t resultCode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(SIGNAL_NUM);
    data.WriteString16(u"bundleName");
    data.WriteString16(u"com.ohos.screenshot");
    data.WriteString16(u"abilityName");
    data.WriteString16(u"com.ohos.screenshot.DialogAbility");
    data.WriteString16(u"parameters");
    data.WriteString16(Str8ToStr16(commandStr_));
    MEDIA_LOGE("UIExtensionAbilityConnection::OnAbilityConnectDone  start ");
    remoteObject->SendRequest(IAbilityConnection::ON_ABILITY_CONNECT_DONE, data, reply, option);
    MEDIA_LOGE("UIExtensionAbilityConnection::OnAbilityConnectDone  end ");
}

void UIExtensionAbilityConnection::OnAbilityDisconnectDone(const AppExecFwk::ElementName &element,
    int32_t resultCode)
{
    MEDIA_LOGE("UIExtensionAbilityConnection::OnAbilityDisconnectDone  start ");
}
} // namespace Media
} // namespace OHOS