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
            matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_BOOT_COMPLETED);
            matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_USER_SWITCHED);
            EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
            instance_ = std::make_shared<AccountSubscriber>(subscribeInfo);
            EventFwk::CommonEventManager::NewSubscribeCommonEvent(instance_);
        }
    }
    return instance_;
}

AccountSubscriber::AccountSubscriber(const EventFwk::CommonEventSubscribeInfo &subscribeInfo)
    : EventFwk::CommonEventSubscriber(subscribeInfo)
{
}

void AccountSubscriber::OnReceiveEvent(const EventFwk::CommonEventData &eventData)
{
    const AAFwk::Want& want = eventData.GetWant();
    std::string action = want.GetAction();
    MEDIA_LOGI("receive action %{public}s, isBootCompleted %{public}d", action.c_str(), isBootCompleted);
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_BOOT_COMPLETED && !isBootCompleted) {
        isBootCompleted = true;
        MEDIA_LOGI("set isBootCompleted true");
        return;
    }
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_USER_SWITCHED && isBootCompleted) {
        std::lock_guard<std::mutex> lock(vctMutex_);
        std::vector<CommonEventReceiver*>::iterator it = eventReceivers_.begin();
        for (; it != eventReceivers_.end(); it++) {
            if (*it) {
                (*it)->OnCommonEventReceived(action);
            }
        }
    }
}

void AccountSubscriber::RegisterCommonEventReceiver(CommonEventReceiver *receiver)
{
    MEDIA_LOGI("receiver is 0x%{public}06" PRIXPTR "", FAKE_POINTER(receiver));
    std::lock_guard<std::mutex> lock(vctMutex_);
    std::vector<CommonEventReceiver*>::iterator it = std::find(eventReceivers_.begin(), eventReceivers_.end(),
        receiver);
    if (it != eventReceivers_.end()) {
        MEDIA_LOGI("register fail, receiver already exists");
        return;
    }
    eventReceivers_.push_back(receiver);
}

void AccountSubscriber::UnregisterCommonEventReceiver(CommonEventReceiver *receiver)
{
    MEDIA_LOGI("receiver is 0x%{public}06" PRIXPTR "", FAKE_POINTER(receiver));
    std::lock_guard<std::mutex> lock(vctMutex_);
    std::vector<CommonEventReceiver*>::iterator it = std::find(eventReceivers_.begin(), eventReceivers_.end(),
        receiver);
    if (it == eventReceivers_.end()) {
        MEDIA_LOGI("unregister fail, cannot find receiver");
        return;
    }
    eventReceivers_.erase(it);
}
} // namespace Media
} // namespace OHOS