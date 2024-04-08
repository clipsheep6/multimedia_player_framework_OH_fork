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

#ifndef AV_PLAYER_CALLBACK_H
#define AV_PLAYER_CALLBACK_H

#include <map>
#include <mutex>
#include "player.h"
#include "media_errors.h"
#include "common_napi.h"
#include "event_handler.h"

namespace OHOS {
namespace Media {
class AVPlayerNotify {
public:
    AVPlayerNotify() = default;
    virtual ~AVPlayerNotify() = default;
    virtual void NotifyDuration(int32_t duration) = 0;
    virtual void NotifyPosition(int32_t position) = 0;
    virtual void NotifyState(PlayerStates state) = 0;
    virtual void NotifyVideoSize(int32_t width, int32_t height) = 0;
    virtual void NotifyIsLiveStream() = 0;
    virtual void NotifyDrmInfoUpdated(const std::multimap<std::string, std::vector<uint8_t>> &infos) = 0;
};

class AVPlayerCallback : public PlayerCallback {
public:
    AVPlayerCallback(napi_env env, AVPlayerNotify *listener);
    virtual ~AVPlayerCallback();
    void OnError(int32_t errorCode, const std::string &errorMsg) override;
    void OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody) override;
    void OnErrorCb(MediaServiceExtErrCodeAPI9 errorCode, const std::string &errorMsg);
    PlayerStates GetCurrentState() const;
    int32_t GetVideoWidth() const;
    int32_t GetVideoHeight() const;
    void SaveCallbackReference(const std::string &name, std::weak_ptr<AutoRef> ref);
    void ClearCallbackReference();
    void ClearCallbackReference(const std::string &name);
    void Start();
    void Pause();
    void Release();

    std::atomic<bool> isSetVolume_ = false;
private:
    void OnStartRenderFrameCb() const;
    void OnStateChangeCb(const int32_t extra, const Format &infoBody);
    void OnVolumeChangeCb(const int32_t extra, const Format &infoBody);
    void OnSeekDoneCb(const int32_t extra, const Format &infoBody);
    void OnSpeedDoneCb(const int32_t extra, const Format &infoBody);
    void OnBitRateDoneCb(const int32_t extra, const Format &infoBody);
    void OnPositionUpdateCb(const int32_t extra, const Format &infoBody);
    void OnDurationUpdateCb(const int32_t extra, const Format &infoBody);
    void OnBufferingUpdateCb(const int32_t extra, const Format &infoBody);
    void OnSubtitleUpdateCb(const int32_t extra, const Format &infoBody);
    void OnMessageCb(const int32_t extra, const Format &infoBody);
    void OnVideoSizeChangedCb(const int32_t extra, const Format &infoBody);
    void OnAudioInterruptCb(const int32_t extra, const Format &infoBody);
    void OnAudioDeviceChangeCb(const int32_t extra, const Format &infoBody);
    void OnBitRateCollectedCb(const int32_t extra, const Format &infoBody);
    void OnDrmInfoUpdatedCb(const int32_t extra, const Format &infoBody);
    void OnSetDecryptConfigDoneCb(const int32_t extra, const Format &infoBody);

    void OnEosCb(const int32_t extra, const Format &infoBody);
    void NotifyIsLiveStream(const int32_t extra, const Format &infoBody);
    void OnTrackChangedCb(const int32_t extra, const Format &infoBody);
    void OnTrackInfoUpdate(const int32_t extra, const Format &infoBody);

    std::mutex mutex_;
    napi_env env_ = nullptr;
    std::map<std::string, std::weak_ptr<AutoRef>> refMap_;
    AVPlayerNotify *listener_ = nullptr;
    std::atomic<bool> isloaded_ = false;
    PlayerStates state_ = PLAYER_IDLE;
    std::shared_ptr<AppExecFwk::EventHandler> handler_ = nullptr;
    std::map<uint32_t, void(AVPlayerCallback::*)(const int32_t extra, const Format &infoBody)> onInfoFuncs_;
};
} // namespace Media
} // namespace OHOS
#endif // AV_PLAYER_CALLBACK_H