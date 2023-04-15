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
#include <codecvt>
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
    auto transform = [](std::string &str) {
        for (auto &c : str) {
            c = tolower(c);
        }
    };
    for (auto &stubUtil : stubUtils_) {
        dumpString += "------------------"+ stubUtil.GetName() +"Server------------------\n";
        bool ret = false;
        if (stubUtil.GetStubType() == StubType::AVMETADATAHELPER) {
            ret = WriteInfo(fd, dumpString, stubUtil.GetDumpers(), false);
        } else {
            // transform from upper to lower u16string
            std::string media_name = stubUtil.GetName();
            transform(media_name);
            std::u16string name = std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> {}.from_bytes(media_name);
            ret = WriteInfo(fd, dumpString, stubUtil.GetDumpers(), argSets.find(name) != argSets.end());
        }
        if (ret != OHOS::NO_ERROR) {
            std::string log = "Failed to write " + stubUtil.GetName() + "Server information";
            MEDIA_LOGW("%{public}s", log.c_str());
            return OHOS::INVALID_OPERATION;
        }
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

sptr<IRemoteObject> MediaServerManager::CreateStubObject(ServiceStubUtil &stubUtil)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (stubUtil.GetStubMapSize() >= SERVER_MAX_NUMBER) {
        MEDIA_LOGE("The number of %{public}s services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", stubUtil.GetName().c_str(), stubUtil.GetStubMapSize());
        return nullptr;
    }
    auto stub = stubUtil.CreateStub();
    Dumper::DumperEntry entry = [media = stub](int32_t fd) -> int32_t {
        return media->DumpInfo(fd);
    };
    auto object = stub->AsObject();
    if (object != nullptr) {
        pid_t pid = IPCSkeleton::GetCallingPid();
        stubUtil.AddObject(object, pid);
        Dumper dumper {pid, IPCSkeleton::GetCallingUid(), entry, object};
        stubUtil.AddDumper(dumper);
        MEDIA_LOGD("The number of %{public}s services(%{public}zu) pid(%{public}d).",
            stubUtil.GetName().c_str(), stubUtil.GetStubMapSize(), pid);
        std::string varName = "The number of " + stubUtil.GetName();
        MediaTrace::CounterTrace(varName, stubUtil.GetStubMapSize());
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
    for (auto &stub : stubUtils_) {
        if (stub.GetStubType() == type) {
            stub.DeleteStubObject(object);
            MEDIA_LOGD("destroy %{public}s stub services(%{public}zu) pid(%{public}d).",
                stub.GetName().c_str(), stub.GetStubMapSize(), pid);
            return;
        }
    }
}

void MediaServerManager::DestroyStubObjectForPid(pid_t pid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    DestroyDumperForPid(pid);
    for (auto &stub : stubUtils_) {
        MEDIA_LOGD("%{public}s stub services(%{public}zu) pid(%{public}d).",
            stub.GetName().c_str(), stub.GetStubMapSize(), pid);
        stub.DeleteStubObjectForPid(pid);
        MEDIA_LOGD("%{public}s stub services(%{public}zu).", stub.GetName().c_str(), stub.GetStubMapSize());
    }
    executor_.Clear();
}

void MediaServerManager::DestroyDumper(StubType type, sptr<IRemoteObject> object)
{
    for (auto &stubUtil : stubUtils_) {
        if (stubUtil.GetStubType() == type) {
            auto &dumpers = stubUtil.GetDumpers();
            for (auto it = dumpers.begin(); it != dumpers.end(); ++it) {
                if (it->remoteObject_ == object) {
                    (void)dumpers.erase(it);
                    MEDIA_LOGD("MediaServerManager::DestroyDumper");
                    if (Dump(-1, std::vector<std::u16string>()) != OHOS::NO_ERROR) {
                        MEDIA_LOGW("failed to call InstanceDump");
                    }
                    return;
                }
            }
        }
    }
}

void MediaServerManager::DestroyDumperForPid(pid_t pid)
{
    for (auto &stubUtil : stubUtils_) {
        auto &dumpers = stubUtil.GetDumpers();
        for (auto it = dumpers.begin(); it != dumpers.end();) {
            if (it->pid_ == pid) {
                it = dumpers.erase(it);
                MEDIA_LOGD("MediaServerManager::DestroyDumperForPid");
            } else {
                ++it;
            }
        }
    }
    if (Dump(-1, std::vector<std::u16string>()) != OHOS::NO_ERROR) {
        MEDIA_LOGW("failed to call InstanceDump");
    }
}

std::vector<ServiceStubUtil> &MediaServerManager::GetServiceStubUtils()
{
    return stubUtils_;
}

AsyncExecutor &MediaServerManager::GetAsyncExecutor()
{
    return executor_;
}

void AsyncExecutor::Commit(sptr<IRemoteObject> obj)
{
    std::lock_guard<std::mutex> lock(listMutex_);
    freeList_.push_back(obj);
}

void AsyncExecutor::Clear()
{
    std::thread(&AsyncExecutor::HandleAsyncExecution, this).detach();
}

void AsyncExecutor::HandleAsyncExecution()
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
