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
#include "system_tone_player_impl.h"

using namespace std;
using namespace OHOS::AbilityRuntime;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SystemSoundManagerImpl"};
}

namespace OHOS {
namespace Media {
const std::string RING_TONE = "ring_tone";
const std::string SYSTEM_TONE = "system_tone";
unique_ptr<SystemSoundManager> SystemSoundManagerFactory::CreateSystemSoundManager()
{
    unique_ptr<SystemSoundManagerImpl> systemSoundMgr = make_unique<SystemSoundManagerImpl>();
    CHECK_AND_RETURN_RET_LOG(systemSoundMgr != nullptr, nullptr, "Failed to create sound manager object");

    return systemSoundMgr;
}

SystemSoundManagerImpl::SystemSoundManagerImpl()
{
    LoadSystemSoundUriMap();
    InitRingerMode();
}

SystemSoundManagerImpl::~SystemSoundManagerImpl()
{
    if (audioGroupManager_ != nullptr) {
        (void)audioGroupManager_->UnsetRingerModeCallback(getpid());
        ringerModeCallback_ = nullptr;
        audioGroupManager_ = nullptr;
    }
}

void SystemSoundManagerImpl::InitRingerMode(void)
{
    audioGroupManager_ = AudioStandard::AudioSystemManager::GetInstance()->
        GetGroupManager(AudioStandard::DEFAULT_VOLUME_GROUP_ID);
    if (audioGroupManager_ == nullptr) {
        MEDIA_LOGE("InitRingerMode: audioGroupManager_ is nullptr");
        return;
    }
    ringerMode_ = audioGroupManager_->GetRingerMode();

    ringerModeCallback_ = std::make_shared<RingerModeCallbackImpl>(*this);
    audioGroupManager_->SetRingerModeCallback(getpid(), ringerModeCallback_);
}

bool SystemSoundManagerImpl::isRingtoneTypeValid(RingtoneType ringtongType)
{
    switch (ringtongType) {
        case RINGTONE_TYPE_SIM_CARD_0:
        case RINGTONE_TYPE_SIM_CARD_1:
            return true;
        default:
            MEDIA_LOGE("isRingtoneTypeValid: ringtongType %{public}d is unavailable", ringtongType);
            return false;
    }
}

bool SystemSoundManagerImpl::isSystemToneTypeValid(SystemToneType systemToneType)
{
    switch (systemToneType) {
        case SYSTEM_TONE_TYPE_SIM_CARD_0:
        case SYSTEM_TONE_TYPE_SIM_CARD_1:
        case SYSTEM_TONE_TYPE_NOTIFICATION:
            return true;
        default:
            MEDIA_LOGE("isSystemToneTypeValid: systemToneType %{public}d is unavailable", systemToneType);
            return false;
    }
}

void SystemSoundManagerImpl::LoadSystemSoundUriMap(void)
{
    ringtoneUriMap_[RINGTONE_TYPE_SIM_CARD_0] =
        GetUriFromDatabase(GetKeyForDatabase(RING_TONE, RINGTONE_TYPE_SIM_CARD_0));
    ringtoneUriMap_[RINGTONE_TYPE_SIM_CARD_1] =
        GetUriFromDatabase(GetKeyForDatabase(RING_TONE, RINGTONE_TYPE_SIM_CARD_1));

    systemToneUriMap_[SYSTEM_TONE_TYPE_SIM_CARD_0] =
        GetUriFromDatabase(GetKeyForDatabase(SYSTEM_TONE, SYSTEM_TONE_TYPE_SIM_CARD_0));
    systemToneUriMap_[SYSTEM_TONE_TYPE_SIM_CARD_1] =
        GetUriFromDatabase(GetKeyForDatabase(SYSTEM_TONE, SYSTEM_TONE_TYPE_SIM_CARD_1));
    systemToneUriMap_[SYSTEM_TONE_TYPE_NOTIFICATION] =
        GetUriFromDatabase(GetKeyForDatabase(SYSTEM_TONE, SYSTEM_TONE_TYPE_NOTIFICATION));
}

void SystemSoundManagerImpl::WriteUriToDatabase(const std::string &key, const std::string &uri)
{
    int32_t result = AudioStandard::AudioSystemManager::GetInstance()->SetSystemSoundUri(key, uri);
    MEDIA_LOGI("WriteUriToDatabase: key: %{public}s, uri: %{public}s, result: %{public}d",
        key.c_str(), uri.c_str(), result);
}

std::string SystemSoundManagerImpl::GetUriFromDatabase(const std::string &key)
{
    std::string uri = AudioStandard::AudioSystemManager::GetInstance()->GetSystemSoundUri(key);

    MEDIA_LOGI("GetUriFromDatabase: key [%{public}s], uri [%{public}s]", key.c_str(), uri.c_str());
    return uri;
}

std::string SystemSoundManagerImpl::GetKeyForDatabase(const std::string &systemSoundType, int32_t type)
{
    if (systemSoundType == RING_TONE) {
        switch (static_cast<RingtoneType>(type)) {
            case RINGTONE_TYPE_SIM_CARD_0:
                return "ringtone_for_sim_card_0";
            case RINGTONE_TYPE_SIM_CARD_1:
                return "ringtone_for_sim_card_1";
            default:
                MEDIA_LOGE("GetKeyForDatabase: ringtoneType %{public}d is unavailable", type);
                return "";
        }
    } else if (systemSoundType == SYSTEM_TONE) {
        switch (static_cast<SystemToneType>(type)) {
            case SYSTEM_TONE_TYPE_SIM_CARD_0:
                return "system_tone_for_sim_card_0";
            case SYSTEM_TONE_TYPE_SIM_CARD_1:
                return "system_tone_for_sim_card_1";
            case SYSTEM_TONE_TYPE_NOTIFICATION:
                return "system_tone_for_notification";
            default:
                MEDIA_LOGE("GetKeyForDatabase: systemToneType %{public}d is unavailable", type);
                return "";
        }
    } else {
        MEDIA_LOGE("GetKeyForDatabase: systemSoundType %{public}s is unavailable", systemSoundType.c_str());
        return "";
    }
}

int32_t SystemSoundManagerImpl::SetRingtoneUri(const shared_ptr<Context> &context, const string &uri,
    RingtoneType ringtoneType)
{
    CHECK_AND_RETURN_RET_LOG(isRingtoneTypeValid(ringtoneType), MSERR_INVALID_VAL, "invalid ringtone type");
    MEDIA_LOGI("SetRingtoneUri: ringtoneType %{public}d, uri %{public}s", ringtoneType, uri.c_str());
    ringtoneUriMap_[ringtoneType] = uri;
    WriteUriToDatabase(GetKeyForDatabase(RING_TONE, ringtoneType), uri);
    return MSERR_OK;
}

string SystemSoundManagerImpl::GetRingtoneUri(const shared_ptr<Context> &context, RingtoneType ringtoneType)
{
    CHECK_AND_RETURN_RET_LOG(isRingtoneTypeValid(ringtoneType), "", "invalid ringtone type");
    MEDIA_LOGI("GetRingtoneUri: for ringtoneType %{public}d", ringtoneType);

    return ringtoneUriMap_[ringtoneType];
}

shared_ptr<RingtonePlayer> SystemSoundManagerImpl::GetRingtonePlayer(const shared_ptr<Context> &context,
    RingtoneType ringtoneType)
{
    CHECK_AND_RETURN_RET_LOG(isRingtoneTypeValid(ringtoneType), nullptr, "invalid ringtone type");
    MEDIA_LOGI("GetRingtonePlayer: for ringtoneType %{public}d", ringtoneType);

    if (ringtonePlayerMap_[ringtoneType] != nullptr &&
        ringtonePlayerMap_[ringtoneType]->GetRingtoneState() == STATE_RELEASED) {
        ringtonePlayerMap_[ringtoneType] = nullptr;
    }

    if (ringtonePlayerMap_[ringtoneType] == nullptr) {
        ringtonePlayerMap_[ringtoneType] = make_shared<RingtonePlayerImpl>(context, *this, ringtoneType);
        CHECK_AND_RETURN_RET_LOG(ringtonePlayerMap_[ringtoneType] != nullptr, nullptr,
            "Failed to create ringtone player object");
    }

    return ringtonePlayerMap_[ringtoneType];
}

std::shared_ptr<SystemTonePlayer> SystemSoundManagerImpl::GetSystemTonePlayer(
    const std::shared_ptr<AbilityRuntime::Context> &context, SystemToneType systemToneType)
{
    CHECK_AND_RETURN_RET_LOG(isSystemToneTypeValid(systemToneType), nullptr, "invalid system tone type");
    MEDIA_LOGI("GetSystemTonePlayer: for systemToneType %{public}d", systemToneType);

    if (systemTonePlayerMap_[systemToneType] != nullptr) {
        (void)systemTonePlayerMap_[systemToneType]->Release();
        systemTonePlayerMap_[systemToneType] = nullptr;
    }

    systemTonePlayerMap_[systemToneType] = make_shared<SystemTonePlayerImpl>(context, *this, systemToneType);
    CHECK_AND_RETURN_RET_LOG(systemTonePlayerMap_[systemToneType] != nullptr, nullptr,
        "Failed to create system tone player object");

    return systemTonePlayerMap_[systemToneType];
}

int32_t SystemSoundManagerImpl::SetSystemToneUri(const shared_ptr<Context> &context, const string &uri,
    SystemToneType systemToneType)
{
    CHECK_AND_RETURN_RET_LOG(isSystemToneTypeValid(systemToneType), MSERR_INVALID_VAL, "invalid system tone type");
    MEDIA_LOGI("SetSystemToneUri: systemToneType %{public}d, uri %{public}s", systemToneType, uri.c_str());

    systemToneUriMap_[systemToneType] = uri;
    WriteUriToDatabase(GetKeyForDatabase(SYSTEM_TONE, systemToneType), uri);
    return MSERR_OK;
}

std::string SystemSoundManagerImpl::GetSystemToneUri(const std::shared_ptr<AbilityRuntime::Context> &context,
    SystemToneType systemToneType)
{
    CHECK_AND_RETURN_RET_LOG(isSystemToneTypeValid(systemToneType), "", "invalid system tone type");
    MEDIA_LOGI("GetSystemToneUri: for systemToneType %{public}d", systemToneType);

    return systemToneUriMap_[systemToneType];
}

int32_t SystemSoundManagerImpl::SetRingerMode(const AudioStandard::AudioRingerMode &ringerMode)
{
    ringerMode_.store(ringerMode);
    return MSERR_OK;
}

AudioStandard::AudioRingerMode SystemSoundManagerImpl::GetRingerMode() const
{
    return ringerMode_.load();
}

// Ringer mode callback class symbols
RingerModeCallbackImpl::RingerModeCallbackImpl(SystemSoundManagerImpl &systemSoundManagerImpl)
    : sysSoundMgr_(systemSoundManagerImpl) {}

void RingerModeCallbackImpl::OnRingerModeUpdated(const AudioStandard::AudioRingerMode &ringerMode)
{
    ringerMode_ = ringerMode;
    int32_t result = sysSoundMgr_.SetRingerMode(ringerMode_);
    if (result == MSERR_OK && ringerMode_ == AudioStandard::AudioRingerMode::RINGER_MODE_SILENT) {
        SystemSoundVibrator::StopVibrator();
    }
}
} // namesapce AudioStandard
} // namespace OHOS
