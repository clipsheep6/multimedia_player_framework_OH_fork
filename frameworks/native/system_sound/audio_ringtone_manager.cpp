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

#include "audio_ringtone_manager.h"

#include "medialibrary_db_const.h"
#include "media_log.h"
#include "media_errors.h"

using namespace std;
using namespace OHOS::AbilityRuntime;
using namespace OHOS::NativeRdb;

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RingtoneSoundManager"};
}

namespace OHOS {
namespace Media {
unique_ptr<IRingtoneSoundManager> RingtoneFactory::CreateRingtoneManager()
{
    unique_ptr<RingtoneSoundManager> soundMgr = make_unique<RingtoneSoundManager>();
    CHECK_AND_RETURN_RET_LOG(soundMgr != nullptr, nullptr, "Failed to create sound manager object");

    return soundMgr;
}

RingtoneSoundManager::RingtoneSoundManager()
{
    LoadSystemSoundUriMap();
}

RingtoneSoundManager::~RingtoneSoundManager()
{
    if (abilityHelper_ != nullptr) {
        abilityHelper_->Release();
        abilityHelper_ = nullptr;
    }
}

void RingtoneSoundManager::LoadSystemSoundUriMap(void)
{
    if (!LoadUriFromKvStore(RINGTONE_TYPE_DEFAULT, RingtoneManagerConstant::RINGTONE_URI)) {
        MEDIA_LOGE("RingtoneSoundManager::LoadSystemSoundUriMap: cann't load uri for default ringtone");
    }
    if (!LoadUriFromKvStore(RINGTONE_TYPE_DEFAULT, RingtoneManagerConstant::NOTIFICATION_URI)) {
        MEDIA_LOGE("RingtoneSoundManager::LoadSystemSoundUriMap: cann't load uri for default notification");
    }
    if (!LoadUriFromKvStore(RINGTONE_TYPE_DEFAULT, RingtoneManagerConstant::ALARM_URI)) {
        MEDIA_LOGE("RingtoneSoundManager::LoadSystemSoundUriMap: cann't load uri for default alarm");
    }
    if (!LoadUriFromKvStore(RINGTONE_TYPE_MULTISIM, RingtoneManagerConstant::RINGTONE_URI)) {
        MEDIA_LOGE("RingtoneSoundManager::LoadSystemSoundUriMap: cann't load uri for multisim ringtone");
    }
    if (!LoadUriFromKvStore(RINGTONE_TYPE_MULTISIM, RingtoneManagerConstant::NOTIFICATION_URI)) {
        MEDIA_LOGE("RingtoneSoundManager::LoadSystemSoundUriMap: cann't load uri for multisim notification");
    }
    if (!LoadUriFromKvStore(RINGTONE_TYPE_MULTISIM, RingtoneManagerConstant::ALARM_URI)) {
        MEDIA_LOGE("RingtoneSoundManager::LoadSystemSoundUriMap: cann't load uri for multisim alarm");
    }
}

void RingtoneSoundManager::WriteUriToKvStore(RingtoneType ringtoneType, const std::string &systemSoundType,
    const std::string &uri)
{
    std::string key = GetKeyForRingtoneKvStore(ringtoneType, systemSoundType);
    MEDIA_LOGI("RingtoneSoundManager::WriteUriToKvStore ringtoneType %{public}d, %{public}s: %{public}s",
        ringtoneType, systemSoundType.c_str(), uri.c_str());
    int32_t result = AudioStandard::AudioSystemManager::GetInstance()->SetSystemSoundUri(key, uri);
    MEDIA_LOGI("RingtoneSoundManager::WriteUriToKvStore result: %{public}d", result);
}

bool RingtoneSoundManager::LoadUriFromKvStore(RingtoneType ringtoneType, const std::string &systemSoundType)
{
    std::string key = GetKeyForRingtoneKvStore(ringtoneType, systemSoundType);
    std::string uri = AudioStandard::AudioSystemManager::GetInstance()->GetSystemSoundUri(key);
    ringtoneUriMap_[ringtoneType][systemSoundType] = uri;
    return uri == "";
}

std::string RingtoneSoundManager::GetKeyForRingtoneKvStore(RingtoneType ringtoneType,
    const std::string &systemSoundType)
{
    switch (ringtoneType) {
        case RINGTONE_TYPE_DEFAULT:
            return systemSoundType + "_for_default_type";
        case RINGTONE_TYPE_MULTISIM:
            return systemSoundType + "_for_multisim_type";
        default:
            MEDIA_LOGE("[GetStreamNameByStreamType] ringtoneType: %{public}d is unavailable", ringtoneType);
            return "";
    }
}

int32_t RingtoneSoundManager::SetSystemRingtoneUri(const shared_ptr<Context> &context, const string &uri,
    RingtoneType type)
{
    MEDIA_LOGI("RingtoneSoundManager::%{public}s", __func__);
    CHECK_AND_RETURN_RET_LOG(type >= RINGTONE_TYPE_DEFAULT && type <= RINGTONE_TYPE_MULTISIM,
                             MSERR_INVALID_VAL, "invalid type");
    ringtoneUriMap_[type][RingtoneManagerConstant::RINGTONE_URI] = uri;
    WriteUriToKvStore(type, RingtoneManagerConstant::RINGTONE_URI, uri);
    return MSERR_OK;
}

int32_t RingtoneSoundManager::SetSystemNotificationUri(const shared_ptr<Context> &context, const string &uri)
{
    MEDIA_LOGI("RingtoneSoundManager::%{public}s", __func__);
    ringtoneUriMap_[RINGTONE_TYPE_DEFAULT][RingtoneManagerConstant::NOTIFICATION_URI] = uri;
    WriteUriToKvStore(RINGTONE_TYPE_DEFAULT, RingtoneManagerConstant::NOTIFICATION_URI, uri);
    return MSERR_OK;
}

int32_t RingtoneSoundManager::SetSystemAlarmUri(const shared_ptr<Context> &context, const string &uri)
{
    MEDIA_LOGI("RingtoneSoundManager::%{public}s", __func__);
    ringtoneUriMap_[RINGTONE_TYPE_DEFAULT][RingtoneManagerConstant::ALARM_URI] = uri;
    WriteUriToKvStore(RINGTONE_TYPE_DEFAULT, RingtoneManagerConstant::ALARM_URI, uri);
    return MSERR_OK;
}

string RingtoneSoundManager::GetSystemRingtoneUri(const shared_ptr<Context> &context, RingtoneType type)
{
    MEDIA_LOGI("RingtoneSoundManager::%{public}s", __func__);
    CHECK_AND_RETURN_RET_LOG(type >= RINGTONE_TYPE_DEFAULT && type <= RINGTONE_TYPE_MULTISIM, "", "invalid type");
    return ringtoneUriMap_[type][RingtoneManagerConstant::RINGTONE_URI];
}

string RingtoneSoundManager::GetSystemNotificationUri(const shared_ptr<Context> &context)
{
    MEDIA_LOGI("RingtoneSoundManager::%{public}s", __func__);
    return ringtoneUriMap_[RINGTONE_TYPE_DEFAULT][RingtoneManagerConstant::NOTIFICATION_URI];
}

string RingtoneSoundManager::GetSystemAlarmUri(const shared_ptr<Context> &context)
{
    MEDIA_LOGI("RingtoneSoundManager::%{public}s", __func__);
    return ringtoneUriMap_[RINGTONE_TYPE_DEFAULT][RingtoneManagerConstant::ALARM_URI];
}

int32_t RingtoneSoundManager::SetUri(const shared_ptr<Context> &context, const ValuesBucket &valueBucket,
    const std::string &operation)
{
    MEDIA_LOGI("RingtoneSoundManager::%{public}s, operation is %{public}s", __func__, operation.c_str());

    CreateDataAbilityHelper(context);
    CHECK_AND_RETURN_RET_LOG(abilityHelper_ != nullptr, MSERR_INVALID_VAL, "Helper is null, failed to set uri");

    Uri uri(MEDIALIBRARY_DATA_URI + "/" + RingtoneManagerConstant::MEDIA_KVSTOREOPRN + "/" + operation);

    int32_t result = 0;
    result = abilityHelper_->Insert(uri, valueBucket);
    if (result != SUCCESS) {
        MEDIA_LOGE("RingtoneSoundManager::insert ringtone uri failed");
    }
    MEDIA_LOGI("RingtoneSoundManager::SetUri::%{public}d", result);
    return result;
}

string RingtoneSoundManager::FetchUri(const shared_ptr<Context> &context, const std::string &operation)
{
    MEDIA_LOGI("RingtoneSoundManager::%{public}s, operation is %{public}s", __func__, operation.c_str());

    CreateDataAbilityHelper(context);
    CHECK_AND_RETURN_RET_LOG(abilityHelper_ != nullptr, "", "Helper is null, failed to retrieve uri");

    Uri uri(MEDIALIBRARY_DATA_URI + "/" + RingtoneManagerConstant::MEDIA_KVSTOREOPRN + "/" + operation);
    std::string result = abilityHelper_->GetType(uri);
    MEDIA_LOGI("RingtoneSoundManager::FetchUri: %{public}s", result.c_str());
    return result;
}

void RingtoneSoundManager::CreateDataAbilityHelper(const shared_ptr<Context> &context)
{
    if (abilityHelper_ == nullptr) {
        auto contextUri = make_unique<Uri>(MEDIALIBRARY_DATA_URI);
        CHECK_AND_RETURN_LOG(contextUri != nullptr, "failed to create context uri");

        MEDIA_LOGI("RingtoneSoundManager::CreateDataAbilityHelper::context: %{public}s, uri: %{public}s",
            context == nullptr ? "nullptr" : "available", contextUri == nullptr ? "nullptr" : "available");
        abilityHelper_ = AppExecFwk::DataAbilityHelper::Creator(context, move(contextUri));
        CHECK_AND_RETURN_LOG(abilityHelper_ != nullptr, "Unable to create data ability helper");
    }
}

shared_ptr<IRingtonePlayer> RingtoneSoundManager::GetRingtonePlayer(const shared_ptr<Context> &context,
    RingtoneType type)
{
    MEDIA_LOGI("RingtoneSoundManager::%{public}s, type %{public}d", __func__, type);
    CHECK_AND_RETURN_RET_LOG(type >= RINGTONE_TYPE_DEFAULT && type <= RINGTONE_TYPE_MULTISIM, nullptr, "invalid type");

    if (ringtonePlayer_[type] != nullptr && ringtonePlayer_[type]->GetRingtoneState() == STATE_RELEASED) {
        ringtonePlayer_[type] = nullptr;
    }

    if (ringtonePlayer_[type] == nullptr) {
        ringtonePlayer_[type] = make_shared<RingtonePlayer>(context, *this, type);
        CHECK_AND_RETURN_RET_LOG(ringtonePlayer_[type] != nullptr, nullptr, "Failed to create ringtone player object");
    }

    return ringtonePlayer_[type];
}

// Player class symbols
RingtonePlayer::RingtonePlayer(const shared_ptr<Context> &context, RingtoneSoundManager &audioMgr, RingtoneType type)
    :volume_(RingtoneManagerConstant::HIGH_VOL),
    loop_(false),
    context_(context),
    audioRingtoneMgr_(audioMgr),
    type_(type)
{
    InitPlayer();
    (void)Configure(volume_, loop_);
}

RingtonePlayer::~RingtonePlayer()
{
    if (player_ != nullptr) {
        player_->Release();
        player_ = nullptr;
        callback_ = nullptr;
    }
}

void RingtonePlayer::InitPlayer()
{
    player_ = PlayerFactory::CreatePlayer();
    CHECK_AND_RETURN_LOG(player_ != nullptr, "Failed to create ringtone player instance");

    callback_ = std::make_shared<RingtonePlayerCallback>(*this);
    CHECK_AND_RETURN_LOG(callback_ != nullptr, "Failed to create callback object");

    player_->SetPlayerCallback(callback_);
    ringtoneState_ = STATE_NEW;
    configuredUri_ = "";
}

int32_t RingtonePlayer::PrepareRingtonePlayer(bool isReInitNeeded)
{
    MEDIA_LOGI("RingtonePlayer::%{public}s", __func__);
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr, MSERR_INVALID_VAL, "Ringtone player instance is null");

    // fetch uri from kvstore
    auto kvstoreUri = audioRingtoneMgr_.GetSystemRingtoneUri(context_, type_);
    CHECK_AND_RETURN_RET_LOG(!kvstoreUri.empty(), MSERR_INVALID_VAL, "Failed to obtain ringtone uri for playing");

    // If uri is different from from configure uri, reset the player
    if (kvstoreUri != configuredUri_ || isReInitNeeded) {
        (void)player_->Reset();

        auto ret = player_->SetSource(kvstoreUri);
        CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Set source failed %{public}d", ret);

        Format format;
        format.PutIntValue(PlayerKeys::CONTENT_TYPE, AudioStandard::CONTENT_TYPE_RINGTONE);
        format.PutIntValue(PlayerKeys::STREAM_USAGE, AudioStandard::STREAM_USAGE_NOTIFICATION_RINGTONE);
        ret = player_->SetParameter(format);
        CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Set stream type to ring failed %{public}d", ret);

        ret = player_->PrepareAsync();
        CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Prepare failed %{public}d", ret);

        configuredUri_ = kvstoreUri;
        ringtoneState_ = STATE_NEW;
    }

    return SUCCESS;
}

int32_t RingtonePlayer::Configure(const float &volume, const bool &loop)
{
    MEDIA_LOGI("RingtonePlayer::%{public}s", __func__);
    CHECK_AND_RETURN_RET_LOG(volume >= RingtoneManagerConstant::LOW_VOL && volume <= RingtoneManagerConstant::HIGH_VOL,
        MSERR_INVALID_VAL, "Volume level invalid");
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr && ringtoneState_ != STATE_INVALID, MSERR_INVALID_VAL, "no player_");

    volume_ = volume;
    loop_ = loop;

    if (ringtoneState_ != STATE_NEW) {
        (void)player_->SetVolume(volume_, volume_);
        (void)player_->SetLooping(loop_);
    }

    (void)PrepareRingtonePlayer(false);

    return SUCCESS;
}

int32_t RingtonePlayer::Start()
{
    MEDIA_LOGI("RingtonePlayer::%{public}s", __func__);

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
        return SUCCESS;
    }

    auto ret = player_->Play();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, MSERR_START_FAILED, "Start failed %{public}d", ret);

    ringtoneState_ = STATE_RUNNING;

    return SUCCESS;
}

int32_t RingtonePlayer::Stop()
{
    MEDIA_LOGI("RingtonePlayer::%{public}s", __func__);
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr && ringtoneState_ != STATE_INVALID, MSERR_INVALID_VAL, "no player_");

    if (ringtoneState_ != STATE_STOPPED && player_->IsPlaying()) {
        (void)player_->Stop();
    }

    ringtoneState_ = STATE_STOPPED;
    isStartQueued_ = false;

    return SUCCESS;
}

int32_t RingtonePlayer::Release()
{
    MEDIA_LOGI("RingtonePlayer::%{public}s player", __func__);

    if (player_ != nullptr) {
        (void)player_->Release();
    }

    ringtoneState_ = STATE_RELEASED;
    player_ = nullptr;
    callback_ = nullptr;

    return SUCCESS;
}

RingtoneState RingtonePlayer::GetRingtoneState()
{
    MEDIA_LOGI("RingtonePlayer::%{public}s", __func__);
    return ringtoneState_;
}

void RingtonePlayer::SetPlayerState(RingtoneState ringtoneState)
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
            CHECK_AND_RETURN_LOG(ret == SUCCESS, "Play failed %{public}d", ret);
            ringtoneState_ = RingtoneState::STATE_RUNNING;
        }
    }
}

int32_t RingtonePlayer::GetAudioRendererInfo(AudioStandard::AudioRendererInfo &rendererInfo) const
{
    MEDIA_LOGI("RingtonePlayer::%{public}s", __func__);
    rendererInfo.contentType = AudioStandard::ContentType::CONTENT_TYPE_RINGTONE;
    rendererInfo.streamUsage = AudioStandard::StreamUsage::STREAM_USAGE_NOTIFICATION_RINGTONE;
    rendererInfo.rendererFlags = 0;
    return SUCCESS;
}

std::string RingtonePlayer::GetTitle()
{
    MEDIA_LOGI("RingtonePlayer::%{public}s", __func__);
    std::string uri = audioRingtoneMgr_.GetSystemRingtoneUri(context_, type_);
    return uri.substr(uri.find_last_of("/") + 1);
}
int32_t RingtonePlayer::SetRingtonePlayerInterruptCallback(
    const std::shared_ptr<RingtonePlayerInterruptCallback> &interruptCallback)
{
    MEDIA_LOGI("RingtonePlayer::%{public}s", __func__);
    interruptCallback_ = interruptCallback;
    return MSERR_OK;
}

void RingtonePlayer::NotifyInterruptEvent(AudioStandard::InterruptEvent &interruptEvent)
{
    if (interruptCallback_ != nullptr) {
        interruptCallback_->OnInterrupt(interruptEvent);
        MEDIA_LOGI("RingtonePlayer::NotifyInterruptEvent");
    } else {
        MEDIA_LOGE("RingtonePlayer::interruptCallback_ is nullptr");
    }
}

// Callback class symbols
RingtonePlayerCallback::RingtonePlayerCallback(RingtonePlayer &ringtonePlayer) : ringtonePlayer_(ringtonePlayer)
{}

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
    ringtonePlayer_.SetPlayerState(ringtoneState_);
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
    ringtonePlayer_.NotifyInterruptEvent(interruptEvent);
}
} // namesapce AudioStandard
} // namespace OHOS
