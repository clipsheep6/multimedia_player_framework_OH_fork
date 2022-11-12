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

#ifndef MEDIA_CLIENT_H
#define MEDIA_CLIENT_H

#include "i_media_service.h"
#include "i_standard_media_service.h"
#include "media_death_recipient.h"
#include "media_listener_stub.h"
#ifdef SUPPORT_RECORDER
#include "recorder_client.h"
#include "recorder_profiles_client.h"
#endif
#ifdef SUPPORT_PLAYER
#include "player_client.h"
#endif
#ifdef SUPPORT_CODEC
#include "avcodeclist_client.h"
#include "avcodec_client.h"
#endif
// #ifdef SUPPORT_METADATA
#include "avmetadatahelper_client.h"
// #endif
// #ifdef SUPPORT_MUXER
#include "avmuxer_client.h"
// #endif
#include "nocopyable.h"


namespace OHOS {
namespace Media {
class MediaClient : public IMediaService, public NoCopyable {
public:
    MediaClient() noexcept;
    ~MediaClient();
    
    /**
     * @brief
     * 
     * @tparam R IMediaStandardService
     * @tparam S Client
     * @param ability
     * @return std::shared_ptr<IMedia>
     */
    template<typename R, typename S>
    std::shared_ptr<IMedia> CreateService(IStandardMediaService::MediaSystemAbility ability);
    std::shared_ptr<IMedia> CreateMediaService(IStandardMediaService::MediaSystemAbility ability) override;
    int32_t DestroyMediaService(std::shared_ptr<IMedia> media, IStandardMediaService::MediaSystemAbility ability) override;
private:
    sptr<IStandardMediaService> GetMediaProxy();
    bool IsAlived();
    static void MediaServerDied(pid_t pid);
    void DoMediaServerDied();
    std::map<IStandardMediaService::MediaSystemAbility, std::list<std::shared_ptr<IMedia>>> mediaClientMap;
    sptr<IStandardMediaService> mediaProxy_ = nullptr;
    sptr<MediaListenerStub> listenerStub_ = nullptr;
    sptr<MediaDeathRecipient> deathRecipient_ = nullptr;
    std::mutex mutex_;
};

// template<typename T>
// int32_t MediaClient::DestroyMediaService(std::shared_ptr<T> media)
// {
    // std::lock_guard<std::mutex> lock(mutex_);
    // // CHECK_AND_RETURN_RET_LOG(media != nullptr, MSERR_NO_MEMORY, "input recorder is nullptr.");
    // if (std::is_same<T, IRecorderService>::value) {
    //     recorderClientList_.remove(media);
    // } else if(std::is_same<T, IPlayerService>::value) {
    //     playerClientList_.remove(media);
    // } else if(std::is_same<T, IRecorderProfilesService>::value) {
    //     recorderProfilesClientList_.remove(media);
    // } else if(std::is_same<T, IAVCodecListService>::value) {
    //     avCodecListClientList_.remove(media);
    // } else if(std::is_same<T, IAVCodecService>::value) {
    //     avCodecClientList_.remove(media);
    // } else if(std::is_same<T, IAVMetadataHelperService>::value) {
    //     avMetadataHelperClientList_.remove(media);
    // } 
    // else if(std::is_same<T, IAVMuxerService>::value) {
    //     avmuxerClientList_.remove(media);
    // }
//     return MSERR_OK;
// }

/**
 * @brief
 * 
 * @tparam T IMediaService
 * @tparam R IMediaStandardService
 * @tparam S Client
 * @param ability
 * @return std::shared_ptr<T>
 */
// template<typename T, typename R, typename S>
// std::shared_ptr<T> MediaClient::CreateService(IStandardMediaService::MediaSystemAbility ability)
// {
//     std::lock_guard<std::mutex> lock(mutex_);
//     if (!IsAlived()) {
//         // MEDIA_LOGE("media service does not exist.");
//         return nullptr;
//     }
//     sptr<IRemoteObject> object = mediaProxy_->GetSubSystemAbility(
//         ability, listenerStub_->AsObject());
//     // CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "player proxy object is nullptr.");
//     sptr<R> proxy = iface_cast<R>(object);
//     // CHECK_AND_RETURN_RET_LOG(proxy != nullptr, nullptr, "player proxy is nullptr.");
//     std::shared_ptr<S> media = S::Create(proxy);
//     // CHECK_AND_RETURN_RET_LOG(media != nullptr, nullptr, "failed to create player client.");
//     return media;
// }

// template<typename T>
// std::shared_ptr<T> MediaClient::CreateMediaService()
// {
//     return nullptr;
    // if (std::is_same<T, IPlayerService>::value) {
    //     auto player = CreateService<IPlayerService, IStandardPlayerService,
    //         PlayerClient>(IStandardMediaService::MediaSystemAbility::MEDIA_PLAYER);
    //     playerClientList_.push_back(player);
    //     return player;
    // } else if (std::is_same<T, IRecorderService>::value) {
    //     std::shared_ptr<IRecorderService> recorder = CreateService<IRecorderService, IStandardRecorderService,
    //         RecorderClient>(IStandardMediaService::MediaSystemAbility::MEDIA_RECORDER);
    //     recorderClientList_.push_back(recorder);
    //     return recorder;
    // } else if(std::is_same<T, IAVCodecListService>::value) {
    //     auto avCodecList = CreateService<IAVCodecListService, IStandardAVCodecListService,
    //     AVCodecListClient>(IStandardMediaService::MediaSystemAbility::MEDIA_RECORDER);
    //     avCodecListClientList_.push_back(avCodecList);
    //     return avCodecList;
    // } else if(std::is_same<T, IRecorderProfilesService>::value) {
    //     auto recorderProfiles = CreateService<IRecorderProfilesService, IStandardRecorderProfilesService,
    //     RecorderProfilesClient>(IStandardMediaService::MediaSystemAbility::RECORDER_PROFILES);
    //     recorderProfilesClientList_.push_back(recorderProfiles);
    //     return recorderProfiles;
    // } else if(std::is_same<T, IAVCodecService>::value) {
    //     auto avCodec = CreateService<IAVCodecService, IStandardAVCodecService,
    //     AVCodecClient>(IStandardMediaService::MediaSystemAbility::MEDIA_AVCODEC);
    //     avCodecClientList_.push_back(avCodec);
    //     return avCodec;
    // }
// }


} // namespace Media
} // namespace OHOS
#endif // MEDIA_CLIENT_H
