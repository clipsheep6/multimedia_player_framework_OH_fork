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

#include "player_impl.h"
#include "i_media_service.h"
#include "media_log.h"
#include "media_errors.h"
#include "media_client.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerImpl"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<Player> PlayerFactory::CreatePlayer()
{
    std::shared_ptr<PlayerImpl> impl = std::make_shared<PlayerImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "failed to new PlayerImpl");

    int32_t ret = impl->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to init PlayerImpl");

    return impl;
}

int32_t PlayerImpl::Init()
{
    std::shared_ptr<IMedia> p = MediaServiceFactory::GetInstance().CreateMediaService(IStandardMediaService::MediaSystemAbility::MEDIA_PLAYER);
    playerService_ = std::static_pointer_cast<IPlayerService>(p);
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_UNKNOWN, "failed to create player service");
    return MSERR_OK;
}

PlayerImpl::PlayerImpl()
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerImpl::~PlayerImpl()
{
    if (playerService_ != nullptr) {
        (void)MediaServiceFactory::GetInstance().DestroyMediaService(playerService_, IStandardMediaService::MediaSystemAbility::MEDIA_PLAYER);
        playerService_ = nullptr;
    }
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t PlayerImpl::SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    CHECK_AND_RETURN_RET_LOG(dataSrc != nullptr, MSERR_INVALID_VAL, "failed to create data source");
    MEDIA_LOGW("KPI-TRACE: PlayerImpl SetSource in(dataSrc)");
    return playerService_->SetSource(dataSrc);
}

int32_t PlayerImpl::SetSource(const std::string &url)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");
    CHECK_AND_RETURN_RET_LOG(!url.empty(), MSERR_INVALID_VAL, "url is empty..");
    MEDIA_LOGW("KPI-TRACE: PlayerImpl SetSource in(url)");
    return playerService_->SetSource(url);
}

int32_t PlayerImpl::SetSource(int32_t fd, int64_t offset, int64_t size)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");
    MEDIA_LOGW("KPI-TRACE: PlayerImpl SetSource in(fd)");
    return playerService_->SetSource(fd, offset, size);
}

int32_t PlayerImpl::Play()
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");
    MEDIA_LOGW("KPI-TRACE: PlayerImpl Play in");
    return playerService_->Play();
}

int32_t PlayerImpl::Prepare()
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");
    MEDIA_LOGW("KPI-TRACE: PlayerImpl Prepare in");
    return playerService_->Prepare();
}

int32_t PlayerImpl::PrepareAsync()
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");
    MEDIA_LOGW("KPI-TRACE: PlayerImpl PrepareAsync in");
    return playerService_->PrepareAsync();
}

int32_t PlayerImpl::Pause()
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->Pause();
}

int32_t PlayerImpl::Stop()
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");
    MEDIA_LOGW("KPI-TRACE: PlayerImpl Stop in");
    return playerService_->Stop();
}

int32_t PlayerImpl::Reset()
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");
    MEDIA_LOGW("KPI-TRACE: PlayerImpl Reset in");
    return playerService_->Reset();
}

int32_t PlayerImpl::Release()
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");
    (void)playerService_->Release();
    (void)MediaServiceFactory::GetInstance().DestroyMediaService(playerService_, IStandardMediaService::MediaSystemAbility::MEDIA_PLAYER);
    playerService_ = nullptr;
    return MSERR_OK;
}

int32_t PlayerImpl::SetVolume(float leftVolume, float rightVolume)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->SetVolume(leftVolume, rightVolume);
}

int32_t PlayerImpl::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->Seek(mSeconds, mode);
}

int32_t PlayerImpl::GetCurrentTime(int32_t &currentTime)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->GetCurrentTime(currentTime);
}

int32_t PlayerImpl::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->GetVideoTrackInfo(videoTrack);
}

int32_t PlayerImpl::GetAudioTrackInfo(std::vector<Format> &audioTrack)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->GetAudioTrackInfo(audioTrack);
}

int32_t PlayerImpl::GetVideoWidth()
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->GetVideoWidth();
}

int32_t PlayerImpl::GetVideoHeight()
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->GetVideoHeight();
}

int32_t PlayerImpl::SetPlaybackSpeed(PlaybackRateMode mode)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->SetPlaybackSpeed(mode);
}

int32_t PlayerImpl::GetPlaybackSpeed(PlaybackRateMode &mode)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->GetPlaybackSpeed(mode);
}

int32_t PlayerImpl::SelectBitRate(uint32_t bitRate)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->SelectBitRate(bitRate);
}

int32_t PlayerImpl::GetDuration(int32_t &duration)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->GetDuration(duration);
}

#ifdef SUPPORT_VIDEO
int32_t PlayerImpl::SetVideoSurface(sptr<Surface> surface)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, MSERR_INVALID_VAL, "surface is nullptr");

    surface_ = surface;
    return playerService_->SetVideoSurface(surface);
}
#endif

bool PlayerImpl::IsPlaying()
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, false, "player service does not exist..");

    return playerService_->IsPlaying();
}

bool PlayerImpl::IsLooping()
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, false, "player service does not exist..");

    return playerService_->IsLooping();
}

int32_t PlayerImpl::SetLooping(bool loop)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");

    return playerService_->SetLooping(loop);
}

int32_t PlayerImpl::SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_INVALID_VAL, "callback is nullptr");
    return playerService_->SetPlayerCallback(callback);
}

int32_t PlayerImpl::SetParameter(const Format &param)
{
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_INVALID_OPERATION, "player service does not exist..");
    return playerService_->SetParameter(param);
}
} // namespace Media
} // namespace OHOS
