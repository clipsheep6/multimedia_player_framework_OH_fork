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

#include "playbin_async_action.h"
#include <chrono>
#include "media_errors.h"
#include "media_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayBinAsynAction"};
}

namespace OHOS {
namespace Media {
namespace PlayBin {
int32_t CombinationAction::Execute()
{
    if (subActions_.empty()) {
        MEDIA_LOGE("Failed to execute action: %{public}s, all sub actions have already been done!", name_.c_str());
        return MSERR_INVALID_OPERATION;
    }

    if (doingAction_ != nullptr) {
        MEDIA_LOGE("Failed to exeute action: %{public}s, currently a sub action is doing!", name_.c_str());
        return MSERR_INVALID_OPERATION;
    }

    doingAction_ = subActions_.front();
    return doingAction_->Execute();
}

void CombinationAction::HandleMessage(const InnerMessage &inMsg)
{
    DoHandleMessage(inMsg);

    if (doingAction_ == nullptr) {
        return;
    }

    doingAction_->HandleMessage(inMsg);
}

bool CombinationAction::IsAllDone()
{
    return subActions_.empty();
}

void CombinationAction::AppendAction(const std::shared_ptr<AsyncAction> &action)
{
    if (action == nullptr) {
        return;
    }

    subActions_.push_back(action);
}

void CombinationAction::EnqueueNextAction()
{
    if (subActions_.empty()) {
        MEDIA_LOGW("All sub actions have been done already for %{public}s", name_.c_str());
        return;
    }

    if (doingAction_ != nullptr) {
        MEDIA_LOGW("Failed to enqueue next action for %{public}s, currently a sub action is doing!", name_.c_str());
        return;
    }

    // re-enqueue this combination action to let the executor to call the Execute again.
    OnActionReEnqueue();
}

void CombinationAction::OnActionDone(const AsyncAction &action)
{
    if (doingAction_ == nullptr) {
        MEDIA_LOGE("Failed to done action for %{public}s, currently no sub action is doing!", name_.c_str());
        return;
    }

    if (doingAction_.get() != &action) {
        MEDIA_LOGE("Failed to done action for %{public}s, currently action mismatch!", name_.c_str());
        return;
    }

    doingAction_ = nullptr;
    subActions_.pop_front();

    // Notify the action executor, this combination action's sub action has done, need to transfer this
    // combination action to pending list temporarily. Then, the executor can execute other async action.
    NotifyActionDone();
}

void CombinationAction::OnActionCancel(const AsyncAction &action)
{
    if (doingAction_ == nullptr) {
        MEDIA_LOGE("Failed to cancel action for %{public}s, currently no sub action is doing!", name_.c_str());
        return;
    }

    if (doingAction_.get() != &action) {
        MEDIA_LOGE("Failed to done action for %{public}s, currently action mismatch!", name_.c_str());
        return;
    }

    doingAction_ = nullptr;
    // If a sub action cancelled, then all rest of sub actions should be cancelled.
    decltype(subActions_) tempList;
    tempList.swap(subActions_);

    NotifyActionCancel();
}

void CombinationAction::OnActionReEnqueue(const AsyncAction &action)
{
    (void)action;
    MEDIA_LOGE("Unsupported api !");
}
}
}
}