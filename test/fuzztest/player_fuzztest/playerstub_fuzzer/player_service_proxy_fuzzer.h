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

namespace OHOS {
namespace Media {
class IStandardPlayerService : public IRemoteBroker {
public:
    virtual ~IStandardPlayerService() = default;

    enum PlayerServiceMsg {
        SET_LISTENER_OBJ = 0,
        SET_SOURCE,
        SET_MEDIA_DATA_SRC_OBJ,
        SET_FD_SOURCE,
        PLAY,
        PREPARE,
        PREPAREASYNC,
        PAUSE,
        STOP,
        RESET,
        RELEASE,
        SET_VOLUME,
        SEEK,
        GET_CURRENT_TIME,
        GET_DURATION,
        SET_PLAYERBACK_SPEED,
        GET_PLAYERBACK_SPEED,
        SET_VIDEO_SURFACE,
        IS_PLAYING,
        IS_LOOPING,
        SET_LOOPING,
        SET_RENDERER_DESC,
        DESTROY,
        SET_CALLBACK,
        GET_VIDEO_TRACK_INFO,
        GET_AUDIO_TRACK_INFO,
        GET_VIDEO_WIDTH,
        GET_VIDEO_HEIGHT,
        SELECT_BIT_RATE,
        SELECT_TRACK,
        DESELECT_TRACK,
        GET_CURRENT_TRACK
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardPlayerService");
};

class PlayerServiceProxyFuzzer : public IRemoteProxy<IStandardPlayerService> {
public:
    static sptr<PlayerServiceProxyFuzzer> Create();
    explicit PlayerServiceProxyFuzzer(const sptr<IRemoteObject> &impl);
    virtual ~PlayerServiceProxyFuzzer() {}
    void SendRequest(int32_t code, uint8_t *inputData, size_t size, bool isFuzz);
private:
    int32_t SetSource(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SetMediaDataSource(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SetFdSource(uint8_t *inputData, size_t size, bool isFuzz);
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
    int32_t GetVideoTrackInfo(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t GetAudioTrackInfo(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t GetVideoWidth(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t GetDuration(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SetPlaybackSpeed(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t GetPlaybackSpeed(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SetVideoSurface(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t IsPlaying(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t IsLooping(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SetLooping(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SetParameter(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SetPlayerCallback(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SelectBitRate(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t DestroyStub(uint8_t *inputData, size_t size, bool isFuzz);
    int32_t SendRequest(uint32_t code, MessageParcel &inputData, MessageParcel &reply, MessageOption &option);
    static inline BrokerDelegator<PlayerServiceProxyFuzzer> delegator_;
    using PlayerStubFunc = int32_t(PlayerServiceProxyFuzzer::*)(uint8_t *inputData, size_t size, bool isFuzz);
    std::map<uint32_t, PlayerStubFunc> playerFuncs_;
};
}
}

#endif // PLAYER_SERVICE_PROXY_FUZZER_H

