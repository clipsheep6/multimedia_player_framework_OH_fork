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

#include "config_policy_utils.h"

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
const std::string DEFAULT_RINGTONE_PATH = "resource/media/audio/ringtones";

RingtonePlayerImpl::RingtonePlayerImpl(const shared_ptr<Context> &context,
    SystemSoundManagerImpl &sysSoundMgr, RingtoneType type)
    : volume_(HIGH_VOL),
      loop_(false),
      context_(context),
      systemSoundMgr_(sysSoundMgr),
      type_(type)
{
    InitPlayer();
    (void)Configure(volume_, loop_);
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

void RingtonePlayerImpl::InitPlayer()
{
    player_ = PlayerFactory::CreatePlayer();
    CHECK_AND_RETURN_LOG(player_ != nullptr, "Failed to create ringtone player instance");

    callback_ = std::make_shared<RingtonePlayerCallback>(*this);
    CHECK_AND_RETURN_LOG(callback_ != nullptr, "Failed to create callback object");

    player_->SetPlayerCallback(callback_);
    ringtoneState_ = STATE_NEW;
    configuredUri_ = "";
}

int32_t RingtonePlayerImpl::PrepareRingtonePlayer(bool isReInitNeeded)
{
    MEDIA_LOGI("RingtonePlayerImpl::%{public}s", __func__);
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr, MSERR_INVALID_VAL, "Ringtone player instance is null");

    // fetch uri from kvstore
    auto kvstoreUri = systemSoundMgr_.GetRingtoneUri(context_, type_);
    if (kvstoreUri.empty()) {
        // if kvstoreUri == "", try to use default path.
        isReInitNeeded = true;
    }

    // If uri is different from from configure uri, reset the player
    if (kvstoreUri != configuredUri_ || isReInitNeeded) {
        (void)player_->Reset();

        int32_t ret = player_->SetSource(kvstoreUri);
        if (ret != MSERR_OK) {
            // failed to set source, try to use default path.
            ret = ApplyDefaultRingtoneUri(kvstoreUri);
            CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Set source to default uri failed %{public}d", ret);
            systemSoundMgr_.SetRingtoneUri(context_, kvstoreUri, type_);
        }
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Set source failed %{public}d", ret);

        Format format;
        format.PutIntValue(PlayerKeys::CONTENT_TYPE, AudioStandard::CONTENT_TYPE_UNKNOWN);
        format.PutIntValue(PlayerKeys::STREAM_USAGE, AudioStandard::STREAM_USAGE_RINGTONE);
        ret = player_->SetParameter(format);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Set stream type to ring failed %{public}d", ret);

        ret = player_->PrepareAsync();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Prepare failed %{public}d", ret);

        configuredUri_ = kvstoreUri;
        ringtoneState_ = STATE_NEW;
    }

    return MSERR_OK;
}

int32_t RingtonePlayerImpl::ApplyDefaultRingtoneUri(std::string &defaultUri)
{
    // kvstoreUri == "", try to use default ringtone uri 1.
    int32_t ret = player_->SetSource(DEFAULT_RINGTONE_URI_1);
    if (ret == MSERR_OK) {
        defaultUri = DEFAULT_RINGTONE_URI_1;
        MEDIA_LOGI("ApplyDefaultRingtoneUri: Set source to default ringtone uri 1.");
        return ret;
    }

    // try to use default ringtone uri 2.
    ret = player_->SetSource(DEFAULT_RINGTONE_URI_2);
    if (ret == MSERR_OK) {
        defaultUri = DEFAULT_RINGTONE_URI_2;
        MEDIA_LOGI("ApplyDefaultRingtoneUri: Set source to default ringtone uri 2.");
        return ret;
    }

    return ret;
}

int32_t RingtonePlayerImpl::Configure(const float &volume, const bool &loop)
{
    MEDIA_LOGI("RingtonePlayerImpl::%{public}s", __func__);
    CHECK_AND_RETURN_RET_LOG(volume >= LOW_VOL && volume <= HIGH_VOL,
        MSERR_INVALID_VAL, "Volume level invalid");
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr && ringtoneState_ != STATE_INVALID, MSERR_INVALID_VAL, "no player_");

    volume_ = volume;
    loop_ = loop;

    if (ringtoneState_ != STATE_NEW) {
        (void)player_->SetVolume(volume_, volume_);
        (void)player_->SetLooping(loop_);
        if (ringtoneState_ == STATE_RUNNING && std::abs(volume_ - 0.0f) <= std::numeric_limits<float>::epsilon()) {
            (void)SystemSoundVibrator::StopVibrator();
        }
    }

    (void)PrepareRingtonePlayer(false);

    return MSERR_OK;
}

int32_t RingtonePlayerImpl::Start()
{
    MEDIA_LOGI("RingtonePlayerImpl::%{public}s", __func__);

    CHECK_AND_RETURN_RET_LOG(player_ != nullptr && ringtoneState_ != STATE_INVALID, MSERR_INVALID_VAL, "no player_");

    if (player_->IsPlaying() || isStartQueued_) {
        MEDIA_LOGE("Play in progress, cannot start now");
        return MSERR_START_FAILED;
    }

    // Player doesn't support play in stopped state. Hence reinitialise player for making start<-->stop to work
    if (ringtoneState_ == STATE_STOPPED) {
        (void)PrepareRingtonePlayer(true);
    } else {
        (void)PrepareRingtonePlayer(false);
    }

    if (ringtoneState_ == STATE_NEW) {
        MEDIA_LOGI("Start received before player preparing is finished");
        isStartQueued_ = true;
        return MSERR_OK;
    }

    auto ret = player_->Play();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_START_FAILED, "Start failed %{public}d", ret);
    if (systemSoundMgr_.GetRingerMode() != AudioStandard::AudioRingerMode::RINGER_MODE_SILENT && volume_ > 0.0f) {
        (void)SystemSoundVibrator::StartVibrator(VibrationType::VIBRATION_RINGTONE);
    }
    ringtoneState_ = STATE_RUNNING;

    return MSERR_OK;
}

int32_t RingtonePlayerImpl::Stop()
{
    MEDIA_LOGI("RingtonePlayerImpl::%{public}s", __func__);
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr && ringtoneState_ != STATE_INVALID, MSERR_INVALID_VAL, "no player_");

    if (ringtoneState_ != STATE_STOPPED && player_->IsPlaying()) {
        (void)player_->Stop();
        (void)SystemSoundVibrator::StopVibrator();
    }

    ringtoneState_ = STATE_STOPPED;
    isStartQueued_ = false;

    return MSERR_OK;
}

int32_t RingtonePlayerImpl::Release()
{
    MEDIA_LOGI("RingtonePlayerImpl::%{public}s player", __func__);

    if (player_ != nullptr) {
        (void)player_->Release();
    }
    (void)SystemSoundVibrator::StopVibrator();

    ringtoneState_ = STATE_RELEASED;
    player_ = nullptr;
    callback_ = nullptr;

    return MSERR_OK;
}

RingtoneState RingtonePlayerImpl::GetRingtoneState()
{
    MEDIA_LOGI("RingtonePlayerImpl::%{public}s", __func__);
    return ringtoneState_;
}

void RingtonePlayerImpl::SetPlayerState(RingtoneState ringtoneState)
{
    CHECK_AND_RETURN_LOG(player_ != nullptr, "Ringtone player instance is null");

    if (ringtoneState_ != RingtoneState::STATE_RELEASED) {
        ringtoneState_ = ringtoneState;
    }

    if (ringtoneState_ == RingtoneState::STATE_PREPARED) {
        MEDIA_LOGI("Player prepared callback received. loop:%{public}d volume:%{public}f", loop_, volume_);
        (void)player_->SetVolume(volume_, volume_);
        (void)player_->SetLooping(loop_);

        if (isStartQueued_) {
            auto ret = player_->Play();
            isStartQueued_ = false;
            CHECK_AND_RETURN_LOG(ret == MSERR_OK, "Play failed %{public}d", ret);
            ringtoneState_ = RingtoneState::STATE_RUNNING;
            if (systemSoundMgr_.GetRingerMode() != AudioStandard::AudioRingerMode::RINGER_MODE_SILENT &&
                volume_ > 0.0f) {
                (void)SystemSoundVibrator::StartVibrator(VibrationType::VIBRATION_RINGTONE);
            }
        }
    }
}

int32_t RingtonePlayerImpl::GetAudioRendererInfo(AudioStandard::AudioRendererInfo &rendererInfo) const
{
    MEDIA_LOGI("RingtonePlayerImpl::%{public}s", __func__);
    rendererInfo.contentType = AudioStandard::ContentType::CONTENT_TYPE_RINGTONE;
    rendererInfo.streamUsage = AudioStandard::StreamUsage::STREAM_USAGE_NOTIFICATION_RINGTONE;
    rendererInfo.rendererFlags = 0;
    return MSERR_OK;
}

std::string RingtonePlayerImpl::GetTitle()
{
    MEDIA_LOGI("RingtonePlayerImpl::%{public}s", __func__);
    std::string uri = systemSoundMgr_.GetRingtoneUri(context_, type_);
    return uri.substr(uri.find_last_of("/") + 1);
}
int32_t RingtonePlayerImpl::SetRingtonePlayerInterruptCallback(
    const std::shared_ptr<RingtonePlayerInterruptCallback> &interruptCallback)
{
    MEDIA_LOGI("RingtonePlayerImpl::%{public}s", __func__);
    interruptCallback_ = interruptCallback;
    return MSERR_OK;
}

void RingtonePlayerImpl::NotifyInterruptEvent(AudioStandard::InterruptEvent &interruptEvent)
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

void RingtonePlayerCallback::OnError(int32_t errorCode, const std::string &errorMsg)
{
    MEDIA_LOGE("Error reported from media server %{public}d", errorCode);
}

void RingtonePlayerCallback::OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    if (type == INFO_TYPE_STATE_CHANGE) {
        HandleStateChangeEvent(extra, infoBody);
    } else if (type == INFO_TYPE_INTERRUPT_EVENT) {
        HandleAudioInterruptEvent(extra, infoBody);
    } else {
        return;
    }
}

void RingtonePlayerCallback::HandleStateChangeEvent(int32_t extra, const Format &infoBody)
{
    MEDIA_LOGI("RingtonePlayerCallback::HandleStateChangeEvent");
    state_ = static_cast<PlayerStates>(extra);
    switch (state_) {
        case PLAYER_STATE_ERROR:
            ringtoneState_ = STATE_INVALID;
            break;
        case PLAYER_IDLE:
        case PLAYER_INITIALIZED:
        case PLAYER_PREPARING:
            ringtoneState_ = STATE_NEW;
            break;
        case PLAYER_PREPARED:
            ringtoneState_ = STATE_PREPARED;
            break;
        case PLAYER_STARTED:
            ringtoneState_ = STATE_RUNNING;
            break;
        case PLAYER_PAUSED:
            ringtoneState_ = STATE_PAUSED;
            break;
        case PLAYER_STOPPED:
        case PLAYER_PLAYBACK_COMPLETE:
            ringtoneState_ = STATE_STOPPED;
            break;
        default:
            break;
    }
    ringtonePlayerImpl_.SetPlayerState(ringtoneState_);
}

void RingtonePlayerCallback::HandleAudioInterruptEvent(int32_t extra, const Format &infoBody)
{
    MEDIA_LOGI("RingtonePlayerCallback::HandleAudioInterruptEvent");
    AudioStandard::InterruptEvent interruptEvent;
    int32_t eventTypeValue = 0;
    int32_t forceTypeValue = 0;
    int32_t hintTypeValue = 0;
    (void)infoBody.GetIntValue(PlayerKeys::AUDIO_INTERRUPT_TYPE, eventTypeValue);
    (void)infoBody.GetIntValue(PlayerKeys::AUDIO_INTERRUPT_FORCE, forceTypeValue);
    (void)infoBody.GetIntValue(PlayerKeys::AUDIO_INTERRUPT_HINT, hintTypeValue);
    interruptEvent.eventType = static_cast<AudioStandard::InterruptType>(eventTypeValue);
    interruptEvent.forceType = static_cast<AudioStandard::InterruptForceType>(forceTypeValue);
    interruptEvent.hintType = static_cast<AudioStandard::InterruptHint>(hintTypeValue);
    ringtonePlayerImpl_.NotifyInterruptEvent(interruptEvent);
}
} // namesapce AudioStandard
} // namespace OHOS
