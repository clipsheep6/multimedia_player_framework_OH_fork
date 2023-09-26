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
#include "native_avplayer.h"
#include "native_avmemory.h"
#include "native_avformat.h"
#include "native_window.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "NativeAVPlayer"};
}

using namespace OHOS::Media;
class NativeAVPlayerCallback;
class NativeAVPlayerMediaDataSourceCallback;
OH_PLAYER_ErrCode ConvertFormatToAVFormat(const Format &format, OH_AVFormat *avFormat);

struct PlayerObject : public OH_AVPlayer {
    explicit PlayerObject(const std::shared_ptr<Player> &player)
            : player_(player) {}
    ~PlayerObject() = default;

    const std::shared_ptr<Player> player_ = nullptr;
    std::shared_ptr<NativeAVPlayerCallback> callback_ = nullptr;
    std::shared_ptr<NativeAVPlayerMediaDataSourceCallback> mediaDataSourceCallback_ = nullptr;
};

class NativeAVPlayerMediaDataSourceCallback : public IMediaDataSource {
public:
    NativeAVPlayerMediaDataSourceCallback(OH_AVPlayer *player, OH_AVPlayerMediaDataSourceCallback callback)
        : player_(player), callback_(callback){}
    virtual ~NativeAVPlayerMediaDataSourceCallback() = default;

    int32_t ReadAt(const std::shared_ptr<AVSharedMemory> &mem, uint32_t length, int64_t pos = -1) override
    {
        std::unique_lock<std::mutex> lock(mutex_);
        OH_AVMemory *object = OH_AVMemory_CreateByAddr(mem->GetBase(), mem->GetSize());
        CHECK_AND_RETURN_RET_LOG(object != nullptr, PLAYER_ERR_INVALID_VAL, "object is null");
        CHECK_AND_RETURN_RET_LOG(player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
        CHECK_AND_RETURN_RET_LOG(callback_.readAt != nullptr, PLAYER_ERR_INVALID_VAL, "readAt is null");
        return callback_.readAt(player_, object, length, pos);
    }

    int32_t GetSize(int64_t &size) override
    {
        std::unique_lock<std::mutex> lock(mutex_);
        CHECK_AND_RETURN_RET_LOG(player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
        CHECK_AND_RETURN_RET_LOG(callback_.getSize != nullptr, PLAYER_ERR_INVALID_VAL, "getSize is null");
        return callback_.getSize(player_, &size);
    }

    int32_t ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory> &mem) override
    {
        (void)pos;
        (void)length;
        (void)mem;
        return MSERR_OK;
    }

    int32_t ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory> &mem) override
    {
        (void)length;
        (void)mem;
        return MSERR_OK;
    }

private:
    OH_AVPlayer *player_;
    OH_AVPlayerMediaDataSourceCallback callback_;
    std::mutex mutex_;
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
            OH_AVFormat *object = OH_AVFormat_Create();
            ConvertFormatToAVFormat(infoBody, object);
            callback_.onInfo(player_, static_cast<OH_PlayerOnInfoType>(type), extra, object);
        }
    }

    void OnError(int32_t errorCode, const std::string &errorMsg) override
    {
        MEDIA_LOGI("OnError() is called, errorCode %{public}d", errorCode);
        std::unique_lock<std::mutex> lock(mutex_);

        if (player_ != nullptr && callback_.onError != nullptr) {
            const char *errmsg = errorMsg.c_str();
            callback_.onError(player_, errorCode, errmsg);
        }
    }

private:
    struct OH_AVPlayer *player_;
    struct OH_AVPlayerCallback callback_;
    std::mutex mutex_;
};

OH_PLAYER_ErrCode ConvertFormatToAVFormat(const Format &format, OH_AVFormat *avFormat)
{
    std::vector<std::string> keys = format.GetFormatKeys();
    for(std::vector<std::string>::iterator it = keys.begin(); it != keys.end(); it++) {
        switch (format.GetValueType(*it)) {
            case FORMAT_TYPE_INT32: {
                int32_t valueTemp = 0;
                (void)format.GetIntValue(*it, valueTemp);
                OH_AVFormat_SetIntValue(avFormat, (*it).c_str(), valueTemp);
                break;
            }
            case FORMAT_TYPE_INT64: {
                int64_t valueTemp = 0;
                (void)format.GetLongValue(*it, valueTemp);
                OH_AVFormat_SetLongValue(avFormat, (*it).c_str(), valueTemp);
                break;
            }
            case FORMAT_TYPE_FLOAT: {
                float valueTemp = 0;
                (void)format.GetFloatValue(*it, valueTemp);
                OH_AVFormat_SetFloatValue(avFormat, (*it).c_str(), valueTemp);
                break;
            }
            case FORMAT_TYPE_DOUBLE: {
                double valueTemp = 0;
                (void)format.GetDoubleValue(*it, valueTemp);
                OH_AVFormat_SetDoubleValue(avFormat, (*it).c_str(), valueTemp);
                break;
            }
            case FORMAT_TYPE_STRING: {
                std::string valueTemp = "";
                (void)format.GetStringValue(*it, valueTemp);
                OH_AVFormat_SetStringValue(avFormat, (*it).c_str(), valueTemp.c_str());
                break;
            }
            case FORMAT_TYPE_ADDR:
                break;
            default:
                break;
        }
    }

    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode ConvertAVFormatToFormat(OH_AVFormat *avFormat, Format &format)
{
    int32_t keysSize = 0;
    char **keys = OH_AVFormat_GetFormatKeys(avFormat, &keysSize);
    for (int i = 0; i < keysSize; i++) {
        switch (OH_AVFormat_GetValueType(avFormat, keys[i])) {
            case AV_FORMAT_TYPE_INT32: {
                int32_t valueTemp = 0;
                OH_AVFormat_GetIntValue(avFormat, keys[i], &valueTemp);
                (void)format.PutIntValue(keys[i], valueTemp);
                break;
            }
            case AV_FORMAT_TYPE_INT64: {
                int64_t valueTemp = 0;
                OH_AVFormat_GetLongValue(avFormat, keys[i], &valueTemp);
                (void)format.PutLongValue(keys[i], valueTemp);
                break;
            }
            case AV_FORMAT_TYPE_FLOAT: {
                float valueTemp = 0;
                OH_AVFormat_GetFloatValue(avFormat, keys[i], &valueTemp);
                (void)format.PutFloatValue(keys[i], valueTemp);
                break;
            }
            case AV_FORMAT_TYPE_DOUBLE: {
                double valueTemp = 0;
                OH_AVFormat_GetDoubleValue(avFormat, keys[i], &valueTemp);
                (void)format.PutDoubleValue(keys[i], valueTemp);
                break;
            }
            case AV_FORMAT_TYPE_STRING: {
                const char *valueTemp;
                OH_AVFormat_GetStringValue(avFormat, keys[i], &valueTemp);
                (void)format.PutStringValue(keys[i], valueTemp);
                break;
            }
            case FORMAT_TYPE_ADDR:
                break;
            default:
                break;
        }
    }

    return PLAYER_ERR_OK;
}

OH_AVPlayer *OH_AVPlayer_Create(void)
{
    std::shared_ptr<Player> player = PlayerFactory::CreatePlayer();
    CHECK_AND_RETURN_RET_LOG(player != nullptr, nullptr, "failed to PlayerFactory::CreatePlayer");

    PlayerObject *object = new(std::nothrow) PlayerObject(player);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new PlayerObject");

    return object;
}

OH_PLAYER_ErrCode OH_AVPlayer_SetMediaDataSource(OH_AVPlayer *player, OH_AVPlayerMediaDataSourceCallback dataSrc)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    CHECK_AND_RETURN_RET_LOG(dataSrc.readAt != nullptr, PLAYER_ERR_INVALID_VAL, "func readAt is null");
    CHECK_AND_RETURN_RET_LOG(dataSrc.getSize != nullptr, PLAYER_ERR_INVALID_VAL, "func getSize is null");
    playerObj->mediaDataSourceCallback_ = std::make_shared<NativeAVPlayerMediaDataSourceCallback>(player, dataSrc);
    int32_t ret = playerObj->player_->SetSource(playerObj->mediaDataSourceCallback_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player setMediaDataSource failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_SetUrlSource(OH_AVPlayer *player, const char *url)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    CHECK_AND_RETURN_RET_LOG(url != nullptr, PLAYER_ERR_INVALID_VAL, "url is null");
    int32_t ret = playerObj->player_->SetSource(url);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player setUrlSource failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_SetFdSource(OH_AVPlayer *player, int32_t fd, int64_t offset, int64_t size)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->SetSource(fd, offset, size);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player setFdSource failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_Play(OH_AVPlayer *player){
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->Play();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player play failed");
    return PLAYER_ERR_OK;
}
OH_PLAYER_ErrCode OH_AVPlayer_Prepare(OH_AVPlayer *player){
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player Prepare failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_PrepareAsync(OH_AVPlayer *player){
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->PrepareAsync();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player PrepareAsync failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_Pause(OH_AVPlayer *player){
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->Pause();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player Pause failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_Stop(OH_AVPlayer *player){
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->Stop();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player Stop failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_Reset(OH_AVPlayer *player)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->Reset();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player Reset failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_Release(OH_AVPlayer *player)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->Release();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player Release failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_ReleaseSync(OH_AVPlayer *player)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->ReleaseSync();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player ReleaseSync failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_SetVolume(OH_AVPlayer *player, float leftVolume, float rightVolume)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->SetVolume(leftVolume, rightVolume);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player SetVolume failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_Seek(OH_AVPlayer *player, int32_t mSeconds, OH_PlayerSeekMode mode)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->Seek(mSeconds, static_cast<PlayerSeekMode>(mode));
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player Seek failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_GetCurrentTime(OH_AVPlayer *player, int32_t *currentTime)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->GetCurrentTime(*currentTime);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player GetCurrentTime failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_GetVideoTrackInfo(OH_AVPlayer *player, OH_AVFormat **videoTrack, int inputLength, int *outputLength)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    CHECK_AND_RETURN_RET_LOG(videoTrack != nullptr, PLAYER_ERR_INVALID_VAL, "videoTrack is null");
    std::vector<Format> vTrack;
    int32_t ret = playerObj->player_->GetVideoTrackInfo(vTrack);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player GetVideoTrackInfo failed");
    CHECK_AND_RETURN_RET_LOG(vTrack.size() != 0, PLAYER_ERR_INVALID_VAL, "vTrack is null");
    *outputLength = vTrack.size();
    CHECK_AND_RETURN_RET_LOG(inputLength >= *outputLength, PLAYER_ERR_INVALID_VAL, "inputLength < *outputLength");
    for (int i = 0; i < *outputLength; i++) {
        ConvertFormatToAVFormat(vTrack[i], videoTrack[i]);
    }

    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_GetAudioTrackInfo(OH_AVPlayer *player, OH_AVFormat **audioTrack, int inputLength, int *outputLength)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    CHECK_AND_RETURN_RET_LOG(audioTrack != nullptr, PLAYER_ERR_INVALID_VAL, "videoTrack is null");
    std::vector<Format> aTrack;
    int32_t ret = playerObj->player_->GetAudioTrackInfo(aTrack);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player GetAudioTrackInfo failed");
    CHECK_AND_RETURN_RET_LOG(aTrack.size() != 0, PLAYER_ERR_INVALID_VAL, "aTrack is null");
    *outputLength = aTrack.size();
    CHECK_AND_RETURN_RET_LOG(inputLength >= *outputLength, PLAYER_ERR_INVALID_VAL, "inputLength < *outputLength");
    for (int i = 0; i < *outputLength; i++) {
        ConvertFormatToAVFormat(aTrack[i], audioTrack[i]);
    }

    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_GetSubtitleTrackInfo(OH_AVPlayer *player, OH_AVFormat **subtitleTrack, int inputLength, int *outputLength)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    CHECK_AND_RETURN_RET_LOG(subtitleTrack != nullptr, PLAYER_ERR_INVALID_VAL, "videoTrack is null");
    std::vector<Format> sTrack;
    int32_t ret = playerObj->player_->GetSubtitleTrackInfo(sTrack);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player GetSubtitleTrackInfo failed");
    CHECK_AND_RETURN_RET_LOG(sTrack.size() != 0, PLAYER_ERR_INVALID_VAL, "sTrack is null");
    *outputLength = sTrack.size();
    CHECK_AND_RETURN_RET_LOG(inputLength >= *outputLength, PLAYER_ERR_INVALID_VAL, "inputLength < *outputLength");
    for (int i = 0; i < *outputLength; i++) {
        ConvertFormatToAVFormat(sTrack[i], subtitleTrack[i]);
    }

    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_GetVideoWidth(OH_AVPlayer *player, int32_t *videoWidth)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    *videoWidth = playerObj->player_->GetVideoWidth();
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_GetVideoHeight(OH_AVPlayer *player, int32_t *videoHeight)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    *videoHeight = playerObj->player_->GetVideoHeight();
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_SetPlaybackSpeed(OH_AVPlayer *player, OH_PlaybackRateMode mode)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->SetPlaybackSpeed(static_cast<PlaybackRateMode>(mode));
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player SetPlaybackSpeed failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_GetPlaybackSpeed(OH_AVPlayer *player, OH_PlaybackRateMode *mode)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    PlaybackRateMode md;
    int32_t ret = playerObj->player_->GetPlaybackSpeed(md);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player GetPlaybackSpeed failed");
    *mode = static_cast<OH_PlaybackRateMode>(md);
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_SelectBitRate(OH_AVPlayer *player, uint32_t bitRate)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->SelectBitRate(bitRate);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player SelectBitRate failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_GetDuration(OH_AVPlayer *player, int32_t *duration)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->GetDuration(*duration);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player GetDuration failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_GetState(OH_AVPlayer *player, OH_PlayerStates *state)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    *state = static_cast<OH_PlayerStates>(player->state_);
    return PLAYER_ERR_OK;
}

#ifdef SUPPORT_VIDEO
OH_PLAYER_ErrCode  OH_AVPlayer_SetVideoSurface(OH_AVPlayer *player, OHNativeWindow *window)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    CHECK_AND_RETURN_RET_LOG(window != nullptr, PLAYER_ERR_INVALID_VAL, "Window is nullptr!");
    CHECK_AND_RETURN_RET_LOG(window->surface != nullptr, PLAYER_ERR_INVALID_VAL, "Input window surface is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->SetVideoSurface(window->surface);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "SetVideoSurface failed!");
    return PLAYER_ERR_OK;
}
#endif

bool OH_AVPlayer_IsPlaying(OH_AVPlayer *player)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    return playerObj->player_->IsPlaying();
}

bool OH_AVPlayer_IsLooping(OH_AVPlayer *player)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    return playerObj->player_->IsLooping();
}

OH_PLAYER_ErrCode OH_AVPlayer_SetLooping(OH_AVPlayer *player, bool loop)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->SetLooping(loop);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player SetLooping failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_SetPlayerCallback(OH_AVPlayer *player, OH_AVPlayerCallback callback)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    CHECK_AND_RETURN_RET_LOG(callback.onInfo != nullptr, PLAYER_ERR_INVALID_VAL, "onInfo is null");
    CHECK_AND_RETURN_RET_LOG(callback.onError != nullptr, PLAYER_ERR_INVALID_VAL, "onError is null");
    playerObj->callback_ = std::make_shared<NativeAVPlayerCallback>(player, callback);
    int32_t ret = playerObj->player_->SetPlayerCallback(playerObj->callback_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player SetPlayerCallback failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_SetParameter(OH_AVPlayer *player, OH_AVFormat *param)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    CHECK_AND_RETURN_RET_LOG(param != nullptr, PLAYER_ERR_INVALID_VAL, "OH_AVFormat param is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t keysSize = 0;
    char **keys = OH_AVFormat_GetFormatKeys(param, &keysSize);
    CHECK_AND_RETURN_RET_LOG(keys != nullptr, PLAYER_ERR_INVALID_VAL, "keys is nullptr!");
    Format format;
    ConvertAVFormatToFormat(param, format);
    int32_t ret = playerObj->player_->SetParameter(format);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player SetParameter failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_SelectTrack(OH_AVPlayer *player, int32_t index)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->SelectTrack(index);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player SelectTrack failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_DeselectTrack(OH_AVPlayer *player, int32_t index)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->DeselectTrack(index);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player DeselectTrack failed");
    return PLAYER_ERR_OK;
}

OH_PLAYER_ErrCode OH_AVPlayer_GetCurrentTrack(OH_AVPlayer *player, int32_t trackType, int32_t *index)
{
    CHECK_AND_RETURN_RET_LOG(player != nullptr, PLAYER_ERR_INVALID_VAL, "input player is nullptr!");
    struct PlayerObject *playerObj = reinterpret_cast<PlayerObject *>(player);
    CHECK_AND_RETURN_RET_LOG(playerObj->player_ != nullptr, PLAYER_ERR_INVALID_VAL, "player_ is null");
    int32_t ret = playerObj->player_->GetCurrentTrack(trackType, *index);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, PLAYER_ERR_INVALID_VAL, "player GetCurrentTrack failed");
    return PLAYER_ERR_OK;
}