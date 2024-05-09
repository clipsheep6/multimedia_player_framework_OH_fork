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
#ifdef SUPPORT_METADATA
#include "avmetadatahelper_client.h"
#endif
#ifdef SUPPORT_SCREEN_CAPTURE
#include "screen_capture_client.h"
#include "screen_capture_controller_client.h"
#endif
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class MediaClient : public IMediaService, public NoCopyable {
public:
    MediaClient() noexcept;
    ~MediaClient();

    sptr<IStandardMonitorService> GetMonitorProxy() override;
#ifdef SUPPORT_RECORDER
    std::shared_ptr<IRecorderService> CreateRecorderService() override;
    int32_t DestroyRecorderService(std::shared_ptr<IRecorderService> recorder) override;
    std::shared_ptr<IRecorderProfilesService> CreateRecorderProfilesService() override;
    int32_t DestroyMediaProfileService(std::shared_ptr<IRecorderProfilesService> recorderProfiles) override;
#endif
#ifdef SUPPORT_PLAYER
    std::shared_ptr<IPlayerService> CreatePlayerService() override;
    int32_t DestroyPlayerService(std::shared_ptr<IPlayerService> player) override;
#endif
#ifdef SUPPORT_METADATA
    std::shared_ptr<IAVMetadataHelperService> CreateAVMetadataHelperService() override;
    int32_t DestroyAVMetadataHelperService(std::shared_ptr<IAVMetadataHelperService> avMetadataHelper) override;
#endif
#ifdef SUPPORT_SCREEN_CAPTURE
    std::shared_ptr<IScreenCaptureService> CreateScreenCaptureService() override;
    int32_t DestroyScreenCaptureService(std::shared_ptr<IScreenCaptureService> screenCapture) override;
    std::shared_ptr<IScreenCaptureController> CreateScreenCaptureControllerClient() override;
    int32_t DestroyScreenCaptureControllerClient(std::shared_ptr<IScreenCaptureController> controller) override;
#endif

private:
    sptr<IStandardMediaService> GetMediaProxy();
    bool IsAlived();
    static void MediaServerDied(pid_t pid);
    void DoMediaServerDied();
    void AVPlayerServerDied();
    void AVCodecServerDied();

    sptr<IStandardMediaService> mediaProxy_ = nullptr;
    sptr<MediaListenerStub> listenerStub_ = nullptr;
    sptr<MediaDeathRecipient> deathRecipient_ = nullptr;
#ifdef SUPPORT_RECORDER
    std::list<std::shared_ptr<IRecorderService>> recorderClientList_;
    std::list<std::shared_ptr<IRecorderProfilesService>> recorderProfilesClientList_;
#endif
#ifdef SUPPORT_PLAYER
    std::list<std::shared_ptr<IPlayerService>> playerClientList_;
#endif
#ifdef SUPPORT_METADATA
    std::list<std::shared_ptr<IAVMetadataHelperService>> avMetadataHelperClientList_;
#endif
#ifdef SUPPORT_SCREEN_CAPTURE
    std::list<std::shared_ptr<IScreenCaptureService>> screenCaptureClientList_;
    std::list<std::shared_ptr<IScreenCaptureController>> screenCaptureControllerList_;
#endif
    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_CLIENT_H
