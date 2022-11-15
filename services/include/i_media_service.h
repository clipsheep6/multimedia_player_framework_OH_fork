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
#include "../services/sa_media/ipc/i_standard_media_service.h"

namespace OHOS {
namespace Media {

class IMedia {
public:
    virtual void MediaServerDied() = 0;
    virtual ~IMedia() {}
};

class IMediaService {
public:
    virtual ~IMediaService() = default;

    virtual std::shared_ptr<IMedia> CreateMediaService(IStandardMediaService::MediaSystemAbility ability) = 0;

    virtual int32_t DestroyMediaService(std::shared_ptr<IMedia> mediaService, IStandardMediaService::MediaSystemAbility ability) = 0;
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
