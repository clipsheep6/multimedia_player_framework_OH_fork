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

#ifndef AV_PLAYER_CALLBACK
#define AV_PLAYER_CALLBACK

#include <map>
#include <mutex>
#include "player.h"
#include "avplayer_napi.h"
#include "media_errors.h"
#include "common_napi.h"

namespace OHOS {
namespace Media {
class AVPlayerCallback : public PlayerCallback {
public:
    AVPlayerCallback(napi_env env, AVPlayerNotify *listener);
    virtual ~AVPlayerCallback();
    void OnError(PlayerErrorType errorType, int32_t errorCode) override;
    void OnError(int32_t errorCode, std::string errorMsg) override;
    void OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody) override;
    void OnErrorCb(MediaServiceExtErrCodeAPI9 errorCode, const std::string &errorMsg);
    PlayerStates GetCurrentState() const;
    int32_t GetVideoWidth() const;
    int32_t GetVideoHeight() const;
    void SaveCallbackReference(const std::string &name, std::weak_ptr<AutoRef> ref);
    void ClearCallbackReference();
    void Start();
    void Pause();
    void Release();

private:
    void OnStateChangeCb(PlayerStates state);
    void OnVolumeChangeCb(double volumeLevel);
    void OnSeekDoneCb(int32_t currentPositon) const;
    void OnSpeedDoneCb(int32_t speedMode) const;
    void OnBitRateDoneCb(int32_t bitRate) const;
    void OnPositionUpdateCb(int32_t position) const;
    void OnBufferingUpdateCb(const Format &infoBody) const;
    void OnMessageCb(int32_t extra, const Format &infoBody) const;
    void OnStartRenderFrameCb() const;
    void OnVideoSizeChangedCb(const Format &infoBody);
    void OnAudioInterruptCb(const Format &infoBody) const;
    void OnBitRateCollectedCb(const Format &infoBody) const;
    void OnEosCb(int32_t isLooping) const;

    std::mutex mutex_;
    napi_env env_ = nullptr;
    std::map<std::string, std::weak_ptr<AutoRef>> refMap_;
    AVPlayerNotify *listener_ = nullptr;
    std::atomic<bool> isloaded_ = false;
};

class NapiCallback {
public:
    struct Base {
        std::weak_ptr<AutoRef> callback;
        std::string callbackName = "unknown";
        Base() = default;
        virtual ~Base() = default;
        virtual void UvWork()
    };

    struct Error : public Base {
        std::string errorMsg = "unknown";
        MediaServiceExtErrCodeAPI9 errorCode = MSERR_EXT_API9_UNSUPPORT_FORMAT;
        void UvWork() override
    };

    struct Int : public Base {
        int32_t value = 0;
        void UvWork() override
    };

    struct IntVec : public Base {
        std::vector<int32_t> valueVec;
        void UvWork() override
    };

    struct IntArray : public Base {
        std::vector<int32_t> valueVec;
        void UvWork() override
    };

    struct Double : public Base {
        double value = 0.0;
        void UvWork() override
    };

    struct PropertyInt : public Base {
        std::map<std::string, int32_t> valueMap;
        void UvWork() override
    };

    struct StateChange : public Base {
        std::string state = "";
        int32_t reason = 0;
        void UvWork() override
    };

    static void CompleteCallback(napi_env env, NapiCallback::Base *jsCb)
};
} // namespace Media
} // namespace OHOS
#endif // PLAYER_CALLBACK_NAPI_H_