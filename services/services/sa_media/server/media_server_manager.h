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
#include "service_stub_util.h"
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
#include "iremote_object.h"
#include "ipc_skeleton.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
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
class MediaServerManager : public NoCopyable {
public:
    static MediaServerManager &GetInstance();
    ~MediaServerManager();

    sptr<IRemoteObject> CreateStubObject(ServiceStubUtil &stubUtil);
    void DestroyStubObject(StubType type, sptr<IRemoteObject> object);
    void DestroyStubObjectForPid(pid_t pid);
    int32_t Dump(int32_t fd, const std::vector<std::u16string> &args);
    void DestroyDumper(StubType type, sptr<IRemoteObject> object);
    void DestroyDumperForPid(pid_t pid);
    std::vector<ServiceStubUtil> &GetServiceStubUtils();
    AsyncExecutor &GetAsyncExecutor();
private:
    MediaServerManager();
    AsyncExecutor executor_;

    std::vector<ServiceStubUtil> stubUtils_{
        ServiceStubUtil("Recorder", StubType::RECORDER,
            IStandardMediaService::MEDIA_RECORDER, RecorderServiceStub::Create),
        ServiceStubUtil("Player", StubType::PLAYER,
            IStandardMediaService::MEDIA_PLAYER, RecorderServiceStub::Create),
        ServiceStubUtil("AvMetaDataHelper", StubType::AVMETADATAHELPER,
            IStandardMediaService::MEDIA_AVMETADATAHELPER,RecorderServiceStub::Create),
        ServiceStubUtil("AVCodecList", StubType::AVCODECLIST,
            IStandardMediaService::MEDIA_CODECLIST,RecorderServiceStub::Create),
        ServiceStubUtil("Codec", StubType::AVCODEC,
            IStandardMediaService::MEDIA_AVCODEC,RecorderServiceStub::Create),
        ServiceStubUtil("RecorderProfiles", StubType::RECORDERPROFILES,
            IStandardMediaService::RECORDER_PROFILES,RecorderServiceStub::Create),
    };
    std::map<StubType, std::vector<Dumper>> dumperTbl_;

    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_SERVER_MANAGER_H