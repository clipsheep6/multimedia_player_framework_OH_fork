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
#include <list>
#include <map>

#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "media_log.h"
#include "media_permission.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaPermission"};
};

namespace OHOS {
namespace Media {
const int32_t ROOT_UID = 0;

int32_t MediaPermission::CheckMicPermission()
{
    auto callerUid = IPCSkeleton::GetCallingUid();
    if (callerUid == ROOT_UID) {
        MEDIA_LOGI("Root user. Permission Granted");
        return Security::AccessToken::PERMISSION_GRANTED;
    }
    Security::AccessToken::AccessTokenID tokenCaller = IPCSkeleton::GetCallingTokenID();
    return Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenCaller, "ohos.permission.MICROPHONE");
}

int32_t MediaPermission::CheckNetWorkPermission(int32_t appUid, int32_t appPid, uint32_t appTokenId)
{
    if (appUid == ROOT_UID) {
        MEDIA_LOGI("Root user. Permission Granted");
        return Security::AccessToken::PERMISSION_GRANTED;
    }
    MEDIA_LOGD("enter and check appUid: %{public}d", appUid);
    Security::AccessToken::AccessTokenID tokenCaller = appTokenId;
    return Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenCaller, "ohos.permission.INTERNET");
}
}
}