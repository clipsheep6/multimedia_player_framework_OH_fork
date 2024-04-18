/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "player_service_proxy_fuzzer.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include "directory_ex.h"
#include "media_parcel.h"

namespace OHOS {
namespace Media {
PlayerServiceProxyFuzzer::PlayerServiceProxyFuzzer(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardPlayerService>(impl)
{
    playerFuncs_[SET_LISTENER_OBJ] = &PlayerServiceProxyFuzzer::SetListenerObject;
    playerFuncs_[SET_SOURCE] = &PlayerServiceProxyFuzzer::SetSource;
    playerFuncs_[SET_MEDIA_DATA_SRC_OBJ] = &PlayerServiceProxyFuzzer::SetMediaDataSource;
    playerFuncs_[SET_FD_SOURCE] = &PlayerServiceProxyFuzzer::SetFdSource;
    playerFuncs_[ADD_SUB_SOURCE] = &PlayerServiceProxyFuzzer::AddSubSource;
    playerFuncs_[ADD_SUB_FD_SOURCE] = &PlayerServiceProxyFuzzer::AddSubFdSource;
    playerFuncs_[PLAY] = &PlayerServiceProxyFuzzer::Play;
    playerFuncs_[PREPARE] = &PlayerServiceProxyFuzzer::Prepare;
    playerFuncs_[PREPAREASYNC] = &PlayerServiceProxyFuzzer::PrepareAsync;
    playerFuncs_[PAUSE] = &PlayerServiceProxyFuzzer::Pause;
    playerFuncs_[STOP] = &PlayerServiceProxyFuzzer::Stop;
    playerFuncs_[RESET] = &PlayerServiceProxyFuzzer::Reset;
    playerFuncs_[RELEASE] = &PlayerServiceProxyFuzzer::Release;
    playerFuncs_[SET_VOLUME] = &PlayerServiceProxyFuzzer::SetVolume;
    playerFuncs_[SEEK] = &PlayerServiceProxyFuzzer::Seek;
    playerFuncs_[GET_CURRENT_TIME] = &PlayerServiceProxyFuzzer::GetCurrentTime;
    playerFuncs_[GET_DURATION] = &PlayerServiceProxyFuzzer::GetDuration;
    playerFuncs_[SET_PLAYERBACK_SPEED] = &PlayerServiceProxyFuzzer::SetPlaybackSpeed;
    playerFuncs_[GET_PLAYERBACK_SPEED] = &PlayerServiceProxyFuzzer::GetPlaybackSpeed;
    playerFuncs_[SET_VIDEO_SURFACE] = &PlayerServiceProxyFuzzer::SetVideoSurface;
    playerFuncs_[IS_PLAYING] = &PlayerServiceProxyFuzzer::IsPlaying;
    playerFuncs_[IS_LOOPING] = &PlayerServiceProxyFuzzer::IsLooping;
    playerFuncs_[SET_LOOPING] = &PlayerServiceProxyFuzzer::SetLooping;
    playerFuncs_[SET_RENDERER_DESC] = &PlayerServiceProxyFuzzer::SetParameter;
    playerFuncs_[DESTROY] = &PlayerServiceProxyFuzzer::DestroyStub;
    playerFuncs_[SET_CALLBACK] = &PlayerServiceProxyFuzzer::SetPlayerCallback;
    playerFuncs_[GET_VIDEO_TRACK_INFO] = &PlayerServiceProxyFuzzer::GetVideoTrackInfo;
    playerFuncs_[GET_AUDIO_TRACK_INFO] = &PlayerServiceProxyFuzzer::GetAudioTrackInfo;
    playerFuncs_[GET_SUBTITLE_TRACK_INFO] = &PlayerServiceProxyFuzzer::GetSubtitleTrackInfo;
    playerFuncs_[GET_VIDEO_WIDTH] = &PlayerServiceProxyFuzzer::GetVideoWidth;
    playerFuncs_[GET_VIDEO_HEIGHT] = &PlayerServiceProxyFuzzer::GetVideoHeight;
    playerFuncs_[SELECT_BIT_RATE] = &PlayerServiceProxyFuzzer::SelectBitRate;
    playerFuncs_[SELECT_TRACK] = &PlayerServiceProxyFuzzer::SelectTrack;
    playerFuncs_[DESELECT_TRACK] = &PlayerServiceProxyFuzzer::DeselectTrack;
    playerFuncs_[GET_CURRENT_TRACK] = &PlayerServiceProxyFuzzer::GetCurrentTrack;
}

sptr<PlayerServiceProxyFuzzer> PlayerServiceProxyFuzzer::Create()
{
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    sptr<IRemoteObject> object = samgr->GetSystemAbility(OHOS::PLAYER_DISTRIBUTED_SERVICE_ID);
    if (object == nullptr) {
        std::cout << "media object is nullptr." << std::endl;
        return nullptr;
    }
    sptr<IStandardMediaService> mediaProxy = iface_cast<IStandardMediaService>(object);
    if (mediaProxy == nullptr) {
        std::cout << "mediaProxy is nullptr." << std::endl;
        return nullptr;
    }
    sptr<IRemoteObject> listenerStub = new(std::nothrow) MediaListenerStubFuzzer();
    sptr<IRemoteObject> playerObject = mediaProxy->GetSubSystemAbility(
        IStandardMediaService::MediaSystemAbility::MEDIA_PLAYER, listenerStub);
    if (playerObject == nullptr) {
        std::cout << "playerObject is nullptr." << std::endl;
        return nullptr;
    }

    sptr<PlayerServiceProxyFuzzer> playerProxy = iface_cast<PlayerServiceProxyFuzzer>(playerObject);
    if (playerProxy == nullptr) {
        std::cout << "playerProxy is nullptr." << std::endl;
        return nullptr;
    }
    return playerProxy;
}

void PlayerServiceProxyFuzzer::SendRequest(int32_t code, uint8_t *inputData, size_t size, bool isFuzz)
{
    auto itFunc = playerFuncs_.find(code);
    if (itFunc != playerFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            (this->*memberFunc)(inputData, size, isFuzz);
        }
    }
}

int32_t PlayerServiceProxyFuzzer::SetListenerObject(uint8_t *inputData, size_t size, bool isFuzz)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "Failed to write descriptor!" << std::endl;
        return false;
    }
    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(SET_LISTENER_OBJ, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::SetSource(uint8_t *inputData, size_t size, bool isFuzz)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "Failed to write descriptor" << std::endl;
        return false;
    }
    std::string urlStr(reinterpret_cast<const char *>(inputData), size);
    (void)data.WriteString(urlStr);
    return SendRequest(SET_SOURCE, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::SetMediaDataSource(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "SetMediaDataSource:Failed to write descriptor!" << std::endl;
        return false;
    }
    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(SET_MEDIA_DATA_SRC_OBJ, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::SetFdSource(uint8_t *inputData, size_t size, bool isFuzz)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "SetFdSource:Failed to write descriptor!" << std::endl;
        return false;
    }

    int32_t fdValue;
    int64_t offsetValue;
    int64_t lengthValue;
    if (isFuzz) {
        fdValue = *reinterpret_cast<int32_t *>(inputData);
        lengthValue = *reinterpret_cast<int64_t *>(inputData);
        offsetValue = *reinterpret_cast<uint32_t *>(inputData) % *reinterpret_cast<int64_t *>(inputData);
        (void)data.WriteFileDescriptor(fdValue);
        (void)data.WriteInt64(offsetValue);
        (void)data.WriteInt64(lengthValue);
        return SendRequest(SET_FD_SOURCE, data, reply, option);
    } else {
        const std::string path = "/data/test/media/H264_AAC.mp4";
        fdValue = open(path.c_str(), O_RDONLY);
        offsetValue = 0;
        if (fdValue < 0) {
            std::cout << "Open file failed" << std::endl;
            (void)close(fdValue);
            return -1;
        }

        struct stat64 buffer;
        if (fstat64(fdValue, &buffer) != 0) {
            std::cout << "Get file state failed" << std::endl;
            (void)close(fdValue);
            return -1;
        }
        lengthValue = static_cast<int64_t>(buffer.st_size);
        (void)data.WriteFileDescriptor(fdValue);
        (void)data.WriteInt64(offsetValue);
        (void)data.WriteInt64(lengthValue);
        int32_t ret = SendRequest(SET_FD_SOURCE, data, reply, option);
        (void)close(fdValue);
        return ret;
    }
}

int32_t PlayerServiceProxyFuzzer::AddSubSource(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "AddSubSource:Failed to write descriptor!" << std::endl;
        return false;
    }
    std::string url(reinterpret_cast<const char *>(inputData), size);
    (void)data.WriteString(url);
    return SendRequest(ADD_SUB_SOURCE, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::AddSubFdSource(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "AddSubFdSource:Failed to write descriptor!" << std::endl;
        return false;
    }

    int32_t fdValue = *reinterpret_cast<int32_t *>(inputData);
    int64_t lengthValue = *reinterpret_cast<int64_t *>(inputData);
    int64_t offsetValue = *reinterpret_cast<uint32_t *>(inputData) % *reinterpret_cast<int64_t *>(inputData);
    (void)data.WriteFileDescriptor(fdValue);
    (void)data.WriteInt64(offsetValue);
    (void)data.WriteInt64(lengthValue);
    return SendRequest(ADD_SUB_FD_SOURCE, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::Play(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "Play:Failed to write descriptor!" << std::endl;
        return false;
    }
    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(PLAY, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::Prepare(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "Prepare:Failed to write descriptor!" << std::endl;
        return false;
    }
    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(PREPARE, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::PrepareAsync(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "PrepareAsync:Failed to write descriptor!" << std::endl;
        return false;
    }
    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(PREPAREASYNC, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::Pause(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "Pause:Failed to write descriptor!" << std::endl;
        return false;
    }
    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(PAUSE, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::Stop(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "Stop:Failed to write descriptor!" << std::endl;
        return false;
    }
    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(STOP, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::Reset(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "Reset:Failed to write descriptor!" << std::endl;
        return false;
    }
    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(RESET, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::Release(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "Release:Failed to write descriptor!" << std::endl;
        return false;
    }
    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(RELEASE, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::SetVolume(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "SetVolume:Failed to write descriptor!" << std::endl;
        return false;
    }

    (void)data.WriteFloat(*reinterpret_cast<int32_t *>(inputData));
    (void)data.WriteFloat(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(SET_VOLUME, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::Seek(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "Seek:Failed to write descriptor!" << std::endl;
        return false;
    }

    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(SEEK, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::GetCurrentTime(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "GetCurrentTime:Failed to write descriptor!" << std::endl;
        return false;
    }

    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(GET_CURRENT_TIME, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::GetDuration(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "GetDuration:Failed to write descriptor!" << std::endl;
        return false;
    }

    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(GET_DURATION, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::SetPlaybackSpeed(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "SetPlaybackSpeed:Failed to write descriptor!" << std::endl;
        return false;
    }

    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(SET_PLAYERBACK_SPEED, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::GetPlaybackSpeed(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "GetPlaybackSpeed:Failed to write descriptor!" << std::endl;
        return false;
    }

    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(GET_PLAYERBACK_SPEED, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::SetVideoSurface(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "SetVideoSurface:Failed to write descriptor!" << std::endl;
        return false;
    }

    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(SET_VIDEO_SURFACE, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::IsPlaying(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "IsPlaying:Failed to write descriptor!" << std::endl;
        return false;
    }

    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(IS_PLAYING, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::IsLooping(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "IsLooping:Failed to write descriptor!" << std::endl;
        return false;
    }

    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(IS_LOOPING, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::SetLooping(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "SetLooping:Failed to write descriptor!" << std::endl;
        return false;
    }
    (void)data.WriteBool(*reinterpret_cast<int32_t *>(inputData) % 2);  // 2 true or false
    return SendRequest(SET_LOOPING, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::SetParameter(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "Failed to write descriptor!" << std::endl;
        return false;
    }

    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(SET_RENDERER_DESC, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::DestroyStub(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "Failed to write descriptor!" << std::endl;
        return false;
    }

    if (isFuzz) {
        (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    }
    return SendRequest(DESTROY, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::SetPlayerCallback(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "SetPlayerCallback:Failed to write descriptor!" << std::endl;
        return false;
    }

    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(SET_CALLBACK, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::GetVideoTrackInfo(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "GetVideoTrackInfo:Failed to write descriptor!" << std::endl;
        return false;
    }

    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(GET_VIDEO_TRACK_INFO, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::GetAudioTrackInfo(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "GetAudioTrackInfo:Failed to write descriptor!" << std::endl;
        return false;
    }

    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(GET_AUDIO_TRACK_INFO, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::GetSubtitleTrackInfo(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "GetSubtitleTrackInfo:Failed to write descriptor!" << std::endl;
        return false;
    }

    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(GET_SUBTITLE_TRACK_INFO, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::GetVideoWidth(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "GetVideoWidth:Failed to write descriptor!" << std::endl;
        return false;
    }

    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(GET_VIDEO_WIDTH, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::GetVideoHeight(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "GetVideoHeight:Failed to write descriptor!" << std::endl;
        return false;
    }

    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(GET_VIDEO_HEIGHT, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::SelectBitRate(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "SelectBitRate:Failed to write descriptor!" << std::endl;
        return false;
    }

    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(SELECT_BIT_RATE, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::SelectTrack(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "SelectTrack:Failed to write descriptor!" << std::endl;
        return false;
    }

    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(SELECT_TRACK, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::DeselectTrack(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "DeselectTrack:Failed to write descriptor!" << std::endl;
        return false;
    }

    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(DESELECT_TRACK, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::GetCurrentTrack(uint8_t *inputData, size_t size, bool isFuzz)
{
    (void)size;
    (void)isFuzz;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(PlayerServiceProxyFuzzer::GetDescriptor());
    if (!token) {
        std::cout << "GetCurrentTrack:Failed to write descriptor!" << std::endl;
        return false;
    }

    (void)data.WriteInt32(*reinterpret_cast<int32_t *>(inputData));
    return SendRequest(GET_CURRENT_TRACK, data, reply, option);
}

int32_t PlayerServiceProxyFuzzer::SendRequest(uint32_t code,
    MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    return Remote()->SendRequest(code, data, reply, option);
}
}
}