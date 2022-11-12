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
#include "service_dump_manager.h"
#include "player_xcollie.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaServerManager"};
}

namespace OHOS {
namespace Media {
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

void MediaServerManager::DestroyStubObject(StubType type, sptr<IRemoteObject> object)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pid_t pid = IPCSkeleton::GetCallingPid();
    DestroyDumper(type, object);
    stubMap_[type].erase(object);
    MEDIA_LOGD("destroy %{public}d stub services(%{public}zu) pid(%{public}d).", type,
        stubMap_[type].size(), pid);
}

void MediaServerManager::DestroyStubObjectForPid(pid_t pid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    DestroyDumperForPid(pid);

    for (auto iter : stubMap_) {
        MEDIA_LOGD("(%{public}d) stub services(%{public}zu) pid(%{public}d).", iter.first, iter.second.size(), pid);
        for (auto it = iter.second.begin(); it != iter.second.end();) {
            if (it->second == pid) {
                iter.second.erase(it);
                MEDIA_LOGD("destroy %{public}d stub services(%{public}zu) pid(%{public}d).", iter.first,
                        iter.second.size(), pid);
            } else {
                it++;
            }
        }
    }
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
