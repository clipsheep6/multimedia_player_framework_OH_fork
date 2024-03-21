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

#include "monitor_client_object.h"
#include "media_log.h"
#include "media_errors.h"
#include "monitor_client.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MonitorClientObject"};
}

namespace OHOS {
namespace Media {
int32_t MonitorClientObject::EnableMonitor()
{
#ifdef USE_MONITOR
    MEDIA_LOGI("0x%{public}06" PRIXPTR " EnableMonitor", FAKE_POINTER(this));
    std::lock_guard<std::mutex> lock(monitorMutex_);
    if (monitorEnable_) {
        return MSERR_OK;
    }

    std::shared_ptr<MonitorClient> monitor = MonitorClient::GetInstance();
    CHECK_AND_RETURN_RET_LOG(monitor != nullptr, MSERR_NO_MEMORY, "Failed to GetInstance!");
    int32_t ret = monitor->StartClick(this);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to StartClick!");
    monitorEnable_ = true;
    return ret;
#else
    return MSERR_OK;
#endif
}

int32_t MonitorClientObject::DisableMonitor()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " DisableMonitor", FAKE_POINTER(this));
    std::lock_guard<std::mutex> lock(monitorMutex_);
    if (!monitorEnable_) {
        return MSERR_OK;
    }

    std::shared_ptr<MonitorClient> monitor = MonitorClient::GetInstance();
    CHECK_AND_RETURN_RET_LOG(monitor != nullptr, MSERR_NO_MEMORY, "Failed to GetInstance!");
    int32_t ret = monitor->StopClick(this);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to StopClick!");
    monitorEnable_ = false;
    return ret;
}
} // namespace Media
} // namespace OHOS
