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
const int32_t MAX_STREAMS = 1; // ensure that only one system tone is playing.
const int32_t LOAD_WAIT_SECONDS = 2;
const std::string DEFAULT_SYSTEM_TONE_URI_1 =
    "sys_prod/resource/media/audio/notifications/Rise.ogg";
const std::string DEFAULT_SYSTEM_TONE_URI_2 =
    "sys_prod/variant/region_comm/china/resource/media/audio/notifications/Rise.ogg";

SystemTonePlayerImpl::SystemTonePlayerImpl(const shared_ptr<Context> &context,
    SystemSoundManagerImpl &systemSoundMgr, SystemToneType systemToneType)
    : context_(context),
      systemSoundMgr_(systemSoundMgr),
      systemToneType_(systemToneType)
{
    InitPlayer();
}

SystemTonePlayerImpl::~SystemTonePlayerImpl()
{
    if (fileDes_ != -1) {
        (void)close(fileDes_);
    }

    player_ = nullptr;
    callback_ = nullptr;
}

void SystemTonePlayerImpl::InitPlayer()
{
    MEDIA_LOGI("Enter InitPlayer()");

    AudioStandard::AudioRendererInfo audioRendererInfo;
    audioRendererInfo.contentType = AudioStandard::ContentType::CONTENT_TYPE_UNKNOWN;
    audioRendererInfo.streamUsage = AudioStandard::StreamUsage::STREAM_USAGE_NOTIFICATION;
    audioRendererInfo.rendererFlags = 0;

    player_ = SoundPoolFactory::CreateSoundPool(MAX_STREAMS, audioRendererInfo);
    CHECK_AND_RETURN_LOG(player_ != nullptr, "Failed to create system tone player instance");

    callback_ = std::make_shared<SystemTonePlayerCallback>(*this);
    CHECK_AND_RETURN_LOG(callback_ != nullptr, "Failed to create callback object");
    player_->SetSoundPoolCallback(callback_);

    configuredUri_ = "";
}

int32_t SystemTonePlayerImpl::Prepare()
{
    MEDIA_LOGI("Enter Prepare()");
    std::lock_guard<std::mutex> lock(systemTonePlayerMutex_);
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr, MSERR_INVALID_STATE, "System tone player instance is null");

    auto systemToneUri = systemSoundMgr_.GetSystemToneUri(context_, systemToneType_);
    if (!configuredUri_.empty() && configuredUri_ == systemToneUri) {
        MEDIA_LOGI("Prepare: The system tone uri has been loaded. Return directly.");
        return MSERR_OK;
    }

    if (soundID_ != -1) {
        (void)player_->Unload(soundID_);
        soundID_ = -1;
        loadCompleted_ = false;
    }
    if (fileDes_ != -1) {
        (void)close(fileDes_);
        fileDes_ = -1;
    }

    fileDes_ = open(systemToneUri.c_str(), O_RDONLY);
    if (fileDes_ == -1) {
        // open file failed, try to use default path.
        int32_t ret = ApplyDefaultSystemToneUri(systemToneUri);
        if (ret == MSERR_OK) {
            systemSoundMgr_.SetSystemToneUri(context_, systemToneUri, systemToneType_);
        } else {
            return ret;
        }
    }
    std::string uri = "fd://" + to_string(fileDes_);

    int32_t soundID = player_->Load(uri);
    if (soundID < 0) {
        MEDIA_LOGE("Prepare: Failed to load system tone uri.");
        return MSERR_OPEN_FILE_FAILED;
    }
    std::unique_lock<std::mutex> lockWait(loadUriMutex_);
    condLoadUri_.wait_for(lockWait, std::chrono::seconds(LOAD_WAIT_SECONDS),
        [this]() { return loadCompleted_ || isReleased_; });
    if (isReleased_) {
        MEDIA_LOGE("Prepare: The system tone player is released when it is preparing.");
        return MSERR_INVALID_OPERATION;
    }
    if (!loadCompleted_) {
        MEDIA_LOGE("Prepare: Failed to load system tone uri (time out).");
        return MSERR_OPEN_FILE_FAILED;
    }

    soundID_ = soundID;
    configuredUri_ = systemToneUri;

    return MSERR_OK;
}

int32_t SystemTonePlayerImpl::ApplyDefaultSystemToneUri(std::string &defaultUri)
{
    // systemToneUri == "", try to use default system tone uri 1.
    fileDes_ = open(DEFAULT_SYSTEM_TONE_URI_1.c_str(), O_RDONLY);
    if (fileDes_ != -1) {
        MEDIA_LOGI("ApplyDefaultSystemToneUri: Set source to default system tone uri 1.");
        defaultUri = DEFAULT_SYSTEM_TONE_URI_1;
        return MSERR_OK;
    }

    // try to use default system tone uri 2.
    fileDes_ = open(DEFAULT_SYSTEM_TONE_URI_2.c_str(), O_RDONLY);
    if (fileDes_ != -1) {
        MEDIA_LOGI("ApplyDefaultSystemToneUri: Set source to default system tone uri 2.");
        defaultUri = DEFAULT_SYSTEM_TONE_URI_2;
        return MSERR_OK;
    }

    return MSERR_OPEN_FILE_FAILED;
}

int32_t SystemTonePlayerImpl::NotifyLoadCompleted()
{
    std::lock_guard<std::mutex> lockWait(loadUriMutex_);
    loadCompleted_ = true;
    condLoadUri_.notify_one();
    return MSERR_OK;
}

int32_t SystemTonePlayerImpl::Start()
{
    MEDIA_LOGI("Enter Start()");
    std::lock_guard<std::mutex> lock(systemTonePlayerMutex_);
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr, MSERR_INVALID_STATE, "System tone player instance is null");

    PlayParams playParams {
        .loop = 0,
        .rate = 0, // default AudioRendererRate::RENDER_RATE_NORMAL
        .leftVolume = 1.0,
        .rightVolume = 1.0,
        .priority = 0,
        .parallelPlayFlag = false,
    };

    int32_t streamID = player_->Play(soundID_, playParams);
    if (systemSoundMgr_.GetRingerMode() != AudioStandard::AudioRingerMode::RINGER_MODE_SILENT) {
        (void)SystemSoundVibrator::StartVibrator(VibrationType::VIBRATION_SYSTEM_TONE);
    }

    return streamID;
}

int32_t SystemTonePlayerImpl::Start(const SystemToneOptions &systemToneOptions)
{
    MEDIA_LOGI("Enter Start() with systemToneOptions: muteAudio %{public}d, muteHaptics %{public}d",
        systemToneOptions.muteAudio, systemToneOptions.muteHaptics);
    std::lock_guard<std::mutex> lock(systemTonePlayerMutex_);
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr, MSERR_INVALID_STATE, "System tone player instance is null");

    int32_t streamID = -1;
    if (!systemToneOptions.muteAudio) {
        PlayParams playParams {
            .loop = 0,
            .rate = 0, // default AudioRendererRate::RENDER_RATE_NORMAL
            .leftVolume = 1.0,
            .rightVolume = 1.0,
            .priority = 0,
            .parallelPlayFlag = false,
        };

        streamID = player_->Play(soundID_, playParams);
    }
    if (!systemToneOptions.muteHaptics &&
        systemSoundMgr_.GetRingerMode() != AudioStandard::AudioRingerMode::RINGER_MODE_SILENT) {
        (void)SystemSoundVibrator::StartVibrator(VibrationType::VIBRATION_SYSTEM_TONE);
    }
    return streamID;
}

int32_t SystemTonePlayerImpl::Stop(const int32_t &streamID)
{
    MEDIA_LOGI("Enter Stop() with streamID %{public}d", streamID);
    std::lock_guard<std::mutex> lock(systemTonePlayerMutex_);
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr, MSERR_INVALID_STATE, "System tone player instance is null");

    if (streamID > 0) {
        (void)player_->Stop(streamID);
        (void)SystemSoundVibrator::StopVibrator();
    } else {
        MEDIA_LOGW("The streamID %{public}d is invalid!", streamID);
    }

    return MSERR_OK;
}

int32_t SystemTonePlayerImpl::Release()
{
    MEDIA_LOGI("Enter Release()");
    {
        std::lock_guard<std::mutex> lockWait(loadUriMutex_);
        isReleased_ = true;
        condLoadUri_.notify_all();
    }

    std::lock_guard<std::mutex> lock(systemTonePlayerMutex_);
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr, MSERR_INVALID_STATE, "System tone player instance is null");

    (void)player_->Unload(soundID_);
    soundID_ = -1;
    loadCompleted_ = false;
    if (fileDes_ != -1) {
        (void)close(fileDes_);
        fileDes_ = -1;
    }
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
SystemTonePlayerCallback::SystemTonePlayerCallback(SystemTonePlayerImpl &systemTonePlayerImpl)
    : systemTonePlayerImpl_(systemTonePlayerImpl) {}

void SystemTonePlayerCallback::OnLoadCompleted(int32_t soundId)
{
    MEDIA_LOGI("OnLoadCompleted reported from sound pool.");
    systemTonePlayerImpl_.NotifyLoadCompleted();
}

void SystemTonePlayerCallback::OnPlayFinished()
{
    MEDIA_LOGI("OnPlayFinished reported from sound pool.");
}

void SystemTonePlayerCallback::OnError(int32_t errorCode)
{
    MEDIA_LOGE("Error reported from sound pool: %{public}d", errorCode);
}
} // namesapce AudioStandard
} // namespace OHOS
