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

#ifndef PLAYER_CALLBACK_NAPI_H_
#define PLAYER_CALLBACK_NAPI_H_

#include "audio_player_napi.h"
#include "player.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Media {
struct AutoRef {
    AutoRef(napi_env env, napi_ref cb)
        : env_(env), cb_(cb)
    {
    }
    ~AutoRef()
    {
        if (env_ != nullptr && cb_ != nullptr) {
            napi_delete_reference(env_, cb_);
        }
    }
    napi_env env_;
    napi_ref cb_;
};

class PlayerCallbackNapi : public PlayerCallback {
public:
    explicit PlayerCallbackNapi(napi_env env);
    virtual ~PlayerCallbackNapi();
    void SaveCallbackReference(const std::string &callbackName, napi_value callback);
    void SendErrorCallback(napi_env env, MediaServiceExtErrCode errCode);
    PlayerStates GetCurrentState() const;

protected:
    void OnError(PlayerErrorType errName, int32_t errMsg) override;
    void OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody = {}) override;

private:
    static napi_status FillErrorArgs(napi_env env, int32_t errCode, napi_value &args);
    void OnSeekDoneCb(int32_t currentPositon);
    void OnEosCb(int32_t isLooping);
    void OnStateChangeCb(PlayerStates state);
    void OnPositionUpdateCb(int32_t postion) const;
    void OnMessageCb(int32_t type) const;
    void OnVolumeChangeCb();
    struct PlayerJsCallback {
        std::shared_ptr<AutoRef> callback = nullptr;
        std::string callbackName = "unknown";
        std::string errorMsg = "unknown";
        MediaServiceExtErrCode errorCode = MSERR_EXT_UNKNOWN;
        int32_t position = -1;
    };
    void OnJsCallBack(PlayerJsCallback *jsCb);
    void OnJsCallBackError(PlayerJsCallback *jsCb);
    void OnJsCallBackPosition(PlayerJsCallback *jsCb);

    std::mutex mutex_;
    napi_env env_ = nullptr;
    PlayerStates currentState_ = PLAYER_IDLE;
    std::shared_ptr<AutoRef> errorCallback_ = nullptr; // error
    std::shared_ptr<AutoRef> playCallback_ = nullptr; // started
    std::shared_ptr<AutoRef> pauseCallback_ = nullptr; // paused
    std::shared_ptr<AutoRef> stopCallback_ = nullptr; // stopped
    std::shared_ptr<AutoRef> resetCallback_ = nullptr; // idle
    std::shared_ptr<AutoRef> dataLoadCallback_ = nullptr; // prepared
    std::shared_ptr<AutoRef> finishCallback_ = nullptr; // endofstream
    std::shared_ptr<AutoRef> timeUpdateCallback_ = nullptr; // seekdone
    std::shared_ptr<AutoRef> volumeChangeCallback_ = nullptr; // volumedone
};
}  // namespace Media
}  // namespace OHOS
#endif // PLAYER_CALLBACK_NAPI_H_
