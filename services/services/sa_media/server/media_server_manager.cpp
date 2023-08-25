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
        CHECK_AND_RETURN_RET((!needDetail) || (iter.entry_(fd) == MSERR_OK), OHOS::INVALID_OPERATION);
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
    for (const auto &it : dumpCollections_) {
        dumpString += "------------------"+ stubCollections_[it.first].name + "Server------------------\n";
        int32_t ret = OHOS::NO_ERROR;
        bool needDetail = false;
        if (it.first != StubType::AVMETADATAHELPER) {
            needDetail = argSets.find(it.second) != argSets.end();
        }
        if (dumperTbl_.count(it.first) > 0) {
            ret = WriteInfo(fd, dumpString, dumperTbl_[it.first], needDetail);
        }
        CHECK_AND_RETURN_RET(ret == OHOS::NO_ERROR, OHOS::INVALID_OPERATION);
    }
    CHECK_AND_RETURN_RET_LOG(ServiceDumpManager::GetInstance().Dump(fd, argSets) == OHOS::NO_ERROR,
        OHOS::INVALID_OPERATION, "Failed to write dfx dump information");

    CHECK_AND_RETURN_RET_LOG(PlayerXCollie::GetInstance().Dump(fd) == OHOS::NO_ERROR,
        OHOS::INVALID_OPERATION, "Failed to write xcollie dump information");

    CHECK_AND_RETURN_RET_LOG(MonitorServiceStub::GetInstance()->DumpInfo(fd) == OHOS::NO_ERROR,
        OHOS::INVALID_OPERATION, "Failed to write monitor dump information");
    return OHOS::NO_ERROR;
}

MediaServerManager::MediaServerManager()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    Init();
}

MediaServerManager::~MediaServerManager()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    decltype(stubCollections_) tempCollection;
    std::swap(stubCollections_, tempCollection);
    decltype(stubMap_) tempStubMap;
    std::swap(stubMap_, tempStubMap);
    tempStubMap.clear();
    tempCollection.clear();
    dumpCollections_.clear();
}

void MediaServerManager::Init()
{
    auto maxSize = SERVER_MAX_NUMBER;
#ifdef SUPPORT_PLAYER
#ifdef PLAYER_USE_MEMORY_MANAGE
    std::function<sptr<IMediaStubService>()> create = PlayerServiceStubMem::Create;
#else
    std::function<sptr<IMediaStubService>()> create = PlayerServiceStub::Create;
#endif
    stubCollections_[StubType::PLAYER] = StubNode {"Player",
        create, maxSize};
    dumpCollections_.emplace_back(std::make_pair(StubType::PLAYER, u"player"));
#endif
#ifdef SUPPORT_RECORDER
    stubCollections_[StubType::RECORDER] = StubNode {"Recorder", RecorderServiceStub::Create, SERVER_MAX_NUMBER / 8};
    stubCollections_[StubType::RECORDERPROFILES] = StubNode {"RecorderProfiles",
        RecorderProfilesServiceStub::Create, SERVER_MAX_NUMBER};
    dumpCollections_.emplace_back(std::make_pair(StubType::RECORDER, u"recorder"));
#endif
#ifdef SUPPORT_CODEC
    stubCollections_[StubType::AVCODEC] = StubNode {"Avcodec", AVCodecServiceStub::Create, maxSize};
    stubCollections_[StubType::AVCODECLIST] = StubNode {"Codeclist",
        AVCodecListServiceStub::Create, SERVER_MAX_NUMBER};
    dumpCollections_.emplace_back(std::make_pair(StubType::AVCODEC, u"codec"));
#endif
#ifdef SUPPORT_METADATA
    stubCollections_[StubType::AVMETADATAHELPER] = StubNode {"AvmetadataHelper",
        AVMetadataHelperServiceStub::Create, SERVER_MAX_NUMBER * 2};
    dumpCollections_.emplace_back(std::make_pair(StubType::AVMETADATAHELPER, u"avmetadatahelper"));
#endif
#ifdef SUPPORT_SCREEN_CAPTURE
    stubCollections_[StubType::SCREEN_CAPTURE] = StubNode {"ScreenCapture",
        ScreenCaptureServiceStub::Create, SERVER_MAX_NUMBER};
#endif
    stubCollections_[StubType::MONITOR] = StubNode {"Monitor",
        MonitorServiceStub::GetInstance, SERVER_MAX_NUMBER / 16
    };
}

sptr<IRemoteObject> MediaServerManager::CreateStubObject(StubType type)
{
    CHECK_AND_RETURN_RET(stubCollections_.count(type) > 0, nullptr);
    auto node = stubCollections_[type];
    CHECK_AND_RETURN_RET(stubMap_[type].size() < node.maxSize, nullptr);
    auto stub = node.create();
    CHECK_AND_RETURN_RET(stub != nullptr, nullptr);
    auto object = stub->AsObject();
    CHECK_AND_RETURN_RET(object != nullptr, nullptr);
    pid_t pid = IPCSkeleton::GetCallingPid();
    stubMap_[type][object] = pid;
    DumperEntry entry = [media = stub](int32_t fd) -> int32_t {
        return media->DumpInfo(fd);
    };
    Dumper dumper {
        pid, IPCSkeleton::GetCallingUid(), entry, object
    };
    dumperTbl_[type].emplace_back(dumper);
    MEDIA_LOGD("The number of %{public}s services(%{public}zu) pid(%{public}d).",
        node.name.c_str(), stubMap_[type].size(), pid);
    std::string traceName = "The number of " + node.name;
    MediaTrace::CounterTrace(traceName, stubMap_[type].size());
    CHECK_AND_RETURN_RET_LOG(Dump(-1, std::vector<std::u16string>()) == OHOS::NO_ERROR,
        object, "failed to call InstanceDump");
    return object;
}

void MediaServerManager::DestroyStubObject(StubType type, sptr<IRemoteObject> object)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pid_t pid = IPCSkeleton::GetCallingPid();
    DestroyDumper(type, object);
    CHECK_AND_RETURN_LOG(stubCollections_.count(type) > 0, "no this StubType %{public}d", type);
    auto &map = stubMap_[type];
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (it->first == object) {
            MEDIA_LOGD("destroy %{public}s stub services(%{public}zu) pid(%{public}d).",
                stubCollections_[type].name.c_str(), map.size(), pid);
            (void)map.erase(it);
            return;
        }
    }
    MEDIA_LOGE("find %{public}s object failed, pid(%{public}d).", stubCollections_[type].name.c_str(), pid);
}

void MediaServerManager::DestroyStubObjectForPid(pid_t pid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    DestroyDumperForPid(pid);
    for (const auto &iter : stubCollections_) {
        const auto &node = iter.second;
        auto &map = stubMap_[iter.first];
        MEDIA_LOGD("%{public}s stub services(%{public}zu) pid(%{public}d).", node.name.c_str(), map.size(), pid);
        for (auto it = map.begin(); it != map.end();) {
            if (it->second == pid) {
                executor_.Commit(it->first);
                it = map.erase(it);
            } else {
                ++it;
            }
        }
        MEDIA_LOGD("%{public}s stub services(%{public}zu).", node.name.c_str(), map.size());
    }
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
                ++it;
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
