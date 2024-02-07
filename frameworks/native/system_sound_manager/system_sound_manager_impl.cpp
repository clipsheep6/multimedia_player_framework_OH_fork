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

#include <fstream>

#include "config_policy_utils.h"
#include "file_ex.h"
#include "nlohmann/json.hpp"

#include "media_log.h"
#include "media_errors.h"
#include "ringtone_player_impl.h"
#include "system_tone_player_impl.h"

using namespace std;
using namespace nlohmann;
using namespace OHOS::AbilityRuntime;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SystemSoundManagerImpl"};
}

namespace OHOS {
namespace Media {
const std::string RING_TONE = "ring_tone";
const std::string SYSTEM_TONE = "system_tone";
const std::string DEFAULT_SYSTEM_SOUND_PATH = "resource/media/audio/";
const std::string DEFAULT_RINGTONE_URI_JSON = "ringtone_incall.json";
const std::string DEFAULT_RINGTONE_PATH = "ringtones/";
const std::string DEFAULT_SYSTEM_TONE_URI_JSON = "ringtone_sms-notification.json";
const std::string DEFAULT_SYSTEM_TONE_PATH = "notifications/";

std::shared_ptr<SystemSoundManager> SystemSoundManagerFactory::systemSoundManager_ = nullptr;
std::mutex SystemSoundManagerFactory::systemSoundManagerMutex_;

std::shared_ptr<SystemSoundManager> SystemSoundManagerFactory::CreateSystemSoundManager()
{
    std::lock_guard<std::mutex> lock(systemSoundManagerMutex_);
    if (systemSoundManager_ == nullptr) {
        systemSoundManager_ = std::make_shared<SystemSoundManagerImpl>();
    }
    CHECK_AND_RETURN_RET_LOG(systemSoundManager_ != nullptr, nullptr, "Failed to create sound manager object");
    return systemSoundManager_;
}

SystemSoundManagerImpl::SystemSoundManagerImpl()
{
    InitDefaultUriMap();
    InitRingerMode();
}

SystemSoundManagerImpl::~SystemSoundManagerImpl()
{
    if (audioGroupManager_ != nullptr) {
        (void)audioGroupManager_->UnsetRingerModeCallback(getpid(), ringerModeCallback_);
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

void SystemSoundManagerImpl::InitDefaultUriMap()
{
    systemSoundPath_ = GetFullPath(DEFAULT_SYSTEM_SOUND_PATH);

    std::string ringtoneJsonPath = systemSoundPath_ + DEFAULT_RINGTONE_URI_JSON;
    InitDefaultRingtoneUriMap(ringtoneJsonPath);

    std::string systemToneJsonPath = systemSoundPath_ + DEFAULT_SYSTEM_TONE_URI_JSON;
    InitDefaultSystemToneUriMap(systemToneJsonPath);
}

void SystemSoundManagerImpl::InitDefaultRingtoneUriMap(const std::string &ringtoneJsonPath)
{
    std::lock_guard<std::mutex> lock(uriMutex_);

    std::string jsonValue = GetJsonValue(ringtoneJsonPath);
    nlohmann::json ringtoneJson = json::parse(jsonValue, nullptr, false);
    if (ringtoneJson.contains("preset_ringtone_sim1") && ringtoneJson["preset_ringtone_sim1"].is_string()) {
        std::string defaultRingtoneName = ringtoneJson["preset_ringtone_sim1"];
        defaultRingtoneUriMap_[RINGTONE_TYPE_SIM_CARD_0] =
            systemSoundPath_ + DEFAULT_RINGTONE_PATH + defaultRingtoneName + ".ogg";
        MEDIA_LOGI("preset_ringtone_sim1 is [%{public}s]", defaultRingtoneUriMap_[RINGTONE_TYPE_SIM_CARD_0].c_str());
    } else {
        defaultRingtoneUriMap_[RINGTONE_TYPE_SIM_CARD_0] = "";
        MEDIA_LOGW("InitDefaultRingtoneUriMap: failed to load uri of preset_ringtone_sim1");
    }
    if (ringtoneJson.contains("preset_ringtone_sim2") && ringtoneJson["preset_ringtone_sim2"].is_string()) {
        std::string defaultRingtoneName = ringtoneJson["preset_ringtone_sim2"];
        defaultRingtoneUriMap_[RINGTONE_TYPE_SIM_CARD_1] =
            systemSoundPath_ + DEFAULT_RINGTONE_PATH + defaultRingtoneName + ".ogg";
        MEDIA_LOGI("preset_ringtone_sim1 is [%{public}s]", defaultRingtoneUriMap_[RINGTONE_TYPE_SIM_CARD_1].c_str());
    } else {
        defaultRingtoneUriMap_[RINGTONE_TYPE_SIM_CARD_1] = "";
        MEDIA_LOGW("InitDefaultRingtoneUriMap: failed to load uri of preset_ringtone_sim2");
    }
}

std::string SystemSoundManagerImpl::GetDefaultRingtoneUri(RingtoneType ringtoneType)
{
    std::lock_guard<std::mutex> lock(uriMutex_);
    if (defaultRingtoneUriMap_.count(ringtoneType) == 0) {
        MEDIA_LOGE("Failed to GetDefaultRingtoneUri: invalid ringtone type %{public}d", ringtoneType);
        return "";
    }
    return defaultRingtoneUriMap_[ringtoneType];
}

std::string SystemSoundManagerImpl::GetDefaultSystemToneUri(SystemToneType systemToneType)
{
    std::lock_guard<std::mutex> lock(uriMutex_);
    if (defaultSystemToneUriMap_.count(systemToneType) == 0) {
        MEDIA_LOGE("Failed to GetDefaultRingtoneUri: invalid system tone type %{public}d", systemToneType);
        return "";
    }
    return defaultSystemToneUriMap_[systemToneType];
}

void SystemSoundManagerImpl::InitDefaultSystemToneUriMap(const std::string &systemToneJsonPath)
{
    std::lock_guard<std::mutex> lock(uriMutex_);

    std::string jsonValue = GetJsonValue(systemToneJsonPath);
    nlohmann::json systemToneJson = json::parse(jsonValue, nullptr, false);
    if (systemToneJson.contains("preset_ringtone_sms") && systemToneJson["preset_ringtone_sms"].is_string()) {
        std::string defaultSystemToneName = systemToneJson["preset_ringtone_sms"];
        defaultSystemToneUriMap_[SYSTEM_TONE_TYPE_SIM_CARD_0] =
            systemSoundPath_ + DEFAULT_SYSTEM_TONE_PATH + defaultSystemToneName + ".ogg";
        defaultSystemToneUriMap_[SYSTEM_TONE_TYPE_SIM_CARD_1] =
            systemSoundPath_ + DEFAULT_SYSTEM_TONE_PATH + defaultSystemToneName + ".ogg";
        MEDIA_LOGI("preset_ringtone_sms is [%{public}s]",
            defaultSystemToneUriMap_[SYSTEM_TONE_TYPE_SIM_CARD_0].c_str());
    } else {
        defaultSystemToneUriMap_[SYSTEM_TONE_TYPE_SIM_CARD_0] = "";
        defaultSystemToneUriMap_[SYSTEM_TONE_TYPE_SIM_CARD_1] = "";
        MEDIA_LOGW("InitDefaultSystemToneUriMap: failed to load uri of preset_ringtone_sms");
    }
    if (systemToneJson.contains("preset_ringtone_notification") &&
        systemToneJson["preset_ringtone_notification"].is_string()) {
        std::string defaultSystemToneName = systemToneJson["preset_ringtone_notification"];
        defaultSystemToneUriMap_[SYSTEM_TONE_TYPE_NOTIFICATION] =
            systemSoundPath_ + DEFAULT_SYSTEM_TONE_PATH + defaultSystemToneName + ".ogg";
        MEDIA_LOGI("preset_ringtone_notification is [%{public}s]",
            defaultSystemToneUriMap_[SYSTEM_TONE_TYPE_NOTIFICATION].c_str());
    } else {
        defaultSystemToneUriMap_[SYSTEM_TONE_TYPE_NOTIFICATION] = "";
        MEDIA_LOGW("InitDefaultSystemToneUriMap: failed to load uri of preset_ringtone_notification");
    }
}

std::string SystemSoundManagerImpl::GetFullPath(const std::string &originalUri)
{
    char buf[MAX_PATH_LEN];
    char *path = GetOneCfgFile(originalUri.c_str(), buf, MAX_PATH_LEN);
    if (path == nullptr || *path == '\0') {
        MEDIA_LOGE("GetOneCfgFile for %{public}s failed.", originalUri.c_str());
        return "";
    }
    std::string filePath = path;
    MEDIA_LOGI("GetFullPath for [%{public}s], result: [%{public}s]", originalUri.c_str(), filePath.c_str());
    return filePath;
}

std::string SystemSoundManagerImpl::GetJsonValue(const std::string &jsonPath)
{
    std::string jsonValue = "";

    ifstream file(jsonPath.c_str());
    if (!file.is_open()) {
        MEDIA_LOGI("file not open! try open first ! ");
        file.open(jsonPath.c_str(), ios::app);
        if (!file.is_open()) {
            MEDIA_LOGE("open file again fail !");
            return "";
        }
    }
    file.seekg(0, ios::end);

    const long maxFileLength = 32 * 1024 * 1024; // max size of the json file
    const long fileLength = file.tellg();
    if (fileLength > maxFileLength) {
        MEDIA_LOGE("invalid file length(%{public}ld)!", fileLength);
        return "";
    }

    jsonValue.clear();
    file.seekg(0, ios::beg);
    copy(istreambuf_iterator<char>(file), istreambuf_iterator<char>(), back_inserter(jsonValue));
    return jsonValue;
}

int32_t SystemSoundManagerImpl::WriteUriToDatabase(const std::string &key, const std::string &uri)
{
    int32_t result = AudioStandard::AudioSystemManager::GetInstance()->SetSystemSoundUri(key, uri);
    MEDIA_LOGI("WriteUriToDatabase: key: %{public}s, uri: %{public}s, result: %{public}d",
        key.c_str(), uri.c_str(), result);
    return result;
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
    std::lock_guard<std::mutex> lock(uriMutex_);
    CHECK_AND_RETURN_RET_LOG(isRingtoneTypeValid(ringtoneType), MSERR_INVALID_VAL, "Invalid ringtone type");

    MEDIA_LOGI("SetRingtoneUri: ringtoneType %{public}d, uri %{public}s", ringtoneType, uri.c_str());
    int32_t result = WriteUriToDatabase(GetKeyForDatabase(RING_TONE, ringtoneType), uri);
    CHECK_AND_RETURN_RET_LOG(result == MSERR_OK, MSERR_INVALID_OPERATION,
        "Failed to write ringtone uri to database: result %{public}d", result);
    return result;
}

std::string SystemSoundManagerImpl::GetRingtoneUri(const shared_ptr<Context> &context, RingtoneType ringtoneType)
{
    std::lock_guard<std::mutex> lock(uriMutex_);
    CHECK_AND_RETURN_RET_LOG(isRingtoneTypeValid(ringtoneType), "", "Invalid ringtone type");

    std::string ringtoneUri = GetUriFromDatabase(GetKeyForDatabase(RING_TONE, ringtoneType));
    if (ringtoneUri == "") {
        MEDIA_LOGI("The ringtone uri for ringtoneType %{public}d is empty. Return default uri.", ringtoneType);
        ringtoneUri = defaultRingtoneUriMap_[ringtoneType];
    }
    return ringtoneUri;
}

std::shared_ptr<RingtonePlayer> SystemSoundManagerImpl::GetRingtonePlayer(const shared_ptr<Context> &context,
    RingtoneType ringtoneType)
{
    std::lock_guard<std::mutex> lock(playerMutex_);
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
    std::lock_guard<std::mutex> lock(playerMutex_);
    CHECK_AND_RETURN_RET_LOG(isSystemToneTypeValid(systemToneType), nullptr, "invalid system tone type");
    MEDIA_LOGI("GetSystemTonePlayer: for systemToneType %{public}d", systemToneType);

    systemTonePlayerMap_[systemToneType] = make_shared<SystemTonePlayerImpl>(context, *this, systemToneType);
    CHECK_AND_RETURN_RET_LOG(systemTonePlayerMap_[systemToneType] != nullptr, nullptr,
        "Failed to create system tone player object");
    return systemTonePlayerMap_[systemToneType];
}

int32_t SystemSoundManagerImpl::SetSystemToneUri(const shared_ptr<Context> &context, const string &uri,
    SystemToneType systemToneType)
{
    std::lock_guard<std::mutex> lock(uriMutex_);
    CHECK_AND_RETURN_RET_LOG(isSystemToneTypeValid(systemToneType), MSERR_INVALID_VAL, "Invalid system tone type");

    MEDIA_LOGI("SetSystemToneUri: systemToneType %{public}d, uri %{public}s", systemToneType, uri.c_str());
    int32_t result = WriteUriToDatabase(GetKeyForDatabase(SYSTEM_TONE, systemToneType), uri);
    CHECK_AND_RETURN_RET_LOG(result == MSERR_OK, MSERR_INVALID_OPERATION,
        "Failed to write system tone uri to database: result %{public}d", result);
    return MSERR_OK;
}

std::string SystemSoundManagerImpl::GetSystemToneUri(const std::shared_ptr<AbilityRuntime::Context> &context,
    SystemToneType systemToneType)
{
    std::lock_guard<std::mutex> lock(uriMutex_);
    CHECK_AND_RETURN_RET_LOG(isSystemToneTypeValid(systemToneType), "", "Invalid system tone type");

    std::string systemToneUri = GetUriFromDatabase(GetKeyForDatabase(SYSTEM_TONE, systemToneType));
    if (systemToneUri == "") {
        MEDIA_LOGI("The system tone uri for systemToneType %{public}d is empty. Return default uri.", systemToneType);
        systemToneUri = defaultSystemToneUriMap_[systemToneType];
    }
    return systemToneUri;
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
