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

#ifndef AV_META_DATA_HELPER_CALLBACK_H
#define AV_META_DATA_HELPER_CALLBACK_H

#include <map>
#include <mutex>
#include "media_errors.h"
#include "common_napi.h"
#include "event_handler.h"
#include "avmetadatahelper.h"

namespace OHOS {
namespace Media {
namespace AVMetadataHelperState {
const std::string STATE_IDLE = "idle";
const std::string STATE_PREPARED = "prepared";
const std::string STATE_RELEASED = "released";
const std::string STATE_CALL_DONE = "callDone";
const std::string STATE_ERROR = "error";
}

namespace AVMetadataHelperEvent {
const std::string EVENT_STATE_CHANGE = "stateChange";
const std::string EVENT_ERROR = "error";
}

class AVMetadataHelperNotify {
public:
    AVMetadataHelperNotify() = default;
    virtual ~AVMetadataHelperNotify() = default;
    virtual void NotifyState(HelperStates state) = 0;
};

class AVMetadataHelperCallback : public HelperCallback {
public:
    AVMetadataHelperCallback(napi_env env, AVMetadataHelperNotify *listener);
    virtual ~AVMetadataHelperCallback();
    void OnError(int32_t errorCode, const std::string &errorMsg);
    void OnInfo(HelperOnInfoType type, int32_t extra, const Format &infoBody);
    void OnErrorCb(MediaServiceExtErrCodeAPI9 errorCode, const std::string &errorMsg);
    HelperStates GetCurrentState() const;
    void SaveCallbackReference(const std::string &name, std::weak_ptr<AutoRef> ref);
    void ClearCallbackReference();
    void ClearCallbackReference(const std::string &name);
    void Start();
    void Pause();
    void Release();

private:
    void OnStateChangeCb(const int32_t extra, const Format &infoBody);

    std::mutex mutex_;
    napi_env env_ = nullptr;
    std::map<std::string, std::weak_ptr<AutoRef>> refMap_;
    AVMetadataHelperNotify *listener_ = nullptr;
    std::atomic<bool> isloaded_ = false;
    HelperStates state_ = HELPER_IDLE;
    std::map<uint32_t, void(AVMetadataHelperCallback::*)(const int32_t extra, const Format &infoBody)> onInfoFuncs_;
};
} // namespace Media
} // namespace OHOS
#endif // AV_META_DATA_HELPER_CALLBACK_H