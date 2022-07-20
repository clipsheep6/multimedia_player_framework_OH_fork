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

#include "change_state_action.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ChangeStateAction"};
}

namespace OHOS {
namespace Media {
namespace PlayBin {
class PlayingToPauseAction : public ChangeStateAction {
public:
    PlayingToPauseAction(StateOperator &stateOperator)
        : ChangeStateAction(stateOperator, PLAYBIN_STATE_PAUSED, true)
    {}
    ~PlayingToPauseAction() = default;

    int32_t Execute() override
    {
        if (stateOperator_.GetPlayBinState() != PLAYBIN_STATE_PLAYING) {
            MEDIA_LOGI("Currently not at PLAYBIN_STATE_PLAYING, cancelled!");
            NotifyActionCancel();
            return MSERR_OK;
        }

        // 查询是否处于buffering状态，若是也需要取消动作执行

        return ChangeStateAction::Execute();
    }
};

class PauseToPlayingAction : public ChangeStateAction {
public:
    PauseToPlayingAction(StateOperator &stateOperator)
        : ChangeStateAction(stateOperator, PLAYBIN_STATE_PLAYING, true)
    {}
    ~PlayingToPauseAction() = default;

    int32_t Execute() override
    {
        if (stateOperator_.GetPlayBinState() != PLAYBIN_STATE_PLAYING) {
            MEDIA_LOGI("Currently not at PLAYBIN_STATE_PLAYING, cancelled!");
            NotifyActionCancel();
            return MSERR_OK;
        }

        return ChangeStateAction::Execute();
    }
};

void ProcessBufferingAction::Init()
{
    auto playingToPauseAction = std::make_shared<PlayingToPauseAction>(stateOperator_);
    auto pauseToPlayingAction = std::make_shared<PauseToPlayingAction>(stateOperator_);

    AppendAction(playingToPauseAction);
    AppendAction(pauseToPlayingAction);
}

void ProcessBufferingAction::DoHandleMessage(const InnerMessage &inMsg)
{
    (void)inMsg;
    // 检查是否收到了buffering 100%的消息，若是的话，则触发pauseToPlayingAction的执行
    // EnqueueNextAction();
}
}
}
}
