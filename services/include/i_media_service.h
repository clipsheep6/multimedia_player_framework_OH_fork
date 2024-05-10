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

#ifndef I_MEDIA_SERVICE_H
#define I_MEDIA_SERVICE_H

#include <memory>
#ifdef SUPPORT_RECORDER
#include "i_recorder_service.h"
#include "i_recorder_profiles_service.h"
#endif
#ifdef SUPPORT_PLAYER
#include "i_player_service.h"
#endif
#ifdef SUPPORT_METADATA
#include "i_avmetadatahelper_service.h"
#endif
#ifdef SUPPORT_SCREEN_CAPTURE
#include "i_screen_capture_service.h"
#include "i_screen_capture_controller.h"
#endif
#include "i_standard_monitor_service.h"

namespace OHOS {
namespace Media {
class IMediaService {
public:
    virtual ~IMediaService() = default;

#ifdef SUPPORT_RECORDER
    /**
     * @brief Create a recorder service.
     *
     * All recorder functions must be created and obtained first.
     *
     * @return Returns a valid pointer if the setting is successful;
     * @since 1.0
     * @version 1.0
     */
    virtual std::shared_ptr<IRecorderService> CreateRecorderService() = 0;

    /**
     * @brief Destroy a recorder service.
     *
     * call the API to destroy the recorder service.
     *
     * @param pointer to the recorder service.
     * @return Returns a valid pointer if the setting is successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t DestroyRecorderService(std::shared_ptr<IRecorderService> recorder) = 0;

    /**
     * @brief Create a mediaprofile service.
     *
     * All player functions must be created and obtained first.
     *
     * @return Returns a valid pointer if the setting is successful;
     * @since 3.2
     * @version 3.2
     */
    virtual std::shared_ptr<IRecorderProfilesService> CreateRecorderProfilesService() = 0;

    /**
     * @brief Destroy a mediaprofile service.
     *
     * call the API to destroy the mediaprofile service.
     *
     * @param pointer to the mediaprofile service.
     * @return Returns a valid pointer if the setting is successful;
     * @since 3.2
     * @version 3.2
     */
    virtual int32_t DestroyMediaProfileService(std::shared_ptr<IRecorderProfilesService> recorderProfiles) = 0;
#endif

#ifdef SUPPORT_PLAYER
    /**
     * @brief Create a player service.
     *
     * All player functions must be created and obtained first.
     *
     * @return Returns a valid pointer if the setting is successful;
     * @since 1.0
     * @version 1.0
     */
    virtual std::shared_ptr<IPlayerService> CreatePlayerService() = 0;

    /**
     * @brief Destroy a player service.
     *
     * call the API to destroy the player service.
     *
     * @param pointer to the player service.
     * @return Returns a valid pointer if the setting is successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t DestroyPlayerService(std::shared_ptr<IPlayerService> player) = 0;
#endif

#ifdef SUPPORT_METADATA
    /**
     * @brief Create an avmetadatahelper service.
     *
     * All player functions must be created and obtained first.
     *
     * @return Returns a valid pointer if the setting is successful;
     * @since 1.0
     * @version 1.0
     */
    virtual std::shared_ptr<IAVMetadataHelperService> CreateAVMetadataHelperService() = 0;

    /**
     * @brief Destroy a avmetadatahelper service.
     *
     * call the API to destroy the avmetadatahelper service.
     *
     * @param pointer to the avmetadatahelper service.
     * @return Returns a valid pointer if the setting is successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t DestroyAVMetadataHelperService(std::shared_ptr<IAVMetadataHelperService> avMetadataHelper) = 0;
#endif

#ifdef SUPPORT_SCREEN_CAPTURE
    /**
     * @brief Create an screenCaptureHelper service.
     *
     * All player functions must be created and obtained first.
     *
     * @return Returns a valid pointer if the setting is successful;
     * @since 1.0
     * @version 1.0
     */
    virtual std::shared_ptr<IScreenCaptureService> CreateScreenCaptureService() = 0;

    /**
     * @brief Destroy a screenCaptureHelper service.
     *
     * call the API to destroy the screenCaptureHelper service.
     *
     * @param pointer to the screenCaptureHelper service.
     * @return Returns a valid pointer if the setting is successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t DestroyScreenCaptureService(std::shared_ptr<IScreenCaptureService> screenCaptureHelper) = 0;

    /**
     * @brief Create an ScreenCaptureControllerClient service.
     *
     * All player functions must be created and obtained first.
     *
     * @return Returns a valid pointer if the setting is successful;
     * @since 1.0
     * @version 1.0
     */
    virtual std::shared_ptr<IScreenCaptureController> CreateScreenCaptureControllerClient() = 0;

    /**
     * @brief Destroy a ScreenCaptureControllerClient service.
     *
     * call the API to destroy the ScreenCaptureControllerClient service.
     *
     * @param pointer to the ScreenCaptureControllerClient service.
     * @return Returns a valid pointer if the setting is successful;
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t DestroyScreenCaptureControllerClient(std::shared_ptr<IScreenCaptureController> controller) = 0;
#endif

    /**
     * @brief Get an monitor proxy.
     *
     * To communicate with the server monitor, you must first obtain the monitor proxy.
     *
     * @return Returns a valid pointer if the setting is successful;
     * @since 1.0
     * @version 1.0
     */
    virtual sptr<IStandardMonitorService> GetMonitorProxy() = 0;
};

class __attribute__((visibility("default"))) MediaServiceFactory {
public:
    /**
     * @brief IMediaService singleton
     *
     * Create Recorder Service and Player Service Through the Media Service.
     *
     * @return Returns IMediaService singleton;
     * @since 1.0
     * @version 1.0
     */
    static IMediaService &GetInstance();
private:
    MediaServiceFactory() = delete;
    ~MediaServiceFactory() = delete;
};
} // namespace Media
} // namespace OHOS
#endif // I_MEDIA_SERVICE_H
