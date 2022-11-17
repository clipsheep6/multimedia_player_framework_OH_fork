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

#ifndef MEDIA_SERVER_MANAGER_H
#define MEDIA_SERVER_MANAGER_H

#include <memory>
#include <functional>
#include <map>
#include <list>
#include <unistd.h>
#include "iremote_object.h"
#include "ipc_skeleton.h"
#include "nocopyable.h"
#include "media_log.h"
#include "media_errors.h"

namespace OHOS {
namespace Media {
using DumperEntry = std::function<int32_t(int32_t)>;
struct Dumper {
    pid_t pid_;
    pid_t uid_;
    DumperEntry entry_;
    sptr<IRemoteObject> remoteObject_;
};

class MediaServerManager : public NoCopyable {
public:
    static MediaServerManager &GetInstance();
    ~MediaServerManager();

    enum StubType {
        RECORDER = 0,
        PLAYER,
        AVMETADATAHELPER,
        AVCODECLIST,
        AVCODEC,
        AVMUXER,
        RECORDERPROFILES,
    };
    template<typename T>
    sptr<IRemoteObject> CreateStubObject(StubType type);
    void DestroyStubObject(StubType type, sptr<IRemoteObject> object);
    void DestroyStubObjectForPid(pid_t pid);
    int32_t Dump(int32_t fd, const std::vector<std::u16string> &args);
    void DestroyDumper(StubType type, sptr<IRemoteObject> object);
    void DestroyDumperForPid(pid_t pid);

private:
    MediaServerManager();
    class AsyncExecutor {
    public:
        AsyncExecutor() = default;
        virtual ~AsyncExecutor() = default;
        void Commit(sptr<IRemoteObject> obj);
        void Clear();
    private:
        void HandleAsyncExecution();
        std::list<sptr<IRemoteObject>> freeList_;
        std::mutex listMutex_;
    };

    std::map<StubType, std::vector<Dumper>> dumperTbl_;
    AsyncExecutor executor_;

    std::list<std::pair<StubType, std::pair<std::string, std::u16string>>> serverList_ = {
        {StubType::PLAYER, {"PlayerServer", u"player"}},
        {StubType::RECORDER, {"RecorderServer", u"recorder"}},
        {StubType::AVCODEC, {"CodecServer", u"codec"}},
        {StubType::AVMETADATAHELPER, {"AVMetaServer", u"avmetahelper"}},
    };

    using StubMap = std::map<StubType, std::map<sptr<IRemoteObject>, pid_t>>;
    StubMap stubMap_;
    std::mutex mutex_;

    static constexpr uint32_t SERVER_MAX_NUMBER = 16;
};

template<typename T>
sptr<IRemoteObject> MediaServerManager::CreateStubObject(StubType type)
{
    std::lock_guard<std::mutex> lock(mutex_);
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaServerManager"};
    auto map = stubMap_[type];
    if (map.size() >= SERVER_MAX_NUMBER) {
        MEDIA_LOGE("The number of (%{public}zu) services(%{public}zu) has reached the upper limit."
            "Please release the applied resources.", static_cast<int32_t>(type), map.size());
        return nullptr;
    }
    sptr<T> stub = T::Create();
    if (stub == nullptr) {
        MEDIA_LOGE("failed to create (%{public}zu) stub", static_cast<int32_t>(type));
        return nullptr;
    }

    sptr<IRemoteObject> object = stub->AsObject();
    if (object != nullptr) {
        pid_t pid = IPCSkeleton::GetCallingPid();
        map[object] = pid;
        dumperTbl_[type].emplace_back(Dumper{
            pid,
            IPCSkeleton::GetCallingUid(),
            [media = stub](int32_t fd) -> int32_t {
                return media->DumpInfo(fd);
            },
            object
        });
        MEDIA_LOGD("The number of (%{public}zu) services(%{public}zu) pid(%{public}d).",
            static_cast<int32_t>(type), map.size(), pid);
        if (Dump(-1, std::vector<std::u16string>()) != OHOS::NO_ERROR) {
            MEDIA_LOGW("failed to call InstanceDump");
        }
    }
    return object;
}
} // namespace Media
} // namespace OHOS
#endif // MEDIA_SERVER_MANAGER_H