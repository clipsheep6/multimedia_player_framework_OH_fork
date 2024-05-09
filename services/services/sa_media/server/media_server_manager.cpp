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

#include "media_server_manager.h"
#include <unordered_set>
#ifdef SUPPORT_RECORDER
#include "recorder_service_stub.h"
#include "recorder_profiles_service_stub.h"
#endif
#ifdef SUPPORT_PLAYER
#include "player_service_stub.h"
#ifdef PLAYER_USE_MEMORY_MANAGE
#include "player_service_stub_mem.h"
#endif
#endif
#ifdef SUPPORT_METADATA
#include "avmetadatahelper_service_stub.h"
#endif
#ifdef SUPPORT_SCREEN_CAPTURE
#include "screen_capture_service_stub.h"
#include "screen_capture_controller_stub.h"
#endif
#include "monitor_service_stub.h"
#include "media_log.h"
#include "media_errors.h"
#include "media_dfx.h"
#include "service_dump_manager.h"
#include "player_xcollie.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaServerManager"};
}

namespace OHOS {
namespace Media {
constexpr uint32_t SERVER_MAX_NUMBER = 16;
MediaServerManager &MediaServerManager::GetInstance()
{
    static MediaServerManager instance;
    return instance;
}

int32_t WriteInfo(int32_t fd, std::string &dumpString, std::vector<Dumper> dumpers, bool needDetail)
{
    int32_t i = 0;
    for (auto iter : dumpers) {
        dumpString += "-----Instance #" + std::to_string(i) + ": ";
        dumpString += "pid = ";
        dumpString += std::to_string(iter.pid_);
        dumpString += " uid = ";
        dumpString += std::to_string(iter.uid_);
        dumpString += "-----\n";
        if (fd != -1) {
            write(fd, dumpString.c_str(), dumpString.size());
            dumpString.clear();
        }
        i++;
        if (needDetail && iter.entry_(fd) != MSERR_OK) {
            return OHOS::INVALID_OPERATION;
        }
    }
    if (fd != -1) {
        write(fd, dumpString.c_str(), dumpString.size());
    } else {
        MEDIA_LOGI("%{public}s", dumpString.c_str());
    }
    dumpString.clear();

    return OHOS::NO_ERROR;
}

int32_t MediaServerManager::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    std::string dumpString;
    std::unordered_set<std::u16string> argSets;
    for (decltype(args.size()) index = 0; index < args.size(); ++index) {
        argSets.insert(args[index]);
    }

    dumpString += "------------------PlayerServer------------------\n";
    auto ret = WriteInfo(fd, dumpString, dumperTbl_[StubType::PLAYER],
        argSets.find(u"player") != argSets.end());
    CHECK_AND_RETURN_RET_LOG(ret == NO_ERROR,
        OHOS::INVALID_OPERATION, "Failed to write PlayerServer information");

    dumpString += "------------------RecorderServer------------------\n";
    ret =  WriteInfo(fd, dumpString, dumperTbl_[StubType::RECORDER],
        argSets.find(u"recorder") != argSets.end());
    CHECK_AND_RETURN_RET_LOG(ret == NO_ERROR,
        OHOS::INVALID_OPERATION, "Failed to write RecorderServer information");

    dumpString += "------------------CodecServer------------------\n";
    ret = WriteInfo(fd, dumpString, dumperTbl_[StubType::AVCODEC],
        argSets.find(u"codec") != argSets.end());
    CHECK_AND_RETURN_RET_LOG(ret == NO_ERROR,
        OHOS::INVALID_OPERATION, "Failed to write CodecServer information");

    dumpString += "------------------AVMetaServer------------------\n";
    ret = WriteInfo(fd, dumpString, dumperTbl_[StubType::AVMETADATAHELPER], false);
    CHECK_AND_RETURN_RET_LOG(ret == NO_ERROR,
        OHOS::INVALID_OPERATION, "Failed to write AVMetaServer information");

    dumpString += "------------------ScreenCaptureServer------------------\n";
    ret = WriteInfo(fd, dumpString, dumperTbl_[StubType::SCREEN_CAPTURE],
        argSets.find(u"screencapture") != argSets.end());
    CHECK_AND_RETURN_RET_LOG(ret == NO_ERROR,
        OHOS::INVALID_OPERATION, "Failed to write ScreenCapture information");

    ret = ServiceDumpManager::GetInstance().Dump(fd, argSets);
    CHECK_AND_RETURN_RET_LOG(ret == NO_ERROR,
        OHOS::INVALID_OPERATION, "Failed to write dfx dump information");

    ret = PlayerXCollie::GetInstance().Dump(fd);
    CHECK_AND_RETURN_RET_LOG(ret == NO_ERROR,
        OHOS::INVALID_OPERATION, "Failed to write xcollie dump information");

    ret = MonitorServiceStub::GetInstance()->DumpInfo(fd,
        argSets.find(u"monitor") != argSets.end());
    CHECK_AND_RETURN_RET_LOG(ret == NO_ERROR,
        OHOS::INVALID_OPERATION, "Failed to write monitor dump information");
    return OHOS::NO_ERROR;
}

MediaServerManager::MediaServerManager()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaServerManager::~MediaServerManager()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

sptr<IRemoteObject> MediaServerManager::CreateStubObject(StubType type)
{
    std::lock_guard<std::mutex> lock(mutex_);
    switch (type) {
#ifdef SUPPORT_RECORDER
        case RECORDER:
            return CreateRecorderStubObject();
        case RECORDERPROFILES:
            return CreateRecorderProfilesStubObject();
#endif
#ifdef SUPPORT_PLAYER
        case PLAYER:
            return CreatePlayerStubObject();
#endif
#ifdef SUPPORT_METADATA
        case AVMETADATAHELPER:
            return CreateAVMetadataHelperStubObject();
#endif
#ifdef SUPPORT_SCREEN_CAPTURE
        case SCREEN_CAPTURE:
            return CreateScreenCaptureStubObject();
        case SCREEN_CAPTURE_CONTROLLER:
            return CreateScreenCaptureControllerStubObject();
#endif
        case MONITOR:
            return GetMonitorStubObject();
        default:
            MEDIA_LOGE("default case, media server manager failed");
            return nullptr;
    }
}

#ifdef SUPPORT_PLAYER
sptr<IRemoteObject> MediaServerManager::CreatePlayerStubObject()
{
#ifdef PLAYER_USE_MEMORY_MANAGE
    sptr<PlayerServiceStub> playerStub = PlayerServiceStubMem::Create();
#else
    sptr<PlayerServiceStub> playerStub = PlayerServiceStub::Create();
#endif
    CHECK_AND_RETURN_RET_LOG(playerStub != nullptr, nullptr, "failed to create PlayerServiceStub");

    sptr<IRemoteObject> object = playerStub->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to create PlayerServiceStub");

    pid_t pid = IPCSkeleton::GetCallingPid();
    playerStubMap_[object] = pid;

    Dumper dumper;
    dumper.entry_ = [player = playerStub](int32_t fd) -> int32_t {
        return player->DumpInfo(fd);
    };
    dumper.pid_ = pid;
    dumper.uid_ = IPCSkeleton::GetCallingUid();
    dumper.remoteObject_ = object;
    dumperTbl_[StubType::PLAYER].emplace_back(dumper);
    MEDIA_LOGD("The number of player services(%{public}zu) pid(%{public}d).",
        playerStubMap_.size(), pid);
    MediaTrace::CounterTrace("The number of player", playerStubMap_.size());
    (void)Dump(-1, std::vector<std::u16string>());
    return object;
}
#endif

#ifdef SUPPORT_RECORDER
sptr<IRemoteObject> MediaServerManager::CreateRecorderStubObject()
{
    sptr<RecorderServiceStub> recorderStub = RecorderServiceStub::Create();
    CHECK_AND_RETURN_RET_LOG(recorderStub != nullptr, nullptr, "failed to create RecorderServiceStub");

    sptr<IRemoteObject> object = recorderStub->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to create RecorderServiceStub");

    pid_t pid = IPCSkeleton::GetCallingPid();
    recorderStubMap_[object] = pid;

    Dumper dumper;
    dumper.entry_ = [recorder = recorderStub](int32_t fd) -> int32_t {
        return recorder->DumpInfo(fd);
    };
    dumper.pid_ = pid;
    dumper.uid_ = IPCSkeleton::GetCallingUid();
    dumper.remoteObject_ = object;
    dumperTbl_[StubType::RECORDER].emplace_back(dumper);
    MEDIA_LOGD("The number of recorder services(%{public}zu) pid(%{public}d).",
        recorderStubMap_.size(), pid);
    (void)Dump(-1, std::vector<std::u16string>());
    return object;
}

sptr<IRemoteObject> MediaServerManager::CreateRecorderProfilesStubObject()
{
    CHECK_AND_RETURN_RET_LOG(recorderProfilesStubMap_.size() < SERVER_MAX_NUMBER,
        nullptr, "The number of recorder_profiles services(%{public}zu) has reached the upper limit."
        "Please release the applied resources.", recorderProfilesStubMap_.size());

    sptr<RecorderProfilesServiceStub> recorderProfilesStub = RecorderProfilesServiceStub::Create();
    CHECK_AND_RETURN_RET_LOG(recorderProfilesStub != nullptr, nullptr,
        "failed to create recorderProfilesStub");

    sptr<IRemoteObject> object = recorderProfilesStub->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to create recorderProfilesStub");

    pid_t pid = IPCSkeleton::GetCallingPid();
    recorderProfilesStubMap_[object] = pid;
    MEDIA_LOGD("The number of recorder_profiles services(%{public}zu).", recorderProfilesStubMap_.size());
    return object;
}
#endif

#ifdef SUPPORT_METADATA
sptr<IRemoteObject> MediaServerManager::CreateAVMetadataHelperStubObject()
{
    sptr<AVMetadataHelperServiceStub> avMetadataHelperStub = AVMetadataHelperServiceStub::Create();
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperStub != nullptr, nullptr,
        "failed to create AVMetadataHelperServiceStub");

    sptr<IRemoteObject> object = avMetadataHelperStub->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr,
        "failed to create AVMetadataHelperServiceStub");

    pid_t pid = IPCSkeleton::GetCallingPid();
    avMetadataHelperStubMap_[object] = pid;

    Dumper dumper;
    dumper.pid_ = pid;
    dumper.uid_ = IPCSkeleton::GetCallingUid();
    dumper.remoteObject_ = object;
    dumperTbl_[StubType::AVMETADATAHELPER].emplace_back(dumper);

    MEDIA_LOGD("The number of avmetadatahelper services(%{public}zu) pid(%{public}d).",
        avMetadataHelperStubMap_.size(), pid);
    (void)Dump(-1, std::vector<std::u16string>());
    return object;
}
#endif

#ifdef SUPPORT_SCREEN_CAPTURE
sptr<IRemoteObject> MediaServerManager::CreateScreenCaptureStubObject()
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureStubMap_.size() < SERVER_MAX_NUMBER,
        nullptr, "The number of screen capture services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", screenCaptureStubMap_.size());

    sptr<ScreenCaptureServiceStub> screenCaptureStub = ScreenCaptureServiceStub::Create();
    CHECK_AND_RETURN_RET_LOG(screenCaptureStub != nullptr, nullptr,
        "failed to create ScreenCaptureServiceStub");

    sptr<IRemoteObject> object = screenCaptureStub->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to create ScreenCaptureServiceStub");

    pid_t pid = IPCSkeleton::GetCallingPid();
    screenCaptureStubMap_[object] = pid;

    Dumper dumper;
    dumper.pid_ = pid;
    dumper.uid_ = IPCSkeleton::GetCallingUid();
    dumper.remoteObject_ = object;
    dumperTbl_[StubType::SCREEN_CAPTURE].emplace_back(dumper);
    MEDIA_LOGD("The number of screen capture services(%{public}zu).", screenCaptureStubMap_.size());
    (void)Dump(-1, std::vector<std::u16string>());
    return object;
}

sptr<IRemoteObject> MediaServerManager::CreateScreenCaptureControllerStubObject()
{
    MEDIA_LOGI("MediaServerManager::CreateScreenCaptureControllerStubObject() start");
    CHECK_AND_RETURN_RET_LOG(screenCaptureControllerStubMap_.size() < SERVER_MAX_NUMBER,
        nullptr, "The number of screen capture controller services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", screenCaptureControllerStubMap_.size());

    sptr<ScreenCaptureControllerStub> screenCaptureControllerStub = ScreenCaptureControllerStub::Create();
    CHECK_AND_RETURN_RET_LOG(screenCaptureControllerStub != nullptr, nullptr,
        "failed to create ScreenCaptureControllerStub");

    sptr<IRemoteObject> object = screenCaptureControllerStub->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to create screenCaptureControllerStub");

    pid_t pid = IPCSkeleton::GetCallingPid();
    screenCaptureControllerStubMap_[object] = pid;
    MEDIA_LOGD("The number of screen capture services(%{public}zu).", screenCaptureControllerStubMap_.size());
    MEDIA_LOGI("MediaServerManager::CreateScreenCaptureControllerStubObject() end");
    return object;
}
#endif

sptr<IRemoteObject> MediaServerManager::GetMonitorStubObject()
{
    sptr<MonitorServiceStub> monitorStub = MonitorServiceStub::GetInstance();
    CHECK_AND_RETURN_RET_LOG(monitorStub != nullptr, nullptr, "failed to create MonitorServiceStub");
    sptr<IRemoteObject> object = monitorStub->AsObject();
    return object;
}

void MediaServerManager::DestroyAVCodecStub(StubType type, sptr<IRemoteObject> object, pid_t pid)
{
    switch (type) {
        case AVCODEC: {
            for (auto it = avCodecStubMap_.begin(); it != avCodecStubMap_.end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destroy avcodec stub services(%{public}zu) pid(%{public}d).",
                        avCodecStubMap_.size(), pid);
                    (void)avCodecStubMap_.erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find avcodec object failed, pid(%{public}d).", pid);
            break;
        }
        case AVCODECLIST: {
            for (auto it = avCodecListStubMap_.begin(); it != avCodecListStubMap_.end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destroy avcodeclist stub services(%{public}zu) pid(%{public}d).",
                        avCodecListStubMap_.size(), pid);
                    (void)avCodecListStubMap_.erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find avcodeclist object failed, pid(%{public}d).", pid);
            break;
        }
        default:
            break;
    }
}

void MediaServerManager::DestroyAVPlayerStub(StubType type, sptr<IRemoteObject> object, pid_t pid)
{
    switch (type) {
        case PLAYER: {
            for (auto it = playerStubMap_.begin(); it != playerStubMap_.end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destroy player stub services(%{public}zu) pid(%{public}d).",
                        playerStubMap_.size(), pid);
                    (void)playerStubMap_.erase(it);
                    MediaTrace::CounterTrace("The number of player", playerStubMap_.size());
                    return;
                }
            }
            MEDIA_LOGE("find player object failed, pid(%{public}d).", pid);
            break;
        }
        case AVMETADATAHELPER: {
            for (auto it = avMetadataHelperStubMap_.begin(); it != avMetadataHelperStubMap_.end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destroy avmetadatahelper stub services(%{public}zu) pid(%{public}d).",
                        avMetadataHelperStubMap_.size(), pid);
                    (void)avMetadataHelperStubMap_.erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find avmetadatahelper object failed, pid(%{public}d).", pid);
            break;
        }
        default:
            break;
    }
}

void MediaServerManager::DestroyAVRecorderStub(StubType type, sptr<IRemoteObject> object, pid_t pid)
{
    switch (type) {
        case RECORDER: {
            for (auto it = recorderStubMap_.begin(); it != recorderStubMap_.end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destroy recorder stub services(%{public}zu) pid(%{public}d).",
                        recorderStubMap_.size(), pid);
                    (void)recorderStubMap_.erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find recorder object failed, pid(%{public}d).", pid);
            break;
        }
        case RECORDERPROFILES: {
            for (auto it = recorderProfilesStubMap_.begin(); it != recorderProfilesStubMap_.end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destroy mediaprofile stub services(%{public}zu) pid(%{public}d).",
                        recorderProfilesStubMap_.size(), pid);
                    (void)recorderProfilesStubMap_.erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find mediaprofile object failed, pid(%{public}d).", pid);
            break;
        }
        default:
            break;
    }
}

void MediaServerManager::DestroyStubObject(StubType type, sptr<IRemoteObject> object)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pid_t pid = IPCSkeleton::GetCallingPid();
    DestroyDumper(type, object);
    switch (type) {
        case RECORDER:
        case RECORDERPROFILES:
            DestroyAVRecorderStub(type, object, pid);
            break;
        case PLAYER:
        case AVMETADATAHELPER:
            DestroyAVPlayerStub(type, object, pid);
            break;
        case AVCODEC:
        case AVCODECLIST:
            DestroyAVCodecStub(type, object, pid);
            break;
        case SCREEN_CAPTURE: {
            for (auto it = screenCaptureStubMap_.begin(); it != screenCaptureStubMap_.end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destroy screen capture stub services(%{public}zu) pid(%{public}d).",
                        screenCaptureStubMap_.size(), pid);
                    (void)screenCaptureStubMap_.erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find screen capture object failed, pid(%{public}d).", pid);
            break;
        }
        case SCREEN_CAPTURE_CONTROLLER: {
            for (auto it = screenCaptureControllerStubMap_.begin();
                it != screenCaptureControllerStubMap_.end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destroy screen capture controller stub services(%{public}zu) pid(%{public}d).",
                        screenCaptureControllerStubMap_.size(), pid);
                    (void)screenCaptureControllerStubMap_.erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find screen capture controller object failed, pid(%{public}d).", pid);
            break;
        }
        default: {
            MEDIA_LOGE("default case, media server manager failed, pid(%{public}d).", pid);
            break;
        }
    }
}

void MediaServerManager::DestroyAVCodecStubForPid(pid_t pid)
{
    MEDIA_LOGD("avcodec stub services(%{public}zu) pid(%{public}d).", avCodecStubMap_.size(), pid);
    for (auto itAvCodec = avCodecStubMap_.begin(); itAvCodec != avCodecStubMap_.end();) {
        if (itAvCodec->second == pid) {
            executor_.Commit(itAvCodec->first);
            itAvCodec = avCodecStubMap_.erase(itAvCodec);
        } else {
            itAvCodec++;
        }
    }
    MEDIA_LOGD("avcodec stub services(%{public}zu).", avCodecStubMap_.size());

    MEDIA_LOGD("avcodeclist stub services(%{public}zu) pid(%{public}d).", avCodecListStubMap_.size(), pid);
    for (auto itAvCodecList = avCodecListStubMap_.begin(); itAvCodecList != avCodecListStubMap_.end();) {
        if (itAvCodecList->second == pid) {
            executor_.Commit(itAvCodecList->first);
            itAvCodecList = avCodecListStubMap_.erase(itAvCodecList);
        } else {
            itAvCodecList++;
        }
    }
    MEDIA_LOGD("avcodeclist stub services(%{public}zu).", avCodecListStubMap_.size());
}

void MediaServerManager::DestroyAVPlayerStubForPid(pid_t pid)
{
    MEDIA_LOGD("player stub services(%{public}zu) pid(%{public}d).", playerStubMap_.size(), pid);
    for (auto itPlayer = playerStubMap_.begin(); itPlayer != playerStubMap_.end();) {
        if (itPlayer->second == pid) {
            executor_.Commit(itPlayer->first);
            itPlayer = playerStubMap_.erase(itPlayer);
        } else {
            itPlayer++;
        }
    }
    MEDIA_LOGD("player stub services(%{public}zu).", playerStubMap_.size());

    MEDIA_LOGD("avmetadatahelper stub services(%{public}zu) pid(%{public}d).", avMetadataHelperStubMap_.size(), pid);
    for (auto itAvMetadata = avMetadataHelperStubMap_.begin(); itAvMetadata != avMetadataHelperStubMap_.end();) {
        if (itAvMetadata->second == pid) {
            executor_.Commit(itAvMetadata->first);
            itAvMetadata = avMetadataHelperStubMap_.erase(itAvMetadata);
        } else {
            itAvMetadata++;
        }
    }
    MEDIA_LOGD("avmetadatahelper stub services(%{public}zu).", avMetadataHelperStubMap_.size());
}

void MediaServerManager::DestroyAVRecorderStubForPid(pid_t pid)
{
    MEDIA_LOGD("recorder stub services(%{public}zu) pid(%{public}d).", recorderStubMap_.size(), pid);
    for (auto itRecorder = recorderStubMap_.begin(); itRecorder != recorderStubMap_.end();) {
        if (itRecorder->second == pid) {
            executor_.Commit(itRecorder->first);
            itRecorder = recorderStubMap_.erase(itRecorder);
        } else {
            itRecorder++;
        }
    }
    MEDIA_LOGD("recorder stub services(%{public}zu).", recorderStubMap_.size());

    MEDIA_LOGD("mediaprofile stub services(%{public}zu) pid(%{public}d).", recorderProfilesStubMap_.size(), pid);
    for (auto itMediaProfile = recorderProfilesStubMap_.begin(); itMediaProfile != recorderProfilesStubMap_.end();) {
        if (itMediaProfile->second == pid) {
            executor_.Commit(itMediaProfile->first);
            itMediaProfile = recorderProfilesStubMap_.erase(itMediaProfile);
        } else {
            itMediaProfile++;
        }
    }
    MEDIA_LOGD("mediaprofile stub services(%{public}zu).", recorderProfilesStubMap_.size());
}

void MediaServerManager::DestroyStubObjectForPid(pid_t pid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    DestroyDumperForPid(pid);
    DestroyAVRecorderStubForPid(pid);
    DestroyAVPlayerStubForPid(pid);
    DestroyAVCodecStubForPid(pid);

    MEDIA_LOGD("screencapture stub services(%{public}zu) pid(%{public}d).", screenCaptureStubMap_.size(), pid);
    for (auto itScreenCapture = screenCaptureStubMap_.begin(); itScreenCapture != screenCaptureStubMap_.end();) {
        if (itScreenCapture->second == pid) {
            executor_.Commit(itScreenCapture->first);
            itScreenCapture = screenCaptureStubMap_.erase(itScreenCapture);
        } else {
            itScreenCapture++;
        }
    }
    MEDIA_LOGD("screencapture stub services(%{public}zu).", screenCaptureStubMap_.size());
    MonitorServiceStub::GetInstance()->OnClientDie(pid);
    executor_.Clear();
}

void MediaServerManager::DestroyDumper(StubType type, sptr<IRemoteObject> object)
{
    for (auto it = dumperTbl_[type].begin(); it != dumperTbl_[type].end(); it++) {
        if (it->remoteObject_ == object) {
            (void)dumperTbl_[type].erase(it);
            MEDIA_LOGD("MediaServerManager::DestroyDumper");
            (void)Dump(-1, std::vector<std::u16string>());
            return;
        }
    }
}

void MediaServerManager::DestroyDumperForPid(pid_t pid)
{
    for (auto &dumpers : dumperTbl_) {
        for (auto it = dumpers.second.begin(); it != dumpers.second.end();) {
            if (it->pid_ == pid) {
                it = dumpers.second.erase(it);
                MEDIA_LOGD("MediaServerManager::DestroyDumperForPid");
            } else {
                it++;
            }
        }
    }
    (void)Dump(-1, std::vector<std::u16string>());
}

void MediaServerManager::AsyncExecutor::Commit(sptr<IRemoteObject> obj)
{
    std::lock_guard<std::mutex> lock(listMutex_);
    freeList_.push_back(obj);
}

void MediaServerManager::AsyncExecutor::Clear()
{
    std::thread(&MediaServerManager::AsyncExecutor::HandleAsyncExecution, this).detach();
}

void MediaServerManager::AsyncExecutor::HandleAsyncExecution()
{
    std::list<sptr<IRemoteObject>> tempList;
    {
        std::lock_guard<std::mutex> lock(listMutex_);
        freeList_.swap(tempList);
    }
    tempList.clear();
}
} // namespace Media
} // namespace OHOS
