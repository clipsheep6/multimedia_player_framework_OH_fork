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
#endif
#ifdef SUPPORT_METADATA
#include "avmetadatahelper_service_stub.h"
#endif
#ifdef SUPPORT_CODEC
#include "avcodec_service_stub.h"
#include "avcodeclist_service_stub.h"
#endif
#include "media_log.h"
#include "media_errors.h"
#include "media_dfx.h"
#include "service_dump_manager.h"
#include "player_xcollie.h"

namespace {
using OHOS::Media::MediaServerManager;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaServerManager"};
std::map<MediaServerManager::StubType, std::string> STUB_TYPE_NAME = {
    {MediaServerManager::RECORDER, "Recorder"},
    {MediaServerManager::PLAYER, "Player"},
    {MediaServerManager::AVMETADATAHELPER, "AvMetaDataHelper"},
    {MediaServerManager::AVCODECLIST, "AVCodecList"},
    {MediaServerManager::AVCODEC, "AVCodec"},
    {MediaServerManager::RECORDERPROFILES, "RecorderProfiles"}
};
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

    if (ServiceDumpManager::GetInstance().Dump(fd, argSets) != OHOS::NO_ERROR) {
        MEDIA_LOGW("Failed to write dfx dump information");
        return OHOS::INVALID_OPERATION;
    }

    if (PlayerXCollie::GetInstance().Dump(fd) != OHOS::NO_ERROR) {
        MEDIA_LOGW("Failed to write xcollie dump information");
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
#ifdef SUPPORT_RECORDER
        case RECORDER: {
            sptr<RecorderServiceStub> stub = RecorderServiceStub::Create();
            DumperEntry entry = [media = stub](int32_t fd) -> int32_t {
                return media->DumpInfo(fd);
            };
            return CreateStub(stub->AsObject(), entry, type);
        }
        case RECORDERPROFILES: {
            sptr<RecorderProfilesServiceStub> stub = RecorderProfilesServiceStub::Create();
            DumperEntry entry = [](int32_t) -> int32_t { return MSERR_OK; };
            return CreateStub(stub->AsObject(), entry, type);
        }
#endif
#ifdef SUPPORT_PLAYER
        case PLAYER: {
            sptr<PlayerServiceStub> stub = PlayerServiceStub::Create();
            if (stub == nullptr) {
                MEDIA_LOGE("failed to create ServiceStub");
                return nullptr;
            }
            DumperEntry entry = [media = stub](int32_t fd) -> int32_t {
                return media->DumpInfo(fd);
            };
            return CreateStub(stub->AsObject(), entry, type);
        }
#endif
#ifdef SUPPORT_METADATA
        case AVMETADATAHELPER: {
            sptr<AVMetadataHelperServiceStub> avMetadataHelperStub = AVMetadataHelperServiceStub::Create();
            DumperEntry entry = [](int32_t) -> int32_t { return MSERR_OK; };
            return CreateStub(avMetadataHelperStub->AsObject(), entry, type);
        }
#endif
#ifdef SUPPORT_CODEC
        case AVCODECLIST: {
            sptr<AVCodecListServiceStub> stub = AVCodecListServiceStub::Create();
            DumperEntry entry = [](int32_t) -> int32_t { return MSERR_OK; };
            return CreateStub(stub->AsObject(), entry, type);
        }
        case AVCODEC: {
            sptr<AVCodecServiceStub> stub = AVCodecServiceStub::Create();
            DumperEntry entry = [media = stub](int32_t fd) -> int32_t { return media->DumpInfo(fd); };
            return CreateStub(stub->AsObject(), entry, type);
        }
#endif
        default: {
            MEDIA_LOGE("default case, media server manager failed");
            return nullptr;
        }
    }
}

sptr<IRemoteObject> MediaServerManager::CreateStub(sptr<IRemoteObject> object, DumperEntry entry, StubType type)
{
    if (stubMap_[type].size() >= SERVER_MAX_NUMBER) {
        // log detail, todo
        MEDIA_LOGE("The number of %{public}s services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", STUB_TYPE_NAME[type].c_str(), stubMap_[type].size());
        return nullptr;
    }
    if (object != nullptr) {
        pid_t pid = IPCSkeleton::GetCallingPid();
        stubMap_[type][object] = pid;

        Dumper dumper = { pid, IPCSkeleton::GetCallingUid(), entry, object };
        dumperTbl_[type].emplace_back(dumper);
        MEDIA_LOGD("The number of %{public}s services(%{public}zu) pid(%{public}d).",
            STUB_TYPE_NAME[type].c_str(), stubMap_[type].size(), pid);
        std::string varName = "The number of " + STUB_TYPE_NAME[type];
        MediaTrace::CounterTrace(varName, stubMap_[type].size());
        if (Dump(-1, std::vector<std::u16string>()) != OHOS::NO_ERROR) {
            MEDIA_LOGW("failed to call InstanceDump");
        }
    }
    return object;
}

void MediaServerManager::DestroyStubObject(StubType type, sptr<IRemoteObject> object)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pid_t pid = IPCSkeleton::GetCallingPid();
    DestroyDumper(type, object);
    for (auto it = stubMap_[type].begin(); it != stubMap_[type].end(); it++) {
        if (it->first == object) {
            MEDIA_LOGD("destroy %{public}s stub services(%{public}zu) pid(%{public}d).",
                STUB_TYPE_NAME[type].c_str(), stubMap_[type].size(), pid);
            (void)stubMap_[type].erase(it);
            return;
        }
    }
}

void MediaServerManager::DestroyStubObjectForPid(pid_t pid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    DestroyDumperForPid(pid);

    for (const auto &stub : stubMap_) {
        auto map = stub.second;
        MEDIA_LOGD("%{public}s stub services(%{public}zu) pid(%{public}d).",
            STUB_TYPE_NAME[stub.first].c_str(), map.size(), pid);
        for (auto iter = map.begin(); iter != map.end();) {
            if (iter->second == pid) {
                executor_.Commit(iter->first);
                iter = map.erase(iter);
            } else {
                iter++;
            }
        }
        MEDIA_LOGD("%{public}s stub services(%{public}zu).", STUB_TYPE_NAME[stub.first].c_str(), map.size());
    }
    executor_.Clear();
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
