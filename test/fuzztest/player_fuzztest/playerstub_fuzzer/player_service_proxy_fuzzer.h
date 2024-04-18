/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef PLAYER_SERVICE_PROXY_FUZZER_H
#define PLAYER_SERVICE_PROXY_FUZZER_H

#include "stub_common.h"
#include "i_standard_player_service.h"

namespace OHOS {
namespace Media {
class PlayerServiceProxyFuzzer : public IRemoteProxy<IStandardPlayerService> {
public:
    static sptr<PlayerServiceProxyFuzzer> Create();
    explicit PlayerServiceProxyFuzzer(const sptr<IRemoteObject> &impl);
    virtual ~PlayerServiceProxyFuzzer() {}
    void SendRequest(int32_t code, uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SetListenerObject(const sptr<IRemoteObject> &object) override
    {
        return 0;
    }
    int32_t SetSource(const std::string &url) override
    {
        return 0;
    }
    int32_t SetSource(const sptr<IRemoteObject> &object) override
    {
        return 0;
    }
    int32_t SetSource(int32_t fd, int64_t offset, int64_t size) override
    {
        return 0;
    }
    int32_t SetMediaSource(const std::shared_ptr<AVMediaSource> &mediaSource, AVPlayStrategy strategy) override
    {
        return 0;
    }
    int32_t AddSubSource(const std::string &url) override
    {
        return 0;
    }
    int32_t AddSubSource(int32_t fd, int64_t offset, int64_t size) override
    {
        return 0;
    }
    int32_t Play() override
    {
        return 0;
    }
    int32_t Prepare() override
    {
        return 0;
    }
    int32_t PrepareAsync() override
    {
        return 0;
    }
    int32_t Pause() override
    {
        return 0;
    }
    int32_t Stop() override
    {
        return 0;
    }
    int32_t Reset() override
    {
        return 0;
    }
    int32_t Release() override
    {
        return 0;
    }
    int32_t ReleaseSync() override
    {
        return 0;
    }
    int32_t SetVolume(float leftVolume, float rightVolume) override
    {
        return 0;
    }
    int32_t Seek(int32_t mSeconds, PlayerSeekMode mode) override
    {
        return 0;
    }
    int32_t GetCurrentTime(int32_t &currentTime) override
    {
        return 0;
    }
    int32_t GetVideoTrackInfo(std::vector<Format> &videoTrack) override
    {
        return 0;
    }
    int32_t GetAudioTrackInfo(std::vector<Format> &audioTrack) override
    {
        return 0;
    }
    int32_t GetSubtitleTrackInfo(std::vector<Format> &subtitleTrack) override
    {
        return 0;
    }
    int32_t GetVideoWidth() override
    {
        return 0;
    }
    int32_t GetVideoHeight() override
    {
        return 0;
    }
    int32_t GetDuration(int32_t &duration) override
    {
        return 0;
    }
    int32_t SetPlaybackSpeed(PlaybackRateMode mode) override
    {
        return 0;
    }
    int32_t GetPlaybackSpeed(PlaybackRateMode &mode) override
    {
        return 0;
    }
    int32_t SetVideoSurface(sptr<Surface> surface) override
    {
        return 0;
    }
    bool IsPlaying() override
    {
        return 0;
    }
    bool IsLooping() override
    {
        return 0;
    }
    int32_t SetLooping(bool loop) override
    {
        return 0;
    }
    int32_t SetParameter(const Format &param) override
    {
        return 0;
    }
    int32_t DestroyStub() override
    {
        return 0;
    }
    int32_t SetPlayerCallback() override
    {
        return 0;
    }
    int32_t SelectBitRate(uint32_t bitRate) override
    {
        return 0;
    }
    int32_t SelectTrack(int32_t index) override
    {
        return 0;
    }
    int32_t DeselectTrack(int32_t index) override
    {
        return 0;
    }
    int32_t GetCurrentTrack(int32_t trackType, int32_t &index) override
    {
        return 0;
    }
    int32_t SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySessionProxy,
        bool svp) override
    {
        return 0;
    }

private:
    int32_t SetListenerObject(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SetSource(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SetMediaDataSource(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SetFdSource(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t AddSubSource(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t AddSubFdSource(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t Play(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t Prepare(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t PrepareAsync(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t Pause(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t Stop(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t Reset(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t Release(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SetVolume(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t Seek(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t GetCurrentTime(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t GetDuration(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SetPlaybackSpeed(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t GetPlaybackSpeed(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SetVideoSurface(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t IsPlaying(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t IsLooping(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SetLooping(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SetParameter(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t DestroyStub(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SetPlayerCallback(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t GetVideoTrackInfo(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t GetAudioTrackInfo(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t GetSubtitleTrackInfo(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t GetVideoWidth(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t GetVideoHeight(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SelectBitRate(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SelectTrack(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t DeselectTrack(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t GetCurrentTrack(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SendRequest(uint32_t code, MessageParcel &inputData, MessageParcel &reply, MessageOption &option);
    static inline BrokerDelegator<PlayerServiceProxyFuzzer> delegator_;
    using PlayerStubFunc = int32_t(PlayerServiceProxyFuzzer::*)(uint8_t *inputData, size_t size, bool isFuzz);
    std::map<uint32_t, PlayerStubFunc> playerFuncs_;
};
}
}

#endif // PLAYER_SERVICE_PROXY_FUZZER_H

