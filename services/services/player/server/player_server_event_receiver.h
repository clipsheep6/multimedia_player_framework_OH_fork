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

#ifndef PLAYER_SERVER_EVENT_RECEIVER_H
#define PLAYER_SERVER_EVENT_RECEIVER_H

#include "account_subscriber.h"
#include "player_server.h"

namespace OHOS {
namespace Media {

class PlayerServer;

class PlayerServerCommonEventReceiver : public CommonEventReceiver {
public:
    explicit PlayerServerCommonEventReceiver(const std::weak_ptr<PlayerServer>& server);
    ~PlayerServerCommonEventReceiver();
    void OnCommonEventReceived(const std::string &event) override;

private:
    std::weak_ptr<PlayerServer> server_;
};
} // namespace Media
} // namespace OHOS
#endif // PLAYER_SERVER_EVENT_RECEIVER_H