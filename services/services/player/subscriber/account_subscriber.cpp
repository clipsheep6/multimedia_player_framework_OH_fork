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

#include "account_subscriber.h"
#include "bundle_info.h"
#include "common_event_manager.h"
#include "want.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AccountSubscriber"};
std::shared_ptr<AccountSubscriber> AccountSubscriber::instance_ = nullptr;

std::shared_ptr<AccountSubscriber> AccountSubscriber::GetInstance()
{
    if (instance_ == nullptr) {
        static std::mutex instanceMutex;
        std::lock_guard<std::mutex> lock(instanceMutex);
        if (instance_ == nullptr) {
            EventFwk::MatchingSkills matchingSkills;
            matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_USER_BACKGROUND);
            EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
            instance_ = std::make_shared<AccountSubscriber>(subscribeInfo);
        }
    }
    return instance_;
}

AccountSubscriber::AccountSubscriber(const EventFwk::CommonEventSubscribeInfo &subscribeInfo)
    : EventFwk::CommonEventSubscriber(subscribeInfo)
{
    MEDIA_LOGI("create instance");
}

AccountSubscriber::~AccountSubscriber()
{
    MEDIA_LOGI("free instance");
}

void AccountSubscriber::OnReceiveEvent(const EventFwk::CommonEventData &eventData)
{
    const AAFwk::Want& want = eventData.GetWant();
    std::string action = want.GetAction();
    int32_t userId = eventData.GetCode();
    MEDIA_LOGI("receive action %{public}s, userId %{public}d", action.c_str(), userId);
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_USER_BACKGROUND) {
        std::lock_guard<std::mutex> lock(userMutex_);
        auto mapIt = userMap_.find(userId);
        if (mapIt == userMap_.end()) {
            return;
        }
        std::shared_ptr<CommonEventReceiver> receiver = nullptr;
        for (size_t i = 0; i < mapIt->second.size(); i++) {
            receiver = mapIt->second[i];
            if (receiver != nullptr) {
                receiver->OnCommonEventReceived(action);
            }
        }
    }
}

void AccountSubscriber::RegisterCommonEventReceiver(int32_t userId,
    const std::shared_ptr<CommonEventReceiver> &receiver)
{
    MEDIA_LOGI("receiver is 0x%{public}06" PRIXPTR ", userId %{public}d", FAKE_POINTER(receiver.get()), userId);
    if (userId < 0 || receiver == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(userMutex_);
    if (userMap_.empty()) {
        EventFwk::CommonEventManager::NewSubscribeCommonEvent(instance_);
        MEDIA_LOGI("first register receiver, SubscribeCommonEvent");
    }
    auto mapIt = userMap_.find(userId);
    if (mapIt == userMap_.end()) {
        std::vector<std::shared_ptr<CommonEventReceiver>> vct;
        vct.push_back(receiver);
        userMap_[userId] = vct;
        return;
    }
    auto receiverIt = std::find(mapIt->second.begin(), mapIt->second.end(),
        receiver);
    if (receiverIt != mapIt->second.end()) {
        MEDIA_LOGI("register fail, receiver already exists");
        return;
    }
    mapIt->second.push_back(receiver);
}

void AccountSubscriber::UnregisterCommonEventReceiver(int32_t userId,
    const std::shared_ptr<CommonEventReceiver> &receiver)
{
    MEDIA_LOGI("receiver is 0x%{public}06" PRIXPTR ", userId %{public}d", FAKE_POINTER(receiver.get()), userId);
    if (userId < 0 || receiver == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(userMutex_);
    auto mapIt = userMap_.find(userId);
    if (mapIt == userMap_.end()) {
        MEDIA_LOGI("unregister fail, cannot find userId");
        return;
    }
    auto receiverIt = std::find(mapIt->second.begin(), mapIt->second.end(),
        receiver);
    if (receiverIt == mapIt->second.end()) {
        MEDIA_LOGI("unregister fail, cannot find receiver");
        return;
    }
    mapIt->second.erase(receiverIt);
    if (mapIt->second.empty()) {
        userMap_.erase(mapIt);
        MEDIA_LOGI("remove userId %{public}d, map size %{public}u", userId,
            static_cast<int32_t>(userMap_.size()));
    }
    if (userMap_.empty()) {
        EventFwk::CommonEventManager::NewUnSubscribeCommonEvent(instance_);
        MEDIA_LOGI("user map empty, UnSubscribeCommonEvent");
    }
}
} // namespace Media
} // namespace OHOS