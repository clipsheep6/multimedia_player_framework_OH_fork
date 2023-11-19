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
const std::string DEFAULT_SYSTEM_TONE_URI_1 =
    "sys_prod/resource/media/audio/notifications/Rise.ogg";
const std::string DEFAULT_SYSTEM_TONE_URI_2 =
    "sys_prod/variant/region_comm/china/resource/media/audio/notifications/Rise.ogg";

SystemTonePlayerImpl::SystemTonePlayerImpl(const shared_ptr<Context> &context,
    SystemSoundManager &systemSoundMgr, SystemToneType systemToneType)
    : context_(context),
      systemSoundMgr_(systemSoundMgr),
      systemToneType_(systemToneType)
{
    InitPlayer();
}

SystemTonePlayerImpl::~SystemTonePlayerImpl()
{
    if (player_ != nullptr) {
        player_->Release();
        (void)SystemSoundVibrator::StopVibrator();
        player_ = nullptr;
    }
}

void SystemTonePlayerImpl::InitPlayer()
{
    AudioStandard::AudioRendererInfo audioRendererInfo;
    audioRendererInfo.contentType = AudioStandard::ContentType::CONTENT_TYPE_UNKNOWN;
    audioRendererInfo.streamUsage = AudioStandard::StreamUsage::STREAM_USAGE_NOTIFICATION;
    audioRendererInfo.rendererFlags = 1;

    player_ = SoundPoolFactory::CreateSoundPool(MAX_STREAMS, audioRendererInfo);
    CHECK_AND_RETURN_LOG(player_ != nullptr, "Failed to create system tone player instance");

    configuredUri_ = "";
}

int32_t SystemTonePlayerImpl::Prepare()
{
    MEDIA_LOGI("Enter Prepare()");
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr, MSERR_INVALID_STATE, "System tone player instance is null");

    int32_t soundID = -1;
    auto systemToneUri = systemSoundMgr_.GetSystemToneUri(context_, systemToneType_);
    if (systemToneUri.empty()) {
        // if systemToneUri == "", try to use default path.
        soundID = ApplyDefaultSystemToneUri(systemToneUri);
        if (soundID < 0) {
            MEDIA_LOGE("Prepare: Failed to load default system tone uri.");
            return MSERR_OPEN_FILE_FAILED;
        }
        systemSoundMgr_.SetSystemToneUri(context_, systemToneUri, systemToneType_);
        soundID_ = soundID;
        configuredUri_ = systemToneUri;
    }

    if (configuredUri_ == systemToneUri) {
        MEDIA_LOGI("Prepare: The system tone uri has been loaded. Return directly.");
        return MSERR_OK;
    }

    soundID = player_->Load(systemToneUri);
    if (soundID < 0) {
        MEDIA_LOGE("Prepare: Failed to load system tone uri.");
        return MSERR_OPEN_FILE_FAILED;
    }
    soundID_ = soundID;
    configuredUri_ = systemToneUri;

    return MSERR_OK;
}

int32_t SystemTonePlayerImpl::ApplyDefaultSystemToneUri(std::string &defaultUri)
{
    // systemToneUri == "", try to use default system tone uri 1.
    int32_t soundID = player_->Load(DEFAULT_SYSTEM_TONE_URI_1);
    if (soundID >= 0) {
        defaultUri = DEFAULT_SYSTEM_TONE_URI_1;
        MEDIA_LOGI("ApplyDefaultSystemToneUri: Set source to default system tone uri 1.");
        return soundID;
    }

    // try to use default system tone uri 2.
    soundID = player_->Load(DEFAULT_SYSTEM_TONE_URI_2);
    if (soundID >= 0) {
        defaultUri = DEFAULT_SYSTEM_TONE_URI_2;
        MEDIA_LOGI("ApplyDefaultSystemToneUri: Set source to default system tone uri 2.");
        return soundID;
    }

    return soundID;
}

int32_t SystemTonePlayerImpl::Start()
{
    MEDIA_LOGI("Enter Start()");
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
    (void)SystemSoundVibrator::StartVibrator(VibrationType::VIBRATION_SYSTEM_TONE);

    return streamID;
}

int32_t SystemTonePlayerImpl::Start(SystemToneOptions systemToneOptions)
{
    MEDIA_LOGI("Enter Start() with systemToneOptions: muteAudio %{public}d, muteHaptics %{public}d",
        systemToneOptions.muteAudio, systemToneOptions.muteHaptics);
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
    if (!systemToneOptions.muteHaptics) {
        (void)SystemSoundVibrator::StartVibrator(VibrationType::VIBRATION_SYSTEM_TONE);
    }
    return streamID;
}

int32_t SystemTonePlayerImpl::Stop(int32_t streamID)
{
    MEDIA_LOGI("Enter Stop() with streamID");
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr, MSERR_INVALID_STATE, "System tone player instance is null");

    (void)player_->Stop(streamID);

    (void)SystemSoundVibrator::StopVibrator();

    return MSERR_OK;
}

int32_t SystemTonePlayerImpl::Release()
{
    MEDIA_LOGI("Enter Release()");
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr, MSERR_INVALID_STATE, "System tone player instance is null");

    (void)player_->Release();
    (void)SystemSoundVibrator::StopVibrator();

    player_ = nullptr;

    return MSERR_OK;
}

std::string SystemTonePlayerImpl::GetTitle() const
{
    MEDIA_LOGI("Enter GetTitle()");
    std::string uri = systemSoundMgr_.GetSystemToneUri(context_, systemToneType_);
    return uri.substr(uri.find_last_of("/") + 1);
}
} // namesapce AudioStandard
} // namespace OHOS
