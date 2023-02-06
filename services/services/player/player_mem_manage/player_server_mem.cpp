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

#include "player_server_mem.h"
#include <unistd.h>
#include "media_log.h"
#include "media_errors.h"
#include "player_mem_manage.h"
#include "mem_mgr_client.h"
#include "media_dfx.h"
#include "param_wrapper.h"
#include "ipc_skeleton.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerServerMem"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<IPlayerService> PlayerServerMem::Create()
{
    MEDIA_LOGI("Create new PlayerServerMem");
    std::shared_ptr<PlayerServerMem> playerServerMem = std::make_shared<PlayerServerMem>();
    CHECK_AND_RETURN_RET_LOG(playerServerMem != nullptr, nullptr, "failed to new PlayerServerMem");

    (void)playerServerMem->Init();
    return playerServerMem;
}

PlayerServerMem::PlayerServerMem()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerServerMem::~PlayerServerMem()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t PlayerServerMem::SetSourceInternal()
{
    int32_t ret;
    MEDIA_LOGI("PlayerServerMem SetSourceInternal");
    switch (recoverConfig_.sourceType) {
        case static_cast<int32_t>(PlayerSourceType::SOURCE_TYPE_URL):
            ret = PlayerServer::SetSource(recoverConfig_.url);
            break;

        case static_cast<int32_t>(PlayerSourceType::SOURCE_TYPE_DATASRC):
            ret = PlayerServer::SetSource(recoverConfig_.dataSrc);
            break;

        case static_cast<int32_t>(PlayerSourceType::SOURCE_TYPE_FD):
            ret = PlayerServer::SetSource(recoverConfig_.fd, recoverConfig_.offset, recoverConfig_.size);
            break;
        
        default:
            ret = MSERR_INVALID_OPERATION;
            break;
    }
    return ret;
}

void PlayerServerMem::SetPlayerServerConfig()
{
    errorCbOnce_ = playerServerConfig_.errorCbOnce;
    disableStoppedCb_ = playerServerConfig_.disableStoppedCb;
    lastErrMsg_ = playerServerConfig_.lastErrMsg;
    uriHelper_ = std::move(playerServerConfig_.uriHelper);
}

void PlayerServerMem::GetPlayerServerConfig()
{
    playerServerConfig_.errorCbOnce = errorCbOnce_;
    playerServerConfig_.disableStoppedCb = disableStoppedCb_;
    playerServerConfig_.lastErrMsg = lastErrMsg_;
    playerServerConfig_.uriHelper = std::move(uriHelper_);
}

int32_t PlayerServerMem::SetConfigInternal()
{
    int32_t ret = MSERR_OK;
    MEDIA_LOGI("PlayerServerMem SetConfigInternal");
    if (recoverConfig_.leftVolume != 1.0f || recoverConfig_.rightVolume != 1.0f) {
        ret = PlayerServer::SetVolume(recoverConfig_.leftVolume, recoverConfig_.rightVolume);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SetVolume");
    }

    if (recoverConfig_.surface != nullptr) {
        ret = PlayerServer::SetVideoSurface(recoverConfig_.surface);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SetVideoSurface");
    }

    if (recoverConfig_.loop != false) {
        ret = PlayerServer::SetLooping(recoverConfig_.loop);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SetLooping");
    }

    ret = SetSaveParameter();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SetParameter");

    if (recoverConfig_.bitRate != 0) {
        ret = PlayerServer::SelectBitRate(recoverConfig_.bitRate);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SelectBitRate");
    }

    return ret;
}

int32_t PlayerServerMem::SetBehaviorInternal()
{
    int ret = MSERR_OK;
    MEDIA_LOGI("PlayerServerMem SetBehaviorInternal");
    if (recoverConfig_.speedMode != SPEED_FORWARD_1_00_X) {
        ret = PlayerServer::SetPlaybackSpeed(recoverConfig_.speedMode);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SetPlaybackSpeed");
    }

    if (recoverConfig_.currentTime != 0) {
        ret = PlayerServer::Seek(recoverConfig_.currentTime, SEEK_CLOSEST);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to Seek");
    }

    return ret;
}

int32_t PlayerServerMem::SetPlaybackSpeedInternal()
{
    if (recoverConfig_.speedMode != SPEED_FORWARD_1_00_X) {
        auto ret = PlayerServer::SetPlaybackSpeed(recoverConfig_.speedMode);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to SetPlaybackSpeed");
    }
    return MSERR_OK;
}

int32_t PlayerServerMem::SetSource(const std::string &url)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (CheckReleaseStateAndRecover() != MSERR_OK) {
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGI("SetSource enter url:%{public}s", url.c_str());
    
    auto ret = PlayerServer::SetSource(url);
    if (ret == MSERR_OK) {
        recoverConfig_.url = url;
        recoverConfig_.sourceType = static_cast<int32_t>(PlayerSourceType::SOURCE_TYPE_URL);
    }
    return ret;
}

int32_t PlayerServerMem::SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (CheckReleaseStateAndRecover() != MSERR_OK) {
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGI("SetSource enter datasrc");

    auto ret = PlayerServer::SetSource(dataSrc);
    if (ret == MSERR_OK) {
        recoverConfig_.dataSrc = dataSrc;
        recoverConfig_.sourceType = static_cast<int32_t>(PlayerSourceType::SOURCE_TYPE_DATASRC);
    }
    return ret;
}

int32_t PlayerServerMem::SetSource(int32_t fd, int64_t offset, int64_t size)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (CheckReleaseStateAndRecover() != MSERR_OK) {
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGI("SetSource enter fd:%{public}d, offset:%{public}ld, size:%{public}ld", fd, offset, size);

    auto ret = PlayerServer::SetSource(fd, offset, size);
    if (ret == MSERR_OK) {
        recoverConfig_.fd = fd;
        recoverConfig_.offset = offset;
        recoverConfig_.size = size;
        recoverConfig_.sourceType = static_cast<int32_t>(PlayerSourceType::SOURCE_TYPE_FD);
    }
    return ret;
}

int32_t PlayerServerMem::Play()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (CheckReleaseStateAndRecover() != MSERR_OK) {
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGI("Play enter");
    return PlayerServer::Play();
}

int32_t PlayerServerMem::Prepare()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (CheckReleaseStateAndRecover() != MSERR_OK) {
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGI("Prepare enter");
    return PlayerServer::Prepare();
}

int32_t PlayerServerMem::PrepareAsync()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (CheckReleaseStateAndRecover() != MSERR_OK) {
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGI("PrepareAsync enter");
    return PlayerServer::PrepareAsync();
}

int32_t PlayerServerMem::Pause()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (CheckReleaseStateAndRecover() != MSERR_OK) {
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGI("Pause enter");
    return PlayerServer::Pause();
}

int32_t PlayerServerMem::Stop()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (CheckReleaseStateAndRecover() != MSERR_OK) {
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGI("Stop enter");
    return PlayerServer::Stop();
}

int32_t PlayerServerMem::Reset()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (isReleaseMemByManage_) {
        if (recoverConfig_.callback != nullptr) {
            playerCb_ = recoverConfig_.callback;
            Format format;
            playerCb_->OnInfo(INFO_TYPE_STATE_CHANGE, PLAYER_IDLE, format);
            return MSERR_OK;
        }
        return MSERR_OK;
    }
    MEDIA_LOGI("Reset enter");
    return PlayerServer::Reset();
}

int32_t PlayerServerMem::Release()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (isReleaseMemByManage_) {
        MEDIA_LOGI("User call Release");
        isReleaseMemByManage_ = false;
    }
    MEDIA_LOGI("Release enter");
    return PlayerServer::Release();
}

int32_t PlayerServerMem::SetVolume(float leftVolume, float rightVolume)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (CheckReleaseStateAndRecover() != MSERR_OK) {
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGI("SetVolume enter leftVolume:%{public}f rightVolume:%{public}f", leftVolume, rightVolume);
    auto ret = PlayerServer::SetVolume(leftVolume, rightVolume);
    if (ret == MSERR_OK) {
        recoverConfig_.leftVolume = leftVolume;
        recoverConfig_.rightVolume = rightVolume;
    }
    return ret;
}

int32_t PlayerServerMem::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (CheckReleaseStateAndRecover() != MSERR_OK) {
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGI("Seek enter mSeconds:%{public}d mode:%{public}d", mSeconds, mode);
    return PlayerServer::Seek(mSeconds, mode);
}

int32_t PlayerServerMem::GetCurrentTime(int32_t &currentTime)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (isReleaseMemByManage_) {
        MEDIA_LOGI("User call GetCurrentTime:%{public}d", recoverConfig_.currentTime);
        currentTime = recoverConfig_.currentTime;
        return MSERR_OK;
    }
    MEDIA_LOGI("GetCurrentTime enter");
    return PlayerServer::GetCurrentTime(currentTime);
}

int32_t PlayerServerMem::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (isReleaseMemByManage_) {
        MEDIA_LOGI("User call GetVideoTrackInfo");
        videoTrack = recoverConfig_.videoTrack;
        return MSERR_OK;
    }

    MEDIA_LOGI("GetVideoTrackInfo enter");
    return PlayerServer::GetVideoTrackInfo(videoTrack);
}

int32_t PlayerServerMem::GetAudioTrackInfo(std::vector<Format> &audioTrack)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (isReleaseMemByManage_) {
        MEDIA_LOGI("User call GetAudioTrackInfo");
        audioTrack = recoverConfig_.audioTrack;
        return MSERR_OK;
    }

    MEDIA_LOGI("GetAudioTrackInfo enter");
    return PlayerServer::GetAudioTrackInfo(audioTrack);
}

int32_t PlayerServerMem::GetVideoWidth()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (isReleaseMemByManage_) {
        MEDIA_LOGI("User call GetVideoWidth:%{public}d", recoverConfig_.videoWidth);
        return recoverConfig_.videoWidth;
    }

    MEDIA_LOGI("GetVideoWidth enter");
    return PlayerServer::GetVideoWidth();
}

int32_t PlayerServerMem::GetVideoHeight()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (isReleaseMemByManage_) {
        MEDIA_LOGI("User call GetVideoHeight:%{public}d", recoverConfig_.videoHeight);
        return recoverConfig_.videoHeight;
    }
    MEDIA_LOGI("GetVideoHeight enter");
    return PlayerServer::GetVideoHeight();
}

int32_t PlayerServerMem::GetDuration(int32_t &duration)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (isReleaseMemByManage_) {
        MEDIA_LOGI("User call GetDuration:%{public}d", recoverConfig_.duration);
        duration = recoverConfig_.duration;
        return MSERR_OK;
    }
    MEDIA_LOGI("GetDuration enter");
    return PlayerServer::GetDuration(duration);
}

int32_t PlayerServerMem::SetPlaybackSpeed(PlaybackRateMode mode)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (CheckReleaseStateAndRecover() != MSERR_OK) {
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGI("SetPlaybackSpeed enter mode:%{public}d", mode);
    auto ret = PlayerServer::SetPlaybackSpeed(mode);
    if (ret == MSERR_OK) {
        recoverConfig_.speedMode = mode;
    }
    return ret;
}

int32_t PlayerServerMem::GetPlaybackSpeed(PlaybackRateMode &mode)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (isReleaseMemByManage_) {
        MEDIA_LOGI("User call GetPlaybackSpeed:%{public}d", recoverConfig_.speedMode);
        mode = recoverConfig_.speedMode;
        return MSERR_OK;
    }
    MEDIA_LOGI("GetPlaybackSpeed enter");
    return PlayerServer::GetPlaybackSpeed(mode);
}

#ifdef SUPPORT_VIDEO
int32_t PlayerServerMem::SetVideoSurface(sptr<Surface> surface)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    isAudioPlayer_ = false;
    if (CheckReleaseStateAndRecover() != MSERR_OK) {
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGI("SetVidoSurface enter");
    auto ret = PlayerServer::SetVideoSurface(surface);
    if (ret == MSERR_OK) {
        recoverConfig_.surface = surface;
    }
    return ret;
}
#endif

bool PlayerServerMem::IsPlaying()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (isReleaseMemByManage_) {
        MEDIA_LOGI("User call IsPlaying:%{public}d", recoverConfig_.isPlaying);
        return recoverConfig_.isPlaying;
    }
    MEDIA_LOGI("IsPlaying enter");
    return PlayerServer::IsPlaying();
}

bool PlayerServerMem::IsLooping()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (isReleaseMemByManage_) {
        MEDIA_LOGI("User call IsLooping:%{public}d", recoverConfig_.loop);
        return recoverConfig_.loop;
    }
    MEDIA_LOGI("IsLooping enter");
    return PlayerServer::IsLooping();
}

int32_t PlayerServerMem::SetLooping(bool loop)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (CheckReleaseStateAndRecover() != MSERR_OK) {
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGI("SetLooping enter loop:%{public}d", loop);
    auto ret = PlayerServer::SetLooping(loop);
    if (ret == MSERR_OK) {
        recoverConfig_.loop = loop;
    }
    return ret;
}

void PlayerServerMem::SaveParameter(const Format &param)
{
    if (param.ContainKey(PlayerKeys::VIDEO_SCALE_TYPE)) {
        param.GetIntValue(PlayerKeys::VIDEO_SCALE_TYPE, recoverConfig_.videoScaleType);
    }
    if (param.ContainKey(PlayerKeys::CONTENT_TYPE) && param.ContainKey(PlayerKeys::STREAM_USAGE)) {
        param.GetIntValue(PlayerKeys::CONTENT_TYPE, recoverConfig_.contentType);
        param.GetIntValue(PlayerKeys::STREAM_USAGE, recoverConfig_.streamUsage);
        param.GetIntValue(PlayerKeys::RENDERER_FLAG, recoverConfig_.rendererFlag);
    }
    if (param.ContainKey(PlayerKeys::AUDIO_INTERRUPT_MODE)) {
        param.GetIntValue(PlayerKeys::AUDIO_INTERRUPT_MODE, recoverConfig_.interruptMode);
    }
}

int32_t PlayerServerMem::SetSaveParameter()
{
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");
    int32_t ret = MSERR_OK;
    if (recoverConfig_.videoScaleType != -1) {
        ret = playerEngine_->SetVideoScaleType(VideoScaleType(recoverConfig_.videoScaleType));
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SetVideoScaleType");
    }
    if (recoverConfig_.contentType != -1 && recoverConfig_.streamUsage != -1) {
        ret = playerEngine_->SetAudioRendererInfo(recoverConfig_.contentType,
            recoverConfig_.streamUsage, recoverConfig_.rendererFlag);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SetAudioRendererInfo");
    }
    if (recoverConfig_.interruptMode != -1) {
        ret = playerEngine_->SetAudioInterruptMode(recoverConfig_.interruptMode);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SetAudioInterruptMode");
    }
    return ret;
}

int32_t PlayerServerMem::SetParameter(const Format &param)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (CheckReleaseStateAndRecover() != MSERR_OK) {
        return MSERR_INVALID_OPERATION;
    }

    MEDIA_LOGI("SetParameter enter");
    auto ret = PlayerServer::SetParameter(param);
    if (ret == MSERR_OK) {
        SaveParameter(param);
    }
    return ret;
}

int32_t PlayerServerMem::SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (CheckReleaseStateAndRecover() != MSERR_OK) {
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGI("SetPlayerCallback enter");
    auto ret = PlayerServer::SetPlayerCallback(callback);
    if (ret == MSERR_OK) {
        recoverConfig_.callback = callback;
    }
    return ret;
}

int32_t PlayerServerMem::SelectBitRate(uint32_t bitRate)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (CheckReleaseStateAndRecover() != MSERR_OK) {
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGI("SelectBitRate enter bitRate:%{public}u", bitRate);
    auto ret = PlayerServer::SelectBitRate(bitRate);
    if (ret == MSERR_OK) {
        recoverConfig_.bitRate = bitRate;
    }
    return ret;
}

void PlayerServerMem::OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    std::lock_guard<std::recursive_mutex> lockCb(recMutexCb_);
    PlayerServer::OnInfo(type, extra, infoBody);

    CheckHasRecover(type, extra);
}

int32_t PlayerServerMem::GetInformationBeforeMemReset()
{
    auto ret = PlayerServer::GetCurrentTime(recoverConfig_.currentTime);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to GetCurrentTime");
    ret = PlayerServer::GetVideoTrackInfo(recoverConfig_.videoTrack);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to GetVideoTrack");
    ret = PlayerServer::GetAudioTrackInfo(recoverConfig_.audioTrack);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to GetAudioTrack");
    recoverConfig_.videoWidth = PlayerServer::GetVideoWidth();
    recoverConfig_.videoHeight = PlayerServer::GetVideoHeight();
    ret = PlayerServer::GetDuration(recoverConfig_.duration);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to GetDuration");
    recoverConfig_.isPlaying = PlayerServer::IsPlaying();
    
    GetPlayerServerConfig();

    return MSERR_OK;
}

int32_t PlayerServerMem::ReleaseMemByManage()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    MEDIA_LOGI("Enter ReleaseMemByManage");
    if (isReleaseMemByManage_ || isRecoverMemByUser_) {
        MEDIA_LOGE("ReleaseMemByManage %{public}d, RecoverMemByUser %{public}d",
            isReleaseMemByManage_, isRecoverMemByUser_);
        return MSERR_OK;
    }

    recoverConfig_.curState = std::static_pointer_cast<BaseState>(GetCurrState());
    auto ret = StateRelease();
    if (ret == MSERR_INVALID_STATE) {
        MEDIA_LOGI("CurState is Idle or Playing");
        return MSERR_OK;
    }
    
    playerCb_ = nullptr;
    ret = PlayerServer::Reset();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("ReleaseMemByManage fail");
        return ret;
    }

    isReleaseMemByManage_ = true;
    MEDIA_LOGI("ReleaseMemByManage success");
    return ret;
}

int32_t PlayerServerMem::CheckReleaseStateAndRecover()
{
    if (isReleaseMemByManage_) {
        if (RecoverMemByUser() != MSERR_OK) {
            MEDIA_LOGE("RecoverMemByUser fail");
            return MSERR_INVALID_OPERATION;
        }
    }
    return MSERR_OK;
}

int32_t PlayerServerMem::StateRecover()
{
    int32_t ret;
    std::string curStateName = recoverConfig_.curState->GetStateName();
    if (recoverConfig_.curState == initializedState_ || recoverConfig_.curState == stoppedState_) {
        SetPlayerServerConfig();
        ret = SetSourceInternal();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION,
            "curState:%{public}s failed to SetSource url", curStateName.c_str());
        ret = SetConfigInternal();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION,
            "curState:%{public}s failed to SetConfigInternal", curStateName.c_str());
    } else if (recoverConfig_.curState == preparingState_ || recoverConfig_.curState == preparedState_ ||
        recoverConfig_.curState == pausedState_) {
        SetPlayerServerConfig();
        ret = SetSourceInternal();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION,
            "curState:%{public}s failed to SetSource url", curStateName.c_str());
        ret = SetConfigInternal();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION,
            "curState:%{public}s failed to SetConfigInternal", curStateName.c_str());
        ret = PrepareAsync();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION,
            "curState:%{public}s failed to PrepareAsync", curStateName.c_str());
        ret = SetBehaviorInternal();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION,
            "curState:%{public}s failed to SetBehaviorInternal", curStateName.c_str());
    } else if (recoverConfig_.curState == playbackCompletedState_) {
        SetPlayerServerConfig();
        ret = SetSourceInternal();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION,
            "curState:%{public}s failed to SetSource url", curStateName.c_str());
        ret = SetConfigInternal();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION,
            "curState:%{public}s failed to SetConfigInternal", curStateName.c_str());
        ret = PrepareAsync();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION,
            "curState:%{public}s failed to PrepareAsync", curStateName.c_str());
        ret = SetPlaybackSpeedInternal();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION,
            "curState:%{public}s failed to SetPlaybackSpeedInternal", curStateName.c_str());
    } else {
        MEDIA_LOGI("StateRecover curState:%{public}s", curStateName.c_str());
    }

    return MSERR_OK;
}

int32_t PlayerServerMem::StateRelease()
{
    std::string curStateName = recoverConfig_.curState->GetStateName();
    if (recoverConfig_.curState == idleState_ || recoverConfig_.curState == playingState_) {
        MEDIA_LOGI("CurState:%{public}s can not release mem", curStateName.c_str());
        return MSERR_INVALID_STATE;
    } else if (recoverConfig_.curState == preparingState_ || recoverConfig_.curState == preparedState_ ||
        recoverConfig_.curState == pausedState_ || recoverConfig_.curState == stoppedState_ ||
        recoverConfig_.curState == playbackCompletedState_) {
        auto ret = GetInformationBeforeMemReset();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION,
            "curState:%{public}s failed to GetInformationBeforeMemReset", curStateName.c_str());
    } else if (recoverConfig_.curState == initializedState_) {
        GetPlayerServerConfig();
    } else {
        MEDIA_LOGI("StateRelease curState:%{public}s", curStateName.c_str());
    }

    return MSERR_OK;
}

int32_t PlayerServerMem::RecoverMemByUser()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    MEDIA_LOGI("Enter RecoverMemByUser");
    if (!isReleaseMemByManage_ || isRecoverMemByUser_) {
        MEDIA_LOGE("ReleaseMemByManage %{public}d, RecoverMemByUser %{public}d",
            isReleaseMemByManage_, isRecoverMemByUser_);
        return MSERR_OK;
    }

    isReleaseMemByManage_ = false;
    isRecoverMemByUser_ = true;
    auto ret = StateRecover();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("RecoverMemByUser StateRecover fail");
        isReleaseMemByManage_ = true;
        isRecoverMemByUser_ = false;
        return ret;
    }
    
    MEDIA_LOGI("RecoverMemByUser success");
    return ret;
}

int32_t PlayerServerMem::RecoverPlayerCb()
{
    MEDIA_LOGI("PlayerServerMem RecoverPlayerCb");
    if (recoverConfig_.callback != nullptr) {
        playerCb_ = recoverConfig_.callback;
    }
    isRecoverMemByUser_ = false;
    return MSERR_OK;
}

void PlayerServerMem::RecoverToInitialized(PlayerOnInfoType type, int32_t extra)
{
    if (type == INFO_TYPE_STATE_CHANGE && extra == PLAYER_INITIALIZED) {
        (void)RecoverPlayerCb();
    }
}

void PlayerServerMem::RecoverToPrepared(PlayerOnInfoType type, int32_t extra)
{
    if (recoverConfig_.currentTime != 0) {
        if (type == INFO_TYPE_SEEKDONE) {
            (void)RecoverPlayerCb();
        }
    } else if (recoverConfig_.speedMode != SPEED_FORWARD_1_00_X) {
        if (type == INFO_TYPE_SPEEDDONE) {
            (void)RecoverPlayerCb();
        }
    } else if (type == INFO_TYPE_STATE_CHANGE && extra == PLAYER_PREPARED) {
        (void)RecoverPlayerCb();
    }
}

void PlayerServerMem::RecoverToCompleted(PlayerOnInfoType type, int32_t extra)
{
    if (recoverConfig_.speedMode != SPEED_FORWARD_1_00_X) {
        if (type == INFO_TYPE_SPEEDDONE) {
            (void)RecoverPlayerCb();
        }
    } else if (type == INFO_TYPE_STATE_CHANGE && extra == PLAYER_PREPARED) {
        (void)RecoverPlayerCb();
    }
}

void PlayerServerMem::StateRecoverPlayerCb(PlayerOnInfoType type, int32_t extra)
{
    if (recoverConfig_.curState == initializedState_ || recoverConfig_.curState == stoppedState_) {
        RecoverToInitialized(type, extra);
    } else if (recoverConfig_.curState == preparingState_ || recoverConfig_.curState == preparedState_ ||
        recoverConfig_.curState == pausedState_) {
        RecoverToPrepared(type, extra);
    } else if (recoverConfig_.curState == playbackCompletedState_) {
        RecoverToCompleted(type, extra);
    } else {
        MEDIA_LOGI("StateRecoverPlayerCb type:%{public}d, extra:%{public}d, curState:%{public}s",
            type, extra, recoverConfig_.curState->GetStateName().c_str());
    }
}

void PlayerServerMem::CheckHasRecover(PlayerOnInfoType type, int32_t extra)
{
    MEDIA_LOGI("CheckHasRecover enter, type:%{public}d, extra:%{public}d, isRecoverMemByUser:%{public}d",
        type, extra, isRecoverMemByUser_);
    if (!isRecoverMemByUser_) {
        return;
    }

    StateRecoverPlayerCb(type, extra);
}

void PlayerServerMem::ResetForMemManage()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (isReleaseMemByManage_ || isAudioPlayer_ || IsPlaying()) {
        return;
    }

    auto ret = ReleaseMemByManage();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("ResetForMemManage ReleaseMemByManage fail");
        return;
    }
    MEDIA_LOGI("ResetForMemManage ReleaseMemByManage success");
}
}
}