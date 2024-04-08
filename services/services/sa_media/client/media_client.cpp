/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "media_client.h"
#include "avmetadatahelper_client.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "ipc_skeleton.h"
#include "i_standard_monitor_service.h"
#include "monitor_client.h"
#ifdef SUPPORT_RECORDER
#include "i_standard_recorder_service.h"
#endif
#ifdef SUPPORT_PLAYER
#include "i_standard_player_service.h"
#endif
#ifdef SUPPORT_METADATA
#include "i_standard_avmetadatahelper_service.h"
#endif
#ifdef SUPPORT_SCREEN_CAPTURE
#include "i_standard_screen_capture_service.h"
#endif
#include "media_log.h"
#include "media_errors.h"
#include "player_xcollie.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaClient"};
}

namespace OHOS {
namespace Media {
static MediaClient g_mediaClientInstance;
IMediaService &MediaServiceFactory::GetInstance()
{
    return g_mediaClientInstance;
}

MediaClient::MediaClient() noexcept
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaClient::~MediaClient()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool MediaClient::IsAlived()
{
    if (mediaProxy_ == nullptr) {
        mediaProxy_ = GetMediaProxy();
    }

    return (mediaProxy_ != nullptr) ? true : false;
}

#ifdef SUPPORT_RECORDER
std::shared_ptr<IRecorderService> MediaClient::CreateRecorderService()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(IsAlived(), nullptr, "media service does not exist.");

    sptr<IRemoteObject> object = mediaProxy_->GetSubSystemAbility(
        IStandardMediaService::MediaSystemAbility::MEDIA_RECORDER, listenerStub_->AsObject());
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "recorder proxy object is nullptr.");

    sptr<IStandardRecorderService> recorderProxy = iface_cast<IStandardRecorderService>(object);
    CHECK_AND_RETURN_RET_LOG(recorderProxy != nullptr, nullptr, "recorder proxy is nullptr.");

    std::shared_ptr<RecorderClient> recorder = RecorderClient::Create(recorderProxy);
    CHECK_AND_RETURN_RET_LOG(recorder != nullptr, nullptr, "failed to create recorder client.");

    recorderClientList_.push_back(recorder);
    return recorder;
}

int32_t MediaClient::DestroyMediaProfileService(std::shared_ptr<IRecorderProfilesService> recorderProfiles)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorderProfiles != nullptr, MSERR_NO_MEMORY, "input recorderProfiles is nullptr.");
    recorderProfilesClientList_.remove(recorderProfiles);
    return MSERR_OK;
}

int32_t MediaClient::DestroyRecorderService(std::shared_ptr<IRecorderService> recorder)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(recorder != nullptr, MSERR_NO_MEMORY, "input recorder is nullptr.");
    recorderClientList_.remove(recorder);
    return MSERR_OK;
}

std::shared_ptr<IRecorderProfilesService> MediaClient::CreateRecorderProfilesService()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(IsAlived(), nullptr, "media service does not exist.");

    sptr<IRemoteObject> object = mediaProxy_->GetSubSystemAbility(
        IStandardMediaService::MediaSystemAbility::RECORDER_PROFILES, listenerStub_->AsObject());
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "recorderProfiles proxy object is nullptr.");

    sptr<IStandardRecorderProfilesService> recorderProfilesProxy = iface_cast<IStandardRecorderProfilesService>(object);
    CHECK_AND_RETURN_RET_LOG(recorderProfilesProxy != nullptr, nullptr, "recorderProfiles proxy is nullptr.");

    std::shared_ptr<RecorderProfilesClient> recorderProfiles = RecorderProfilesClient::Create(recorderProfilesProxy);
    CHECK_AND_RETURN_RET_LOG(recorderProfiles != nullptr, nullptr, "failed to create recorderProfiles client.");

    recorderProfilesClientList_.push_back(recorderProfiles);
    return recorderProfiles;
}
#endif

#ifdef SUPPORT_PLAYER
std::shared_ptr<IPlayerService> MediaClient::CreatePlayerService()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(IsAlived(), nullptr, "media service does not exist.");

    sptr<IRemoteObject> object = mediaProxy_->GetSubSystemAbility(
        IStandardMediaService::MediaSystemAbility::MEDIA_PLAYER, listenerStub_->AsObject());
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "player proxy object is nullptr.");

    sptr<IStandardPlayerService> playerProxy = iface_cast<IStandardPlayerService>(object);
    CHECK_AND_RETURN_RET_LOG(playerProxy != nullptr, nullptr, "player proxy is nullptr.");

    std::shared_ptr<PlayerClient> player = PlayerClient::Create(playerProxy);
    CHECK_AND_RETURN_RET_LOG(player != nullptr, nullptr, "failed to create player client.");

    playerClientList_.push_back(player);
    return player;
}

int32_t MediaClient::DestroyPlayerService(std::shared_ptr<IPlayerService> player)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(player != nullptr, MSERR_NO_MEMORY, "input player is nullptr.");
    playerClientList_.remove(player);
    return MSERR_OK;
}
#endif

#ifdef SUPPORT_METADATA
std::shared_ptr<IAVMetadataHelperService> MediaClient::CreateAVMetadataHelperService()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(IsAlived(), nullptr, "media service does not exist.");

    sptr<IRemoteObject> object = mediaProxy_->GetSubSystemAbility(
        IStandardMediaService::MediaSystemAbility::MEDIA_AVMETADATAHELPER, listenerStub_->AsObject());
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "avmetadatahelper proxy object is nullptr.");

    sptr<IStandardAVMetadataHelperService> avMetadataHelperProxy = iface_cast<IStandardAVMetadataHelperService>(object);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperProxy != nullptr, nullptr, "avmetadatahelper proxy is nullptr.");

    std::shared_ptr<AVMetadataHelperClient> avMetadataHelper = AVMetadataHelperClient::Create(avMetadataHelperProxy);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelper != nullptr, nullptr, "failed to create avmetadatahelper client.");

    avMetadataHelperClientList_.push_back(avMetadataHelper);
    return avMetadataHelper;
}

int32_t MediaClient::DestroyAVMetadataHelperService(std::shared_ptr<IAVMetadataHelperService> avMetadataHelper)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelper != nullptr, MSERR_NO_MEMORY,
        "input avmetadatahelper is nullptr.");
    avMetadataHelperClientList_.remove(avMetadataHelper);
    return MSERR_OK;
}
#endif

#ifdef SUPPORT_SCREEN_CAPTURE
std::shared_ptr<IScreenCaptureService> MediaClient::CreateScreenCaptureService()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(IsAlived(), nullptr, "media service does not exist.");

    sptr<IRemoteObject> object = mediaProxy_->GetSubSystemAbility(
        IStandardMediaService::MediaSystemAbility::MEDIA_SCREEN_CAPTURE, listenerStub_->AsObject());
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "screenCapture proxy object is nullptr.");

    sptr<IStandardScreenCaptureService> screenCaptureProxy = iface_cast<IStandardScreenCaptureService>(object);
    CHECK_AND_RETURN_RET_LOG(screenCaptureProxy != nullptr, nullptr, "screenCapture proxy is nullptr.");

    std::shared_ptr<ScreenCaptureClient> screenCapture = ScreenCaptureClient::Create(screenCaptureProxy);
    CHECK_AND_RETURN_RET_LOG(screenCapture != nullptr, nullptr, "failed to create screenCapture client.");

    screenCaptureClientList_.push_back(screenCapture);
    return screenCapture;
}

int32_t MediaClient::DestroyScreenCaptureService(std::shared_ptr<IScreenCaptureService> screenCapture)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(screenCapture != nullptr, MSERR_NO_MEMORY,
        "input screenCapture is nullptr.");
    screenCaptureClientList_.remove(screenCapture);
    return MSERR_OK;
}

std::shared_ptr<IScreenCaptureController> MediaClient::CreateScreenCaptureControllerClient()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGI("MediaClient::CreateScreenCaptureControllerClient() start");
    CHECK_AND_RETURN_RET_LOG(IsAlived(), nullptr, "media service does not exist.");

    sptr<IRemoteObject> object = mediaProxy_->GetSubSystemAbility(
        IStandardMediaService::MediaSystemAbility::MEDIA_SCREEN_CAPTURE_CONTROLLER, listenerStub_->AsObject());
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "screenCapture controller proxy object is nullptr.");

    sptr<IStandardScreenCaptureController> controllerProxy = iface_cast<IStandardScreenCaptureController>(object);
    CHECK_AND_RETURN_RET_LOG(controllerProxy != nullptr, nullptr, "controllerProxy is nullptr.");

    std::shared_ptr<ScreenCaptureControllerClient> controller = ScreenCaptureControllerClient::Create(controllerProxy);
    CHECK_AND_RETURN_RET_LOG(controller != nullptr, nullptr, "failed to create screenCapture controller.");

    screenCaptureControllerList_.push_back(controller);
    MEDIA_LOGI("MediaClient::CreateScreenCaptureControllerClient() end");
    return controller;
}

int32_t MediaClient::DestroyScreenCaptureControllerClient(std::shared_ptr<IScreenCaptureController> controller)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(controller != nullptr, MSERR_NO_MEMORY,
        "input screenCapture controller is nullptr.");
    screenCaptureControllerList_.remove(controller);
    return MSERR_OK;
}
#endif

sptr<IStandardMonitorService> MediaClient::GetMonitorProxy()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(IsAlived(), nullptr, "media service does not exist.");

    sptr<IRemoteObject> object = mediaProxy_->GetSubSystemAbility(
        IStandardMediaService::MediaSystemAbility::MEDIA_MONITOR, listenerStub_->AsObject());
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "monitor proxy object is nullptr.");

    sptr<IStandardMonitorService> monitorProxy = iface_cast<IStandardMonitorService>(object);
    CHECK_AND_RETURN_RET_LOG(monitorProxy != nullptr, nullptr, "monitor proxy is nullptr.");

    return monitorProxy;
}

sptr<IStandardMediaService> MediaClient::GetMediaProxy()
{
    MEDIA_LOGD("enter");
    sptr<ISystemAbilityManager> samgr = nullptr;
    samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_RET_LOG(samgr != nullptr, nullptr, "system ability manager is nullptr.");
    sptr<IRemoteObject> object = nullptr;
    object = samgr->GetSystemAbility(OHOS::PLAYER_DISTRIBUTED_SERVICE_ID);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "media object is nullptr.");

    mediaProxy_ = iface_cast<IStandardMediaService>(object);
    CHECK_AND_RETURN_RET_LOG(mediaProxy_ != nullptr, nullptr, "media proxy is nullptr.");

    pid_t pid = 0;
    deathRecipient_ = new(std::nothrow) MediaDeathRecipient(pid);
    CHECK_AND_RETURN_RET_LOG(deathRecipient_ != nullptr, nullptr, "failed to new MediaDeathRecipient.");

    deathRecipient_->SetNotifyCb(std::bind(&MediaClient::MediaServerDied, std::placeholders::_1));
    bool result = object->AddDeathRecipient(deathRecipient_);
    if (!result) {
        MEDIA_LOGE("failed to add deathRecipient");
        return nullptr;
    }

    listenerStub_ = new(std::nothrow) MediaListenerStub();
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, nullptr, "failed to new MediaListenerStub");
    return mediaProxy_;
}

void MediaClient::MediaServerDied(pid_t pid)
{
    MEDIA_LOGE("media server is died, pid:%{public}d!", pid);
    g_mediaClientInstance.DoMediaServerDied();
}

void MediaClient::AVPlayerServerDied()
{
#ifdef SUPPORT_PLAYER
    for (auto &it : playerClientList_) {
        auto player = std::static_pointer_cast<PlayerClient>(it);
        if (player != nullptr) {
            player->MediaServerDied();
        }
    }
#endif

#ifdef SUPPORT_METADATA
    for (auto &it : avMetadataHelperClientList_) {
        auto avMetadataHelper = std::static_pointer_cast<AVMetadataHelperClient>(it);
        if (avMetadataHelper != nullptr) {
            avMetadataHelper->MediaServerDied();
        }
    }
#endif
}

void MediaClient::DoMediaServerDied()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (mediaProxy_ != nullptr) {
        (void)mediaProxy_->AsObject()->RemoveDeathRecipient(deathRecipient_);
        mediaProxy_ = nullptr;
    }
    listenerStub_ = nullptr;
    deathRecipient_ = nullptr;

    std::shared_ptr<MonitorClient> monitor = MonitorClient::GetInstance();
    CHECK_AND_RETURN_LOG(monitor != nullptr, "Failed to get monitor Instance!");
    monitor->MediaServerDied();

    AVPlayerServerDied();
#ifdef SUPPORT_RECORDER
    for (auto &it : recorderClientList_) {
        auto recorder = std::static_pointer_cast<RecorderClient>(it);
        if (recorder != nullptr) {
            recorder->MediaServerDied();
        }
    }

    for (auto &it : recorderProfilesClientList_) {
        auto recorderProfilesClient = std::static_pointer_cast<RecorderProfilesClient>(it);
        if (recorderProfilesClient != nullptr) {
            recorderProfilesClient->MediaServerDied();
        }
    }
#endif

#ifdef SUPPORT_SCREEN_CAPTURE
    for (auto &it : screenCaptureClientList_) {
        auto screenCaptureClient = std::static_pointer_cast<ScreenCaptureClient>(it);
        if (screenCaptureClient != nullptr) {
            screenCaptureClient->MediaServerDied();
        }
    }
#endif
}
} // namespace Media
} // namespace OHOS
