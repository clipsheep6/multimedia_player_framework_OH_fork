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

#ifndef SOUNDPOOL_CALLBACK_NAPI_H
#define SOUNDPOOL_CALLBACK_NAPI_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "common_napi.h"
#include "media_errors.h"
#include "media_log.h"
#include "isoundpool.h"

namespace OHOS {
namespace Media {
namespace SoundPoolEvent {
    const std::string EVENT_LOAD_COMPLETED = "loadComplete";
    const std::string EVENT_PLAY_FINISHED = "playFinished";
    const std::string EVENT_ERROR = "error";
}

class SoundPoolCallBackNapi : public ISoundPoolCallback {
public:
    explicit SoundPoolCallBackNapi(napi_env env);
    ~SoundPoolCallBackNapi() = default;
    void SaveCallbackReference(const std::string &name, std::weak_ptr<AutoRef> ref);
    void CancelCallbackReference(const std::string &name);
    void ClearCallbackReference();
    void SendErrorCallback(int32_t errCode, const std::string &msg);
    void SendLoadCompletedCallback(int32_t soundId);
    void SendPlayCompletedCallback();

protected:
    void OnLoadCompleted(int32_t soundId) override;
    void OnPlayFinished() override;
    void OnError(int32_t errorCode) override;

private:
    struct SoundPoolJsCallBack {
        std::weak_ptr<AutoRef> autoRef;
        std::string callbackName = "unknown";
        std::string errorMsg = "unknown";
        int32_t errorCode = MSERR_EXT_UNKNOWN;
        int32_t reason = 1;
        int32_t loadSoundId = 0;
    };
    void OnJsErrorCallBack(SoundPoolJsCallBack *jsCb) const;
    void OnJsloadCompletedCallBack(SoundPoolJsCallBack *jsCb) const;
    void OnJsplayCompletedCallBack(SoundPoolJsCallBack *jsCb) const;
    napi_env env_ = nullptr;
    std::mutex mutex_;
    std::map<std::string, std::weak_ptr<AutoRef>> refMap_;
};
} // namespace Media
} // namespace OHOS
#endif // SOUNDPOOL_CALLBACK_NAPI_H
