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

#include "screen_capture_server.h"
#include "media_log.h"
#include "media_errors.h"
#include "uri_helper.h"
#include "media_dfx.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ScreenCaptureServer"};
}

namespace OHOS {
namespace Media {
const int32_t ROOT_UID = 0;
std::shared_ptr<IScreenCaptureService> ScreenCaptureServer::Create()
{
    std::shared_ptr<IScreenCaptureService> server = std::make_shared<ScreenCaptureServer>();
    CHECK_AND_RETURN_RET_LOG(server != nullptr, nullptr, "Failed to new ScreenCaptureServer");
    return server;
}

ScreenCaptureServer::ScreenCaptureServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

ScreenCaptureServer::~ScreenCaptureServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));

    std::lock_guard<std::mutex> lock(mutex_);

    ReleaseAudioCapture();
    ReleaseVideoCapture();
}

int32_t ScreenCaptureServer::SetCaptureMode(CaptureMode captureMode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if ((captureMode > CAPTURE_SPECIFIED_WINDOW) || (captureMode < CAPTURE_HOME_SCREEN)) {
        MEDIA_LOGI("invalid capture mode");
        return MSERR_INVALID_VAL;
    }
    if (captureMode == CAPTURE_SPECIFIED_SCREEN || captureMode == CAPTURE_SPECIFIED_WINDOW) {
        MEDIA_LOGI("the capture Mode:%{public}d still not supported", captureMode);
        return MSERR_UNSUPPORT;
    }
    captureMode_ = captureMode;
    return MSERR_OK;
}

int32_t ScreenCaptureServer::SetScreenCaptureCallback(const std::shared_ptr<ScreenCaptureCallBack> &callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    {
        std::lock_guard<std::mutex> cbLock(cbMutex_);
        screenCaptureCb_ = callback;
    }
    return MSERR_OK;
}

bool ScreenCaptureServer::CheckScreenCapturePermission()
{
    auto callerUid = IPCSkeleton::GetCallingUid();
    // Root users should be whitelisted
    if (callerUid == ROOT_UID) {
        MEDIA_LOGI("Root user. Permission Granted");
        return true;
    }

    Security::AccessToken::AccessTokenID tokenCaller = IPCSkeleton::GetCallingTokenID();
    clientTokenId = tokenCaller;
    int result = Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenCaller,
        "ohos.permission.CAPTURE_SCREEN");
    if (result == Security::AccessToken::PERMISSION_GRANTED) {
        MEDIA_LOGI("user have the right to access capture screen!");
    } else {
        MEDIA_LOGE("user do not have the right to access capture screen!");
        return false;
    }

    if (!PrivacyKit::IsAllowedUsingPermission(clientTokenId, "ohos.permission.CAPTURE_SCREEN")) {
        MEDIA_LOGE("app background, not allow using perm for client %{public}d", clientTokenId);
    }
    return true;
}

int32_t ScreenCaptureServer::CheckAudioParam(AudioCaptureInfo audioInfo)
{
    std::vector<AudioSamplingRate> supportedSamplingRates = AudioStandard::AudioCapturer::GetSupportedSamplingRates();
    bool foundSupportSample = false;
    for (auto iter = supportedSamplingRates.begin(); iter != supportedSamplingRates.end(); ++iter) {
        if (static_cast<AudioSamplingRate>(audioInfo.audioSampleRate) == *iter) {
            foundSupportSample = true;
        }
    }
    if (!foundSupportSample) {
        MEDIA_LOGE("set audioSampleRate is not support");
        return MSERR_UNSUPPORT;
    }

    std::vector<AudioChannel> supportedChannelList = AudioStandard::AudioCapturer::GetSupportedChannels();
    bool foundSupportChannel = false;
    for (auto iter = supportedChannelList.begin(); iter != supportedChannelList.end(); ++iter) {
        if (static_cast<AudioChannel>(audioInfo.audioChannels) == *iter) {
            foundSupportChannel = true;
        }
    }
    if (!foundSupportChannel) {
        MEDIA_LOGE("set audioChannel is not support");
        return MSERR_UNSUPPORT;
    }

    if ((audioInfo.audioSource <= SOURCE_INVALID) || (audioInfo.audioSource > APP_PLAYBACK)) {
        MEDIA_LOGE("audioSource is invalid");
        return MSERR_INVALID_VAL;
    }
    return MSERR_OK;
}

int32_t ScreenCaptureServer::CheckVideoParam(VideoCaptureInfo videoInfo)
{
    if ((videoInfo.videoFrameWidth <= 0) || (videoInfo.videoFrameHeight <= 0)) {
        MEDIA_LOGE("videoInfo size is invalid, videoFrameWidth:%{public}d,videoFrameHeight:%{public}d",
            videoInfo.videoFrameWidth, videoInfo.videoFrameHeight);
        return MSERR_INVALID_VAL;
    }

    if (videoInfo.videoSource != VIDEO_SOURCE_SURFACE_RGBA) {
        MEDIA_LOGE("videoSource is invalid");
        return MSERR_INVALID_VAL;
    }
    return MSERR_OK;
}

bool ScreenCaptureServer::CheckAudioCaptureMicPermission()
{
    auto callerUid = IPCSkeleton::GetCallingUid();
    // Root users should be whitelisted
    if (callerUid == ROOT_UID) {
        MEDIA_LOGI("Root user. Permission Granted");
        return true;
    }

    Security::AccessToken::AccessTokenID tokenCaller = IPCSkeleton::GetCallingTokenID();
    int result = Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenCaller,
        "ohos.permission.MICROPHONE");
    if (result == Security::AccessToken::PERMISSION_GRANTED) {
        MEDIA_LOGI("user have the right to access microphone !");
        return true;
    } else {
        MEDIA_LOGE("user do not have the right to access microphone!");
        return false;
    }
}

int32_t ScreenCaptureServer::InitAudioCap(AudioCaptureInfo audioInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("ScreenCaptureServer::InitAudioCap");
    int ret = MSERR_OK;
    ret = CheckAudioParam(audioInfo);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "CheckAudioParam failed");

    switch (audioInfo.audioSource) {
        case SOURCE_DEFAULT:
        case MIC: {
            if (!CheckAudioCaptureMicPermission()) {
                return MSERR_INVALID_OPERATION;
            }
            audioMicCapturer_ = CreateAudioCapture(audioInfo);
            CHECK_AND_RETURN_RET_LOG(audioMicCapturer_ != nullptr, MSERR_UNKNOWN, "initMicAudioCap failed");
            break;
        }
        case ALL_PLAYBACK:
        case APP_PLAYBACK: {
            audioInnerCapturer_ = CreateAudioCapture(audioInfo);
            audioCurrentInnerType_ = audioInfo.audioSource;
            CHECK_AND_RETURN_RET_LOG(audioInnerCapturer_ != nullptr, MSERR_UNKNOWN, "initInnerAudioCap failed");
            break;
        }
        default:
            MEDIA_LOGE("the audio source Type is invalid");
            return MSERR_INVALID_OPERATION;
    }
    return MSERR_OK;
}

std::shared_ptr<AudioCapturer> ScreenCaptureServer::CreateAudioCapture(AudioCaptureInfo audioInfo)
{
    AudioCapturerOptions capturerOptions;
    std::shared_ptr<AudioCapturer> audioCapture;
    capturerOptions.streamInfo.samplingRate = static_cast<AudioSamplingRate>(audioInfo.audioSampleRate);
    capturerOptions.streamInfo.encoding = AudioEncodingType::ENCODING_PCM;
    capturerOptions.streamInfo.format = AudioSampleFormat::SAMPLE_S16LE;
    capturerOptions.streamInfo.channels = static_cast<AudioChannel>(audioInfo.audioChannels);
    if (audioInfo.audioSource == MIC) {
        /* Audio SourceType Mic is 0 */
        capturerOptions.capturerInfo.sourceType = static_cast<SourceType>(audioInfo.audioSource - MIC);
    } else {
        capturerOptions.capturerInfo.sourceType = static_cast<SourceType>(audioInfo.audioSource);
    }
    capturerOptions.capturerInfo.capturerFlags = 0;
    AppInfo appinfo_;
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    int32_t appUid = IPCSkeleton::GetCallingUid();
    int32_t appPid = IPCSkeleton::GetCallingPid();
    appinfo_.appUid = appUid;
    appinfo_.appTokenId = tokenId;
    appinfo_.appPid = appPid;
    audioCapture = AudioCapturer::Create(capturerOptions, appinfo_);
    CHECK_AND_RETURN_RET_LOG(audioCapture != nullptr, nullptr, "initAudioCap failed");

    int ret = audioCapture->SetCapturerCallback(cb1_);
    CHECK_AND_RETURN_RET_LOG(ret != MSERR_OK, nullptr, "SetCapturerCallback failed");
    return audioCapture;
}

int32_t ScreenCaptureServer::InitVideoCap(VideoCaptureInfo videoInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("ScreenCaptureServer::InitVideoCap");
    if (!CheckScreenCapturePermission()) {
        return MSERR_INVALID_OPERATION;
    }

    int ret = CheckVideoParam(videoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "CheckVideoParam failed");

    consumer_ = OHOS::Surface::CreateSurfaceAsConsumer();
    if (consumer_ == nullptr) {
        MEDIA_LOGE("CreateSurfaceAsConsumer failed");
        return MSERR_NO_MEMORY;
    }
    videoInfo_ = videoInfo;
    return MSERR_OK;
}

bool ScreenCaptureServer::GetUsingPermissionFromPrivacy(VideoPermissionState state)
{
    auto callerUid = IPCSkeleton::GetCallingUid();
    // Root users should be whitelisted
    if (callerUid == ROOT_UID) {
        MEDIA_LOGI("Root user. Privacy Granted");
        return true;
    }

    if (clientTokenId == 0) {
        clientTokenId = IPCSkeleton::GetCallingTokenID();
    }
    int res = 0;
    if (state == START_VIDEO) {
        res = PrivacyKit::StartUsingPermission(clientTokenId, "ohos.permission.CAPTURE_SCREEN");
        if (res != 0) {
            MEDIA_LOGE("start using perm error for client %{public}d", clientTokenId);
        }
    } else if (state == STOP_VIDEO) {
        res = PrivacyKit::StopUsingPermission(clientTokenId, "ohos.permission.CAPTURE_SCREEN");
        if (res != 0) {
            MEDIA_LOGE("stop using perm error for client %{public}d", clientTokenId);
        }
    }
    return true;
}

int32_t ScreenCaptureServer::StartScreenCapture()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("ScreenCaptureServer::StartScreenCapture");
    isAudioStart_ = true;
    if (audioMicCapturer_ != nullptr) {
        if (!audioMicCapturer_->Start()) {
            MEDIA_LOGE("Start mic audio stream failed");
            audioMicCapturer_->Release();
            audioMicCapturer_ = nullptr;
            isAudioStart_ = false;
        }
        if (isAudioStart_) {
            MEDIA_LOGE("Capturing started");
            isRunning_.store(true);
            readAudioLoop_ = std::make_unique<std::thread>(&ScreenCaptureServer::StartAudioCapture, this);
        }
    }
    isAudioInnerStart_ = true;
    if (audioInnerCapturer_ != nullptr) {
        if (!audioInnerCapturer_->Start()) {
            MEDIA_LOGE("Start inner audio stream failed");
            audioInnerCapturer_->Release();
            audioInnerCapturer_ = nullptr;
            isAudioInnerStart_ = false;
        }
        if (isAudioInnerStart_) {
            MEDIA_LOGE("Capturing started");
            isInnerRunning_.store(true);
            readInnerAudioLoop_ = std::make_unique<std::thread>(&ScreenCaptureServer::StartAudioInnerCapture, this);
        }
    }
    return StartVideoCapture();
}

int32_t ScreenCaptureServer::StartVideoCapture()
{
    if (!GetUsingPermissionFromPrivacy(START_VIDEO)) {
        MEDIA_LOGE("getUsingPermissionFromPrivacy");
    }
    if (captureMode_ == CAPTURE_HOME_SCREEN) {
        return StartHomeVideoCapture();
    } else {
        MEDIA_LOGE("The capture Mode Init still not supported,start failed");
        return MSERR_UNSUPPORT;
    }
    return MSERR_OK;
}

int32_t ScreenCaptureServer::StartHomeVideoCapture()
{
    if (consumer_ == nullptr) {
        MEDIA_LOGE("consumer_ is not created");
        return MSERR_INVALID_OPERATION;
    }
    surfaceCb_ = new ScreenCapBufferConsumerListener(consumer_, screenCaptureCb_);
    consumer_->RegisterConsumerListener((sptr<IBufferConsumerListener> &)surfaceCb_);
    auto producer = consumer_->GetProducer();
    auto psurface = OHOS::Surface::CreateSurfaceAsProducer(producer);
    CHECK_AND_RETURN_RET_LOG(psurface != nullptr, MSERR_UNKNOWN, "CreateSurfaceAsProducer failed");
    isConsumerStart_ = true;
    VirtualScreenOption virScrOption = {
        .name_ = "screen_capture",
        .width_ = videoInfo_.videoFrameWidth,
        .height_ = videoInfo_.videoFrameHeight,
        .density_ = 0,
        .surface_ = psurface,
        .flags_ = 0,
        .isForShot_ = true,
    };
    sptr<Rosen::Display> display = Rosen::DisplayManager::GetInstance().GetDefaultDisplaySync();
    if (display != nullptr) {
        MEDIA_LOGI("get displayinfo width:%{public}d,height:%{public}d,density:%{public}d", display->GetWidth(),
                   display->GetHeight(), display->GetDpi());
        virScrOption.density_ = display->GetDpi();
    }
    screenId_ = ScreenManager::GetInstance().CreateVirtualScreen(virScrOption);
    if (screenId_ < 0) {
        consumer_->UnregisterConsumerListener();
        isConsumerStart_ = false;
        MEDIA_LOGE("CreateVirtualScreen failed,release ConsumerListener");
        return MSERR_INVALID_OPERATION;
    }
    auto screen = ScreenManager::GetInstance().GetScreenById(screenId_);
    if (screen == nullptr) {
        consumer_->UnregisterConsumerListener();
        isConsumerStart_ = false;
        MEDIA_LOGE("GetScreenById failed,release ConsumerListener");
        return MSERR_INVALID_OPERATION;
    }
    std::vector<sptr<Screen>> screens;
    ScreenManager::GetInstance().GetAllScreens(screens);
    std::vector<ScreenId> mirrorIds;
    mirrorIds.push_back(screenId_);
    ScreenId mirrorGroup = static_cast<ScreenId>(1);
    ScreenManager::GetInstance().MakeMirror(screens[0]->GetId(), mirrorIds, mirrorGroup);

    return MSERR_OK;
}

int32_t ScreenCaptureServer::StartAudioInnerCapture()
{
    size_t bufferLen;
    CHECK_AND_RETURN_RET_LOG(audioInnerCapturer_ != nullptr, MSERR_NO_MEMORY, "audioInner capture is nullptr");
    if (audioInnerCapturer_->GetBufferSize(bufferLen) < 0) {
        MEDIA_LOGE("audioMicCapturer_ GetBufferSize failed");
        return MSERR_NO_MEMORY;
    }
    int32_t bufferRead = 0;
    Timestamp timestamp;
    int64_t audioTime;

    while (true) {
        if (audioInnerCapturer_ == nullptr || !(isAudioInnerStart_) || !(isInnerRunning_.load())) {
            MEDIA_LOGI("audioInnerCapturer_ has been released, end the capture!!");
            break;
        }
        uint8_t *buffer = static_cast<uint8_t *>(malloc(bufferLen));
        if (buffer == nullptr)
            return MSERR_NO_MEMORY;
        memset_s(buffer, bufferLen, 0, bufferLen);
        bufferRead = audioInnerCapturer_->Read(*buffer, bufferLen, true);
        if (bufferRead <= 0) {
            free(buffer);
            buffer = nullptr;
            MEDIA_LOGE("read audioBuffer failed, continue");
            continue;
        }
        audioInnerCapturer_->GetAudioTime(timestamp, Timestamp::Timestampbase::MONOTONIC);
        audioTime = timestamp.time.tv_nsec + timestamp.time.tv_sec * SEC_TO_NANOSECOND;
        std::unique_lock<std::mutex> lock(audioInnerMutex_);
        if (availableInnerAudioBuffers_.size() > MAX_AUDIO_BUFFER_SIZE) {
            free(buffer);
            buffer = nullptr;
            MEDIA_LOGE("no client consumer the buffer, drop the frame!!");
            continue;
        }
        availableInnerAudioBuffers_.push(std::make_unique<AudioBuffer>(buffer, bufferRead,
            audioTime, audioCurrentInnerType_));
        if (screenCaptureCb_ != nullptr) {
            std::lock_guard<std::mutex> cbLock(cbMutex_);
            screenCaptureCb_->OnAudioBufferAvailable(true, audioCurrentInnerType_);
        }
        bufferCond_.notify_all();
    }
    return MSERR_OK;
}

int32_t ScreenCaptureServer::StartAudioCapture()
{
    size_t bufferLen;
    CHECK_AND_RETURN_RET_LOG(audioMicCapturer_ != nullptr, MSERR_NO_MEMORY, "audiomic capture is nullptr");
    if (audioMicCapturer_->GetBufferSize(bufferLen) < 0) {
        MEDIA_LOGE("audioMicCapturer_ GetBufferSize failed");
        return MSERR_NO_MEMORY;
    }
    int32_t bufferRead = 0;
    Timestamp timestamp;
    int64_t audioTime;
    while (true) {
        if (audioMicCapturer_ == nullptr || !(isAudioStart_) || !(isRunning_.load())) {
            MEDIA_LOGI("audioMicCapturer_ has been released,end the capture!!");
            break;
        }
        uint8_t *buffer = static_cast<uint8_t *>(malloc(bufferLen));
        if (buffer == nullptr)
            return MSERR_NO_MEMORY;
        memset_s(buffer, bufferLen, 0, bufferLen);
        bufferRead = audioMicCapturer_->Read(*buffer, bufferLen, true);
        if (bufferRead <= 0) {
            free(buffer);
            buffer = nullptr;
            MEDIA_LOGE("read audioBuffer failed,continue");
            continue;
        }
        audioMicCapturer_->GetAudioTime(timestamp, Timestamp::Timestampbase::MONOTONIC);
        audioTime = timestamp.time.tv_nsec + timestamp.time.tv_sec * SEC_TO_NANOSECOND;
        std::unique_lock<std::mutex> lock(audioMutex_);
        if (availableAudioBuffers_.size() > MAX_AUDIO_BUFFER_SIZE) {
            free(buffer);
            buffer = nullptr;
            MEDIA_LOGE("no client consumer the buffer, drop the frame!!");
            continue;
        }
        if (!isMicrophoneOn) {
            memset_s(buffer, bufferLen, 0, bufferLen);
            availableAudioBuffers_.push(std::make_unique<AudioBuffer>(buffer, bufferRead, audioTime, MIC));
        } else {
            availableAudioBuffers_.push(std::make_unique<AudioBuffer>(buffer, bufferRead, audioTime, MIC));
        }
        if (screenCaptureCb_ != nullptr) {
            std::lock_guard<std::mutex> cbLock(cbMutex_);
            screenCaptureCb_->OnAudioBufferAvailable(true, MIC);
        }
        bufferCond_.notify_all();
    }
    return MSERR_OK;
}

int32_t ScreenCaptureServer::AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audioBuffer, AudioCaptureSourceType type)
{
    if (type == MIC) {
        using namespace std::chrono_literals;
        std::unique_lock<std::mutex> alock(audioMutex_);
        if (availableAudioBuffers_.empty()) {
            if (bufferCond_.wait_for(alock, 200ms) == std::cv_status::timeout) {
                MEDIA_LOGE("AcquireAudioBuffer timeout return!");
                return MSERR_UNKNOWN;
            }
        }
        if (availableAudioBuffers_.front() != nullptr) {
            audioBuffer = availableAudioBuffers_.front();
            return MSERR_OK;
        }
    } else if ((type == ALL_PLAYBACK) || (type == APP_PLAYBACK)) {
        using namespace std::chrono_literals;
        std::unique_lock<std::mutex> alock(audioInnerMutex_);
        if (availableInnerAudioBuffers_.empty()) {
            if (bufferCond_.wait_for(alock, 200ms) == std::cv_status::timeout) {
                MEDIA_LOGE("AcquireAudioBuffer timeout return!");
                return MSERR_UNKNOWN;
            }
        }
        if (availableInnerAudioBuffers_.front() != nullptr) {
            audioBuffer = availableInnerAudioBuffers_.front();
            return MSERR_OK;
        }
    } else {
        MEDIA_LOGE("The Type you request not support");
        return MSERR_UNSUPPORT;
    }
    return MSERR_UNKNOWN;
}

int32_t ScreenCaptureServer::ReleaseAudioBuffer(AudioCaptureSourceType type)
{
    if (type == MIC) {
        std::unique_lock<std::mutex> alock(audioMutex_);
        if (availableAudioBuffers_.empty()) {
            MEDIA_LOGE("availableAudioBuffers_ is empty, no frame need release");
            return MSERR_OK;
        }
        if (availableAudioBuffers_.front() != nullptr) {
            free(availableAudioBuffers_.front()->buffer);
            availableAudioBuffers_.front()->buffer = nullptr;
        }
        availableAudioBuffers_.pop();
    } else if ((type == ALL_PLAYBACK) || (type == APP_PLAYBACK)) {
        std::unique_lock<std::mutex> alock(audioInnerMutex_);
        if (availableInnerAudioBuffers_.empty()) {
            MEDIA_LOGE("availableAudioBuffers_ is empty, no frame need release");
            return MSERR_OK;
        }
        if (availableInnerAudioBuffers_.front() != nullptr) {
            free(availableInnerAudioBuffers_.front()->buffer);
            availableInnerAudioBuffers_.front()->buffer = nullptr;
        }
        availableInnerAudioBuffers_.pop();
    } else {
        MEDIA_LOGE("The Type you release not support");
        return MSERR_UNSUPPORT;
    }
    return MSERR_OK;
}

int32_t ScreenCaptureServer::AcquireVideoBuffer(sptr<OHOS::SurfaceBuffer> &surfaceBuffer, int32_t &fence,
                                                int64_t &timestamp, OHOS::Rect &damage)
{
    CHECK_AND_RETURN_RET_LOG(surfaceCb_ != nullptr, MSERR_NO_MEMORY,
                             "Failed to AcquireVideoBuffer,no callback object");
    surfaceCb_->AcquireVideoBuffer(surfaceBuffer, fence, timestamp, damage);
    if (surfaceBuffer != nullptr) {
        MEDIA_LOGD("getcurrent surfaceBuffer info, size:%{public}u", surfaceBuffer->GetSize());
        return MSERR_OK;
    }
    return MSERR_UNKNOWN;
}

int32_t ScreenCaptureServer::ReleaseVideoBuffer()
{
    CHECK_AND_RETURN_RET_LOG(surfaceCb_ != nullptr, MSERR_NO_MEMORY,
        "Failed to ReleaseVideoBuffer,no callback object");
    return surfaceCb_->ReleaseVideoBuffer();
}

int32_t ScreenCaptureServer::SetMicrophoneEnabled(bool isMicrophone)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGI("SetMicrophoneEnabled:%{public}d", isMicrophone);
    isMicrophoneOn = isMicrophone;
    return MSERR_OK;
}

int32_t ScreenCaptureServer::StopAudioCapture()
{
    isRunning_.store(false);
    if (readAudioLoop_ != nullptr && readAudioLoop_->joinable()) {
        readAudioLoop_->join();
        readAudioLoop_.reset();
        readAudioLoop_ = nullptr;
        audioMicCapturer_->Stop();
    }

    isInnerRunning_.store(false);
    if (readInnerAudioLoop_ != nullptr && readInnerAudioLoop_->joinable()) {
        readInnerAudioLoop_->join();
        readInnerAudioLoop_.reset();
        readInnerAudioLoop_ = nullptr;
        audioInnerCapturer_->Stop();
    }
    return MSERR_OK;
}

int32_t ScreenCaptureServer::StopVideoCapture()
{
    MEDIA_LOGI("StopVideoCapture");
    int32_t stopVideoSuccess = MSERR_OK;
    if (!GetUsingPermissionFromPrivacy(STOP_VIDEO)) {
        MEDIA_LOGE("getUsingPermissionFromPrivacy");
    }

    if ((screenId_ < 0) || (consumer_ == nullptr) || !isConsumerStart_) {
        MEDIA_LOGI("audio and video all start failed");
        stopVideoSuccess = MSERR_INVALID_OPERATION;
    }

    if (screenId_ != SCREEN_ID_INVALID) {
        ScreenManager::GetInstance().DestroyVirtualScreen(screenId_);
    }

    if ((consumer_ != nullptr) && isConsumerStart_) {
        isConsumerStart_ = false;
        consumer_->UnregisterConsumerListener();
    }

    if (surfaceCb_ != nullptr) {
        surfaceCb_->Release();
        surfaceCb_ = nullptr;
    }

    return stopVideoSuccess;
}

int32_t ScreenCaptureServer::StopScreenCapture()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("ScreenCaptureServer::StopScreenCapture");

    int32_t stopFlagSuccess = StopAudioCapture();
    CHECK_AND_RETURN_RET_LOG(stopFlagSuccess == MSERR_OK, stopFlagSuccess, "StopAudioCapture failed");

    stopFlagSuccess = StopVideoCapture();
    return stopFlagSuccess;
}

void ScreenCaptureServer::ReleaseAudioCapture()
{
    if ((audioMicCapturer_ != nullptr) && isAudioStart_) {
        isRunning_.store(false);
        if (readAudioLoop_ != nullptr && readAudioLoop_->joinable()) {
            readAudioLoop_->join();
            readAudioLoop_.reset();
            readAudioLoop_ = nullptr;
        }
        audioMicCapturer_->Release();
        isAudioStart_ = false;
        audioMicCapturer_ = nullptr;
    }

    if ((audioInnerCapturer_ != nullptr) && isAudioInnerStart_) {
        isInnerRunning_.store(false);
        if (readInnerAudioLoop_ != nullptr && readInnerAudioLoop_->joinable()) {
            readInnerAudioLoop_->join();
            readInnerAudioLoop_.reset();
            readInnerAudioLoop_ = nullptr;
        }
        audioInnerCapturer_->Release();
        isAudioInnerStart_ = false;
        audioInnerCapturer_ = nullptr;
    }

    std::unique_lock<std::mutex> alock(audioMutex_);
    while (!availableAudioBuffers_.empty()) {
        if (availableAudioBuffers_.front() != nullptr) {
            free(availableAudioBuffers_.front()->buffer);
            availableAudioBuffers_.front()->buffer = nullptr;
        }
        availableAudioBuffers_.pop();
    }

    std::unique_lock<std::mutex> alock_inner(audioInnerMutex_);
    while (!availableInnerAudioBuffers_.empty()) {
        if (availableInnerAudioBuffers_.front() != nullptr) {
            free(availableInnerAudioBuffers_.front()->buffer);
            availableInnerAudioBuffers_.front()->buffer = nullptr;
        }
        availableInnerAudioBuffers_.pop();
    }
}

void ScreenCaptureServer::ReleaseVideoCapture()
{
    if (screenId_ != SCREEN_ID_INVALID) {
        ScreenManager::GetInstance().DestroyVirtualScreen(screenId_);
    }

    if ((consumer_ != nullptr) && isConsumerStart_) {
        consumer_->UnregisterConsumerListener();
        isConsumerStart_ = false;
    }
    consumer_ = nullptr;
    if (surfaceCb_ != nullptr) {
        surfaceCb_->Release();
        surfaceCb_ = nullptr;
    }
}

void ScreenCaptureServer::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("ScreenCaptureServer::Release");

    screenCaptureCb_ = nullptr;
    ReleaseAudioCapture();
    ReleaseVideoCapture();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

void AudioCapturerCallbackImpl::OnInterrupt(const InterruptEvent &interruptEvent)
{
    MEDIA_LOGD("AudioCapturerCallbackImpl: OnInterrupt Hint : %{public}d eventType : %{public}d forceType : %{public}d",
        interruptEvent.hintType, interruptEvent.eventType, interruptEvent.forceType);
}

void AudioCapturerCallbackImpl::OnStateChange(const CapturerState state)
{
    MEDIA_LOGD("AudioCapturerCallbackImpl:: OnStateChange");
    switch (state) {
        case CAPTURER_PREPARED:
            MEDIA_LOGD("AudioCapturerCallbackImpl: OnStateChange CAPTURER_PREPARED");
            break;
        default:
            MEDIA_LOGD("AudioCapturerCallbackImpl: OnStateChange NOT A VALID state");
            break;
    }
}

void ScreenCapBufferConsumerListener::OnBufferAvailable()
{
    int32_t flushFence = 0;
    int64_t timestamp = 0;
    OHOS::Rect damage;
    OHOS::sptr<OHOS::SurfaceBuffer> buffer = nullptr;
    if (consumer_ == nullptr) {
        MEDIA_LOGE("consumer_ is nullptr");
        return;
    }
    consumer_->AcquireBuffer(buffer, flushFence, timestamp, damage);
    if (buffer != nullptr) {
        void* addr = buffer->GetVirAddr();
        uint32_t size = buffer->GetSize();
        if (addr != nullptr) {
            MEDIA_LOGD("consumer receive buffer length:%{public}u", size);
            std::unique_lock<std::mutex> vlock(vmutex_);
            if (availableVideoBuffers_.size() > MAX_BUFFER_SIZE) {
                consumer_->ReleaseBuffer(buffer, flushFence);
                MEDIA_LOGE("no client consumer the buffer,drop the frame!!");
                return;
            }
            availableVideoBuffers_.push(std::make_unique<SurfaceBufferEntry>(buffer, flushFence,
                timestamp, damage));
            if (screenCaptureCb_ != nullptr) {
                std::lock_guard<std::mutex> cbLock(cbMutex_);
                screenCaptureCb_->OnVideoBufferAvailable(true);
            } else {
                MEDIA_LOGE("no callback client consumer the buffer,drop the frame!!");
            }
            bufferCond_.notify_all();
        }
    } else {
        MEDIA_LOGE("consumer receive buffer failed");
        return;
    }
}

int32_t  ScreenCapBufferConsumerListener::AcquireVideoBuffer(sptr<OHOS::SurfaceBuffer> &surfaceBuffer, int32_t &fence,
                                                             int64_t &timestamp, OHOS::Rect &damage)
{
    using namespace std::chrono_literals;
    std::unique_lock<std::mutex> vlock(vmutex_);
    if (availableVideoBuffers_.empty()) {
        if (bufferCond_.wait_for(vlock, 1000ms) == std::cv_status::timeout) {
            return MSERR_UNKNOWN;
        }
    }
    surfaceBuffer = availableVideoBuffers_.front()->buffer;
    fence = availableVideoBuffers_.front()->flushFence;
    timestamp = availableVideoBuffers_.front()->timeStamp;
    damage = availableVideoBuffers_.front()->damageRect;
    return MSERR_OK;
}

int32_t  ScreenCapBufferConsumerListener::ReleaseVideoBuffer()
{
    std::unique_lock<std::mutex> vlock(vmutex_);
    if (availableVideoBuffers_.empty()) {
        MEDIA_LOGE("availableVideoBuffers_ is empty,no video frame need release");
        return MSERR_OK;
    }
    if (consumer_ != nullptr) {
        consumer_->ReleaseBuffer(availableVideoBuffers_.front()->buffer,
            availableVideoBuffers_.front()->flushFence);
    }
    availableVideoBuffers_.pop();
    return MSERR_OK;
}

int32_t ScreenCapBufferConsumerListener::Release()
{
    MEDIA_LOGI("release ScreenCapBufferConsumerListener");
    std::unique_lock<std::mutex> vlock(vmutex_);
    while (!availableVideoBuffers_.empty()) {
        if (consumer_ != nullptr) {
            consumer_->ReleaseBuffer(availableVideoBuffers_.front()->buffer,
                availableVideoBuffers_.front()->flushFence);
        }
        availableVideoBuffers_.pop();
    }
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
