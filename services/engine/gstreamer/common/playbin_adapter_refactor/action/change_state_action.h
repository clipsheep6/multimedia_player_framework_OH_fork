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

#ifndef CHANGE_STATE_ACTION_H
#define CHANGE_STATE_ACTION_H

#include "playbin_async_action.h"
#include "playbin_bridge.h"

namespace OHOS {
namespace Media {
namespace PlayBin {
class ChangeStateAction : public AsyncAction {
public:
    ChangeStateAction(StateOperator &stateOperator, PlayBinState targetState, bool internal)
        : AsyncAction(AsyncActionType::ACTION_CHANGE_STATE, internal),
          stateOperator_(stateOperator),
          targetState_(targetState)
    {}
    ~ChangeStateAction() = default;

    int32_t Execute() override;
    void HandleMessage(const InnerMessage &inMsg) override;

protected:
    PlayBinState targetState_;
    StateOperator &stateOperator_;
    GstState startGstState_ = GST_STATE_VOID_PENDING;
    GstState targetGstState_ = GST_STATE_VOID_PENDING;
    PlayBinState startState_ = PLAYBIN_STATE_BUTT;
};
}
}
}

#endif