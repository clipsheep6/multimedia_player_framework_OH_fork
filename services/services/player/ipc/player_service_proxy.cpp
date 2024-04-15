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

#include "player_service_proxy.h"
#include "player_listener_stub.h"
#ifdef SUPPORT_DRM
#include "key_session_service_proxy.h"
#endif
#include "media_log.h"
#include "media_errors.h"
#include "media_parcel.h"
#include "media_dfx.h"
#include "player_xcollie.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerServiceProxy"};
}

namespace OHOS {
namespace Media {
PlayerServiceProxy::PlayerServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardPlayerService>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    playerFuncs_[SET_LISTENER_OBJ] = "Player::SetListenerObject";
    playerFuncs_[SET_SOURCE] = "Player::SetSource";
    playerFuncs_[SET_MEDIA_DATA_SRC_OBJ] = "Player::SetMediaDataSource";
    playerFuncs_[SET_FD_SOURCE] = "Player::SetFdSource";
    playerFuncs_[PLAY] = "Player::Play";
    playerFuncs_[PREPARE] = "Player::Prepare";
    playerFuncs_[SET_RENDER_FIRST_FRAME] = "Player::SetRenderFirstFrame";
    playerFuncs_[PREPAREASYNC] = "Player::PrepareAsync";
    playerFuncs_[PAUSE] = "Player::Pause";
    playerFuncs_[STOP] = "Player::Stop";
    playerFuncs_[RESET] = "Player::Reset";
    playerFuncs_[RELEASE] = "Player::Release";
    playerFuncs_[SET_VOLUME] = "Player::SetVolume";
    playerFuncs_[SEEK] = "Player::Seek";
    playerFuncs_[GET_CURRENT_TIME] = "Player::GetCurrentTime";
    playerFuncs_[GET_DURATION] = "Player::GetDuration";
    playerFuncs_[SET_PLAYERBACK_SPEED] = "Player::SetPlaybackSpeed";
    playerFuncs_[GET_PLAYERBACK_SPEED] = "Player::GetPlaybackSpeed";
#ifdef SUPPORT_VIDEO
    playerFuncs_[SET_VIDEO_SURFACE] = "Player::SetVideoSurface";
#endif
    playerFuncs_[IS_PLAYING] = "Player::IsPlaying";
    playerFuncs_[IS_LOOPING] = "Player::IsLooping";
    playerFuncs_[SET_LOOPING] = "Player::SetLooping";
    playerFuncs_[SET_RENDERER_DESC] = "Player::SetParameter";
    playerFuncs_[DESTROY] = "Player::DestroyStub";
    playerFuncs_[SET_CALLBACK] = "Player::SetPlayerCallback";
    playerFuncs_[GET_VIDEO_TRACK_INFO] = "Player::GetVideoTrackInfo";
    playerFuncs_[GET_AUDIO_TRACK_INFO] = "Player::GetAudioTrackInfo";
    playerFuncs_[GET_SUBTITLE_TRACK_INFO] = "Player::GetSubtitleTrackInfo";
    playerFuncs_[GET_VIDEO_WIDTH] = "Player::GetVideoWidth";
    playerFuncs_[GET_VIDEO_HEIGHT] = "Player::GetVideoHeight";
    playerFuncs_[SELECT_BIT_RATE] = "Player::SelectBitRate";
    playerFuncs_[SELECT_TRACK] = "Player::SelectTrack";
    playerFuncs_[DESELECT_TRACK] = "Player::DeslectTrack";
    playerFuncs_[GET_CURRENT_TRACK] = "Player::GetCurrentTrack";
    playerFuncs_[SET_DECRYPT_CONFIG] = "Player::SetDecryptConfig";
    playerFuncs_[SET_MEDIA_SOURCE] = "Player::SetMediaSource";
}

PlayerServiceProxy::~PlayerServiceProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t PlayerServiceProxy::SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    std::string funcName = "Unknown";
    auto itFunc = playerFuncs_.find(code);
    if (itFunc != playerFuncs_.end()) {
        funcName = itFunc->second;
    }

    if (funcName.compare("Player::SetVolume") == 0) {
        MEDIA_LOGD("0x%{public}06" PRIXPTR " Proxy: SendRequest task: %{public}s is received",
            FAKE_POINTER(this), funcName.c_str());
    } else {
        MEDIA_LOGI("0x%{public}06" PRIXPTR " Proxy: SendRequest task: %{public}s is received",
            FAKE_POINTER(this), funcName.c_str());
    }
    int32_t error = -1;
    error = Remote()->SendRequest(code, data, reply, option);
    return error;
}

int32_t PlayerServiceProxy::SetListenerObject(const sptr<IRemoteObject> &object)
{
    MediaTrace trace("binder::SetListenerObject");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    (void)data.WriteRemoteObject(object);
    int32_t error = SendRequest(SET_LISTENER_OBJ, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetListenerObject failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::SetSource(const std::string &url)
{
    MediaTrace trace("binder::SetSource");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    data.WriteString(url);
    int32_t error = SendRequest(SET_SOURCE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetSource failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::SetSource(const sptr<IRemoteObject> &object)
{
    MediaTrace trace("binder::SetSource");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    (void)data.WriteRemoteObject(object);
    int32_t error = SendRequest(SET_MEDIA_DATA_SRC_OBJ, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetSource failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::SetSource(int32_t fd, int64_t offset, int64_t size)
{
    MediaTrace trace("binder::SetSource");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    (void)data.WriteFileDescriptor(fd);
    (void)data.WriteInt64(offset);
    (void)data.WriteInt64(size);
    int32_t error = SendRequest(SET_FD_SOURCE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetSource failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::AddSubSource(const std::string &url)
{
    MediaTrace trace("binder::AddSubSource");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    (void)data.WriteString(url);
    int32_t error = SendRequest(ADD_SUB_SOURCE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "AddSubSource failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::AddSubSource(int32_t fd, int64_t offset, int64_t size)
{
    MediaTrace trace("binder::AddSubSource");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    (void)data.WriteFileDescriptor(fd);
    (void)data.WriteInt64(offset);
    (void)data.WriteInt64(size);
    int32_t error = SendRequest(ADD_SUB_FD_SOURCE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "AddSubSource failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::Play()
{
    MediaTrace trace("binder::Play");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int32_t error = SendRequest(PLAY, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "Play failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::Prepare()
{
    MediaTrace trace("binder::Prepare");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int32_t error = SendRequest(PREPARE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "Prepare failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::SetRenderFirstFrame(bool display)
{
    MediaTrace trace("Proxy::SetRenderFirstFrame");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    data.WriteBool(display);
    int32_t error = SendRequest(SET_RENDER_FIRST_FRAME, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetRenderFirstFrame failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::PrepareAsync()
{
    MediaTrace trace("binder::PrepareAsync");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int32_t error = SendRequest(PREPAREASYNC, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "PrepareAsync failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::Pause()
{
    MediaTrace trace("binder::Pause");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int32_t error = SendRequest(PAUSE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "Pause failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::Stop()
{
    MediaTrace trace("binder::Stop");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int32_t error = SendRequest(STOP, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "Stop failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::Reset()
{
    MediaTrace trace("binder::Reset");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int32_t error = SendRequest(RESET, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "Reset failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::Release()
{
    MediaTrace trace("binder::Release");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int32_t error = SendRequest(RELEASE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "Release failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::ReleaseSync()
{
    MediaTrace trace("binder::ReleaseSync");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int32_t error = SendRequest(RELEASE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "Release failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::SetVolume(float leftVolume, float rightVolume)
{
    MediaTrace trace("binder::SetVolume");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    data.WriteFloat(leftVolume);
    data.WriteFloat(rightVolume);
    int32_t error = SendRequest(SET_VOLUME, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetVolume failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    MediaTrace trace("binder::Seek");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    data.WriteInt32(mSeconds);
    data.WriteInt32(mode);
    int32_t error = SendRequest(SEEK, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "Seek failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::GetCurrentTime(int32_t &currentTime)
{
    MediaTrace trace("binder::GetCurrentTime");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int32_t error = SendRequest(GET_CURRENT_TIME, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "GetCurrentTime failed, error: %{public}d", error);
    currentTime = reply.ReadInt32();
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    MediaTrace trace("binder::GetVideoTrackInfo");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int32_t error = SendRequest(GET_VIDEO_TRACK_INFO, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "GetVideoTrackInfo failed, error: %{public}d", error);
    int32_t trackCnt = reply.ReadInt32();
    for (int32_t i = 0; i < trackCnt; i++) {
        Format trackInfo;
        (void)MediaParcel::Unmarshalling(reply, trackInfo);
        videoTrack.push_back(trackInfo);
    }
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::GetAudioTrackInfo(std::vector<Format> &audioTrack)
{
    MediaTrace trace("binder::GetAudioTrackInfo");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int32_t error = SendRequest(GET_AUDIO_TRACK_INFO, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "GetAudioTrackInfo failed, error: %{public}d", error);
    int32_t trackCnt = reply.ReadInt32();
    for (int32_t i = 0; i < trackCnt; i++) {
        Format trackInfo;
        (void)MediaParcel::Unmarshalling(reply, trackInfo);

        audioTrack.push_back(trackInfo);
    }
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::GetSubtitleTrackInfo(std::vector<Format> &subtitleTrack)
{
    MediaTrace trace("PlayerServiceProxy::GetSubtitleTrackInfo");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int32_t error = SendRequest(GET_SUBTITLE_TRACK_INFO, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "GetSubtitleTrackInfo failed, error: %{public}d", error);
    int32_t trackCnt = reply.ReadInt32();
    for (int32_t i = 0; i < trackCnt; i++) {
        Format trackInfo;
        (void)MediaParcel::Unmarshalling(reply, trackInfo);

        subtitleTrack.push_back(trackInfo);
    }
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::GetVideoWidth()
{
    MediaTrace trace("binder::GetVideoWidth");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, 0, "Failed to write descriptor!");

    int32_t error = SendRequest(GET_VIDEO_WIDTH, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, 0,
        "GetVideoWidth failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::GetVideoHeight()
{
    MediaTrace trace("binder::GetVideoHeight");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, 0, "Failed to write descriptor!");

    int32_t error = SendRequest(GET_VIDEO_HEIGHT, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, 0,
        "GetVideoHeight failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::GetDuration(int32_t &duration)
{
    MediaTrace trace("binder::GetDuration");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int32_t error = SendRequest(GET_DURATION, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "GetDuration failed, error: %{public}d", error);
    duration = reply.ReadInt32();
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::SetPlaybackSpeed(PlaybackRateMode mode)
{
    MediaTrace trace("binder::SetPlaybackSpeed");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    data.WriteInt32(mode);
    int32_t error = SendRequest(SET_PLAYERBACK_SPEED, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetPlaybackSpeed failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::SetMediaSource(const std::shared_ptr<AVMediaSource> &mediaSource, AVPlayStrategy strategy)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    CHECK_AND_RETURN_RET_LOG(mediaSource != nullptr, MSERR_INVALID_VAL, "mediaSource is nullptr!");
    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    data.WriteString(mediaSource->url);
    auto headerSize = static_cast<uint32_t>(mediaSource->header.size());
    if (!data.WriteUint32(headerSize)) {
        MEDIA_LOGI("Write mapSize failed");
        return MSERR_INVALID_OPERATION;
    }
    for (auto [kstr, vstr] : mediaSource->header) {
        if (!data.WriteString(kstr)) {
            MEDIA_LOGI("Write kstr failed");
            return MSERR_INVALID_OPERATION;
        }
        if (!data.WriteString(vstr)) {
            MEDIA_LOGI("Write vstr failed");
            return MSERR_INVALID_OPERATION;
        }
    }
    (void)data.WriteUint32(strategy.preferredWidth);
    (void)data.WriteUint32(strategy.preferredHeight);
    (void)data.WriteUint32(strategy.preferredBufferDuration);
    (void)data.WriteBool(strategy.preferredHdr);
    int32_t error = SendRequest(SET_MEDIA_SOURCE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetMediaSource failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::GetPlaybackSpeed(PlaybackRateMode &mode)
{
    MediaTrace trace("binder::GetPlaybackSpeed");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int32_t error = SendRequest(GET_PLAYERBACK_SPEED, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "GetPlaybackSpeed failed, error: %{public}d", error);
    int32_t tempMode = reply.ReadInt32();
    mode = static_cast<PlaybackRateMode>(tempMode);
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::SelectBitRate(uint32_t bitRate)
{
    MediaTrace trace("binder::SelectBitRate");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    data.WriteInt32(bitRate);
    int32_t error = SendRequest(SELECT_BIT_RATE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SelectBitRate failed, error: %{public}d", error);
    return reply.ReadInt32();
}

#ifdef SUPPORT_VIDEO
int32_t PlayerServiceProxy::SetVideoSurface(sptr<Surface> surface)
{
    MediaTrace trace("binder::SetVideoSurface");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    CHECK_AND_RETURN_RET_LOG(surface != nullptr, MSERR_NO_MEMORY, "surface is nullptr");
    sptr<IBufferProducer> producer = surface->GetProducer();
    CHECK_AND_RETURN_RET_LOG(producer != nullptr, MSERR_NO_MEMORY, "producer is nullptr");

    sptr<IRemoteObject> object = producer->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_NO_MEMORY, "object is nullptr");

    std::string format = surface->GetUserData("SURFACE_FORMAT");
    MEDIA_LOGI("0x%{public}06" PRIXPTR " surfaceFormat is %{public}s!", FAKE_POINTER(this), format.c_str());

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    (void)data.WriteRemoteObject(object);
    data.WriteString(format);
    int32_t error = SendRequest(SET_VIDEO_SURFACE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetVideoSurface failed, error: %{public}d", error);
    return reply.ReadInt32();
}
#endif

int32_t PlayerServiceProxy::SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySessionProxy,
    bool svp)
{
    MediaTrace trace("binder::SetDecryptConfig");
    MEDIA_LOGI("PlayerServiceProxy SetDecryptConfig");
#ifdef SUPPORT_DRM
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    CHECK_AND_RETURN_RET_LOG(keySessionProxy != nullptr, MSERR_INVALID_OPERATION, "keySessionProxy is nullptr");
    sptr<IRemoteObject> object = keySessionProxy->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_INVALID_OPERATION, "keySessionProxy object is nullptr");

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    if (data.WriteRemoteObject(object)) {
        MEDIA_LOGI("PlayerServiceProxy SetDecryptConfig WriteRemoteObject successfully");
    } else {
        MEDIA_LOGI("PlayerServiceProxy SetDecryptConfig WriteRemoteObject failed");
        return MSERR_INVALID_OPERATION;
    }
    data.WriteBool(svp);
    int32_t error = SendRequest(SET_DECRYPT_CONFIG, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetDecryptConfig failed, error: %{public}d", error);
    return reply.ReadInt32();
#else
    (void)keySessionProxy;
    (void)svp;
    return 0;
#endif
}

bool PlayerServiceProxy::IsPlaying()
{
    MediaTrace trace("binder::IsPlaying");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, false, "Failed to write descriptor!");

    int32_t error = SendRequest(IS_PLAYING, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, false,
        "IsPlaying failed, error: %{public}d", error);

    return reply.ReadBool();
}

bool PlayerServiceProxy::IsLooping()
{
    MediaTrace trace("binder::IsLooping");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, false, "Failed to write descriptor!");

    int32_t error = SendRequest(IS_LOOPING, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, false,
        "IsPlaying failed, error: %{public}d", error);

    return reply.ReadBool();
}

int32_t PlayerServiceProxy::SetLooping(bool loop)
{
    MediaTrace trace("binder::SetLooping");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    data.WriteBool(loop);
    int32_t error = SendRequest(SET_LOOPING, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetLooping failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::SetParameter(const Format &param)
{
    MediaTrace trace("binder::SetParameter");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    MediaParcel::Marshalling(data, param);

    int32_t error = SendRequest(SET_RENDERER_DESC, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetParameter failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::DestroyStub()
{
    MediaTrace trace("binder::DestroyStub");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int32_t error = SendRequest(DESTROY, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "DestroyStub failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::SetPlayerCallback()
{
    MediaTrace trace("binder::SetPlayerCallback");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int32_t error = SendRequest(SET_CALLBACK, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetPlayerCallback failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::SelectTrack(int32_t index)
{
    MediaTrace trace("PlayerServiceProxy::SelectTrack");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    data.WriteInt32(index);
    int32_t error = SendRequest(SELECT_TRACK, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SelectTrack failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::DeselectTrack(int32_t index)
{
    MediaTrace trace("PlayerServiceProxy::DeselectTrack");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    data.WriteInt32(index);
    int32_t error = SendRequest(DESELECT_TRACK, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "DeselectTrack failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t PlayerServiceProxy::GetCurrentTrack(int32_t trackType, int32_t &index)
{
    MediaTrace trace("PlayerServiceProxy::GetCurrentTrack");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    data.WriteInt32(trackType);
    int32_t error = SendRequest(GET_CURRENT_TRACK, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "GetCurrentTrack failed, error: %{public}d", error);
    index = reply.ReadInt32();
    return reply.ReadInt32();
}
} // namespace Media
} // namespace OHOS
