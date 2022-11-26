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

#include "player_service_stub.h"
#include <unistd.h>
#include "player_listener_proxy.h"
#include "media_data_source_proxy.h"
#include "media_server_manager.h"
#include "media_log.h"
#include "media_errors.h"
#include "media_parcel.h"
#include "parameter.h"
#include "media_dfx.h"
#include "player_xcollie.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerServiceStub"};
}

namespace OHOS {
namespace Media {
sptr<PlayerServiceStub> PlayerServiceStub::Create()
{
    sptr<PlayerServiceStub> playerStub = new(std::nothrow) PlayerServiceStub();
    CHECK_AND_RETURN_RET_LOG(playerStub != nullptr, nullptr, "failed to new PlayerServiceStub");

    int32_t ret = playerStub->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to player stub init");
    return playerStub;
}

PlayerServiceStub::PlayerServiceStub()
    : taskQue_("PlayerServer")
{
    (void)taskQue_.Start();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerServiceStub::~PlayerServiceStub()
{
    if (playerServer_ != nullptr) {
        auto task = std::make_shared<TaskHandler<void>>([&, this] {
            int32_t id = PlayerXCollie::GetInstance().SetTimer("PlayerServiceStub::~PlayerServiceStub");
            (void)playerServer_->Release();
            PlayerXCollie::GetInstance().CancelTimer(id);
            playerServer_ = nullptr;
        });
        (void)taskQue_.EnqueueTask(task);
        (void)task->GetResult();
    }
    (void)taskQue_.Stop();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t PlayerServiceStub::Init()
{
#ifdef SUPPORT_HISTREAMER
    char useHistreamer[10] = {0}; // 10 for system parameter usage
    auto res = GetParameter("debug.media_service.histreamer", "0", useHistreamer, sizeof(useHistreamer));
    if (res == 1 && useHistreamer[0] == '1') {
        MEDIA_LOGD("use histreamer");
        playerServer_ = PlayerServerHi::Create();
    }
#endif

    if (playerServer_ == nullptr) {
        playerServer_ = PlayerServer::Create();
    }
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "failed to create PlayerServer");

    playerFuncs_[SET_LISTENER_OBJ] = { &PlayerServiceStub::SetListenerObject, "Player::SetListenerObject" };
    playerFuncs_[SET_SOURCE] = { &PlayerServiceStub::SetSource, "Player::SetSource" };
    playerFuncs_[SET_MEDIA_DATA_SRC_OBJ] = { &PlayerServiceStub::SetMediaDataSource, "Player::SetMediaDataSource" };
    playerFuncs_[SET_FD_SOURCE] = { &PlayerServiceStub::SetFdSource, "Player::SetFdSource" };
    playerFuncs_[PLAY] = { &PlayerServiceStub::Play, "Player::Play" };
    playerFuncs_[PREPARE] = { &PlayerServiceStub::Prepare, "Player::Prepare" };
    playerFuncs_[PREPAREASYNC] = { &PlayerServiceStub::PrepareAsync, "Player::PrepareAsync" };
    playerFuncs_[PAUSE] = { &PlayerServiceStub::Pause, "Player::Pause" };
    playerFuncs_[STOP] = { &PlayerServiceStub::Stop, "Player::Stop" };
    playerFuncs_[RESET] = { &PlayerServiceStub::Reset, "Player::Reset" };
    playerFuncs_[RELEASE] = { &PlayerServiceStub::Release, "Player::Release" };
    playerFuncs_[SET_VOLUME] = { &PlayerServiceStub::SetVolume, "Player::SetVolume" };
    playerFuncs_[SEEK] = { &PlayerServiceStub::Seek, "Player::Seek" };
    playerFuncs_[GET_CURRENT_TIME] = { &PlayerServiceStub::GetCurrentTime, "Player::GetCurrentTime" };
    playerFuncs_[GET_DURATION] = { &PlayerServiceStub::GetDuration, "Player::GetDuration" };
    playerFuncs_[SET_PLAYERBACK_SPEED] = { &PlayerServiceStub::SetPlaybackSpeed, "Player::SetPlaybackSpeed" };
    playerFuncs_[GET_PLAYERBACK_SPEED] = { &PlayerServiceStub::GetPlaybackSpeed, "Player::GetPlaybackSpeed" };
#ifdef SUPPORT_VIDEO
    playerFuncs_[SET_VIDEO_SURFACE] = { &PlayerServiceStub::SetVideoSurface, "Player::SetVideoSurface" };
#endif
    playerFuncs_[IS_PLAYING] = { &PlayerServiceStub::IsPlaying, "Player::IsPlaying" };
    playerFuncs_[IS_LOOPING] = { &PlayerServiceStub::IsLooping, "Player::IsLooping" };
    playerFuncs_[SET_LOOPING] = { &PlayerServiceStub::SetLooping, "Player::SetLooping" };
    playerFuncs_[SET_RENDERER_DESC] = { &PlayerServiceStub::SetParameter, "Player::SetParameter" };
    playerFuncs_[DESTROY] = { &PlayerServiceStub::DestroyStub, "Player::DestroyStub" };
    playerFuncs_[SET_CALLBACK] = { &PlayerServiceStub::SetPlayerCallback, "Player::SetPlayerCallback" };
    playerFuncs_[GET_VIDEO_TRACK_INFO] = { &PlayerServiceStub::GetVideoTrackInfo, "Player::GetVideoTrackInfo" };
    playerFuncs_[GET_AUDIO_TRACK_INFO] = { &PlayerServiceStub::GetAudioTrackInfo, "Player::GetAudioTrackInfo" };
    playerFuncs_[GET_VIDEO_WIDTH] = { &PlayerServiceStub::GetVideoWidth, "Player::GetVideoWidth" };
    playerFuncs_[GET_VIDEO_HEIGHT] = { &PlayerServiceStub::GetVideoHeight, "Player::GetVideoHeight" };
    playerFuncs_[SELECT_BIT_RATE] = { &PlayerServiceStub::SelectBitRate, "Player::SelectBitRate" };
    return MSERR_OK;
}

int32_t PlayerServiceStub::DestroyStub()
{
    MediaTrace trace("binder::DestroyStub");
    playerCallback_ = nullptr;
    if (playerServer_ != nullptr) {
        (void)playerServer_->Release();
        playerServer_ = nullptr;
    }

    MediaServerManager::GetInstance().DestroyStubObject(MediaServerManager::PLAYER, AsObject());
    return MSERR_OK;
}

int PlayerServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (PlayerServiceStub::GetDescriptor() != remoteDescriptor) {
        MEDIA_LOGE("Invalid descriptor");
        return MSERR_INVALID_OPERATION;
    }

    auto itFunc = playerFuncs_.find(code);
    if (itFunc != playerFuncs_.end()) {
        auto memberFunc = itFunc->second.first;
        auto funcName = itFunc->second.second;
        MEDIA_LOGI("Stub: OnRemoteRequest task: %{public}s is received", funcName.c_str());
        if (memberFunc != nullptr) {
            auto task = std::make_shared<TaskHandler<int>>([&, this] {
                int32_t id = PlayerXCollie::GetInstance().SetTimer(funcName);
                auto ret = (this->*memberFunc)(data, reply);
                PlayerXCollie::GetInstance().CancelTimer(id);
                return ret;
            });
            (void)taskQue_.EnqueueTask(task);
            auto result = task->GetResult();
            CHECK_AND_RETURN_RET_LOG(result.HasResult(), MSERR_INVALID_OPERATION,
                "failed to OnRemoteRequest code: %{public}u", code);
            return result.Value();
        }
    }
    MEDIA_LOGW("PlayerServiceStub: no member func supporting, applying default process");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t PlayerServiceStub::SetListenerObject(const sptr<IRemoteObject> &object)
{
    MediaTrace trace("binder::SetListenerObject");
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_NO_MEMORY, "set listener object is nullptr");

    sptr<IStandardPlayerListener> listener = iface_cast<IStandardPlayerListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, MSERR_NO_MEMORY, "failed to convert IStandardPlayerListener");

    std::shared_ptr<PlayerCallback> callback = std::make_shared<PlayerListenerCallback>(listener);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_NO_MEMORY, "failed to new PlayerListenerCallback");

    playerCallback_ = callback;
    return MSERR_OK;
}

int32_t PlayerServiceStub::SetSource(const std::string &url)
{
    MediaTrace trace("binder::SetSource(url)");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->SetSource(url);
}

int32_t PlayerServiceStub::SetSource(const sptr<IRemoteObject> &object)
{
    MediaTrace trace("binder::SetSource(datasource)");
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_NO_MEMORY, "set mediadatasrc object is nullptr");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");

    sptr<IStandardMediaDataSource> proxy = iface_cast<IStandardMediaDataSource>(object);
    CHECK_AND_RETURN_RET_LOG(proxy != nullptr, MSERR_NO_MEMORY, "failed to convert MediaDataSourceProxy");

    std::shared_ptr<IMediaDataSource> mediaDataSrc = std::make_shared<MediaDataCallback>(proxy);
    CHECK_AND_RETURN_RET_LOG(mediaDataSrc != nullptr, MSERR_NO_MEMORY, "failed to new PlayerListenerCallback");

    return playerServer_->SetSource(mediaDataSrc);
}

int32_t PlayerServiceStub::SetSource(int32_t fd, int64_t offset, int64_t size)
{
    MediaTrace trace("binder::SetSource(fd)");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->SetSource(fd, offset, size);
}

int32_t PlayerServiceStub::Play()
{
    MediaTrace trace("binder::Play");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->Play();
}

int32_t PlayerServiceStub::Prepare()
{
    MediaTrace trace("binder::Prepare");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->Prepare();
}

int32_t PlayerServiceStub::PrepareAsync()
{
    MediaTrace trace("binder::PrepareAsync");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->PrepareAsync();
}

int32_t PlayerServiceStub::Pause()
{
    MediaTrace trace("binder::Pause");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->Pause();
}

int32_t PlayerServiceStub::Stop()
{
    MediaTrace trace("binder::Stop");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->Stop();
}

int32_t PlayerServiceStub::Reset()
{
    MediaTrace trace("binder::Reset");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->Reset();
}

int32_t PlayerServiceStub::Release()
{
    MediaTrace trace("binder::Release");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->Release();
}

int32_t PlayerServiceStub::SetVolume(float leftVolume, float rightVolume)
{
    MediaTrace trace("binder::SetVolume");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->SetVolume(leftVolume, rightVolume);
}

int32_t PlayerServiceStub::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    MediaTrace trace("binder::Seek");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->Seek(mSeconds, mode);
}

int32_t PlayerServiceStub::GetCurrentTime(int32_t &currentTime)
{
    MediaTrace trace("binder::GetCurrentTime");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->GetCurrentTime(currentTime);
}

int32_t PlayerServiceStub::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    MediaTrace trace("binder::GetVideoTrackInfo");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->GetVideoTrackInfo(videoTrack);
}

int32_t PlayerServiceStub::GetAudioTrackInfo(std::vector<Format> &audioTrack)
{
    MediaTrace trace("binder::GetAudioTrackInfo");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->GetAudioTrackInfo(audioTrack);
}

int32_t PlayerServiceStub::GetVideoWidth()
{
    MediaTrace trace("binder::GetVideoWidth");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->GetVideoWidth();
}

int32_t PlayerServiceStub::GetVideoHeight()
{
    MediaTrace trace("binder::GetVideoHeight");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->GetVideoHeight();
}

int32_t PlayerServiceStub::GetDuration(int32_t &duration)
{
    MediaTrace trace("binder::GetDuration");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->GetDuration(duration);
}

int32_t PlayerServiceStub::SetPlaybackSpeed(PlaybackRateMode mode)
{
    MediaTrace trace("binder::SetPlaybackSpeed");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->SetPlaybackSpeed(mode);
}

int32_t PlayerServiceStub::GetPlaybackSpeed(PlaybackRateMode &mode)
{
    MediaTrace trace("binder::GetPlaybackSpeed");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->GetPlaybackSpeed(mode);
}

int32_t PlayerServiceStub::SelectBitRate(uint32_t bitRate)
{
    MediaTrace trace("binder::SelectBitRate");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->SelectBitRate(bitRate);
}

#ifdef SUPPORT_VIDEO
int32_t PlayerServiceStub::SetVideoSurface(sptr<Surface> surface)
{
    MediaTrace trace("binder::SetVideoSurface");
    MEDIA_LOGD("SetVideoSurface");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->SetVideoSurface(surface);
}
#endif

bool PlayerServiceStub::IsPlaying()
{
    MediaTrace trace("binder::IsPlaying");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, false, "player server is nullptr");
    return playerServer_->IsPlaying();
}

bool PlayerServiceStub::IsLooping()
{
    MediaTrace trace("binder::IsLooping");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, false, "player server is nullptr");
    return playerServer_->IsLooping();
}

int32_t PlayerServiceStub::SetLooping(bool loop)
{
    MediaTrace trace("binder::SetLooping");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->SetLooping(loop);
}

int32_t PlayerServiceStub::SetParameter(const Format &param)
{
    MediaTrace trace("binder::SetParameter");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->SetParameter(param);
}

int32_t PlayerServiceStub::SetPlayerCallback()
{
    MediaTrace trace("binder::SetPlayerCallback");
    MEDIA_LOGD("SetPlayerCallback");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->SetPlayerCallback(playerCallback_);
}

int32_t PlayerServiceStub::DumpInfo(int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return std::static_pointer_cast<PlayerServer>(playerServer_)->DumpInfo(fd);
}

int32_t PlayerServiceStub::SetListenerObject(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    reply.WriteInt32(SetListenerObject(object));
    return MSERR_OK;
}

int32_t PlayerServiceStub::SetSource(MessageParcel &data, MessageParcel &reply)
{
    std::string url = data.ReadString();
    reply.WriteInt32(SetSource(url));
    return MSERR_OK;
}

int32_t PlayerServiceStub::SetMediaDataSource(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    reply.WriteInt32(SetSource(object));
    return MSERR_OK;
}

int32_t PlayerServiceStub::SetFdSource(MessageParcel &data, MessageParcel &reply)
{
    int32_t fd = data.ReadFileDescriptor();
    int64_t offset = data.ReadInt64();
    int64_t size = data.ReadInt64();
    reply.WriteInt32(SetSource(fd, offset, size));
    (void)::close(fd);
    return MSERR_OK;
}

int32_t PlayerServiceStub::Play(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Play());
    return MSERR_OK;
}

int32_t PlayerServiceStub::Prepare(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Prepare());
    return MSERR_OK;
}

int32_t PlayerServiceStub::PrepareAsync(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(PrepareAsync());
    return MSERR_OK;
}

int32_t PlayerServiceStub::Pause(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Pause());
    return MSERR_OK;
}

int32_t PlayerServiceStub::Stop(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Stop());
    return MSERR_OK;
}

int32_t PlayerServiceStub::Reset(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Reset());
    return MSERR_OK;
}

int32_t PlayerServiceStub::Release(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Release());
    return MSERR_OK;
}

int32_t PlayerServiceStub::SetVolume(MessageParcel &data, MessageParcel &reply)
{
    float leftVolume = data.ReadFloat();
    float rightVolume = data.ReadFloat();
    reply.WriteInt32(SetVolume(leftVolume, rightVolume));
    return MSERR_OK;
}

int32_t PlayerServiceStub::Seek(MessageParcel &data, MessageParcel &reply)
{
    int32_t mSeconds = data.ReadInt32();
    int32_t mode = data.ReadInt32();
    reply.WriteInt32(Seek(mSeconds, static_cast<PlayerSeekMode>(mode)));
    return MSERR_OK;
}

int32_t PlayerServiceStub::GetCurrentTime(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t currentTime = -1;
    int32_t ret = GetCurrentTime(currentTime);
    reply.WriteInt32(currentTime);
    reply.WriteInt32(ret);
    return MSERR_OK;
}

int32_t PlayerServiceStub::GetVideoTrackInfo(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    std::vector<Format> videoTrack;
    int32_t ret = GetVideoTrackInfo(videoTrack);
    reply.WriteInt32(static_cast<int32_t>(videoTrack.size()));
    for (auto iter = videoTrack.begin(); iter != videoTrack.end(); iter++) {
        (void)MediaParcel::Marshalling(reply, *iter);
    }
    reply.WriteInt32(ret);

    return MSERR_OK;
}

int32_t PlayerServiceStub::GetAudioTrackInfo(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    std::vector<Format> audioTrack;
    int32_t ret = GetAudioTrackInfo(audioTrack);
    reply.WriteInt32(static_cast<int32_t>(audioTrack.size()));
    for (auto iter = audioTrack.begin(); iter != audioTrack.end(); iter++) {
        (void)MediaParcel::Marshalling(reply, *iter);
    }
    reply.WriteInt32(ret);

    return MSERR_OK;
}

int32_t PlayerServiceStub::GetVideoWidth(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t witdh = GetVideoWidth();
    reply.WriteInt32(witdh);

    return MSERR_OK;
}

int32_t PlayerServiceStub::GetVideoHeight(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t height = GetVideoHeight();
    reply.WriteInt32(height);

    return MSERR_OK;
}

int32_t PlayerServiceStub::GetDuration(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t duration = -1;
    int32_t ret = GetDuration(duration);
    reply.WriteInt32(duration);
    reply.WriteInt32(ret);
    return MSERR_OK;
}

int32_t PlayerServiceStub::SetPlaybackSpeed(MessageParcel &data, MessageParcel &reply)
{
    int32_t mode = data.ReadInt32();
    reply.WriteInt32(SetPlaybackSpeed(static_cast<PlaybackRateMode>(mode)));
    return MSERR_OK;
}

int32_t PlayerServiceStub::GetPlaybackSpeed(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    PlaybackRateMode mode = SPEED_FORWARD_1_00_X;
    int32_t ret = GetPlaybackSpeed(mode);
    reply.WriteInt32(mode);
    reply.WriteInt32(ret);
    return MSERR_OK;
}

int32_t PlayerServiceStub::SelectBitRate(MessageParcel &data, MessageParcel &reply)
{
    int32_t bitrate = data.ReadInt32();
    reply.WriteInt32(SelectBitRate(static_cast<uint32_t>(bitrate)));
    return MSERR_OK;
}

#ifdef SUPPORT_VIDEO
int32_t PlayerServiceStub::SetVideoSurface(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_NO_MEMORY, "object is nullptr");

    sptr<IBufferProducer> producer = iface_cast<IBufferProducer>(object);
    CHECK_AND_RETURN_RET_LOG(producer != nullptr, MSERR_NO_MEMORY, "failed to convert object to producer");

    sptr<Surface> surface = Surface::CreateSurfaceAsProducer(producer);
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, MSERR_NO_MEMORY, "failed to create surface");

    std::string format = data.ReadString();
    MEDIA_LOGI("surfaceFormat is %{public}s!", format.c_str());
    (void)surface->SetUserData("SURFACE_FORMAT", format);
    reply.WriteInt32(SetVideoSurface(surface));
    return MSERR_OK;
}
#endif

int32_t PlayerServiceStub::IsPlaying(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteBool(IsPlaying());
    return MSERR_OK;
}

int32_t PlayerServiceStub::IsLooping(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteBool(IsLooping());
    return MSERR_OK;
}

int32_t PlayerServiceStub::SetLooping(MessageParcel &data, MessageParcel &reply)
{
    bool loop = data.ReadBool();
    reply.WriteInt32(SetLooping(loop));
    return MSERR_OK;
}

int32_t PlayerServiceStub::SetParameter(MessageParcel &data, MessageParcel &reply)
{
    Format param;
    (void)MediaParcel::Unmarshalling(data, param);

    reply.WriteInt32(SetParameter(param));

    return MSERR_OK;
}

int32_t PlayerServiceStub::DestroyStub(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(DestroyStub());
    return MSERR_OK;
}

int32_t PlayerServiceStub::SetPlayerCallback(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(SetPlayerCallback());
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
