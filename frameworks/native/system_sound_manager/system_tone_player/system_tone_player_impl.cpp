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

#include "system_tone_player_impl.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <thread>

#include "audio_info.h"

#include "media_log.h"
#include "media_errors.h"

using namespace std;
using namespace OHOS::AbilityRuntime;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO_NAPI, "SystemTonePlayer"};
}

namespace OHOS {
namespace Media {
const std::string FDHEAD = "fd://";
const std::string AUDIO_FORMAT_STR = ".ogg";
const std::string HAPTIC_FORMAT_STR = ".json";
const int32_t MAX_STREAM_ID = 128;

SystemTonePlayerImpl::SystemTonePlayerImpl(const shared_ptr<Context> &context,
    SystemSoundManagerImpl &systemSoundMgr, SystemToneType systemToneType)
    : context_(context),
      systemSoundMgr_(systemSoundMgr),
      systemToneType_(systemToneType)
{
    audioHapticManager_ = AudioHapticManagerFactory::CreateAudioHapticManager();
    CHECK_AND_RETURN_LOG(audioHapticManager_ != nullptr, "Failed to get audio haptic manager");

    std::string systemToneUri = systemSoundMgr_.GetSystemToneUri(context_, systemToneType_);
    InitPlayer(systemToneUri);
}

SystemTonePlayerImpl::~SystemTonePlayerImpl()
{
    DeleteAllPlayer();
    audioHapticManager_->UnregisterSource(sourceId_);
}

int32_t SystemTonePlayerImpl::InitPlayer(const std::string &audioUri)
{
    MEDIA_LOGI("Enter InitPlayer() with audio uri %{public}s", audioUri.c_str());

    if (sourceId_ != -1) {
        (void)audioHapticManager_->UnregisterSource(sourceId_);
        sourceId_ = -1;
    }

    // Determine whether vibration is needed
    muteHaptics_ = GetMuteHapticsValue();
    // Get the haptic file uri according to the audio file uri.
    std::string hapticUri = GetHapticUriForAudioUri(audioUri);
    if (hapticUri == "") {
        MEDIA_LOGW("haptic uri is empty. Play system tone without vibration");
        muteHaptics_ = true;
    }

    sourceId_ = audioHapticManager_->RegisterSource(ChangeUri(audioUri), hapticUri);
    CHECK_AND_RETURN_RET_LOG(sourceId_ != -1, MSERR_OPEN_FILE_FAILED,
        "Failed to register source for audio haptic manager");
    (void)audioHapticManager_->SetAudioLatencyMode(sourceId_, AUDIO_LATENCY_MODE_NORMAL);
    (void)audioHapticManager_->SetStreamUsage(sourceId_, AudioStandard::StreamUsage::STREAM_USAGE_NOTIFICATION);

    configuredUri_ = audioUri;
    systemToneState_ = SystemToneState::STATE_NEW;
    return MSERR_OK;
}

int32_t SystemTonePlayerImpl::CreatePlayerWithOptions(const AudioHapticPlayerOptions &options)
{
    streamId_++;
    if (streamId_ > MAX_STREAM_ID) {
        streamId_ = 1;
        DeletePlayer(streamId_);
    }

    playerMap_[streamId_] = audioHapticManager_->CreatePlayer(sourceId_, options);
    CHECK_AND_RETURN_RET_LOG(playerMap_[streamId_] != nullptr, MSERR_OPEN_FILE_FAILED,
        "Failed to create system tone player instance");

    callbackMap_[streamId_] = std::make_shared<SystemTonePlayerCallback>(streamId_, shared_from_this());
    CHECK_AND_RETURN_RET_LOG(callbackMap_[streamId_] != nullptr, MSERR_OPEN_FILE_FAILED,
        "Failed to create system tone player callback object");
    (void)playerMap_[streamId_]->SetAudioHapticPlayerCallback(callbackMap_[streamId_]);

    int32_t result = playerMap_[streamId_]->Prepare();
    CHECK_AND_RETURN_RET_LOG(result == MSERR_OK, result,
        "Failed to prepare for system tone player: %{public}d", result);
    return MSERR_OK;
}

bool SystemTonePlayerImpl::GetMuteHapticsValue()
{
    bool muteHaptics = false;
    if (systemSoundMgr_.GetRingerMode() == AudioStandard::AudioRingerMode::RINGER_MODE_SILENT) {
        muteHaptics = true;
    }
    return muteHaptics;
}

std::string SystemTonePlayerImpl::GetHapticUriForAudioUri(const std::string &audioUri)
{
    std::string hapticUri = "";
    if (audioUri.length() > AUDIO_FORMAT_STR.length() &&
        audioUri.rfind(AUDIO_FORMAT_STR) == audioUri.length() - AUDIO_FORMAT_STR.length()) {
        // the end of audio uri is ".ogg"
        hapticUri = audioUri;
        hapticUri.replace(hapticUri.rfind(AUDIO_FORMAT_STR), AUDIO_FORMAT_STR.length(), HAPTIC_FORMAT_STR);
    }

    if (hapticUri == "" || !IsFileExisting(hapticUri)) {
        MEDIA_LOGW("Failed to find the vibration json file for audioUri. Use the default json file.");
        std::string defaultSystemToneUri = systemSoundMgr_.GetDefaultSystemToneUri(SYSTEM_TONE_TYPE_NOTIFICATION);
        if (defaultSystemToneUri.length() > AUDIO_FORMAT_STR.length() &&
            defaultSystemToneUri.rfind(AUDIO_FORMAT_STR) == defaultSystemToneUri.length() - AUDIO_FORMAT_STR.length()) {
            // the end of default system tone uri is ".ogg"
            hapticUri = defaultSystemToneUri;
            hapticUri.replace(hapticUri.rfind(AUDIO_FORMAT_STR), AUDIO_FORMAT_STR.length(), HAPTIC_FORMAT_STR);
        } else {
            MEDIA_LOGW("The default system tone uri is invalid!");
        }
    }

    return hapticUri;
}

bool SystemTonePlayerImpl::IsFileExisting(const std::string &fileUri)
{
    struct stat buffer;
    return (stat(fileUri.c_str(), &buffer) == 0);
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

std::string SystemTonePlayerImpl::ChangeUri(const std::string &uri)
{
    std::string systemtoneUri = uri;
    size_t found = uri.find(RINGTONE_CUSTOMIZED_BASE_PATH);
    if (found != std::string::npos) {
        std::shared_ptr<DataShare::DataShareHelper> dataShareHelper =
            CreateDataShareHelper(STORAGE_MANAGER_MANAGER_ID);
        CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, uri, "Failed to create dataShareHelper.");
        DataShare::DatashareBusinessError businessError;
        DataShare::DataSharePredicates queryPredicates;
        Uri ringtonePathUri(RINGTONE_PATH_URI);
        vector<string> columns = {{RINGTONE_COLUMN_TONE_ID}, {RINGTONE_COLUMN_DATA}};
        queryPredicates.EqualTo(RINGTONE_COLUMN_DATA, uri);
        auto resultSet = dataShareHelper->Query(ringtonePathUri, queryPredicates, columns, &businessError);
        auto results = make_unique<RingtoneFetchResult<RingtoneAsset>>(move(resultSet));
        unique_ptr<RingtoneAsset> ringtoneAsset = results->GetFirstObject();
        if (ringtoneAsset != nullptr) {
            string uriStr = RINGTONE_PATH_URI + RINGTONE_SLASH_CHAR + to_string(ringtoneAsset->GetId());
            Uri ofUri(uriStr);
            int32_t fd = dataShareHelper->OpenFile(ofUri, "r");
            if (fd > 0) {
                systemtoneUri = FDHEAD + to_string(fd);
            }
        }
        resultSet == nullptr ? : resultSet->Close();
        dataShareHelper->Release();
    }
    MEDIA_LOGI("SystemTonePlayerImpl::ChangeUri systemtoneUri is %{public}s", systemtoneUri.c_str());
    return systemtoneUri;
}

int32_t SystemTonePlayerImpl::Prepare()
{
    MEDIA_LOGI("Enter Prepare()");
    std::lock_guard<std::mutex> lock(systemTonePlayerMutex_);
    CHECK_AND_RETURN_RET_LOG(systemToneState_ != SystemToneState::STATE_RELEASED, MSERR_INVALID_STATE,
        "System tone player has been released!");

    std::string systemToneUri = systemSoundMgr_.GetSystemToneUri(context_, systemToneType_);
    if (!configuredUri_.empty() && configuredUri_ == systemToneUri) {
        MEDIA_LOGI("The right system tone uri has been registered. Return directly.");
        systemToneState_ = SystemToneState::STATE_PREPARED;
        return MSERR_OK;
    }

    // reload audio haptic player for system tone.
    int32_t result = InitPlayer(systemToneUri);
    CHECK_AND_RETURN_RET_LOG(result == MSERR_OK, result,
        "Failed to init player for system tone player: %{public}d", result);
    systemToneState_ = SystemToneState::STATE_PREPARED;
    return result;
}

int32_t SystemTonePlayerImpl::Start()
{
    MEDIA_LOGI("Enter Start()");
    SystemToneOptions systemToneOptions = {false, false};
    return Start(systemToneOptions);
}

int32_t SystemTonePlayerImpl::Start(const SystemToneOptions &systemToneOptions)
{
    MEDIA_LOGI("Enter Start() with systemToneOptions: muteAudio %{public}d, muteHaptics %{public}d",
        systemToneOptions.muteAudio, systemToneOptions.muteHaptics);
    std::lock_guard<std::mutex> lock(systemTonePlayerMutex_);
    CHECK_AND_RETURN_RET_LOG(systemToneState_ != SystemToneState::STATE_RELEASED, MSERR_INVALID_STATE,
        "System tone player has been released!");

    int32_t result = MSERR_OK;
    bool actualMuteHaptics = systemToneOptions.muteHaptics || muteHaptics_;
    AudioHapticPlayerOptions actualOptions = {systemToneOptions.muteAudio, actualMuteHaptics};
    result = CreatePlayerWithOptions(actualOptions);
    CHECK_AND_RETURN_RET_LOG(result == MSERR_OK, -1,
        "Failed to create audio haptic player: %{public}d", result);

    result = playerMap_[streamId_]->Start();
    CHECK_AND_RETURN_RET_LOG(result == MSERR_OK, -1,
        "Failed to start audio haptic player: %{public}d", result);
    return streamId_;
}

int32_t SystemTonePlayerImpl::Stop(const int32_t &streamId)
{
    MEDIA_LOGI("Enter Stop() with streamId %{public}d", streamId);
    std::lock_guard<std::mutex> lock(systemTonePlayerMutex_);
    CHECK_AND_RETURN_RET_LOG(systemToneState_ != SystemToneState::STATE_RELEASED, MSERR_INVALID_STATE,
        "System tone player has been released!");

    if (playerMap_.count(streamId) == 0) {
        MEDIA_LOGW("The stream has been stopped or the id %{public}d is invalid.", streamId);
        return MSERR_OK;
    }

    int32_t result = playerMap_[streamId]->Stop();
    DeletePlayer(streamId);
    CHECK_AND_RETURN_RET_LOG(result == MSERR_OK, MSERR_INVALID_OPERATION,
        "Failed to stop audio haptic player: %{public}d", result);
    return result;
}

int32_t SystemTonePlayerImpl::Release()
{
    MEDIA_LOGI("Enter Release()");
    std::lock_guard<std::mutex> lock(systemTonePlayerMutex_);
    if (systemToneState_ == SystemToneState::STATE_RELEASED) {
        MEDIA_LOGW("System tone player has been released!");
        return MSERR_OK;
    }
    DeleteAllPlayer();
    audioHapticManager_->UnregisterSource(sourceId_);
    systemToneState_ = SystemToneState::STATE_RELEASED;
    return MSERR_OK;
}

std::string SystemTonePlayerImpl::GetTitle() const
{
    MEDIA_LOGI("Enter GetTitle()");
    std::string uri = systemSoundMgr_.GetSystemToneUri(context_, systemToneType_);
    return uri.substr(uri.find_last_of("/") + 1);
}

void SystemTonePlayerImpl::DeletePlayer(const int32_t &streamId)
{
    MEDIA_LOGI("DeletePlayer for streamId %{public}d", streamId);
    if (playerMap_.count(streamId) > 0) {
        playerMap_[streamId]->Release();
        playerMap_.erase(streamId);
    }
    if (callbackMap_.count(streamId) > 0) {
        callbackMap_.erase(streamId);
    }
    MEDIA_LOGI("DeletePlayer. playerMap_.size() %{public}zu  callbackMap_.size() %{public}zu ",
        playerMap_.size(), callbackMap_.size());
}

void SystemTonePlayerImpl::DeleteAllPlayer()
{
    MEDIA_LOGI("Delete all audio haptic player!");
    for (auto iter = playerMap_.begin(); iter != playerMap_.end(); iter++) {
        iter->second->Release();
    }
    playerMap_.clear();
    callbackMap_.clear();
}

void SystemTonePlayerImpl::NotifyEndofStreamEvent(const int32_t &streamId)
{
    std::lock_guard<std::mutex> lock(systemTonePlayerMutex_);
    // onPlayFinished for a stream.
    DeletePlayer(streamId);
}

void SystemTonePlayerImpl::NotifyInterruptEvent(const int32_t &streamId,
    const AudioStandard::InterruptEvent &interruptEvent)
{
    MEDIA_LOGW("Interrupt event is not supported for system tone player!");
}

// Callback class symbols
SystemTonePlayerCallback::SystemTonePlayerCallback(int32_t streamId,
    std::shared_ptr<SystemTonePlayerImpl> systemTonePlayerImpl)
{
    streamId_ = streamId;
    systemTonePlayerImpl_ = systemTonePlayerImpl;
}

void SystemTonePlayerCallback::OnInterrupt(const AudioStandard::InterruptEvent &interruptEvent)
{
    MEDIA_LOGI("OnInterrupt from audio haptic player: hintTye %{public}d", interruptEvent.hintType);
    std::shared_ptr<SystemTonePlayerImpl> player = systemTonePlayerImpl_.lock();
    if (player == nullptr) {
        MEDIA_LOGE("The audio haptic player has been released.");
        return;
    }
    player->NotifyInterruptEvent(streamId_, interruptEvent);
}

void SystemTonePlayerCallback::OnEndOfStream(void)
{
    MEDIA_LOGI("OnEndOfStream from audio haptic player. StreamId %{public}d", streamId_);
    std::shared_ptr<SystemTonePlayerImpl> player = systemTonePlayerImpl_.lock();
    if (player == nullptr) {
        MEDIA_LOGE("The audio haptic player has been released.");
        return;
    }
    player->NotifyEndofStreamEvent(streamId_);
}

void SystemTonePlayerCallback::OnError(int32_t errorCode)
{
    MEDIA_LOGI("OnError from audio haptic player. errorCode %{public}d", errorCode);
}
} // namesapce AudioStandard
} // namespace OHOS
