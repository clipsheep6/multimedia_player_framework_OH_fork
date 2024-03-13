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
#include "ui_extension_ability_connection.h"
#include "extension_manager_client.h"
#include "image_source.h"
#include "image_type.h"
#include "pixel_map.h"
#include "media_log.h"
#include "media_errors.h"
#include "uri_helper.h"
#include "media_dfx.h"
#include "scope_guard.h"
#include "screen_capture_listener_proxy.h"

using OHOS::Rosen::DMError;
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ScreenCaptureServer"};
static std::map<int32_t, std::shared_ptr<OHOS::Media::ScreenCaptureServer>> serverMap;
std::atomic<int32_t> activeSessionId(-1);
std::mutex mutexGlobal;
}

namespace OHOS {
namespace Media {
constexpr int32_t ROOT_UID = 0;
const std::string MP4 = "mp4";
const std::string M4A = "m4a";
static const std::string USER_CHOICE_ALLOW = "true";
static const std::string USER_CHOICE_DENY = "false";
static const int32_t MAX_SESSION_ID = 256;
static const auto NOTIFICATION_SUBSCRIBER = NotificationSubscriber();

void NotificationSubscriber::OnConnected()
{
    MEDIA_LOGI("NotificationSubscriber OnConnected");
}

void NotificationSubscriber::OnDisconnected()
{
    MEDIA_LOGI("NotificationSubscriber OnDisconnected");
}

void NotificationSubscriber::OnResponse(int32_t notificationId,
                                        OHOS::sptr<OHOS::Notification::NotificationButtonOption> buttonOption)
{
    MEDIA_LOGI("NotificationSubscriber OnResponse notificationId : %{public}d ", notificationId);
    MEDIA_LOGI("NotificationSubscriber OnResponse ButtonName : %{public}s ", (buttonOption->GetButtonName()).c_str());
    if (buttonNameStop_.compare(buttonOption->GetButtonName()) == 0) {
        std::shared_ptr<OHOS::Media::ScreenCaptureServer> server = serverMap.at(notificationId);
        server->StopScreenCapture();
    }
}

void NotificationSubscriber::OnDied()
{
    MEDIA_LOGI("NotificationSubscriber OnDied");
}

std::shared_ptr<IScreenCaptureService> ScreenCaptureServer::Create()
{
    std::shared_ptr<ScreenCaptureServer> serverTemp = std::make_shared<ScreenCaptureServer>();
    CHECK_AND_RETURN_RET_LOG(serverTemp != nullptr, nullptr, "Failed to new ScreenCaptureServer");
    
    int32_t newSessionId = 0;
    for (int32_t i = 0; i < MAX_SESSION_ID; i++) {
        auto it = serverMap.find(newSessionId);
        if (it != serverMap.end()) {
            newSessionId++;
            continue;
        }
        serverMap.insert(std::make_pair(newSessionId, serverTemp));
        break;
    }
    MEDIA_LOGI("ScreenCaptureServer::Create newSessionId: %{public}d", newSessionId);
    MEDIA_LOGI("ScreenCaptureServer::Create serverMap size : %{public}s",
        std::to_string(serverMap.size()).c_str());
    serverTemp->SetSessionId(newSessionId);
    std::shared_ptr<IScreenCaptureService> server =
        std::static_pointer_cast<OHOS::Media::IScreenCaptureService>(serverTemp);
    return server;
}

int32_t ScreenCaptureServer::ReportAVScreenCaptureUserChoice(int32_t sessionId, std::string choice)
{
    std::lock_guard<std::mutex> lock(mutexGlobal);
    MEDIA_LOGI("ScreenCaptureServer::ReportAVScreenCaptureUserChoice user sessionId is : %{public}d", sessionId);
    MEDIA_LOGI("ScreenCaptureServer::ReportAVScreenCaptureUserChoice user choice is : %{public}s", choice.c_str());
    std::shared_ptr<ScreenCaptureServer> server;
    auto it = serverMap.find(sessionId);
    if (it != serverMap.end() && it->sencond != nullptr) {
        server = it->second;
    } else {
        MEDIA_LOGI("ScreenCaptureServer::ReportAVScreenCaptureUserChoice Failed to get report ScreenCaptureServer");
        return MSERR_UNKNOWN;
    }

    if (USER_CHOICE_ALLOW.compare(choice) == 0) {
        if (activeSessionId.load() >= 0) {
            std::shared_ptr<ScreenCaptureServer> currentServer = serverMap.at(activeSessionId.load());
            currentServer->StopScreenCapture();
        }
        activeSessionId.store(SESSION_ID_INVALID);
        int32_t ret = server->OnReceiveUserPrivacyAuthority(true);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret,
            "ReportAVScreenCaptureUserChoice user choice is true but start failed");
        activeSessionId.store(sessionId);
        MEDIA_LOGI("ScreenCaptureServer::ReportAVScreenCaptureUserChoice user choice is true and start success");
        return MSERR_OK;
    } else if (USER_CHOICE_DENY.compare(choice) == 0) {
        return server->OnReceiveUserPrivacyAuthority(false);
    }
    MEDIA_LOGI("ScreenCaptureServer::ReportAVScreenCaptureUserChoice user choice is not support");
    return MSERR_UNKNOWN;
}

ScreenCaptureServer::ScreenCaptureServer()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    InitAppInfo();
}

ScreenCaptureServer::~ScreenCaptureServer()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    std::lock_guard<std::mutex> lock(mutex_);
    OnStopScreenCapture();
}

void ScreenCaptureServer::SetSessionId(int32_t sessionId)
{
    sessionId_ = sessionId;
}

int32_t ScreenCaptureServer::SetCaptureMode(CaptureMode captureMode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "SetCaptureMode failed, capture is not CREATED, state:%{public}d, mode:%{public}d", captureState_, captureMode);
    MEDIA_LOGI("ScreenCaptureServer::SetCaptureMode start, captureMode:%{public}d", captureMode);
    int32_t ret = CheckCaptureMode(captureMode);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
    captureConfig_.captureMode = captureMode;
    return MSERR_OK;
}

int32_t ScreenCaptureServer::SetDataType(DataType dataType)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "SetDataType failed, capture is not CREATED, state:%{public}d, dataType:%{public}d", captureState_, dataType);
    MEDIA_LOGI("ScreenCaptureServer::SetDataType start, dataType:%{public}d", dataType);
    int32_t ret = CheckDataType(dataType);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
    captureConfig_.dataType = dataType;
    return MSERR_OK;
}

int32_t ScreenCaptureServer::SetRecorderInfo(RecorderInfo recorderInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "SetRecorderInfo failed, capture is not CREATED, state:%{public}d", captureState_);
    MEDIA_LOGI("ScreenCaptureServer::SetRecorderInfo start");
    url_ = recorderInfo.url;

    if (MP4.compare(recorderInfo.fileFormat) == 0) {
        fileFormat_ = OutputFormatType::FORMAT_MPEG_4;
    } else if (M4A.compare(recorderInfo.fileFormat) == 0) {
        MEDIA_LOGI("only recorder audio, still not support");
        return MSERR_UNSUPPORT;
    } else {
        MEDIA_LOGE("invalid fileFormat type");
        return MSERR_INVALID_VAL;
    }
    return MSERR_OK;
}

int32_t ScreenCaptureServer::SetOutputFile(int32_t outputFd)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "SetOutputFile failed, capture is not CREATED, state:%{public}d", captureState_);
    MEDIA_LOGI("ScreenCaptureServer::SetOutputFile start");
    if (outputFd < 0) {
        MEDIA_LOGI("invalid outputFd");
        return MSERR_INVALID_VAL;
    }

    int flags = fcntl(outputFd, F_GETFL);
    if (flags == -1) {
        MEDIA_LOGE("Fail to get File Status Flags");
        return MSERR_INVALID_VAL;
    }
    if ((static_cast<unsigned int>(flags) & (O_RDWR | O_WRONLY)) == 0) {
        MEDIA_LOGE("File descriptor is not in read-write mode or write-only mode");
        return MSERR_INVALID_VAL;
    }

    if (outputFd_ > 0) {
        (void)::close(outputFd_);
    }
    outputFd_ = dup(outputFd);
    MEDIA_LOGI("ScreenCaptureServer SetOutputFile ok");
    return MSERR_OK;
}

int32_t ScreenCaptureServer::SetScreenCaptureCallback(const std::shared_ptr<ScreenCaptureCallBack> &callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "SetScreenCaptureCallback failed, capture is not CREATED, state:%{public}d", captureState_);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_INVALID_VAL,
        "SetScreenCaptureCallback failed, callback is nullptr, state:%{public}d", captureState_);
    MEDIA_LOGI("ScreenCaptureServer::SetScreenCaptureCallback start");
    screenCaptureCb_ = callback;
    return MSERR_OK;
}

int32_t ScreenCaptureServer::InitAudioEncInfo(AudioEncInfo audioEncInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "InitAudioEncInfo failed, capture is not CREATED, state:%{public}d", captureState_);
    MEDIA_LOGI("ScreenCaptureServer::InitAudioEncInfo start");
    MEDIA_LOGD("audioEncInfo audioBitrate:%{public}d, audioCodecformat:%{public}d", audioEncInfo.audioBitrate,
        audioEncInfo.audioCodecformat);
    int32_t ret = CheckAudioEncInfo(audioEncInfo);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "InitAudioEncInfo failed, ret:%{public}d", ret);
    captureConfig_.audioInfo.audioEncInfo = audioEncInfo;
    return MSERR_OK;
}

int32_t ScreenCaptureServer::InitVideoEncInfo(VideoEncInfo videoEncInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "InitVideoEncInfo failed, capture is not CREATED, state:%{public}d", captureState_);
    MEDIA_LOGI("ScreenCaptureServer::InitVideoEncInfo start");
    MEDIA_LOGD("videoEncInfo videoCodec:%{public}d,  videoBitrate:%{public}d, videoFrameRate:%{public}d",
        videoEncInfo.videoCodec, videoEncInfo.videoBitrate, videoEncInfo.videoFrameRate);
    int32_t ret = CheckVideoEncInfo(videoEncInfo);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "CheckVideoEncInfo failed, ret:%{public}d", ret);
    captureConfig_.videoInfo.videoEncInfo = videoEncInfo;
    return MSERR_OK;
}

bool ScreenCaptureServer::CheckScreenCapturePermission()
{
    // Root users should be whitelisted
    if (appInfo_.appUid == ROOT_UID) {
        MEDIA_LOGI("Root user. Permission Granted");
        return true;
    }

    int result = Security::AccessToken::AccessTokenKit::VerifyAccessToken(appInfo_.appTokenId,
        "ohos.permission.CAPTURE_SCREEN");
    if (result == Security::AccessToken::PERMISSION_GRANTED) {
        MEDIA_LOGI("user have the right to access capture screen!");
        return true;
    } else {
        MEDIA_LOGE("user do not have the right to access capture screen!");
        return false;
    }
}

int32_t ScreenCaptureServer::CheckCaptureMode(CaptureMode captureMode)
{
    MEDIA_LOGD("CheckCaptureMode start, captureMode:%{public}d", captureMode);
    if ((captureMode > CAPTURE_SPECIFIED_WINDOW) || (captureMode < CAPTURE_HOME_SCREEN)) {
        MEDIA_LOGE("invalid captureMode:%{public}d", captureMode);
        return MSERR_INVALID_VAL;
    }
    return MSERR_OK;
}

int32_t ScreenCaptureServer::CheckDataType(DataType dataType)
{
    MEDIA_LOGD("CheckDataType start, dataType:%{public}d", dataType);
    if ((dataType > DataType::CAPTURE_FILE) || (dataType < DataType::ORIGINAL_STREAM)) {
        MEDIA_LOGE("invalid dataType:%{public}d", dataType);
        return MSERR_INVALID_VAL;
    }
    if (dataType == DataType::ENCODED_STREAM) {
        MEDIA_LOGE("not supported dataType:%{public}d", dataType);
        return MSERR_UNSUPPORT;
    }
    return MSERR_OK;
}

int32_t ScreenCaptureServer::CheckAudioCapParam(const AudioCaptureInfo &audioCapInfo)
{
    MEDIA_LOGD("CheckAudioCapParam audioSampleRate:%{public}d, audioChannels:%{public}d, audioSource:%{public}d",
        audioCapInfo.audioSampleRate, audioCapInfo.audioChannels, audioCapInfo.audioSource);
    std::vector<AudioSamplingRate> supportedSamplingRates = AudioStandard::AudioCapturer::GetSupportedSamplingRates();
    bool foundSupportSample = false;
    for (auto iter = supportedSamplingRates.begin(); iter != supportedSamplingRates.end(); ++iter) {
        if (static_cast<AudioSamplingRate>(audioCapInfo.audioSampleRate) == *iter) {
            foundSupportSample = true;
        }
    }
    if (!foundSupportSample) {
        MEDIA_LOGE("invalid audioSampleRate:%{public}d", audioCapInfo.audioSampleRate);
        return MSERR_UNSUPPORT;
    }

    std::vector<AudioChannel> supportedChannelList = AudioStandard::AudioCapturer::GetSupportedChannels();
    bool foundSupportChannel = false;
    for (auto iter = supportedChannelList.begin(); iter != supportedChannelList.end(); ++iter) {
        if (static_cast<AudioChannel>(audioCapInfo.audioChannels) == *iter) {
            foundSupportChannel = true;
        }
    }
    if (!foundSupportChannel) {
        MEDIA_LOGE("invalid audioChannels:%{public}d", audioCapInfo.audioChannels);
        return MSERR_UNSUPPORT;
    }

    if ((audioCapInfo.audioSource <= SOURCE_INVALID) || (audioCapInfo.audioSource > APP_PLAYBACK)) {
        MEDIA_LOGE("invalid audioSource:%{public}d", audioCapInfo.audioSource);
        return MSERR_INVALID_VAL;
    }

    return MSERR_OK;
}

int32_t ScreenCaptureServer::CheckVideoCapParam(const VideoCaptureInfo &videoCapInfo)
{
    MEDIA_LOGD("videoCapInfo videoFrameWidth:%{public}d, videoFrameHeight:%{public}d",
        videoCapInfo.videoFrameWidth, videoCapInfo.videoFrameHeight);
    if ((videoCapInfo.videoFrameWidth <= 0) || (videoCapInfo.videoFrameWidth > VIDEO_FRAME_WIDTH_MAX)) {
        MEDIA_LOGE("videoCapInfo size is invalid, videoFrameWidth:%{public}d, videoFrameHeight:%{public}d",
            videoCapInfo.videoFrameWidth, videoCapInfo.videoFrameHeight);
        return MSERR_INVALID_VAL;
    }
    if ((videoCapInfo.videoFrameWidth <= 0) || (videoCapInfo.videoFrameWidth > VIDEO_FRAME_HEIGHT_MAX)) {
        MEDIA_LOGE("videoCapInfo size is invalid, videoFrameWidth:%{public}d, videoFrameHeight:%{public}d",
            videoCapInfo.videoFrameWidth, videoCapInfo.videoFrameHeight);
        return MSERR_INVALID_VAL;
    }

    if (videoCapInfo.videoSource != VIDEO_SOURCE_SURFACE_RGBA) {
        MEDIA_LOGE("videoSource is invalid");
        return MSERR_INVALID_VAL;
    }
    return MSERR_OK;
}

int32_t ScreenCaptureServer::CheckAudioEncParam(const AudioEncInfo &audioEncInfo)
{
    MEDIA_LOGD("CheckAudioEncParam audioBitrate:%{public}d, audioCodecformat:%{public}d",
        audioEncInfo.audioBitrate, audioEncInfo.audioCodecformat);
    if ((audioEncInfo.audioCodecformat >= AudioCodecFormat::AUDIO_CODEC_FORMAT_BUTT) ||
        (audioEncInfo.audioCodecformat < AudioCodecFormat::AUDIO_DEFAULT)) {
        MEDIA_LOGE("invalid AudioCodecFormat:%{public}d", audioEncInfo.audioCodecformat);
        return MSERR_INVALID_VAL;
    }
    if (audioEncInfo.audioBitrate < AUDIO_BITRATE_MIN || audioEncInfo.audioBitrate > AUDIO_BITRATE_MAX) {
        MEDIA_LOGE("invalid audioBitrate:%{public}d", audioEncInfo.audioBitrate);
        return MSERR_INVALID_VAL;
    }
    return MSERR_OK;
}

int32_t ScreenCaptureServer::CheckVideoEncParam(const VideoEncInfo &videoEncInfo)
{
    MEDIA_LOGD("CheckVideoEncParam videoCodec:%{public}d, videoBitrate:%{public}d, videoFrameRate:%{public}d",
        videoEncInfo.videoCodec, videoEncInfo.videoBitrate, videoEncInfo.videoFrameRate);
    if ((videoEncInfo.videoCodec >= VideoCodecFormat::VIDEO_CODEC_FORMAT_BUTT) ||
        (videoEncInfo.videoCodec < VideoCodecFormat::VIDEO_DEFAULT)) {
        MEDIA_LOGE("invalid VideoCodecFormat:%{public}d", videoEncInfo.videoCodec);
        return MSERR_INVALID_VAL;
    }
    if (videoEncInfo.videoBitrate < VIDEO_BITRATE_MIN || videoEncInfo.videoBitrate > VIDEO_BITRATE_MAX) {
        MEDIA_LOGE("invalid videoBitrate:%{public}d", videoEncInfo.videoBitrate);
        return MSERR_INVALID_VAL;
    }
    if (videoEncInfo.videoFrameRate < VIDEO_FRAME_RATE_MIN || videoEncInfo.videoFrameRate > VIDEO_FRAME_RATE_MAX) {
        MEDIA_LOGE("invalid videoFrameRate:%{public}d", videoEncInfo.videoFrameRate);
        return MSERR_INVALID_VAL;
    }
    return MSERR_OK;
}

int32_t ScreenCaptureServer::CheckAudioCapInfo(AudioCaptureInfo &audioCapInfo)
{
    if (audioCapInfo.audioChannels == 0 && audioCapInfo.audioSampleRate == 0) {
        audioCapInfo.state = AVScreenCaptureParamValidationState::VALIDATION_IGNORE;
        return MSERR_OK;
    }
    int32_t ret = CheckAudioCapParam(audioCapInfo);
    audioCapInfo.state = ret == MSERR_OK ? AVScreenCaptureParamValidationState::VALIDATION_VALID :
        AVScreenCaptureParamValidationState::VALIDATION_INVALID;
    return ret;
}

int32_t ScreenCaptureServer::CheckVideoCapInfo(VideoCaptureInfo &videoCapInfo)
{
    if (videoCapInfo.videoFrameWidth == 0 && videoCapInfo.videoFrameHeight == 0) {
        videoCapInfo.state = AVScreenCaptureParamValidationState::VALIDATION_IGNORE;
        return MSERR_OK;
    }
    int32_t ret = CheckVideoCapParam(videoCapInfo);
    videoCapInfo.state = ret == MSERR_OK ? AVScreenCaptureParamValidationState::VALIDATION_VALID :
        AVScreenCaptureParamValidationState::VALIDATION_INVALID;
    return ret;
}

int32_t ScreenCaptureServer::CheckAudioEncInfo(AudioEncInfo &audioEncInfo)
{
    int32_t ret = CheckAudioEncParam(audioEncInfo);
    audioEncInfo.state = ret == MSERR_OK ? AVScreenCaptureParamValidationState::VALIDATION_VALID :
        AVScreenCaptureParamValidationState::VALIDATION_INVALID;
    return ret;
}

int32_t ScreenCaptureServer::CheckVideoEncInfo(VideoEncInfo &videoEncInfo)
{
    int32_t ret = CheckVideoEncParam(videoEncInfo);
    videoEncInfo.state = ret == MSERR_OK ? AVScreenCaptureParamValidationState::VALIDATION_VALID :
        AVScreenCaptureParamValidationState::VALIDATION_INVALID;
    return ret;
}

int32_t ScreenCaptureServer::CheckAllParams()
{
    int32_t ret = CheckDataType(captureConfig_.dataType);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "CheckAllParams CheckDataType failed, ret:%{public}d", ret);

    ret = CheckCaptureMode(captureConfig_.captureMode);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "CheckAllParams CheckCaptureMode failed, ret:%{public}d", ret);

    if (captureConfig_.dataType == DataType::ORIGINAL_STREAM) {
        return CheckCaptureStreamParams();
    }
    if (captureConfig_.dataType == DataType::CAPTURE_FILE) {
        return CheckCaptureFileParams();
    }
    return MSERR_INVALID_VAL;
}

int32_t ScreenCaptureServer::CheckCaptureStreamParams()
{
    CheckAudioCapInfo(captureConfig_.audioInfo.micCapInfo);
    CheckAudioCapInfo(captureConfig_.audioInfo.innerCapInfo);
    CheckVideoCapInfo(captureConfig_.videoInfo.videoCapInfo);
    if (isSurfaceMode_) {
        // surface mode, surface must not nullptr and videoCapInfo must valid.
        if (surface_ == nullptr ||
            captureConfig_.videoInfo.videoCapInfo.state != AVScreenCaptureParamValidationState::VALIDATION_VALID) {
            return MSERR_INVALID_VAL;
        }
    }
    if (captureConfig_.audioInfo.innerCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_INVALID ||
        captureConfig_.videoInfo.videoCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_INVALID) {
        return MSERR_INVALID_VAL;
    }
    if (captureConfig_.audioInfo.innerCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_IGNORE &&
        captureConfig_.videoInfo.videoCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_IGNORE) {
        return MSERR_INVALID_VAL;
    }
    if (captureConfig_.audioInfo.innerCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_INVALID) {
        return MSERR_INVALID_VAL;
    }
    return MSERR_OK;
}

int32_t ScreenCaptureServer::CheckCaptureFileParams()
{
    CheckAudioCapInfo(captureConfig_.audioInfo.micCapInfo);
    CheckAudioCapInfo(captureConfig_.audioInfo.innerCapInfo);
    CheckVideoCapInfo(captureConfig_.videoInfo.videoCapInfo);
    CheckAudioEncInfo(captureConfig_.audioInfo.audioEncInfo);
    CheckVideoEncInfo(captureConfig_.videoInfo.videoEncInfo);
    if (captureConfig_.audioInfo.innerCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_INVALID ||
        captureConfig_.videoInfo.videoCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_INVALID ||
        captureConfig_.audioInfo.audioEncInfo.state == AVScreenCaptureParamValidationState::VALIDATION_INVALID ||
        captureConfig_.videoInfo.videoEncInfo.state == AVScreenCaptureParamValidationState::VALIDATION_INVALID) {
        return MSERR_INVALID_VAL;
    }
    if (captureConfig_.audioInfo.micCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_INVALID) {
        return MSERR_INVALID_VAL;
    }
    if (captureConfig_.audioInfo.micCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_IGNORE) {
        return MSERR_OK;
    }
    AudioCaptureInfo &micCapInfo = captureConfig_.audioInfo.micCapInfo;
    AudioCaptureInfo &innerCapInfo = captureConfig_.audioInfo.innerCapInfo;
    if (micCapInfo.audioSampleRate == innerCapInfo.audioSampleRate &&
        micCapInfo.audioChannels == innerCapInfo.audioChannels) {
        return MSERR_OK;
    }
    MEDIA_LOGE("CheckCaptureFileParams failed, inner and mic param not consistentt");
    return MSERR_INVALID_VAL;
}

// Should call in ipc thread
void ScreenCaptureServer::InitAppInfo()
{
    appInfo_.appTokenId = IPCSkeleton::GetCallingTokenID();
    appInfo_.appFullTokenId = IPCSkeleton::GetCallingFullTokenID();
    appInfo_.appUid = IPCSkeleton::GetCallingUid();
    appInfo_.appPid = IPCSkeleton::GetCallingPid();
}

int32_t ScreenCaptureServer::RequestUserPrivacyAuthority()
{
    captureState_ = AVScreenCaptureState::STARTING;
    // TODO If Root is treated as whitelisted, how to guarantee RequestUserPrivacyAuthority function by TDD cases.
    // Root users should be whitelisted
    if (appInfo_.appUid == ROOT_UID) {
        MEDIA_LOGI("Root user. Permission Granted");
        return MSERR_OK;
    }
    return StartPrivacyWindow();
}

int32_t ScreenCaptureServer::OnReceiveUserPrivacyAuthority(bool isAllowed)
{
    // TODO Should callback be running in seperate thread
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGI("OnReceiveUserPrivacyAuthority start, isAllowed:%{public}d, state:%{public}d", isAllowed, captureState_);
    if (screenCaptureCb_ == nullptr) {
        MEDIA_LOGE("OnReceiveUserPrivacyAuthority failed, screenCaptureCb is nullptr, state:%{public}d", captureState_);
        captureState_ = AVScreenCaptureState::STOPPED;
        screenCaptureCb_->OnError(ScreenCaptureErrorType::SCREEN_CAPTURE_ERROR_INTERNAL,
            AVScreenCaptureErrorCode::SCREEN_CAPTURE_ERR_UNKNOWN);
        return MSERR_UNKNOWN;
    }

    if (captureState_ != AVScreenCaptureState::STARTING) {
        MEDIA_LOGE("OnReceiveUserPrivacyAuthority failed, capture is not STARTING");
        screenCaptureCb_->OnError(ScreenCaptureErrorType::SCREEN_CAPTURE_ERROR_INTERNAL,
            AVScreenCaptureErrorCode::SCREEN_CAPTURE_ERR_UNKNOWN);
        return MSERR_UNKNOWN;
    }
    if (!isAllowed) {
        captureState_ = AVScreenCaptureState::CREATED;
        screenCaptureCb_->OnStateChange(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_CANCELED);
        return MSERR_UNKNOWN;
    }
    int32_t ret = OnStartScreenCapture();
    if (ret == MSERR_OK) {
        MEDIA_LOGI("OnReceiveUserPrivacyAuthority capture start success");
        captureState_ = AVScreenCaptureState::STARTED;
        screenCaptureCb_->OnStateChange(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED);
    #ifdef SUPPORT_SCREEN_CAPTURE_WINDOW_NOTIFICATION
        ret = StartNotification();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "StartNotification failed");
    #endif
        return MSERR_OK;
    } else {
        captureState_ = AVScreenCaptureState::STOPPED;
        screenCaptureCb_->OnError(ScreenCaptureErrorType::SCREEN_CAPTURE_ERROR_INTERNAL,
            AVScreenCaptureErrorCode::SCREEN_CAPTURE_ERR_UNKNOWN);
        return MSERR_UNKNOWN;
    }
}

int32_t ScreenCaptureServer::StartAudioCapture()
{
    CHECK_AND_RETURN_RET(captureConfig_.dataType == DataType::ORIGINAL_STREAM, MSERR_INVALID_OPERATION);
    std::shared_ptr<AudioCapturerWrapper> innerCapture;
    if (captureConfig_.audioInfo.innerCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_VALID) {
        MediaTrace trace("ScreenCaptureServer::StartAudioCaptureInner");
        innerCapture = std::make_shared<AudioCapturerWrapper>(captureConfig_.audioInfo.innerCapInfo, screenCaptureCb_,
            std::string("OS_InnerAudioCapture"));
        int32_t ret = innerCapture->Start(appInfo_);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "StartAudioCapture innerCapture failed");
    }
    std::shared_ptr<AudioCapturerWrapper> micCapture;
    if (captureConfig_.audioInfo.innerCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_VALID) {
        MediaTrace trace("ScreenCaptureServer::StartAudioCaptureMic");
        micCapture = std::make_shared<AudioCapturerWrapper>(captureConfig_.audioInfo.micCapInfo, screenCaptureCb_,
            std::string("OS_MicAudioCapture"));
        int32_t ret = micCapture->Start(appInfo_);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "StartAudioCapture micCapture failed");
        micCapture->SetIsMuted(!isMicrophoneOn_);
    }
    innerAudioCapture_ = innerCapture;
    micAudioCapture_ = micCapture;
    return MSERR_OK;
}

int32_t ScreenCaptureServer::StartScreenCaptureStream()
{
    CHECK_AND_RETURN_RET(captureConfig_.dataType == DataType::ORIGINAL_STREAM, MSERR_INVALID_OPERATION);

    int32_t ret = StartAudioCapture();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "StartAudioCapture failed, ret:%{public}d, dataType:%{public}d",
        ret, captureConfig_.dataType);

    ret = StartVideoCapture();
    if (ret != MSERR_OK) {
        StopAudioCapture();
        MEDIA_LOGE("StartScreenCaptureStream failed");
        return ret;
    }
    MEDIA_LOGI("StartScreenCaptureStream success");
    return ret;
}

int32_t ScreenCaptureServer::StartScreenCaptureFile()
{
    CHECK_AND_RETURN_RET(captureConfig_.dataType == DataType::CAPTURE_FILE, MSERR_INVALID_OPERATION);

    int32_t ret = InitRecorder();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "InitRecorder failed, ret:%{public}d, dataType:%{public}d",
        ret, captureConfig_.dataType);

    ON_SCOPE_EXIT(0) {
        if (recorder_ != nullptr) {
            recorder_->Release();
            recorder_ = nullptr;
            consumer_ = nullptr;
        }
    };
    std::string virtualScreenName = "screen_capture_file";
    ret = CreateVirtualScreen(virtualScreenName, consumer_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "CreateVirtualScreen failed, ret:%{public}d, dataType:%{public}d",
        ret, captureConfig_.dataType);

    ON_SCOPE_EXIT(1) {
        if ((screenId_ >= 0) && (screenId_ != SCREEN_ID_INVALID)) {
            ScreenManager::GetInstance().DestroyVirtualScreen(screenId_);
            screenId_ = SCREEN_ID_INVALID;
            isConsumerStart_ = false;
        }
    };

    ret = recorder_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "recorder failed, ret:%{public}d, dataType:%{public}d",
        ret, captureConfig_.dataType);
    CANCEL_SCOPE_EXIT_GUARD(1);
    CANCEL_SCOPE_EXIT_GUARD(0);

    MEDIA_LOGI("StartScreenCaptureFile success");
    return ret;
}

int32_t ScreenCaptureServer::OnStartScreenCapture()
{
    MediaTrace trace("ScreenCaptureServer::OnStartScreenCapture");
    MEDIA_LOGI("OnStartScreenCapture start, dataType:%{public}d", captureConfig_.dataType);
    int32_t ret = MSERR_UNSUPPORT;
    if (captureConfig_.dataType == DataType::ORIGINAL_STREAM) {
        ret = StartScreenCaptureStream();
    }
    if (captureConfig_.dataType == DataType::CAPTURE_FILE) {
        ret = StartScreenCaptureFile();
    }
    if (ret == MSERR_OK) {
        if (!UpdatePrivacyUsingPermissionState(START_VIDEO)) {
            MEDIA_LOGE("UpdatePrivacyUsingPermissionState START failed, dataType:%{public}d", captureConfig_.dataType);
        }
        BehaviorEventWriteForScreenCapture("start", "AVScreenCapture", appInfo_.appUid, appInfo_.appPid);
    } else {
        MEDIA_LOGE("OnStartScreenCapture start failed, dataType:%{public}d", captureConfig_.dataType);
    }
    return ret;
}

int32_t ScreenCaptureServer::InitAudioCap(AudioCaptureInfo audioInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "InitAudioCap failed, capture is not CREATED, state:%{public}d", captureState_);
    MEDIA_LOGI("ScreenCaptureServer::InitAudioCap start");

    int ret = CheckAudioCapInfo(audioInfo);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "InitAudioCap CheckAudioCapInfo failed, audioSource:%{public}d",
        audioInfo.audioSource);
    if (audioInfo.audioSource == AudioCaptureSourceType::SOURCE_DEFAULT ||
        audioInfo.audioSource == AudioCaptureSourceType::MIC) {
        captureConfig_.audioInfo.micCapInfo = audioInfo;
    } else if (audioInfo.audioSource == AudioCaptureSourceType::ALL_PLAYBACK ||
        audioInfo.audioSource == AudioCaptureSourceType::APP_PLAYBACK) {
        captureConfig_.audioInfo.micCapInfo = audioInfo;
    }
    return MSERR_OK;
}

int32_t ScreenCaptureServer::InitVideoCap(VideoCaptureInfo videoInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "InitVideoCap failed, capture is not CREATED, state:%{public}d", captureState_);
    MEDIA_LOGI("ScreenCaptureServer::InitAudioCap start");

    int ret = CheckVideoCapInfo(videoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "InitVideoCap CheckVideoCapInfo failed");

    captureConfig_.videoInfo.videoCapInfo = videoInfo;
    return MSERR_OK;
}

int32_t ScreenCaptureServer::InitRecorder()
{
    CHECK_AND_RETURN_RET_LOG(outputFd_>0, MSERR_INVALID_OPERATION, "the outputFd is invalid");
    MEDIA_LOGI("InitRecorder start");
    MediaTrace trace("ScreenCaptureServer::InitRecorder");
    std::shared_ptr<IRecorderService> recorder = Media::RecorderServer::Create();
    CHECK_AND_RETURN_RET_LOG(recorder != nullptr, MSERR_UNKNOWN, "init Recoder failed");
    ON_SCOPE_EXIT(0) {
        recorder->Release();
    };
    int32_t ret = recorder->SetVideoSource(captureConfig_.videoInfo.videoCapInfo.videoSource, videoSourceId_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "SetAudioSource failed");
    AudioCaptureInfo audioInfo;
    if (captureConfig_.audioInfo.innerCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_VALID) {
        audioInfo = captureConfig_.audioInfo.innerCapInfo;
        ret = recorder->SetAudioSource(AudioSourceType::AUDIO_INNER, audioSourceId_);
    } else if (captureConfig_.audioInfo.micCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_VALID) {
        audioInfo = captureConfig_.audioInfo.micCapInfo;
        ret = recorder->SetAudioSource(AudioSourceType::AUDIO_MIC, audioSourceId_);
    }
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "SetAudioSource failed");
    ret = recorder->SetOutputFormat(fileFormat_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "SetOutputFormat failed");
    ret = recorder->SetAudioEncoder(audioSourceId_, captureConfig_.audioInfo.audioEncInfo.audioCodecformat);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "SetAudioEncoder failed");
    ret = recorder->SetAudioSampleRate(audioSourceId_, audioInfo.audioSampleRate);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "SetAudioSampleRate failed");
    ret = recorder->SetAudioChannels(audioSourceId_, audioInfo.audioChannels);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "SetAudioChannels failed");
    ret = recorder->SetAudioEncodingBitRate(audioSourceId_, captureConfig_.audioInfo.audioEncInfo.audioBitrate);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "SetAudioEncodingBitRate failed");
    ret = recorder->SetVideoEncoder(videoSourceId_, captureConfig_.videoInfo.videoEncInfo.videoCodec);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "SetVideoEncoder failed");
    ret = recorder->SetVideoSize(videoSourceId_, captureConfig_.videoInfo.videoCapInfo.videoFrameWidth,
        captureConfig_.videoInfo.videoCapInfo.videoFrameHeight);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "SetVideoSize failed");
    ret = recorder->SetVideoFrameRate(videoSourceId_, captureConfig_.videoInfo.videoEncInfo.videoFrameRate);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "SetVideoFrameRate failed");
    ret = recorder->SetVideoEncodingBitRate(videoSourceId_, captureConfig_.videoInfo.videoEncInfo.videoBitrate);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "SetVideoEncodingBitRate failed");
    ret = recorder->SetOutputFile(outputFd_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "SetOutputFile failed");
    ret = recorder->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "recorder Prepare failed");
    consumer_ = recorder->GetSurface(videoSourceId_);
    CHECK_AND_RETURN_RET_LOG(consumer_ != nullptr, MSERR_UNKNOWN, "recorder GetSurface failed");
    recorder_= recorder;
    MEDIA_LOGI("InitRecorder success");
    return MSERR_OK;
}

bool ScreenCaptureServer::UpdatePrivacyUsingPermissionState(VideoPermissionState state)
{
    // Root users should be whitelisted
    if (appInfo_.appUid == ROOT_UID) {
        MEDIA_LOGI("Root user. Privacy Granted");
        return true;
    }

    int res = 0;
    if (state == START_VIDEO) {
        res = PrivacyKit::StartUsingPermission(appInfo_.appTokenId, "ohos.permission.CAPTURE_SCREEN");
        if (res != 0) {
            MEDIA_LOGE("start using perm error for client %{public}d", appInfo_.appTokenId);
        }
    } else if (state == STOP_VIDEO) {
        res = PrivacyKit::StopUsingPermission(appInfo_.appTokenId, "ohos.permission.CAPTURE_SCREEN");
        if (res != 0) {
            MEDIA_LOGE("stop using perm error for client %{public}d", appInfo_.appTokenId);
        }
    }
    return true;
}

int32_t ScreenCaptureServer::StartScreenCaptureInner(bool isPrivacyAuthorityEnabled)
{
    MediaTrace trace("ScreenCaptureServer::StartScreenCaptureInner");
    int32_t ret = CheckAllParams();
    CHECK_AND_RETURN_RET_LOG(CheckAllParams() == MSERR_OK, ret, "StartScreenCaptureInner failed, invalid params");

    isPrivacyAuthorityEnabled_ = isPrivacyAuthorityEnabled;
    if (isPrivacyAuthorityEnabled) {
    #ifdef SUPPORT_SCREEN_CAPTURE_WINDOW_NOTIFICATION
        return RequestUserPrivacyAuthority();
    #else
        MEDIA_LOGE("StartScreenCaptureInner privacy notification window not support, check CAPTURE_SCREEN permission");
        isPrivacyAuthorityEnabled_ = false;
    #endif
    }
    if (!CheckScreenCapturePermission()) {
        captureState_ = AVScreenCaptureState::STOPPED;
        MEDIA_LOGE("StartScreenCaptureInner CheckScreenCapturePermission failed");
        return MSERR_INVALID_OPERATION;
    }

    ret = OnStartScreenCapture();
    if (ret == MSERR_OK) {
        MEDIA_LOGI("StartScreenCaptureInner OnStartScreenCapture success");
        captureState_ = AVScreenCaptureState::STARTED;
    } else {
        isPrivacyAuthorityEnabled_ = false;
        isSurfaceMode_ = false;
        captureState_ = AVScreenCaptureState::STOPPED;
        MEDIA_LOGE("StartScreenCaptureInner OnStartScreenCapture failed");
    }
    return ret;
}

int32_t ScreenCaptureServer::StartPrivacyWindow()
{
    std::string comStr = "{\"ability.want.params.uiExtensionType\":\"sys/commonUI\",\"sessionId\":\"";
    comStr += std::to_string(sessionId_).c_str();
    comStr += "\",\"callerUid\":\"";
    comStr += std::to_string(appInfo_.appUid).c_str();
    comStr += "\"}";
    
    AAFwk::Want want;
    want.SetElementName(bundleName_, abilityName_);
    auto connection_ = sptr<UIExtensionAbilityConnection>(new (std::nothrow) UIExtensionAbilityConnection(comStr));
    auto ret = OHOS::AAFwk::ExtensionManagerClient::GetInstance().ConnectServiceExtensionAbility(want, connection_,\
        nullptr, -1);
    MEDIA_LOGI("ConnectServiceExtensionAbility end %{public}d", ret);
    return MSERR_OK;
}

int32_t ScreenCaptureServer::StartNotification()
{
    int32_t result = NotificationHelper::SubscribeLocalLiveViewNotification(NOTIFICATION_SUBSCRIBER);

    NotificationRequest request;
    std::shared_ptr<NotificationLocalLiveViewContent> localLiveViewContent = GetLocalLiveViewContent();

    std::shared_ptr<NotificationContent> content =
        std::make_shared<NotificationContent>(localLiveViewContent);

    auto uid = getuid();
    request.SetSlotType(NotificationConstant::SlotType::LIVE_VIEW);
    notificationId_ = sessionId_;
    request.SetNotificationId(notificationId_);
    request.SetContent(content);
    request.SetCreatorUid(uid);
    request.SetUnremovable(true);
    request.SetInProgress(true);

    std::shared_ptr<PixelMap> pixelMapTotalSpr = GetPixelMap(ICON_PATH_CAPSULE);
    request.SetLittleIcon(pixelMapTotalSpr);
    request.SetBadgeIconStyle(NotificationRequest::BadgeStyle::LITTLE);

    result = NotificationHelper::PublishNotification(request);
    MEDIA_LOGI("Screencapture service PublishNotification uid %{public}d, result %{public}d", uid, result);
    return MSERR_OK;
}

std::shared_ptr<NotificationLocalLiveViewContent> ScreenCaptureServer::GetLocalLiveViewContent()
{
    std::shared_ptr<NotificationLocalLiveViewContent> localLiveViewContent =
        std::make_shared<NotificationLocalLiveViewContent>();
    localLiveViewContent->SetType(1);
    localLiveViewContent->SetTitle("系统录屏");
    localLiveViewContent->SetText("录屏中...");

    auto capsule = NotificationCapsule();
    std::string backgroundColor = "#E84026";
    capsule.SetBackgroundColor(backgroundColor);
    std::shared_ptr<PixelMap> pixelMapCapSpr = GetPixelMap(ICON_PATH_CAPSULE);
    capsule.SetIcon(pixelMapCapSpr);

    localLiveViewContent->SetCapsule(capsule);
    localLiveViewContent->addFlag(NotificationLocalLiveViewContent::LiveViewContentInner::CAPSULE);

    auto testTime = NotificationTime();
    testTime.SetInitialTime(1);
    testTime.SetIsCountDown(false);
    testTime.SetIsPaused(false);
    testTime.SetIsInTitle(true);

    localLiveViewContent->SetTime(testTime);
    localLiveViewContent->addFlag(NotificationLocalLiveViewContent::LiveViewContentInner::TIME);

    auto testButton = NotificationLocalLiveViewButton();
    testButton.addSingleButtonName(buttonNameMic_);
    std::shared_ptr<PixelMap> pixelMapSpr = GetPixelMap(ICON_PATH_MIC);
    testButton.addSingleButtonIcon(pixelMapSpr);

    testButton.addSingleButtonName(buttonNameStop_);
    std::shared_ptr<PixelMap> pixelMapStopSpr = GetPixelMap(ICON_PATH_STOP);
    testButton.addSingleButtonIcon(pixelMapStopSpr);

    localLiveViewContent->SetButton(testButton);
    localLiveViewContent->addFlag(NotificationLocalLiveViewContent::LiveViewContentInner::BUTTON);

    return localLiveViewContent;
}

std::shared_ptr<PixelMap> ScreenCaptureServer::GetPixelMap(std::string path)
{
    uint32_t errorCode = 0;
    SourceOptions opts;
    opts.formatHint = "image/png";
    std::unique_ptr<ImageSource> imageSource =
        ImageSource::CreateImageSource(path, opts, errorCode);
    DecodeOptions decodeOpts;
    std::unique_ptr<PixelMap> pixelMap = imageSource->CreatePixelMap(decodeOpts, errorCode);
    std::shared_ptr<PixelMap> pixelMapSpr = std::move(pixelMap);
    return pixelMapSpr;
}

int32_t ScreenCaptureServer::StartScreenCapture(bool isPrivacyAuthorityEnabled)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "StartScreenCapture failed, capture is not CREATED, state:%{public}d", captureState_);
    MEDIA_LOGI("StartScreenCapture isPrivacyAuthorityEnabled:%{public}d", isPrivacyAuthorityEnabled);
    isSurfaceMode_ = false;
    return StartScreenCaptureInner(isPrivacyAuthorityEnabled);
}

int32_t ScreenCaptureServer::StartScreenCaptureWithSurface(sptr<Surface> surface, bool isPrivacyAuthorityEnabled)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "StartScreenCapture failed, capture is not CREATED, state:%{public}d", captureState_);
    MEDIA_LOGI("StartScreenCapture isPrivacyAuthorityEnabled:%{public}d", isPrivacyAuthorityEnabled);
    if (surface == nullptr) {
        MEDIA_LOGE("surface is nullptr");
        return MSERR_INVALID_OPERATION;
    }
    surface_ = surface;
    isSurfaceMode_ = true;
    return StartScreenCaptureInner(isPrivacyAuthorityEnabled);
}

int32_t ScreenCaptureServer::StartVideoCapture()
{
    if (captureConfig_.audioInfo.innerCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_IGNORE) {
        MEDIA_LOGI("StartVideoCapture is ignored");
        return MSERR_OK;
    }
    CHECK_AND_RETURN_RET_LOG(
        captureConfig_.audioInfo.innerCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_VALID,
        MSERR_INVALID_VAL, "StartScreenCapture failed, invalid param, dataType:%{public}d", captureConfig_.dataType);

    int32_t ret = StartHomeVideoCapture();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret,
        "StartHomeVideoCapture failed, invalid param, dataType:%{public}d", captureConfig_.dataType);
    return MSERR_OK;
}

int32_t ScreenCaptureServer::StartHomeVideoCapture()
{
    std::string virtualScreenName = "screen_capture";
    if (isSurfaceMode_) {
        int32_t ret = CreateVirtualScreen(virtualScreenName, surface_);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "create virtual screen with surface failed");
        return MSERR_OK;
    }

    consumer_ = OHOS::Surface::CreateSurfaceAsConsumer();
    CHECK_AND_RETURN_RET_LOG(consumer_ != nullptr, MSERR_UNKNOWN, "CreateSurfaceAsConsumer failed");
    auto producer = consumer_->GetProducer();
    CHECK_AND_RETURN_RET_LOG(producer != nullptr, MSERR_UNKNOWN, "GetProducer failed");
    auto producerSurface = OHOS::Surface::CreateSurfaceAsProducer(producer);
    CHECK_AND_RETURN_RET_LOG(producerSurface != nullptr, MSERR_UNKNOWN, "CreateSurfaceAsProducer failed");

    surfaceCb_ = OHOS::sptr<ScreenCapBufferConsumerListener>::MakeSptr(consumer_, screenCaptureCb_);
    CHECK_AND_RETURN_RET_LOG(surfaceCb_ != nullptr, MSERR_UNKNOWN, "MakeSptr surfaceCb_ failed");

    consumer_->RegisterConsumerListener(surfaceCb_);
    return MSERR_OK;
}

int32_t ScreenCaptureServer::CreateVirtualScreen(const std::string &name, sptr<OHOS::Surface> consumer)
{
    isConsumerStart_ = false;
    VirtualScreenOption virScrOption = InitVirtualScreenOption(name, consumer);
    sptr<Rosen::Display> display = Rosen::DisplayManager::GetInstance().GetDefaultDisplaySync();
    if (display != nullptr) {
        MEDIA_LOGI("get displayinfo width:%{public}d,height:%{public}d,density:%{public}d", display->GetWidth(),
                   display->GetHeight(), display->GetDpi());
        virScrOption.density_ = display->GetDpi();
    }
    VideoCaptureInfo &videoCapInfo = captureConfig_.videoInfo.videoCapInfo;
    if (missionIds_.size() > 0 && captureConfig_.captureMode == CAPTURE_SPECIFIED_WINDOW) {
        virScrOption.missionIds_ = missionIds_;
    } else {
        if (videoCapInfo.taskIDs.size() > 0 && captureConfig_.captureMode == CAPTURE_SPECIFIED_WINDOW) {
            GetMissionIds(missionIds_);
            virScrOption.missionIds_ = missionIds_;
        }
    }
    screenId_ = ScreenManager::GetInstance().CreateVirtualScreen(virScrOption);
    CHECK_AND_RETURN_RET_LOG(screenId_ > 0, MSERR_UNKNOWN, "CreateVirtualScreen failed");

    auto screen = ScreenManager::GetInstance().GetScreenById(screenId_);
    if (screen == nullptr) {
        MEDIA_LOGE("GetScreenById failed");
        if (screenId_ > 0 && screenId_ != SCREEN_ID_INVALID) {
            ScreenManager::GetInstance().DestroyVirtualScreen(screenId_);
        }
        screenId_ = SCREEN_ID_INVALID;
        return MSERR_UNKNOWN;
    }
    MEDIA_LOGI("CreateVirtualScreen success");
    std::vector<sptr<Screen>> screens;
    ScreenManager::GetInstance().GetAllScreens(screens);
    std::vector<ScreenId> mirrorIds;
    mirrorIds.push_back(screenId_);
    ScreenId mirrorGroup = static_cast<ScreenId>(1);

    if (captureConfig_.captureMode == CAPTURE_SPECIFIED_SCREEN) {
        for (uint32_t i = 0; i < screens.size() ; i++) {
            if (screens[i]->GetId() == videoCapInfo.displayId) {
                ScreenManager::GetInstance().MakeMirror(screens[i]->GetId(), mirrorIds, mirrorGroup);
                MEDIA_LOGI("MakeMirror extand screen success");
                return MSERR_OK;
            }
        }
    }
    ScreenManager::GetInstance().MakeMirror(screens[0]->GetId(), mirrorIds, mirrorGroup);
    MEDIA_LOGI("MakeMirror main screen success");
    return MSERR_OK;
}

VirtualScreenOption ScreenCaptureServer::InitVirtualScreenOption(const std::string &name, sptr<OHOS::Surface> consumer)
{
    VirtualScreenOption virScrOption = {
        .name_ = name,
        .width_ = captureConfig_.videoInfo.videoCapInfo.videoFrameWidth,
        .height_ = captureConfig_.videoInfo.videoCapInfo.videoFrameHeight,
        .density_ = 0,
        .surface_ = consumer,
        .flags_ = 0,
        .isForShot_ = true,
        .missionIds_ = {},
    };
    return virScrOption;
}

int32_t ScreenCaptureServer::GetMissionIds(std::vector<uint64_t> &missionIds)
{
    int32_t size = captureConfig_.videoInfo.videoCapInfo.taskIDs.size();
    std::list<int32_t> taskIDListTemp = captureConfig_.videoInfo.videoCapInfo.taskIDs;
    for (int32_t i = 0; i < size; i++) {
        int32_t taskId = taskIDListTemp.front();
        taskIDListTemp.pop_front();
        MEDIA_LOGI("ScreenCaptureServer::GetMissionIds taskId : %{public}s", std::to_string(taskId).c_str());
        uint64_t uintNum = static_cast<uint64_t>(taskId);
        missionIds.push_back(uintNum);
    }
    return MSERR_OK;
}

int32_t ScreenCaptureServer::AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audioBuffer, AudioCaptureSourceType type)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "AcquireAudioBuffer failed, capture is not STARTED, state:%{public}d, type:%{public}d", captureState_, type);

    if (((type == AudioCaptureSourceType::MIC) || (type == AudioCaptureSourceType::SOURCE_DEFAULT)) &&
        micAudioCapture_ != nullptr) {
        return micAudioCapture_->AcquireAudioBuffer(audioBuffer);
    }
    if (((type == AudioCaptureSourceType::ALL_PLAYBACK) || (type == AudioCaptureSourceType::APP_PLAYBACK)) &&
        innerAudioCapture_ != nullptr) {
        return innerAudioCapture_->AcquireAudioBuffer(audioBuffer);
    }
    MEDIA_LOGE("AcquireAudioBuffer failed, source type not support, type:%{public}d", type);
    return MSERR_UNKNOWN;
}

int32_t ScreenCaptureServer::ReleaseAudioBuffer(AudioCaptureSourceType type)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "ReleaseAudioBuffer failed, capture is not STARTED, state:%{public}d, type:%{public}d", captureState_, type);

    if (((type == AudioCaptureSourceType::MIC) || (type == AudioCaptureSourceType::SOURCE_DEFAULT)) &&
        micAudioCapture_ != nullptr) {
        return micAudioCapture_->ReleaseAudioBuffer();
    }
    if (((type == AudioCaptureSourceType::ALL_PLAYBACK) || (type == AudioCaptureSourceType::APP_PLAYBACK)) &&
        innerAudioCapture_ != nullptr) {
        return innerAudioCapture_->ReleaseAudioBuffer();
    }
    MEDIA_LOGE("ReleaseAudioBuffer failed, source type not support, type:%{public}d", type);
    return MSERR_UNKNOWN;
}

int32_t ScreenCaptureServer::AcquireVideoBuffer(sptr<OHOS::SurfaceBuffer> &surfaceBuffer, int32_t &fence,
                                                int64_t &timestamp, OHOS::Rect &damage)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "AcquireVideoBuffer failed, capture is not STARTED, state:%{public}d", captureState_);

    CHECK_AND_RETURN_RET_LOG(surfaceCb_ != nullptr, MSERR_NO_MEMORY, "AcquireVideoBuffer failed, callback is nullptr");
    (static_cast<ScreenCapBufferConsumerListener *>(surfaceCb_.GetRefPtr()))->
        AcquireVideoBuffer(surfaceBuffer, fence, timestamp, damage);
    if (surfaceBuffer != nullptr) {
        MEDIA_LOGD("getcurrent surfaceBuffer info, size:%{public}u", surfaceBuffer->GetSize());
        return MSERR_OK;
    }
    return MSERR_UNKNOWN;
}

int32_t ScreenCaptureServer::ReleaseVideoBuffer()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "AcquireVideoBuffer failed, capture is not STARTED, state:%{public}d", captureState_);

    CHECK_AND_RETURN_RET_LOG(surfaceCb_ != nullptr, MSERR_NO_MEMORY, "AcquireVideoBuffer failed, callback is nullptr");
    return (static_cast<ScreenCapBufferConsumerListener *>(surfaceCb_.GetRefPtr()))->ReleaseVideoBuffer();
}

int32_t ScreenCaptureServer::ExcludeContent(ScreenCaptureContentFilter &contentFilter)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "AcquireVideoBuffer failed, capture is not STARTED, state:%{public}d", captureState_);

    MEDIA_LOGI("ScreenCaptureServer::SetMicrophoneEnabled start");
    contentFilter_ = contentFilter;

    // For the moment, not support:
    // TODO For STREAM, should call AudioCapturer interface to make effect when start
    // TODO For CAPTURE FILE, should call Recorder interface to make effect when start
    return MSERR_UNSUPPORT;
}

int32_t ScreenCaptureServer::SetMicrophoneEnabled(bool isMicrophone)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGI("SetMicrophoneEnabled isMicrophoneOn_:%{public}d, new isMicrophone:%{public}d",
        isMicrophoneOn_, isMicrophone);
    isMicrophoneOn_ = isMicrophone;
    if (captureState_ == AVScreenCaptureState::STARTED) {
        if (micAudioCapture_ != nullptr) {
            micAudioCapture_->SetIsMuted(!isMicrophone);
        }
        // TODO For CAPTURE FILE, should call Recorder interface to make effect
    }
    return MSERR_OK;
}

int32_t ScreenCaptureServer::SetScreenCanvasRotation(bool canvasRotation)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(screenId_ != SCREEN_ID_INVALID, MSERR_INVALID_VAL,
                             "SetScreenCanvasRotation failed virtual screen not init");
    MEDIA_LOGI("ScreenCaptureServer::SetScreenCanvasRotation, canvasRotation:%{public}d", canvasRotation);
    auto ret = ScreenManager::GetInstance().SetVirtualMirrorScreenCanvasRotation(screenId_, canvasRotation);
    CHECK_AND_RETURN_RET_LOG(ret == DMError::DM_OK, MSERR_UNSUPPORT,
                             "SetVirtualMirrorScreenCanvasRotation failed, ret: %{public}d", ret);
    return MSERR_OK;
}

int32_t ScreenCaptureServer::StopAudioCapture()
{
    if (micAudioCapture_ != nullptr) {
        MediaTrace trace("ScreenCaptureServer::StopAudioCaptureMic");
        micAudioCapture_->Stop();
        micAudioCapture_ = nullptr;
    }

    if (innerAudioCapture_ != nullptr) {
        MediaTrace trace("ScreenCaptureServer::StopAudioCaptureInner");
        innerAudioCapture_->Stop();
        innerAudioCapture_ = nullptr;
    }
    return MSERR_OK;
}

int32_t ScreenCaptureServer::StopVideoCapture()
{
    MediaTrace trace("ScreenCaptureServer::StopVideoCapture");
    MEDIA_LOGI("StopVideoCapture");
    if ((screenId_ < 0) || (consumer_ == nullptr) || !isConsumerStart_) {
        MEDIA_LOGI("StopVideoCapture failed, stop");
        surfaceCb_ = nullptr;
        return MSERR_INVALID_OPERATION;
    }

    if (screenId_ >=0 && screenId_ != SCREEN_ID_INVALID) {
        ScreenManager::GetInstance().DestroyVirtualScreen(screenId_);
        screenId_ = SCREEN_ID_INVALID;
    }

    if ((consumer_ != nullptr) && isConsumerStart_) {
        isConsumerStart_ = false;
        consumer_->UnregisterConsumerListener();
    }

    if (surfaceCb_ != nullptr) {
        (static_cast<ScreenCapBufferConsumerListener *>(surfaceCb_.GetRefPtr()))->Release();
        surfaceCb_ = nullptr;
    }

    return MSERR_OK;
}

int32_t ScreenCaptureServer::StopScreenCaptureRecorder()
{
    MediaTrace trace("ScreenCaptureServer::StopScreenCaptureRecorder");
    int32_t ret = MSERR_OK;
    if (recorder_ != nullptr) {
        ret = recorder_->Stop(true);
        if (ret != MSERR_OK) {
            MEDIA_LOGE("StopScreenCaptureRecorder recorder stop failed, ret:%{public}d", ret);
        }
        if (screenId_ >=0 && screenId_ != SCREEN_ID_INVALID) {
            ScreenManager::GetInstance().DestroyVirtualScreen(screenId_);
            screenId_ = SCREEN_ID_INVALID;
        }
        recorder_->Release();
        recorder_ = nullptr;
    }

    isConsumerStart_ = false;
    if (outputFd_ > 0) {
        (void)::close(outputFd_);
        outputFd_ = -1;
    }
    return ret;
}

int32_t ScreenCaptureServer::OnStopScreenCapture()
{
    if (screenCaptureCb_ != nullptr) {
        (static_cast<ScreenCaptureListenerCallback *>(screenCaptureCb_.get()))->Stop();
    }
    if (captureState_ == AVScreenCaptureState::CREATED || captureState_ == AVScreenCaptureState::STARTING) {
        if (outputFd_ > 0) {
            (void)::close(outputFd_);
            outputFd_ = -1;
        }
        captureState_ = AVScreenCaptureState::STOPPED;
        isSurfaceMode_ = false;
        surface_ = nullptr;
        return MSERR_OK;
    }
    CHECK_AND_RETURN_RET(captureState_ != AVScreenCaptureState::STOPPED, MSERR_OK);

    int32_t ret = MSERR_OK;
    if (captureConfig_.dataType == DataType::CAPTURE_FILE) {
        ret = StopScreenCaptureRecorder();
    } else if (captureConfig_.dataType == DataType::ORIGINAL_STREAM) {
        int32_t retAudio = StopAudioCapture();
        int32_t retVideo = StopVideoCapture();
        ret = (retAudio == MSERR_OK && retVideo == MSERR_OK) ? MSERR_OK : MSERR_STOP_FAILED;
    } else {
        MEDIA_LOGW("OnStopScreenCapture unsupport and ignore");
        return MSERR_OK;
    }

    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::STARTED, ret, "state:%{public}d", captureState_);
    if (isPrivacyAuthorityEnabled_) {
        // Remove real time notification
#ifdef SUPPORT_SCREEN_CAPTURE_WINDOW_NOTIFICATION
        int32_t ret = NotificationHelper::CancelNotification(notificationId_);
        MEDIA_LOGI("OnStopScreenCapture CancelNotification ret:%{public}d ", ret);
#endif
        isPrivacyAuthorityEnabled_ = false;
    }

    if (!UpdatePrivacyUsingPermissionState(STOP_VIDEO)) {
        MEDIA_LOGE("UpdatePrivacyUsingPermissionState STOP failed, dataType:%{public}d", captureConfig_.dataType);
    }
    BehaviorEventWriteForScreenCapture("stop", "AVScreenCapture", appInfo_.appUid, appInfo_.appPid);

    MEDIA_LOGI("OnStopScreenCapture sessionId:%{public}d, activeSessionId:%{public}d", sessionId_, activeSessionId);
    if (sessionId_ == activeSessionId.load()) {
        activeSessionId.store(SCREEN_ID_INVALID);
    }
    return ret;
}

int32_t ScreenCaptureServer::StopScreenCapture()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("ScreenCaptureServer::StopScreenCapture");
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances StopScreenCapture", FAKE_POINTER(this));
    return OnStopScreenCapture();
}

void ScreenCaptureServer::Release()
{
    MediaTrace trace("ScreenCaptureServer::Release");
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances Release", FAKE_POINTER(this));
    int32_t sessionId;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sessionId = sessionId_;
        sessionId_ = SESSION_ID_INVALID;
    }
    {
        std::lock_guard<std::mutex> lock(mutexGlobal);
        serverMap.erase(sessionId);
    }
    std::lock_guard<std::mutex> lock(mutex_);
    OnStopScreenCapture();
}

void ScreenCapBufferConsumerListener::OnBufferAvailable()
{
    CHECK_AND_RETURN(consumer_ != nullptr);
    int32_t flushFence = 0;
    int64_t timestamp = 0;
    OHOS::Rect damage;
    OHOS::sptr<OHOS::SurfaceBuffer> buffer = nullptr;
    consumer_->AcquireBuffer(buffer, flushFence, timestamp, damage);
    CHECK_AND_RETURN_LOG(buffer != nullptr, "Acquire SurfaceBuffer failed");

    void *addr = buffer->GetVirAddr();
    if (addr == nullptr) {
        MEDIA_LOGE("Acquire SurfaceBuffer addr invalid");
        consumer_->ReleaseBuffer(buffer, flushFence);
        return;
    }
    MEDIA_LOGD("SurfaceBuffer size:%{public}u", buffer->GetSize());

    {
        std::unique_lock<std::mutex> lock(bufferMutex_);
        if (availBuffers_.size() > MAX_BUFFER_SIZE) {
            MEDIA_LOGE("consume slow, drop video frame");
            consumer_->ReleaseBuffer(buffer, flushFence);
            return;
        }
        availBuffers_.push(std::make_unique<SurfaceBufferEntry>(buffer, flushFence, timestamp, damage));

    }
    bufferCond_.notify_all();

    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(screenCaptureCb_ != nullptr, "no consumer, will drop video frame");
    screenCaptureCb_->OnVideoBufferAvailable(true);
}

int32_t ScreenCapBufferConsumerListener::AcquireVideoBuffer(sptr<OHOS::SurfaceBuffer> &surfaceBuffer, int32_t &fence,
    int64_t &timestamp, OHOS::Rect &damage)
{
    using namespace std::chrono_literals;
    std::unique_lock<std::mutex> lock(bufferMutex_);
    if (!bufferCond_.wait_for(
        lock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this]{ return !availBuffers_.empty(); })) {
        return MSERR_UNKNOWN;
    }
    surfaceBuffer = availBuffers_.front()->buffer;
    fence = availBuffers_.front()->flushFence;
    timestamp = availBuffers_.front()->timeStamp;
    damage = availBuffers_.front()->damageRect;
    return MSERR_OK;
}

int32_t  ScreenCapBufferConsumerListener::ReleaseVideoBuffer()
{
    std::unique_lock<std::mutex> lock(bufferMutex_);
    CHECK_AND_RETURN_RET_LOG(!availBuffers_.empty(), MSERR_OK, "buffer queue is empty, no video frame to release");

    if (consumer_ != nullptr) {
        consumer_->ReleaseBuffer(availBuffers_.front()->buffer, availBuffers_.front()->flushFence);
    }
    availBuffers_.pop();
    return MSERR_OK;
}

int32_t ScreenCapBufferConsumerListener::Release()
{
    std::unique_lock<std::mutex> lock(bufferMutex_);
    MEDIA_LOGI("Release");
    return ReleaseBuffer();
}
} // namespace Media
} // namespace OHOS
