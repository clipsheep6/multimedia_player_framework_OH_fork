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
#include "media_log.h"
#include "media_errors.h"
#include "media_parcel.h"
#include "media_dfx.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerServiceProxy"};
}

namespace OHOS {
namespace Media {
PlayerServiceProxy::PlayerServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardPlayerService>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerServiceProxy::~PlayerServiceProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
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
    int error = Remote()->SendRequest(SET_LISTENER_OBJ, data, reply, option);
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
    int error = Remote()->SendRequest(SET_SOURCE, data, reply, option);
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
    int error = Remote()->SendRequest(SET_MEDIA_DATA_SRC_OBJ, data, reply, option);
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
    int error = Remote()->SendRequest(SET_FD_SOURCE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetSource failed, error: %{public}d", error);
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

    int error = Remote()->SendRequest(PLAY, data, reply, option);
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

    int error = Remote()->SendRequest(PREPARE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "Prepare failed, error: %{public}d", error);
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

    int error = Remote()->SendRequest(PREPAREASYNC, data, reply, option);
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

    int error = Remote()->SendRequest(PAUSE, data, reply, option);
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

    int error = Remote()->SendRequest(STOP, data, reply, option);
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

    int error = Remote()->SendRequest(RESET, data, reply, option);
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

    int error = Remote()->SendRequest(RELEASE, data, reply, option);
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

    int error = Remote()->SendRequest(RELEASE, data, reply, option);
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
    int error = Remote()->SendRequest(SET_VOLUME, data, reply, option);
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
    int error = Remote()->SendRequest(SEEK, data, reply, option);
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

    int error = Remote()->SendRequest(GET_CURRENT_TIME, data, reply, option);
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

    int error = Remote()->SendRequest(GET_VIDEO_TRACK_INFO, data, reply, option);
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

    int error = Remote()->SendRequest(GET_AUDIO_TRACK_INFO, data, reply, option);
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

int32_t PlayerServiceProxy::GetVideoWidth()
{
    MediaTrace trace("binder::GetVideoWidth");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, 0, "Failed to write descriptor!");

    int error = Remote()->SendRequest(GET_VIDEO_WIDTH, data, reply, option);
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

    int error = Remote()->SendRequest(GET_VIDEO_HEIGHT, data, reply, option);
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

    int error = Remote()->SendRequest(GET_DURATION, data, reply, option);
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
    int error = Remote()->SendRequest(SET_PLAYERBACK_SPEED, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetPlaybackSpeed failed, error: %{public}d", error);
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

    int error = Remote()->SendRequest(GET_PLAYERBACK_SPEED, data, reply, option);
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
    int error = Remote()->SendRequest(SELECT_BIT_RATE, data, reply, option);
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
    MEDIA_LOGI("surfaceFormat is %{public}s!", format.c_str());

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    (void)data.WriteRemoteObject(object);
    data.WriteString(format);
    int error = Remote()->SendRequest(SET_VIDEO_SURFACE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetVideoSurface failed, error: %{public}d", error);
    return reply.ReadInt32();
}
#endif

bool PlayerServiceProxy::IsPlaying()
{
    MediaTrace trace("binder::IsPlaying");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, false, "Failed to write descriptor!");

    int error = Remote()->SendRequest(IS_PLAYING, data, reply, option);
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

    int error = Remote()->SendRequest(IS_LOOPING, data, reply, option);
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
    int error = Remote()->SendRequest(SET_LOOPING, data, reply, option);
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

    int error = Remote()->SendRequest(SET_RENDERER_DESC, data, reply, option);
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

    int error = Remote()->SendRequest(DESTROY, data, reply, option);
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

    int error = Remote()->SendRequest(SET_CALLBACK, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "SetPlayerCallback failed, error: %{public}d", error);

    return reply.ReadInt32();
}
} // namespace Media
} // namespace OHOS
