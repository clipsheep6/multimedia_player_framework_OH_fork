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

#include "ringtone_player_impl.h"

#include "media_log.h"
#include "media_errors.h"

using namespace std;
using namespace OHOS::AbilityRuntime;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RingtonePlayer"};
}

namespace OHOS {
namespace Media {
const float HIGH_VOL = 1.0f;
const float LOW_VOL = 0.0f;

RingtonePlayerImpl::RingtonePlayerImpl(const shared_ptr<Context> &context,
    SystemSoundManagerImpl &sysSoundMgr, RingtoneType type)
    : volume_(HIGH_VOL),
      loop_(false),
      context_(context),
      systemSoundMgr_(sysSoundMgr),
      type_(type)
{
    audioHapticManager_ = AudioHapticManagerFactory::CreateAudioHapticManager();
    CHECK_AND_RETURN_LOG(audioHapticManager_ != nullptr, "Failed to get audio haptic manager");

    std::string ringtoneUri = systemSoundMgr_.GetRingtoneUri(context_, type_);
    InitPlayer(ringtoneUri);
}

RingtonePlayerImpl::~RingtonePlayerImpl()
{
    if (player_ != nullptr) {
        player_->Release();
        (void)SystemSoundVibrator::StopVibrator();
        player_ = nullptr;
        callback_ = nullptr;
    }
}

void RingtonePlayerImpl::InitPlayer(std::string &audioUri)
{
    if (sourceId_ != -1) {
        (void)audioHapticManager_->UnregisterSource(sourceId_);
        sourceId_ = -1;
    }

    AudioHapticPlayerOptions options = {false, false};
    // Get the default haptic source uri.
    std::string hapticUri = "";
    std::string defaultRingtoneUri = systemSoundMgr_.GetDefaultRingtoneUri(RINGTONE_TYPE_SIM_CARD_0);
    if (defaultRingtoneUri == "") {
        MEDIA_LOGW("Default ringtone uri is empty. Play ringtone without vibration");
        options.muteHaptics = true;
    } else {
        // the size of "ogg" is 3 and the size of ".ogg" is 4.
        hapticUri = defaultRingtoneUri.replace(defaultRingtoneUri.find_last_of(".ogg") - 3, 4, ".json");
    }

    sourceId_ = audioHapticManager_->RegisterSource(audioUri, hapticUri);
    CHECK_AND_RETURN_LOG(sourceId_ != -1, "Failed to register source for audio haptic manager");
    (void)audioHapticManager_->SetAudioLatencyMode(sourceId_, AUDIO_LATENCY_MODE_NORMAL);
    (void)audioHapticManager_->SetStreamUsage(sourceId_, AudioStandard::StreamUsage::STREAM_USAGE_VOICE_RINGTONE);

    if (systemSoundMgr_.GetRingerMode() == AudioStandard::AudioRingerMode::RINGER_MODE_SILENT) {
        options.muteHaptics = true;
    }
    player_ = audioHapticManager_->CreatePlayer(sourceId_, options);
    CHECK_AND_RETURN_LOG(player_ != nullptr, "Failed to create ringtone player instance");
    int32_t result = player_->Prepare();
    CHECK_AND_RETURN_LOG(result == MSERR_OK, "Failed to load source for audio haptic manager");
    configuredUri_ = audioUri;

    if (callback_ == nullptr) {
        callback_ = std::make_shared<RingtonePlayerCallback>(*this);
    }
    CHECK_AND_RETURN_LOG(callback_ != nullptr, "Failed to create callback object");
    (void)player_->SetAudioHapticPlayerCallback(callback_);
    (void)player_->SetVolume(volume_);
    (void)player_->SetLoop(loop_);

    ringtoneState_ = STATE_NEW;
}

int32_t RingtonePlayerImpl::Configure(const float &volume, const bool &loop)
{
    MEDIA_LOGI("RingtonePlayerImpl::Configure with volume %{public}f, loop %{public}d", volume, loop);
    CHECK_AND_RETURN_RET_LOG(volume >= LOW_VOL && volume <= HIGH_VOL,
        MSERR_INVALID_VAL, "Volume level invalid");

    std::lock_guard<std::mutex> lock(playerMutex_);
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr && ringtoneState_ != STATE_INVALID, MSERR_INVALID_VAL, "no player_");
    volume_ = volume;
    loop_ = loop;
    (void)player_->SetVolume(volume_);
    (void)player_->SetLoop(loop_);

    return MSERR_OK;
}

int32_t RingtonePlayerImpl::Start()
{
    MEDIA_LOGI("RingtonePlayerImpl::Start");
    std::lock_guard<std::mutex> lock(playerMutex_);
    CHECK_AND_RETURN_RET_LOG(ringtoneState_ != STATE_RUNNING, MSERR_INVALID_OPERATION, "ringtone player is running");
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr && ringtoneState_ != STATE_INVALID, MSERR_INVALID_VAL, "no player_");

    std::string ringtoneUri = systemSoundMgr_.GetRingtoneUri(context_, type_);
    if (ringtoneUri != configuredUri_) {
        MEDIA_LOGI("Ringtone uri changed. Reload player");
        InitPlayer(ringtoneUri);
    }
    int32_t ret = player_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_START_FAILED, "Start failed %{public}d", ret);
    ringtoneState_ = STATE_RUNNING;

    return MSERR_OK;
}

int32_t RingtonePlayerImpl::Stop()
{
    MEDIA_LOGI("RingtonePlayerImpl::Stop");
    std::lock_guard<std::mutex> lock(playerMutex_);
    CHECK_AND_RETURN_RET_LOG(ringtoneState_ != STATE_STOPPED, MSERR_INVALID_OPERATION,
        "ringtone player has been stopped");
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr && ringtoneState_ != STATE_INVALID, MSERR_INVALID_VAL, "no player_");

    (void)player_->Stop();
    ringtoneState_ = STATE_STOPPED;

    return MSERR_OK;
}

int32_t RingtonePlayerImpl::Release()
{
    MEDIA_LOGI("RingtonePlayerImpl::Release");
    std::lock_guard<std::mutex> lock(playerMutex_);
    CHECK_AND_RETURN_RET_LOG(ringtoneState_ != STATE_RELEASED, MSERR_INVALID_OPERATION,
        "ringtone player has been released");
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr && ringtoneState_ != STATE_INVALID, MSERR_INVALID_VAL, "no player_");

    (void)player_->Release();
    ringtoneState_ = STATE_RELEASED;
    player_ = nullptr;
    callback_ = nullptr;

    return MSERR_OK;
}

RingtoneState RingtonePlayerImpl::GetRingtoneState()
{
    MEDIA_LOGI("RingtonePlayerImpl::GetRingtoneState");
    std::lock_guard<std::mutex> lock(playerMutex_);
    return ringtoneState_;
}

int32_t RingtonePlayerImpl::GetAudioRendererInfo(AudioStandard::AudioRendererInfo &rendererInfo) const
{
    MEDIA_LOGI("RingtonePlayerImpl::GetAudioRendererInfo");
    rendererInfo.contentType = AudioStandard::ContentType::CONTENT_TYPE_UNKNOWN;
    rendererInfo.streamUsage = AudioStandard::StreamUsage::STREAM_USAGE_RINGTONE;
    rendererInfo.rendererFlags = 0;
    return MSERR_OK;
}

std::string RingtonePlayerImpl::GetTitle()
{
    MEDIA_LOGI("RingtonePlayerImpl::GetTitle");
    std::lock_guard<std::mutex> lock(playerMutex_);
    CHECK_AND_RETURN_RET_LOG(configuredUri_ != "", "", "Configured uri is null");
    std::string uri = configuredUri_;
    return uri.substr(uri.find_last_of("/") + 1);
}

int32_t RingtonePlayerImpl::SetRingtonePlayerInterruptCallback(
    const std::shared_ptr<RingtonePlayerInterruptCallback> &interruptCallback)
{
    MEDIA_LOGI("RingtonePlayerImpl::SetRingtonePlayerInterruptCallback");
    std::lock_guard<std::mutex> lock(playerMutex_);
    interruptCallback_ = interruptCallback;
    return MSERR_OK;
}

void RingtonePlayerImpl::NotifyEndofStreamEvent()
{
    std::lock_guard<std::mutex> lock(playerMutex_);
    ringtoneState_ = RingtoneState::STATE_STOPPED;
}

void RingtonePlayerImpl::NotifyInterruptEvent(const AudioStandard::InterruptEvent &interruptEvent)
{
    if (interruptCallback_ != nullptr) {
        interruptCallback_->OnInterrupt(interruptEvent);
        MEDIA_LOGI("RingtonePlayerImpl::NotifyInterruptEvent");
    } else {
        MEDIA_LOGE("RingtonePlayerImpl::interruptCallback_ is nullptr");
    }
}

// Callback class symbols
RingtonePlayerCallback::RingtonePlayerCallback(RingtonePlayerImpl &ringtonePlayerImpl)
    : ringtonePlayerImpl_(ringtonePlayerImpl) {}

void RingtonePlayerCallback::OnInterrupt(const AudioStandard::InterruptEvent &interruptEvent)
{
    MEDIA_LOGI("RingtonePlayerCallback::OnInterrupt: hintTye %{public}d", interruptEvent.hintType);
    ringtonePlayerImpl_.NotifyInterruptEvent(interruptEvent);
}

void RingtonePlayerCallback::OnEndOfStream(void)
{
    MEDIA_LOGI("RingtonePlayerCallback::OnEndOfStream");
    ringtonePlayerImpl_.NotifyEndofStreamEvent();
}
} // namesapce AudioStandard
} // namespace OHOS
