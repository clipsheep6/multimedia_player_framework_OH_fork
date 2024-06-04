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

#include <iostream>
#include "system_ability_definition.h"
#include "ringtone_db_const.h"
#include "ringtone_asset.h"
#include "ringtone_fetch_result.h"
#include "iservice_registry.h"
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>

using namespace std;
using namespace nlohmann;
using namespace OHOS::AbilityRuntime;
using namespace OHOS::DataShare;

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
const int STORAGE_MANAGER_MANAGER_ID = 5003;
const int TYPEERROR = -2;
const int ERROR = -1;
const int SUCCESS = 0;

std::shared_ptr<SystemSoundManager> SystemSoundManagerFactory::systemSoundManager_ = nullptr;
std::mutex SystemSoundManagerFactory::systemSoundManagerMutex_;
std::shared_ptr<DataShare::DataShareHelper> g_dataShareHelper = nullptr;
std::unordered_map<RingtoneType, RingToneType> ringtoneTypeMap_;
std::unordered_map<int32_t, ToneCustomizedType> sourceTypeMap_;
std::unordered_map<SystemToneType, int32_t> systemTypeMap_;
Uri RINGTONEURI(RINGTONE_PATH_URI);
vector<string> COLUMNS = {{RINGTONE_COLUMN_TONE_ID}, {RINGTONE_COLUMN_DATA}, {RINGTONE_COLUMN_DISPLAY_NAME},
    {RINGTONE_COLUMN_TITLE}, {RINGTONE_COLUMN_TONE_TYPE}, {RINGTONE_COLUMN_SOURCE_TYPE},
    {RINGTONE_COLUMN_SHOT_TONE_TYPE}, {RINGTONE_COLUMN_SHOT_TONE_SOURCE_TYPE}, {RINGTONE_COLUMN_NOTIFICATION_TONE_TYPE},
    {RINGTONE_COLUMN_NOTIFICATION_TONE_SOURCE_TYPE}, {RINGTONE_COLUMN_RING_TONE_TYPE},
    {RINGTONE_COLUMN_RING_TONE_SOURCE_TYPE}, {RINGTONE_COLUMN_ALARM_TONE_TYPE},
    {RINGTONE_COLUMN_ALARM_TONE_SOURCE_TYPE}};
std::vector<std::string> RINGTONETYPE = {{RINGTONE_CONTAINER_TYPE_MP3}, {RINGTONE_CONTAINER_TYPE_OGG},
    {RINGTONE_CONTAINER_TYPE_AC3}, {RINGTONE_CONTAINER_TYPE_AAC}, {RINGTONE_CONTAINER_TYPE_FLAC},
    {RINGTONE_CONTAINER_TYPE_WAV}};

std::shared_ptr<SystemSoundManager> SystemSoundManagerFactory::CreateSystemSoundManager()
{
    std::lock_guard<std::mutex> lock(systemSoundManagerMutex_);
    if (systemSoundManager_ == nullptr) {
        systemSoundManager_ = std::make_shared<SystemSoundManagerImpl>();
    }
    CHECK_AND_RETURN_RET_LOG(systemSoundManager_ != nullptr, nullptr, "Failed to create sound manager object");
    return systemSoundManager_;
}

static shared_ptr<DataShare::DataShareHelper> CreateDataShareHelper(int32_t systemAbilityId)
{
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        return nullptr;
    }
    auto remoteObj = saManager->GetSystemAbility(systemAbilityId);
    if (remoteObj == nullptr) {
        return nullptr;
    }
    return DataShare::DataShareHelper::Creator(remoteObj, RINGTONE_URI);
}

SystemSoundManagerImpl::SystemSoundManagerImpl()
{
    InitDefaultUriMap();
    InitRingerMode();
    ringtoneTypeMap_[RINGTONE_TYPE_SIM_CARD_0] = RING_TONE_TYPE_SIM_CARD_1;
    ringtoneTypeMap_[RINGTONE_TYPE_SIM_CARD_1] = RING_TONE_TYPE_SIM_CARD_2;
    sourceTypeMap_[SOURCE_TYPE_PRESET] = PRE_INSTALLED;
    sourceTypeMap_[SOURCE_TYPE_CUSTOMISED] = CUSTOMISED;
    systemTypeMap_[SYSTEM_TONE_TYPE_SIM_CARD_0] = SHOT_TONE_TYPE_SIM_CARD_1;
    systemTypeMap_[SYSTEM_TONE_TYPE_SIM_CARD_1] = SHOT_TONE_TYPE_SIM_CARD_2;
    systemTypeMap_[SYSTEM_TONE_TYPE_NOTIFICATION] = NOTIFICATION_TONE_TYPE;
    g_dataShareHelper = CreateDataShareHelper(STORAGE_MANAGER_MANAGER_ID);
    if (g_dataShareHelper == nullptr) {
        MEDIA_LOGE("SystemSoundManagerImpl: g_dataShareHelper is nullptr");
    }
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
    if (ringtoneJson.is_discarded()) {
        MEDIA_LOGE("ringtoneJson parsing is false !");
        return;
    }
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
    if (systemToneJson.is_discarded()) {
        MEDIA_LOGE("systemToneJson parsing is false !");
        return;
    }
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
    const std::string fdHead = "fd://";
    size_t found = ringtoneUri.find(RINGTONE_CUSTOMIZED_BASE_PATH);
    if (found != std::string::npos) {
        DataShare::DatashareBusinessError businessError;
        DataShare::DataSharePredicates queryPredicates;
        queryPredicates.EqualTo(RINGTONE_COLUMN_DATA, ringtoneUri);
        auto resultSet = g_dataShareHelper->Query(RINGTONEURI, queryPredicates, COLUMNS, &businessError);
        auto results = make_unique<RingtoneFetchResult<RingtoneAsset>>(move(resultSet));
        unique_ptr<RingtoneAsset> ringtoneAsset = results->GetFirstObject();
        if (ringtoneAsset != nullptr) {
            string uriStr = RINGTONE_PATH_URI + RINGTONE_SLASH_CHAR + to_string(ringtoneAsset->GetId());
            Uri ofUri(uriStr);
            int32_t fd = g_dataShareHelper->OpenFile(ofUri, "rw");
            if (fd > 0) {
                ringtoneUri = fdHead + to_string(fd);
            }
        }
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
    const std::string fdHead = "fd://";
    size_t found = systemToneUri.find(RINGTONE_CUSTOMIZED_BASE_PATH);
    if (found != std::string::npos) {
        DataShare::DatashareBusinessError businessError;
        DataShare::DataSharePredicates queryPredicates;
        queryPredicates.EqualTo(RINGTONE_COLUMN_DATA, systemToneUri);
        auto resultSet = g_dataShareHelper->Query(RINGTONEURI, queryPredicates, COLUMNS, &businessError);
        auto results = make_unique<RingtoneFetchResult<RingtoneAsset>>(move(resultSet));
        unique_ptr<RingtoneAsset> ringtoneAsset = results->GetFirstObject();
        if (ringtoneAsset != nullptr) {
            string uriStr = RINGTONE_PATH_URI + RINGTONE_SLASH_CHAR + to_string(ringtoneAsset->GetId());
            Uri ofUri(uriStr);
            int32_t fd = g_dataShareHelper->OpenFile(ofUri, "rw");
            if (fd > 0) {
                systemToneUri = fdHead + to_string(fd);
            }
        }
    }
    return systemToneUri;
}

std::shared_ptr<ToneAttrs> SystemSoundManagerImpl::GetDefaultRingtoneAttrs(
    const shared_ptr<Context> &context, RingtoneType ringtoneType)
{
    std::lock_guard<std::mutex> lock(uriMutex_);
    CHECK_AND_RETURN_RET_LOG(isRingtoneTypeValid(ringtoneType),  nullptr, "Invalid ringtone type");
    DataShare::DatashareBusinessError businessError;
    DataShare::DataSharePredicates queryPredicates;
    queryPredicates.EqualTo(RINGTONE_COLUMN_RING_TONE_TYPE, to_string(ringtoneTypeMap_[ringtoneType]));
    auto resultSet = g_dataShareHelper->Query(RINGTONEURI, queryPredicates, COLUMNS, &businessError);
    auto results = make_unique<RingtoneFetchResult<RingtoneAsset>>(move(resultSet));
    unique_ptr<RingtoneAsset> ringtoneAsset = results->GetFirstObject();
    while ((ringtoneAsset != nullptr) && (SOURCE_TYPE_PRESET != ringtoneAsset->GetSourceType())) {
        ringtoneAsset = results->GetNextObject();
    }
    if (ringtoneAsset != nullptr) {
        ringtoneAttrs_ = std::make_shared<ToneAttrs>(ringtoneAsset->GetTitle(),
            ringtoneAsset->GetDisplayName(), ringtoneAsset->GetPath(),
            sourceTypeMap_[ringtoneAsset->GetSourceType()], TONE_CATEGORY_RINGTONE);
    } else {
        MEDIA_LOGE("GetDefaultRingtoneAttrs: no single card default ringtone in the ringtone library!");
    }
    DataShare::DataSharePredicates queryPredicatesBothCard;
    queryPredicatesBothCard.EqualTo(RINGTONE_COLUMN_RING_TONE_TYPE, to_string(RING_TONE_TYPE_SIM_CARD_BOTH));
    resultSet = g_dataShareHelper->Query(RINGTONEURI, queryPredicatesBothCard, COLUMNS, &businessError);
    results = make_unique<RingtoneFetchResult<RingtoneAsset>>(move(resultSet));
    unique_ptr<RingtoneAsset> ringtoneAssetBothCard = results->GetFirstObject();
    while ((ringtoneAssetBothCard != nullptr) &&
        (SOURCE_TYPE_PRESET != ringtoneAssetBothCard->GetSourceType())) {
        ringtoneAssetBothCard = results->GetNextObject();
    }
    if (ringtoneAssetBothCard != nullptr) {
        ringtoneAttrs_ = std::make_shared<ToneAttrs>(ringtoneAssetBothCard->GetTitle(),
            ringtoneAssetBothCard->GetDisplayName(), ringtoneAssetBothCard->GetPath(),
            sourceTypeMap_[ringtoneAssetBothCard->GetSourceType()], TONE_CATEGORY_RINGTONE);
    } else {
        MEDIA_LOGE("GetDefaultRingtoneAttrs: no both card default ringtone in the ringtone library!");
    }
    return ringtoneAttrs_;
}

std::vector<std::shared_ptr<ToneAttrs>> SystemSoundManagerImpl::GetRingtoneAttrList(
    const std::shared_ptr<AbilityRuntime::Context> &context, RingtoneType ringtoneType)
{
    std::lock_guard<std::mutex> lock(uriMutex_);
    DataShare::DatashareBusinessError businessError;
    DataShare::DataSharePredicates queryPredicates;
    ringtoneAttrsArray_.clear();
    queryPredicates.EqualTo(RINGTONE_COLUMN_TONE_TYPE, to_string(TONE_TYPE_RINGTONE));
    auto resultSet = g_dataShareHelper->Query(RINGTONEURI, queryPredicates, COLUMNS, &businessError);
    auto results = make_unique<RingtoneFetchResult<RingtoneAsset>>(move(resultSet));
    unique_ptr<RingtoneAsset> ringtoneAsset = results->GetFirstObject();
    while (ringtoneAsset != nullptr && ringtoneAsset->GetSourceType() == SOURCE_TYPE_PRESET) {
        ringtoneAttrs_ = std::make_shared<ToneAttrs>(ringtoneAsset->GetTitle(),
            ringtoneAsset->GetDisplayName(), ringtoneAsset->GetPath(),
            sourceTypeMap_[ringtoneAsset->GetSourceType()], TONE_CATEGORY_RINGTONE);
        ringtoneAttrsArray_.push_back(ringtoneAttrs_);
        ringtoneAsset = results->GetNextObject();
    }
    DataShare::DataSharePredicates queryPredicatesBothCard;
    queryPredicatesBothCard.EqualTo(RINGTONE_COLUMN_RING_TONE_TYPE, to_string(RING_TONE_TYPE_SIM_CARD_BOTH));
    resultSet = g_dataShareHelper->Query(RINGTONEURI, queryPredicatesBothCard, COLUMNS, &businessError);
    results = make_unique<RingtoneFetchResult<RingtoneAsset>>(move(resultSet));
    unique_ptr<RingtoneAsset> ringtoneAssetBothCard = results->GetFirstObject();
    while (ringtoneAssetBothCard != nullptr) {
        ringtoneAttrs_ = std::make_shared<ToneAttrs>(ringtoneAssetBothCard->GetTitle(),
            ringtoneAssetBothCard->GetDisplayName(), ringtoneAssetBothCard->GetPath(),
            sourceTypeMap_[ringtoneAssetBothCard->GetSourceType()], TONE_CATEGORY_RINGTONE);
        ringtoneAttrsArray_.push_back(ringtoneAttrs_);
        ringtoneAssetBothCard = results->GetNextObject();
    }
    if (ringtoneAttrsArray_.empty()) {
        MEDIA_LOGE("GetRingtoneAttrList: no ringtone in the ringtone library!");
    }
    return ringtoneAttrsArray_;
}

std::shared_ptr<ToneAttrs> SystemSoundManagerImpl::GetDefaultSystemToneAttrs(
    const std::shared_ptr<AbilityRuntime::Context> &context, SystemToneType systemToneType)
{
    std::lock_guard<std::mutex> lock(uriMutex_);
    CHECK_AND_RETURN_RET_LOG(isSystemToneTypeValid(systemToneType),  nullptr, "Invalid systemtone type");
    std::string ringToneType = systemToneType == SYSTEM_TONE_TYPE_NOTIFICATION ?
        RINGTONE_COLUMN_NOTIFICATION_TONE_TYPE : RINGTONE_COLUMN_SHOT_TONE_TYPE;
    int32_t category = systemToneType == SYSTEM_TONE_TYPE_NOTIFICATION ?
        TONE_CATEGORY_NOTIFICATION : TONE_CATEGORY_TEXT_MESSAGE;
    int32_t valueOfRingToneType = systemToneType == SYSTEM_TONE_TYPE_NOTIFICATION ?
        NOTIFICATION_TONE_TYPE : SHOT_TONE_TYPE_SIM_CARD_BOTH;
    DataShare::DatashareBusinessError businessError;
    DataShare::DataSharePredicates queryPredicates;
    queryPredicates.EqualTo(ringToneType, to_string(systemTypeMap_[systemToneType]));
    auto resultSet = g_dataShareHelper->Query(RINGTONEURI, queryPredicates, COLUMNS, &businessError);
    auto results = make_unique<RingtoneFetchResult<RingtoneAsset>>(move(resultSet));
    unique_ptr<RingtoneAsset> ringtoneAsset = results->GetFirstObject();
    while ((ringtoneAsset != nullptr) && (SOURCE_TYPE_PRESET != ringtoneAsset->GetSourceType())) {
        ringtoneAsset = results->GetNextObject();
    }
    if (ringtoneAsset != nullptr) {
        systemtoneAttrs_ = std::make_shared<ToneAttrs>(ringtoneAsset->GetTitle(),
            ringtoneAsset->GetDisplayName(), ringtoneAsset->GetPath(),
            sourceTypeMap_[ringtoneAsset->GetSourceType()], category);
    } else {
        MEDIA_LOGE("GetDefaultSystemToneAttrs: no single default systemtone in the ringtone library!");
    }
    DataShare::DataSharePredicates queryPredicatesBothCard;
    queryPredicatesBothCard.EqualTo(ringToneType, to_string(valueOfRingToneType));
    resultSet = g_dataShareHelper->Query(RINGTONEURI, queryPredicatesBothCard, COLUMNS, &businessError);
    results = make_unique<RingtoneFetchResult<RingtoneAsset>>(move(resultSet));
    unique_ptr<RingtoneAsset> ringtoneAssetBothCard = results->GetFirstObject();
    while ((ringtoneAssetBothCard != nullptr) &&
        (SOURCE_TYPE_PRESET != ringtoneAssetBothCard->GetSourceType())) {
        ringtoneAssetBothCard = results->GetNextObject();
    }
    if (ringtoneAssetBothCard != nullptr) {
        systemtoneAttrs_ = std::make_shared<ToneAttrs>(ringtoneAssetBothCard->GetTitle(),
            ringtoneAssetBothCard->GetDisplayName(), ringtoneAssetBothCard->GetPath(),
            sourceTypeMap_[ringtoneAssetBothCard->GetSourceType()], category);
    } else {
        MEDIA_LOGE("GetDefaultSystemToneAttrs: no both card default systemtone in the ringtone library!");
    }
    return systemtoneAttrs_;
}

std::vector<std::shared_ptr<ToneAttrs>> SystemSoundManagerImpl::GetSystemToneAttrList(
    const std::shared_ptr<AbilityRuntime::Context> &context, SystemToneType systemToneType)
{
    std::lock_guard<std::mutex> lock(uriMutex_);
    std::string ringToneType = systemToneType == SYSTEM_TONE_TYPE_NOTIFICATION ?
        RINGTONE_COLUMN_NOTIFICATION_TONE_TYPE : RINGTONE_COLUMN_SHOT_TONE_TYPE;
    int32_t category = systemToneType == SYSTEM_TONE_TYPE_NOTIFICATION ?
        TONE_CATEGORY_NOTIFICATION : TONE_CATEGORY_TEXT_MESSAGE;
    int32_t valueOfRingToneType = systemToneType == SYSTEM_TONE_TYPE_NOTIFICATION ?
        NOTIFICATION_TONE_TYPE : SHOT_TONE_TYPE_SIM_CARD_BOTH;
    DataShare::DatashareBusinessError businessError;
    DataShare::DataSharePredicates queryPredicates;
    systemtoneAttrsArray_.clear();
    queryPredicates.EqualTo(RINGTONE_COLUMN_TONE_TYPE, to_string(TONE_TYPE_NOTIFICATION));
    auto resultSet = g_dataShareHelper->Query(RINGTONEURI, queryPredicates, COLUMNS, &businessError);
    auto results = make_unique<RingtoneFetchResult<RingtoneAsset>>(move(resultSet));
    unique_ptr<RingtoneAsset> ringtoneAsset = results->GetFirstObject();
    while (ringtoneAsset != nullptr && ringtoneAsset->GetSourceType() == SOURCE_TYPE_PRESET) {
        systemtoneAttrs_ = std::make_shared<ToneAttrs>(ringtoneAsset->GetTitle(),
            ringtoneAsset->GetDisplayName(), ringtoneAsset->GetPath(),
            sourceTypeMap_[ringtoneAsset->GetSourceType()], category);
        systemtoneAttrsArray_.push_back(systemtoneAttrs_);
        ringtoneAsset = results->GetNextObject();
    }
    DataShare::DataSharePredicates queryPredicatesBothCard;
    queryPredicatesBothCard.EqualTo(ringToneType, to_string(valueOfRingToneType));
    resultSet = g_dataShareHelper->Query(RINGTONEURI, queryPredicatesBothCard, COLUMNS, &businessError);
    results = make_unique<RingtoneFetchResult<RingtoneAsset>>(move(resultSet));
    unique_ptr<RingtoneAsset> ringtoneAssetBothCard = results->GetFirstObject();
    while (ringtoneAssetBothCard != nullptr) {
        systemtoneAttrs_ = std::make_shared<ToneAttrs>(ringtoneAssetBothCard->GetTitle(),
            ringtoneAssetBothCard->GetDisplayName(), ringtoneAssetBothCard->GetPath(),
            sourceTypeMap_[ringtoneAssetBothCard->GetSourceType()], category);
        systemtoneAttrsArray_.push_back(systemtoneAttrs_);
        ringtoneAssetBothCard = results->GetNextObject();
    }
    if (systemtoneAttrsArray_.empty()) {
        MEDIA_LOGE("GetSystemToneAttrList: no systemtone in the ringtone library!");
    }
    return systemtoneAttrsArray_;
}

int32_t SystemSoundManagerImpl::SetAlarmToneUri(const std::shared_ptr<AbilityRuntime::Context> &context,
    const std::string &uri)
{
    std::lock_guard<std::mutex> lock(uriMutex_);
    DataShare::DatashareBusinessError businessError;
    DataShare::DataSharePredicates queryPredicates;
    DataShare::DataSharePredicates queryPredicatesByUri;
    queryPredicatesByUri.EqualTo(RINGTONE_COLUMN_DATA, uri);
    auto resultSetByUri = g_dataShareHelper->Query(RINGTONEURI, queryPredicatesByUri, COLUMNS, &businessError);
    auto resultsByUri = make_unique<RingtoneFetchResult<RingtoneAsset>>(move(resultSetByUri));
    unique_ptr<RingtoneAsset> ringtoneAssetByUri = resultsByUri->GetFirstObject();
    if (ringtoneAssetByUri == nullptr) {
        MEDIA_LOGE("SetAlarmToneUri: tone of uri is not in the ringtone library!");
        return ERROR;
    }
    queryPredicates.EqualTo(RINGTONE_COLUMN_TONE_TYPE, TONE_TYPE_ALARM);
    auto resultSet = g_dataShareHelper->Query(RINGTONEURI, queryPredicates, COLUMNS, &businessError);
    auto results = make_unique<RingtoneFetchResult<RingtoneAsset>>(move(resultSet));
    unique_ptr<RingtoneAsset> ringtoneAsset = results->GetFirstObject();
    while ((ringtoneAsset != nullptr) && (uri != ringtoneAsset->GetPath())) {
        ringtoneAsset = results->GetNextObject();
    }
    if (ringtoneAsset != nullptr) {
        DataSharePredicates updateOldPredicates;
        DataShareValuesBucket updateOldValuesBucket;
        updateOldPredicates.SetWhereClause(RINGTONE_COLUMN_ALARM_TONE_SOURCE_TYPE + " = ? ");
        updateOldPredicates.SetWhereArgs({to_string(SOURCE_TYPE_CUSTOMISED)});
        updateOldValuesBucket.Put(RINGTONE_COLUMN_ALARM_TONE_TYPE, ALARM_TONE_TYPE_NOT);
        updateOldValuesBucket.Put(RINGTONE_COLUMN_ALARM_TONE_SOURCE_TYPE, SOURCE_TYPE_INVALID);
        g_dataShareHelper->Update(RINGTONEURI, updateOldPredicates, updateOldValuesBucket);

        DataSharePredicates updatePredicates;
        DataShareValuesBucket updateValuesBucket;
        updatePredicates.SetWhereClause(RINGTONE_COLUMN_TONE_ID + " = ? ");
        updatePredicates.SetWhereArgs({to_string(ringtoneAsset->GetId())});
        updateValuesBucket.Put(RINGTONE_COLUMN_ALARM_TONE_TYPE, ALARM_TONE_TYPE);
        updateValuesBucket.Put(RINGTONE_COLUMN_ALARM_TONE_SOURCE_TYPE, SOURCE_TYPE_CUSTOMISED);
        int32_t changedRows = g_dataShareHelper->Update(RINGTONEURI, updatePredicates, updateValuesBucket);
        return changedRows > 0 ? SUCCESS : ERROR;
    }
    MEDIA_LOGE("SetAlarmToneUri: tone of uri is not alarm!");
    return TYPEERROR;
}

std::string SystemSoundManagerImpl::GetAlarmToneUri(const std::shared_ptr<AbilityRuntime::Context> &context)
{
    int32_t count = 2;
    std::string alarmToneUri;
    std::lock_guard<std::mutex> lock(uriMutex_);
    DataShare::DatashareBusinessError businessError;
    DataShare::DataSharePredicates queryPredicates;
    queryPredicates.EqualTo(RINGTONE_COLUMN_ALARM_TONE_TYPE, to_string(ALARM_TONE_TYPE));
    auto resultSet = g_dataShareHelper->Query(RINGTONEURI, queryPredicates, COLUMNS, &businessError);
    auto results = make_unique<RingtoneFetchResult<RingtoneAsset>>(move(resultSet));
    unique_ptr<RingtoneAsset> ringtoneAsset = results->GetFirstObject();
    while ((ringtoneAsset != nullptr) && (SOURCE_TYPE_CUSTOMISED !=
        ringtoneAsset->GetAlarmtoneSourceType()) && (results->GetCount() == count)) {
        ringtoneAsset = results->GetNextObject();
    }
    if (ringtoneAsset != nullptr) {
        alarmToneUri = ringtoneAsset->GetPath();
    } else {
        MEDIA_LOGE("GetAlarmToneUri: no alarmtone in the ringtone library!");
    }
    return alarmToneUri;
}

std::shared_ptr<ToneAttrs> SystemSoundManagerImpl::GetDefaultAlarmToneAttrs(
    const std::shared_ptr<AbilityRuntime::Context> &context)
{
    std::lock_guard<std::mutex> lock(uriMutex_);
    DataShare::DatashareBusinessError businessError;
    DataShare::DataSharePredicates queryPredicates;
    queryPredicates.EqualTo(RINGTONE_COLUMN_ALARM_TONE_TYPE, to_string(ALARM_TONE_TYPE));
    auto resultSet = g_dataShareHelper->Query(RINGTONEURI, queryPredicates, COLUMNS, &businessError);
    auto results = make_unique<RingtoneFetchResult<RingtoneAsset>>(move(resultSet));
    unique_ptr<RingtoneAsset> ringtoneAsset = results->GetFirstObject();
    while ((ringtoneAsset != nullptr) && (SOURCE_TYPE_PRESET != ringtoneAsset->GetSourceType())) {
        ringtoneAsset = results->GetNextObject();
    }
    if (ringtoneAsset != nullptr) {
        alarmtoneAttrs_ = std::make_shared<ToneAttrs>(ringtoneAsset->GetTitle(),
            ringtoneAsset->GetDisplayName(), ringtoneAsset->GetPath(),
            sourceTypeMap_[ringtoneAsset->GetSourceType()], TONE_CATEGORY_ALARM);
    } else {
        MEDIA_LOGE("GetDefaultAlarmToneAttrs: no default alarmtone in the ringtone library!");
    }
    return alarmtoneAttrs_;
}

std::vector<std::shared_ptr<ToneAttrs>> SystemSoundManagerImpl::GetAlarmToneAttrList
    (const std::shared_ptr<AbilityRuntime::Context> &context)
{
    std::lock_guard<std::mutex> lock(uriMutex_);
    DataShare::DatashareBusinessError businessError;
    DataShare::DataSharePredicates queryPredicates;
    alarmtoneAttrsArray_.clear();
    queryPredicates.EqualTo(RINGTONE_COLUMN_ALARM_TONE_TYPE, to_string(ALARM_TONE_TYPE));
    auto resultSet = g_dataShareHelper->Query(RINGTONEURI, queryPredicates, COLUMNS, &businessError);
    auto results = make_unique<RingtoneFetchResult<RingtoneAsset>>(move(resultSet));
    unique_ptr<RingtoneAsset> ringtoneAsset = results->GetFirstObject();
    while (ringtoneAsset != nullptr) {
        alarmtoneAttrs_ = std::make_shared<ToneAttrs>(ringtoneAsset->GetTitle(),
            ringtoneAsset->GetDisplayName(), ringtoneAsset->GetPath(),
            sourceTypeMap_[ringtoneAsset->GetSourceType()], TONE_CATEGORY_ALARM);
        alarmtoneAttrsArray_.push_back(alarmtoneAttrs_);
        ringtoneAsset = results->GetNextObject();
    }
    if (alarmtoneAttrsArray_.empty()) {
        MEDIA_LOGE("GetAlarmToneAttrList: no alarmtone in the ringtone library!");
    }
    return alarmtoneAttrsArray_;
}

int32_t SystemSoundManagerImpl::OpenAlarmTone(const std::shared_ptr<AbilityRuntime::Context> &context,
    const std::string &uri)
{
    std::lock_guard<std::mutex> lock(uriMutex_);
    DataShare::DatashareBusinessError businessError;
    DataShare::DataSharePredicates queryPredicates;
    DataShare::DataSharePredicates queryPredicatesByUri;
    queryPredicatesByUri.EqualTo(RINGTONE_COLUMN_DATA, uri);
    auto resultSetByUri = g_dataShareHelper->Query(RINGTONEURI, queryPredicatesByUri, COLUMNS, &businessError);
    auto resultsByUri = make_unique<RingtoneFetchResult<RingtoneAsset>>(move(resultSetByUri));
    unique_ptr<RingtoneAsset> ringtoneAssetByUri = resultsByUri->GetFirstObject();
    if (ringtoneAssetByUri == nullptr) {
        MEDIA_LOGE("OpenAlarmTone: tone of uri is not in the ringtone library!");
        return ERROR;
    }
    queryPredicates.EqualTo(RINGTONE_COLUMN_TONE_TYPE, TONE_TYPE_ALARM);
    auto resultSet = g_dataShareHelper->Query(RINGTONEURI, queryPredicates, COLUMNS, &businessError);
    auto results = make_unique<RingtoneFetchResult<RingtoneAsset>>(move(resultSet));
    unique_ptr<RingtoneAsset> ringtoneAsset = results->GetFirstObject();
    while ((ringtoneAsset != nullptr) && (uri != ringtoneAsset->GetPath())) {
        ringtoneAsset = results->GetNextObject();
    }
    if (ringtoneAsset != nullptr) {
        string uriStr = RINGTONE_PATH_URI + RINGTONE_SLASH_CHAR + to_string(ringtoneAsset->GetId());
        Uri ofUri(uriStr);
        int32_t fd = g_dataShareHelper->OpenFile(ofUri, "rw");
        return fd > 0 ? fd : ERROR;
    }
    MEDIA_LOGE("OpenAlarmTone: tone of uri is not alarm!");
    return TYPEERROR;
}

int32_t SystemSoundManagerImpl::Close(const int32_t &fd)
{
    std::lock_guard<std::mutex> lock(uriMutex_);
    return close(fd);
}

std::string SystemSoundManagerImpl::AddCustomizedToneByExternalUri(
    const std::shared_ptr<AbilityRuntime::Context> &context, const std::shared_ptr<ToneAttrs> &toneAttrs,
    const std::string &externalUri)
{
    std::string fdHead = "fd://";
    std::string srcPath = externalUri;
    int32_t srcFd;
    if (srcPath.find(fdHead) != std::string::npos) {
        StrToInt(srcPath.substr(fdHead.size()), srcFd);
    } else {
        srcFd = open(srcPath.c_str(), O_RDONLY);
    }
    if (srcFd < 0) {
        MEDIA_LOGE("AddCustomizedTone: fd open error is %{public}s", strerror(errno));
        fdHead.clear();
        return fdHead;
    }
    return AddCustomizedToneByFd(context, toneAttrs, srcFd);
}

std::string SystemSoundManagerImpl::AddCustomizedToneByFd(const std::shared_ptr<AbilityRuntime::Context> &context,
    const std::shared_ptr<ToneAttrs> &toneAttrs, const int32_t &fd)
{
    return AddCustomizedToneByFdAndOffset(context, toneAttrs, fd, 0, INT_MAX);
}

void SystemSoundManagerImpl::GetCustomizedTone(const std::shared_ptr<ToneAttrs> &toneAttrs)
{
    displayName_ = toneAttrs->GetFileName();
    mimeType_ = "";
    for (const auto& type : RINGTONETYPE) {
        size_t found = displayName_.find("." + type);
        if (found != std::string::npos) {
            mimeType_ = type;
        }
    }
    if (mimeType_.empty()) {
        mimeType_ = RINGTONE_CONTAINER_TYPE_OGG;
        displayName_ = displayName_ + ".ogg";
    }
}

int32_t SystemSoundManagerImpl::AddCustomizedTone(const std::shared_ptr<ToneAttrs> &toneAttrs)
{
    int32_t category = -1;
    category = toneAttrs->GetCategory();
    GetCustomizedTone(toneAttrs);
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(RINGTONE_COLUMN_DISPLAY_NAME, static_cast<string>(displayName_));
    valuesBucket.Put(RINGTONE_COLUMN_TITLE, static_cast<string>(toneAttrs->GetTitle()));
    valuesBucket.Put(RINGTONE_COLUMN_MIME_TYPE, static_cast<string>(mimeType_));
    valuesBucket.Put(RINGTONE_COLUMN_SOURCE_TYPE, static_cast<int>(SOURCE_TYPE_CUSTOMISED));
    switch (category) {
        case TONE_CATEGORY_RINGTONE:
            toneAttrs->SetUri(RINGTONE_CUSTOMIZED_RINGTONE_PATH + RINGTONE_SLASH_CHAR + displayName_);
            valuesBucket.Put(RINGTONE_COLUMN_TONE_TYPE, static_cast<int>(TONE_TYPE_RINGTONE));
            valuesBucket.Put(RINGTONE_COLUMN_RING_TONE_TYPE, static_cast<int>(RING_TONE_TYPE_SIM_CARD_BOTH));
            break;
        case TONE_CATEGORY_TEXT_MESSAGE:
            toneAttrs->SetUri(RINGTONE_CUSTOMIZED_NOTIFICATIONS_PATH + RINGTONE_SLASH_CHAR + displayName_);
            valuesBucket.Put(RINGTONE_COLUMN_TONE_TYPE, static_cast<int>(TONE_TYPE_NOTIFICATION));
            valuesBucket.Put(RINGTONE_COLUMN_SHOT_TONE_TYPE, static_cast<int>(SHOT_TONE_TYPE_SIM_CARD_BOTH));
            break;
        case TONE_CATEGORY_NOTIFICATION:
            toneAttrs->SetUri(RINGTONE_CUSTOMIZED_NOTIFICATIONS_PATH + RINGTONE_SLASH_CHAR + displayName_);
            valuesBucket.Put(RINGTONE_COLUMN_TONE_TYPE, static_cast<int>(TONE_TYPE_NOTIFICATION));
            valuesBucket.Put(RINGTONE_COLUMN_NOTIFICATION_TONE_TYPE, static_cast<int>(NOTIFICATION_TONE_TYPE));
            break;
        case TONE_CATEGORY_ALARM:
            toneAttrs->SetUri(RINGTONE_CUSTOMIZED_ALARM_PATH + RINGTONE_SLASH_CHAR + displayName_);
            valuesBucket.Put(RINGTONE_COLUMN_TONE_TYPE, static_cast<int>(TONE_TYPE_ALARM));
            valuesBucket.Put(RINGTONE_COLUMN_ALARM_TONE_TYPE, static_cast<int>(ALARM_TONE_TYPE));
            break;
        default:
            break;
    }
    valuesBucket.Put(RINGTONE_COLUMN_DATA, static_cast<string>(toneAttrs->GetUri()));
    return g_dataShareHelper->Insert(RINGTONEURI, valuesBucket);
}

std::string SystemSoundManagerImpl::AddCustomizedToneByFdAndOffset(
    const std::shared_ptr<AbilityRuntime::Context> &context, const std::shared_ptr<ToneAttrs> &toneAttrs,
    const int32_t &fd, const int32_t &offset, const int32_t &length)
{
    std::string result = "TYPEERROR";
    if (toneAttrs->GetCustomizedType() != CUSTOMISED) {
        MEDIA_LOGE("AddCustomizedTone: the ringtone is not customized!");
        return result;
    }
    std::lock_guard<std::mutex> lock(uriMutex_);
    int32_t sert = AddCustomizedTone(toneAttrs);
    std::string dstPath = RINGTONE_PATH_URI + RINGTONE_SLASH_CHAR + to_string(sert);
    Uri ofUri(dstPath);
    int32_t srcFd = fd;
    lseek(srcFd, offset, SEEK_SET);
    int32_t dstFd = g_dataShareHelper->OpenFile(ofUri, "rw");
    if (srcFd < 0 || dstFd < 0 || offset < 0 || length < 0) {
        MEDIA_LOGE("AddCustomizedTone: open error is %{public}s", strerror(errno));
        result.clear();
        return result;
    }
    char buffer[4096];
    int32_t len = length;
    memset_s(buffer, sizeof(buffer), 0, sizeof(buffer));
    uint32_t bytesRead = 0;
    while ((bytesRead = read(srcFd, buffer, sizeof(buffer))) > 0 && len > 0) {
        uint32_t bytesWritten = write(dstFd, buffer, (bytesRead < len) ? bytesRead : len);
        memset_s(buffer, sizeof(buffer), 0, sizeof(buffer));
        len -= bytesWritten;
        if (bytesWritten == -1) {
            break;
        }
    }
    close(srcFd);
    close(dstFd);
    return toneAttrs->GetUri();
}

int32_t SystemSoundManagerImpl::RemoveCustomizedTone(
    const std::shared_ptr<AbilityRuntime::Context> &context, const std::string &uri)
{
    std::lock_guard<std::mutex> lock(uriMutex_);
    int32_t changedRows = TYPEERROR;
    DataShare::DatashareBusinessError businessError;
    DataShare::DataSharePredicates queryPredicates;
    queryPredicates.EqualTo(RINGTONE_COLUMN_DATA, uri);
    auto resultSet = g_dataShareHelper->Query(RINGTONEURI, queryPredicates, COLUMNS, &businessError);
    auto results = make_unique<RingtoneFetchResult<RingtoneAsset>>(move(resultSet));
    unique_ptr<RingtoneAsset> ringtoneAsset = results->GetFirstObject();
    if (ringtoneAsset == nullptr) {
        MEDIA_LOGE("RemoveCustomizedTone: tone of uri is not in the ringtone library!");
        return ERROR;
    }
    while ((ringtoneAsset != nullptr) &&
        (SOURCE_TYPE_CUSTOMISED != ringtoneAsset->GetSourceType())) {
        ringtoneAsset = results->GetNextObject();
    }
    if (ringtoneAsset != nullptr) {
        DataShare::DataSharePredicates deletePredicates;
        deletePredicates.SetWhereClause(RINGTONE_COLUMN_TONE_ID + " = ? ");
        deletePredicates.SetWhereArgs({to_string(ringtoneAsset->GetId())});
        changedRows = g_dataShareHelper->Delete(RINGTONEURI, deletePredicates);
    } else {
        MEDIA_LOGE("RemoveCustomizedTone: the ringtone is not customized!");
    }
    return changedRows;
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
