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

#ifndef PLAYER_ENGINE_GST_IMPL
#define PLAYER_ENGINE_GST_IMPL

#include <mutex>
#include <cstdint>
#include <thread>
#include <map>

#include "audio_info.h"
#include "i_player_engine.h"
#include "i_playbin_ctrler.h"
#include "gst_appsrc_engine.h"
#include "player_codec_ctrl.h"
#include "task_queue.h"

namespace OHOS {
namespace Media {
class PlayerEngineGstImpl : public IPlayerEngine, public NoCopyable {
public:
    explicit PlayerEngineGstImpl(int32_t uid = 0, int32_t pid = 0, uint32_t tokenId = 0);
    ~PlayerEngineGstImpl();

    int32_t SetSource(const std::string &url) override;
    int32_t SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc) override;
    int32_t AddSubSource(const std::string &url) override;
    int32_t SetObs(const std::weak_ptr<IPlayerEngineObs> &obs) override;
    int32_t SetVideoSurface(sptr<Surface> surface) override;
    int32_t Seek(int32_t mSeconds, PlayerSeekMode mode) override;
    int32_t PrepareAsync() override;
    int32_t Play() override;
    int32_t Pause() override;
    int32_t Stop() override;
    int32_t Reset() override;
    int32_t GetAudioTrackInfo(std::vector<Format> &audioTrack) override;
    int32_t SetVolume(float leftVolume, float rightVolume) override;
    int32_t GetSubtitleTrackInfo(std::vector<Format> &subtitleTrack) override;
    int32_t GetCurrentTime(int32_t &currentTime) override;
    int32_t SelectBitRate(uint32_t bitRate) override;
    int32_t SelectTrack(int32_t index) override;
    int32_t GetPlaybackSpeed(PlaybackRateMode &mode) override;
    int32_t GetVideoWidth() override;
    int32_t GetVideoTrackInfo(std::vector<Format> &videoTrack) override;
    int32_t GetDuration(int32_t &duration) override;
    int32_t SetPlaybackSpeed(PlaybackRateMode mode) override;
    int32_t SetMediaSource(std::map<std::string, std::string> header, u_int32_t preferedWidth, 
        u_int32_t preferedHeight, u_int32_t bufferDuration, bool preferHDR) override;
    int32_t SetParameter(const Format &param) override;
    int32_t GetVideoHeight() override;
    int32_t SetLooping(bool loop) override;
    int32_t SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySessionProxy,
        bool svp) override;
    int32_t SetVideoScaleType(Plugins::VideoScaleType videoScaleType) override;
    int32_t SetAudioRendererInfo(const int32_t contentType, const int32_t streamUsage,
        const int32_t rendererFlag) override;
    int32_t SetAudioInterruptMode(const int32_t interruptMode) override;
    int32_t DeselectTrack(int32_t index) override;
    int32_t GetCurrentTrack(int32_t trackType, int32_t &index) override;
    int32_t SetAudioEffectMode(const int32_t effectMode) override;
    int32_t GetHEBCMode() override;
    int32_t HandleCodecBuffers(bool enable) override;
    int32_t SeekToCurrentTime(int32_t mSeconds, PlayerSeekMode mode) override;

private:
    void OnNotifyMessage(const PlayBinMessage &msg);
    void OnNotifyElemSetup(GstElement &elem);
    void OnNotifyElemUnSetup(GstElement &elem);
    void OnReset();
    GValueArray *OnNotifyAutoPlugSort(GValueArray &factories);
    double ChangeModeToSpeed(const PlaybackRateMode &mode) const;
    PlaybackRateMode ChangeSpeedToMode(double rate) const;
    int32_t PlayBinCtrlerInit();
    int32_t PlayBinCtrlerPrepare();
    void PlayBinCtrlerDeInit();
    int32_t GetRealPath(const std::string &url, std::string &realUrlPath) const;
    bool IsFileUrl(const std::string &url) const;
    void HandleErrorMessage(const PlayBinMessage &msg);
    void HandleInfoMessage(const PlayBinMessage &msg);
    void HandleSeekDoneMessage(const PlayBinMessage &msg);
    void HandleSpeedDoneMessage(const PlayBinMessage &msg);
    void HandleSubTypeMessage(const PlayBinMessage &msg);
    void HandleBufferingStart(const PlayBinMessage &msg);
    void HandleBufferingEnd(const PlayBinMessage &msg);
    void HandleBufferingTime(const PlayBinMessage &msg);
    void HandleBufferingPercent(const PlayBinMessage &msg);
    void HandleBufferingUsedMqNum(const PlayBinMessage &msg);
    void HandleVideoRenderingStart(const PlayBinMessage &msg);
    void HandleVideoSizeChanged(const PlayBinMessage &msg);
    void HandleBitRateCollect(const PlayBinMessage &msg);
    void HandleIsLiveStream(const PlayBinMessage &msg);
    void HandleTrackNumUpdate(const PlayBinMessage &msg);
    void HandleTrackInfoUpdate(const PlayBinMessage &msg);
    void HandleAudioMessage(const PlayBinMessage &msg);
    void HandleDrmInfoUpdated(const PlayBinMessage &msg);
    void HandleSetDecryptConfigDone(const PlayBinMessage &msg);

    void HandleInterruptMessage(const PlayBinMessage &msg);
    void HandleAudioFirstFrameMessage(const PlayBinMessage &msg);
    void HandleDeviceChangeMessage(const PlayBinMessage &msg);
    void HandlePositionUpdateMessage(const PlayBinMessage &msg);
    void HandleSubtitleUpdate(const PlayBinMessage &msg);
    void HandleTrackChanged(const PlayBinMessage &msg);
    void HandleDefaultTrack(const PlayBinMessage &msg);
    void HandleTrackDone(const PlayBinMessage &msg);
    void HandleAddSubDone(const PlayBinMessage &msg);
    void HandleOnError(const PlayBinMessage &msg);
    void OnCapsFixError();
    void ResetPlaybinToSoftDec();

    std::mutex mutex_;
    std::mutex sinkProviderMutex_;
    std::shared_ptr<IPlayBinCtrler> playBinCtrler_ = nullptr;
    std::shared_ptr<PlayBinSinkProvider> sinkProvider_;
    std::weak_ptr<IPlayerEngineObs> obs_;
    sptr<Surface> producerSurface_ = nullptr;
    std::string url_ = "";
    std::shared_ptr<GstAppsrcEngine> appsrcWrap_ = nullptr;
    PlayerCodecCtrl codecCtrl_;
    int32_t videoWidth_ = 0;
    int32_t videoHeight_ = 0;
    int32_t updatePercent_ = -1;
    uint32_t mqNum_ = 0;
    uint64_t bufferingTime_ = 0;
    int32_t appuid_ = 0;
    int32_t apppid_ = 0;
    uint32_t apptokenid_ = 0;
    std::map<uint32_t, uint64_t> mqBufferingTime_;
    Plugins::VideoScaleType videoScaleType_ = Plugins::VideoScaleType::VIDEO_SCALE_TYPE_FIT;
    int32_t contentType_ = AudioStandard::CONTENT_TYPE_MUSIC; // CONTENT_TYPE_MUSIC
    int32_t streamUsage_ = AudioStandard::STREAM_USAGE_MEDIA; // STREAM_USAGE_MEDIA
    int32_t rendererFlag_ = 0;
    int32_t currentTime_ = -1;
    int32_t duration_ = -1;
    int32_t currentTimeOnInfoCnt_ = 0;
    bool isPlaySinkFlagsSet_ = false;
    bool useSoftDec_ = false;
    std::unique_ptr<TaskQueue> taskQueue_;
    bool isAdaptiveLiveStream_ = false;
    std::map<int32_t, void(PlayerEngineGstImpl::*)(const PlayBinMessage &msg)> subMsgHandler_;
};
} // namespace Media
} // namespace OHOS
#endif // PLAYER_ENGINE_GST_IMPL
