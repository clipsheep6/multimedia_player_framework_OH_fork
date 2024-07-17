/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
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
#include "player_server_mem_state.h"
#include "av_common.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerServerMem"};
}

namespace OHOS {
namespace Media {
constexpr int32_t CONTINUE_RESET_MAX_NUM = 5;
constexpr int32_t WAIT_RECOVER_TIME_SEC = 1;
constexpr double APP_BACK_GROUND_DESTROY_MEMERY_LAST_SET_TIME = 60.0;
constexpr double APP_FRONT_GROUND_DESTROY_MEMERY_LAST_SET_TIME = 120.0;
std::shared_ptr<IPlayerService> PlayerServerMem::Create()
{
    MEDIA_LOGD("Create new PlayerServerMem");
    std::shared_ptr<PlayerServerMem> playerServerMem = std::make_shared<PlayerServerMem>();
    CHECK_AND_RETURN_RET_LOG(playerServerMem != nullptr, nullptr, "failed to new PlayerServerMem");

    (void)playerServerMem->Init();
    return playerServerMem;
}

void PlayerServerMem::SetStateMap()
{
    stateMap_[idleState_.get()] = std::static_pointer_cast<MemBaseState>(memIdleState_);
    stateMap_[initializedState_.get()] = std::static_pointer_cast<MemBaseState>(memInitializedState_);
    stateMap_[preparingState_.get()] = std::static_pointer_cast<MemBaseState>(memPreparingState_);
    stateMap_[preparedState_.get()] = std::static_pointer_cast<MemBaseState>(memPreparedState_);
    stateMap_[playingState_.get()] = std::static_pointer_cast<MemBaseState>(memPlayingState_);
    stateMap_[pausedState_.get()] = std::static_pointer_cast<MemBaseState>(memPausedState_);
    stateMap_[stoppedState_.get()] = std::static_pointer_cast<MemBaseState>(memStoppedState_);
    stateMap_[playbackCompletedState_.get()] = std::static_pointer_cast<MemBaseState>(memPlaybackCompletedState_);
}

int32_t PlayerServerMem::Init()
{
    memIdleState_ = std::make_shared<MemIdleState>(*this);
    memInitializedState_ = std::make_shared<MemInitializedState>(*this);
    memPreparingState_ = std::make_shared<MemPreparingState>(*this);
    memPreparedState_ = std::make_shared<MemPreparedState>(*this);
    memPlayingState_ = std::make_shared<MemPlayingState>(*this);
    memPausedState_ = std::make_shared<MemPausedState>(*this);
    memStoppedState_ = std::make_shared<MemStoppedState>(*this);
    memPlaybackCompletedState_ = std::make_shared<MemPlaybackCompletedState>(*this);

    recoverConfig_.currState = std::static_pointer_cast<MemBaseState>(memIdleState_);
    lastestUserSetTime_ = std::chrono::steady_clock::now();
    auto ret = PlayerServer::Init();
    SetStateMap();
    return ret;
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
    MEDIA_LOGI("sourceType:%{public}d", recoverConfig_.sourceType);
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

int32_t PlayerServerMem::AddSubSourceInternal()
{
    if (!recoverConfig_.subUrl.empty()) {
        for (auto &url : recoverConfig_.subUrl) {
            (void)PlayerServer::AddSubSource(url);
        }
    }

    if (!recoverConfig_.subFdSrc.empty()) {
        for (auto &fdSrc : recoverConfig_.subFdSrc) {
            (void)PlayerServer::AddSubSource(fdSrc.fd, fdSrc.offset, fdSrc.size);
        }
    }

    return MSERR_OK;
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
    MEDIA_LOGI("leftVolume:%{public}f rightVolume:%{public}f loop:%{public}d bitRate:%{public}d",
        recoverConfig_.leftVolume, recoverConfig_.rightVolume, recoverConfig_.loop, recoverConfig_.bitRate);
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
    MEDIA_LOGI("speedMode:%{public}d currentTime:%{public}d audioIndex:%{public}d",
        recoverConfig_.speedMode, recoverConfig_.currentTime, recoverConfig_.audioIndex);
    if (recoverConfig_.speedMode != SPEED_FORWARD_1_00_X) {
        ret = PlayerServer::SetPlaybackSpeed(recoverConfig_.speedMode);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SetPlaybackSpeed");
    }

    if (recoverConfig_.currentTime != 0) {
        ret = PlayerServer::Seek(recoverConfig_.currentTime, SEEK_CLOSEST);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to Seek");
    }

    if (NeedSelectAudioTrack()) {
        ret = PlayerServer::SelectTrack(recoverConfig_.audioIndex);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SelectTrack");
    }

    return ret;
}

int32_t PlayerServerMem::SetPlaybackSpeedInternal()
{
    MEDIA_LOGI("speedMode:%{public}d audioIndex:%{public}d", recoverConfig_.speedMode, recoverConfig_.audioIndex);
    int ret;
    if (recoverConfig_.speedMode != SPEED_FORWARD_1_00_X) {
        ret = PlayerServer::SetPlaybackSpeed(recoverConfig_.speedMode);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "PreparedState failed to SetPlaybackSpeed");
    }

    if (NeedSelectAudioTrack()) {
        ret = PlayerServer::SelectTrack(recoverConfig_.audioIndex);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to SelectTrack");
    }
    return MSERR_OK;
}

void PlayerServerMem::SetResourceTypeBySysParam()
{
    std::string cmd;
    int32_t ret = OHOS::system::GetStringParameter("sys.media.player.resource.type", cmd, "");
    if (ret == 0 && !cmd.empty()) {
        if (cmd == "NetWork") {
            isLocalResource_ = false;
        } else if (cmd == "Local") {
            isLocalResource_ = true;
        }
    }
}

int32_t PlayerServerMem::SetSource(const std::string &url)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");
    
    lastestUserSetTime_ = std::chrono::steady_clock::now();
    recoverConfig_.url = url;
    recoverConfig_.sourceType = static_cast<int32_t>(PlayerSourceType::SOURCE_TYPE_URL);
    auto ret = PlayerServer::SetSource(url);
    if (ret == MSERR_OK) {
        isLocalResource_ = false;
        if (url.find("fd://") != std::string::npos || url.find("file://") != std::string::npos) {
            isLocalResource_ = true;
        }
        SetResourceTypeBySysParam();
    }
    return ret;
}

int32_t PlayerServerMem::SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    recoverConfig_.dataSrc = dataSrc;
    recoverConfig_.sourceType = static_cast<int32_t>(PlayerSourceType::SOURCE_TYPE_DATASRC);
    auto ret = PlayerServer::SetSource(dataSrc);
    if (ret == MSERR_OK) {
        isLocalResource_ = false;
    }
    return ret;
}

int32_t PlayerServerMem::SetSource(int32_t fd, int64_t offset, int64_t size)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();

    recoverConfig_.fd = fd;
    recoverConfig_.offset = offset;
    recoverConfig_.size = size;
    recoverConfig_.sourceType = static_cast<int32_t>(PlayerSourceType::SOURCE_TYPE_FD);
    auto ret = PlayerServer::SetSource(fd, offset, size);
    if (ret == MSERR_OK) {
        isLocalResource_ = true;
    }
    return ret;
}

int32_t PlayerServerMem::AddSubSource(const std::string &url)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    recoverConfig_.subUrl.emplace_back(url);
    return PlayerServer::AddSubSource(url);
}

int32_t PlayerServerMem::AddSubSource(int32_t fd, int64_t offset, int64_t size)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    FdSrcInfo fdSrc {fd, offset, size};
    recoverConfig_.subFdSrc.emplace_back(fdSrc);
    return PlayerServer::AddSubSource(fd, offset, size);
}

int32_t PlayerServerMem::Play()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::Play();
}

int32_t PlayerServerMem::Prepare()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::Prepare();
}

int32_t PlayerServerMem::PrepareAsync()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::PrepareAsync();
}

int32_t PlayerServerMem::PrepareAsyncInner()
{
    return PlayerServer::PrepareAsync();
}

int32_t PlayerServerMem::Pause()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::Pause();
}

int32_t PlayerServerMem::Stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "Stop:RecoverMemByUser fail");

    recoverCond_.wait_for(lock, std::chrono::seconds(WAIT_RECOVER_TIME_SEC), [this] {
        return isLocalResource_ ? (!isRecoverMemByUser_) : (!isSeekToCurrentTime_);
    });
    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::Stop();
}

int32_t PlayerServerMem::Reset()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "Reset:RecoverMemByUser fail");

    recoverCond_.wait_for(lock, std::chrono::seconds(WAIT_RECOVER_TIME_SEC), [this] {
        return isLocalResource_ ? (!isRecoverMemByUser_) : (!isSeekToCurrentTime_);
    });
    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::Reset();
}

int32_t PlayerServerMem::Release()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "Release:RecoverMemByUser fail");

    recoverCond_.wait_for(lock, std::chrono::seconds(WAIT_RECOVER_TIME_SEC), [this] {
        return isLocalResource_ ? (!isRecoverMemByUser_) : (!isSeekToCurrentTime_);
    });
    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::Release();
}

int32_t PlayerServerMem::SetVolume(float leftVolume, float rightVolume)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    recoverConfig_.leftVolume = leftVolume;
    recoverConfig_.rightVolume = rightVolume;
    return PlayerServer::SetVolume(leftVolume, rightVolume);
}

int32_t PlayerServerMem::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::Seek(mSeconds, mode);
}

int32_t PlayerServerMem::GetCurrentTime(int32_t &currentTime)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (isLocalResource_ && isReleaseMemByManage_) {
        MEDIA_LOGI("User call GetCurrentTime:%{public}d", recoverConfig_.currentTime);
        currentTime = recoverConfig_.currentTime;
        return MSERR_OK;
    }
    return PlayerServer::GetCurrentTime(currentTime);
}

int32_t PlayerServerMem::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (isLocalResource_ && isReleaseMemByManage_) {
        MEDIA_LOGI("User call GetVideoTrackInfo");
        videoTrack = recoverConfig_.videoTrack;
        return MSERR_OK;
    }
    return PlayerServer::GetVideoTrackInfo(videoTrack);
}

int32_t PlayerServerMem::GetAudioTrackInfo(std::vector<Format> &audioTrack)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (isLocalResource_ && isReleaseMemByManage_) {
        MEDIA_LOGI("User call GetAudioTrackInfo");
        audioTrack = recoverConfig_.audioTrack;
        return MSERR_OK;
    }
    return PlayerServer::GetAudioTrackInfo(audioTrack);
}

int32_t PlayerServerMem::GetVideoWidth()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (isLocalResource_ && isReleaseMemByManage_) {
        MEDIA_LOGI("User call GetVideoWidth:%{public}d", recoverConfig_.videoWidth);
        return recoverConfig_.videoWidth;
    }
    return PlayerServer::GetVideoWidth();
}

int32_t PlayerServerMem::GetVideoHeight()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (isLocalResource_ && isReleaseMemByManage_) {
        MEDIA_LOGI("User call GetVideoHeight:%{public}d", recoverConfig_.videoHeight);
        return recoverConfig_.videoHeight;
    }
    return PlayerServer::GetVideoHeight();
}

int32_t PlayerServerMem::GetDuration(int32_t &duration)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (isLocalResource_ && isReleaseMemByManage_) {
        MEDIA_LOGI("User call GetDuration:%{public}d", recoverConfig_.duration);
        duration = recoverConfig_.duration;
        return MSERR_OK;
    }
    return PlayerServer::GetDuration(duration);
}

int32_t PlayerServerMem::SetPlaybackSpeed(PlaybackRateMode mode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    recoverConfig_.speedMode = mode;
    return PlayerServer::SetPlaybackSpeed(mode);
}

int32_t PlayerServerMem::GetPlaybackSpeed(PlaybackRateMode &mode)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (isLocalResource_ && isReleaseMemByManage_) {
        MEDIA_LOGI("User call GetPlaybackSpeed:%{public}d", recoverConfig_.speedMode);
        mode = recoverConfig_.speedMode;
        return MSERR_OK;
    }
    return PlayerServer::GetPlaybackSpeed(mode);
}

#ifdef SUPPORT_VIDEO
int32_t PlayerServerMem::SetVideoSurface(sptr<Surface> surface)
{
    std::unique_lock<std::mutex> lock(mutex_);
    isAudioPlayer_ = false;
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    recoverConfig_.surface = surface;
    return PlayerServer::SetVideoSurface(surface);
}
#endif

bool PlayerServerMem::IsPlaying()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (isLocalResource_ && isReleaseMemByManage_) {
        MEDIA_LOGI("User call IsPlaying:%{public}d", recoverConfig_.isPlaying);
        return recoverConfig_.isPlaying;
    }
    return PlayerServer::IsPlaying();
}

bool PlayerServerMem::IsLooping()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (isLocalResource_ && isReleaseMemByManage_) {
        MEDIA_LOGI("User call IsLooping:%{public}d", recoverConfig_.loop);
        return recoverConfig_.loop;
    }
    return PlayerServer::IsLooping();
}

int32_t PlayerServerMem::SetLooping(bool loop)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    recoverConfig_.loop = loop;
    return PlayerServer::SetLooping(loop);
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
        ret = playerEngine_->SetVideoScaleType(Plugins::VideoScaleType(recoverConfig_.videoScaleType));
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
    config_.effectMode = recoverConfig_.effectMode;
    return ret;
}

int32_t PlayerServerMem::SetParameter(const Format &param)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    SaveParameter(param);
    return PlayerServer::SetParameter(param);
}

int32_t PlayerServerMem::SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    recoverConfig_.callback = callback;
    return PlayerServer::SetPlayerCallback(callback);
}

int32_t PlayerServerMem::SelectBitRate(uint32_t bitRate)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    recoverConfig_.bitRate = bitRate;
    return PlayerServer::SelectBitRate(bitRate);
}

int32_t PlayerServerMem::SelectTrack(int32_t index)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::SelectTrack(index);
}

int32_t PlayerServerMem::DeselectTrack(int32_t index)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::DeselectTrack(index);
}

int32_t PlayerServerMem::GetCurrentTrack(int32_t trackType, int32_t &index)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (isLocalResource_ && isReleaseMemByManage_) {
        if (trackType == MediaType::MEDIA_TYPE_AUD) {
            index = recoverConfig_.audioIndex;
        } else if (trackType == MediaType::MEDIA_TYPE_VID) {
            index = recoverConfig_.videoIndex;
        } else if (trackType == MediaType::MEDIA_TYPE_SUBTITLE) {
            index = recoverConfig_.textIndex;
        }  else {
            MEDIA_LOGE("User call GetCurrentTrack, Invalid trackType %{public}d", trackType);
            return MSERR_INVALID_OPERATION;
        }

        MEDIA_LOGI("User call GetCurrentTrack, type %{public}d, index %{public}d", trackType, index);
        return MSERR_OK;
    }
    return PlayerServer::GetCurrentTrack(trackType, index);
}

int32_t PlayerServerMem::DumpInfo(int32_t fd)
{
    PlayerServer::DumpInfo(fd);
    std::string dumpString;
    dumpString += "PlayerServer has been reset by memory manage: ";
    if (isReleaseMemByManage_) {
        dumpString += "Yes\n";
    } else {
        dumpString += "No\n";
    }
    write(fd, dumpString.c_str(), dumpString.size());

    return MSERR_OK;
}

int32_t PlayerServerMem::HandleCodecBuffers(bool enable)
{
    std::lock_guard<std::mutex> lock(PlayerServer::mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");
    MEDIA_LOGD("HandleCodecBuffers in, enable:%{public}d", enable);

    if (lastOpStatus_ != PLAYER_PREPARED && lastOpStatus_ != PLAYER_PAUSED &&
        lastOpStatus_ != PLAYER_PLAYBACK_COMPLETE) {
        MEDIA_LOGE("0x%{public}06" PRIXPTR " Instance Can not HandleCodecBuffers, currentState is %{public}s",
            FAKE_POINTER(this), GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }
    return playerEngine_->HandleCodecBuffers(enable);
}

int32_t PlayerServerMem::SeekToCurrentTime(int32_t mSeconds, PlayerSeekMode mode)
{
    std::lock_guard<std::mutex> lock(PlayerServer::mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");

    if (lastOpStatus_ != PLAYER_PLAYBACK_COMPLETE && lastOpStatus_ != PLAYER_PAUSED &&
        lastOpStatus_ != PLAYER_PREPARED) {
        MEDIA_LOGE("Can not Seek, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }

    CHECK_AND_RETURN_RET_LOG(IsValidSeekMode(mode) == true, MSERR_INVALID_VAL, "Seek failed, inValid mode");

    if (isLiveStream_) {
        MEDIA_LOGE("Can not Seek, it is live-stream.");
        OnErrorMessage(MSERR_EXT_API9_UNSUPPORT_CAPABILITY, "Can not Seek, it is live-stream.");
        return MSERR_INVALID_OPERATION;
    }

    MEDIA_LOGD("seek position %{public}d, seek mode is %{public}d.", mSeconds, mode);
    mSeconds = std::max(0, mSeconds);

    auto seekTask = std::make_shared<TaskHandler<void>>([this, mSeconds, mode]() {
        MediaTrace::TraceBegin("PlayerServerMem::Seek", FAKE_POINTER(this));
        playerEngine_->SeekToCurrentTime(mSeconds, mode);
    });

    auto cancelTask = std::make_shared<TaskHandler<void>>([&, this]() {
        MEDIA_LOGI("Interrupted seek action");
        taskMgr_.MarkTaskDone("interrupted seek done");
    });
    isSeekToCurrentTime_ = true;
    int32_t ret = taskMgr_.SeekTask(seekTask, cancelTask, "seek", mode, mSeconds);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Seek failed");
    recoverCond_.notify_all();
    return MSERR_OK;
}

void PlayerServerMem::OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    if (isLocalResource_) {
        std::unique_lock<std::mutex> lockCb(mutexCb_);
        GetDefaultTrack(type, extra, infoBody);
        PlayerServer::OnInfo(type, extra, infoBody);
        CheckHasRecover(type, extra);
    } else {
        if (isSeekToCurrentTime_ && type == INFO_TYPE_SEEKDONE) {
            MediaTrace::TraceEnd("PlayerServerMem::Seek", FAKE_POINTER(this));
            std::lock_guard<std::mutex> lockCb(PlayerServer::mutexCb_);
            isSeekToCurrentTime_ = false;
            recoverCond_.notify_all();
            PlayerServer::HandleMessage(type, extra, infoBody);
        } else {
            PlayerServer::OnInfo(type, extra, infoBody);
        }
    }
}

int32_t PlayerServerMem::GetInformationBeforeMemReset()
{
    auto ret = PlayerServer::GetCurrentTime(recoverConfig_.currentTime);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to GetCurrentTime");
    ret = PlayerServer::GetVideoTrackInfo(recoverConfig_.videoTrack);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to GetVideoTrack");
    ret = PlayerServer::GetAudioTrackInfo(recoverConfig_.audioTrack);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to GetAudioTrack");

    ret = PlayerServer::GetCurrentTrack(MediaType::MEDIA_TYPE_AUD, recoverConfig_.audioIndex);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to GetCurrentTrack");
    ret = PlayerServer::GetCurrentTrack(MediaType::MEDIA_TYPE_VID, recoverConfig_.videoIndex);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to GetCurrentTrack");
    ret = PlayerServer::GetCurrentTrack(MediaType::MEDIA_TYPE_SUBTITLE, recoverConfig_.textIndex);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to GetCurrentTrack");

    recoverConfig_.videoWidth = PlayerServer::GetVideoWidth();
    recoverConfig_.videoHeight = PlayerServer::GetVideoHeight();
    ret = PlayerServer::GetDuration(recoverConfig_.duration);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "failed to GetDuration");
    recoverConfig_.isPlaying = PlayerServer::IsPlaying();
    recoverConfig_.effectMode = config_.effectMode;
    GetPlayerServerConfig();

    return MSERR_OK;
}

int32_t PlayerServerMem::LocalResourceRelease()
{
    if (isReleaseMemByManage_ || isRecoverMemByUser_) {
        return MSERR_OK;
    }

    auto itSatetMap = stateMap_.find(GetCurrState().get());
    CHECK_AND_RETURN_RET_LOG(itSatetMap != stateMap_.end(), MSERR_INVALID_OPERATION, "Not find correct stateMap");

    recoverConfig_.currState = itSatetMap->second;
    auto ret = recoverConfig_.currState->MemStateRelease();
    CHECK_AND_RETURN_RET(ret != MSERR_INVALID_STATE, MSERR_OK);
    
    playerCb_ = nullptr;
    ClearConfigInfo();
    ret = PlayerServer::Reset();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "PlayerServer::Reset fail");

    isReleaseMemByManage_ = true;
    MEDIA_LOGI("exit");
    return ret;
}

int32_t PlayerServerMem::NetworkResourceRelease()
{
    if (!isReleaseMemByManage_) {
        MediaTrace trace("PlayerServerMem::ReleaseMemByManage");
        MEDIA_LOGI("0x%{public}06" PRIXPTR " Instance NetworkResourceRelease enter", FAKE_POINTER(this));
        int32_t ret = HandleCodecBuffers(true);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "0x%{public}06" PRIXPTR " Instance FreeCodecBuffers Fail",
            FAKE_POINTER(this));
        isReleaseMemByManage_ = true;
        MEDIA_LOGI("0x%{public}06" PRIXPTR " Instance NetworkResourceRelease success", FAKE_POINTER(this));
    }
    return MSERR_OK;
}

int32_t PlayerServerMem::ReleaseMemByManage()
{
    if (isLocalResource_) {
        return LocalResourceRelease();
    } else {
        return NetworkResourceRelease();
    }
}

int32_t PlayerServerMem::LocalResourceRecover()
{
    if (!isReleaseMemByManage_ || isRecoverMemByUser_) {
        return MSERR_OK;
    }
    MEDIA_LOGI("Enter, currState:%{public}s", recoverConfig_.currState->GetStateName().c_str());

    isReleaseMemByManage_ = false;
    isRecoverMemByUser_ = true;
    auto ret = recoverConfig_.currState->MemStateRecover();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("StateRecover fail");
        isReleaseMemByManage_ = true;
        isRecoverMemByUser_ = false;
        return ret;
    }
    
    MEDIA_LOGI("exit");
    return ret;
}

int32_t PlayerServerMem::NetworkRecover()
{
    if (isReleaseMemByManage_) {
        MediaTrace trace("PlayerServerMem::RecoverMemByUser");
        MEDIA_LOGI("enter");
        int32_t ret = HandleCodecBuffers(false);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "RecoverCodecBuffers Fail");
        int32_t currentTime = 0;
        if (!IsCompleted()) {
            ret = PlayerServer::GetCurrentTime(currentTime);
            CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "GetCurrentTime Fail");
        }
        ret = SeekToCurrentTime(currentTime, SEEK_CLOSEST);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Seek Fail");
        isReleaseMemByManage_ = false;
        MEDIA_LOGI("exit");
    }
    return MSERR_OK;
}

int32_t PlayerServerMem::RecoverMemByUser()
{
    if (isLocalResource_) {
        return LocalResourceRecover();
    } else {
        return NetworkRecover();
    }
}

int32_t PlayerServerMem::RecoverPlayerCb()
{
    MEDIA_LOGI("enter");
    if (recoverConfig_.callback != nullptr) {
        playerCb_ = recoverConfig_.callback;
    }
    isRecoverMemByUser_ = false;
    recoverCond_.notify_all();
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
    if (NeedSelectAudioTrack() && type == INFO_TYPE_TRACKCHANGE) {
        (void)RecoverPlayerCb();
    } else if (recoverConfig_.currentTime != 0 && type == INFO_TYPE_SEEKDONE) {
        (void)RecoverPlayerCb();
    } else if (recoverConfig_.speedMode != SPEED_FORWARD_1_00_X && type == INFO_TYPE_SPEEDDONE) {
        (void)RecoverPlayerCb();
    } else if (type == INFO_TYPE_STATE_CHANGE && extra == PLAYER_PREPARED) {
        (void)RecoverPlayerCb();
        (void)AddSubSourceInternal();
    }
}

void PlayerServerMem::RecoverToCompleted(PlayerOnInfoType type, int32_t extra)
{
    if (NeedSelectAudioTrack() && type == INFO_TYPE_TRACKCHANGE) {
        (void)RecoverPlayerCb();
    } else if (recoverConfig_.speedMode != SPEED_FORWARD_1_00_X && type == INFO_TYPE_SPEEDDONE) {
        (void)RecoverPlayerCb();
    } else if (type == INFO_TYPE_STATE_CHANGE && extra == PLAYER_PREPARED) {
        (void)RecoverPlayerCb();
        (void)AddSubSourceInternal();
    }
}

void PlayerServerMem::CheckHasRecover(PlayerOnInfoType type, int32_t extra)
{
    if (isRecoverMemByUser_) {
        MEDIA_LOGI("enter, type:%{public}d, extra:%{public}d", type, extra);
        recoverConfig_.currState->MemPlayerCbRecover(type, extra);
    }
}

void PlayerServerMem::ResetFrontGroundForMemManage()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (!PlayerServer::IsPrepared() || isAudioPlayer_) {
        continueReset = 0;
        return;
    }
    if (continueReset < CONTINUE_RESET_MAX_NUM) {
        continueReset++;
        return;
    }
    continueReset = 0;

    std::chrono::duration<double> lastSetToNow = std::chrono::duration_cast<
        std::chrono::duration<double>>(std::chrono::steady_clock::now() - lastestUserSetTime_);
    if (lastSetToNow.count() > APP_FRONT_GROUND_DESTROY_MEMERY_LAST_SET_TIME) {
        ReleaseMemByManage();
    }
}

void PlayerServerMem::ResetBackGroundForMemManage()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (isAudioPlayer_ || PlayerServer::IsPlaying()) {
        continueReset = 0;
        return;
    }
    if (continueReset < CONTINUE_RESET_MAX_NUM) {
        continueReset++;
        return;
    }
    continueReset = 0;

    std::chrono::duration<double> lastSetToNow = std::chrono::duration_cast<std::chrono::duration<double>>(
        std::chrono::steady_clock::now() - lastestUserSetTime_);
    if (lastSetToNow.count() > APP_BACK_GROUND_DESTROY_MEMERY_LAST_SET_TIME) {
        ReleaseMemByManage();
    }
}

void PlayerServerMem::ResetMemmgrForMemManage()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (!(isAudioPlayer_ || PlayerServer::IsPlaying())) {
        ReleaseMemByManage();
    }
}

void PlayerServerMem::RecoverByMemManage()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(RecoverMemByUser() == MSERR_OK, "RecoverMemByUser fail");
}

bool PlayerServerMem::NeedSelectAudioTrack()
{
    return (recoverConfig_.audioIndex >= 0 && defaultAudioIndex_ >= 0 &&
        recoverConfig_.audioIndex != defaultAudioIndex_);
}

void PlayerServerMem::GetDefaultTrack(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    (void)extra;
    if (type == INFO_TYPE_DEFAULTTRACK) {
        int32_t mediaType = -1;
        (void)infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_TRACK_TYPE), mediaType);
        if (mediaType == MediaType::MEDIA_TYPE_AUD) {
            infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_TRACK_INDEX), defaultAudioIndex_);
            MEDIA_LOGI("default audio track index %{public}d", defaultAudioIndex_);
        }
        return;
    }
}
}
}