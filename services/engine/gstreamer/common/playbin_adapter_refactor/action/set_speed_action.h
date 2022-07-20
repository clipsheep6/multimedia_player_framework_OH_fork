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

#ifndef SET_SPEED_ACTION_H
#define SET_SPEED_ACTION_H

#include "playbin_async_action.h"
#include "playbin_bridge.h"

namespace OHOS {
namespace Media {
namespace PlayBin {
class SetSpeedAction : public AsyncAction {
public:
    SetSpeedAction(const StateOperator &stateOperator, EventMsgSender &sender, double speed)
        : AsyncAction(AsyncActionType::ACTION_SET_SPEED, false),
          stateOperator_(stateOperator),
          sender_(sender),
          speed_(speed)
    {}
    ~SetSpeedAction() = default;

    int32_t Execute() override;
    void HandleMessage(const InnerMessage &inMsg) override;

private:
    const StateOperator &stateOperator_;
    EventMsgSender &sender_;
    double speed_;
};
}
}
}

#endif