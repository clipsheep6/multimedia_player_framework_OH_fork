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
#include "service_stub_util.h"
#include "media_server_manager.h"
namespace OHOS {
namespace Media {

void ServiceStubUtil::Init()
{

}

std::string ServiceStubUtil::GetName() const
{
    return name_;
}

StubType ServiceStubUtil::GetStubType() const
{
    return type_;
}

IStandardMediaService::MediaSystemAbility ServiceStubUtil::GetAbility() const
{
    return ability_;
}

std::map<sptr<IRemoteObject>, pid_t> ServiceStubUtil::GetStubMap() const
{
    return stubMap_;
}

size_t ServiceStubUtil::GetStubMapSize() const
{
    return stubMap_.size();
}

void ServiceStubUtil::AddObject(sptr<IRemoteObject> object, pid_t pid)
{
    stubMap_[object] = pid;
}

bool ServiceStubUtil::DeleteStubObject(sptr<IRemoteObject> object)
{
    for (auto it = stubMap_.begin(); it != stubMap_.end(); it++) {
        if (it->first == object) {
            (void)stubMap_.erase(it);
            return true;
        }
    }
    return false;
}

void ServiceStubUtil::DeleteStubObjectForPid(pid_t pid)
{
    for (auto iter = stubMap_.begin(); iter != stubMap_.end();) {
        if (iter->second == pid) {
            MediaServerManager::GetInstance().GetAsyncExecutor().Commit(iter->first);
            iter = stubMap_.erase(iter);
        } else {
            ++iter; // less memory than iter++
        }
    }
}

sptr<IMediaStub> ServiceStubUtil::CreateStub()
{
    return create_();
}

std::vector<Dumper> ServiceStubUtil::GetDumpers() const
{
    return dumpers_;
}
} // namespace Media
} // namespace OHOS