/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef MEDIA_ACCOUNT_SUBSCRIBER_H
#define MEDIA_ACCOUNT_SUBSCRIBER_H

#include "common_event_subscriber.h"
#include "common_event_support.h"
#include <vector>
#include <mutex>

namespace OHOS {
namespace Media {
class CommonEventReceiver {
public:
    virtual void OnCommonEventReceived(const std::string &event) {};
    virtual ~CommonEventReceiver() = default;
};

class AccountSubscriber : public EventFwk::CommonEventSubscriber,
                          public std::enable_shared_from_this<AccountSubscriber> {
public:
    explicit AccountSubscriber(const EventFwk::CommonEventSubscribeInfo &subscribeInfo);
    virtual ~AccountSubscriber();
    void OnReceiveEvent(const EventFwk::CommonEventData &eventData) override;
    void RegisterCommonEventReceiver(int32_t userId, const std::shared_ptr<CommonEventReceiver> &receiver);
    void UnregisterCommonEventReceiver(int32_t userId, const std::shared_ptr<CommonEventReceiver> &receiver);

    static std::shared_ptr<AccountSubscriber> GetInstance();
private:
    AccountSubscriber() = default;

    static std::shared_ptr<AccountSubscriber> instance_;
    std::mutex userMutex_;
    std::map<int32_t, std::vector<std::shared_ptr<CommonEventReceiver>>> userMap_;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_ACCOUNT_SUBSCRIBER_H