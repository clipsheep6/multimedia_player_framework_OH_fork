/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include "player_server.h"

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
    ~PlayerServerMem();

    int32_t SetSource(const std::string &url) override;
    int32_t SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc) override;
    int32_t SetSource(int32_t fd, int64_t offset, int64_t size) override;
    int32_t Play() override;
    int32_t Prepare() override;
    int32_t PrepareAsync() override;
    int32_t Pause() override;
    int32_t Stop() override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t ReleaseSync() override;
    int32_t SetVolume(float leftVolume, float rightVolume) override;
    int32_t Seek(int32_t mSeconds, PlayerSeekMode mode) override;
    int32_t GetCurrentTime(int32_t &currentTime) override;
    int32_t GetVideoTrackInfo(std::vector<Format> &videoTrack) override;
    int32_t GetAudioTrackInfo(std::vector<Format> &audioTrack) override;
    int32_t GetVideoWidth() override;
    int32_t GetVideoHeight() override;
    int32_t GetDuration(int32_t &duration) override;
    int32_t SetPlaybackSpeed(PlaybackRateMode mode) override;
    int32_t GetPlaybackSpeed(PlaybackRateMode &mode) override;
#ifdef SUPPORT_VIDEO
    int32_t SetVideoSurface(sptr<Surface> surface) override;
#endif
    bool IsPlaying() override;
    bool IsLooping() override;
    int32_t SetLooping(bool loop) override;
    int32_t SetParameter(const Format &param) override;
    int32_t SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback) override;
    int32_t SelectBitRate(uint32_t bitRate) override;
    void OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody = {}) override;

    void ResetForMemManage(int32_t recallType, int32_t onTrimLevel);

protected:
    int32_t Init() override;
    int32_t SetSourceInternal() override;
    int32_t SetConfigInternal() override;
    int32_t SetBehaviorInternal() override;
    int32_t SetPlaybackSpeedInternal() override;
    int32_t GetInformationBeforeMemReset() override;
    void RecoverToInitialized(PlayerOnInfoType type, int32_t extra) override;
    void RecoverToPrepared(PlayerOnInfoType type, int32_t extra) override;
    void RecoverToCompleted(PlayerOnInfoType type, int32_t extra) override;

private:
    struct RecoverConfigInfo {
        std::shared_ptr<PlayerServerState> curState = nullptr;
        int32_t sourceType = static_cast<int32_t>(PlayerSourceType::SOURCE_TYPE_NULL);
        std::string url = "";
        std::shared_ptr<IMediaDataSource> dataSrc = nullptr;
        int32_t fd = 0;
        int64_t offset = 0;
        int64_t size = 0;
        float leftVolume = 1.0f;
        float rightVolume = 1.0f;
        PlaybackRateMode speedMode = SPEED_FORWARD_1_00_X;
        sptr<Surface> surface = nullptr;
        bool loop = false;
        bool isPlaying = false;
        bool isHasSetParam = false;
        Format param;
        std::shared_ptr<PlayerCallback> callback = nullptr;
        uint32_t bitRate = 0;
        int32_t currentTime = 0;
        std::vector<Format> videoTrack;
        std::vector<Format> audioTrack;
        int32_t videoWidth = 0;
        int32_t videoHeight = 0;
        int32_t duration = 0;
    } recoverConfig_;
    bool isReleaseMemByManage_ = false;
    bool isRecoverMemByUser_ = false;
    bool isAudioPlayer_ = true;
    int32_t continuousNotPlayingCnt_ = 0;

    int32_t RecoverPlayerCb();
    void CheckHasRecover(PlayerOnInfoType type, int32_t extra);
    int32_t ReleaseMemByManage();
    int32_t RecoverMemByUser();
};
}
}
#endif // PLAYER_SERVER_MEM_H
