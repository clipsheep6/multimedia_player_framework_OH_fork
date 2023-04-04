/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#ifndef SERVICE_STUB_UTIL_H
#define SERVICE_STUB_UTIL_H

#include <map>
#include <functional>
#include <string>
#include "i_media_stub.h"
#include "i_standard_media_service.h"
#include "stub_type.h"
#include "dumper.h"

namespace OHOS {
namespace Media {
class ServiceStubUtil {
public:
    ServiceStubUtil() = default;
    ServiceStubUtil(std::string name, StubType type, IStandardMediaService::MediaSystemAbility ability,
        std::function<sptr<IMediaStub>()> create) : name_(name), type_(type), ability_(ability),
        create_(create) {}

    std::string GetName() const;
    StubType GetStubType() const;
    IStandardMediaService::MediaSystemAbility GetAbility() const;
    std::map<sptr<IRemoteObject>, pid_t> GetStubMap() const;
    size_t GetStubMapSize() const;
    void AddObject(sptr<IRemoteObject> object, pid_t pid);
    bool DeleteStubObject(sptr<IRemoteObject> object);
    void DeleteStubObjectForPid(pid_t pid);
    sptr<IMediaStub> CreateStub();
    std::vector<Dumper> GetDumpers() const;
private:
    void Init();
    std::string name_;
    StubType type_ = StubType::PLAYER;
    IStandardMediaService::MediaSystemAbility ability_ = IStandardMediaService::MediaSystemAbility::MEDIA_PLAYER;
    std::map<sptr<IRemoteObject>, pid_t> stubMap_;
    std::function<sptr<IMediaStub>()> create_;
    std::vector<Dumper> dumpers_;
};
} // namespace Media
} // namespace OHOS
#endif