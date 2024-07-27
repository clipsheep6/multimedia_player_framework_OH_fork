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

#include <sys/stat.h>

#include "media_log.h"
#include "media_errors.h"

using namespace std;
using namespace OHOS::AbilityRuntime;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO_NAPI, "RingtonePlayer"};
}

namespace OHOS {
namespace Media {
const float HIGH_VOL = 1.0f;
const float LOW_VOL = 0.0f;
const std::string AUDIO_FORMAT_STR = ".ogg";
const std::string HAPTIC_FORMAT_STR = ".json";

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

bool RingtonePlayerImpl::IsFileExisting(const std::string &fileUri)
{
    struct stat buffer;
    return (stat(fileUri.c_str(), &buffer) == 0);
}

std::string RingtonePlayerImpl::GetHapticUriForAudioUri(const std::string &audioUri)
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
        std::string defaultRingtoneUri = systemSoundMgr_.GetDefaultRingtoneUri(RINGTONE_TYPE_SIM_CARD_0);
        if (defaultRingtoneUri.length() > AUDIO_FORMAT_STR.length() &&
            defaultRingtoneUri.rfind(AUDIO_FORMAT_STR) == defaultRingtoneUri.length() - AUDIO_FORMAT_STR.length()) {
            // the end of default ringtone uri is ".ogg"
            hapticUri = defaultRingtoneUri;
            hapticUri.replace(hapticUri.rfind(AUDIO_FORMAT_STR), AUDIO_FORMAT_STR.length(), HAPTIC_FORMAT_STR);
        } else {
            MEDIA_LOGW("The default ringtone uri is invalid!");
        }
    }

    return hapticUri;
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

std::string RingtonePlayerImpl::ChangeUri(const std::string &audioUri)
{
    const std::string FDHEAD = "fd://";
    std::string ringtoneUri = audioUri;
    size_t found = audioUri.find(RINGTONE_CUSTOMIZED_BASE_PATH);
    if (found != std::string::npos) {
        std::shared_ptr<DataShare::DataShareHelper> dataShareHelper =
            CreateDataShareHelper(STORAGE_MANAGER_MANAGER_ID);
        CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, audioUri, "Failed to create dataShareHelper.");
        DataShare::DatashareBusinessError businessError;
        DataShare::DataSharePredicates queryPredicates;
        Uri ringtonePathUri(RINGTONE_PATH_URI);
        vector<string> columns = {{RINGTONE_COLUMN_TONE_ID}, {RINGTONE_COLUMN_DATA}};
        queryPredicates.EqualTo(RINGTONE_COLUMN_DATA, audioUri);
        auto resultSet = dataShareHelper->Query(ringtonePathUri, queryPredicates, columns, &businessError);
        auto results = make_unique<RingtoneFetchResult<RingtoneAsset>>(move(resultSet));
        unique_ptr<RingtoneAsset> ringtoneAsset = results->GetFirstObject();
        if (ringtoneAsset != nullptr) {
            string uriStr = RINGTONE_PATH_URI + RINGTONE_SLASH_CHAR + to_string(ringtoneAsset->GetId());
            Uri ofUri(uriStr);
            int32_t fd = dataShareHelper->OpenFile(ofUri, "r");
            if (fd > 0) {
                ringtoneUri = FDHEAD + to_string(fd);
            }
        }
        resultSet == nullptr ? : resultSet->Close();
        dataShareHelper->Release();
    }
    MEDIA_LOGI("RingtonePlayerImpl::ChangeUri ringtoneUri is %{public}s", ringtoneUri.c_str());
    return ringtoneUri;
}

void RingtonePlayerImpl::InitPlayer(std::string &audioUri)
{
    if (sourceId_ != -1) {
        (void)audioHapticManager_->UnregisterSource(sourceId_);
        sourceId_ = -1;
    }

    AudioHapticPlayerOptions options = {false, false};
    // Get the haptic file uri according to the audio file uri.
    std::string hapticUri = GetHapticUriForAudioUri(audioUri);
    if (hapticUri == "") {
        MEDIA_LOGW("haptic uri is empty. Play ringtone without vibration");
        options.muteHaptics = true;
    }

    sourceId_ = audioHapticManager_->RegisterSource(ChangeUri(audioUri), hapticUri);
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
    CHECK_AND_RETURN_RET_LOG(configuredUri_ != "", "", "Configured uri is null");
    return systemSoundMgr_.GetRingtoneTitle(configuredUri_);
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

void RingtonePlayerCallback::OnError(int32_t errorCode)
{
    MEDIA_LOGI("OnError from audio haptic player. errorCode %{public}d", errorCode);
}
} // namesapce AudioStandard
} // namespace OHOS
