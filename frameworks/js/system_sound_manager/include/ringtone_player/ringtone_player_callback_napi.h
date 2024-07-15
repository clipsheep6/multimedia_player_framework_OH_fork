/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef RINGTONE_PLAYER_CALLBACK_NAPI_H
#define RINGTONE_PLAYER_CALLBACK_NAPI_H

#include "ringtone_player.h"
#include "common_napi.h"
#include "audio_info.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
class RingtonePlayerCallbackNapi : public RingtonePlayerInterruptCallback {
public:
    explicit RingtonePlayerCallbackNapi(napi_env env);
    virtual ~RingtonePlayerCallbackNapi();
    void SaveCallbackReference(const std::string &callbackName, napi_value callback);
    void RemoveCallbackReference(const std::string &callbackName);
    void OnInterrupt(const AudioStandard::InterruptEvent &interruptEvent) override;
private:
    struct RingtonePlayerJsCallback {
        std::shared_ptr<AutoRef> callback = nullptr;
        std::string callbackName = "unknown";
        AudioStandard::InterruptEvent interruptEvent;
    };

    void OnJsCallbackInterrupt(std::unique_ptr<RingtonePlayerJsCallback> &jsCb);

    std::mutex mutex_;
    napi_env env_ = nullptr;
    std::shared_ptr<AutoRef> interruptCallback_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif // RINGTONE_PLAYER_CALLBACK_NAPI_H
