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

#include <mutex>
#include "media_log.h"
#include "media_errors.h"
#include "native_player_magic.h"
#include "native_window.h"
#include "native_avplayer.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "NativeAVPlayer"};
}

using namespace OHOS::Media;
class NativeAVPlayerCallback;

struct PlayerObject : public OH_AVPlayer {
    explicit PlayerObject(const std::shared_ptr<Player> &player)
        : player_(player) {}
    ~PlayerObject() = default;

    const std::shared_ptr<Player> player_ = nullptr;
    std::shared_ptr<NativeAVPlayerCallback> callback_ = nullptr;
};


class NativeAVPlayerCallback : public PlayerCallback {
public:
    NativeAVPlayerCallback(OH_AVPlayer *player, OH_AVPlayerCallback callback)
        : player_(player), callback_(callback) {}
    virtual ~NativeAVPlayerCallback() = default;

    void OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody) override
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (type == INFO_TYPE_STATE_CHANGE) {
            PlayerStates state = static_cast<PlayerStates>(extra);
            player_->state_ = state;
        }

        if (player_ != nullptr && callback_.onInfo != nullptr) {
            callback_.onInfo(player_, static_cast<OH_PlayerOnInfoType>(type), extra);
        }
    }

    void OnError(int32_t errorCode, const std::string &errorMsg) override
    {
        MEDIA_LOGI("OnError() is called, errorCode %{public}d", errorCode);
        std::unique_lock<std::mutex> lock(mutex_);

        if (player_ != nullptr && callback_.onError != nullptr) {
            callback_.onError(player_, errorCode, errorMsg.c_str());
        }
    }

private:
    struct OH_AVPlayer *player_;
    struct OH_AVPlayerCallback callback_;
    std::mutex mutex_;
};


OH_AVPlayer *OH_AVPlayer_Create(void)
{
    std::shared_ptr<Player> player = PlayerFactory::CreatePlayer();
    CHECK_AND_RETURN_RET_LOG(player != nullptr, nullptr, "failed to PlayerFactory::CreatePlayer");

    PlayerObject *object = new(std::nothrow) PlayerObject(player);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new PlayerObject");

    return object;
}


OH_AVErrCode OH_AVPlayer_SetUrlSource(OH_AVPlayer *player, const char *url)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    CHECK_AND_RETURN_RET_LOG(url != nullptr, AV_ERR_INVALID_VAL, "url is null");
    int32_t ret = playerObj->player_->SetSource(url);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player setUrlSource failed");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_SetFdSource(OH_AVPlayer *player, int32_t fd, int64_t offset, int64_t size)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->SetSource(fd, offset, size);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player setFdSource failed");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_Play(OH_AVPlayer *player)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->Play();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player play failed");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_Prepare(OH_AVPlayer *player)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->PrepareAsync();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player Prepare failed");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_Pause(OH_AVPlayer *player)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->Pause();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player Pause failed");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_Stop(OH_AVPlayer *player)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->Stop();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player Stop failed");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_Reset(OH_AVPlayer *player)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->Reset();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player Reset failed");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_Release(OH_AVPlayer *player)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->Release();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player Release failed");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_ReleaseSync(OH_AVPlayer *player)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->ReleaseSync();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player ReleaseSync failed");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_SetVolume(OH_AVPlayer *player, float leftVolume, float rightVolume)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->SetVolume(leftVolume, rightVolume);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player SetVolume failed");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_Seek(OH_AVPlayer *player, int32_t mSeconds, OH_PlayerSeekMode mode)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->Seek(mSeconds, static_cast<PlayerSeekMode>(mode));
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player Seek failed");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_GetCurrentTime(OH_AVPlayer *player, int32_t *currentTime)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->GetCurrentTime(*currentTime);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player GetCurrentTime failed");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_GetVideoWidth(OH_AVPlayer *player, int32_t *videoWidth)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    *videoWidth = playerObj->player_->GetVideoWidth();
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_GetVideoHeight(OH_AVPlayer *player, int32_t *videoHeight)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    *videoHeight = playerObj->player_->GetVideoHeight();
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_SetPlaybackSpeed(OH_AVPlayer *player, OH_PlaybackRateMode mode)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->SetPlaybackSpeed(static_cast<PlaybackRateMode>(mode));
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player SetPlaybackSpeed failed");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_GetPlaybackSpeed(OH_AVPlayer *player, OH_PlaybackRateMode *mode)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    PlaybackRateMode md;
    int32_t ret = playerObj->player_->GetPlaybackSpeed(md);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player GetPlaybackSpeed failed");
    *mode = static_cast<OH_PlaybackRateMode>(md);
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_SelectBitRate(OH_AVPlayer *player, uint32_t bitRate)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->SelectBitRate(bitRate);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player SelectBitRate failed");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_GetDuration(OH_AVPlayer *player, int32_t *duration)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->GetDuration(*duration);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player GetDuration failed");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_GetState(OH_AVPlayer *player, OH_PlayerStates *state)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    *state = static_cast<OH_PlayerStates>(player->state_);
    return AV_ERR_OK;
}

#ifdef SUPPORT_VIDEO
OH_AVErrCode  OH_AVPlayer_SetVideoSurface(OH_AVPlayer *player, OHNativeWindow *window)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    CHECK_AND_RETURN_RET_LOG(window != nullptr, AV_ERR_INVALID_VAL, "Window is nullptr!");
    CHECK_AND_RETURN_RET_LOG(window->surface != nullptr, AV_ERR_INVALID_VAL, "Input window surface is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->SetVideoSurface(window->surface);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "SetVideoSurface failed!");
    return AV_ERR_OK;
}
#endif

bool OH_AVPlayer_IsPlaying(OH_AVPlayer *player)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    return playerObj->player_->IsPlaying();
}

bool OH_AVPlayer_IsLooping(OH_AVPlayer *player)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    return playerObj->player_->IsLooping();
}

OH_AVErrCode OH_AVPlayer_SetLooping(OH_AVPlayer *player, bool loop)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->SetLooping(loop);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player SetLooping failed");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_SetPlayerCallback(OH_AVPlayer *player, OH_AVPlayerCallback callback)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    CHECK_AND_RETURN_RET_LOG(callback.onInfo != nullptr, AV_ERR_INVALID_VAL, "onInfo is null");
    CHECK_AND_RETURN_RET_LOG(callback.onError != nullptr, AV_ERR_INVALID_VAL, "onError is null");
    playerObj->callback_ = std::make_shared<NativeAVPlayerCallback>(player, callback);
    int32_t ret = playerObj->player_->SetPlayerCallback(playerObj->callback_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player SetPlayerCallback failed");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_SelectTrack(OH_AVPlayer *player, int32_t index)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->SelectTrack(index);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player SelectTrack failed");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_DeselectTrack(OH_AVPlayer *player, int32_t index)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->DeselectTrack(index);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player DeselectTrack failed");
    return AV_ERR_OK;
}

OH_AVErrCode OH_AVPlayer_GetCurrentTrack(OH_AVPlayer *player, int32_t trackType, int32_t *index)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, AV_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, AV_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->GetCurrentTrack(trackType, *index);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_INVALID_VAL, "player GetCurrentTrack failed");
    return AV_ERR_OK;
}