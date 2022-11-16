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
#include <queue>
#include <mutex>
#include "avplayer_napi.h"
#include "player.h"

namespace OHOS {
namespace Media {
class AVPlayerCallback : public PlayerCallback {
public:
    explicit AVPlayerCallback(napi_env env);
    virtual ~AVPlayerCallback();
    void OnError(PlayerErrorType errorType, int32_t errorCode) override;
    void OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody) override;
    void OnErrorCb(MediaServiceExtErrCodeAPI9 errorCode, const std::string &errorMsg);
    PlayerStates GetCurrentState() const;
    int32_t GetVideoWidth() const;
    int32_t GetVideoHeight() const;
    void SaveCallbackReference(const std::string &name, std::weak_ptr<AutoRef> ref);
    void ClearCallbackReference();
    void SaveAsyncContext(const std::string &name, std::shared_ptr<MediaAsyncContext> ctx);
    void ClearAsyncContext();

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
    std::map<std::string, std::queue<std::shared_ptr<MediaAsyncContext>>> ctxMap_;
    PlayerStates state_ = PLAYER_IDLE;
    int32_t width_ = 0;
    int32_t height_ = 0;
};
} // namespace Media
} // namespace OHOS
#endif // PLAYER_CALLBACK_NAPI_H_