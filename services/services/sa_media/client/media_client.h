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

#include <map>
#include "i_media_service.h"
#include "i_standard_media_service.h"
#include "media_death_recipient.h"
#include "media_listener_stub.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class MediaClient : public IMediaService, public NoCopyable {
public:
    MediaClient() noexcept;
    ~MediaClient();
    
    /**
     * @brief template CreateService
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
    std::map<IStandardMediaService::MediaSystemAbility, std::list<std::shared_ptr<IMedia>>> mediaClientMap_;
    sptr<IStandardMediaService> mediaProxy_ = nullptr;
    sptr<MediaListenerStub> listenerStub_ = nullptr;
    sptr<MediaDeathRecipient> deathRecipient_ = nullptr;
    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_CLIENT_H
