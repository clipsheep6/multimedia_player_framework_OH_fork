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

#include "seek_action.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SeekAction"};
}

namespace OHOS {
namespace Media {
namespace PlayBin {
int32_t SeekAction::Execute()
{
    PlayBinState currState = stateOperator_.GetPlayBinState();
    if (currState != PLAYBIN_STATE_PREPARED &&
        currState != PLAYBIN_STATE_PLAYING &&
        currState != PLAYBIN_STATE_PAUSED) {
        MEDIA_LOGE("Invalid state %{public}s to seek", StringifyPlayBinState(currState).data());
        return MSERR_INVALID_STATE;
    }

    MEDIA_LOGI("Begin seek to position %{public}" PRIi64 "us, mode: %{public}hhu", position_, mode_);
    return sender_.SendSeekEvent(position_, mode_);
}

void SeekAction::HandleMessage(const InnerMessage &inMsg)
{
    if (inMsg.type != INNER_MSG_ASYNC_DONE && inMsg.type != INNER_MSG_STATE_CHANGED) {
        return;
    }

    if (inMsg.type == INNER_MSG_ASYNC_DONE) {
        GstState currGstState = stateOperator_.GetGstState();
        if (currGstState < GST_STATE_PAUSED) {
            return;
        }

        MEDIA_LOGI("Success to seek to position %{public}" PRIi64 "", position_);
        NotifyActionDone();

        if (!IsInternalAction()) {
            sender_.SendMessage(InnerMessage { PLAYBIN_MSG_SEEK_DONE, position_ });
        }
        return;
    }

    if (inMsg.type == INNER_MSG_STATE_CHANGED) {
        if (inMsg.detail1 != GST_STATE_PAUSED || inMsg.detail2 != GST_STATE_PLAYING) {
            return;
        }

        PlayBinState currState = stateOperator_.GetPlayBinState();
        if (currState != PLAYBIN_STATE_PLAYING) {
            MEDIA_LOGW("Not seek in PLAYBIN_STATE_PLAYING, but got gst state change from PAUSED to PLAYING");
            return;
        }

        MEDIA_LOGI("Success to seek to position %{public}" PRIi64 "", position_);
        NotifyActionDone();

        if (!IsInternalAction()) {
            sender_.SendMessage(InnerMessage { PLAYBIN_MSG_SEEK_DONE, position_ });
        }

        return;
    }
}
}
}
}