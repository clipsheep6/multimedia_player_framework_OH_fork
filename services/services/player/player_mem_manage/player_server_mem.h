/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
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

#ifndef PLAYER_SERVER_MEM_H
#define PLAYER_SERVER_MEM_H

#include <vector>
#include <chrono>
#include "player_server.h"
#include "player_server_state.h"

namespace OHOS {
namespace Media {
enum class PlayerSourceType : int32_t {
    SOURCE_TYPE_NULL,
    SOURCE_TYPE_URL,
    SOURCE_TYPE_DATASRC,
    SOURCE_TYPE_FD,
};

class PlayerServerMem : public PlayerServer {
public:
    static std::shared_ptr<IPlayerService> Create();
    PlayerServerMem();
    virtual ~PlayerServerMem();

    int32_t Reset() override;
    int32_t Stop() override;
    int32_t AddSubSource(const std::string &url) override;
    int32_t PrepareAsync() override;
    int32_t SetSource(int32_t fd, int64_t offset, int64_t size) override;
    int32_t Pause() override;
    int32_t SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc) override;
    int32_t Prepare() override;
    int32_t SetSource(const std::string &url) override;
    int32_t Play() override;
    int32_t AddSubSource(int32_t fd, int64_t offset, int64_t size) override;
    int32_t Release() override;
    int32_t GetCurrentTime(int32_t &currentTime) override;
    int32_t GetVideoWidth() override;
    int32_t GetVideoHeight() override;
    int32_t GetVideoTrackInfo(std::vector<Format> &videoTrack) override;
    int32_t GetAudioTrackInfo(std::vector<Format> &audioTrack) override;
    int32_t GetDuration(int32_t &duration) override;
    int32_t SetPlaybackSpeed(PlaybackRateMode mode) override;
    int32_t GetPlaybackSpeed(PlaybackRateMode &mode) override;
    int32_t SetVolume(float leftVolume, float rightVolume) override;
    int32_t Seek(int32_t mSeconds, PlayerSeekMode mode) override;
#ifdef SUPPORT_VIDEO
    int32_t SetVideoSurface(sptr<Surface> surface) override;
#endif
    bool IsPlaying() override;
    bool IsLooping() override;
    int32_t SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback) override;
    int32_t SelectBitRate(uint32_t bitRate) override;
    int32_t SelectTrack(int32_t index) override;
    int32_t SetLooping(bool loop) override;
    int32_t SetParameter(const Format &param) override;
    int32_t DeselectTrack(int32_t index) override;
    int32_t GetCurrentTrack(int32_t trackType, int32_t &index) override;
    int32_t DumpInfo(int32_t fd) override;
    void OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody = {}) override;
    void ResetFrontGroundForMemManage();
    void ResetBackGroundForMemManage();
    void ResetMemmgrForMemManage();
    void RecoverByMemManage();

private:
    class MemBaseState;
    class MemIdleState;
    class MemInitializedState;
    class MemPreparingState;
    class MemPreparedState;
    class MemPlayingState;
    class MemPausedState;
    class MemStoppedState;
    class MemPlaybackCompletedState;

    std::shared_ptr<MemIdleState> memIdleState_ = nullptr;
    std::shared_ptr<MemInitializedState> memInitializedState_ = nullptr;
    std::shared_ptr<MemPreparingState> memPreparingState_ = nullptr;
    std::shared_ptr<MemPreparedState> memPreparedState_ = nullptr;
    std::shared_ptr<MemPlayingState> memPlayingState_ = nullptr;
    std::shared_ptr<MemPausedState> memPausedState_ = nullptr;
    std::shared_ptr<MemStoppedState> memStoppedState_ = nullptr;
    std::shared_ptr<MemPlaybackCompletedState> memPlaybackCompletedState_ = nullptr;
    struct FdSrcInfo {
        int32_t fd = 0;
        int64_t offset = 0;
        int64_t size = 0;
    };
    struct RecoverConfigInfo {
        std::shared_ptr<MemBaseState> currState = nullptr;
        int32_t sourceType = static_cast<int32_t>(PlayerSourceType::SOURCE_TYPE_NULL);
        std::string url = "";
        std::shared_ptr<IMediaDataSource> dataSrc = nullptr;
        int32_t fd = 0;
        int64_t offset = 0;
        int64_t size = 0;
        std::vector<std::string> subUrl;
        std::vector<FdSrcInfo> subFdSrc;
        float leftVolume = 1.0f;
        float rightVolume = 1.0f;
        PlaybackRateMode speedMode = SPEED_FORWARD_1_00_X;
        sptr<Surface> surface = nullptr;
        bool loop = false;
        bool isPlaying = false;
        int32_t videoScaleType = -1;
        int32_t contentType = -1;
        int32_t streamUsage = -1;
        int32_t rendererFlag = -1;
        int32_t interruptMode = -1;
        int32_t effectMode = OHOS::AudioStandard::AudioEffectMode::EFFECT_DEFAULT;
        std::shared_ptr<PlayerCallback> callback = nullptr;
        uint32_t bitRate = 0;
        int32_t currentTime = 0;
        std::vector<Format> videoTrack;
        std::vector<Format> audioTrack;
        int32_t videoWidth = 0;
        int32_t videoHeight = 0;
        int32_t duration = 0;
        int32_t audioIndex = 0;
        int32_t videoIndex = 0;
        int32_t textIndex = 0;
    } recoverConfig_;
    struct PlayerServerConfig {
        bool errorCbOnce = false;
        bool disableStoppedCb = false;
        std::string lastErrMsg = "";
        std::unique_ptr<UriHelper> uriHelper = nullptr;
    } playerServerConfig_;
    std::mutex mutex_;
    std::mutex mutexCb_;
    bool isReleaseMemByManage_ = false;
    bool isRecoverMemByUser_ = false;
    bool isAudioPlayer_ = true;
    int32_t continueReset = 0;
    std::map<void *, std::shared_ptr<MemBaseState>> stateMap_;
    std::chrono::steady_clock::time_point lastestUserSetTime_;
    std::condition_variable recoverCond_;
    int32_t defaultAudioIndex_ = -1;
    bool isSeekToCurrentTime_ = false;
    bool isLocalResource_ = false;

    int32_t Init() override;
    void SetStateMap();
    void SaveParameter(const Format &param);
    int32_t SetSaveParameter();
    int32_t SetSourceInternal();
    int32_t AddSubSourceInternal();
    int32_t PrepareAsyncInner();
    void SetPlayerServerConfig();
    void GetPlayerServerConfig();
    int32_t SetConfigInternal();
    int32_t SetBehaviorInternal();
    int32_t SetPlaybackSpeedInternal();
    int32_t GetInformationBeforeMemReset();
    void RecoverToInitialized(PlayerOnInfoType type, int32_t extra);
    void RecoverToPrepared(PlayerOnInfoType type, int32_t extra);
    void RecoverToCompleted(PlayerOnInfoType type, int32_t extra);
    int32_t RecoverPlayerCb();
    void CheckHasRecover(PlayerOnInfoType type, int32_t extra);
    int32_t ReleaseMemByManage();
    int32_t RecoverMemByUser();
    bool NeedSelectAudioTrack();
    void GetDefaultTrack(PlayerOnInfoType type, int32_t extra, const Format &infoBody);
    int32_t SeekToCurrentTime(int32_t mSeconds, PlayerSeekMode mode);
    int32_t HandleCodecBuffers(bool enable);
    int32_t LocalResourceRelease();
    int32_t NetworkResourceRelease();
    int32_t LocalResourceRecover();
    int32_t NetworkRecover();
    void SetResourceTypeBySysParam();
};
}
}
#endif // PLAYER_SERVER_MEM_H
