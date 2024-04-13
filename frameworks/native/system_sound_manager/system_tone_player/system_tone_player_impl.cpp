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
#include <thread>

#include "audio_info.h"

#include "media_log.h"
#include "media_errors.h"

using namespace std;
using namespace OHOS::AbilityRuntime;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SystemTonePlayer"};
}

namespace OHOS {
namespace Media {
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
    if (player_ != nullptr) {
        (void)player_->Release();
        player_ = nullptr;
    }
    callback_ = nullptr;
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
    std::string hapticUri = "";
    if (!muteHaptics_) {
        // Get the default haptic source uri.
        std::string defaultSystemToneUri = systemSoundMgr_.GetDefaultSystemToneUri(systemToneType_);
        if (defaultSystemToneUri == "") {
            MEDIA_LOGW("Default system tone uri is empty. Play system tone without vibration");
            muteHaptics_ = true;
        } else {
            // the size of "ogg" is 3 and the size of ".ogg" is 4.
            hapticUri = defaultSystemToneUri.replace(defaultSystemToneUri.find_last_of(".ogg") - 3, 4, ".json");
        }
    }

    sourceId_ = audioHapticManager_->RegisterSource(audioUri, hapticUri);
    CHECK_AND_RETURN_RET_LOG(sourceId_ != -1, MSERR_OPEN_FILE_FAILED,
        "Failed to register source for audio haptic manager");
    (void)audioHapticManager_->SetAudioLatencyMode(sourceId_, AUDIO_LATENCY_MODE_NORMAL);
    (void)audioHapticManager_->SetStreamUsage(sourceId_, AudioStandard::StreamUsage::STREAM_USAGE_NOTIFICATION);

    player_ = audioHapticManager_->CreatePlayer(sourceId_, {muteAudio_, muteHaptics_});
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr, MSERR_OPEN_FILE_FAILED,
        "Failed to create system tone player instance");
    int32_t result = player_->Prepare();
    CHECK_AND_RETURN_RET_LOG(result == MSERR_OK, result,
        "Failed to load source for system tone player");
    configuredUri_ = audioUri;

    if (callback_ == nullptr) {
        callback_ = std::make_shared<SystemTonePlayerCallback>(shared_from_this());
    }
    CHECK_AND_RETURN_RET_LOG(callback_ != nullptr, MSERR_OPEN_FILE_FAILED,
        "Failed to create system tone player callback object");
    (void)player_->SetAudioHapticPlayerCallback(callback_);

    systemToneState_ = SystemToneState::STATE_PREPARED;
    return MSERR_OK;
}

int32_t SystemTonePlayerImpl::Prepare()
{
    MEDIA_LOGI("Enter Prepare()");
    std::lock_guard<std::mutex> lock(systemTonePlayerMutex_);
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr, MSERR_INVALID_STATE, "System tone player instance is null");

    std::string systemToneUri = systemSoundMgr_.GetSystemToneUri(context_, systemToneType_);
    bool muteHaptics = GetMuteHapticsValue();
    if (!configuredUri_.empty() && configuredUri_ == systemToneUri && muteHaptics_ == muteHaptics) {
        MEDIA_LOGI("Prepare: The system tone uri has been loaded and the vibration mode is right. Return directly.");
        return MSERR_OK;
    }

    int32_t result = InitPlayer(systemToneUri);
    CHECK_AND_RETURN_RET_LOG(result == MSERR_OK, result, "Failed to initialize the audio haptic player!");
    systemToneState_ = SystemToneState::STATE_PREPARED;
    return result;
}

bool SystemTonePlayerImpl::GetMuteHapticsValue()
{
    bool muteHaptics = false;
    if (systemSoundMgr_.GetRingerMode() == AudioStandard::AudioRingerMode::RINGER_MODE_SILENT) {
        muteHaptics = true;
    }
    return muteHaptics;
}

void SystemTonePlayerImpl::NotifyEndOfStream()
{
    std::lock_guard<std::mutex> lock(systemTonePlayerMutex_);
    systemToneState_ = SystemToneState::STATE_STOPPED;
}

int32_t SystemTonePlayerImpl::Start()
{
    MEDIA_LOGI("Enter Start()");
    SystemToneOptions options = {false, false};
    return Start(options);
}

int32_t SystemTonePlayerImpl::Start(const SystemToneOptions &systemToneOptions)
{
    MEDIA_LOGI("Enter Start() with systemToneOptions: muteAudio %{public}d, muteHaptics %{public}d",
        systemToneOptions.muteAudio, systemToneOptions.muteHaptics);
    std::lock_guard<std::mutex> lock(systemTonePlayerMutex_);
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr, MSERR_INVALID_STATE, "System tone player instance is null");

    // to do: Use one Soundpool for all SystemTopePlayer
    if (systemToneOptions.muteAudio != muteAudio_ || systemToneOptions.muteHaptics != muteHaptics_) {
        // reset audio haptic player
    }




    int32_t streamID = -1;
    // if (!systemToneOptions.muteAudio) {
    //     PlayParams playParams {
    //         .loop = 0,
    //         .rate = 0, // default AudioRendererRate::RENDER_RATE_NORMAL
    //         .leftVolume = 1.0,
    //         .rightVolume = 1.0,
    //         .priority = 0,
    //         .parallelPlayFlag = false,
    //     };

    //     streamID = player_->Play(soundID_, playParams);
    // }
    // if (!systemToneOptions.muteHaptics &&
    //     systemSoundMgr_.GetRingerMode() != AudioStandard::AudioRingerMode::RINGER_MODE_SILENT) {
    //     (void)SystemSoundVibrator::StartVibrator(VibrationType::VIBRATION_SYSTEM_TONE);
    // }
    return streamID;
}

int32_t SystemTonePlayerImpl::Stop(const int32_t &streamID)
{
    MEDIA_LOGI("Enter Stop() with streamID %{public}d", streamID);
    std::lock_guard<std::mutex> lock(systemTonePlayerMutex_);
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr, MSERR_INVALID_STATE, "System tone player instance is null");

    // to do: streamId
    int32_t result = player_->Stop();
    CHECK_AND_RETURN_RET_LOG(result == MSERR_OK, result, "Error %{public}d to stop audio haptic player!", result);

    systemToneState_ = SystemToneState::STATE_STOPPED;

    // if (streamID > 0) {
    //     (void)player_->Stop(streamID);
    //     (void)SystemSoundVibrator::StopVibrator();
    // } else {
    //     MEDIA_LOGW("The streamID %{public}d is invalid!", streamID);
    // }

    return MSERR_OK;
}

int32_t SystemTonePlayerImpl::Release()
{
    MEDIA_LOGI("Enter Release()");

    std::lock_guard<std::mutex> lock(systemTonePlayerMutex_);
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr, MSERR_INVALID_STATE, "System tone player instance is null");

    int32_t result = player_->Release();
    CHECK_AND_RETURN_RET_LOG(result == MSERR_OK, result, "Error %{public}d to release audio haptic player!", result);

    systemToneState_ = SystemToneState::STATE_RELEASED;
    player_ = nullptr;
    callback_ = nullptr;

    return MSERR_OK;
}

std::string SystemTonePlayerImpl::GetTitle() const
{
    MEDIA_LOGI("Enter GetTitle()");
    std::string uri = systemSoundMgr_.GetSystemToneUri(context_, systemToneType_);
    return uri.substr(uri.find_last_of("/") + 1);
}

// Callback class symbols
SystemTonePlayerCallback::SystemTonePlayerCallback(std::shared_ptr<SystemTonePlayerImpl> systemTonePlayerImpl)
    : systemTonePlayerImpl_(systemTonePlayerImpl) {}

void SystemTonePlayerCallback::OnInterrupt(const AudioStandard::InterruptEvent &interruptEvent)
{
    // Audio interrupt callback is not supported for system tone player
    MEDIA_LOGI("OnInterrupt reported from audio haptic player.");
}
void SystemTonePlayerCallback::OnEndOfStream(void)
{
    MEDIA_LOGI("OnEndOfStream reported from audio haptic player.");
    std::shared_ptr<SystemTonePlayerImpl> player = systemTonePlayerImpl_.lock();
    if (player != nullptr) {
        player->NotifyEndOfStream();
    } else {
        MEDIA_LOGE("The system tone player has been released!");
    }
}
} // namesapce AudioStandard
} // namespace OHOS
