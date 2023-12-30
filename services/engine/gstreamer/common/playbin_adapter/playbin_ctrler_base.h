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

#ifndef PLAYBIN_CTRLER_BASE_H
#define PLAYBIN_CTRLER_BASE_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <future>
#include <gst/gst.h>
#include "nocopyable.h"
#include "i_playbin_ctrler.h"
#include "state_machine.h"
#include "gst_msg_processor.h"
#include "task_queue.h"
#include "player_track_parse.h"
#include "av_common.h"
#include "gst_utils.h"

namespace OHOS {
namespace Media {
class PlayBinCtrlerBase;
using PlayBinCtrlerWrapper = ThizWrapper<PlayBinCtrlerBase>;

enum GstPlayerStatus : int32_t {
    GST_PLAYER_STATUS_IDLE = 0,
    GST_PLAYER_STATUS_BUFFERING,
    GST_PLAYER_STATUS_READY,
    GST_PLAYER_STATUS_PAUSED,
    GST_PLAYER_STATUS_PLAYING,
};

class PlayBinCtrlerBase
    : public IPlayBinCtrler,
      public StateMachine,
      public std::enable_shared_from_this<PlayBinCtrlerBase> {
public:
    explicit PlayBinCtrlerBase(const PlayBinCreateParam &createParam);
    virtual ~PlayBinCtrlerBase();

    int32_t Init();
    bool EnableBufferingBySysParam() const;
    int32_t SetSource(const std::string &url)  override;
    int32_t SetSource(const std::shared_ptr<GstAppsrcEngine> &appsrcWrap) override;
    int32_t AddSubSource(const std::string &url) override;
    int32_t PrepareAsync() override;
    int32_t Play() override;
    int32_t Pause() override;
    int32_t Seek(int64_t timeUs, int32_t seekOption) override;
    int32_t Stop(bool needWait) override;
    int32_t SetRate(double rate) override;
    int64_t QueryPosition() override;
    int32_t SetLoop(bool loop) override;
    void SetVolume(const float &leftVolume, const float &rightVolume) override;
    void SetAudioInterruptMode(const int32_t interruptMode) override;
    int32_t SetAudioRendererInfo(const uint32_t rendererInfo, const int32_t rendererFlag) override;
    int32_t SetAudioEffectMode(const int32_t effectMode) override;
    int32_t SelectBitRate(uint32_t bitRate) override;
#ifdef SUPPORT_DRM
    int32_t SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySessionProxy,
        bool svp) override;
#endif

    void SetElemSetupListener(ElemSetupListener listener) final;
    void SetElemUnSetupListener(ElemSetupListener listener) final;
    void SetAutoPlugSortListener(AutoPlugSortListener listener) final;
    void RemoveGstPlaySinkVideoConvertPlugin() final;
    void SetNotifier(PlayBinMsgNotifier notifier) final;
    void SetAutoSelectBitrate(bool enable) final;
    int32_t GetVideoTrackInfo(std::vector<Format> &videoTrack) override;
    int32_t GetAudioTrackInfo(std::vector<Format> &audioTrack) override;
    int32_t GetSubtitleTrackInfo(std::vector<Format> &subtitleTrack) override;
    int32_t SelectTrack(int32_t index) override;
    int32_t DeselectTrack(int32_t index) override;
    int32_t GetCurrentTrack(int32_t trackType, int32_t &index) override;
protected:
    virtual int32_t OnInit() = 0;

    GstPipeline *playbin_ = nullptr;

private:
    class BaseState;
    class IdleState;
    class InitializedState;
    class PreparingState;
    class PreparedState;
    class PlayingState;
    class PausedState;
    class StoppedState;
    class StoppingState;
    class PlaybackCompletedState;
    enum SvpMode : int32_t {
        SVP_CLEAR = -1, /* it's not a protection video */
        SVP_FALSE, /* it's a protection video but not need secure decoder */
        SVP_TRUE, /* it's a protection video and need secure decoder */
    };

    int32_t EnterInitializedState();
    void ExitInitializedState();
    int32_t PrepareAsyncInternal();
    int32_t SeekInternal(int64_t timeUs, int32_t seekOption);
    int32_t StopInternal();
    int32_t SetRateInternal(double rate);
    void SetupCustomElement();
    GstSeekFlags ChooseSetRateFlags(double rate);
    void SetupSourceSetupSignal();
    int32_t SetupSignalMessage();
    int32_t SetupElementUnSetupSignal();
    void QueryDuration();
    void ProcessEndOfStream();
    static void ElementSetup(const GstElement *playbin, GstElement *elem, gpointer userData);
    static void ElementUnSetup(const GstElement *playbin, GstElement *subbin, GstElement *child, gpointer userData);
    static void SourceSetup(const GstElement *playbin, GstElement *elem, gpointer userData);
    static void OnBitRateParseCompleteCb(const GstElement *playbin, uint32_t *bitrateInfo,
        uint32_t bitrateNum, gpointer userData);
    static void OnSelectBitrateDoneCb(const GstElement *playbin, uint32_t bandwidth, gpointer userData);
    static GValueArray *AutoPlugSort(const GstElement *uriDecoder, GstPad *pad, GstCaps *caps,
        GValueArray *factories, gpointer userData);
    static void OnInterruptEventCb(const GstElement *audioSink, const uint32_t eventType, const uint32_t forceType,
        const uint32_t hintType, gpointer userData);
    static void OnDeviceChangeEventCb(const GstElement *audioSink, const AudioStandard::DeviceInfo deviceInfo,
        const AudioStandard::AudioStreamDeviceChangeReason reason);
    static void OnAudioSegmentEventCb(const GstElement *audioSink, gpointer userData);
    static void OnAudioDiedEventCb(const GstElement *audioSink, gpointer userData);
    static void OnIsLiveStream(const GstElement *demux, gboolean isLiveStream, gpointer userData);
    static void AudioChanged(const GstElement *playbin, gpointer userData);
#ifdef SUPPORT_DRM
    static int32_t OnDrmInfoUpdatedSignalReceived(const GstElement *demux, gpointer drmInfoArray, uint32_t infoCount,
        gpointer userData);
    static int32_t OnMediaDecryptSignalReceived(const GstElement *elem, int64_t inputBuffer,
        int64_t outputBuffer, uint32_t length, gpointer keyId, uint32_t keyIdLength, gpointer iv, uint32_t ivLength,
        uint32_t subsampleCount, gpointer subsamples, uint32_t mode, uint32_t svp, uint32_t cryptByteBlock,
        uint32_t skipByteBlock, gpointer userData);
#endif
    void SetupInterruptEventCb();
    void SetupAudioSegmentEventCb();
    void SetupAudioDiedEventCb();
    void SetupAudioDeviceEventCb();
    void OnElementSetup(GstElement &elem);
    void OnElementUnSetup(GstElement &elem);
    void OnSourceSetup(const GstElement *playbin, GstElement *src,
        const std::shared_ptr<PlayBinCtrlerBase> &playbinCtrl);
    bool OnVideoDecoderSetup(GstElement &elem);
    bool OnAppsrcMessageReceived(const InnerMessage &msg);
    void OnMessageReceived(const InnerMessage &msg);
    void OnSinkMessageReceived(const PlayBinMessage &msg);
    GValueArray *OnAutoPlugSort(GValueArray &factories);
    void ReportMessage(const PlayBinMessage &msg);
    int32_t Reset() noexcept;
    bool IsLiveSource() const;
    int32_t DoInitializeForDataSource();
    void DoInitializeForHttp();
    void HandleCacheCtrl(int32_t percent);
    void HandleCacheCtrlCb(const InnerMessage &msg);
    void HandleCacheCtrlWhenNoBuffering(int32_t percent);
    void HandleCacheCtrlWhenBuffering(int32_t percent);
    void OnAdaptiveElementSetup(GstElement &elem);
    void OnAudioChanged();
    void OnSubtitleChanged();
    void ReportTrackChange();
    void OnTrackDone();
    void OnAddSubDone();
    void OnError(int32_t errorCode, std::string message);
    void CheckAndAddSignalIds(gulong id, PlayBinCtrlerWrapper *wrapper, GstElement *elem);
    bool SetPlayerState(GstPlayerStatus status);
#ifdef SUPPORT_DRM
    void OnDemuxElementSetup(GstElement &elem);
    void OnCodecElementSetup(GstElement &elem);
    void OnDecryptElementSetup(GstElement &elem);
#endif

    inline void AddSignalIds(GstElement *element, gulong signalId)
    {
        if (signalIds_.find(element) == signalIds_.end()) {
            signalIds_[element] = {signalId};
        } else {
            signalIds_[element].push_back(signalId);
        }
    }
    inline void RemoveSignalIds(GstElement *element)
    {
        if (signalIds_.find(element) != signalIds_.end()) {
            for (auto id : signalIds_[element]) {
                g_signal_handler_disconnect(element, id);
            }
            signalIds_.erase(element);
        }
    }
    std::mutex mutex_;
    std::mutex cacheCtrlMutex_;
    std::mutex stateChangePropertyMutex_;
    std::mutex listenerMutex_;
    std::mutex appsrcMutex_;
    std::unique_ptr<TaskQueue> msgQueue_;
    PlayBinRenderMode renderMode_ = PlayBinRenderMode::DEFAULT_RENDER;
    PlayBinMsgNotifier notifier_;
    ElemSetupListener elemSetupListener_;
    ElemSetupListener elemUnSetupListener_;
    AutoPlugSortListener autoPlugSortListener_;
    std::shared_ptr<PlayBinSinkProvider> sinkProvider_;
    std::unique_ptr<GstMsgProcessor> msgProcessor_;
    std::string uri_;
    std::shared_ptr<PlayerTrackParse> trackParse_;

    int32_t svpMode_ = SVP_CLEAR;
    std::mutex drmInfoMutex_;
    /* drmInfo Will be Updated when received a drminfo-update signal,
       and cleared when the source uri is changed. */
    std::map<std::string, std::vector<uint8_t>> drmInfo_;
#ifdef SUPPORT_DRM
    sptr<DrmStandard::IMediaKeySessionService> keySessionServiceProxy_;
    sptr<DrmStandard::IMediaDecryptModuleService> decryptModuleProxy_;
#endif
    enum DrmSignalRetCode : int32_t {
        DRM_SIGNAL_OK = 0,
        DRM_SIGNAL_INVALID_PARAM,
        DRM_SIGNAL_INVALID_OPERATOR,
        DRM_SIGNAL_SERVICE_WRONG,
        DRM_SIGNAL_UNKNOWN,
    };
    std::map<GstElement *, std::vector<gulong>> signalIds_;
    std::vector<uint32_t> bitRateVec_;
    bool isInitialized_ = false;

    bool isErrorHappened_ = false;
    std::future<gboolean> seekFuture_;
    std::condition_variable preparingCond_;
    std::condition_variable preparedCond_;
    std::condition_variable stoppingCond_;
    std::condition_variable drmConfigCond_;
    bool isDrmPrepared_ = false;
    bool stopWaitingDrmConfig_ = false;
    std::mutex drmMutex_;
    
    PlayBinSinkProvider::SinkPtr audioSink_ = nullptr;
    PlayBinSinkProvider::SinkPtr videoSink_ = nullptr;
    PlayBinSinkProvider::SinkPtr subtitleSink_ = nullptr;

    int64_t duration_ = 0;
    double rate_ = 0;
    int64_t seekPos_ = 0;
    int64_t lastTime_ = 0;

    bool stopBuffering_ = false;
    bool isSeeking_ = false;
    bool isClosetSeeking_ = false;
    bool isRating_ = false;
    bool isAddingSubtitle_ = false;
    bool isBuffering_ = false;
    bool isSelectBitRate_ = false;
    bool isNetWorkPlay_ = false;
    bool isUserSetPlay_ = false;
    bool isUserSetPause_ = false;
    bool isReplay_ = false;
    uint32_t rendererInfo_ = 0;
    int32_t rendererFlag_ = 0;
    int32_t cachePercent_ = 100; // 100% cache percent
    uint64_t connectSpeed_ = 0;
    GstClockTime lastStartTime_ = GST_CLOCK_TIME_NONE;

    bool isTrackChanging_ = false;
    int32_t trackChangeType_ = MediaType::MEDIA_TYPE_AUD;
    int32_t audioIndex_ = -1;
    bool hasSubtitleTrackSelected_ = true;
    uint32_t subtitleTrackNum_ = 0;

    std::atomic<bool> isDuration_ = false;
    std::atomic<bool> enableLooping_ = false;
    std::shared_ptr<GstAppsrcEngine> appsrcWrap_ = nullptr;

    std::shared_ptr<IdleState> idleState_;
    std::shared_ptr<InitializedState> initializedState_;
    std::shared_ptr<PreparingState> preparingState_;
    std::shared_ptr<PreparedState> preparedState_;
    std::shared_ptr<PlayingState> playingState_;
    std::shared_ptr<PausedState> pausedState_;
    std::shared_ptr<StoppedState> stoppedState_;
    std::shared_ptr<StoppingState> stoppingState_;
    std::shared_ptr<PlaybackCompletedState> playbackCompletedState_;
};
} // namespace Media
} // namespace OHOS
#endif // PLAYBIN_CTRLER_BASE_H