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
#include "av_common.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerServerMem"};
}

namespace OHOS {
namespace Media {
constexpr int32_t CONTINUE_RESET_MAX_NUM = 5;
constexpr double APP_BACK_GROUND_DESTROY_MEMERY_LAST_SET_TIME = 60.0;
constexpr double APP_FRONT_GROUND_DESTROY_MEMERY_LAST_SET_TIME = 120.0;
std::shared_ptr<IPlayerService> PlayerServerMem::Create()
{
    MEDIA_LOGI("Create new PlayerServerMem");
    std::shared_ptr<PlayerServerMem> playerServerMem = std::make_shared<PlayerServerMem>();
    CHECK_AND_RETURN_RET_LOG(playerServerMem != nullptr, nullptr, "failed to new PlayerServerMem");

    (void)playerServerMem->Init();
    return playerServerMem;
}

int32_t PlayerServerMem::Init()
{
    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::Init();
}

PlayerServerMem::PlayerServerMem()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerServerMem::~PlayerServerMem()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t PlayerServerMem::SetSource(const std::string &url)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");
    
    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::SetSource(url);
}

int32_t PlayerServerMem::SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::SetSource(dataSrc);
}

int32_t PlayerServerMem::SetSource(int32_t fd, int64_t offset, int64_t size)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::SetSource(fd, offset, size);
}

int32_t PlayerServerMem::Play()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::Play();
}

int32_t PlayerServerMem::Prepare()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::Prepare();
}

int32_t PlayerServerMem::PrepareAsync()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::PrepareAsync();
}

int32_t PlayerServerMem::Pause()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::Pause();
}

int32_t PlayerServerMem::Stop()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::Stop();
}

int32_t PlayerServerMem::Reset()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::Reset();
}

int32_t PlayerServerMem::Release()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::Release();
}

int32_t PlayerServerMem::SetVolume(float leftVolume, float rightVolume)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::SetVolume(leftVolume, rightVolume);
}

int32_t PlayerServerMem::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::Seek(mSeconds, mode);
}

int32_t PlayerServerMem::SetPlaybackSpeed(PlaybackRateMode mode)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::SetPlaybackSpeed(mode);
}

#ifdef SUPPORT_VIDEO
int32_t PlayerServerMem::SetVideoSurface(sptr<Surface> surface)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    isAudioPlayer_ = false;
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::SetVideoSurface(surface);
}
#endif

int32_t PlayerServerMem::SetLooping(bool loop)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::SetLooping(loop);
}

int32_t PlayerServerMem::SetParameter(const Format &param)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::SetParameter(param);
}

int32_t PlayerServerMem::SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::SetPlayerCallback(callback);
}

int32_t PlayerServerMem::SelectBitRate(uint32_t bitRate)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::SelectBitRate(bitRate);
}

int32_t PlayerServerMem::SelectTrack(int32_t index)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::SelectTrack(index);
}

int32_t PlayerServerMem::DeselectTrack(int32_t index)
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    CHECK_AND_RETURN_RET_LOG(RecoverMemByUser() == MSERR_OK, MSERR_INVALID_OPERATION, "RecoverMemByUser fail");

    lastestUserSetTime_ = std::chrono::steady_clock::now();
    return PlayerServer::DeselectTrack(index);
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

int32_t PlayerServerMem::ReleaseMemByManage()
{
    if (!isReleaseMemByManage_) {
        MEDIA_LOGI("enter");
        int32_t ret = PlayerServer::FreeCodecBuffers();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "FreeCodecBuffers Fail");
        isReleaseMemByManage_ = true;
        MEDIA_LOGI("exit");
    }
    return MSERR_OK;
}

int32_t PlayerServerMem::RecoverMemByUser()
{
    if (isReleaseMemByManage_) {
        MEDIA_LOGI("enter");
        int32_t ret = PlayerServer::RecoverCodecBuffers();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "RecoverCodecBuffers Fail");
        int32_t currentTime = 0;
        ret = PlayerServer::GetCurrentTime(currentTime);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "GetCurrentTime Fail");
        ret = PlayerServer::SeekCurrentTime(currentTime, SEEK_CLOSEST);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Seek Fail");
        isReleaseMemByManage_ = false;
        MEDIA_LOGI("exit");
    }
    return MSERR_OK;
}

void PlayerServerMem::ResetFrontGroundForMemManage()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (!IsPrepared()) {
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
        CHECK_AND_RETURN_LOG(ReleaseMemByManage() == MSERR_OK, "ReleaseMemByManage fail");
    }
}

void PlayerServerMem::ResetBackGroundForMemManage()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (IsPlaying()) {
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
    if (lastSetToNow.count() > APP_BACK_GROUND_DESTROY_MEMERY_LAST_SET_TIME) {
        CHECK_AND_RETURN_LOG(ReleaseMemByManage() == MSERR_OK, "ReleaseMemByManage fail");
    }
}

void PlayerServerMem::ResetMemmgrForMemManage()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    if (!(isAudioPlayer_ || IsPlaying())) {
        CHECK_AND_RETURN_LOG(ReleaseMemByManage() == MSERR_OK, "ReleaseMemByManage fail");
    }
}

void PlayerServerMem::RecoverByMemManage()
{
    std::lock_guard<std::recursive_mutex> lock(recMutex_);
    CHECK_AND_RETURN_LOG(RecoverMemByUser() == MSERR_OK, "RecoverMemByUser fail");
}
}
}