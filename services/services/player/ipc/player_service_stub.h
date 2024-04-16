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

#ifndef PLAYER_SERVICE_STUB_H
#define PLAYER_SERVICE_STUB_H

#include <map>
#include "i_standard_player_service.h"
#include "i_standard_player_listener.h"
#include "media_death_recipient.h"
#include "player_server.h"
#include "monitor_server_object.h"

namespace OHOS {
namespace Media {
class PlayerServiceStub
    : public IRemoteStub<IStandardPlayerService>,
      public MonitorServerObject,
      public NoCopyable {
public:
    static sptr<PlayerServiceStub> Create();
    virtual ~PlayerServiceStub();

    int OnRemoteRequest(uint32_t code, MessageParcel &data,
        MessageParcel &reply, MessageOption &option) override;
    int32_t AddSubSource(const std::string &url) override;
    int32_t Play() override;
    int32_t SetSource(const sptr<IRemoteObject> &object) override;
    int32_t SetListenerObject(const sptr<IRemoteObject> &object) override;
    int32_t SetSource(int32_t fd, int64_t offset, int64_t size) override;
    int32_t Release() override;
    int32_t SetSource(const std::string &url) override;
    int32_t Pause() override;
    int32_t Prepare() override;
    int32_t Seek(int32_t mSeconds, PlayerSeekMode mode) override;
    int32_t AddSubSource(int32_t fd, int64_t offset, int64_t size) override;
    int32_t SetRenderFirstFrame(bool display) override;
    int32_t PrepareAsync() override;
    int32_t Stop() override;
    int32_t Reset() override;
    int32_t SetVolume(float leftVolume, float rightVolume) override;
    int32_t SetPlaybackSpeed(PlaybackRateMode mode) override;
    int32_t GetVideoWidth() override;
    int32_t GetAudioTrackInfo(std::vector<Format> &audioTrack) override;
    int32_t GetCurrentTime(int32_t &currentTime) override;
    int32_t GetVideoTrackInfo(std::vector<Format> &videoTrack) override;
    int32_t GetPlaybackSpeed(PlaybackRateMode &mode) override;
    int32_t GetSubtitleTrackInfo(std::vector<Format> &subtitleTrack) override;
    int32_t GetVideoHeight() override;
    int32_t GetDuration(int32_t &duration) override;
#ifdef SUPPORT_VIDEO
    int32_t SetVideoSurface(sptr<Surface> surface) override;
#endif

    int32_t SetLooping(bool loop) override;
    int32_t SetParameter(const Format &param) override;
    int32_t SelectBitRate(uint32_t bitRate) override;
    int32_t SelectTrack(int32_t index) override;
    int32_t DestroyStub() override;
    int32_t SetPlayerCallback() override;
    int32_t DumpInfo(int32_t fd);
    int32_t DeselectTrack(int32_t index) override;
    int32_t GetCurrentTrack(int32_t trackType, int32_t &index) override;
    int32_t SetMediaSource(const std::shared_ptr<AVMediaSource> &mediaSource, AVPlayStrategy strategy) override;
    bool IsPlaying() override;
    bool IsLooping() override;
    int32_t SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySessionProxy,
        bool svp) override;
    // MonitorServerObject override
    int32_t DoIpcAbnormality() override;
    int32_t DoIpcRecovery(bool fromMonitor) override;
protected:
    PlayerServiceStub();
    virtual int32_t Init();
    void SetPlayerFuncs();

    TaskQueue taskQue_;
    std::shared_ptr<IPlayerService> playerServer_ = nullptr;
    std::shared_ptr<PlayerCallback> playerCallback_ = nullptr;
    int32_t appUid_ = 0;
    int32_t appPid_ = 0;

private:
    int32_t SetListenerObject(MessageParcel &data, MessageParcel &reply);
    int32_t SetSource(MessageParcel &data, MessageParcel &reply);
    int32_t SetMediaDataSource(MessageParcel &data, MessageParcel &reply);
    int32_t SetFdSource(MessageParcel &data, MessageParcel &reply);
    int32_t AddSubSource(MessageParcel &data, MessageParcel &reply);
    int32_t AddSubFdSource(MessageParcel &data, MessageParcel &reply);
    int32_t Play(MessageParcel &data, MessageParcel &reply);
    int32_t Prepare(MessageParcel &data, MessageParcel &reply);
    int32_t SetRenderFirstFrame(MessageParcel &data, MessageParcel &reply);
    int32_t PrepareAsync(MessageParcel &data, MessageParcel &reply);
    int32_t Pause(MessageParcel &data, MessageParcel &reply);
    int32_t Stop(MessageParcel &data, MessageParcel &reply);
    int32_t Reset(MessageParcel &data, MessageParcel &reply);
    int32_t Release(MessageParcel &data, MessageParcel &reply);
    int32_t SetVolume(MessageParcel &data, MessageParcel &reply);
    int32_t Seek(MessageParcel &data, MessageParcel &reply);
    int32_t GetCurrentTime(MessageParcel &data, MessageParcel &reply);
    int32_t GetVideoTrackInfo(MessageParcel &data, MessageParcel &reply);
    int32_t GetAudioTrackInfo(MessageParcel &data, MessageParcel &reply);
    int32_t GetSubtitleTrackInfo(MessageParcel &data, MessageParcel &reply);
    int32_t GetVideoWidth(MessageParcel &data, MessageParcel &reply);
    int32_t GetVideoHeight(MessageParcel &data, MessageParcel &reply);
    int32_t GetDuration(MessageParcel &data, MessageParcel &reply);
    int32_t SetPlaybackSpeed(MessageParcel &data, MessageParcel &reply);
    int32_t GetPlaybackSpeed(MessageParcel &data, MessageParcel &reply);
#ifdef SUPPORT_VIDEO
    int32_t SetVideoSurface(MessageParcel &data, MessageParcel &reply);
#endif
    int32_t IsPlaying(MessageParcel &data, MessageParcel &reply);
    int32_t IsLooping(MessageParcel &data, MessageParcel &reply);
    int32_t SetLooping(MessageParcel &data, MessageParcel &reply);
    int32_t SetParameter(MessageParcel &data, MessageParcel &reply);
    int32_t DestroyStub(MessageParcel &data, MessageParcel &reply);
    int32_t SetPlayerCallback(MessageParcel &data, MessageParcel &reply);
    int32_t SelectBitRate(MessageParcel &data, MessageParcel &reply);
    int32_t SelectTrack(MessageParcel &data, MessageParcel &reply);
    int32_t DeselectTrack(MessageParcel &data, MessageParcel &reply);
    int32_t GetCurrentTrack(MessageParcel &data, MessageParcel &reply);
    int32_t SetDecryptConfig(MessageParcel &data, MessageParcel &reply);
    int32_t SetMediaSource(MessageParcel &data, MessageParcel &reply);
    using PlayerStubFunc = int32_t(PlayerServiceStub::*)(MessageParcel &data, MessageParcel &reply);
    std::map<uint32_t, std::pair<PlayerStubFunc, std::string>> playerFuncs_;
};
} // namespace Media
} // namespace OHOS
#endif // PLAYER_SERVICE_STUB_H
