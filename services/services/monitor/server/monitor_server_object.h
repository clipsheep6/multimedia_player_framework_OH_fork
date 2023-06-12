/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
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

#ifndef MONITOR_SERVER_OBJECT_H
#define MONITOR_SERVER_OBJECT_H

#include <mutex>
#include "refbase.h"

namespace OHOS {
namespace Media {
class MonitorServerObject : public virtual RefBase {
public:
    virtual ~MonitorServerObject() = default;

    virtual int32_t DoIpcAbnormality() = 0;
    virtual int32_t DoIpcRecovery(bool fromMonitor) = 0;
    int32_t IpcAbnormality();
    int32_t IpcRecovery(bool fromMonitor);
    void SetIpcAlarmedFlag();
    void UnSetIpcAlarmedFlag();

protected:
    int32_t RegisterMonitor(int32_t pid);
    int32_t CancellationMonitor(int32_t pid);

    std::mutex monitorMutex_;
    bool alarmed_ = false;
};
} // namespace Media
} // namespace OHOS
#endif // MONITOR_SERVER_OBJECT_H
