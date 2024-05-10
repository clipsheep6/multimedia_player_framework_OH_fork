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

#ifndef SCREEN_CAPTURE_SERVICE_SERVER_H
#define SCREEN_CAPTURE_SERVICE_SERVER_H

#include <mutex>
#include <cstdlib>
#include <thread>
#include <string>
#include <memory>
#include <atomic>
#include <queue>
#include <vector>
#include <chrono>

#include "audio_capturer_wrapper.h"
#include "i_screen_capture_service.h"
#include "nocopyable.h"
#include "uri_helper.h"
#include "task_queue.h"
#include "accesstoken_kit.h"
#include "privacy_kit.h"
#include "ipc_skeleton.h"
#include "screen_capture.h"
#include "audio_capturer.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "audio_info.h"
#include "surface.h"
#include "display_manager.h"
#include "screen_manager.h"
#include "i_recorder_service.h"
#include "recorder_server.h"
#include "notification_content.h"
#include "notification_helper.h"
#include "notification_request.h"
#include "notification_constant.h"
#include "notification_slot.h"
#include "incall_observer.h"
#include "media_data_source.h"
#include "meta/meta.h"

namespace OHOS {
namespace Media {
using namespace Rosen;
using namespace AudioStandard;
using namespace OHOS::Notification;
using OHOS::Security::AccessToken::PrivacyKit;

class ScreenCaptureServer;

class NotificationSubscriber : public OHOS::Notification::NotificationLocalLiveViewSubscriber {
public:
    void OnConnected() override;
    void OnDisconnected() override;
    void OnResponse(int32_t notificationId,
        OHOS::sptr<OHOS::Notification::NotificationButtonOption> buttonOption) override;
    void OnDied() override;
};

enum VideoPermissionState : int32_t {
    START_VIDEO = 0,
    STOP_VIDEO = 1
};

enum AVScreenCaptureState : int32_t {
    CREATED = 0,
    STARTING = 1,
    STARTED = 2,
    STOPPED = 3
};

enum AVScreenCaptureMixMode : int32_t {
    MIX_MODE = 0,
    MIC_MODE = 1,
    INNER_MODE = 2,
    INVAILD_MODE = 3
};

enum AVScreenCaptureAvType : int8_t {
    INVALID_TYPE = -1,
    AUDIO_TYPE = 0,
    VIDEO_TYPE = 1,
    AV_TYPE = 2
};

enum AVScreenCaptureDataMode : int8_t {
    BUFFER_MODE = 0,
    SUFFACE_MODE = 1,
    FILE_MODE = 2
};

enum StopReason: int8_t {
    NORMAL_STOPPED = 0,
    RECEIVE_USER_PRIVACY_AUTHORITY_FAILED = 1,
    POST_START_SCREENCAPTURE_HANDLE_FAILURE = 2,
    REQUEST_USER_PRIVACY_AUTHORITY_FAILED = 3,
    STOP_REASON_INVALID = 4
};

struct SurfaceBufferEntry {
    SurfaceBufferEntry(sptr<OHOS::SurfaceBuffer> buf, int32_t fence, int64_t timeStamp, OHOS::Rect& damage)
        : buffer(std::move(buf)), flushFence(fence), timeStamp(timeStamp), damageRect(damage) {}
    ~SurfaceBufferEntry() noexcept = default;

    sptr<OHOS::SurfaceBuffer> buffer;
    int32_t flushFence;
    int64_t timeStamp = 0;
    OHOS::Rect damageRect = {0, 0, 0, 0};
};

struct StatisticalEventInfo {
    int32_t errCode = 0;
    std::string errMsg;
    int32_t captureDuration = -1;
    bool userAgree = false;
    bool requireMic = false;
    bool enableMic = false;
    std::string videoResolution;
    StopReason stopReason = StopReason::STOP_REASON_INVALID;
    int32_t startLatency = -1;
};

class ScreenCapBufferConsumerListener : public IBufferConsumerListener {
public:
    ScreenCapBufferConsumerListener(
        sptr<Surface> consumer, const std::shared_ptr<ScreenCaptureCallBack> &screenCaptureCb)
        : consumer_(consumer), screenCaptureCb_(screenCaptureCb) {}
    ~ScreenCapBufferConsumerListener()
    {
        std::unique_lock<std::mutex> lock(bufferMutex_);
        ReleaseBuffer();
    }

    void OnBufferAvailable() override;
    int32_t AcquireVideoBuffer(sptr<OHOS::SurfaceBuffer> &surfaceBuffer, int32_t &fence, int64_t &timestamp,
        OHOS::Rect &damage);
    int32_t ReleaseVideoBuffer();
    int32_t Release();

private:
    int32_t ReleaseBuffer()
    {
        while (!availBuffers_.empty()) {
            if (consumer_ != nullptr) {
                consumer_->ReleaseBuffer(availBuffers_.front()->buffer,
                    availBuffers_.front()->flushFence);
            }
            availBuffers_.pop();
        }
        return MSERR_OK;
    }

private:
    std::mutex mutex_;
    sptr<OHOS::Surface> consumer_ = nullptr;
    std::shared_ptr<ScreenCaptureCallBack> screenCaptureCb_ = nullptr;

    std::mutex bufferMutex_;
    std::condition_variable bufferCond_;
    std::queue<std::unique_ptr<SurfaceBufferEntry>> availBuffers_;

    static constexpr uint32_t MAX_BUFFER_SIZE = 3;
    static constexpr uint32_t OPERATION_TIMEOUT_IN_MS = 1000; // 1000ms
};

class ScreenCaptureObserverCallBack : public InCallObserverCallBack {
public:
    explicit ScreenCaptureObserverCallBack(std::weak_ptr<ScreenCaptureServer> screenCaptureServer);
    ~ScreenCaptureObserverCallBack() = default;
    bool StopAndRelease() override;

private:
    std::weak_ptr<ScreenCaptureServer> screenCaptureServer_;
};

class AudioDataSource : public IAudioDataSource {
public:
    AudioDataSource(AVScreenCaptureMixMode type, ScreenCaptureServer* screenCaptureServer) : type_(type),
        screenCaptureServer_(screenCaptureServer) {}

    int32_t ReadAt(std::shared_ptr<AVBuffer> buffer, uint32_t length) override;
    int32_t GetSize(int64_t &size) override;

private:
    void MixAudio(char** srcData, char* mixData, int channels, int bufferSize);

    AVScreenCaptureMixMode type_;
    ScreenCaptureServer* screenCaptureServer_;
};

class ScreenCaptureServer : public std::enable_shared_from_this<ScreenCaptureServer>,
        public IScreenCaptureService, public NoCopyable {
public:
    static std::shared_ptr<IScreenCaptureService> Create();
    static int32_t ReportAVScreenCaptureUserChoice(int32_t sessionId, const std::string &choice);
    ScreenCaptureServer();
    ~ScreenCaptureServer();

    int32_t SetCaptureMode(CaptureMode captureMode) override;
    int32_t SetDataType(DataType dataType) override;
    int32_t SetRecorderInfo(RecorderInfo recorderInfo) override;
    int32_t SetOutputFile(int32_t outputFd) override;
    int32_t InitAudioEncInfo(AudioEncInfo audioEncInfo) override;
    int32_t InitAudioCap(AudioCaptureInfo audioInfo) override;
    int32_t InitVideoEncInfo(VideoEncInfo videoEncInfo) override;
    int32_t InitVideoCap(VideoCaptureInfo videoInfo) override;
    int32_t StartScreenCapture(bool isPrivacyAuthorityEnabled) override;
    int32_t StartScreenCaptureWithSurface(sptr<Surface> surface, bool isPrivacyAuthorityEnabled) override;
    int32_t StopScreenCapture() override;
    int32_t SetScreenCaptureCallback(const std::shared_ptr<ScreenCaptureCallBack> &callback) override;
    int32_t AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audioBuffer, AudioCaptureSourceType type) override;
    int32_t AcquireVideoBuffer(sptr<OHOS::SurfaceBuffer> &surfaceBuffer, int32_t &fence,
        int64_t &timestamp, OHOS::Rect &damage) override;
    int32_t ReleaseAudioBuffer(AudioCaptureSourceType type) override;
    int32_t ReleaseVideoBuffer() override;
    int32_t SetMicrophoneEnabled(bool isMicrophone) override;
    int32_t SetCanvasRotation(bool canvasRotation) override;
    void Release() override;
    int32_t ExcludeContent(ScreenCaptureContentFilter &contentFilter) override;

    void SetSessionId(int32_t sessionId);
    int32_t OnReceiveUserPrivacyAuthority(bool isAllowed);
    int32_t StopScreenCaptureByEvent(AVScreenCaptureStateCode stateCode);
    void UpdateMicrophoneEnabled();

    int32_t AcquireAudioBufferMix(std::shared_ptr<AudioBuffer> &innerAudioBuffer,
        std::shared_ptr<AudioBuffer> &micAudioBuffer, AVScreenCaptureMixMode type);
    int32_t ReleaseAudioBufferMix(AVScreenCaptureMixMode type);
    int32_t GetInnerAudioCaptureBufferSize(size_t &size);
    int32_t GetMicAudioCaptureBufferSize(size_t &size);

private:
    int32_t StartScreenCaptureInner(bool isPrivacyAuthorityEnabled);
    int32_t OnStartScreenCapture();
    void PostStartScreenCapture(bool isSuccess);
    int32_t InitRecorderInfo(std::shared_ptr<IRecorderService> &recorder, AudioCaptureInfo audioInfo);
    int32_t InitRecorder();
    int32_t StartScreenCaptureFile();
    int32_t StartScreenCaptureStream();
    int32_t StartAudioCapture();
    int32_t StartVideoCapture();
    int32_t StartHomeVideoCapture();
    int32_t StopScreenCaptureInner(AVScreenCaptureStateCode stateCode);
    void PostStopScreenCapture(AVScreenCaptureStateCode stateCode);
    int32_t StopAudioCapture();
    int32_t StopVideoCapture();
    int32_t StopScreenCaptureRecorder();
    int32_t CheckAllParams();
    int32_t CheckCaptureStreamParams();
    int32_t CheckCaptureFileParams();
    int32_t SetCanvasRotationInner();
    void InitAppInfo();
    void CloseFd();
    void ReleaseInner();

    VirtualScreenOption InitVirtualScreenOption(const std::string &name, sptr<OHOS::Surface> consumer);
    int32_t GetMissionIds(std::vector<uint64_t> &missionIds);
    int32_t MakeVirtualScreenMirror();
    int32_t CreateVirtualScreen(const std::string &name, sptr<OHOS::Surface> consumer);
    void DestroyVirtualScreen();

    bool CheckScreenCapturePermission();
    bool IsUserPrivacyAuthorityNeeded();
    bool UpdatePrivacyUsingPermissionState(VideoPermissionState state);
    int32_t RequestUserPrivacyAuthority();
    int32_t StartPrivacyWindow();
#ifdef SUPPORT_SCREEN_CAPTURE_WINDOW_NOTIFICATION
    int32_t TryStartNotification();
#endif
    int32_t StartNotification();
    std::shared_ptr<NotificationLocalLiveViewContent> GetLocalLiveViewContent();
    void UpdateLiveViewContent();
    std::shared_ptr<PixelMap> GetPixelMap(std::string path);
    std::shared_ptr<PixelMap> GetPixelMapSvg(std::string path, int32_t width, int32_t height);
    void ResSchedReportData(int64_t value, std::unordered_map<std::string, std::string> payload);
    int64_t GetCurrentMillisecond();
    void SetMetaDataReport();
    void SetErrorInfo(int32_t errCode, std::string errMsg, StopReason stopReason, bool userAgree);

private:
    std::mutex mutex_;
    std::mutex cbMutex_;
    std::shared_ptr<ScreenCaptureObserverCallBack> screenCaptureObserverCb_ = nullptr;
    std::shared_ptr<ScreenCaptureCallBack> screenCaptureCb_ = nullptr;
    bool canvasRotation_ = false;
    bool isMicrophoneOn_ = true;
    bool isPrivacyAuthorityEnabled_ = false;

    int32_t sessionId_ = 0;
    int32_t notificationId_ = 0;
    std::string callingLabel_;
    std::string liveViewText_;
    std::atomic<int32_t> micCount_{0};
    int32_t density_ = 0;
    int32_t capsuleVpSize_ = 18;
    int32_t capsulePxSize_ = 0;

    /* used for both CAPTURE STREAM and CAPTURE FILE */
    OHOS::AudioStandard::AppInfo appInfo_;
    std::string appName_ = "";
    AVScreenCaptureConfig captureConfig_;
    AVScreenCaptureAvType avType_ = AVScreenCaptureAvType::INVALID_TYPE;
    AVScreenCaptureDataMode dataMode_ = AVScreenCaptureDataMode::BUFFER_MODE;
    StatisticalEventInfo statisticalEventInfo_;
    sptr<OHOS::Surface> consumer_ = nullptr;
    bool isConsumerStart_ = false;
    ScreenId screenId_ = SCREEN_ID_INVALID;
    std::vector<uint64_t> missionIds_;
    ScreenCaptureContentFilter contentFilter_;
    AVScreenCaptureState captureState_ = AVScreenCaptureState::CREATED;
    std::shared_ptr<NotificationLocalLiveViewContent> localLiveViewContent_;
    int64_t startTime_ = 0;
    std::string bundleName_;

    /* used for CAPTURE STREAM */
    sptr<IBufferConsumerListener> surfaceCb_ = nullptr;
    sptr<OHOS::Surface> surface_ = nullptr;
    bool isSurfaceMode_ = false;
    std::shared_ptr<AudioCapturerWrapper> innerAudioCapture_;
    std::shared_ptr<AudioCapturerWrapper> micAudioCapture_;

    /* used for CAPTURE FILE */
    std::shared_ptr<IRecorderService> recorder_ = nullptr;
    std::string url_;
    OutputFormatType fileFormat_ = OutputFormatType::FORMAT_DEFAULT;
    int32_t outputFd_ = -1;
    int32_t audioSourceId_ = 0;
    int32_t videoSourceId_ = 0;
    std::shared_ptr<AudioDataSource> audioSource_ = nullptr;

    /* used for DFX events */
    uint64_t instanceId_ = 0;
private:
    static int32_t CheckAudioCapParam(const AudioCaptureInfo &audioCapInfo);
    static int32_t CheckVideoCapParam(const VideoCaptureInfo &videoCapInfo);
    static int32_t CheckAudioEncParam(const AudioEncInfo &audioEncInfo);
    static int32_t CheckVideoEncParam(const VideoEncInfo &videoEncInfo);
    static int32_t CheckAudioCapInfo(AudioCaptureInfo &audioCapInfo);
    static int32_t CheckVideoCapInfo(VideoCaptureInfo &videoCapInfo);
    static int32_t CheckAudioEncInfo(AudioEncInfo &audioEncInfo);
    static int32_t CheckVideoEncInfo(VideoEncInfo &videoEncInfo);
    static int32_t CheckCaptureMode(CaptureMode captureMode);
    static int32_t CheckDataType(DataType dataType);

private:
    static constexpr int32_t ROOT_UID = 0;
    static constexpr int32_t AUDIO_BITRATE_MIN = 8000;
    static constexpr int32_t AUDIO_BITRATE_MAX = 384000;
    static constexpr int32_t VIDEO_BITRATE_MIN = 1;
    static constexpr int32_t VIDEO_BITRATE_MAX = 30000000;
    static constexpr int32_t VIDEO_FRAME_RATE_MIN = 1;
    static constexpr int32_t VIDEO_FRAME_RATE_MAX = 60;
    static constexpr int32_t VIDEO_FRAME_WIDTH_MAX = 10240;
    static constexpr int32_t VIDEO_FRAME_HEIGHT_MAX = 4320;
    static constexpr int32_t SESSION_ID_INVALID = -1;
};
} // namespace Media
} // namespace OHOS
#endif // SCREEN_CAPTURE_SERVICE_SERVER_H
