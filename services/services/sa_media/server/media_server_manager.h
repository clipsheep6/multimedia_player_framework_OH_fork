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
#include <vector>
#include "i_media_stub_service.h"
#include "iremote_object.h"
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
#ifdef SUPPORT_CODEC
#include "avcodec_service_stub.h"
#include "avcodeclist_service_stub.h"
#endif
#ifdef SUPPORT_SCREEN_CAPTURE
#include "screen_capture_service_stub.h"
#endif
#include "monitor_service_stub.h"
#include "ipc_skeleton.h"
#include "nocopyable.h"

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
        RECORDERPROFILES,
        MONITOR,
        SCREEN_CAPTURE,
    };
    sptr<IRemoteObject> CreateStubObject(StubType type);
    void DestroyStubObject(StubType type, sptr<IRemoteObject> object);
    void DestroyStubObjectForPid(pid_t pid);
    int32_t Dump(int32_t fd, const std::vector<std::u16string> &args);
    void DestroyDumper(StubType type, sptr<IRemoteObject> object);
    void DestroyDumperForPid(pid_t pid);

private:
    MediaServerManager();
    sptr<IRemoteObject> GetMonitorStubObject();
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
    struct StubNode {
        std::string name;
        std::function<sptr<IMediaStubService>()> create;
        unsigned int maxSize;
    };
    bool alreadyInit = false;
    void Init();
    std::map<StubType, StubNode> stubCollections_;
    std::vector<std::pair<StubType, std::u16string>> dumpCollections_ = {};
    std::map<StubType, std::vector<Dumper>> dumperTbl_;
    std::map<StubType, std::map<sptr<IRemoteObject>, pid_t>> stubMap_;
    AsyncExecutor executor_;

    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_SERVER_MANAGER_H