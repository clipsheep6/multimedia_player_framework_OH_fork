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

#ifndef MONITOR_CLIENT_OBJECT_H
#define MONITOR_CLIENT_OBJECT_H

#include <memory>
#include <mutex>

namespace OHOS {
namespace Media {
class MonitorClientObject : public std::enable_shared_from_this<MonitorClientObject> {
public:
    virtual ~MonitorClientObject() = default;
    int32_t EnableMonitor();
    int32_t DisableMonitor();

protected:
    bool monitorEnable_ = false;
    std::mutex monitorMutex_;
};
} // namespace Media
} // namespace OHOS
#endif // MONITOR_CLIENT_OBJECT_H
