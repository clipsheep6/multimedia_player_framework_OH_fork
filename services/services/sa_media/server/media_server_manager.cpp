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
#ifdef SUPPORT_METADATA
#include "avmetadatahelper_service_stub.h"
#endif
#ifdef SUPPORT_CODEC
#include "avcodeclist_service_stub.h"
#include "avcodec_service_stub.h"
#endif
#ifdef SUPPORT_MUXER
#include "avmuxer_service_stub.h"
#endif
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaServerManager"};
}

namespace OHOS {
namespace Media {
constexpr uint32_t SERVER_MAX_NUMBER = 16;
#ifdef SUPPORT_RECORDER
constexpr uint32_t RECORDER_MAX_NUMBER = 2;
#endif
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
    if (WriteInfo(fd, dumpString, dumperTbl_[StubType::PLAYER],
        argSets.find(u"player") != argSets.end()) != OHOS::NO_ERROR) {
        MEDIA_LOGW("Failed to write PlayerServer information");
        return OHOS::INVALID_OPERATION;
    }

    dumpString += "------------------RecorderServer------------------\n";
    if (WriteInfo(fd, dumpString, dumperTbl_[StubType::RECORDER],
        argSets.find(u"recorder") != argSets.end()) != OHOS::NO_ERROR) {
        MEDIA_LOGW("Failed to write RecorderServer information");
        return OHOS::INVALID_OPERATION;
    }

    dumpString += "------------------CodecServer------------------\n";
    if (WriteInfo(fd, dumpString, dumperTbl_[StubType::AVCODEC],
        argSets.find(u"codec") != argSets.end()) != OHOS::NO_ERROR) {
        MEDIA_LOGW("Failed to write CodecServer information");
        return OHOS::INVALID_OPERATION;
    }

    dumpString += "------------------AVMetaServer------------------\n";
    if (WriteInfo(fd, dumpString, dumperTbl_[StubType::AVMETADATAHELPER], false) != OHOS::NO_ERROR) {
        MEDIA_LOGW("Failed to write AVMetaServer information");
        return OHOS::INVALID_OPERATION;
    }

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
        case PLAYER: {
            return CreatePlayerStubObject();
        }
#ifdef SUPPORT_RECORDER
        case RECORDER: {
            return CreateRecorderStubObject();
        }
        case RECORDERPROFILES: {
            return CreateRecorderProfilesStubObject();
        }
#endif
#ifdef SUPPORT_METADATA
        case AVMETADATAHELPER: {
            return CreateAVMetadataHelperStubObject();
        }
#endif
#ifdef SUPPORT_CODEC
        case AVCODECLIST: {
            return CreateAVCodecListStubObject();
        }
        case AVCODEC: {
            return CreateAVCodecStubObject();
        }
#endif
#ifdef SUPPORT_MUXER
        case AVMUXER: {
            return CreateAVMuxerStubObject();
        }
#endif
        default: {
            MEDIA_LOGE("default case, media server manager failed");
            return nullptr;
        }
    }
}

sptr<IRemoteObject> MediaServerManager::CreatePlayerStubObject()
{
    if (stubMap_[StubType::PLAYER].size() >= SERVER_MAX_NUMBER) {
        MEDIA_LOGE("The number of player services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", stubMap_[StubType::PLAYER].size());
        return nullptr;
    }
    sptr<PlayerServiceStub> playerStub = PlayerServiceStub::Create();
    if (playerStub == nullptr) {
        MEDIA_LOGE("failed to create PlayerServiceStub");
        return nullptr;
    }

    sptr<IRemoteObject> object = playerStub->AsObject();
    if (object != nullptr) {
        pid_t pid = IPCSkeleton::GetCallingPid();
        stubMap_[StubType::PLAYER][object] = pid;

        Dumper dumper;
        dumper.entry_ = [player = playerStub](int32_t fd) -> int32_t {
            return player->DumpInfo(fd);
        };
        dumper.pid_ = pid;
        dumper.uid_ = IPCSkeleton::GetCallingUid();
        dumper.remoteObject_ = object;
        dumperTbl_[StubType::PLAYER].emplace_back(dumper);
        MEDIA_LOGD("The number of player services(%{public}zu) pid(%{public}d).",
            stubMap_[StubType::PLAYER].size(), pid);
        if (Dump(-1, std::vector<std::u16string>()) != OHOS::NO_ERROR) {
            MEDIA_LOGW("failed to call InstanceDump");
        }
    }
    return object;
}

#ifdef SUPPORT_RECORDER
sptr<IRemoteObject> MediaServerManager::CreateRecorderStubObject()
{
    if (stubMap_[StubType::RECORDER].size() >= RECORDER_MAX_NUMBER) {
        MEDIA_LOGE("The number of recorder services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", stubMap_[StubType::RECORDER].size());
        return nullptr;
    }
    sptr<RecorderServiceStub> recorderStub = RecorderServiceStub::Create();
    if (recorderStub == nullptr) {
        MEDIA_LOGE("failed to create RecorderServiceStub");
        return nullptr;
    }
    sptr<IRemoteObject> object = recorderStub->AsObject();
    if (object != nullptr) {
        pid_t pid = IPCSkeleton::GetCallingPid();
        stubMap_[StubType::RECORDER][object] = pid;

        Dumper dumper;
        dumper.entry_ = [recorder = recorderStub](int32_t fd) -> int32_t {
            return recorder->DumpInfo(fd);
        };
        dumper.pid_ = pid;
        dumper.uid_ = IPCSkeleton::GetCallingUid();
        dumper.remoteObject_ = object;
        dumperTbl_[StubType::RECORDER].emplace_back(dumper);
        MEDIA_LOGD("The number of recorder services(%{public}zu) pid(%{public}d).",
            stubMap_[StubType::RECORDER].size(), pid);
        if (Dump(-1, std::vector<std::u16string>()) != OHOS::NO_ERROR) {
            MEDIA_LOGW("failed to call InstanceDump");
        }
    }
    return object;
}
#endif

#ifdef SUPPORT_METADATA
sptr<IRemoteObject> MediaServerManager::CreateAVMetadataHelperStubObject()
{
    constexpr uint32_t metadataHelperNumMax = 32;
    if (stubMap_[StubType::AVMETADATAHELPER].size() >= metadataHelperNumMax) {
        MEDIA_LOGE("The number of avmetadatahelper services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", stubMap_[StubType::AVMETADATAHELPER].size());
        return nullptr;
    }
    sptr<AVMetadataHelperServiceStub> avMetadataHelperStub = AVMetadataHelperServiceStub::Create();
    if (avMetadataHelperStub == nullptr) {
        MEDIA_LOGE("failed to create AVMetadataHelperServiceStub");
        return nullptr;
    }
    sptr<IRemoteObject> object = avMetadataHelperStub->AsObject();
    if (object != nullptr) {
        pid_t pid = IPCSkeleton::GetCallingPid();
        stubMap_[StubType::AVMETADATAHELPER][object] = pid;

        Dumper dumper;
        dumper.pid_ = pid;
        dumper.uid_ = IPCSkeleton::GetCallingUid();
        dumper.remoteObject_ = object;
        dumperTbl_[StubType::AVMETADATAHELPER].emplace_back(dumper);

        MEDIA_LOGD("The number of avmetadatahelper services(%{public}zu) pid(%{public}d).",
            stubMap_[StubType::AVMETADATAHELPER].size(), pid);
        if (Dump(-1, std::vector<std::u16string>()) != OHOS::NO_ERROR) {
            MEDIA_LOGW("failed to call InstanceDump");
        }
    }
    return object;
}
#endif

#ifdef SUPPORT_CODEC
sptr<IRemoteObject> MediaServerManager::CreateAVCodecListStubObject()
{
    if (stubMap_[StubType::AVCODECLIST].size() >= SERVER_MAX_NUMBER) {
        MEDIA_LOGE("The number of codeclist services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", stubMap_[StubType::AVCODECLIST].size());
        return nullptr;
    }
    sptr<AVCodecListServiceStub> avCodecListStub = AVCodecListServiceStub::Create();
    if (avCodecListStub == nullptr) {
        MEDIA_LOGE("failed to create AVCodecListServiceStub");
        return nullptr;
    }
    sptr<IRemoteObject> object = avCodecListStub->AsObject();
    if (object != nullptr) {
        pid_t pid = IPCSkeleton::GetCallingPid();
        stubMap_[StubType::AVCODECLIST][object] = pid;
        MEDIA_LOGD("The number of codeclist services(%{public}zu).", stubMap_[StubType::AVCODECLIST].size());
    }
    return object;
}

sptr<IRemoteObject> MediaServerManager::CreateAVCodecStubObject()
{
    if (stubMap_[StubType::AVCODEC].size() >= SERVER_MAX_NUMBER) {
        MEDIA_LOGE("The number of avcodec services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", stubMap_[StubType::AVCODEC].size());
        return nullptr;
    }
    sptr<AVCodecServiceStub> avCodecHelperStub = AVCodecServiceStub::Create();
    if (avCodecHelperStub == nullptr) {
        MEDIA_LOGE("failed to create AVCodecServiceStub");
        return nullptr;
    }
    sptr<IRemoteObject> object = avCodecHelperStub->AsObject();
    if (object != nullptr) {
        pid_t pid = IPCSkeleton::GetCallingPid();
        stubMap_[StubType::AVCODEC][object] = pid;

        Dumper dumper;
        dumper.entry_ = [avcodec = avCodecHelperStub](int32_t fd) -> int32_t {
            return avcodec->DumpInfo(fd);
        };
        dumper.pid_ = pid;
        dumper.uid_ = IPCSkeleton::GetCallingUid();
        dumper.remoteObject_ = object;
        dumperTbl_[StubType::AVCODEC].emplace_back(dumper);
        MEDIA_LOGD("The number of avcodec services(%{public}zu).", stubMap_[StubType::AVCODEC].size());
        if (Dump(-1, std::vector<std::u16string>()) != OHOS::NO_ERROR) {
            MEDIA_LOGW("failed to call InstanceDump");
        }
    }
    return object;
}
#endif

#ifdef SUPPORT_RECORDER
sptr<IRemoteObject> MediaServerManager::CreateRecorderProfilesStubObject()
{
    if (stubMap_[StubType::RECORDERPROFILES].size() >= SERVER_MAX_NUMBER) {
        MEDIA_LOGE("The number of recorder_profiles services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", stubMap_[StubType::RECORDERPROFILES].size());
        return nullptr;
    }
    sptr<RecorderProfilesServiceStub> recorderProfilesStub = RecorderProfilesServiceStub::Create();
    if (recorderProfilesStub == nullptr) {
        MEDIA_LOGE("failed to create recorderProfilesStub");
        return nullptr;
    }
    sptr<IRemoteObject> object = recorderProfilesStub->AsObject();
    if (object != nullptr) {
        pid_t pid = IPCSkeleton::GetCallingPid();
        stubMap_[StubType::RECORDERPROFILES][object] = pid;
        MEDIA_LOGD("The number of recorder_profiles services(%{public}zu).", stubMap_[StubType::RECORDERPROFILES].size());
    }
    return object;
}
#endif

#ifdef SUPPORT_MUXER
sptr<IRemoteObject> MediaServerManager::CreateAVMuxerStubObject()
{
    if (stubMap_[StubType::AVMUXER].size() >= SERVER_MAX_NUMBER) {
        MEDIA_LOGE("The number of avmuxer services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", stubMap_[StubType::AVMUXER].size());
        return nullptr;
    }
    sptr<AVMuxerServiceStub> avmuxerStub = AVMuxerServiceStub::Create();
    if (avmuxerStub == nullptr) {
        MEDIA_LOGE("failed to create AVMuxerServiceStub");
        return nullptr;
    }
    sptr<IRemoteObject> object = avmuxerStub->AsObject();
    if (object != nullptr) {
        pid_t pid = IPCSkeleton::GetCallingPid();
        stubMap_[StubType::AVMUXER][object] = pid;
        MEDIA_LOGD("The number of avmuxer services(%{public}zu).", stubMap_[StubType::AVMUXER].size());
    }
    return object;
}
#endif

void MediaServerManager::DestroyStubObject(StubType type, sptr<IRemoteObject> object)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pid_t pid = IPCSkeleton::GetCallingPid();
    DestroyDumper(type, object);
    switch (type) {
        case RECORDER: {
            for (auto it = stubMap_[StubType::RECORDER].begin(); it != stubMap_[StubType::RECORDER].end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destroy recorder stub services(%{public}zu) pid(%{public}d).",
                        stubMap_[StubType::RECORDER].size(), pid);
                    (void)stubMap_[StubType::RECORDER].erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find recorder object failed, pid(%{public}d).", pid);
            break;
        }
        case PLAYER: {
            for (auto it = stubMap_[StubType::PLAYER].begin(); it != stubMap_[StubType::PLAYER].end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destroy player stub services(%{public}zu) pid(%{public}d).",
                        stubMap_[StubType::PLAYER].size(), pid);
                    (void)stubMap_[StubType::PLAYER].erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find player object failed, pid(%{public}d).", pid);
            break;
        }
        case AVMETADATAHELPER: {
            for (auto it = stubMap_[StubType::AVMETADATAHELPER].begin(); it != stubMap_[StubType::AVMETADATAHELPER].end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destroy avmetadatahelper stub services(%{public}zu) pid(%{public}d).",
                        stubMap_[StubType::AVMETADATAHELPER].size(), pid);
                    (void)stubMap_[StubType::AVMETADATAHELPER].erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find avmetadatahelper object failed, pid(%{public}d).", pid);
            break;
        }
        case AVCODEC: {
            for (auto it = stubMap_[StubType::AVCODEC].begin(); it != stubMap_[StubType::AVCODEC].end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destroy avcodec stub services(%{public}zu) pid(%{public}d).",
                        stubMap_[StubType::AVCODEC].size(), pid);
                    (void)stubMap_[StubType::AVCODEC].erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find avcodec object failed, pid(%{public}d).", pid);
            break;
        }
        case AVCODECLIST: {
            for (auto it = stubMap_[StubType::AVCODECLIST].begin(); it != stubMap_[StubType::AVCODECLIST].end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destroy avcodeclist stub services(%{public}zu) pid(%{public}d).",
                        stubMap_[StubType::AVCODECLIST].size(), pid);
                    (void)stubMap_[StubType::AVCODECLIST].erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find avcodeclist object failed, pid(%{public}d).", pid);
            break;
        }
        case RECORDERPROFILES: {
            for (auto it = stubMap_[StubType::RECORDERPROFILES].begin(); it != stubMap_[StubType::RECORDERPROFILES].end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destroy mediaprofile stub services(%{public}zu) pid(%{public}d).",
                        stubMap_[StubType::RECORDERPROFILES].size(), pid);
                    (void)stubMap_[StubType::RECORDERPROFILES].erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find mediaprofile object failed, pid(%{public}d).", pid);
            break;
        }
        case AVMUXER: {
            for (auto it = stubMap_[StubType::AVMUXER].begin(); it != stubMap_[StubType::AVMUXER].end(); it++) {
                if (it->first == object) {
                    MEDIA_LOGD("destory avmuxer stub services(%{public}zu) pid(%{public}d).",
                        stubMap_[StubType::AVMUXER].size(), pid);
                    (void)stubMap_[StubType::AVMUXER].erase(it);
                    return;
                }
            }
            MEDIA_LOGE("find avmuxer object failed, pid(%{public}d).", pid);
            break;
        }
        default: {
            MEDIA_LOGE("default case, media server manager failed, pid(%{public}d).", pid);
            break;
        }
    }
}

void MediaServerManager::DestroyStubObjectForPid(pid_t pid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("recorder stub services(%{public}zu) pid(%{public}d).", stubMap_[StubType::RECORDER].size(), pid);
    DestroyDumperForPid(pid);
    for (auto itRecorder = stubMap_[StubType::RECORDER].begin(); itRecorder != stubMap_[StubType::RECORDER].end();) {
        if (itRecorder->second == pid) {
            itRecorder = stubMap_[StubType::RECORDER].erase(itRecorder);
        } else {
            itRecorder++;
        }
    }
    MEDIA_LOGD("recorder stub services(%{public}zu).", stubMap_[StubType::RECORDER].size());

    MEDIA_LOGD("player stub services(%{public}zu) pid(%{public}d).", stubMap_[StubType::PLAYER].size(), pid);
    for (auto itPlayer = stubMap_[StubType::PLAYER].begin(); itPlayer != stubMap_[StubType::PLAYER].end();) {
        if (itPlayer->second == pid) {
            itPlayer = stubMap_[StubType::PLAYER].erase(itPlayer);
        } else {
            itPlayer++;
        }
    }
    MEDIA_LOGD("player stub services(%{public}zu).", stubMap_[StubType::PLAYER].size());

    MEDIA_LOGD("avmetadatahelper stub services(%{public}zu) pid(%{public}d).", stubMap_[StubType::AVMETADATAHELPER].size(), pid);
    for (auto itAvMetadata = stubMap_[StubType::AVMETADATAHELPER].begin(); itAvMetadata != stubMap_[StubType::AVMETADATAHELPER].end();) {
        if (itAvMetadata->second == pid) {
            itAvMetadata = stubMap_[StubType::AVMETADATAHELPER].erase(itAvMetadata);
        } else {
            itAvMetadata++;
        }
    }
    MEDIA_LOGD("avmetadatahelper stub services(%{public}zu).", stubMap_[StubType::AVMETADATAHELPER].size());

    MEDIA_LOGD("avcodec stub services(%{public}zu) pid(%{public}d).", stubMap_[StubType::AVCODEC].size(), pid);
    for (auto itAvCodec = stubMap_[StubType::AVCODEC].begin(); itAvCodec != stubMap_[StubType::AVCODEC].end();) {
        if (itAvCodec->second == pid) {
            itAvCodec = stubMap_[StubType::AVCODEC].erase(itAvCodec);
        } else {
            itAvCodec++;
        }
    }
    MEDIA_LOGD("avcodec stub services(%{public}zu).", stubMap_[StubType::AVCODEC].size());

    MEDIA_LOGD("avcodeclist stub services(%{public}zu) pid(%{public}d).", stubMap_[StubType::AVCODECLIST].size(), pid);
    for (auto itAvCodecList = stubMap_[StubType::AVCODECLIST].begin(); itAvCodecList != stubMap_[StubType::AVCODECLIST].end();) {
        if (itAvCodecList->second == pid) {
            itAvCodecList = stubMap_[StubType::AVCODECLIST].erase(itAvCodecList);
        } else {
            itAvCodecList++;
        }
    }
    MEDIA_LOGD("avcodeclist stub services(%{public}zu).", stubMap_[StubType::AVCODECLIST].size());

    MEDIA_LOGD("mediaprofile stub services(%{public}zu) pid(%{public}d).", stubMap_[StubType::RECORDERPROFILES].size(), pid);
    for (auto itMediaProfile = stubMap_[StubType::RECORDERPROFILES].begin(); itMediaProfile != stubMap_[StubType::RECORDERPROFILES].end();) {
        if (itMediaProfile->second == pid) {
            itMediaProfile = stubMap_[StubType::RECORDERPROFILES].erase(itMediaProfile);
        } else {
            itMediaProfile++;
        }
    }
    MEDIA_LOGD("mediaprofile stub services(%{public}zu).", stubMap_[StubType::RECORDERPROFILES].size());

    MEDIA_LOGD("avmuxer stub services(%{public}zu) pid(%{public}d).", stubMap_[StubType::AVMUXER].size(), pid);
    for (auto itAVMuxer = stubMap_[StubType::AVMUXER].begin(); itAVMuxer != stubMap_[StubType::AVMUXER].end();) {
        if (itAVMuxer->second == pid) {
            itAVMuxer = stubMap_[StubType::AVMUXER].erase(itAVMuxer);
        } else {
            itAVMuxer++;
        }
    }
    MEDIA_LOGD("avmuxer stub services(%{public}zu).", stubMap_[StubType::AVMUXER].size());
}

void MediaServerManager::DestroyDumper(StubType type, sptr<IRemoteObject> object)
{
    for (auto it = dumperTbl_[type].begin(); it != dumperTbl_[type].end(); it++) {
        if (it->remoteObject_ == object) {
            (void)dumperTbl_[type].erase(it);
            MEDIA_LOGD("MediaServerManager::DestroyDumper");
            if (Dump(-1, std::vector<std::u16string>()) != OHOS::NO_ERROR) {
                MEDIA_LOGW("failed to call InstanceDump");
            }
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
    if (Dump(-1, std::vector<std::u16string>()) != OHOS::NO_ERROR) {
        MEDIA_LOGW("failed to call InstanceDump");
    }
}
} // namespace Media
} // namespace OHOS
