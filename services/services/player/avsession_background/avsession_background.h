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

#ifndef AVSESSION_BACKGROUND
#define AVSESSION_BACKGROUND

#include <mutex>
#include <list>
#include <map>
#include "avsession_manager.h"
#include "i_player_service.h"

namespace OHOS {
namespace Media {
class AVsessionBackground
    : public AVSession::SessionListener,
      public std::enable_shared_from_this<AVsessionBackground> {
public:
    static AVsessionBackground &Instance();
    void AddListener(std::weak_ptr<IPlayerService> player, int32_t uid);
    AVsessionBackground();
    ~AVsessionBackground();

private:
    void Init();
    void OnSessionCreate(const AVSession::AVSessionDescriptor &descriptor) override
    {
        (void)descriptor;
    }
    void OnSessionRelease(const AVSession::AVSessionDescriptor &descriptor) override
    {
        (void)descriptor;
    }
    void OnTopSessionChange(const AVSession::AVSessionDescriptor &descriptor) override
    {
        (void)descriptor;
    }
    void OnAudioSessionChecked(const int32_t uid) override;

    static std::shared_ptr<AVsessionBackground> instance_;
    static std::once_flag onceFlag_;
    bool init_ = false;
    std::map<int32_t, std::list<std::weak_ptr<IPlayerService>>> playerMap_;
    std::mutex mutex_;
};
}
}
#endif // AVSESSION_BACKGROUND