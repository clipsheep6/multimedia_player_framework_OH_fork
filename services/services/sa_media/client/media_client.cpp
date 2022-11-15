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
#ifdef SUPPORT_RECORDER
#include "i_standard_recorder_service.h"
#include "recorder_client.h"
#include "recorder_profiles_client.h"
#endif
#ifdef SUPPORT_PLAYER
#include "i_standard_player_service.h"
#include "player_client.h"
#endif
#ifdef SUPPORT_METADATA
#include "i_standard_avmetadatahelper_service.h"
#include "avmetadatahelper_client.h"
#endif
#ifdef SUPPORT_MUXER
#include "i_standard_avmuxer_service.h"
#include "avmuxer_client.h"
#endif
#ifdef SUPPORT_CODEC
#include "avcodeclist_client.h"
#include "avcodec_client.h"
#endif
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaClient"};
}

namespace OHOS {
namespace Media {
static MediaClient mediaClientInstance;
IMediaService &MediaServiceFactory::GetInstance()
{
    return mediaClientInstance;
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

template<typename R, typename S>
std::shared_ptr<IMedia> MediaClient::CreateService(IStandardMediaService::MediaSystemAbility ability)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!IsAlived()) {
        MEDIA_LOGE("media service does not exist.");
        return nullptr;
    }
    sptr<IRemoteObject> object = mediaProxy_->GetSubSystemAbility(
        ability, listenerStub_->AsObject());
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "player proxy object is nullptr.");
    sptr<R> proxy = iface_cast<R>(object);
    CHECK_AND_RETURN_RET_LOG(proxy != nullptr, nullptr, "player proxy is nullptr.");
    std::shared_ptr<S> media = S::Create(proxy);
    CHECK_AND_RETURN_RET_LOG(media != nullptr, nullptr, "failed to create player client.");
    return media;
}

std::shared_ptr<IMedia> MediaClient::CreateMediaService(IStandardMediaService::MediaSystemAbility ability)
{
    switch (ability)
    {
#ifdef SUPPORT_PLAYER
        case IStandardMediaService::MediaSystemAbility::MEDIA_PLAYER: {
            auto player = CreateService<IStandardPlayerService, PlayerClient>(ability);
            mediaClientMap_[ability].emplace_back(player);
            return player;
        }
#endif
#ifdef SUPPORT_RECORDER
        case IStandardMediaService::MediaSystemAbility::MEDIA_RECORDER: {
            auto recorder = CreateService<IStandardRecorderService, RecorderClient>(ability);
            mediaClientMap_[ability].emplace_back(recorder);
            return recorder;
        }
        case IStandardMediaService::MediaSystemAbility::RECORDER_PROFILES: {
            auto recorderProfiles = CreateService<IStandardRecorderProfilesService, RecorderProfilesClient>(ability);
            mediaClientMap_[ability].emplace_back(recorderProfiles);
            return recorderProfiles;
        }
#endif
#ifdef SUPPORT_CODEC
        case IStandardMediaService::MediaSystemAbility::MEDIA_AVCODEC: {
            auto avCodec = CreateService<IStandardAVCodecService, AVCodecClient>(ability);
            mediaClientMap_[ability].emplace_back(avCodec);
            return avCodec;
        }
        case IStandardMediaService::MediaSystemAbility::MEDIA_CODECLIST: {
            auto codecList = CreateService<IStandardAVCodecListService, AVCodecListClient>(ability);
            mediaClientMap_[ability].emplace_back(codecList);
            return codecList;
        }
#endif
#ifdef SUPPORT_MUXER
        case IStandardMediaService::MediaSystemAbility::MEDIA_AVMUXER: {
            auto muxer = CreateService<IStandardAVMuxerService, AVMuxerClient>(ability);
            mediaClientMap_[ability].emplace_back(muxer);
            return muxer;
        }
#endif
#ifdef SUPPORT_METADATA
        case IStandardMediaService::MediaSystemAbility::MEDIA_AVMETADATAHELPER: {
            auto metaData = CreateService<IStandardAVMetadataHelperService, AVMetadataHelperClient>(ability);
            mediaClientMap_[ability].emplace_back(metaData);
            return metaData;
        }
#endif
    default:
        return nullptr;
    }
}

int32_t MediaClient::DestroyMediaService(std::shared_ptr<IMedia> media, IStandardMediaService::MediaSystemAbility ability)
{
    mediaClientMap_[ability].remove(media);
    return MSERR_OK;
}

sptr<IStandardMediaService> MediaClient::GetMediaProxy()
{
    MEDIA_LOGD("enter");
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_RET_LOG(samgr != nullptr, nullptr, "system ability manager is nullptr.");

    sptr<IRemoteObject> object = samgr->GetSystemAbility(OHOS::PLAYER_DISTRIBUTED_SERVICE_ID);
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
    mediaClientInstance.DoMediaServerDied();
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

    for (const auto &it : mediaClientMap_) {
        for (const auto &clientList : it.second) {
            clientList->MediaServerDied();
        }
    }
}
} // namespace Media
} // namespace OHOS
