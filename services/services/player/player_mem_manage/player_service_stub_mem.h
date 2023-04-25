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

#ifndef PLAYER_SERVICE_STUB_MEM_H
#define PLAYER_SERVICE_STUB_MEM_H

#include "player_service_stub.h"
#include "player_mem_manage.h"

namespace OHOS {
namespace Media {
class PlayerServiceStubMem : public PlayerServiceStub {
public:
    static sptr<PlayerServiceStub> Create();
    virtual ~PlayerServiceStubMem();
    int32_t SetSource(const std::string &url) override;
    int32_t SetSource(const sptr<IRemoteObject> &object) override;
    int32_t SetSource(int32_t fd, int64_t offset, int64_t size) override;
    int32_t DestroyStub() override;
    int32_t Release() override;
    void ResetFrontGroundForMemManageRecall();
    void ResetBackGroundForMemManageRecall();
    void ResetMemmgrForMemManageRecall();
    void RecoverByMemManageRecall();

private:
    PlayerServiceStubMem();
    int32_t Init() override;
    MemManageRecall memRecallStruct_;
    std::recursive_mutex recMutex_;
    bool isControlByMemManage_ = false;
};
} // namespace Media
} // namespace OHOS
#endif // PLAYER_SERVICE_STUB_H