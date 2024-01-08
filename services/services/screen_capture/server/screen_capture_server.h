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
#include "securec.h"
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

namespace OHOS {
namespace Media {
using namespace Rosen;
using namespace AudioStandard;
using OHOS::Security::AccessToken::PrivacyKit;

struct SurfaceBufferEntry {
    SurfaceBufferEntry(sptr<OHOS::SurfaceBuffer> buf, int32_t fence, int64_t timeStamp, OHOS::Rect& damage)
        : buffer(std::move(buf)), flushFence(fence), timeStamp(timeStamp), damageRect(damage)
    {
    }
    ~SurfaceBufferEntry() noexcept = default;

    sptr<OHOS::SurfaceBuffer> buffer;
    int32_t flushFence;
    int64_t timeStamp = 0;
    OHOS::Rect damageRect = {0, 0, 0, 0};
};

enum VideoPermissionState : int32_t {
    START_VIDEO = 0,
    STOP_VIDEO = 1
};

class ScreenCapBufferConsumerListener : public IBufferConsumerListener {
public:
    ScreenCapBufferConsumerListener(sptr<Surface> consumer,
        const std::shared_ptr<ScreenCaptureCallBack> &screenCaptureCb) : consumer_(consumer),
        screenCaptureCb_(screenCaptureCb)
    {
    }
    ~ScreenCapBufferConsumerListener()
    {
        std::unique_lock<std::mutex> vlock(vmutex_);
        while (!availableVideoBuffers_.empty()) {
            if (consumer_ != nullptr) {
                consumer_->ReleaseBuffer(availableVideoBuffers_.front()->buffer,
                    availableVideoBuffers_.front()->flushFence);
            }
            availableVideoBuffers_.pop();
        }
    }

    void OnBufferAvailable() override;
    int32_t AcquireVideoBuffer(sptr<OHOS::SurfaceBuffer> &surfaceBuffer, int32_t &fence,
                               int64_t &timestamp, OHOS::Rect &damage);
    int32_t ReleaseVideoBuffer();
    int32_t Release();
    static constexpr uint32_t MAX_BUFFER_SIZE = 3;

private:
    sptr<OHOS::Surface> consumer_ = nullptr;
    std::mutex cbMutex_;
    std::mutex vmutex_;
    std::condition_variable bufferCond_;
    std::shared_ptr<ScreenCaptureCallBack> screenCaptureCb_ = nullptr;
    std::queue<std::unique_ptr<SurfaceBufferEntry>> availableVideoBuffers_;
};

class AudioCapturerCallbackImpl : public AudioCapturerCallback {
public:
    void OnInterrupt(const InterruptEvent &interruptEvent) override;
    void OnStateChange(const CapturerState state) override;
};

class ScreenCaptureServer : public IScreenCaptureService, public NoCopyable {
public:
    static std::shared_ptr<IScreenCaptureService> Create();
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
    int32_t StartScreenCapture() override;
    int32_t StopScreenCapture() override;
    int32_t SetScreenCaptureCallback(const std::shared_ptr<ScreenCaptureCallBack> &callback) override;
    int32_t AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audioBuffer, AudioCaptureSourceType type) override;
    int32_t AcquireVideoBuffer(sptr<OHOS::SurfaceBuffer> &surfaceBuffer, int32_t &fence,
                               int64_t &timestamp, OHOS::Rect &damage) override;
    int32_t ReleaseAudioBuffer(AudioCaptureSourceType type) override;
    int32_t ReleaseVideoBuffer() override;
    int32_t SetMicrophoneEnabled(bool isMicrophone) override;
    void Release() override;

private:
    bool CheckAudioCaptureMicPermission();
    bool CheckScreenCapturePermission();
    bool GetUsingPermissionFromPrivacy(VideoPermissionState state);
    int32_t CheckVideoParam(VideoCaptureInfo videoInfo);
    int32_t CheckAudioParam(AudioCaptureInfo audioInfo);
    std::shared_ptr<AudioCapturer> CreateAudioCapture(AudioCaptureInfo audioInfo);
    int32_t InitRecorder();
    int32_t StartAudioCapture();
    int32_t StartAudioInnerCapture();
    int32_t StartVideoCapture();
    int32_t StartHomeVideoCapture();
    int32_t StartHomeVideoCaptureFile();
    int32_t CreateVirtualScreen(const std::string name, sptr<OHOS::Surface> consumer);
    VirtualScreenOption InitVirtualScreenOption(const std::string name, sptr<OHOS::Surface> consumer);
    int32_t GetMissionIds(std::vector<uint64_t> &missionIds);
    int32_t StopAudioCapture();
    int32_t StopVideoCapture();
    int32_t StopScreenCaptureRecorder();
    void ReleaseAudioCapture();
    void ReleaseVideoCapture();

    std::shared_ptr<ScreenCaptureCallBack> screenCaptureCb_ = nullptr;
    std::mutex mutex_;
    std::mutex audioMutex_;
    std::mutex audioInnerMutex_;
    std::mutex cbMutex_;
    std::condition_variable bufferCond_;
    std::condition_variable bufferInnerCond_;
    /* use Mic AudioCaptureHandler */
    std::shared_ptr<AudioCapturer> audioMicCapturer_ = nullptr;
    std::shared_ptr<AudioCapturerCallbackImpl> cb1_ = nullptr;
    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::thread> readAudioLoop_ = nullptr;
    std::queue<std::shared_ptr<AudioBuffer>> availableAudioBuffers_;
    bool isMicrophoneOn = true;
    /* use Inner AudioCaptureHandler */
    std::shared_ptr<AudioCapturer> audioInnerCapturer_ = nullptr;
    std::atomic<bool> isInnerRunning_ = false;
    std::unique_ptr<std::thread> readInnerAudioLoop_ = nullptr;
    std::queue<std::shared_ptr<AudioBuffer>> availableInnerAudioBuffers_;
    /* use VideoCapture */
    sptr<OHOS::Surface> consumer_ = nullptr;
    ScreenCapBufferConsumerListener* surfaceCb_ = nullptr;
    ScreenId screenId_ = SCREEN_ID_INVALID;
    bool isConsumerStart_ = false;
    bool isAudioStart_ = false;
    bool isAudioInnerStart_ = false;
    AudioCaptureSourceType audioCurrentInnerType_;
    VideoCaptureInfo videoInfo_;
    Security::AccessToken::AccessTokenID clientTokenId = 0;
    CaptureMode captureMode_ = CAPTURE_HOME_SCREEN;
    int32_t outputFd_ = -1;
    std::shared_ptr<IRecorderService> recorder_ = nullptr;
    int32_t audioSourceId_;
    int32_t videoSourceId_;
    AudioCaptureInfo audioInfo_;
    DataType dataType_ = ORIGINAL_STREAM;
    std::string url_;
    OutputFormatType fileFormat_;
    AudioEncInfo audioEncInfo_;
    VideoEncInfo videoEncInfo_;
    std::vector<uint64_t> missionIds_;
    const int32_t audioBitrateMin_ = 8000;
    const int32_t audioBitrateMax_ = 384000;
    const int32_t videoBitrateMin_ = 1;
    const int32_t videoBitrateMax_ = 3000000;
    const int32_t videoFrameRateMin_ = 1;
    const int32_t videoFrameRateMax_ = 30;
    const std::string MP4 = "mp4";
    const std::string M4A = "m4a";
    OHOS::AudioStandard::AppInfo appinfo_;

    static constexpr uint32_t MAX_AUDIO_BUFFER_SIZE = 128;
    static constexpr uint64_t SEC_TO_NANOSECOND = 1000000000;
};
} // namespace Media
} // namespace OHOS
#endif // SCREEN_CAPTURE_SERVICE_SERVER_H
