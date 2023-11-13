/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "system_sound_manager_impl.h"

#include "media_log.h"
#include "media_errors.h"

#include "ringtone_player_impl.h"

using namespace std;
using namespace OHOS::AbilityRuntime;
using namespace OHOS::NativeRdb;

namespace {
    const std::string RINGTONE_URI = "ringtone_uri";
    const std::string SYSTEM_TONE_URI = "system_tone_uri";
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SystemSoundManagerImpl"};
}

namespace OHOS {
namespace Media {
unique_ptr<SystemSoundManager> SystemSoundManagerFactory::CreateSystemSoundManager()
{
    unique_ptr<SystemSoundManagerImpl> systemSoundMgr = make_unique<SystemSoundManagerImpl>();
    CHECK_AND_RETURN_RET_LOG(systemSoundMgr != nullptr, nullptr, "Failed to create sound manager object");

    return systemSoundMgr;
}

SystemSoundManagerImpl::SystemSoundManagerImpl()
{
    LoadSystemSoundUriMap();
}

SystemSoundManagerImpl::~SystemSoundManagerImpl()
{
    if (abilityHelper_ != nullptr) {
        abilityHelper_->Release();
        abilityHelper_ = nullptr;
    }
}

void SystemSoundManagerImpl::LoadSystemSoundUriMap(void)
{
    if (!LoadUriFromKvStore(RINGTONE_TYPE_SIM_CARD_0, RINGTONE_URI)) {
        MEDIA_LOGE("SystemSoundManagerImpl::LoadSystemSoundUriMap: cann't load uri for default ringtone");
    }
    if (!LoadUriFromKvStore(RINGTONE_TYPE_SIM_CARD_1, RINGTONE_URI)) {
        MEDIA_LOGE("SystemSoundManagerImpl::LoadSystemSoundUriMap: cann't load uri for multisim ringtone");
    }
}

void SystemSoundManagerImpl::WriteUriToKvStore(RingtoneType ringtoneType, const std::string &systemSoundType,
    const std::string &uri)
{
    std::string key = GetKeyForRingtoneKvStore(ringtoneType, systemSoundType);
    MEDIA_LOGI("SystemSoundManagerImpl::WriteUriToKvStore ringtoneType %{public}d, %{public}s: %{public}s",
        ringtoneType, systemSoundType.c_str(), uri.c_str());
    int32_t result = AudioStandard::AudioSystemManager::GetInstance()->SetSystemSoundUri(key, uri);
    MEDIA_LOGI("SystemSoundManagerImpl::WriteUriToKvStore result: %{public}d", result);
}

bool SystemSoundManagerImpl::LoadUriFromKvStore(RingtoneType ringtoneType, const std::string &systemSoundType)
{
    std::string key = GetKeyForRingtoneKvStore(ringtoneType, systemSoundType);
    std::string uri = AudioStandard::AudioSystemManager::GetInstance()->GetSystemSoundUri(key);
    ringtoneUriMap_[ringtoneType][systemSoundType] = uri;
    return uri == "";
}

std::string SystemSoundManagerImpl::GetKeyForRingtoneKvStore(RingtoneType ringtoneType,
    const std::string &systemSoundType)
{
    switch (ringtoneType) {
        case RINGTONE_TYPE_SIM_CARD_0:
            return systemSoundType + "_for_sim_card_0";
        case RINGTONE_TYPE_SIM_CARD_1:
            return systemSoundType + "_for_sim_card_1";
        default:
            MEDIA_LOGE("[GetStreamNameByStreamType] ringtoneType: %{public}d is unavailable", ringtoneType);
            return "";
    }
}

int32_t SystemSoundManagerImpl::SetRingtoneUri(const shared_ptr<Context> &context, const string &uri,
    RingtoneType type)
{
    MEDIA_LOGI("SystemSoundManagerImpl::%{public}s", __func__);
    CHECK_AND_RETURN_RET_LOG(type >= RINGTONE_TYPE_SIM_CARD_0 && type <= RINGTONE_TYPE_SIM_CARD_1,
        MSERR_INVALID_VAL, "invalid type");
    ringtoneUriMap_[type][RINGTONE_URI] = uri;
    WriteUriToKvStore(type, RINGTONE_URI, uri);
    return MSERR_OK;
}

int32_t SystemSoundManagerImpl::SetSystemToneUri(const shared_ptr<Context> &context, const string &uri)
{
    MEDIA_LOGI("SystemSoundManagerImpl::%{public}s", __func__);
    ringtoneUriMap_[RINGTONE_TYPE_SIM_CARD_0][SYSTEM_TONE_URI] = uri;
    WriteUriToKvStore(RINGTONE_TYPE_SIM_CARD_0, SYSTEM_TONE_URI, uri);
    return MSERR_OK;
}

string SystemSoundManagerImpl::GetRingtoneUri(const shared_ptr<Context> &context, RingtoneType type)
{
    MEDIA_LOGI("SystemSoundManagerImpl::%{public}s", __func__);
    CHECK_AND_RETURN_RET_LOG(type >= RINGTONE_TYPE_SIM_CARD_0 && type <= RINGTONE_TYPE_SIM_CARD_1,
        "", "invalid type");
    return ringtoneUriMap_[type][RINGTONE_URI];
}

string SystemSoundManagerImpl::GetSystemToneUri(const shared_ptr<Context> &context)
{
    MEDIA_LOGI("SystemSoundManagerImpl::%{public}s", __func__);
    return ringtoneUriMap_[RINGTONE_TYPE_SIM_CARD_0][SYSTEM_TONE_URI];
}

shared_ptr<RingtonePlayer> SystemSoundManagerImpl::GetRingtonePlayer(const shared_ptr<Context> &context,
    RingtoneType type)
{
    MEDIA_LOGI("SystemSoundManagerImpl::%{public}s, type %{public}d", __func__, type);
    CHECK_AND_RETURN_RET_LOG(type >= RINGTONE_TYPE_SIM_CARD_0 && type <= RINGTONE_TYPE_SIM_CARD_1,
        nullptr, "invalid type");

    if (ringtonePlayer_[type] != nullptr && ringtonePlayer_[type]->GetRingtoneState() == STATE_RELEASED) {
        ringtonePlayer_[type] = nullptr;
    }

    if (ringtonePlayer_[type] == nullptr) {
        ringtonePlayer_[type] = make_shared<RingtonePlayerImpl>(context, *this, type);
        CHECK_AND_RETURN_RET_LOG(ringtonePlayer_[type] != nullptr, nullptr, "Failed to create ringtone player object");
    }

    return ringtonePlayer_[type];
}
} // namesapce AudioStandard
} // namespace OHOS
