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
#ifndef AVTRANSCODER_CALLBACK_H
#define AVTRANSCODER_CALLBACK_H

#include "avtranscoder_napi.h"
#include "transcoder.h"
#include "av_common.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "common_napi.h"
#include <uv.h>
#include "scope_guard.h"

namespace OHOS {
namespace Media {
class AVTransCoderCallback : public TransCoderCallback {
public:
    explicit AVTransCoderCallback(napi_env env);
    virtual ~AVTransCoderCallback();

    void SaveCallbackReference(const std::string &name, std::weak_ptr<AutoRef> ref);
    void CancelCallbackReference(const std::string &name);
    void ClearCallbackReference();
    void SendErrorCallback(int32_t errCode, const std::string &msg);
    void SendStateCallback(const std::string &state, const StateChangeReason &reason);
    void SendCompleteCallback();
    void SendProgressUpdateCallback(int32_t progress);
    std::string GetState();

protected:
    void OnError(TransCoderErrorType errorType, int32_t errCode) override;
    void OnInfo(int32_t type, int32_t extra) override;

private:
    struct AVTransCoderJsCallback {
        std::weak_ptr<AutoRef> autoRef;
        std::string callbackName = "unknown";
        std::string errorMsg = "unknown";
        int32_t errorCode = MSERR_EXT_UNKNOWN;
        int32_t reason = 1;
        std::string state = "unknown";
        int32_t progress;
    };
    void OnJsErrorCallBack(AVTransCoderJsCallback *jsCb) const;
    void OnJsCompleteCallBack(AVTransCoderJsCallback *jsCb) const;
    void OnJsProgressUpdateCallback(AVTransCoderJsCallback *jsCb) const;
    int32_t QueueErrorWork(uv_loop_s *loop, uv_work_t *work) const;
    int32_t QueueCompleteWork(uv_loop_s *loop, uv_work_t *work) const;
    int32_t QueueProgressUpdateWork(uv_loop_s *loop, uv_work_t *work) const;
    napi_env env_ = nullptr;
    std::mutex mutex_;
    std::string currentState_ = AVTransCoderState::STATE_IDLE;
    std::map<std::string, std::weak_ptr<AutoRef>> refMap_;
};
} // namespace Media
} // namespace OHOS
#endif // AVRECORDER_CALLBACK_H