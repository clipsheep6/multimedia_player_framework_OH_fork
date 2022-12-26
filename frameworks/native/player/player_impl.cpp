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

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerImpl"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<Player> PlayerFactory::CreatePlayer()
{
    MEDIA_LOGD("PlayerImpl: CreatePlayer in");
    std::shared_ptr<PlayerImpl> impl = std::make_shared<PlayerImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "failed to new PlayerImpl");

    int32_t ret = impl->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to init PlayerImpl");

    return impl;
}

int32_t PlayerImpl::Init()
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " Init in", FAKE_POINTER(this));
    playerService_ = MediaServiceFactory::GetInstance().CreatePlayerService();
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
        (void)MediaServiceFactory::GetInstance().DestroyPlayerService(playerService_);
        playerService_ = nullptr;
    }
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t PlayerImpl::SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " SetSource in(dataSrc)", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(dataSrc != nullptr, MSERR_INVALID_VAL, "failed to create data source");
    return playerService_->SetSource(dataSrc);
}

int32_t PlayerImpl::SetSource(const std::string &url)
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " SetSource in(url): %{public}s", FAKE_POINTER(this), url.c_str());
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    CHECK_AND_RETURN_RET_LOG(!url.empty(), MSERR_INVALID_VAL, "url is empty..");
    return playerService_->SetSource(url);
}

int32_t PlayerImpl::SetSource(int32_t fd, int64_t offset, int64_t size)
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " SetSource in(fd)", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    return playerService_->SetSource(fd, offset, size);
}

int32_t PlayerImpl::Play()
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " Play in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    return playerService_->Play();
}

int32_t PlayerImpl::Prepare()
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " Prepare in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    return playerService_->Prepare();
}

int32_t PlayerImpl::PrepareAsync()
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " PrepareAsync in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    return playerService_->PrepareAsync();
}

int32_t PlayerImpl::Pause()
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " Pause in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    return playerService_->Pause();
}

int32_t PlayerImpl::Stop()
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " Stop in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    return playerService_->Stop();
}

int32_t PlayerImpl::Reset()
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " Reset in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    return playerService_->Reset();
}

int32_t PlayerImpl::Release()
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " Release in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    (void)playerService_->Release();
    (void)MediaServiceFactory::GetInstance().DestroyPlayerService(playerService_);
    playerService_ = nullptr;
    return MSERR_OK;
}

int32_t PlayerImpl::ReleaseSync()
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " ReleaseSync in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    (void)playerService_->ReleaseSync();
    (void)MediaServiceFactory::GetInstance().DestroyPlayerService(playerService_);
    playerService_ = nullptr;
    return MSERR_OK;
}

int32_t PlayerImpl::SetVolume(float leftVolume, float rightVolume)
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " SetVolume(%{public}f, %{public}f) in",
        FAKE_POINTER(this), leftVolume, rightVolume);
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    return playerService_->SetVolume(leftVolume, rightVolume);
}

int32_t PlayerImpl::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " Seek in, seek to %{public}d ms, mode is %{public}d",
        FAKE_POINTER(this), mSeconds, mode);
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    return playerService_->Seek(mSeconds, mode);
}

int32_t PlayerImpl::GetCurrentTime(int32_t &currentTime)
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " GetCurrentTime in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    return playerService_->GetCurrentTime(currentTime);
}

int32_t PlayerImpl::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " GetVideoTrackInfo in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    return playerService_->GetVideoTrackInfo(videoTrack);
}

int32_t PlayerImpl::GetAudioTrackInfo(std::vector<Format> &audioTrack)
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " GetAudioTrackInfo in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    return playerService_->GetAudioTrackInfo(audioTrack);
}

int32_t PlayerImpl::GetVideoWidth()
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " GetVideoWidth in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    return playerService_->GetVideoWidth();
}

int32_t PlayerImpl::GetVideoHeight()
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " GetVideoHeight in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    return playerService_->GetVideoHeight();
}

int32_t PlayerImpl::SetPlaybackSpeed(PlaybackRateMode mode)
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " SetPlaybackSpeed in, mode is %{public}d", FAKE_POINTER(this), mode);
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    return playerService_->SetPlaybackSpeed(mode);
}

int32_t PlayerImpl::GetPlaybackSpeed(PlaybackRateMode &mode)
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " GetPlaybackSpeed in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    return playerService_->GetPlaybackSpeed(mode);
}

int32_t PlayerImpl::SelectBitRate(uint32_t bitRate)
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " SelectBitRate(%{public}d) in", FAKE_POINTER(this), bitRate);
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    return playerService_->SelectBitRate(bitRate);
}

int32_t PlayerImpl::GetDuration(int32_t &duration)
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " GetDuration in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    return playerService_->GetDuration(duration);
}

#ifdef SUPPORT_VIDEO
int32_t PlayerImpl::SetVideoSurface(sptr<Surface> surface)
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " SetVideoSurface in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, MSERR_INVALID_VAL, "surface is nullptr");
    surface_ = surface;
    return playerService_->SetVideoSurface(surface);
}
#endif

bool PlayerImpl::IsPlaying()
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " IsPlaying in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, false, "player service does not exist..");

    return playerService_->IsPlaying();
}

bool PlayerImpl::IsLooping()
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " IsLooping in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, false, "player service does not exist..");

    return playerService_->IsLooping();
}

int32_t PlayerImpl::SetLooping(bool loop)
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " SetLooping in, loop %{public}d", FAKE_POINTER(this), loop);
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    return playerService_->SetLooping(loop);
}

int32_t PlayerImpl::SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback)
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " SetPlayerCallback in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_INVALID_VAL, "callback is nullptr");
    return playerService_->SetPlayerCallback(callback);
}

int32_t PlayerImpl::SetParameter(const Format &param)
{
    MEDIA_LOGD("PlayerImpl:0x%{public}06" PRIXPTR " SetParameter in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(playerService_ != nullptr, MSERR_SERVICE_DIED, "player service does not exist..");
    return playerService_->SetParameter(param);
}
} // namespace Media
} // namespace OHOS
