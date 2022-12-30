/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#ifndef AVRECORDER_CALLBACK_H
#define AVRECORDER_CALLBACK_H

#include "avrecorder_napi.h"
#include "recorder.h"
#include "av_common.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "common_napi.h"

namespace OHOS {
namespace Media {
class AVRecorderCallback : public RecorderCallback {
public:
    explicit AVRecorderCallback(napi_env env);
    virtual ~AVRecorderCallback();

    void SaveCallbackReference(const std::string &name, std::weak_ptr<AutoRef> ref);
    void CancelCallbackReference(const std::string &name);
    void ClearCallbackReference();
    void SendErrorCallback(int32_t errCode, const std::string &msg);
    void SendStateCallback(const std::string &state, const StateChangeReason &reason);
    std::string GetState();

protected:
    void OnError(RecorderErrorType errorType, int32_t errCode) override;
    void OnInfo(int32_t type, int32_t extra) override;

private:
    struct AVRecordJsCallback {
        std::weak_ptr<AutoRef> autoRef;
        std::string callbackName = "unknown";
        std::string errorMsg = "unknown";
        int32_t errorCode = MSERR_EXT_UNKNOWN;
        int32_t reason = 1;
        std::string state = "unknown";
    };
    void OnJsErrorCallBack(AVRecordJsCallback *jsCb) const;
    void OnJsStateCallBack(AVRecordJsCallback *jsCb) const;
    napi_env env_ = nullptr;
    std::mutex mutex_;
    std::string currentState_ = AVRecorderState::STATE_IDLE;
    std::map<std::string, std::weak_ptr<AutoRef>> refMap_;
};
} // namespace Media
} // namespace OHOS
#endif // AVRECORDER_CALLBACK_H