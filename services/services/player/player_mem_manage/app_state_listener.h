/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
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

#ifndef APP_STATE_LISTENER_H
#define APP_STATE_LISTENER_H

#include "app_state_subscriber.h"

namespace OHOS {
namespace Media {
enum class AppState : int32_t {
    APP_STATE_NULL,
    APP_STATE_FRONT_GROUND = 2,
    APP_STATE_BACK_GROUND = 4,
};
class AppStateListener : public Memory::AppStateSubscriber {
public:
    AppStateListener();
    ~AppStateListener();

    void OnConnected() override;
    void OnDisconnected() override;
    void OnAppStateChanged(int32_t pid, int32_t uid, int32_t state) override;
    void ForceReclaim(int32_t pid, int32_t uid) override;
    void OnTrim(Memory::SystemMemoryLevel level) override;
    void OnRemoteDied(const wptr<IRemoteObject> &object) override;
};
}
}
#endif // APP_STATE_LISTENER_H