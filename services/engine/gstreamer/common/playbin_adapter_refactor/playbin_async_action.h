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

#ifndef PLAYBIN_ASYNC_ACTION_H
#define PLAYBIN_ASYNC_ACTION_H

#include <list>
#include <memory>
#include <string>
#include "playbin_msg_define.h"

namespace OHOS {
namespace Media {
namespace PlayBin {
enum AsyncActionType {
    ACTION_CHANGE_STATE,
    ACTION_SEEK,
    ACTION_SET_SPEED,
    ACTION_COMBINATION,
};

class ActionObserver {
public:
    virtual ~ActionObserver() = default;

    virtual void OnActionDone(const AsyncAction &action) = 0;
    virtual void OnActionCancel(const AsyncAction &action) = 0;
    // Currently, only valid for UnDone Combination Action
    virtual void OnActionReEnqueue(const AsyncAction &action) = 0;
};

class AsyncAction {
public:
    AsyncAction(AsyncActionType type, bool internal = false) : type_(type), isInternalAction_(internal)  {}
    virtual ~AsyncAction() = default;

    void SetObserver(const std::weak_ptr<ActionObserver>& observer)
    {
        observer_ = observer;
    }

    virtual int32_t Execute() = 0;
    virtual void HandleMessage(const InnerMessage &inMsg) = 0;

    AsyncActionType GetType() const
    {
        return type_;
    }

protected:
    bool IsInternalAction() const
    {
        return isInternalAction_;
    }

    void NotifyActionDone()
    {
        auto obs = observer_.lock();
        if (obs != nullptr) {
            obs->OnActionDone(*this);
        }
    }

    void NotifyActionCancel()
    {
        auto obs = observer_.lock();
        if (obs != nullptr) {
            obs->OnActionCancel(*this);
        }
    }

    void NotifyActionReEnqueue()
    {
        auto obs = observer_.lock();
        if (obs != nullptr) {
            obs->OnActionReEnqueue(*this);
        }
    }

private:
    const AsyncActionType type_;
    std::weak_ptr<ActionObserver> observer_;
    const bool isInternalAction_ = false;
};

class CombinationAction : public AsyncAction, public ActionObserver {
public:
    CombinationAction(const std::string &name)
        : AsyncAction(AsyncActionType::ACTION_COMBINATION, true)
    {}
    ~CombinationAction() = default;

    int32_t Execute() override final;
    void HandleMessage(const InnerMessage &inMsg) override final;
    bool IsAllDone();

protected:
    void AppendAction(const std::shared_ptr<AsyncAction> &action);
    void EnqueueNextAction();
    virtual void DoHandleMessage(const InnerMessage &inMsg)
    {
        (void)inMsg;
    }

private:
    void OnActionDone(const AsyncAction &action) override final;
    // 若取消该动作，则队列中后面的动作也全部取消，因为理论上不应该存在前一个动作没有执行，却需要执行后面动作的理由
    void OnActionCancel(const AsyncAction &action) override final;
    void OnActionReEnqueue(const AsyncAction &action) override final;

    std::list<std::shared_ptr<AsyncAction>> subActions_;
    std::shared_ptr<AsyncAction> doingAction_;
    const std::string name_;
};
}
}
}

#endif