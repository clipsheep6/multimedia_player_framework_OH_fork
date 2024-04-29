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
#ifdef SUPPORT_DRM
#include "key_session_service_proxy.h"
#endif
#include "media_log.h"
#include "media_errors.h"
#include "media_parcel.h"
#include "parameter.h"
#include "media_dfx.h"
#include "player_xcollie.h"
#ifdef SUPPORT_AVSESSION
#include "avsession_background.h"
#endif

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerServiceStub"};
    constexpr uint32_t MAX_MAP_SIZE = 100;
}

namespace OHOS {
namespace Media {
sptr<PlayerServiceStub> PlayerServiceStub::Create()
{
    sptr<PlayerServiceStub> playerStub = new(std::nothrow) PlayerServiceStub();
    CHECK_AND_RETURN_RET_LOG(playerStub != nullptr, nullptr, "failed to new PlayerServiceStub");

    int32_t ret = playerStub->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to player stub init");
    StatisticEventWriteBundleName("create", "PlayerServiceStub");
    return playerStub;
}

PlayerServiceStub::PlayerServiceStub()
    : taskQue_("PlayerRequest")
{
    (void)taskQue_.Start();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerServiceStub::~PlayerServiceStub()
{
    (void)CancellationMonitor(appPid_);
    if (playerServer_ != nullptr) {
        auto task = std::make_shared<TaskHandler<void>>([&, this] {
            (void)playerServer_->Release();
            playerServer_ = nullptr;
        });
        (void)taskQue_.EnqueueTask(task);
        (void)task->GetResult();
    }
    (void)taskQue_.Stop();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void PlayerServiceStub::SetPlayerFuncs()
{
    playerFuncs_[SET_LISTENER_OBJ] = { &PlayerServiceStub::SetListenerObject, "Player::SetListenerObject" };
    playerFuncs_[SET_SOURCE] = { &PlayerServiceStub::SetSource, "Player::SetSource" };
    playerFuncs_[SET_MEDIA_DATA_SRC_OBJ] = { &PlayerServiceStub::SetMediaDataSource, "Player::SetMediaDataSource" };
    playerFuncs_[SET_FD_SOURCE] = { &PlayerServiceStub::SetFdSource, "Player::SetFdSource" };
    playerFuncs_[ADD_SUB_SOURCE] = { &PlayerServiceStub::AddSubSource, "Player::AddSubSource" };
    playerFuncs_[ADD_SUB_FD_SOURCE] = { &PlayerServiceStub::AddSubFdSource, "Player::AddSubFdSource" };
    playerFuncs_[PLAY] = { &PlayerServiceStub::Play, "Player::Play" };
    playerFuncs_[PREPARE] = { &PlayerServiceStub::Prepare, "Player::Prepare" };
    playerFuncs_[SET_RENDER_FIRST_FRAME] = { &PlayerServiceStub::SetRenderFirstFrame, "Player::SetRenderFirstFrame" };
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
    playerFuncs_[SET_MEDIA_SOURCE] = { &PlayerServiceStub::SetMediaSource, "Player::SetMediaSource" };
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
    playerFuncs_[GET_SUBTITLE_TRACK_INFO] = {
        &PlayerServiceStub::GetSubtitleTrackInfo, "Player::GetSubtitleTrackInfo"
    };
    playerFuncs_[GET_VIDEO_WIDTH] = { &PlayerServiceStub::GetVideoWidth, "Player::GetVideoWidth" };
    playerFuncs_[GET_VIDEO_HEIGHT] = { &PlayerServiceStub::GetVideoHeight, "Player::GetVideoHeight" };
    playerFuncs_[SELECT_BIT_RATE] = { &PlayerServiceStub::SelectBitRate, "Player::SelectBitRate" };
    playerFuncs_[SELECT_TRACK] = { &PlayerServiceStub::SelectTrack, "Player::SelectTrack" };
    playerFuncs_[DESELECT_TRACK] = { &PlayerServiceStub::DeselectTrack, "Player::DeselectTrack" };
    playerFuncs_[GET_CURRENT_TRACK] = { &PlayerServiceStub::GetCurrentTrack, "Player::GetCurrentTrack" };
    playerFuncs_[SET_DECRYPT_CONFIG] = { &PlayerServiceStub::SetDecryptConfig, "Player::SetDecryptConfig" };

    (void)RegisterMonitor(appPid_);
}

int32_t PlayerServiceStub::Init()
{
    if (playerServer_ == nullptr) {
        playerServer_ = PlayerServer::Create();
    }
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "failed to create PlayerServer");

    appUid_ = IPCSkeleton::GetCallingUid();
    appPid_ = IPCSkeleton::GetCallingPid();
    SetPlayerFuncs();
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
    MediaTrace trace("binder::OnRemoteRequest");
    auto remoteDescriptor = data.ReadInterfaceToken();
    CHECK_AND_RETURN_RET_LOG(PlayerServiceStub::GetDescriptor() == remoteDescriptor,
        MSERR_INVALID_OPERATION, "Invalid descriptor");

    auto itFunc = playerFuncs_.find(code);
    if (itFunc != playerFuncs_.end()) {
        auto memberFunc = itFunc->second.first;
        auto funcName = itFunc->second.second;
        if (funcName.compare("Player::SetVolume") == 0) {
            MEDIA_LOGD("0x%{public}06" PRIXPTR " Stub: OnRemoteRequest task: %{public}s is received",
                FAKE_POINTER(this), funcName.c_str());
        } else {
            MEDIA_LOGI("0x%{public}06" PRIXPTR " Stub: OnRemoteRequest task: %{public}s is received",
                FAKE_POINTER(this), funcName.c_str());
        }
        if (memberFunc != nullptr) {
            auto task = std::make_shared<TaskHandler<int>>([&, this] {
                (void)IpcRecovery(false);
                int32_t ret = -1;
                ret = (this->*memberFunc)(data, reply);
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

int32_t PlayerServiceStub::AddSubSource(const std::string &url)
{
    MediaTrace trace("binder::AddSubSource(url)");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->AddSubSource(url);
}

int32_t PlayerServiceStub::AddSubSource(int32_t fd, int64_t offset, int64_t size)
{
    MediaTrace trace("binder::AddSubSource(fd)");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->AddSubSource(fd, offset, size);
}

int32_t PlayerServiceStub::Play()
{
    MediaTrace trace("binder::Play");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
#ifdef SUPPORT_AVSESSION
    AVsessionBackground::Instance().AddListener(playerServer_, appUid_);
#endif
    return playerServer_->Play();
}

int32_t PlayerServiceStub::Prepare()
{
    MediaTrace trace("binder::Prepare");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->Prepare();
}

int32_t PlayerServiceStub::SetRenderFirstFrame(bool display)
{
    MediaTrace trace("Stub::SetRenderFirstFrame");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->SetRenderFirstFrame(display);
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

int32_t PlayerServiceStub::GetSubtitleTrackInfo(std::vector<Format> &subtitleTrack)
{
    MediaTrace trace("binder::GetSubtitleTrackInfo");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->GetSubtitleTrackInfo(subtitleTrack);
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

int32_t PlayerServiceStub::SetMediaSource(const std::shared_ptr<AVMediaSource> &mediaSource, AVPlayStrategy strategy)
{
    CHECK_AND_RETURN_RET_LOG(mediaSource != nullptr, MSERR_INVALID_VAL, "mediaSource is nullptr");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->SetMediaSource(mediaSource, strategy);
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

int32_t PlayerServiceStub::SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySessionProxy,
    bool svp)
{
#ifdef SUPPORT_DRM
    MediaTrace trace("binder::SetDecryptConfig");
    MEDIA_LOGI("PlayerServiceStub SetDecryptConfig");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->SetDecryptConfig(keySessionProxy, svp);
#else
    (void)keySessionProxy;
    (void)svp;
    return 0;
#endif
}

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

int32_t PlayerServiceStub::DoIpcAbnormality()
{
    MEDIA_LOGI("Enter DoIpcAbnormality.");
    auto task = std::make_shared<TaskHandler<int>>([&, this] {
        MEDIA_LOGI("DoIpcAbnormality.");
        CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, static_cast<int>(MSERR_NO_MEMORY),
            "player server is nullptr");
        CHECK_AND_RETURN_RET_LOG(IsPlaying(), static_cast<int>(MSERR_INVALID_OPERATION), "Not in playback state");
        auto playerServer = std::static_pointer_cast<PlayerServer>(playerServer_);
        int32_t ret = playerServer->BackGroundChangeState(PlayerStates::PLAYER_PAUSED, false);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "DoIpcAbnormality End.");
        SetIpcAlarmedFlag();
        MEDIA_LOGI("DoIpcAbnormality End.");
        return ret;
    });
    (void)taskQue_.EnqueueTask(task);
    return MSERR_OK;
}

int32_t PlayerServiceStub::DoIpcRecovery(bool fromMonitor)
{
    MEDIA_LOGI("Enter DoIpcRecovery %{public}d.", fromMonitor);
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    if (fromMonitor) {
        auto task = std::make_shared<TaskHandler<int>>([&, this] {
            MEDIA_LOGI("DoIpcRecovery.");
            auto playerServer = std::static_pointer_cast<PlayerServer>(playerServer_);
            int32_t ret = playerServer->BackGroundChangeState(PlayerStates::PLAYER_STARTED, false);
            CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK || ret == MSERR_INVALID_OPERATION, ret, "Failed to ChangeState");
            UnSetIpcAlarmedFlag();
            MEDIA_LOGI("DoIpcRecovery End.");
            return ret;
        });
        (void)taskQue_.EnqueueTask(task);
    } else {
        auto playerServer = std::static_pointer_cast<PlayerServer>(playerServer_);
        int32_t ret = playerServer->BackGroundChangeState(PlayerStates::PLAYER_STARTED, false);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK || ret == MSERR_INVALID_OPERATION, ret, "Failed to ChangeState");
        UnSetIpcAlarmedFlag();
    }
    return MSERR_OK;
}

int32_t PlayerServiceStub::SelectTrack(int32_t index)
{
    MediaTrace trace("PlayerServiceStub::SelectTrack");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->SelectTrack(index);
}

int32_t PlayerServiceStub::DeselectTrack(int32_t index)
{
    MediaTrace trace("PlayerServiceStub::DeselectTrack");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->DeselectTrack(index);
}

int32_t PlayerServiceStub::GetCurrentTrack(int32_t trackType, int32_t &index)
{
    MediaTrace trace("PlayerServiceStub::GetCurrentTrack");
    CHECK_AND_RETURN_RET_LOG(playerServer_ != nullptr, MSERR_NO_MEMORY, "player server is nullptr");
    return playerServer_->GetCurrentTrack(trackType, index);
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

int32_t PlayerServiceStub::AddSubSource(MessageParcel &data, MessageParcel &reply)
{
    std::string url = data.ReadString();
    reply.WriteInt32(AddSubSource(url));
    return MSERR_OK;
}

int32_t PlayerServiceStub::AddSubFdSource(MessageParcel &data, MessageParcel &reply)
{
    int32_t fd = data.ReadFileDescriptor();
    int64_t offset = data.ReadInt64();
    int64_t size = data.ReadInt64();
    reply.WriteInt32(AddSubSource(fd, offset, size));
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

int32_t PlayerServiceStub::SetRenderFirstFrame(MessageParcel &data, MessageParcel &reply)
{
    bool display = data.ReadBool();
    reply.WriteInt32(SetRenderFirstFrame(display));
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

int32_t PlayerServiceStub::GetSubtitleTrackInfo(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    std::vector<Format> subtitleTrack;
    int32_t ret = GetSubtitleTrackInfo(subtitleTrack);
    reply.WriteInt32(static_cast<int32_t>(subtitleTrack.size()));
    for (auto iter = subtitleTrack.begin(); iter != subtitleTrack.end(); iter++) {
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

int32_t PlayerServiceStub::SetMediaSource(MessageParcel &data, MessageParcel &reply)
{
    std::string url = data.ReadString();
    auto mapSize = data.ReadUint32();
    std::map<std::string, std::string> header;
    if (mapSize >= MAX_MAP_SIZE) {
        MEDIA_LOGI("Exceeded maximum table size limit");
        return MSERR_INVALID_OPERATION;
    }
    for (size_t i = 0; i < mapSize; i++) {
        auto kstr = data.ReadString();
        auto vstr = data.ReadString();
        header.emplace(kstr, vstr);
    }
    std::shared_ptr<AVMediaSource> meidaSource = std::make_shared<AVMediaSource>(url, header);
    struct AVPlayStrategy strategy;
    strategy.preferredWidth = data.ReadUint32();
    strategy.preferredHeight = data.ReadUint32();
    strategy.preferredBufferDuration = data.ReadUint32();
    strategy.preferredHdr = data.ReadBool();
    reply.WriteInt32(SetMediaSource(meidaSource, strategy));
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
    MEDIA_LOGI("0x%{public}06" PRIXPTR " surfaceFormat is %{public}s!", FAKE_POINTER(this), format.c_str());
    (void)surface->SetUserData("SURFACE_FORMAT", format);
    reply.WriteInt32(SetVideoSurface(surface));
    return MSERR_OK;
}
#endif

int32_t PlayerServiceStub::SetDecryptConfig(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_LOGI("PlayerServiceStub SetDecryptConfig");
#ifdef SUPPORT_DRM
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_NO_MEMORY, "KeySessionServiceProxy object is nullptr");
    bool svp = data.ReadBool();

    sptr<DrmStandard::MediaKeySessionServiceProxy> keySessionServiceProxy =
        iface_cast<DrmStandard::MediaKeySessionServiceProxy>(object);
    if (keySessionServiceProxy != nullptr) {
        MEDIA_LOGD("And it's count is: %{public}d", keySessionServiceProxy->GetSptrRefCount());
        reply.WriteInt32(SetDecryptConfig(keySessionServiceProxy, svp));
        return MSERR_OK;
    }
    MEDIA_LOGE("PlayerServiceStub keySessionServiceProxy is nullptr!");
    return MSERR_INVALID_VAL;
#else
    (void)data;
    (void)reply;
    return 0;
#endif
}

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

int32_t PlayerServiceStub::SelectTrack(MessageParcel &data, MessageParcel &reply)
{
    int32_t index = data.ReadInt32();
    reply.WriteInt32(SelectTrack(index));
    return MSERR_OK;
}

int32_t PlayerServiceStub::DeselectTrack(MessageParcel &data, MessageParcel &reply)
{
    int32_t index = data.ReadInt32();
    reply.WriteInt32(DeselectTrack(index));
    return MSERR_OK;
}

int32_t PlayerServiceStub::GetCurrentTrack(MessageParcel &data, MessageParcel &reply)
{
    int32_t trackType = data.ReadInt32();
    int32_t index = -1;
    int32_t ret = GetCurrentTrack(trackType, index);
    reply.WriteInt32(index);
    reply.WriteInt32(ret);
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
