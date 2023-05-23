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
static const std::list<std::string> AUDIO_PERMISSION = {"ohos.permission.MICROPHONE"};
static const std::list<std::string> VIDEO_PERMISSION = {"ohos.permission.CAMERA", "ohos.permission.CAPTURE_SCREEN"};
static const std::list<std::string> COMMON_PERMISSION = {"ohos.permission.MICROPHONE",
    "ohos.permission.CAMERA", "ohos.permission.CAPTURE_SCREEN"};
};

namespace OHOS {
namespace Media {
static const std::map<MediaPermission::RecorderPermissionType, std::list<std::string>> PERMISSION_MAP = {
    {MediaPermission::RecorderPermissionType::PERMISSION_AUDIO, AUDIO_PERMISSION},
    {MediaPermission::RecorderPermissionType::PERMISSION_VIDEO, VIDEO_PERMISSION},
    {MediaPermission::RecorderPermissionType::PERMISSION_COMMON, COMMON_PERMISSION},
};
const int32_t ROOT_UID = 0;

int32_t MediaPermission::CheckPermission(MediaPermission::RecorderPermissionType code)
{
    auto callerUid = IPCSkeleton::GetCallingUid();
    if (callerUid == ROOT_UID) {
        MEDIA_LOGI("Root user. Permission Granted");
        return Security::AccessToken::PERMISSION_GRANTED;
    }
    if (PERMISSION_MAP.find(code) == PERMISSION_MAP.end()) {
        MEDIA_LOGE("Permission type error");
        return Security::AccessToken::PERMISSION_DENIED;
    }
    Security::AccessToken::AccessTokenID tokenCaller = IPCSkeleton::GetCallingTokenID();
    std::list permissionList = PERMISSION_MAP.at(code);
    int32_t result = Security::AccessToken::PERMISSION_DENIED;
    for (auto iter = permissionList.begin(); iter != permissionList.end(); ++iter) {
        result = result && Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenCaller, *iter);
    }
    MEDIA_LOGI("Permission result: %{public}d", result);
    return result;
}
}
}