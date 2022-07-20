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

#ifndef PLAYBIN_BRIDGE_H
#define PLAYBIN_BRIDGE_H

#include <gst/gst.h>
#include "i_playbin_ctrler.h"

namespace OHOS {
namespace Media {
namespace PlayBin {
class StateOperator {
public:
    virtual ~StateOperator() = default;
    virtual PlayBinState GetPlayBinState() const = 0;
    virtual int32_t SetPlayBinState(PlayBinState targetState) = 0;
    virtual GstState GetGstState() const = 0;
    virtual int32_t SetGstState(GstState targetState) = 0;
};

class EventMsgSender {
public:
    virtual ~EventMsgSender() = default;
    virtual int32_t SendSeekEvent(int64_t position, IPlayBinCtrler::SeekMode mode) = 0;
    virtual int32_t SendSpeedEvent(double rate) = 0;
    virtual void SendMessage(const InnerMessage &msg) = 0;
};
}
}
}

#endif