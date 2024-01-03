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

#include "player_engine_gst_impl.h"

#include <unistd.h>
#include "media_log.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "audio_system_manager.h"
#include "player_sinkprovider.h"
#include "av_common.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerEngineGstImpl"};
}

namespace OHOS {
namespace Media {
constexpr float EPSINON = 0.0001;
constexpr float SPEED_0_75_X = 0.75;
constexpr float SPEED_1_00_X = 1.00;
constexpr float SPEED_1_25_X = 1.25;
constexpr float SPEED_1_75_X = 1.75;
constexpr float SPEED_2_00_X = 2.00;
constexpr size_t MAX_URI_SIZE = 4096;
constexpr int32_t MSEC_PER_USEC = 1000;
constexpr int32_t MSEC_PER_NSEC = 1000000;
constexpr int32_t BUFFER_TIME_DEFAULT = 15000; // 15s
constexpr uint32_t INTERRUPT_EVENT_SHIFT = 8;
constexpr int32_t POSITION_REPORT_PER_TIMES = 1;

PlayerEngineGstImpl::PlayerEngineGstImpl(int32_t uid, int32_t pid, uint32_t tokenId)
    : appuid_(uid), apppid_(pid), apptokenid_(tokenId)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerEngineGstImpl::~PlayerEngineGstImpl()
{
    (void)OnReset();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool PlayerEngineGstImpl::IsFileUrl(const std::string &url) const
{
    return url.find("://") == std::string::npos || url.find("file://") == 0;
}

int32_t PlayerEngineGstImpl::GetRealPath(const std::string &url, std::string &realUrlPath) const
{
    std::string fileHead = "file://";
    std::string tempUrlPath;

    if (url.find(fileHead) == 0 && url.size() > fileHead.size()) {
        tempUrlPath = url.substr(fileHead.size());
    } else {
        tempUrlPath = url;
    }

    bool ret = PathToRealPath(tempUrlPath, realUrlPath);
    CHECK_AND_RETURN_RET_LOG(ret, MSERR_OPEN_FILE_FAILED,
        "invalid url. The Url (%{public}s) path may be invalid.", url.c_str());

    if (access(realUrlPath.c_str(), R_OK) != 0) {
        return MSERR_FILE_ACCESS_FAILED;
    }

    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::SetSource(const std::string &url)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(!url.empty(), MSERR_INVALID_VAL, "input url is empty!");
    CHECK_AND_RETURN_RET_LOG(url.length() <= MAX_URI_SIZE, MSERR_INVALID_VAL, "input url length is invalid!");

    int32_t ret = MSERR_OK;
    if (IsFileUrl(url)) {
        std::string realUriPath;
        ret = GetRealPath(url, realUriPath);
        if (ret != MSERR_OK) {
            return ret;
        }
        url_ = "file://" + realUriPath;
    } else {
        url_ = url;
    }

    MEDIA_LOGD("set player source: %{public}s", url_.c_str());
    return ret;
}

int32_t PlayerEngineGstImpl::SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(dataSrc != nullptr, MSERR_INVALID_VAL, "input dataSrc is empty!");
    appsrcWrap_ = GstAppsrcEngine::Create(dataSrc);
    CHECK_AND_RETURN_RET_LOG(appsrcWrap_ != nullptr, MSERR_NO_MEMORY, "new appsrcwrap failed!");
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::AddSubSource(const std::string &url)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_VAL, "playBinCtrler_ is nullptr");

    std::string subUrl = url;
    int32_t ret = MSERR_OK;

    if (IsFileUrl(url)) {
        std::string realUriPath;
        ret = GetRealPath(url, realUriPath);
        if (ret != MSERR_OK) {
            return ret;
        }
        subUrl = "file://" + realUriPath;
    }

    MEDIA_LOGD("add subtitle source: %{public}s", subUrl.c_str());
    return playBinCtrler_->AddSubSource(subUrl);
}

int32_t PlayerEngineGstImpl::SetObs(const std::weak_ptr<IPlayerEngineObs> &obs)
{
    std::unique_lock<std::mutex> lock(mutex_);
    obs_ = obs;
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::SetVideoSurface(sptr<Surface> surface)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, MSERR_INVALID_VAL, "surface is nullptr");

    producerSurface_ = surface;
    if (appsrcWrap_) {
        appsrcWrap_->SetVideoMode();
    }
    return MSERR_OK;
}

#ifdef SUPPORT_DRM
int32_t PlayerEngineGstImpl::SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySessionProxy,
    bool svp)
{
    MEDIA_LOGD("SetDecryptConfig ok in");
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(keySessionProxy != nullptr, MSERR_INVALID_VAL, "keySessionProxy is nullptr");
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_VAL, "playBinCtrler_ is nullptr");

    int32_t ret = playBinCtrler_->SetDecryptConfig(keySessionProxy, svp);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_VAL, "SetDecryptConfig failed");

    MEDIA_LOGD("SetDecryptConfig ok out");
    return MSERR_OK;
}
#endif

int32_t PlayerEngineGstImpl::PrepareAsync()
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("Prepare in");

    int32_t ret = PlayBinCtrlerInit();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_VAL, "PlayBinCtrlerInit failed");

    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_VAL, "playBinCtrler_ is nullptr");
    ret = playBinCtrler_->PrepareAsync();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "PrepareAsync failed");

    // The duration of some resources without header information cannot be obtained.
    MEDIA_LOGD("Prepared ok out");
    return MSERR_OK;
}

void PlayerEngineGstImpl::HandleErrorMessage(const PlayBinMessage &msg)
{
    MEDIA_LOGE("error happended, cancel inprocessing job");

    int32_t errorCode = msg.code;
    std::string errorMsg = "Unknown Error";
    if (msg.subType == PlayBinMsgErrorSubType::PLAYBIN_SUB_MSG_ERROR_WITH_MESSAGE) {
        errorMsg = std::any_cast<std::string>(msg.extra);
    }

    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    Format format;
    if (notifyObs != nullptr) {
        notifyObs->OnErrorMessage(errorCode, errorMsg);
        notifyObs->OnInfo(INFO_TYPE_STATE_CHANGE, PLAYER_STATE_ERROR, format);
    }
}

void PlayerEngineGstImpl::HandleInfoMessage(const PlayBinMessage &msg)
{
    MEDIA_LOGI("info msg type:%{public}d, value:%{public}d", msg.type, msg.code);

    int32_t status = msg.code;
    Format format;
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(static_cast<PlayerOnInfoType>(msg.type), status, format);
    }
}

void PlayerEngineGstImpl::HandleSeekDoneMessage(const PlayBinMessage &msg)
{
    MEDIA_LOGI("seek done, seek position = %{public}dms", msg.code);

    codecCtrl_.EnhanceSeekPerformance(false);

    int32_t status = msg.code;
    Format format;
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_SEEKDONE, status, format);
    }
}

void PlayerEngineGstImpl::HandleSpeedDoneMessage(const PlayBinMessage &msg)
{
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    codecCtrl_.EnhanceSeekPerformance(false);
    if (notifyObs != nullptr) {
        Format format;
        double rate = std::any_cast<double>(msg.extra);
        PlaybackRateMode speedMode = ChangeSpeedToMode(rate);
        notifyObs->OnInfo(INFO_TYPE_SPEEDDONE, speedMode, format);
    }
}

void PlayerEngineGstImpl::HandleBufferingStart(const PlayBinMessage &msg)
{
    (void)msg;
    Format format;
    (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_BUFFERING_START), 0);
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_BUFFERING_UPDATE, 0, format);
    }
}

void PlayerEngineGstImpl::HandleBufferingEnd(const PlayBinMessage &msg)
{
    (void)msg;
    Format format;
    (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_BUFFERING_END), 0);
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_BUFFERING_UPDATE, 0, format);
    }
    updatePercent_ = -1; // invalid percent
}

void PlayerEngineGstImpl::HandleBufferingTime(const PlayBinMessage &msg)
{
    if (!isAdaptiveLiveStream_) {
        std::pair<uint32_t, int64_t> bufferingTimePair = std::any_cast<std::pair<uint32_t, int64_t>>(msg.extra);
        uint32_t mqNumId = bufferingTimePair.first;
        uint64_t bufferingTime = bufferingTimePair.second / MSEC_PER_NSEC;

        if (bufferingTime > BUFFER_TIME_DEFAULT) {
            bufferingTime = BUFFER_TIME_DEFAULT;
        }

        mqBufferingTime_[mqNumId] = bufferingTime;
        MEDIA_LOGD("ProcessBufferingTime(%{public}" PRIu64 " ms), mqNumId = %{public}u, "
            "mqNum = %{public}u", bufferingTime, mqNumId, mqNum_);

        if (mqBufferingTime_.size() == mqNum_) {
            uint64_t mqBufferingTime = mqBufferingTime_[mqNumId];
            if (bufferingTime_ != mqBufferingTime) {
                bufferingTime_ = mqBufferingTime;
                std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
                CHECK_AND_RETURN_LOG(notifyObs != nullptr, "notifyObs is nullptr");
                Format format;
                (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_CACHED_DURATION),
                    static_cast<int32_t>(mqBufferingTime));
                notifyObs->OnInfo(INFO_TYPE_BUFFERING_UPDATE, 0, format);
            }
        }
    }
}

void PlayerEngineGstImpl::HandleBufferingPercent(const PlayBinMessage &msg)
{
    int32_t curPercent = msg.code;
    if (curPercent != updatePercent_) {
        updatePercent_ = curPercent;
        std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
        if (tempObs != nullptr) {
            Format format;
            (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_BUFFERING_PERCENT), updatePercent_);
            MEDIA_LOGD("updatePercent_ = %{public}d, 0x%{public}06" PRIXPTR "", updatePercent_, FAKE_POINTER(this));
            tempObs->OnInfo(INFO_TYPE_BUFFERING_UPDATE, 0, format);
        }
    }
}

void PlayerEngineGstImpl::HandleBufferingUsedMqNum(const PlayBinMessage &msg)
{
    mqNum_ = std::any_cast<uint32_t>(msg.extra);
}

void PlayerEngineGstImpl::HandleVideoRenderingStart(const PlayBinMessage &msg)
{
    (void)msg;
    Format format;
    MEDIA_LOGD("video rendering start");
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_MESSAGE, PlayerMessageType::PLAYER_INFO_VIDEO_RENDERING_START, format);
    }
}

void PlayerEngineGstImpl::HandleVideoSizeChanged(const PlayBinMessage &msg)
{
    std::pair<int32_t, int32_t> resolution = std::any_cast<std::pair<int32_t, int32_t>>(msg.extra);
    Format format;
    (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_WIDTH), resolution.first);
    (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_HEIGHT), resolution.second);
    MEDIA_LOGD("video size changed, width = %{public}d, height = %{public}d", resolution.first, resolution.second);
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_RESOLUTION_CHANGE, 0, format);
    }
    videoWidth_ = resolution.first;
    videoHeight_ = resolution.second;
}

void PlayerEngineGstImpl::HandleBitRateCollect(const PlayBinMessage &msg)
{
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_BITRATE_COLLECT, 0, std::any_cast<Format>(msg.extra));
    }
}

void PlayerEngineGstImpl::HandleIsLiveStream(const PlayBinMessage &msg)
{
    (void)msg;
    isAdaptiveLiveStream_ = true;
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    Format format;
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_IS_LIVE_STREAM, 0, format);
    }
}

void PlayerEngineGstImpl::HandleDrmInfoUpdated(const PlayBinMessage &msg)
{
    MEDIA_LOGI("HandleDrmInfoUpdated");
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_DRM_INFO_UPDATED, 0, std::any_cast<Format>(msg.extra));
    }
}

void PlayerEngineGstImpl::HandleSetDecryptConfigDone(const PlayBinMessage &msg)
{
    MEDIA_LOGI("HandleSetDecryptConfigDone");
    (void)msg;
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    Format format;
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_SET_DECRYPT_CONFIG_DONE, 0, format);
    }
}

void PlayerEngineGstImpl::HandleTrackChanged(const PlayBinMessage &msg)
{
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_TRACKCHANGE, 0, std::any_cast<Format>(msg.extra));
    }
}

void PlayerEngineGstImpl::HandleDefaultTrack(const PlayBinMessage &msg)
{
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_DEFAULTTRACK, 0, std::any_cast<Format>(msg.extra));
    }
}

void PlayerEngineGstImpl::HandleTrackDone(const PlayBinMessage &msg)
{
    (void)msg;
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        Format format;
        notifyObs->OnInfo(INFO_TYPE_TRACK_DONE, 0, format);
    }
}

void PlayerEngineGstImpl::HandleAddSubDone(const PlayBinMessage &msg)
{
    (void)msg;
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        Format format;
        notifyObs->OnInfo(INFO_TYPE_ADD_SUBTITLE_DONE, 0, format);
    }
}

void PlayerEngineGstImpl::HandleOnError(const PlayBinMessage &msg)
{
    Format format;
    (void)format.PutIntValue(std::string(PlayerKeys::PLAYER_ERROR_TYPE), msg.code);
    (void)format.PutStringValue(std::string(PlayerKeys::PLAYER_ERROR_MSG), std::any_cast<std::string>(msg.extra));

    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_ERROR_MSG, 0, format);
    }
}

void PlayerEngineGstImpl::HandleTrackNumUpdate(const PlayBinMessage &msg)
{
    int32_t textTrackNum = msg.code;

    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        Format format;
        MEDIA_LOGI("track num update, textTrackNum = %{public}d", textTrackNum);
        notifyObs->OnInfo(INFO_TYPE_TRACK_NUM_UPDATE, textTrackNum, format);
    }
}

void PlayerEngineGstImpl::HandleTrackInfoUpdate(const PlayBinMessage &msg)
{
    std::vector<Format> trackInfo = std::any_cast<std::vector<Format>>(msg.extra);
    Format format;
    format.PutFormatVector(std::string(PlayerKeys::PLAYER_TRACK_INFO), trackInfo);

    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_TRACK_INFO_UPDATE, 0, format);
    }
}

void PlayerEngineGstImpl::HandleSubtitleUpdate(const PlayBinMessage &msg)
{
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        notifyObs->OnInfo(INFO_TYPE_SUBTITLE_UPDATE, 0, std::any_cast<Format>(msg.extra));
    }
}

void PlayerEngineGstImpl::HandleSubTypeMessage(const PlayBinMessage &msg)
{
    if (subMsgHandler_.count(msg.subType) > 0) {
        (this->*subMsgHandler_[msg.subType])(msg);
    } else {
        MEDIA_LOGI("No this sub msg handler, subType = %{public}d", msg.subType);
    }
}

void PlayerEngineGstImpl::HandleAudioMessage(const PlayBinMessage &msg)
{
    if (msg.subType == PLAYBIN_MSG_INTERRUPT_EVENT) {
        HandleInterruptMessage(msg);
    } else if (msg.subType == PLAYBIN_MSG_FIRST_FRAME_EVENT) {
        HandleAudioFirstFrameMessage(msg);
    }
}

void PlayerEngineGstImpl::HandleInterruptMessage(const PlayBinMessage &msg)
{
    MEDIA_LOGI("Audio interrupt event in");
    uint32_t value = std::any_cast<uint32_t>(msg.extra);
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        Format format;
        int32_t hintType = value & 0x000000FF;
        int32_t forceType = (value >> INTERRUPT_EVENT_SHIFT) & 0x000000FF;
        int32_t eventType = value >> (INTERRUPT_EVENT_SHIFT * 2);
        (void)format.PutIntValue(PlayerKeys::AUDIO_INTERRUPT_TYPE, eventType);
        (void)format.PutIntValue(PlayerKeys::AUDIO_INTERRUPT_FORCE, forceType);
        (void)format.PutIntValue(PlayerKeys::AUDIO_INTERRUPT_HINT, hintType);
        notifyObs->OnInfo(INFO_TYPE_INTERRUPT_EVENT, 0, format);
    }
}

void PlayerEngineGstImpl::HandleAudioFirstFrameMessage(const PlayBinMessage &msg)
{
    MEDIA_LOGI("Audio first frame event in");
    uint64_t value = std::any_cast<uint64_t>(msg.extra);
    std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
    if (notifyObs != nullptr) {
        Format format;
        (void)format.PutLongValue(PlayerKeys::AUDIO_FIRST_FRAME, value);
        notifyObs->OnInfo(INFO_TYPE_AUDIO_FIRST_FRAME, 0, format);
    }
}

void PlayerEngineGstImpl::HandlePositionUpdateMessage(const PlayBinMessage &msg)
{
    if (!isAdaptiveLiveStream_) {
        currentTime_ = msg.code;
        int32_t duration = std::any_cast<int32_t>(msg.extra);
        MEDIA_LOGD("update position %{public}d ms, duration %{public}d ms", currentTime_, duration);

        if (duration != duration_) {
            duration_ = duration;
            Format format;
            std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
            if (notifyObs != nullptr) {
                notifyObs->OnInfo(INFO_TYPE_DURATION_UPDATE, duration_, format);
            }
        }

        // 10: report once at 1000ms
        if (currentTimeOnInfoCnt_ % POSITION_REPORT_PER_TIMES == 0 ||
            msg.subType == PLAYBIN_SUB_MSG_POSITION_UPDATE_FORCE) {
            Format format;
            std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
            if (notifyObs != nullptr) {
                notifyObs->OnInfo(INFO_TYPE_POSITION_UPDATE, currentTime_, format);
            }
            if (currentTimeOnInfoCnt_ % POSITION_REPORT_PER_TIMES == 0) {
                currentTimeOnInfoCnt_ = 0;
            }
        }
        if (msg.subType == PLAYBIN_SUB_MSG_POSITION_UPDATE_UNFORCE) {
            currentTimeOnInfoCnt_++;
        }
    }
}

using MsgNotifyFunc = std::function<void(const PlayBinMessage&)>;

void PlayerEngineGstImpl::OnNotifyMessage(const PlayBinMessage &msg)
{
    const std::unordered_map<int32_t, MsgNotifyFunc> MSG_NOTIFY_FUNC_TABLE = {
        { PLAYBIN_MSG_ERROR, std::bind(&PlayerEngineGstImpl::HandleErrorMessage, this, std::placeholders::_1) },
        { PLAYBIN_MSG_SEEKDONE, std::bind(&PlayerEngineGstImpl::HandleSeekDoneMessage, this, std::placeholders::_1) },
        { PLAYBIN_MSG_SPEEDDONE, std::bind(&PlayerEngineGstImpl::HandleSpeedDoneMessage, this, std::placeholders::_1) },
        { PLAYBIN_MSG_BITRATEDONE, std::bind(&PlayerEngineGstImpl::HandleInfoMessage, this, std::placeholders::_1)},
        { PLAYBIN_MSG_EOS, std::bind(&PlayerEngineGstImpl::HandleInfoMessage, this, std::placeholders::_1) },
        { PLAYBIN_MSG_STATE_CHANGE, std::bind(&PlayerEngineGstImpl::HandleInfoMessage, this, std::placeholders::_1) },
        { PLAYBIN_MSG_SUBTYPE, std::bind(&PlayerEngineGstImpl::HandleSubTypeMessage, this, std::placeholders::_1) },
        { PLAYBIN_MSG_AUDIO_SINK, std::bind(&PlayerEngineGstImpl::HandleAudioMessage, this, std::placeholders::_1) },
        { PLAYBIN_MSG_POSITION_UPDATE, std::bind(&PlayerEngineGstImpl::HandlePositionUpdateMessage, this,
            std::placeholders::_1) },
    };
    if (MSG_NOTIFY_FUNC_TABLE.count(msg.type) != 0) {
        MSG_NOTIFY_FUNC_TABLE.at(msg.type)(msg);
    }
}

int32_t PlayerEngineGstImpl::PlayBinCtrlerInit()
{
    if (playBinCtrler_) {
        return MSERR_OK;
    }

    MEDIA_LOGD("PlayBinCtrlerInit in");
    int ret = PlayBinCtrlerPrepare();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("PlayBinCtrlerPrepare failed");
        PlayBinCtrlerDeInit();
        return MSERR_INVALID_VAL;
    }

    subMsgHandler_[PLAYBIN_SUB_MSG_BUFFERING_START] = &PlayerEngineGstImpl::HandleBufferingStart;
    subMsgHandler_[PLAYBIN_SUB_MSG_BUFFERING_END] = &PlayerEngineGstImpl::HandleBufferingEnd;
    subMsgHandler_[PLAYBIN_SUB_MSG_BUFFERING_TIME] = &PlayerEngineGstImpl::HandleBufferingTime;
    subMsgHandler_[PLAYBIN_SUB_MSG_BUFFERING_PERCENT] = &PlayerEngineGstImpl::HandleBufferingPercent;
    subMsgHandler_[PLAYBIN_SUB_MSG_BUFFERING_USED_MQ_NUM] = &PlayerEngineGstImpl::HandleBufferingUsedMqNum;
    subMsgHandler_[PLAYBIN_SUB_MSG_VIDEO_RENDERING_START] = &PlayerEngineGstImpl::HandleVideoRenderingStart;
    subMsgHandler_[PLAYBIN_SUB_MSG_VIDEO_SIZE_CHANGED] = &PlayerEngineGstImpl::HandleVideoSizeChanged;
    subMsgHandler_[PLAYBIN_SUB_MSG_BITRATE_COLLECT] = &PlayerEngineGstImpl::HandleBitRateCollect;
    subMsgHandler_[PLAYBIN_SUB_MSG_IS_LIVE_STREAM] = &PlayerEngineGstImpl::HandleIsLiveStream;
    subMsgHandler_[PLAYBIN_SUB_MSG_AUDIO_CHANGED] = &PlayerEngineGstImpl::HandleTrackChanged;
    subMsgHandler_[PLAYBIN_SUB_MSG_SUBTITLE_CHANGED] = &PlayerEngineGstImpl::HandleTrackChanged;
    subMsgHandler_[PLAYBIN_SUB_MSG_DEFAULE_TRACK] = &PlayerEngineGstImpl::HandleDefaultTrack;
    subMsgHandler_[PLAYBIN_SUB_MSG_TRACK_DONE] = &PlayerEngineGstImpl::HandleTrackDone;
    subMsgHandler_[PLAYBIN_SUB_MSG_ADD_SUBTITLE_DONE] = &PlayerEngineGstImpl::HandleAddSubDone;
    subMsgHandler_[PLAYBIN_SUB_MSG_ONERROR] = &PlayerEngineGstImpl::HandleOnError;
    subMsgHandler_[PLAYBIN_SUB_MSG_TRACK_NUM_UPDATE] = &PlayerEngineGstImpl::HandleTrackNumUpdate;
    subMsgHandler_[PLAYBIN_SUB_MSG_TRACK_INFO_UPDATE] = &PlayerEngineGstImpl::HandleTrackInfoUpdate;
    subMsgHandler_[PLAYBIN_SUB_MSG_SUBTITLE_UPDATED] = &PlayerEngineGstImpl::HandleSubtitleUpdate;
    subMsgHandler_[PLAYBIN_SUB_MSG_DRM_INFO_UPDATED] = &PlayerEngineGstImpl::HandleDrmInfoUpdated;
    subMsgHandler_[PLAYBIN_SUB_MSG_SET_DRM_CONFIG_DONE] = &PlayerEngineGstImpl::HandleSetDecryptConfigDone;

    MEDIA_LOGD("PlayBinCtrlerInit out");
    return MSERR_OK;
}

void PlayerEngineGstImpl::PlayBinCtrlerDeInit()
{
    url_.clear();
    useSoftDec_ = false;
    appsrcWrap_ = nullptr;

    if (playBinCtrler_ != nullptr) {
        playBinCtrler_->SetElemSetupListener(nullptr);
        playBinCtrler_->SetElemUnSetupListener(nullptr);
        playBinCtrler_->SetAutoPlugSortListener(nullptr);
        playBinCtrler_ = nullptr;
    }

    {
        std::unique_lock<std::mutex> lk(sinkProviderMutex_);
        sinkProvider_ = nullptr;
    }
}

int32_t PlayerEngineGstImpl::PlayBinCtrlerPrepare()
{
    uint8_t renderMode = IPlayBinCtrler::PlayBinRenderMode::DEFAULT_RENDER;
    auto notifier = std::bind(&PlayerEngineGstImpl::OnNotifyMessage, this, std::placeholders::_1);

    {
        std::unique_lock<std::mutex> lk(sinkProviderMutex_);
        sinkProvider_ = std::make_shared<PlayerSinkProvider>(producerSurface_);
        sinkProvider_->SetAppInfo(appuid_, apppid_, apptokenid_);
    }

    IPlayBinCtrler::PlayBinCreateParam createParam = {
        static_cast<IPlayBinCtrler::PlayBinRenderMode>(renderMode), notifier, sinkProvider_
    };
    playBinCtrler_ = IPlayBinCtrler::Create(IPlayBinCtrler::PlayBinKind::PLAYBIN2, createParam);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_VAL, "playBinCtrler_ is nullptr");

    int32_t ret;
    if (appsrcWrap_ == nullptr) {
        ret = playBinCtrler_->SetSource(url_);
    } else {
        ret = playBinCtrler_->SetSource(appsrcWrap_);
    }
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_VAL, "SetSource failed");

    ret = SetVideoScaleType(videoScaleType_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_VAL, "SetVideoScaleType failed");

    ret = SetAudioRendererInfo(contentType_, streamUsage_, rendererFlag_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_VAL, "SetAudioRendererInfo failed");

    auto setupListener = std::bind(&PlayerEngineGstImpl::OnNotifyElemSetup, this, std::placeholders::_1);
    playBinCtrler_->SetElemSetupListener(setupListener);

    auto unSetupListener = std::bind(&PlayerEngineGstImpl::OnNotifyElemUnSetup, this, std::placeholders::_1);
    playBinCtrler_->SetElemUnSetupListener(unSetupListener);

    auto autoPlugSortListener = std::bind(&PlayerEngineGstImpl::OnNotifyAutoPlugSort, this, std::placeholders::_1);
    playBinCtrler_->SetAutoPlugSortListener(autoPlugSortListener);

    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::Play()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_OPERATION, "playBinCtrler_ is nullptr");

    MEDIA_LOGI("Play in");
    playBinCtrler_->Play();
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::Pause()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_OPERATION, "playBinCtrler_ is nullptr");

    MEDIA_LOGI("Pause in");
    (void)playBinCtrler_->Pause();
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::GetCurrentTime(int32_t &currentTime)
{
    currentTime = currentTime_;
    MEDIA_LOGD("Time in milliseconds: %{public}d", currentTime);
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_OPERATION, "playBinCtrler_ is nullptr");
    return playBinCtrler_->GetVideoTrackInfo(videoTrack);
}

int32_t PlayerEngineGstImpl::GetAudioTrackInfo(std::vector<Format> &audioTrack)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_OPERATION, "playBinCtrler_ is nullptr");
    return playBinCtrler_->GetAudioTrackInfo(audioTrack);
}

int32_t PlayerEngineGstImpl::GetSubtitleTrackInfo(std::vector<Format> &subtitleTrack)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_OPERATION, "playBinCtrler_ is nullptr");
    return playBinCtrler_->GetSubtitleTrackInfo(subtitleTrack);
}

int32_t PlayerEngineGstImpl::GetVideoWidth()
{
    return videoWidth_;
}

int32_t PlayerEngineGstImpl::GetVideoHeight()
{
    return videoHeight_;
}

int32_t PlayerEngineGstImpl::GetDuration(int32_t &duration)
{
    duration = duration_;
    MEDIA_LOGD("Duration in milliseconds: %{public}d", duration);
    return MSERR_OK;
}

double PlayerEngineGstImpl::ChangeModeToSpeed(const PlaybackRateMode &mode) const
{
    switch (mode) {
        case SPEED_FORWARD_0_75_X:
            return SPEED_0_75_X;
        case SPEED_FORWARD_1_00_X:
            return SPEED_1_00_X;
        case SPEED_FORWARD_1_25_X:
            return SPEED_1_25_X;
        case SPEED_FORWARD_1_75_X:
            return SPEED_1_75_X;
        case SPEED_FORWARD_2_00_X:
            return SPEED_2_00_X;
        default:
            MEDIA_LOGW("unknown mode:%{public}d, return default speed(SPEED_1_00_X)", mode);
    }

    return SPEED_1_00_X;
}

PlaybackRateMode PlayerEngineGstImpl::ChangeSpeedToMode(double rate) const
{
    if (abs(rate - SPEED_0_75_X) < EPSINON) {
        return SPEED_FORWARD_0_75_X;
    }
    if (abs(rate - SPEED_1_00_X) < EPSINON) {
        return SPEED_FORWARD_1_00_X;
    }
    if (abs(rate - SPEED_1_25_X) < EPSINON) {
        return SPEED_FORWARD_1_25_X;
    }
    if (abs(rate - SPEED_1_75_X) < EPSINON) {
        return SPEED_FORWARD_1_75_X;
    }
    if (abs(rate - SPEED_2_00_X) < EPSINON) {
        return SPEED_FORWARD_2_00_X;
    }

    MEDIA_LOGW("unknown rate:%{public}lf, return default speed(SPEED_FORWARD_1_00_X)", rate);

    return  SPEED_FORWARD_1_00_X;
}

int32_t PlayerEngineGstImpl::SetPlaybackSpeed(PlaybackRateMode mode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (playBinCtrler_ != nullptr) {
        codecCtrl_.EnhanceSeekPerformance(true);
        double rate = ChangeModeToSpeed(mode);
        return playBinCtrler_->SetRate(rate);
    }
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::GetPlaybackSpeed(PlaybackRateMode &mode)
{
    // invliad api
    (void)mode;
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::SetLooping(bool loop)
{
    if (playBinCtrler_ != nullptr) {
        MEDIA_LOGD("SetLooping in");
        return playBinCtrler_->SetLoop(loop);
    }
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::SetParameter(const Format &param)
{
    if (param.ContainKey(PlayerKeys::VIDEO_SCALE_TYPE)) {
        int32_t videoScaleType = 0;
        param.GetIntValue(PlayerKeys::VIDEO_SCALE_TYPE, videoScaleType);
        return SetVideoScaleType(Plugins::VideoScaleType(videoScaleType));
    }
    if (param.ContainKey(PlayerKeys::CONTENT_TYPE) && param.ContainKey(PlayerKeys::STREAM_USAGE)) {
        param.GetIntValue(PlayerKeys::CONTENT_TYPE, contentType_);
        param.GetIntValue(PlayerKeys::STREAM_USAGE, streamUsage_);
        param.GetIntValue(PlayerKeys::RENDERER_FLAG, rendererFlag_);
        return SetAudioRendererInfo(contentType_, streamUsage_, rendererFlag_);
    }
    if (param.ContainKey(PlayerKeys::AUDIO_INTERRUPT_MODE)) {
        int32_t interruptMode = 0;
        param.GetIntValue(PlayerKeys::AUDIO_INTERRUPT_MODE, interruptMode);
        return SetAudioInterruptMode(interruptMode);
    }
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::Stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_OPERATION, "playBinCtrler_ is nullptr");

    MEDIA_LOGD("Stop in");
    codecCtrl_.StopFormatChange();
    playBinCtrler_->Stop(true);
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::Reset()
{
    MEDIA_LOGD("Reset in");
    OnReset();
    return MSERR_OK;
}

void PlayerEngineGstImpl::OnReset()
{
    if (taskQueue_ != nullptr) {
        (void)taskQueue_->Stop();
    }

    std::unique_lock<std::mutex> lock(mutex_);
    PlayBinCtrlerDeInit();
}

int32_t PlayerEngineGstImpl::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_OPERATION, "playBinCtrler_ is nullptr");
    MEDIA_LOGI("Seek in %{public}dms", mSeconds);

    if (playBinCtrler_->QueryPosition() == mSeconds) {
        MEDIA_LOGW("current time same to seek time, invalid seek");
        Format format;
        std::shared_ptr<IPlayerEngineObs> notifyObs = obs_.lock();
        if (notifyObs != nullptr) {
            notifyObs->OnInfo(INFO_TYPE_SEEKDONE, mSeconds, format);
        }
        return MSERR_OK;
    }

    codecCtrl_.EnhanceSeekPerformance(true);

    int64_t position = static_cast<int64_t>(mSeconds) * MSEC_PER_USEC;
    return playBinCtrler_->Seek(position, mode);
}

int32_t PlayerEngineGstImpl::SetVolume(float leftVolume, float rightVolume)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (playBinCtrler_ != nullptr) {
        MEDIA_LOGD("SetVolume in");
        playBinCtrler_->SetVolume(leftVolume, rightVolume);
    }
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::SelectBitRate(uint32_t bitRate)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (playBinCtrler_ != nullptr) {
        MEDIA_LOGD("SelectBitRate in");
        return playBinCtrler_->SelectBitRate(bitRate);
    }
    return MSERR_INVALID_OPERATION;
}

int32_t PlayerEngineGstImpl::SetVideoScaleType(Plugins::VideoScaleType videoScaleType)
{
    std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
    if (sinkProvider_ != nullptr) {
        MEDIA_LOGD("SetVideoScaleType in");
        sinkProvider_->SetVideoScaleType(static_cast<uint32_t>(videoScaleType));
    }
    videoScaleType_ = videoScaleType;
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::SetAudioRendererInfo(const int32_t contentType,
    const int32_t streamUsage, const int32_t rendererFlag)
{
    std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
    contentType_ = contentType;
    streamUsage_ = streamUsage;
    rendererFlag_ = rendererFlag;
    MEDIA_LOGI("content = %{public}d, usage = %{public}d, rendererFlags = %{public}d",
        contentType, streamUsage, rendererFlag);
    if (playBinCtrler_ != nullptr) {
        MEDIA_LOGI("SetAudioRendererInfo in");
        uint32_t rendererInfo(0);
        rendererInfo |= (contentType | (static_cast<uint32_t>(streamUsage) <<
            AudioStandard::RENDERER_STREAM_USAGE_SHIFT));
        playBinCtrler_->SetAudioRendererInfo(rendererInfo, rendererFlag);
    }
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::SetAudioInterruptMode(const int32_t interruptMode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (playBinCtrler_ != nullptr) {
        MEDIA_LOGD("SetAudioInterruptMode in");
        playBinCtrler_->SetAudioInterruptMode(interruptMode);
    }
    return MSERR_OK;
}

int32_t PlayerEngineGstImpl::SetAudioEffectMode(const int32_t effectMode)
{
    MEDIA_LOGD("SetAudioEffectMode in");
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_OPERATION, "playBinCtrler_ is nullptr");
    return playBinCtrler_->SetAudioEffectMode(effectMode);
}

int32_t PlayerEngineGstImpl::GetHEBCMode()
{
    return codecCtrl_.GetHEBCMode();
}

int32_t PlayerEngineGstImpl::SelectTrack(int32_t index)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_OPERATION, "playBinCtrler_ is nullptr");
    return playBinCtrler_->SelectTrack(index);
}

int32_t PlayerEngineGstImpl::DeselectTrack(int32_t index)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_OPERATION, "playBinCtrler_ is nullptr");
    return playBinCtrler_->DeselectTrack(index);
}

int32_t PlayerEngineGstImpl::GetCurrentTrack(int32_t trackType, int32_t &index)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_OPERATION, "playBinCtrler_ is nullptr");
    return playBinCtrler_->GetCurrentTrack(trackType, index);
}

GValueArray *PlayerEngineGstImpl::OnNotifyAutoPlugSort(GValueArray &factories)
{
    if (isPlaySinkFlagsSet_) {
        return nullptr;
    }

    if (useSoftDec_) {
        GValueArray *result = g_value_array_new(factories.n_values);

        for (uint32_t i = 0; i < factories.n_values; i++) {
            GstElementFactory *factory =
                static_cast<GstElementFactory *>(g_value_get_object(g_value_array_get_nth(&factories, i)));
            if (strstr(gst_element_factory_get_metadata(factory, GST_ELEMENT_METADATA_KLASS),
                "Codec/Decoder/Video/Hardware")) {
                MEDIA_LOGD("set remove hardware codec plugins from pipeline");
                continue;
            }
            GValue val = G_VALUE_INIT;
            g_value_init(&val, G_TYPE_OBJECT);
            g_value_set_object(&val, factory);
            result = g_value_array_append(result, &val);
            g_value_unset(&val);
        }
        return result;
    } else {
        for (uint32_t i = 0; i < factories.n_values; i++) {
            GstElementFactory *factory =
                static_cast<GstElementFactory *>(g_value_get_object(g_value_array_get_nth(&factories, i)));
            if (strstr(gst_element_factory_get_metadata(factory, GST_ELEMENT_METADATA_KLASS),
                "Codec/Decoder/Video/Hardware")) {
                MEDIA_LOGD("set remove GstPlaySinkVideoConvert plugins from pipeline");
                playBinCtrler_->RemoveGstPlaySinkVideoConvertPlugin();
                isPlaySinkFlagsSet_ = true;
                break;
            }
        }
    }

    return nullptr;
}

void PlayerEngineGstImpl::OnNotifyElemSetup(GstElement &elem)
{
    std::unique_lock<std::mutex> lock(sinkProviderMutex_);

    const gchar *metadata = gst_element_get_metadata(&elem, GST_ELEMENT_METADATA_KLASS);
    CHECK_AND_RETURN_LOG(metadata != nullptr, "gst_element_get_metadata return nullptr");

    MEDIA_LOGD("get element_name %{public}s, get metadata %{public}s", GST_ELEMENT_NAME(&elem), metadata);
    std::string metaStr(metadata);

    if (metaStr.find("Codec/Decoder/Video") != std::string::npos || metaStr.find("Sink/Video") != std::string::npos) {
        if (producerSurface_ != nullptr) {
            CHECK_AND_RETURN_LOG(sinkProvider_ != nullptr, "sinkProvider_ is nullptr");
            GstElement *videoSink = sinkProvider_->GetVideoSink();
            CHECK_AND_RETURN_LOG(videoSink != nullptr, "videoSink is nullptr");
            auto notifier = std::bind(&PlayerEngineGstImpl::OnCapsFixError, this);
            codecCtrl_.DetectCodecSetup(metaStr, &elem, videoSink, notifier);
        }
    }
}

void PlayerEngineGstImpl::OnNotifyElemUnSetup(GstElement &elem)
{
    std::unique_lock<std::mutex> lock(sinkProviderMutex_);

    const gchar *metadata = gst_element_get_metadata(&elem, GST_ELEMENT_METADATA_KLASS);
    CHECK_AND_RETURN_LOG(metadata != nullptr, "gst_element_get_metadata return nullptr");

    MEDIA_LOGD("get element_name %{public}s, get metadata %{public}s", GST_ELEMENT_NAME(&elem), metadata);
    std::string metaStr(metadata);

    if (metaStr.find("Codec/Decoder/Video") != std::string::npos) {
        if (producerSurface_ != nullptr) {
            CHECK_AND_RETURN_LOG(sinkProvider_ != nullptr, "sinkProvider_ is nullptr");
            GstElement *videoSink = sinkProvider_->GetVideoSink();
            CHECK_AND_RETURN_LOG(videoSink != nullptr, "videoSink is nullptr");
            codecCtrl_.DetectCodecUnSetup(&elem, videoSink);
        }
    }
}

void PlayerEngineGstImpl::OnCapsFixError()
{
    MEDIA_LOGD("OnCapsFixError in");

    if (taskQueue_ == nullptr) {
        taskQueue_ = std::make_unique<TaskQueue>("reset-playbin-task");
        int32_t ret = taskQueue_->Start();
        CHECK_AND_RETURN_LOG(ret == MSERR_OK, "task queue start failed");
    }

    useSoftDec_ = true;
    auto resetTask = std::make_shared<TaskHandler<void>>([this]() {
        ResetPlaybinToSoftDec();
    });
    (void)taskQueue_->EnqueueTask(resetTask);
}

// fix video with small resolution cannot play in hardware decoding
// reset pipeline to use software decoder
void PlayerEngineGstImpl::ResetPlaybinToSoftDec()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(playBinCtrler_ != nullptr, "playBinCtrler_ is nullptr");

    MEDIA_LOGD("ResetPlaybinToSoftDec in");
    isPlaySinkFlagsSet_ = false;

    if (playBinCtrler_ != nullptr) {
        playBinCtrler_->SetNotifier(nullptr);
        if (appsrcWrap_) {
            appsrcWrap_->DecoderSwitch();
        }
        playBinCtrler_->Stop(false);
        playBinCtrler_->SetElemSetupListener(nullptr);
        playBinCtrler_->SetElemUnSetupListener(nullptr);
        playBinCtrler_->SetAutoPlugSortListener(nullptr);
        playBinCtrler_ = nullptr;
    }

    {
        std::unique_lock<std::mutex> lk(sinkProviderMutex_);
        sinkProvider_ = nullptr;
    }

    lock.unlock();
    PrepareAsync();
}

int32_t PlayerEngineGstImpl::HandleCodecBuffers(bool enable)
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("HandleCodecBuffers in, enable:%{public}d", enable);
    return codecCtrl_.HandleCodecBuffers(enable);
}

int32_t PlayerEngineGstImpl::SeekToCurrentTime(int32_t mSeconds, PlayerSeekMode mode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_OPERATION, "playBinCtrler_ is nullptr");
    MEDIA_LOGI("SeekToCurrentTime in %{public}dms", mSeconds);

    codecCtrl_.EnhanceSeekPerformance(true);

    int64_t position = static_cast<int64_t>(mSeconds) * MSEC_PER_USEC;
    return playBinCtrler_->Seek(position, mode);
}
} // namespace Media
} // namespace OHOS
