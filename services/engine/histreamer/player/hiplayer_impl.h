/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#ifndef HI_PLAYER_IMPL_H
#define HI_PLAYER_IMPL_H

#include <memory>
#include <unordered_map>
#include <queue>

#include "audio_decoder_filter.h"
#include "audio_sink_filter.h"
#include "common/status.h"
#include "demuxer_filter.h"
#include "filter/filter.h"
#include "filter/filter_factory.h"
#include "hiplayer_callback_looper.h"
#include "media_utils.h"
#include "i_player_engine.h"
#include "media_sync_manager.h"
#include "pipeline/pipeline.h"
#ifdef SUPPORT_VIDEO
#include "decoder_surface_filter.h"
#endif

namespace OHOS {
namespace Media {
using namespace Pipeline;

class HiPlayerImpl : public IPlayerEngine, public std::enable_shared_from_this<HiPlayerImpl> {
public:
    HiPlayerImpl(int32_t appUid, int32_t appPid, uint32_t appTokenId, uint64_t appFullTokenId);
    ~HiPlayerImpl() override;
    HiPlayerImpl(const HiPlayerImpl& other) = delete;
    HiPlayerImpl& operator=(const HiPlayerImpl& other) = delete;
    Status Init();
    // interface from PlayerInterface
    int32_t SetSource(const std::string& uri) override;
    int32_t SetSource(const std::shared_ptr<IMediaDataSource>& dataSrc) override;
    int32_t Prepare() override;
    int32_t PrepareAsync() override;
    int32_t Play() override;
    int32_t Pause() override;
    int32_t Stop() override;
    int32_t Reset() override;
    int32_t Seek(int32_t mSeconds, PlayerSeekMode mode) override;
    int32_t SetVolume(float leftVolume, float rightVolume) override;
    int32_t SetVideoSurface(sptr<Surface> surface) override;
    int32_t SetDecryptConfig(const sptr<OHOS::DrmStandard::IMediaKeySessionService> &keySessionProxy,
        bool svp) override;
    int32_t SetLooping(bool loop) override;
    int32_t SetParameter(const Format& params) override;
    int32_t SetObs(const std::weak_ptr<IPlayerEngineObs>& obs) override;
    int32_t GetCurrentTime(int32_t& currentPositionMs) override;
    int32_t GetDuration(int32_t& durationMs) override;
    int32_t SetPlaybackSpeed(PlaybackRateMode mode) override;
    int32_t GetPlaybackSpeed(PlaybackRateMode& mode) override;
    int32_t SelectBitRate(uint32_t bitRate) override;

    int32_t GetVideoTrackInfo(std::vector<Format>& videoTrack) override;
    int32_t GetAudioTrackInfo(std::vector<Format>& audioTrack) override;
    int32_t GetVideoWidth() override;
    int32_t GetVideoHeight() override;
    int32_t SetVideoScaleType(VideoScaleType videoScaleType) override;
    int32_t SetAudioRendererInfo(const int32_t contentType, const int32_t streamUsage,
                                 const int32_t rendererFlag) override;
    int32_t SetAudioInterruptMode(const int32_t interruptMode) override;

    // internal interfaces
    void OnEvent(const Event &event);
    void OnStateChanged(PlayerStateId state);
    void OnCallback(std::shared_ptr<Filter> filter, const FilterCallBackCommand cmd,
                    StreamType outType);

private:
    enum HiplayerSvpMode : int32_t {
        SVP_CLEAR = -1, /* it's not a protection video */
        SVP_FALSE, /* it's a protection video but not need secure decoder */
        SVP_TRUE, /* it's a protection video and need secure decoder */
    };

    Status DoSetSource(const std::shared_ptr<MediaSource> source);
    Status Resume();
    void HandleCompleteEvent(const Event& event);
    void HandleDrmInfoUpdatedEvent(const Event& event);
    void HandleIsLiveStreamEvent(bool isLiveStream);
    void HandleErrorEvent(int32_t errorCode);
    void UpdateStateNoLock(PlayerStates newState, bool notifyUpward = true);
    double ChangeModeToSpeed(const PlaybackRateMode& mode) const;
    void NotifyBufferingUpdate(const std::string_view& type, int32_t param);
    void NotifyDurationUpdate(const std::string_view& type, int32_t param);
    void NotifySeekDone(int32_t seekPos);
    void NotifyAudioInterrupt(const Event& event);
    void NotifyAudioFirstFrame(const Event& event);
    void NotifyResolutionChange();
    void NotifyPositionUpdate();
    Status LinkAudioDecoderFilter(const std::shared_ptr<Filter>& preFilter, StreamType type);
    Status LinkAudioSinkFilter(const std::shared_ptr<Filter>& preFilter, StreamType type);
    void DoInitializeForHttp();
    bool EnableBufferingBySysParam() const;
#ifdef SUPPORT_VIDEO
    Status LinkVideoDecoderFilter(const std::shared_ptr<Filter>& preFilter, StreamType type);
    bool IsVideoMime(const std::string& mime);
#endif
    bool isNetWorkPlay_ = false;
    int32_t appUid_{0};
    int32_t appPid_{0};
    int32_t appTokenId_{0};
    int64_t appFullTokenId_{0};
    OHOS::Media::Mutex stateMutex_{};
    OHOS::Media::ConditionVariable cond_{};
    int64_t duration_{-1};
    std::atomic<bool> singleLoop_ {false};
    std::atomic<PlaybackRateMode> playbackRateMode_ {PlaybackRateMode::SPEED_FORWARD_1_00_X};

    std::shared_ptr<EventReceiver> playerEventReceiver_;
    std::shared_ptr<FilterCallback> playerFilterCallback_;
    std::weak_ptr<Meta> sourceMeta_{};
    std::vector<std::weak_ptr<Meta>> streamMeta_{};
    std::atomic<PlayerStates> pipelineStates_ {PlayerStates::PLAYER_IDLE}; // only update in UpdateStateNoLock()
    std::queue<PlayerStates> pendingStates_ {};
    std::shared_ptr<OHOS::Media::Pipeline::Pipeline> pipeline_;
    std::shared_ptr<DemuxerFilter> demuxer_;
    std::shared_ptr<AudioDecoderFilter> audioDecoder_;
    std::shared_ptr<AudioSinkFilter> audioSink_;
#ifdef SUPPORT_VIDEO
    std::shared_ptr<DecoderSurfaceFilter> videoDecoder_;
#endif
    std::shared_ptr<MediaSyncManager> syncManager_;
    std::atomic<PlayerStateId> curState_;
    HiPlayerCallbackLooper callbackLooper_{};
    sptr<Surface> surface_ {nullptr};
    std::string url_;
    int32_t videoWidth_{0};
    int32_t videoHeight_{0};

    std::shared_ptr<Meta> audioRenderInfo_{nullptr};
    std::shared_ptr<Meta> audioInterruptMode_{nullptr};

    bool isStreaming_{false};

    std::mutex drmMutex_;
    std::condition_variable drmConfigCond_;
    bool isDrmProtected_ = false;
    bool isDrmPrepared_ = false;
    bool stopWaitingDrmConfig_ = false;
    sptr<DrmStandard::IMediaKeySessionService> keySessionServiceProxy_{nullptr};
    int32_t svpMode_ = HiplayerSvpMode::SVP_CLEAR;
};
} // namespace Media
} // namespace OHOS
#endif // HI_PLAYER_IMPL_H