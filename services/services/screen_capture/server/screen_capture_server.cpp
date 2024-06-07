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
#include "media_utils.h"
#include "uri_helper.h"
#include "media_dfx.h"
#include "scope_guard.h"
#include "screen_capture_listener_proxy.h"
#include "res_type.h"
#include "res_sched_client.h"
#include "param_wrapper.h"
#include <unistd.h>
#include <sys/stat.h>
#include "hitrace/tracechain.h"

using OHOS::Rosen::DMError;

namespace OHOS {
namespace Media {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ScreenCaptureServer"};
static std::map<int32_t, std::weak_ptr<OHOS::Media::ScreenCaptureServer>> serverMap;
std::atomic<int32_t> activeSessionId_(-1);

// As in the ScreenCaptureServer destructor function mutexGlobal_ is required to update serverMap
// MAKE SURE THAT when mutexGlobal_ is been holding MUST NOT trigger ScreenCaptureServer destructor to be called
std::mutex mutexGlobal_;

std::shared_ptr<OHOS::Media::ScreenCaptureServer> GetScreenCaptureServerByIdWithLock(int32_t id)
{
    auto iter = serverMap.find(id);
    if (iter == serverMap.end()) {
        return nullptr;
    }
    return (iter->second).lock();
}
}

static const std::string MP4 = "mp4";
static const std::string M4A = "m4a";

static const std::string USER_CHOICE_ALLOW = "true";
static const std::string USER_CHOICE_DENY = "false";
static const std::string BUTTON_NAME_MIC = "mic";
static const std::string BUTTON_NAME_STOP = "stop";
static const std::string ICON_PATH_CAPSULE = "/etc/screencapture/capsule.svg";
static const std::string ICON_PATH_NOTIFICATION = "/etc/screencapture/notification.png";
static const std::string ICON_PATH_MIC = "/etc/screencapture/mic.svg";
static const std::string ICON_PATH_MIC_OFF = "/etc/screencapture/mic_off.svg";
static const std::string ICON_PATH_STOP = "/etc/screencapture/stop.png";
static const std::string BUNDLE_NAME = "com.ohos.systemui";
static const std::string ABILITY_NAME = "com.ohos.systemui.dialog";
static const std::string BACK_GROUND_COLOR = "#E84026";
static const std::string SCREEN_RECORDER_BUNDLE_NAME = "com.huawei.ohos.screenrecorder";
static const int32_t SVG_HEIGHT = 80;
static const int32_t SVG_WIDTH = 80;
static const int32_t MDPI = 160;
static const int32_t MICROPHONE_OFF = 0;
static const int32_t MICROPHONE_STATE_COUNT = 2;
#ifdef SUPPORT_SCREEN_CAPTURE_WINDOW_NOTIFICATION
    static const int32_t NOTIFICATION_MAX_TRY_NUM = 3;
#endif

static const int32_t MAX_SESSION_ID = 256;
static const int32_t MAX_SESSION_PER_UID = 8;
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
    MEDIA_LOGI("NotificationSubscriber OnResponse notificationId : %{public}d, ButtonName : %{public}s ",
        notificationId, (buttonOption->GetButtonName()).c_str());

    // To avoid deadlock: first release mutexGlobal_, then be destructed
    std::shared_ptr<OHOS::Media::ScreenCaptureServer> server;
    {
        std::unique_lock<std::mutex> lock(mutexGlobal_);
        server = GetScreenCaptureServerByIdWithLock(notificationId);
    }
    if (server == nullptr) {
        MEDIA_LOGW("OnResponse ScreenCaptureServer not exist, notificationId : %{public}d, ButtonName : %{public}s ",
            notificationId, (buttonOption->GetButtonName()).c_str());
        return;
    }
    if (BUTTON_NAME_STOP.compare(buttonOption->GetButtonName()) == 0) {
        server->StopScreenCaptureByEvent(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STOPPED_BY_USER);
        return;
    }
    if (BUTTON_NAME_MIC.compare(buttonOption->GetButtonName()) == 0) {
        server->UpdateMicrophoneEnabled();
        return;
    }
}

void NotificationSubscriber::OnDied()
{
    MEDIA_LOGI("NotificationSubscriber OnDied");
}

std::shared_ptr<IScreenCaptureService> ScreenCaptureServer::Create()
{
    MEDIA_LOGI("ScreenCaptureServer Create start.");
    std::shared_ptr<ScreenCaptureServer> serverTemp = std::make_shared<ScreenCaptureServer>();
    CHECK_AND_RETURN_RET_LOG(serverTemp != nullptr, nullptr, "Failed to new ScreenCaptureServer");
    int32_t countForUid = 0;
    int32_t newSessionId = 0;

    // To avoid deadlock: first release mutexGlobal_, then be destructed
    std::shared_ptr<ScreenCaptureServer> server;
    for (int32_t i = 0; i < MAX_SESSION_ID; i++) {
        // To avoid deadlock: first release mutexGlobal_, then be destructed
        {
            std::unique_lock<std::mutex> lock(mutexGlobal_);
            server = GetScreenCaptureServerByIdWithLock(newSessionId);
        }
        if (server != nullptr) {
            newSessionId++;
            countForUid += (server->appInfo_.appUid == serverTemp->appInfo_.appUid) ? 1 : 0;
            // To avoid deadlock: first release mutexGlobal_, then be destructed
            // Do this without holding mutexGlobal_ to avoid dead lock when set nullptr trigger server destruct
            server = nullptr;
            continue;
        }
        if (countForUid >= MAX_SESSION_PER_UID) {
            MEDIA_LOGE("Create failed, uid(%{public}d) has created too many ScreenCaptureServer instances",
                serverTemp->appInfo_.appUid);
            return nullptr;
        }
        {
            std::unique_lock<std::mutex> lock(mutexGlobal_);
            serverMap.insert(std::make_pair(newSessionId, serverTemp));
            MEDIA_LOGI("ScreenCaptureServer::Create newSessionId: %{public}d, serverMap size:%{public}zu",
                newSessionId, serverMap.size());
        }
        break;
    }

    if (newSessionId == MAX_SESSION_ID) {
        MEDIA_LOGE("Create failed for uid(%{public}d), there are too many ScreenCaptureServer instances",
            serverTemp->appInfo_.appUid);
        return nullptr;
    }
    serverTemp->SetSessionId(newSessionId);
    MEDIA_LOGI("ScreenCaptureServer Create end.");
    return std::static_pointer_cast<OHOS::Media::IScreenCaptureService>(serverTemp);
}

int32_t ScreenCaptureServer::ReportAVScreenCaptureUserChoice(int32_t sessionId, const std::string &choice)
{
    MEDIA_LOGI("ReportAVScreenCaptureUserChoice sessionId: %{public}d, choice: %{public}s", sessionId, choice.c_str());

    // To avoid deadlock: first release mutexGlobal_, then be destructed
    std::shared_ptr<ScreenCaptureServer> server;
    {
        std::lock_guard<std::mutex> lock(mutexGlobal_);
        server = GetScreenCaptureServerByIdWithLock(sessionId);
    }
    if (server == nullptr) {
        MEDIA_LOGE("ReportAVScreenCaptureUserChoice failed to get instance, sessionId: %{public}d", sessionId);
        return MSERR_UNKNOWN;
    }

    // To avoid deadlock: first release mutexGlobal_, then be destructed
    std::shared_ptr<ScreenCaptureServer> currentServer;
    if (USER_CHOICE_ALLOW.compare(choice) == 0) {
        std::lock_guard<std::mutex> lock(mutexGlobal_);
        if (activeSessionId_.load() >= 0) {
            currentServer = GetScreenCaptureServerByIdWithLock(activeSessionId_.load());
            if (currentServer != nullptr) {
                MEDIA_LOGW("ReportAVScreenCaptureUserChoice uid(%{public}d) is interrupted by uid(%{public}d)",
                    currentServer->appInfo_.appUid, server->appInfo_.appUid);
                currentServer->StopScreenCaptureByEvent(
                    AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_INTERRUPTED_BY_OTHER);
            }
        }
        activeSessionId_.store(SESSION_ID_INVALID);
        int32_t ret = server->OnReceiveUserPrivacyAuthority(true);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret,
            "ReportAVScreenCaptureUserChoice user choice is true but start failed");
        activeSessionId_.store(sessionId);
        MEDIA_LOGI("ReportAVScreenCaptureUserChoice user choice is true and start success");
        return MSERR_OK;
    } else if (USER_CHOICE_DENY.compare(choice) == 0) {
        return server->OnReceiveUserPrivacyAuthority(false);
    }
    MEDIA_LOGW("ReportAVScreenCaptureUserChoice user choice is not support");
    return MSERR_UNKNOWN;
}

void ScreenCaptureServer::SetMetaDataReport()
{
    std::shared_ptr<Media::Meta> meta = std::make_shared<Media::Meta>();
    meta->SetData(Tag::SCREEN_CAPTURE_ERR_CODE, statisticalEventInfo_.errCode);
    meta->SetData(Tag::SCREEN_CAPTURE_ERR_MSG, statisticalEventInfo_.errMsg);
    meta->SetData(Tag::SCREEN_CAPTURE_DURATION, statisticalEventInfo_.captureDuration);
    meta->SetData(Tag::SCREEN_CAPTURE_AV_TYPE, avType_);
    meta->SetData(Tag::SCREEN_CAPTURE_DATA_TYPE, dataMode_);
    meta->SetData(Tag::SCREEN_CAPTURE_USER_AGREE, statisticalEventInfo_.userAgree);
    meta->SetData(Tag::SCREEN_CAPTURE_REQURE_MIC, statisticalEventInfo_.requireMic);
    meta->SetData(Tag::SCREEN_CAPTURE_ENABLE_MIC, statisticalEventInfo_.enableMic);
    meta->SetData(Tag::SCREEN_CAPTURE_VIDEO_RESOLUTION, statisticalEventInfo_.videoResolution);
    meta->SetData(Tag::SCREEN_CAPTURE_STOP_REASON, statisticalEventInfo_.stopReason);
    meta->SetData(Tag::SCREEN_CAPTURE_START_LATENCY, statisticalEventInfo_.startLatency);
    AppendMediaInfo(meta, instanceId_);
    ReportMediaInfo(instanceId_);
}

ScreenCaptureServer::ScreenCaptureServer()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    InitAppInfo();
    instanceId_ = OHOS::HiviewDFX::HiTraceChain::GetId().GetChainId();
    CreateMediaInfo(SCREEN_CAPTRUER, IPCSkeleton::GetCallingUid(), instanceId_);
}

ScreenCaptureServer::~ScreenCaptureServer()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    ReleaseInner();
}

void ScreenCaptureServer::SetSessionId(int32_t sessionId)
{
    sessionId_ = sessionId;
}

int32_t ScreenCaptureServer::SetCaptureMode(CaptureMode captureMode)
{
    MediaTrace trace("ScreenCaptureServer::SetCaptureMode");
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "SetCaptureMode failed, capture is not CREATED, state:%{public}d, mode:%{public}d", captureState_, captureMode);
    MEDIA_LOGI("ScreenCaptureServer::SetCaptureMode start, captureMode:%{public}d", captureMode);
    int32_t ret = CheckCaptureMode(captureMode);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
    captureConfig_.captureMode = captureMode;
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "SetCaptureMode OK.", FAKE_POINTER(this));
    return MSERR_OK;
}

int32_t ScreenCaptureServer::SetDataType(DataType dataType)
{
    MediaTrace trace("ScreenCaptureServer::SetDataType");
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "SetDataType failed, capture is not CREATED, state:%{public}d, dataType:%{public}d", captureState_, dataType);
    MEDIA_LOGI("ScreenCaptureServer::SetDataType start, dataType:%{public}d", dataType);
    int32_t ret = CheckDataType(dataType);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
    captureConfig_.dataType = dataType;
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "SetDataType OK.", FAKE_POINTER(this));
    return MSERR_OK;
}

int32_t ScreenCaptureServer::SetRecorderInfo(RecorderInfo recorderInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "SetRecorderInfo failed, capture is not CREATED, state:%{public}d", captureState_);
    MEDIA_LOGI("ScreenCaptureServer::SetRecorderInfo start");
    url_ = recorderInfo.url;
    avType_ = AVScreenCaptureAvType::AV_TYPE;

    if (MP4.compare(recorderInfo.fileFormat) == 0) {
        fileFormat_ = OutputFormatType::FORMAT_MPEG_4;
    } else if (M4A.compare(recorderInfo.fileFormat) == 0) {
        MEDIA_LOGI("only recorder audio, still not support");
        FaultScreenCaptureEventWrite(appName_, instanceId_, avType_, dataMode_, SCREEN_CAPTURE_ERR_UNSUPPORT,
            "only recorder audio, still not support");
        return MSERR_UNSUPPORT;
    } else {
        MEDIA_LOGE("invalid fileFormat type");
        FaultScreenCaptureEventWrite(appName_, instanceId_, avType_, dataMode_,
            SCREEN_CAPTURE_ERR_INVALID_VAL, "invalid fileFormat type");
        return MSERR_INVALID_VAL;
    }
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "SetRecorderInfo OK.", FAKE_POINTER(this));
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
        FaultScreenCaptureEventWrite(appName_, instanceId_, avType_, dataMode_,
            SCREEN_CAPTURE_ERR_INVALID_VAL, "invalid outputFd");
        return MSERR_INVALID_VAL;
    }

    int flags = fcntl(outputFd, F_GETFL);
    if (flags == -1) {
        MEDIA_LOGE("Fail to get File Status Flags");
        FaultScreenCaptureEventWrite(appName_, instanceId_, avType_, dataMode_, SCREEN_CAPTURE_ERR_INVALID_VAL,
            "Fail to get File Status Flags");
        return MSERR_INVALID_VAL;
    }
    if ((static_cast<unsigned int>(flags) & (O_RDWR | O_WRONLY)) == 0) {
        MEDIA_LOGE("File descriptor is not in read-write mode or write-only mode");
        FaultScreenCaptureEventWrite(appName_, instanceId_, avType_, dataMode_, SCREEN_CAPTURE_ERR_INVALID_VAL,
            "File descriptor is not in read-write mode or write-only mode");
        return MSERR_INVALID_VAL;
    }

    CloseFd();
    outputFd_ = dup(outputFd);
    MEDIA_LOGI("ScreenCaptureServer SetOutputFile ok");
    return MSERR_OK;
}

int32_t ScreenCaptureServer::SetScreenCaptureCallback(const std::shared_ptr<ScreenCaptureCallBack> &callback)
{
    MediaTrace trace("ScreenCaptureServer::SetScreenCaptureCallback");
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "SetScreenCaptureCallback failed, capture is not CREATED, state:%{public}d", captureState_);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_INVALID_VAL,
        "SetScreenCaptureCallback failed, callback is nullptr, state:%{public}d", captureState_);
    MEDIA_LOGI("ScreenCaptureServer::SetScreenCaptureCallback start");
    screenCaptureCb_ = callback;
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "SetScreenCaptureCallback OK.", FAKE_POINTER(this));
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
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "InitVideoEncInfo failed, ret:%{public}d", ret);
    captureConfig_.videoInfo.videoEncInfo = videoEncInfo;
    return MSERR_OK;
}

bool ScreenCaptureServer::CheckScreenCapturePermission()
{
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

bool ScreenCaptureServer::IsUserPrivacyAuthorityNeeded()
{
    MediaTrace trace("ScreenCaptureServer::IsUserPrivacyAuthorityNeeded");
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "IsUserPrivacyAuthorityNeeded start, appUid:%{public}d",
        FAKE_POINTER(this), appInfo_.appUid);
    if (appInfo_.appUid == ROOT_UID) {
        MEDIA_LOGI("Root user. Privacy Authority Granted automaticly");
        return false;
    }
    return true;
}

int32_t ScreenCaptureServer::CheckCaptureMode(CaptureMode captureMode)
{
    MEDIA_LOGD("CheckCaptureMode start, captureMode:%{public}d", captureMode);
    if ((captureMode > CAPTURE_SPECIFIED_WINDOW) || (captureMode < CAPTURE_HOME_SCREEN)) {
        MEDIA_LOGE("invalid captureMode:%{public}d", captureMode);
        return MSERR_INVALID_VAL;
    }
    MEDIA_LOGD("ScreenCaptureServer CheckCaptureMode OK.");
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
    MEDIA_LOGD("ScreenCaptureServer CheckDataType OK.");
    return MSERR_OK;
}

int32_t ScreenCaptureServer::CheckAudioCapParam(const AudioCaptureInfo &audioCapInfo)
{
    MEDIA_LOGD("CheckAudioCapParam sampleRate:%{public}d, channels:%{public}d, source:%{public}d, state:%{public}d",
        audioCapInfo.audioSampleRate, audioCapInfo.audioChannels, audioCapInfo.audioSource, audioCapInfo.state);
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
    MEDIA_LOGD("ScreenCaptureServer CheckAudioCapParam OK.");
    return MSERR_OK;
}

int32_t ScreenCaptureServer::CheckVideoCapParam(const VideoCaptureInfo &videoCapInfo)
{
    MEDIA_LOGD("CheckVideoCapParam width:%{public}d, height:%{public}d, source:%{public}d, state:%{public}d",
        videoCapInfo.videoFrameWidth, videoCapInfo.videoFrameHeight, videoCapInfo.videoSource, videoCapInfo.state);
    if ((videoCapInfo.videoFrameWidth <= 0) || (videoCapInfo.videoFrameWidth > VIDEO_FRAME_WIDTH_MAX)) {
        MEDIA_LOGE("videoCapInfo Width is invalid, videoFrameWidth:%{public}d, videoFrameHeight:%{public}d",
            videoCapInfo.videoFrameWidth, videoCapInfo.videoFrameHeight);
        return MSERR_INVALID_VAL;
    }
    if ((videoCapInfo.videoFrameHeight <= 0) || (videoCapInfo.videoFrameHeight > VIDEO_FRAME_HEIGHT_MAX)) {
        MEDIA_LOGE("videoCapInfo Height is invalid, videoFrameWidth:%{public}d, videoFrameHeight:%{public}d",
            videoCapInfo.videoFrameWidth, videoCapInfo.videoFrameHeight);
        return MSERR_INVALID_VAL;
    }

    if (videoCapInfo.videoSource != VIDEO_SOURCE_SURFACE_RGBA) {
        MEDIA_LOGE("videoSource is invalid");
        return MSERR_INVALID_VAL;
    }
    MEDIA_LOGD("ScreenCaptureServer CheckVideoCapParam OK.");
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
    MEDIA_LOGD("ScreenCaptureServer CheckAudioCapInfo start, audioChannels:%{public}d, "
        "audioSampleRate:%{public}d, audioSource:%{public}d, state:%{public}d.",
        audioCapInfo.audioChannels, audioCapInfo.audioSampleRate, audioCapInfo.audioSource, audioCapInfo.state);
    if (audioCapInfo.audioChannels == 0 && audioCapInfo.audioSampleRate == 0) {
        MEDIA_LOGD("audioCap IGNORED sampleRate:%{public}d, channels:%{public}d, source:%{public}d, state:%{public}d",
            audioCapInfo.audioSampleRate, audioCapInfo.audioChannels, audioCapInfo.audioSource, audioCapInfo.state);
        audioCapInfo.state = AVScreenCaptureParamValidationState::VALIDATION_IGNORE;
        return MSERR_OK;
    }
    MEDIA_LOGD("CheckAudioCapParam S sampleRate:%{public}d, channels:%{public}d, source:%{public}d, state:%{public}d",
        audioCapInfo.audioSampleRate, audioCapInfo.audioChannels, audioCapInfo.audioSource, audioCapInfo.state);
    int32_t ret = CheckAudioCapParam(audioCapInfo);
    audioCapInfo.state = ret == MSERR_OK ? AVScreenCaptureParamValidationState::VALIDATION_VALID :
        AVScreenCaptureParamValidationState::VALIDATION_INVALID;
    MEDIA_LOGD("CheckAudioCapParam E sampleRate:%{public}d, channels:%{public}d, source:%{public}d, state:%{public}d",
        audioCapInfo.audioSampleRate, audioCapInfo.audioChannels, audioCapInfo.audioSource, audioCapInfo.state);
    MEDIA_LOGD("ScreenCaptureServer CheckAudioCapInfo end.");
    return ret;
}

int32_t ScreenCaptureServer::CheckVideoCapInfo(VideoCaptureInfo &videoCapInfo)
{
    MEDIA_LOGD("CheckVideoCapInfo start, videoFrameWidth:%{public}d, videoFrameHeight:%{public}d, "
        "videoSource:%{public}d, state:%{public}d.", videoCapInfo.videoFrameWidth, videoCapInfo.videoFrameHeight,
        videoCapInfo.videoSource, videoCapInfo.state);
    if (videoCapInfo.videoFrameWidth == 0 && videoCapInfo.videoFrameHeight == 0) {
        MEDIA_LOGD("videoCap IGNORED width:%{public}d, height:%{public}d, source:%{public}d, state:%{public}d",
            videoCapInfo.videoFrameWidth, videoCapInfo.videoFrameHeight, videoCapInfo.videoSource, videoCapInfo.state);
        videoCapInfo.state = AVScreenCaptureParamValidationState::VALIDATION_IGNORE;
        return MSERR_OK;
    }
    MEDIA_LOGD("CheckVideoCapParam S width:%{public}d, height:%{public}d, source:%{public}d, state:%{public}d",
        videoCapInfo.videoFrameWidth, videoCapInfo.videoFrameHeight, videoCapInfo.videoSource, videoCapInfo.state);
    int32_t ret = CheckVideoCapParam(videoCapInfo);
    videoCapInfo.state = ret == MSERR_OK ? AVScreenCaptureParamValidationState::VALIDATION_VALID :
        AVScreenCaptureParamValidationState::VALIDATION_INVALID;
    MEDIA_LOGD("CheckVideoCapParam E width:%{public}d, height:%{public}d, source:%{public}d, state:%{public}d",
        videoCapInfo.videoFrameWidth, videoCapInfo.videoFrameHeight, videoCapInfo.videoSource, videoCapInfo.state);
    MEDIA_LOGD("ScreenCaptureServer CheckVideoCapInfo end.");
    return ret;
}

int32_t ScreenCaptureServer::CheckAudioEncInfo(AudioEncInfo &audioEncInfo)
{
    MEDIA_LOGD("ScreenCaptureServer CheckAudioEncInfo start.");
    int32_t ret = CheckAudioEncParam(audioEncInfo);
    audioEncInfo.state = ret == MSERR_OK ? AVScreenCaptureParamValidationState::VALIDATION_VALID :
        AVScreenCaptureParamValidationState::VALIDATION_INVALID;
    MEDIA_LOGD("ScreenCaptureServer CheckAudioEncInfo end, state: %{public}d.", audioEncInfo.state);
    return ret;
}

int32_t ScreenCaptureServer::CheckVideoEncInfo(VideoEncInfo &videoEncInfo)
{
    MEDIA_LOGD("ScreenCaptureServer CheckVideoEncInfo start.");
    int32_t ret = CheckVideoEncParam(videoEncInfo);
    videoEncInfo.state = ret == MSERR_OK ? AVScreenCaptureParamValidationState::VALIDATION_VALID :
        AVScreenCaptureParamValidationState::VALIDATION_INVALID;
    MEDIA_LOGD("ScreenCaptureServer CheckVideoEncInfo end, state: %{public}d.", videoEncInfo.state);
    return ret;
}

int32_t ScreenCaptureServer::CheckAllParams()
{
    MEDIA_LOGD("ScreenCaptureServer: 0x%{public}06" PRIXPTR "CheckAllParams start, dataType:%{public}d.",
        FAKE_POINTER(this), captureConfig_.dataType);
    int32_t ret = CheckDataType(captureConfig_.dataType);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "CheckAllParams CheckDataType failed, ret:%{public}d", ret);

    ret = CheckCaptureMode(captureConfig_.captureMode);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "CheckAllParams CheckCaptureMode failed, ret:%{public}d", ret);

    if (captureConfig_.dataType == DataType::ORIGINAL_STREAM) {
        dataMode_ = AVScreenCaptureDataMode::BUFFER_MODE;
        return CheckCaptureStreamParams();
    }
    if (captureConfig_.dataType == DataType::CAPTURE_FILE) {
        dataMode_ = AVScreenCaptureDataMode::FILE_MODE;
        return CheckCaptureFileParams();
    }
    return MSERR_INVALID_VAL;
}

int32_t ScreenCaptureServer::CheckCaptureStreamParams()
{
    // For original stream:
    // 1. Any of innerCapInfo/videoCapInfo should be not invalid and should not be both ignored
    // 2. micCapInfo should not be invalid
    // 3. For surface mode, videoCapInfo should be valid
    CheckAudioCapInfo(captureConfig_.audioInfo.micCapInfo);
    CheckAudioCapInfo(captureConfig_.audioInfo.innerCapInfo);
    CheckVideoCapInfo(captureConfig_.videoInfo.videoCapInfo);
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "CheckCaptureStreamParams start, isSurfaceMode:%{public}s, "
        "videoCapInfo.state:%{public}d, innerCapInfo.state:%{public}d.", FAKE_POINTER(this),
        isSurfaceMode_ ? "true" : "false", captureConfig_.videoInfo.videoCapInfo.state,
        captureConfig_.audioInfo.innerCapInfo.state);
    if (isSurfaceMode_) {
        // surface mode, surface must not nullptr and videoCapInfo must valid.
        if (surface_ == nullptr ||
            captureConfig_.videoInfo.videoCapInfo.state != AVScreenCaptureParamValidationState::VALIDATION_VALID) {
            FaultScreenCaptureEventWrite(appName_, instanceId_, avType_, dataMode_, SCREEN_CAPTURE_ERR_INVALID_VAL,
                "video Cap state fault, videoCapInfo is invalid");
            return MSERR_INVALID_VAL;
        }
    }
    if (captureConfig_.audioInfo.innerCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_INVALID ||
        captureConfig_.videoInfo.videoCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_INVALID) {
        FaultScreenCaptureEventWrite(appName_, instanceId_, avType_, dataMode_, SCREEN_CAPTURE_ERR_INVALID_VAL,
            "audio inner cap or video cap state invalid");
        return MSERR_INVALID_VAL;
    }
    if (captureConfig_.audioInfo.innerCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_IGNORE &&
        captureConfig_.videoInfo.videoCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_IGNORE) {
        FaultScreenCaptureEventWrite(appName_, instanceId_, avType_, dataMode_, SCREEN_CAPTURE_ERR_INVALID_VAL,
            "audio inner cap or video cap state ignore");
        return MSERR_INVALID_VAL;
    }
    if (captureConfig_.audioInfo.innerCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_INVALID) {
        FaultScreenCaptureEventWrite(appName_, instanceId_, avType_, dataMode_, SCREEN_CAPTURE_ERR_INVALID_VAL,
            "audio inner cap state invalid");
        return MSERR_INVALID_VAL;
    }
    MEDIA_LOGD("ScreenCaptureServer: 0x%{public}06" PRIXPTR "CheckCaptureStreamParams OK.", FAKE_POINTER(this));
    return MSERR_OK;
}

int32_t ScreenCaptureServer::CheckCaptureFileParams()
{
    // For capture file:
    // 1. All of innerCapInfo/videoCapInfo/audioEncInfo/videoEncInfo should be be valid
    // 2. micCapInfo should not be invalid
    CheckAudioCapInfo(captureConfig_.audioInfo.micCapInfo);
    CheckAudioCapInfo(captureConfig_.audioInfo.innerCapInfo);
    CheckVideoCapInfo(captureConfig_.videoInfo.videoCapInfo);
    CheckAudioEncInfo(captureConfig_.audioInfo.audioEncInfo);
    CheckVideoEncInfo(captureConfig_.videoInfo.videoEncInfo);
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "CheckCaptureFileParams start, "
        "innerCapInfo.state:%{public}d, videoCapInfo.state:%{public}d, audioEncInfo.state:%{public}d, "
        "videoEncInfo.state:%{public}d, micCapInfo.state:%{public}d.", FAKE_POINTER(this),
        captureConfig_.audioInfo.innerCapInfo.state, captureConfig_.videoInfo.videoCapInfo.state,
        captureConfig_.audioInfo.audioEncInfo.state, captureConfig_.videoInfo.videoEncInfo.state,
        captureConfig_.audioInfo.micCapInfo.state);

    if (captureConfig_.audioInfo.innerCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_INVALID ||
        captureConfig_.videoInfo.videoCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_INVALID ||
        captureConfig_.audioInfo.audioEncInfo.state == AVScreenCaptureParamValidationState::VALIDATION_INVALID ||
        captureConfig_.videoInfo.videoEncInfo.state == AVScreenCaptureParamValidationState::VALIDATION_INVALID) {
        FaultScreenCaptureEventWrite(appName_, instanceId_, avType_, dataMode_, SCREEN_CAPTURE_ERR_INVALID_VAL,
            "innerCap audioEnc videoCap videoEnc state invalid");
        return MSERR_INVALID_VAL;
    }
    if (captureConfig_.audioInfo.micCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_INVALID) {
        FaultScreenCaptureEventWrite(appName_, instanceId_, avType_, dataMode_, SCREEN_CAPTURE_ERR_INVALID_VAL,
            "audio mic cap state invalid");
        return MSERR_INVALID_VAL;
    }
    if (captureConfig_.audioInfo.micCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_IGNORE) {
        return MSERR_OK;
    }
    const AudioCaptureInfo &micCapInfo = captureConfig_.audioInfo.micCapInfo;
    const AudioCaptureInfo &innerCapInfo = captureConfig_.audioInfo.innerCapInfo;
    if (micCapInfo.audioSampleRate == innerCapInfo.audioSampleRate &&
        micCapInfo.audioChannels == innerCapInfo.audioChannels) {
        return MSERR_OK;
    }
    MEDIA_LOGE("CheckCaptureFileParams failed, inner and mic param not consistent");
    FaultScreenCaptureEventWrite(appName_, instanceId_, avType_, dataMode_, SCREEN_CAPTURE_ERR_INVALID_VAL,
        "CheckCaptureFileParams failed, inner and mic param not consistent");
    return MSERR_INVALID_VAL;
}

// Should call in ipc thread
void ScreenCaptureServer::InitAppInfo()
{
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "InitAppInfo start.", FAKE_POINTER(this));
    appInfo_.appTokenId = IPCSkeleton::GetCallingTokenID();
    appInfo_.appFullTokenId = IPCSkeleton::GetCallingFullTokenID();
    appInfo_.appUid = IPCSkeleton::GetCallingUid();
    appInfo_.appPid = IPCSkeleton::GetCallingPid();
    appName_ = GetClientBundleName(appInfo_.appUid);
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "InitAppInfo end.", FAKE_POINTER(this));
}

int64_t ScreenCaptureServer::GetCurrentMillisecond()
{
    std::chrono::system_clock::duration duration = std::chrono::system_clock::now().time_since_epoch();
    int64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return time;
}

void ScreenCaptureServer::SetErrorInfo(int32_t errCode, std::string errMsg, StopReason stopReason, bool userAgree)
{
    statisticalEventInfo_.errCode = errCode;
    statisticalEventInfo_.errMsg = errMsg;
    statisticalEventInfo_.stopReason = stopReason;
    statisticalEventInfo_.userAgree = userAgree;
}

int32_t ScreenCaptureServer::RequestUserPrivacyAuthority()
{
    MediaTrace trace("ScreenCaptureServer::RequestUserPrivacyAuthority");
    // If Root is treated as whitelisted, how to guarantee RequestUserPrivacyAuthority function by TDD cases.
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "RequestUserPrivacyAuthority start.", FAKE_POINTER(this));
    if (!IsUserPrivacyAuthorityNeeded()) {
        MEDIA_LOGI("Privacy Authority Granted. uid:%{public}d", appInfo_.appUid);
        return MSERR_OK;
    }

#ifdef SUPPORT_SCREEN_CAPTURE_WINDOW_NOTIFICATION
    if (isPrivacyAuthorityEnabled_) {
        if (SCREEN_RECORDER_BUNDLE_NAME.compare(bundleName_) != 0) {
            return StartPrivacyWindow();
        } else {
            MEDIA_LOGI("ScreenCaptureServer::RequestUserPrivacyAuthority support screenrecorder");
            return MSERR_OK;
        }
    }
#endif
    MEDIA_LOGI("privacy notification window not support, go on to check CAPTURE_SCREEN permission");
    return CheckScreenCapturePermission() ? MSERR_OK : MSERR_INVALID_OPERATION;
}

int32_t ScreenCaptureServer::OnReceiveUserPrivacyAuthority(bool isAllowed)
{
    // Should callback be running in seperate thread?
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGI("OnReceiveUserPrivacyAuthority start, isAllowed:%{public}d, state:%{public}d", isAllowed, captureState_);
    if (screenCaptureCb_ == nullptr) {
        MEDIA_LOGE("OnReceiveUserPrivacyAuthority failed, screenCaptureCb is nullptr, state:%{public}d", captureState_);
        captureState_ = AVScreenCaptureState::STOPPED;
        SetErrorInfo(MSERR_UNKNOWN, "OnReceiveUserPrivacyAuthority failed, screenCaptureCb is nullptr",
            StopReason::RECEIVE_USER_PRIVACY_AUTHORITY_FAILED, IsUserPrivacyAuthorityNeeded());
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
    PostStartScreenCapture(ret == MSERR_OK);
    return ret;
}

int32_t ScreenCaptureServer::StartAudioCapture()
{
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "StartAudioCapture start, dataType:%{public}d, "
        "innerCapInfo.state:%{public}d, micCapInfo.state:%{public}d.",
        FAKE_POINTER(this), captureConfig_.dataType, captureConfig_.audioInfo.innerCapInfo.state,
        captureConfig_.audioInfo.micCapInfo.state);
    std::shared_ptr<AudioCapturerWrapper> innerCapture;
    if (captureConfig_.audioInfo.innerCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_VALID) {
        MediaTrace trace("ScreenCaptureServer::StartAudioCaptureInner");
        innerCapture = std::make_shared<AudioCapturerWrapper>(captureConfig_.audioInfo.innerCapInfo, screenCaptureCb_,
            std::string("OS_InnerAudioCapture"), contentFilter_);
        int32_t ret = innerCapture->Start(appInfo_);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "StartAudioCapture innerCapture failed");
    }
    std::shared_ptr<AudioCapturerWrapper> micCapture;
    if (captureConfig_.audioInfo.micCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_VALID) {
        MediaTrace trace("ScreenCaptureServer::StartAudioCaptureMic");
        ScreenCaptureContentFilter contentFilterMic;
        micCapture = std::make_shared<AudioCapturerWrapper>(captureConfig_.audioInfo.micCapInfo, screenCaptureCb_,
            std::string("OS_MicAudioCapture"), contentFilterMic);
        int32_t ret = micCapture->Start(appInfo_);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "StartAudioCapture micCapture failed");
        micCapture->SetIsMuted(!isMicrophoneOn_);
    }
    innerAudioCapture_ = innerCapture;
    micAudioCapture_ = micCapture;
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "StartAudioCapture OK.", FAKE_POINTER(this));
    return MSERR_OK;
}

int32_t ScreenCaptureServer::StartScreenCaptureStream()
{
    MediaTrace trace("ScreenCaptureServer::StartScreenCaptureStream");
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "StartScreenCaptureStream start, dataType:%{public}d.",
        FAKE_POINTER(this), captureConfig_.dataType);
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

    MEDIA_LOGI("StartScreenCaptureFile S");
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
        DestroyVirtualScreen();
    };

    MEDIA_LOGI("StartScreenCaptureFile RecorderServer S");

    ret = StartAudioCapture();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "StartAudioCapture failed, ret:%{public}d, dataType:%{public}d",
        ret, captureConfig_.dataType);
    ret = recorder_->Start();
    MEDIA_LOGI("StartScreenCaptureFile RecorderServer E");

    if (ret != MSERR_OK) {
        StopAudioCapture();
        MEDIA_LOGE("StartScreenCaptureFile recorder start failed");
        return ret;
    }

    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "recorder failed, ret:%{public}d, dataType:%{public}d",
        ret, captureConfig_.dataType);
    CANCEL_SCOPE_EXIT_GUARD(1);
    CANCEL_SCOPE_EXIT_GUARD(0);

    MEDIA_LOGI("StartScreenCaptureFile E");
    return ret;
}

int32_t ScreenCaptureServer::OnStartScreenCapture()
{
    MediaTrace trace("ScreenCaptureServer::OnStartScreenCapture");
    MEDIA_LOGI("OnStartScreenCapture start, dataType:%{public}d", captureConfig_.dataType);
    int32_t ret = MSERR_UNSUPPORT;
    if (captureConfig_.dataType == DataType::ORIGINAL_STREAM) {
        ret = StartScreenCaptureStream();
    } else if (captureConfig_.dataType == DataType::CAPTURE_FILE) {
        ret = StartScreenCaptureFile();
    }
    if (ret == MSERR_OK) {
        int64_t endTime = GetCurrentMillisecond();
        statisticalEventInfo_.startLatency = static_cast<int32_t>(endTime - startTime_);
        MEDIA_LOGI("OnStartScreenCapture start success, dataType:%{public}d", captureConfig_.dataType);
    } else {
        MEDIA_LOGE("OnStartScreenCapture start failed, dataType:%{public}d", captureConfig_.dataType);
        statisticalEventInfo_.startLatency = -1; // latency -1 means invalid
    }
    return ret;
}

void ScreenCaptureServer::ResSchedReportData(int64_t value, std::unordered_map<std::string, std::string> payload)
{
    payload["uid"] = std::to_string(appInfo_.appUid);
    payload["pid"] = std::to_string(appInfo_.appPid);
    uint32_t type = ResourceSchedule::ResType::RES_TYPE_REPORT_SCREEN_CAPTURE;
    ResourceSchedule::ResSchedClient::GetInstance().ReportData(type, value, payload);
}

void ScreenCaptureServer::PostStartScreenCapture(bool isSuccess)
{
    MediaTrace trace("ScreenCaptureServer::PostStartScreenCapture.");
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "PostStartScreenCapture start, isSuccess:%{public}s, "
        "dataType:%{public}d.", FAKE_POINTER(this), isSuccess ? "true" : "false", captureConfig_.dataType);
    if (isSuccess) {
        MEDIA_LOGI("PostStartScreenCapture handle success");
#ifdef SUPPORT_SCREEN_CAPTURE_WINDOW_NOTIFICATION
        if (isPrivacyAuthorityEnabled_ && SCREEN_RECORDER_BUNDLE_NAME.compare(bundleName_) != 0) {
            int32_t tryTimes = TryStartNotification();
            if (tryTimes > NOTIFICATION_MAX_TRY_NUM) {
                captureState_ = AVScreenCaptureState::STARTED;
                StopScreenCaptureInner(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_INVLID);
                screenCaptureCb_->OnError(ScreenCaptureErrorType::SCREEN_CAPTURE_ERROR_INTERNAL,
                    AVScreenCaptureErrorCode::SCREEN_CAPTURE_ERR_UNKNOWN);
                return;
            }
        }
#endif
        if (!UpdatePrivacyUsingPermissionState(START_VIDEO)) {
            MEDIA_LOGE("UpdatePrivacyUsingPermissionState START failed, dataType:%{public}d", captureConfig_.dataType);
        }
        std::unordered_map<std::string, std::string> payload;
        int64_t value = ResourceSchedule::ResType::ScreenCaptureStatus::START_SCREEN_CAPTURE;
        ResSchedReportData(value, payload);
        captureState_ = AVScreenCaptureState::STARTED;
        screenCaptureCb_->OnStateChange(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED);
    } else {
        MEDIA_LOGE("PostStartScreenCapture handle failure");
#ifdef SUPPORT_SCREEN_CAPTURE_WINDOW_NOTIFICATION
        if (isPrivacyAuthorityEnabled_) {
            screenCaptureCb_->OnError(ScreenCaptureErrorType::SCREEN_CAPTURE_ERROR_INTERNAL,
                AVScreenCaptureErrorCode::SCREEN_CAPTURE_ERR_UNKNOWN);
        }
#endif
        isPrivacyAuthorityEnabled_ = false;
        isSurfaceMode_ = false;
        captureState_ = AVScreenCaptureState::STOPPED;
        SetErrorInfo(MSERR_UNKNOWN, "PostStartScreenCapture handle failure",
            StopReason::POST_START_SCREENCAPTURE_HANDLE_FAILURE, IsUserPrivacyAuthorityNeeded());
    }
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "PostStartScreenCapture end.", FAKE_POINTER(this));
}

#ifdef SUPPORT_SCREEN_CAPTURE_WINDOW_NOTIFICATION
int32_t ScreenCaptureServer::TryStartNotification()
{
    int32_t tryTimes;
    for (tryTimes = 1; tryTimes <= NOTIFICATION_MAX_TRY_NUM; tryTimes++) {
        int32_t ret = StartNotification();
        if (ret == MSERR_OK) {
            break;
        }
    }
    return tryTimes;
}
#endif

int32_t ScreenCaptureServer::InitAudioCap(AudioCaptureInfo audioInfo)
{
    MediaTrace trace("ScreenCaptureServer::InitAudioCap");
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "InitAudioCap start, audioChannels:%{public}d, "
        "audioSampleRate:%{public}d, audioSource:%{public}d, state:%{public}d.", FAKE_POINTER(this),
        audioInfo.audioChannels, audioInfo.audioSampleRate, audioInfo.audioSource, audioInfo.state);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "InitAudioCap failed, capture is not CREATED, state:%{public}d", captureState_);

    int ret = CheckAudioCapInfo(audioInfo);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "InitAudioCap CheckAudioCapInfo failed, audioSource:%{public}d",
        audioInfo.audioSource);
    if (audioInfo.audioSource == AudioCaptureSourceType::SOURCE_DEFAULT ||
        audioInfo.audioSource == AudioCaptureSourceType::MIC) {
        captureConfig_.audioInfo.micCapInfo = audioInfo;
        statisticalEventInfo_.requireMic = true;
    } else if (audioInfo.audioSource == AudioCaptureSourceType::ALL_PLAYBACK ||
        audioInfo.audioSource == AudioCaptureSourceType::APP_PLAYBACK) {
        captureConfig_.audioInfo.innerCapInfo = audioInfo;
        avType_ = (avType_ == AVScreenCaptureAvType::INVALID_TYPE) ? AVScreenCaptureAvType::AUDIO_TYPE :
            AVScreenCaptureAvType::AV_TYPE;
    }
    MEDIA_LOGI("InitAudioCap success sampleRate:%{public}d, channels:%{public}d, source:%{public}d, state:%{public}d",
        audioInfo.audioSampleRate, audioInfo.audioChannels, audioInfo.audioSource, audioInfo.state);
    return MSERR_OK;
}

int32_t ScreenCaptureServer::InitVideoCap(VideoCaptureInfo videoInfo)
{
    MediaTrace trace("ScreenCaptureServer::InitVideoCap");
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "InitVideoCap failed, capture is not CREATED, state:%{public}d", captureState_);

    int ret = CheckVideoCapInfo(videoInfo);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "InitVideoCap CheckVideoCapInfo failed");
    captureConfig_.videoInfo.videoCapInfo = videoInfo;
    avType_ = (avType_ == AVScreenCaptureAvType::AUDIO_TYPE) ? AVScreenCaptureAvType::AV_TYPE :
        AVScreenCaptureAvType::VIDEO_TYPE;
    statisticalEventInfo_.videoResolution = std::to_string(videoInfo.videoFrameWidth) + "x" +
        std::to_string(videoInfo.videoFrameHeight);
    MEDIA_LOGI("InitVideoCap success width:%{public}d, height:%{public}d, source:%{public}d, state:%{public}d",
        videoInfo.videoFrameWidth, videoInfo.videoFrameHeight, videoInfo.videoSource, videoInfo.state);
    return MSERR_OK;
}

int32_t ScreenCaptureServer::InitRecorderInfo(std::shared_ptr<IRecorderService> &recorder, AudioCaptureInfo audioInfo)
{
    int32_t ret = MSERR_OK;
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
    return MSERR_OK;
}

int32_t ScreenCaptureServer::InitRecorder()
{
    CHECK_AND_RETURN_RET_LOG(outputFd_ > 0, MSERR_INVALID_OPERATION, "the outputFd is invalid");
    MEDIA_LOGI("InitRecorder start");
    MediaTrace trace("ScreenCaptureServer::InitRecorder");
    recorder_ = Media::RecorderServer::Create();
    CHECK_AND_RETURN_RET_LOG(recorder_ != nullptr, MSERR_UNKNOWN, "init Recoder failed");
    ON_SCOPE_EXIT(0) {
        recorder_->Release();
    };

    int32_t ret;
    AudioCaptureInfo audioInfo;

    if (captureConfig_.audioInfo.innerCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_VALID &&
        captureConfig_.audioInfo.micCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_VALID) {
        MEDIA_LOGI("InitRecorder prepare to SetAudioDataSource");
        audioInfo = captureConfig_.audioInfo.innerCapInfo;
        audioSource_ = std::make_unique<AudioDataSource>(AVScreenCaptureMixMode::MIX_MODE, this);
        ret = recorder_->SetAudioDataSource(audioSource_, audioSourceId_);
        MEDIA_LOGI("InitRecorder recorder SetAudioDataSource ret:%{public}d", ret);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "SetAudioDataSource failed");
    } else if (captureConfig_.audioInfo.innerCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_VALID) {
        audioInfo = captureConfig_.audioInfo.innerCapInfo;
        MEDIA_LOGI("InitRecorder prepare to SetAudioSource inner");
        audioSource_ = std::make_unique<AudioDataSource>(AVScreenCaptureMixMode::INNER_MODE, this);
        ret = recorder_->SetAudioDataSource(audioSource_, audioSourceId_);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "SetAudioDataSource failed");
    } else if (captureConfig_.audioInfo.micCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_VALID) {
        audioInfo = captureConfig_.audioInfo.micCapInfo;
        MEDIA_LOGI("InitRecorder prepare to SetAudioSource mic");
        audioSource_ = std::make_unique<AudioDataSource>(AVScreenCaptureMixMode::MIC_MODE, this);
        ret = recorder_->SetAudioDataSource(audioSource_, audioSourceId_);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "SetAudioDataSource failed");
    } else {
        MEDIA_LOGE("InitRecorder not VALIDATION_VALID");
        return MSERR_UNKNOWN;
    }

    ret = recorder_->SetVideoSource(captureConfig_.videoInfo.videoCapInfo.videoSource, videoSourceId_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "SetVideoSource failed");

    ret = InitRecorderInfo(recorder_, audioInfo);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "InitRecorderInfo failed");

    ret = recorder_->SetOutputFile(outputFd_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "SetOutputFile failed");
    ret = recorder_->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_UNKNOWN, "recorder Prepare failed");
    consumer_ = recorder_->GetSurface(videoSourceId_);
    CHECK_AND_RETURN_RET_LOG(consumer_ != nullptr, MSERR_UNKNOWN, "recorder GetSurface failed");
    CANCEL_SCOPE_EXIT_GUARD(0);
    MEDIA_LOGI("InitRecorder success");
    return MSERR_OK;
}

bool ScreenCaptureServer::UpdatePrivacyUsingPermissionState(VideoPermissionState state)
{
    MediaTrace trace("ScreenCaptureServer::UpdatePrivacyUsingPermissionState");
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "UpdatePrivacyUsingPermissionState start, "
        "state: %{public}d, uid: %{public}d", FAKE_POINTER(this), state, appInfo_.appUid);
    if (!IsUserPrivacyAuthorityNeeded()) {
        MEDIA_LOGI("Using Permission Ignored. state: %{public}d, uid: %{public}d", state, appInfo_.appUid);
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
    MEDIA_LOGI("StartScreenCaptureInner S, appUid:%{public}d, appPid:%{pulibc}d, isPrivacyAuthorityEnabled:%{pulibc}d"
        ", isSurfaceMode:%{public}d, dataType:%{public}d", appInfo_.appUid, appInfo_.appPid, isPrivacyAuthorityEnabled,
        isSurfaceMode_, captureConfig_.dataType);
    MediaTrace trace("ScreenCaptureServer::StartScreenCaptureInner");
    if (InCallObserver::GetInstance().IsInCall()) {
        MEDIA_LOGI("ScreenCaptureServer Start InCall Abort");
        screenCaptureCb_->OnStateChange(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STOPPED_BY_CALL);
        FaultScreenCaptureEventWrite(appName_, instanceId_, avType_, dataMode_, SCREEN_CAPTURE_ERR_UNSUPPORT,
            "ScreenCaptureServer Start InCall Abort");
        return MSERR_UNSUPPORT;
    } else {
        MEDIA_LOGI("ScreenCaptureServer Start RegisterScreenCaptureCallBack");
        InCallObserver::GetInstance().RegisterObserver();
        std::weak_ptr<ScreenCaptureServer> wpScreenCaptureServer(shared_from_this());
        screenCaptureObserverCb_ = std::make_shared<ScreenCaptureObserverCallBack>(wpScreenCaptureServer);
        InCallObserver::GetInstance().RegisterInCallObserverCallBack(screenCaptureObserverCb_);
    }

    int32_t ret = CheckAllParams();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "StartScreenCaptureInner failed, invalid params");

    sptr<Rosen::Display> display = Rosen::DisplayManager::GetInstance().GetDefaultDisplaySync();
    density_ = display->GetDpi();

    bundleName_ = GetClientBundleName(appInfo_.appUid);

    isPrivacyAuthorityEnabled_ = isPrivacyAuthorityEnabled;
    captureState_ = AVScreenCaptureState::STARTING;
    ret = RequestUserPrivacyAuthority();
    if (ret != MSERR_OK) {
        captureState_ = AVScreenCaptureState::STOPPED;
        SetErrorInfo(ret, "StartScreenCaptureInner RequestUserPrivacyAuthority failed",
            StopReason::REQUEST_USER_PRIVACY_AUTHORITY_FAILED, IsUserPrivacyAuthorityNeeded());
        MEDIA_LOGE("StartScreenCaptureInner RequestUserPrivacyAuthority failed");
        return ret;
    }

    if (IsUserPrivacyAuthorityNeeded()) {
#ifdef SUPPORT_SCREEN_CAPTURE_WINDOW_NOTIFICATION
        if (isPrivacyAuthorityEnabled_ && SCREEN_RECORDER_BUNDLE_NAME.compare(bundleName_) != 0) {
            // Wait for user interactions to ALLOW/DENY capture
            MEDIA_LOGI("Wait for user interactions to ALLOW/DENY capture");
            return MSERR_OK;
        }
#endif
        MEDIA_LOGI("privacy notification window not support, app has CAPTURE_SCREEN permission and go on");
    } else {
        MEDIA_LOGI("Privacy Authority granted automaticly and go on");
    }

    ret = OnStartScreenCapture();
    PostStartScreenCapture(ret == MSERR_OK);

    MEDIA_LOGI("StartScreenCaptureInner E, appUid:%{public}d, appPid:%{pulibc}d", appInfo_.appUid, appInfo_.appPid);
    return ret;
}

int32_t ScreenCaptureServer::StartPrivacyWindow()
{
    auto bundleName = GetClientBundleName(appInfo_.appUid);
    callingLabel_ = GetBundleResourceLabel(bundleName);

    std::string comStr = "{\"ability.want.params.uiExtensionType\":\"sys/commonUI\",\"sessionId\":\"";
    comStr += std::to_string(sessionId_).c_str();
    comStr += "\",\"callerUid\":\"";
    comStr += std::to_string(appInfo_.appUid).c_str();
    comStr += "\",\"appLabel\":\"";
    comStr += callingLabel_.c_str();
    comStr += "\"}";

    AAFwk::Want want;
    want.SetElementName(BUNDLE_NAME, ABILITY_NAME);
    auto connection_ = sptr<UIExtensionAbilityConnection>(new (std::nothrow) UIExtensionAbilityConnection(comStr));
    auto ret = OHOS::AAFwk::ExtensionManagerClient::GetInstance().ConnectServiceExtensionAbility(want, connection_,
        nullptr, -1);
    MEDIA_LOGI("ConnectServiceExtensionAbility end %{public}d", ret);
    return MSERR_OK;
}

int32_t ScreenCaptureServer::StartNotification()
{
    int32_t result = NotificationHelper::SubscribeLocalLiveViewNotification(NOTIFICATION_SUBSCRIBER);
    MEDIA_LOGD("Screencapture service PublishNotification, result %{public}d", result);
    NotificationRequest request;
    localLiveViewContent_ = GetLocalLiveViewContent();

    std::shared_ptr<NotificationContent> content =
        std::make_shared<NotificationContent>(localLiveViewContent_);

    auto uid = getuid();
    request.SetSlotType(NotificationConstant::SlotType::LIVE_VIEW);
    notificationId_ = sessionId_;
    request.SetNotificationId(notificationId_);
    request.SetContent(content);
    request.SetCreatorUid(uid);
    request.SetUnremovable(true);
    request.SetInProgress(true);

    std::shared_ptr<PixelMap> pixelMapTotalSpr = GetPixelMap(ICON_PATH_NOTIFICATION);
    request.SetLittleIcon(pixelMapTotalSpr);
    request.SetBadgeIconStyle(NotificationRequest::BadgeStyle::LITTLE);

    result = NotificationHelper::PublishNotification(request);
    MEDIA_LOGI("Screencapture service PublishNotification uid %{public}d, result %{public}d", uid, result);
    return result;
}

std::shared_ptr<NotificationLocalLiveViewContent> ScreenCaptureServer::GetLocalLiveViewContent()
{
    std::shared_ptr<NotificationLocalLiveViewContent> localLiveViewContent =
        std::make_shared<NotificationLocalLiveViewContent>();
    localLiveViewContent->SetType(1);
    liveViewText_ = "\"";
    liveViewText_ += callingLabel_.c_str();
    liveViewText_ += "\"正在使用屏幕";
    localLiveViewContent->SetText(liveViewText_);

    auto capsule = NotificationCapsule();
    capsule.SetBackgroundColor(BACK_GROUND_COLOR);
    capsulePxSize_ = capsuleVpSize_ * density_ / MDPI;
    std::shared_ptr<PixelMap> pixelMapCapSpr = GetPixelMapSvg(ICON_PATH_CAPSULE, capsulePxSize_, capsulePxSize_);
    capsule.SetIcon(pixelMapCapSpr);

    localLiveViewContent->SetCapsule(capsule);
    localLiveViewContent->addFlag(NotificationLocalLiveViewContent::LiveViewContentInner::CAPSULE);

    auto countTime = NotificationTime();
    countTime.SetInitialTime(1);
    countTime.SetIsCountDown(false);
    countTime.SetIsPaused(false);
    countTime.SetIsInTitle(true);

    localLiveViewContent->SetTime(countTime);
    localLiveViewContent->addFlag(NotificationLocalLiveViewContent::LiveViewContentInner::TIME);

    auto basicButton = NotificationLocalLiveViewButton();
    basicButton.addSingleButtonName(BUTTON_NAME_STOP);
    std::shared_ptr<PixelMap> pixelMapStopSpr = GetPixelMap(ICON_PATH_STOP);
    basicButton.addSingleButtonIcon(pixelMapStopSpr);

    localLiveViewContent->SetButton(basicButton);
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

std::shared_ptr<PixelMap> ScreenCaptureServer::GetPixelMapSvg(std::string path, int32_t width, int32_t height)
{
    uint32_t errorCode = 0;
    SourceOptions opts;
    opts.formatHint = "image/svg+xml";
    std::unique_ptr<ImageSource> imageSource =
        ImageSource::CreateImageSource(path, opts, errorCode);
    DecodeOptions decodeOpts;
    decodeOpts.desiredSize.width = width;
    decodeOpts.desiredSize.height = height;
    std::unique_ptr<PixelMap> pixelMap = imageSource->CreatePixelMap(decodeOpts, errorCode);
    std::shared_ptr<PixelMap> pixelMapSpr = std::move(pixelMap);
    return pixelMapSpr;
}

void ScreenCaptureServer::UpdateMicrophoneEnabled()
{
    UpdateLiveViewContent();
    NotificationRequest request;

    std::shared_ptr<NotificationContent> content =
        std::make_shared<NotificationContent>(localLiveViewContent_);

    auto uid = getuid();
    request.SetSlotType(NotificationConstant::SlotType::LIVE_VIEW);
    request.SetNotificationId(notificationId_);
    request.SetContent(content);
    request.SetCreatorUid(uid);
    request.SetUnremovable(true);
    request.SetInProgress(true);

    std::shared_ptr<PixelMap> pixelMapTotalSpr = GetPixelMap(ICON_PATH_CAPSULE);
    request.SetLittleIcon(pixelMapTotalSpr);
    request.SetBadgeIconStyle(NotificationRequest::BadgeStyle::LITTLE);

    int32_t result = NotificationHelper::PublishNotification(request);
    MEDIA_LOGI("Screencapture service UpdateMicrophoneEnabled uid %{public}d, result %{public}d", uid, result);
    micCount_.store(micCount_.load() + 1);
}

void ScreenCaptureServer::UpdateLiveViewContent()
{
    localLiveViewContent_->SetType(1);
    localLiveViewContent_->SetText(liveViewText_);

    auto capsule = NotificationCapsule();
    capsule.SetBackgroundColor(BACK_GROUND_COLOR);
    std::shared_ptr<PixelMap> pixelMapCapSpr = GetPixelMap(ICON_PATH_CAPSULE);
    capsule.SetIcon(pixelMapCapSpr);

    localLiveViewContent_->SetCapsule(capsule);
    localLiveViewContent_->addFlag(NotificationLocalLiveViewContent::LiveViewContentInner::CAPSULE);

    auto countTime = NotificationTime();
    countTime.SetIsCountDown(false);
    countTime.SetIsPaused(false);
    countTime.SetIsInTitle(true);

    localLiveViewContent_->SetTime(countTime);
    localLiveViewContent_->addFlag(NotificationLocalLiveViewContent::LiveViewContentInner::TIME);

    auto basicButton = NotificationLocalLiveViewButton();
    basicButton.addSingleButtonName(BUTTON_NAME_MIC);
    if (micCount_.load() % MICROPHONE_STATE_COUNT == MICROPHONE_OFF) {
        std::shared_ptr<PixelMap> pixelMapSpr = GetPixelMapSvg(ICON_PATH_MIC_OFF, SVG_HEIGHT, SVG_WIDTH);
        basicButton.addSingleButtonIcon(pixelMapSpr);
    } else {
        std::shared_ptr<PixelMap> pixelMapSpr = GetPixelMapSvg(ICON_PATH_MIC, SVG_HEIGHT, SVG_WIDTH);
        basicButton.addSingleButtonIcon(pixelMapSpr);
    }

    basicButton.addSingleButtonName(BUTTON_NAME_STOP);
    std::shared_ptr<PixelMap> pixelMapStopSpr = GetPixelMap(ICON_PATH_STOP);
    basicButton.addSingleButtonIcon(pixelMapStopSpr);

    localLiveViewContent_->SetButton(basicButton);
    localLiveViewContent_->addFlag(NotificationLocalLiveViewContent::LiveViewContentInner::BUTTON);
}

int32_t ScreenCaptureServer::StartScreenCapture(bool isPrivacyAuthorityEnabled)
{
    MediaTrace trace("ScreenCaptureServer::StartScreenCapture");
    std::lock_guard<std::mutex> lock(mutex_);
    startTime_ = GetCurrentMillisecond();
    statisticalEventInfo_.enableMic = isMicrophoneOn_;
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "StartScreenCapture start, "
        "isPrivacyAuthorityEnabled:%{public}s, captureState:%{public}d.",
        FAKE_POINTER(this), isPrivacyAuthorityEnabled ? "true" : "false", captureState_);
    CHECK_AND_RETURN_RET_LOG(
        captureState_ == AVScreenCaptureState::CREATED || captureState_ == AVScreenCaptureState::STOPPED,
        MSERR_INVALID_OPERATION, "StartScreenCapture failed, not in CREATED or STOPPED, state:%{public}d",
        captureState_);
    MEDIA_LOGI("StartScreenCapture isPrivacyAuthorityEnabled:%{public}d", isPrivacyAuthorityEnabled);
    isSurfaceMode_ = false;
    return StartScreenCaptureInner(isPrivacyAuthorityEnabled);
}

int32_t ScreenCaptureServer::StartScreenCaptureWithSurface(sptr<Surface> surface, bool isPrivacyAuthorityEnabled)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(
        captureState_ == AVScreenCaptureState::CREATED || captureState_ == AVScreenCaptureState::STOPPED,
        MSERR_INVALID_OPERATION, "StartScreenCaptureWithSurface failed, not in CREATED or STOPPED, state:%{public}d",
        captureState_);
    MEDIA_LOGI("StartScreenCaptureWithSurface isPrivacyAuthorityEnabled:%{public}d", isPrivacyAuthorityEnabled);
    if (surface == nullptr) {
        MEDIA_LOGE("surface is nullptr");
        return MSERR_INVALID_OPERATION;
    }
    surface_ = surface;
    isSurfaceMode_ = true;
    dataMode_ = AVScreenCaptureDataMode::SUFFACE_MODE;
    return StartScreenCaptureInner(isPrivacyAuthorityEnabled);
}

int32_t ScreenCaptureServer::StartVideoCapture()
{
    MediaTrace trace("ScreenCaptureServer::StartVideoCapture");
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "StartVideoCapture start, state:%{public}d, "
        "dataType:%{public}d, isSurfaceMode:%{public}s.", FAKE_POINTER(this),
        captureConfig_.videoInfo.videoCapInfo.state, captureConfig_.dataType, isSurfaceMode_ ? "true" : "false");
    if (captureConfig_.videoInfo.videoCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_IGNORE) {
        MEDIA_LOGI("StartVideoCapture is ignored");
        return MSERR_OK;
    }
    CHECK_AND_RETURN_RET_LOG(
        captureConfig_.videoInfo.videoCapInfo.state == AVScreenCaptureParamValidationState::VALIDATION_VALID,
        MSERR_INVALID_VAL, "StartVideoCapture failed, invalid param, dataType:%{public}d", captureConfig_.dataType);

    int32_t ret = StartHomeVideoCapture();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret,
        "StartHomeVideoCapture failed, isSurfaceMode:%{public}d, dataType:%{public}d",
        isSurfaceMode_, captureConfig_.dataType);
    MEDIA_LOGI("StartVideoCapture end.");
    return MSERR_OK;
}

int32_t ScreenCaptureServer::StartHomeVideoCapture()
{
    MediaTrace trace("ScreenCaptureServer::StartHomeVideoCapture");
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "StartHomeVideoCapture start, isSurfaceMode:%{public}s.",
        FAKE_POINTER(this), isSurfaceMode_ ? "true" : "false");
    std::string virtualScreenName = "screen_capture";
    if (isSurfaceMode_) {
        int32_t ret = CreateVirtualScreen(virtualScreenName, surface_);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "create virtual screen with input surface failed");
        return MSERR_OK;
    }

    ON_SCOPE_EXIT(0) {
        DestroyVirtualScreen();
        if (consumer_ != nullptr && surfaceCb_ != nullptr) {
            consumer_->UnregisterConsumerListener();
        }
        consumer_ = nullptr;
        surfaceCb_ = nullptr;
    };
    consumer_ = OHOS::Surface::CreateSurfaceAsConsumer();
    CHECK_AND_RETURN_RET_LOG(consumer_ != nullptr, MSERR_UNKNOWN, "CreateSurfaceAsConsumer failed");
    auto producer = consumer_->GetProducer();
    CHECK_AND_RETURN_RET_LOG(producer != nullptr, MSERR_UNKNOWN, "GetProducer failed");
    auto producerSurface = OHOS::Surface::CreateSurfaceAsProducer(producer);
    CHECK_AND_RETURN_RET_LOG(producerSurface != nullptr, MSERR_UNKNOWN, "CreateSurfaceAsProducer failed");
    surfaceCb_ = OHOS::sptr<ScreenCapBufferConsumerListener>::MakeSptr(consumer_, screenCaptureCb_);
    CHECK_AND_RETURN_RET_LOG(surfaceCb_ != nullptr, MSERR_UNKNOWN, "MakeSptr surfaceCb_ failed");
    consumer_->RegisterConsumerListener(surfaceCb_);
    int32_t ret = CreateVirtualScreen(virtualScreenName, producerSurface);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "create virtual screen without input surface failed");
    CANCEL_SCOPE_EXIT_GUARD(0);
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "StartHomeVideoCapture OK.", FAKE_POINTER(this));
    return MSERR_OK;
}

int32_t ScreenCaptureServer::CreateVirtualScreen(const std::string &name, sptr<OHOS::Surface> consumer)
{
    MediaTrace trace("ScreenCaptureServer::CreateVirtualScreen");
    MEDIA_LOGI("CreateVirtualScreen Start");
    isConsumerStart_ = false;
    VirtualScreenOption virScrOption = InitVirtualScreenOption(name, consumer);
    sptr<Rosen::Display> display = Rosen::DisplayManager::GetInstance().GetDefaultDisplaySync();
    if (display != nullptr) {
        MEDIA_LOGI("get displayInfo width:%{public}d,height:%{public}d,density:%{public}d", display->GetWidth(),
                   display->GetHeight(), display->GetDpi());
        virScrOption.density_ = display->GetDpi();
    }
    if (missionIds_.size() > 0 && captureConfig_.captureMode == CAPTURE_SPECIFIED_WINDOW) {
        virScrOption.missionIds_ = missionIds_;
    } else {
        if (captureConfig_.videoInfo.videoCapInfo.taskIDs.size() > 0 &&
            captureConfig_.captureMode == CAPTURE_SPECIFIED_WINDOW) {
            GetMissionIds(missionIds_);
            virScrOption.missionIds_ = missionIds_;
        }
    }
    screenId_ = ScreenManager::GetInstance().CreateVirtualScreen(virScrOption);
    CHECK_AND_RETURN_RET_LOG(screenId_ >= 0, MSERR_UNKNOWN, "CreateVirtualScreen failed, invalid screenId");
    for (size_t i = 0; i < contentFilter_.windowIDsVec.size(); i++) {
        MEDIA_LOGI("After CreateVirtualScreen windowIDsVec value :%{public}d", contentFilter_.windowIDsVec[i]);
    }
    auto screen = ScreenManager::GetInstance().GetScreenById(screenId_);
    if (screen == nullptr) {
        MEDIA_LOGE("GetScreenById failed");
        DestroyVirtualScreen();
        FaultScreenCaptureEventWrite(appName_, instanceId_, avType_, dataMode_, SCREEN_CAPTURE_ERR_UNKNOWN,
            "GetScreenById failed");
        return MSERR_UNKNOWN;
    }
    if (canvasRotation_) {
        SetCanvasRotationInner();
    }
    int32_t ret = MakeVirtualScreenMirror();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("MakeVirtualScreenMirror failed");
        DestroyVirtualScreen();
        FaultScreenCaptureEventWrite(appName_, instanceId_, avType_, dataMode_, SCREEN_CAPTURE_ERR_UNKNOWN,
            "MakeVirtualScreenMirror failed");
        return MSERR_UNKNOWN;
    }

    isConsumerStart_ = true;
    MEDIA_LOGI("CreateVirtualScreen success");
    return MSERR_OK;
}

int32_t ScreenCaptureServer::MakeVirtualScreenMirror()
{
    MediaTrace trace("ScreenCaptureServer::MakeVirtualScreenMirror");
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "MakeVirtualScreenMirror start.", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenId_ >= 0 && screenId_ != SCREEN_ID_INVALID, MSERR_UNKNOWN,
        "MakeVirtualScreenMirror failed, invalid screenId");
    std::vector<sptr<Screen>> screens;
    DMError ret = ScreenManager::GetInstance().GetAllScreens(screens);
    CHECK_AND_RETURN_RET_LOG(screens.size() > 0, MSERR_UNKNOWN,
        "MakeVirtualScreenMirror failed to GetAllScreens, ret:%{public}d", ret);
    std::vector<ScreenId> mirrorIds;
    mirrorIds.push_back(screenId_);
    sptr<Rosen::Display> defaultDisplay = Rosen::DisplayManager::GetInstance().GetDefaultDisplaySync();
    ScreenId mirrorGroup = defaultDisplay->GetScreenId();

    if (captureConfig_.captureMode != CAPTURE_SPECIFIED_SCREEN) {
        MEDIA_LOGI("MakeVirtualScreenMirror DefaultDisplay, screenId:%{public}" PRIu64, defaultDisplay->GetScreenId());
        ret = ScreenManager::GetInstance().MakeMirror(defaultDisplay->GetScreenId(), mirrorIds, mirrorGroup);
        CHECK_AND_RETURN_RET_LOG(ret == DMError::DM_OK, MSERR_UNKNOWN,
            "MakeVirtualScreenMirror failed to MakeMirror, captureMode:%{public}d, ret:%{public}d",
            captureConfig_.captureMode, ret);
        MEDIA_LOGI("MakeVirtualScreenMirror main screen success, screenId:%{public}" PRIu64, screens[0]->GetId());
        return MSERR_OK;
    }
    for (uint32_t i = 0; i < screens.size() ; i++) {
        if (screens[i]->GetId() == captureConfig_.videoInfo.videoCapInfo.displayId) {
            ret = ScreenManager::GetInstance().MakeMirror(screens[i]->GetId(), mirrorIds, mirrorGroup);
            CHECK_AND_RETURN_RET_LOG(ret == DMError::DM_OK, MSERR_UNKNOWN,
                "MakeVirtualScreenMirror failed to MakeMirror for CAPTURE_SPECIFIED_SCREEN, ret:%{public}d", ret);
            MEDIA_LOGI("MakeVirtualScreenMirror extand screen success, screenId:%{public}" PRIu64,
                captureConfig_.videoInfo.videoCapInfo.displayId);
            return MSERR_OK;
        }
    }
    MEDIA_LOGE("MakeVirtualScreenMirror failed to find screenId:%{public}" PRIu64,
        captureConfig_.videoInfo.videoCapInfo.displayId);
    FaultScreenCaptureEventWrite(appName_, instanceId_, avType_, dataMode_, SCREEN_CAPTURE_ERR_UNKNOWN,
        "MakeVirtualScreenMirror failed to find screenId");
    return MSERR_UNKNOWN;
}

void ScreenCaptureServer::DestroyVirtualScreen()
{
    MediaTrace trace("ScreenCaptureServer::DestroyVirtualScreen");
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "DestroyVirtualScreen start.", FAKE_POINTER(this));
    if (screenId_ >=0 && screenId_ != SCREEN_ID_INVALID) {
        if (isConsumerStart_) {
            std::vector<ScreenId> screenIds;
            screenIds.push_back(screenId_);
            ScreenManager::GetInstance().StopMirror(screenIds);
        }
        ScreenManager::GetInstance().DestroyVirtualScreen(screenId_);
        screenId_ = SCREEN_ID_INVALID;
        isConsumerStart_ = false;
    }
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "DestroyVirtualScreen end.", FAKE_POINTER(this));
}

void ScreenCaptureServer::CloseFd()
{
    MediaTrace trace("ScreenCaptureServer::CloseFd");
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "CloseFd start, outputFd:%{public}d.", FAKE_POINTER(this),
        outputFd_);
    if (outputFd_ > 0) {
        (void)::close(outputFd_);
        outputFd_ = -1;
    }
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "CloseFd end.", FAKE_POINTER(this));
}
VirtualScreenOption ScreenCaptureServer::InitVirtualScreenOption(const std::string &name, sptr<OHOS::Surface> consumer)
{
    MediaTrace trace("ScreenCaptureServer::InitVirtualScreenOption");
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "InitVirtualScreenOption start, naem:%{public}s.",
        FAKE_POINTER(this), name.c_str());
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
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "InitVirtualScreenOption end.", FAKE_POINTER(this));
    return virScrOption;
}

int32_t ScreenCaptureServer::GetMissionIds(std::vector<uint64_t> &missionIds)
{
    int32_t size = static_cast<int32_t>(captureConfig_.videoInfo.videoCapInfo.taskIDs.size());
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
    MediaTrace trace("ScreenCaptureServer::AcquireAudioBuffer");
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("ScreenCaptureServer: 0x%{public}06" PRIXPTR "AcquireAudioBuffer start, state:%{public}d, "
        "type:%{public}d.", FAKE_POINTER(this), captureState_, type);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::STARTED, MSERR_INVALID_OPERATION,
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
    FaultScreenCaptureEventWrite(appName_, instanceId_, avType_, dataMode_, SCREEN_CAPTURE_ERR_UNKNOWN,
        "AcquireAudioBuffer failed, source type not support");
    return MSERR_UNKNOWN;
}

int32_t ScreenCaptureServer::AcquireAudioBufferMix(std::shared_ptr<AudioBuffer> &innerAudioBuffer,
    std::shared_ptr<AudioBuffer> &micAudioBuffer, AVScreenCaptureMixMode type)
{
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::STARTED, MSERR_INVALID_OPERATION,
        "AcquireAudioBuffer failed, capture is not STARTED, state:%{public}d, type:%{public}d", captureState_, type);
    int32_t retInner = MSERR_OK;
    int32_t retMic = MSERR_OK;
    if (type == AVScreenCaptureMixMode::MIX_MODE && micAudioCapture_ != nullptr &&
        innerAudioCapture_ != nullptr) {
        retInner = innerAudioCapture_->AcquireAudioBuffer(innerAudioBuffer);
        retMic = micAudioCapture_->AcquireAudioBuffer(micAudioBuffer);
        if (retInner == MSERR_OK && retMic == MSERR_OK) {
            return MSERR_OK;
        } else {
            return MSERR_UNKNOWN;
        }
    }
    if (type == AVScreenCaptureMixMode::MIC_MODE && micAudioCapture_ != nullptr) {
        return micAudioCapture_->AcquireAudioBuffer(micAudioBuffer);
    }
    if (type == AVScreenCaptureMixMode::INNER_MODE && innerAudioCapture_ != nullptr) {
        return innerAudioCapture_->AcquireAudioBuffer(innerAudioBuffer);
    }
    MEDIA_LOGE("AcquireAudioBufferMix failed, source type not support, type:%{public}d", type);
    return MSERR_UNKNOWN;
}

int32_t ScreenCaptureServer::ReleaseAudioBuffer(AudioCaptureSourceType type)
{
    MediaTrace trace("ScreenCaptureServer::ReleaseAudioBuffer");
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("ScreenCaptureServer: 0x%{public}06" PRIXPTR "ReleaseAudioBuffer start, state:%{public}d, "
        "type:%{public}d.", FAKE_POINTER(this), captureState_, type);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::STARTED, MSERR_INVALID_OPERATION,
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
    FaultScreenCaptureEventWrite(appName_, instanceId_, avType_, dataMode_, SCREEN_CAPTURE_ERR_UNKNOWN,
        "ReleaseAudioBuffer failed, source type not support");
    return MSERR_UNKNOWN;
}

int32_t ScreenCaptureServer::ReleaseAudioBufferMix(AVScreenCaptureMixMode type)
{
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::STARTED, MSERR_INVALID_OPERATION,
        "ReleaseAudioBuffer failed, capture is not STARTED, state:%{public}d, type:%{public}d", captureState_, type);
    int32_t retInner = MSERR_OK;
    int32_t retMic = MSERR_OK;
    if (type == AVScreenCaptureMixMode::MIX_MODE && micAudioCapture_ != nullptr &&
        innerAudioCapture_ != nullptr) {
        retInner = innerAudioCapture_->ReleaseAudioBuffer();
        retMic = micAudioCapture_->ReleaseAudioBuffer();
        if (retInner == MSERR_OK && retMic == MSERR_OK) {
            return MSERR_OK;
        } else {
            return MSERR_UNKNOWN;
        }
    }
    if (type == AVScreenCaptureMixMode::MIC_MODE && micAudioCapture_ != nullptr) {
        return micAudioCapture_->ReleaseAudioBuffer();
    }
    if (type == AVScreenCaptureMixMode::INNER_MODE && innerAudioCapture_ != nullptr) {
        return innerAudioCapture_->ReleaseAudioBuffer();
    }
    MEDIA_LOGE("ReleaseAudioBufferMix failed, source type not support, type:%{public}d", type);
    return MSERR_UNKNOWN;
}

int32_t ScreenCaptureServer::GetInnerAudioCaptureBufferSize(size_t &size)
{
    if (innerAudioCapture_ == nullptr) {
        MEDIA_LOGE("innerAudioCapture_ is nullptr");
        return MSERR_UNKNOWN;
    }
    int32_t ret = innerAudioCapture_->GetBufferSize(size);
    return ret;
}

int32_t ScreenCaptureServer::GetMicAudioCaptureBufferSize(size_t &size)
{
    if (micAudioCapture_ == nullptr) {
        MEDIA_LOGE("micAudioCapture_ is nullptr");
        return MSERR_UNKNOWN;
    }
    int32_t ret = micAudioCapture_->GetBufferSize(size);
    return ret;
}

int32_t ScreenCaptureServer::AcquireVideoBuffer(sptr<OHOS::SurfaceBuffer> &surfaceBuffer, int32_t &fence,
                                                int64_t &timestamp, OHOS::Rect &damage)
{
    MediaTrace trace("ScreenCaptureServer::AcquireVideoBuffer");
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("ScreenCaptureServer: 0x%{public}06" PRIXPTR "AcquireVideoBuffer start, state:%{public}d, "
        "fence:%{public}d, timestamp:%{public}" PRId64, FAKE_POINTER(this), captureState_, fence, timestamp);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::STARTED, MSERR_INVALID_OPERATION,
        "AcquireVideoBuffer failed, capture is not STARTED, state:%{public}d", captureState_);

    CHECK_AND_RETURN_RET_LOG(surfaceCb_ != nullptr, MSERR_NO_MEMORY, "AcquireVideoBuffer failed, callback is nullptr");
    (static_cast<ScreenCapBufferConsumerListener *>(surfaceCb_.GetRefPtr()))->
        AcquireVideoBuffer(surfaceBuffer, fence, timestamp, damage);
    std::string dumpEnable;
    const std::string dumpTag = "sys.media.screenCapture.dump.enable";
    int32_t dumpRes = OHOS::system::GetStringParameter(dumpTag, dumpEnable, "");
    if (dumpRes == 0 && !dumpEnable.empty() && dumpEnable == "true") {
        if (surfaceBuffer != nullptr) {
            void* addr = surfaceBuffer->GetVirAddr();
            uint32_t bufferSize = surfaceBuffer->GetSize();
            std::string path = "/data/media/screen_capture.bin";
            FILE *desFile = fopen(path.c_str(), "wb+");
            if (desFile) {
                (void)fwrite(addr, 1, bufferSize, desFile);
                (void)fclose(desFile);
            }
        }
    }
    if (surfaceBuffer != nullptr) {
        MEDIA_LOGD("getcurrent surfaceBuffer info, size:%{public}u", surfaceBuffer->GetSize());
        return MSERR_OK;
    }
    FaultScreenCaptureEventWrite(appName_, instanceId_, avType_, dataMode_, SCREEN_CAPTURE_ERR_UNKNOWN,
        "AcquireVideoBuffer fault");
    MEDIA_LOGD("ScreenCaptureServer: 0x%{public}06" PRIXPTR "AcquireVideoBuffer end.", FAKE_POINTER(this));
    return MSERR_UNKNOWN;
}

int32_t ScreenCaptureServer::ReleaseVideoBuffer()
{
    MediaTrace trace("ScreenCaptureServer::ReleaseVideoBuffer");
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("ScreenCaptureServer: 0x%{public}06" PRIXPTR "ReleaseVideoBuffer start, state:%{public}d.",
        FAKE_POINTER(this), captureState_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::STARTED, MSERR_INVALID_OPERATION,
        "ReleaseVideoBuffer failed, capture is not STARTED, state:%{public}d", captureState_);

    CHECK_AND_RETURN_RET_LOG(surfaceCb_ != nullptr, MSERR_NO_MEMORY, "ReleaseVideoBuffer failed, callback is nullptr");
    MEDIA_LOGD("ScreenCaptureServer: 0x%{public}06" PRIXPTR "ReleaseVideoBuffer end.", FAKE_POINTER(this));
    return (static_cast<ScreenCapBufferConsumerListener *>(surfaceCb_.GetRefPtr()))->ReleaseVideoBuffer();
}

int32_t ScreenCaptureServer::ExcludeContent(ScreenCaptureContentFilter &contentFilter)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::CREATED, MSERR_INVALID_OPERATION,
        "ExcludeContent failed, capture is not STARTED, state:%{public}d", captureState_);

    MEDIA_LOGI("ScreenCaptureServer::ExcludeContent start");
    contentFilter_ = contentFilter;

    // For the moment, not support:
    // For STREAM, should call AudioCapturer interface to make effect when start
    // For CAPTURE FILE, should call Recorder interface to make effect when start
    FaultScreenCaptureEventWrite(appName_, instanceId_, avType_, dataMode_, SCREEN_CAPTURE_ERR_UNSUPPORT,
        "ExcludeContent failed, capture is not STARTED");
    return MSERR_OK;
}

int32_t ScreenCaptureServer::SetMicrophoneEnabled(bool isMicrophone)
{
    MediaTrace trace("ScreenCaptureServer::SetMicrophoneEnabled");
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "SetMicrophoneEnabled isMicrophoneOn_:%{public}d, "
        "new isMicrophone:%{public}d", FAKE_POINTER(this), isMicrophoneOn_, isMicrophone);
    isMicrophoneOn_ = isMicrophone;
    if (isMicrophone) {
        statisticalEventInfo_.enableMic = true;
    }
    if (captureState_ == AVScreenCaptureState::STARTED) {
        if (micAudioCapture_ != nullptr) {
            micAudioCapture_->SetIsMuted(!isMicrophone);
        }
        // For CAPTURE FILE, should call Recorder interface to make effect
    }
    MEDIA_LOGI("SetMicrophoneEnabled OK.");
    return MSERR_OK;
}

int32_t ScreenCaptureServer::SetCanvasRotation(bool canvasRotation)
{
    MediaTrace trace("ScreenCaptureServer::SetCanvasRotation");
    std::lock_guard<std::mutex> lock(mutex_);
    canvasRotation_ = canvasRotation;
    MEDIA_LOGI("ScreenCaptureServer::SetCanvasRotation, canvasRotation:%{public}d", canvasRotation);
    if (captureState_ != AVScreenCaptureState::STARTED) { // Before Start
        return MSERR_OK;
    }
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "SetCanvasRotation end.", FAKE_POINTER(this));
    return SetCanvasRotationInner();
}

int32_t ScreenCaptureServer::SetCanvasRotationInner()
{
    MediaTrace trace("ScreenCaptureServer::SetCanvasRotationInner");
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "SetCanvasRotationInner start.", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenId_ != SCREEN_ID_INVALID, MSERR_INVALID_VAL,
                             "SetCanvasRotation failed virtual screen not init");
    auto ret = ScreenManager::GetInstance().SetVirtualMirrorScreenCanvasRotation(screenId_, canvasRotation_);
    CHECK_AND_RETURN_RET_LOG(ret == DMError::DM_OK, MSERR_UNSUPPORT,
                             "SetVirtualMirrorScreenCanvasRotation failed, ret: %{public}d", ret);
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "SetCanvasRotationInner OK.", FAKE_POINTER(this));
    return MSERR_OK;
}

int32_t ScreenCaptureServer::StopAudioCapture()
{
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "StopAudioCapture start.", FAKE_POINTER(this));
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
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "StopAudioCapture end.", FAKE_POINTER(this));
    return MSERR_OK;
}

int32_t ScreenCaptureServer::StopVideoCapture()
{
    MediaTrace trace("ScreenCaptureServer::StopVideoCapture");
    MEDIA_LOGI("StopVideoCapture");
    if ((screenId_ < 0) || ((consumer_ == nullptr) && !isSurfaceMode_) || !isConsumerStart_) {
        MEDIA_LOGI("StopVideoCapture IGNORED, video capture not start");
        surfaceCb_ = nullptr;
        return MSERR_OK;
    }

    DestroyVirtualScreen();
    if (consumer_ != nullptr) {
        consumer_->UnregisterConsumerListener();
        consumer_ = nullptr;
    }

    if (surfaceCb_ != nullptr) {
        (static_cast<ScreenCapBufferConsumerListener *>(surfaceCb_.GetRefPtr()))->Release();
        surfaceCb_ = nullptr;
    }
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "StopVideoCapture end.", FAKE_POINTER(this));
    return MSERR_OK;
}

int32_t ScreenCaptureServer::StopScreenCaptureRecorder()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances StopScreenCaptureRecorder S", FAKE_POINTER(this));
    MediaTrace trace("ScreenCaptureServer::StopScreenCaptureRecorder");
    int32_t ret = MSERR_OK;
    if (recorder_ != nullptr) {
        ret = recorder_->Stop(false);
        if (ret != MSERR_OK) {
            MEDIA_LOGE("StopScreenCaptureRecorder recorder stop failed, ret:%{public}d", ret);
        }
        DestroyVirtualScreen();
        recorder_->Release();
        recorder_ = nullptr;
        StopAudioCapture();
    }

    isConsumerStart_ = false;
    CloseFd();
    return ret;
}

int32_t ScreenCaptureServer::StopScreenCaptureByEvent(AVScreenCaptureStateCode stateCode)
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances StopScreenCaptureByEvent S", FAKE_POINTER(this));
    MediaTrace trace("ScreenCaptureServer::StopScreenCaptureByEvent");
    std::lock_guard<std::mutex> lock(mutex_);
    return StopScreenCaptureInner(stateCode);
}

int32_t ScreenCaptureServer::StopScreenCaptureInner(AVScreenCaptureStateCode stateCode)
{
    MediaTrace trace("ScreenCaptureServer::StopScreenCaptureInner");
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "StopScreenCaptureInner start, stateCode:%{public}d.",
        FAKE_POINTER(this), stateCode);
    if (screenCaptureCb_ != nullptr) {
        (static_cast<ScreenCaptureListenerCallback *>(screenCaptureCb_.get()))->Stop();
    }
    if (captureState_ == AVScreenCaptureState::CREATED || captureState_ == AVScreenCaptureState::STARTING) {
        CloseFd();
        captureState_ = AVScreenCaptureState::STOPPED;
        isSurfaceMode_ = false;
        surface_ = nullptr;
        SetErrorInfo(MSERR_OK, "normal stopped", StopReason::NORMAL_STOPPED, IsUserPrivacyAuthorityNeeded());
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
        MEDIA_LOGW("StopScreenCaptureInner unsupport and ignore");
        return MSERR_OK;
    }
    CHECK_AND_RETURN_RET_LOG(captureState_ == AVScreenCaptureState::STARTED, ret, "state:%{public}d", captureState_);
    captureState_ = AVScreenCaptureState::STOPPED;
    SetErrorInfo(MSERR_OK, "normal stopped", StopReason::NORMAL_STOPPED, IsUserPrivacyAuthorityNeeded());
    PostStopScreenCapture(stateCode);
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "StopScreenCaptureInner end.", FAKE_POINTER(this));
    return ret;
}

void ScreenCaptureServer::PostStopScreenCapture(AVScreenCaptureStateCode stateCode)
{
    MediaTrace trace("ScreenCaptureServer::PostStopScreenCapture");
    MEDIA_LOGI("ScreenCaptureServer: 0x%{public}06" PRIXPTR "PostStopScreenCapture start, stateCode:%{public}d.",
        FAKE_POINTER(this), stateCode);
    if (screenCaptureCb_ != nullptr) {
        if (stateCode != AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_INVLID) {
            screenCaptureCb_->OnStateChange(stateCode);
        }
    }
#ifdef SUPPORT_SCREEN_CAPTURE_WINDOW_NOTIFICATION
    if (isPrivacyAuthorityEnabled_ && SCREEN_RECORDER_BUNDLE_NAME.compare(bundleName_) != 0) {
        // Remove real time notification
        int32_t ret = NotificationHelper::CancelNotification(notificationId_);
        MEDIA_LOGI("StopScreenCaptureInner CancelNotification id:%{public}d, ret:%{public}d ", notificationId_, ret);
        micCount_.store(0);
    }
#endif
    isPrivacyAuthorityEnabled_ = false;

    if (!UpdatePrivacyUsingPermissionState(STOP_VIDEO)) {
        MEDIA_LOGE("UpdatePrivacyUsingPermissionState STOP failed, dataType:%{public}d", captureConfig_.dataType);
    }
    std::unordered_map<std::string, std::string> payload;
    int64_t value = ResourceSchedule::ResType::ScreenCaptureStatus::STOP_SCREEN_CAPTURE;
    ResSchedReportData(value, payload);
    MEDIA_LOGI("StopScreenCaptureInner sessionId:%{public}d, activeSessionId_:%{public}d", sessionId_,
        activeSessionId_.load());
    if (sessionId_ == activeSessionId_.load()) {
        activeSessionId_.store(SESSION_ID_INVALID);
    }
}

int32_t ScreenCaptureServer::StopScreenCapture()
{
    MediaTrace trace("ScreenCaptureServer::StopScreenCapture");
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances StopScreenCapture S", FAKE_POINTER(this));

    std::lock_guard<std::mutex> lock(mutex_);
    int32_t ret = StopScreenCaptureInner(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_INVLID);
    if (statisticalEventInfo_.startLatency < 0) {
        statisticalEventInfo_.captureDuration = -1; // latency -1 means invalid
    } else {
        int64_t endTime = GetCurrentMillisecond();
        statisticalEventInfo_.captureDuration = static_cast<int32_t>(endTime - startTime_ -
            statisticalEventInfo_.startLatency);
    }
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances StopScreenCapture E", FAKE_POINTER(this));
    return ret;
}

void ScreenCaptureServer::Release()
{
    ReleaseInner();
}

void ScreenCaptureServer::ReleaseInner()
{
    MediaTrace trace("ScreenCaptureServer::ReleaseInner");
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances ReleaseInner S", FAKE_POINTER(this));
    int32_t sessionId;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        StopScreenCaptureInner(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_INVLID);
        sessionId = sessionId_;
        sessionId_ = SESSION_ID_INVALID;
        MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances ReleaseInner Stop done, sessionId:%{public}d",
            FAKE_POINTER(this), sessionId);
    }
    {
        std::lock_guard<std::mutex> lock(mutexGlobal_);
        serverMap.erase(sessionId);
    }
    SetMetaDataReport();
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances ReleaseInner E", FAKE_POINTER(this));
}

ScreenCaptureObserverCallBack::ScreenCaptureObserverCallBack(
    std::weak_ptr<ScreenCaptureServer> screenCaptureServer)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    screenCaptureServer_ = screenCaptureServer;
}

bool ScreenCaptureObserverCallBack::StopAndRelease()
{
    MEDIA_LOGI("ScreenCaptureObserverCallBack: StopAndRelease");
    auto scrServer = screenCaptureServer_.lock();
    if (scrServer) {
        scrServer->StopScreenCaptureByEvent(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STOPPED_BY_CALL);
        scrServer->Release();
    }
    return true;
}

void ScreenCapBufferConsumerListener::OnBufferAvailable()
{
    MediaTrace trace("ScreenCaptureServer::OnBufferAvailable");
    MEDIA_LOGD("ScreenCaptureServer: 0x%{public}06" PRIXPTR "OnBufferAvailable start.", FAKE_POINTER(this));
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
    MEDIA_LOGD("ScreenCaptureServer: 0x%{public}06" PRIXPTR "OnBufferAvailable end.", FAKE_POINTER(this));
    screenCaptureCb_->OnVideoBufferAvailable(true);
}

int32_t ScreenCapBufferConsumerListener::AcquireVideoBuffer(sptr<OHOS::SurfaceBuffer> &surfaceBuffer, int32_t &fence,
    int64_t &timestamp, OHOS::Rect &damage)
{
    MediaTrace trace("ScreenCaptureServer::AcquireVideoBuffer");
    using namespace std::chrono_literals;
    std::unique_lock<std::mutex> lock(bufferMutex_);
    MEDIA_LOGD("ScreenCaptureServer: 0x%{public}06" PRIXPTR "AcquireVideoBuffer start, fence:%{public}d, "
        "timestamp:%{public}" PRId64, FAKE_POINTER(this), fence, timestamp);
    if (!bufferCond_.wait_for(
        lock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] { return !availBuffers_.empty(); })) {
        return MSERR_UNKNOWN;
    }
    surfaceBuffer = availBuffers_.front()->buffer;
    fence = availBuffers_.front()->flushFence;
    timestamp = availBuffers_.front()->timeStamp;
    damage = availBuffers_.front()->damageRect;
    MEDIA_LOGD("ScreenCaptureServer: 0x%{public}06" PRIXPTR "AcquireVideoBuffer end.", FAKE_POINTER(this));
    return MSERR_OK;
}

int32_t ScreenCapBufferConsumerListener::ReleaseVideoBuffer()
{
    MediaTrace trace("ScreenCaptureServer::ReleaseVideoBuffer");
    std::unique_lock<std::mutex> lock(bufferMutex_);
    MEDIA_LOGD("ScreenCapBufferConsumerListener: 0x%{public}06" PRIXPTR "ReleaseVideoBuffer start.",
        FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(!availBuffers_.empty(), MSERR_OK, "buffer queue is empty, no video frame to release");

    if (consumer_ != nullptr) {
        consumer_->ReleaseBuffer(availBuffers_.front()->buffer, availBuffers_.front()->flushFence);
    }
    availBuffers_.pop();
    MEDIA_LOGD("ScreenCapBufferConsumerListener: 0x%{public}06" PRIXPTR "ReleaseVideoBuffer end.", FAKE_POINTER(this));
    return MSERR_OK;
}

int32_t ScreenCapBufferConsumerListener::Release()
{
    std::unique_lock<std::mutex> lock(bufferMutex_);
    MEDIA_LOGI("Release");
    return ReleaseBuffer();
}

int32_t AudioDataSource::ReadAt(std::shared_ptr<AVBuffer> buffer, uint32_t length)
{
    MEDIA_LOGD("AudioDataSource ReadAt start");
    std::shared_ptr<AudioBuffer> innerAudioBuffer = nullptr;
    std::shared_ptr<AudioBuffer> micAudioBuffer = nullptr;
    int32_t ret = MSERR_OK;
    ret = screenCaptureServer_->AcquireAudioBufferMix(innerAudioBuffer, micAudioBuffer, type_);
    if (ret == MSERR_OK) {
        MEDIA_LOGD("AcquireAudioBufferMix sucess");
        std::shared_ptr<AVMemory> &bufferMem = buffer->memory_;
        if (buffer->memory_ == nullptr) {
            MEDIA_LOGE("buffer->memory_ is nullptr");
            return MSERR_INVALID_VAL;
        }
        if (type_ == AVScreenCaptureMixMode::MIX_MODE) {
            char* mixData = new char[innerAudioBuffer->length];
            char* srcData[2] = {nullptr};
            srcData[0] = reinterpret_cast<char*>(innerAudioBuffer->buffer);
            srcData[1] = reinterpret_cast<char*>(micAudioBuffer->buffer);
            int channels = 2;
            MixAudio(srcData, mixData, channels, innerAudioBuffer->length);
            bufferMem->Write((uint8_t*)mixData, innerAudioBuffer->length, 0);
            return screenCaptureServer_->ReleaseAudioBufferMix(type_);
        } else if (type_ == AVScreenCaptureMixMode::INNER_MODE) {
            bufferMem->Write((uint8_t*)innerAudioBuffer->buffer, innerAudioBuffer->length, 0);
            return screenCaptureServer_->ReleaseAudioBufferMix(type_);
        } else if (type_ == AVScreenCaptureMixMode::MIC_MODE) {
            bufferMem->Write((uint8_t*)micAudioBuffer->buffer, innerAudioBuffer->length, 0);
            return screenCaptureServer_->ReleaseAudioBufferMix(type_);
        }
    } else {
        MEDIA_LOGE("AudioDataSource AcquireAudioBufferMix failed");
        return MSERR_INVALID_VAL;
    }
    return MSERR_UNKNOWN;
}

int32_t AudioDataSource::GetSize(int64_t &size)
{
    size_t bufferLen = 0;
    int32_t ret = screenCaptureServer_->GetInnerAudioCaptureBufferSize(bufferLen);
    MEDIA_LOGD("AudioDataSource::GetSize : %{public}zu", bufferLen);
    size = static_cast<int64_t>(bufferLen);
    return ret;
}

void AudioDataSource::MixAudio(char** srcData, char* mixData, int channels, int bufferSize)
{
    MEDIA_LOGD("AudioDataSource MixAudio");
    int const max = 32767;
    int const min = -32768;
    double const splitNum = 32;
    int const doubleChannels = 2;
    double coefficient = 1;
    int output;
    int totalNum = 0;
    int channelNum = 0;
    if (channels == 0) {
        return;
    }
    for (totalNum = 0; totalNum < bufferSize / channels; totalNum++) {
        int temp = 0;
        for (channelNum = 0; channelNum < channels; channelNum++) {
            temp += *reinterpret_cast<short*>(srcData[channelNum] + totalNum * channels);
        }
        output = static_cast<int32_t>(temp * coefficient);
        if (output > max) {
            coefficient = static_cast<double>(max) / static_cast<double>(output);
            output = max;
        }
        if (output < min) {
            coefficient = static_cast<double>(min) / static_cast<double>(output);
            output = min;
        }
        if (coefficient < 1) {
            coefficient += (static_cast<double>(1) - coefficient) / splitNum;
        }
        *reinterpret_cast<short*>(mixData + totalNum * doubleChannels) = static_cast<short>(output);
    }
}
} // namespace Media
} // namespace OHOS
