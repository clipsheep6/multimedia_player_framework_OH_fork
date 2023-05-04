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

#ifndef MONITOR_SERVICE_PROXY_H
#define MONITOR_SERVICE_PROXY_H

#include "i_standard_monitor_service.h"

namespace OHOS {
namespace Media {
class MonitorServiceProxy : public IRemoteProxy<IStandardMonitorService>, public NoCopyable {
public:
    explicit MonitorServiceProxy(const sptr<IRemoteObject> &impl);
    virtual ~MonitorServiceProxy();

    int32_t Click() override;
    int32_t EnableMonitor() override;
    int32_t DisableMonitor() override;

private:
    int32_t SendIpc(uint32_t code);
    static inline BrokerDelegator<MonitorServiceProxy> delegator_;
};
} // namespace Media
} // namespace OHOS
#endif // MONITOR_SERVICE_PROXY_H
