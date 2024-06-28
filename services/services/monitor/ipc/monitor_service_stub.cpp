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

#include "monitor_service_stub.h"
#include "monitor_server.h"
#include "media_log.h"
#include "media_errors.h"
#include "ipc_skeleton.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MonitorServiceStub"};
}

namespace OHOS {
namespace Media {
MonitorServiceStub::MonitorServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MonitorServiceStub::~MonitorServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

sptr<MonitorServiceStub> MonitorServiceStub::GetInstance()
{
    static sptr<MonitorServiceStub> monitor = nullptr;
    if (monitor == nullptr) {
        monitor = new(std::nothrow) MonitorServiceStub();
        (void)monitor->Init();
    }
    return monitor;
}

int32_t MonitorServiceStub::Init()
{
    monitorFuncs_[MONITOR_CLICK] = &MonitorServiceStub::Click;
    monitorFuncs_[MONITOR_ENABLE] = &MonitorServiceStub::EnableMonitor;
    monitorFuncs_[MONITOR_DISABLE] = &MonitorServiceStub::DisableMonitor;
    return MSERR_OK;
}

int32_t MonitorServiceStub::DumpInfo(int32_t fd, bool needDetail)
{
    return MonitorServer::GetInstance().Dump(fd, needDetail);
}

int32_t MonitorServiceStub::OnClientDie(int32_t pid)
{
    return MonitorServer::GetInstance().OnClientDie(pid);
}

int MonitorServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    MEDIA_LOGD("Stub: OnRemoteRequest of code: %{public}d is received", code);

    auto remoteDescriptor = data.ReadInterfaceToken();
    CHECK_AND_RETURN_RET_LOG(MonitorServiceStub::GetDescriptor() == remoteDescriptor,
        MSERR_INVALID_OPERATION, "Invalid descriptor");

    auto itFunc = monitorFuncs_.find(code);
    CHECK_AND_RETURN_RET_LOG(itFunc != monitorFuncs_.end(), IPCObjectStub::OnRemoteRequest(code, data, reply, option),
        "MonitorServiceStub: no member func supporting, applying default process");

    auto memberFunc = itFunc->second;
    CHECK_AND_RETURN_RET_LOG(memberFunc != nullptr, IPCObjectStub::OnRemoteRequest(code, data, reply, option),
        "MonitorServiceStub: no member func supporting, applying default process");

    int32_t ret = (this->*memberFunc)(data, reply);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_OK, "calling memberFunc is failed.");
    return MSERR_OK;
}

int32_t MonitorServiceStub::Click()
{
    int32_t pid = IPCSkeleton::GetCallingPid();
    return MonitorServer::GetInstance().Click(pid);
}

int32_t MonitorServiceStub::EnableMonitor()
{
    int32_t pid = IPCSkeleton::GetCallingPid();
    return MonitorServer::GetInstance().EnableMonitor(pid);
}

int32_t MonitorServiceStub::DisableMonitor()
{
    int32_t pid = IPCSkeleton::GetCallingPid();
    return MonitorServer::GetInstance().DisableMonitor(pid);
}

int32_t MonitorServiceStub::Click(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Click());
    return MSERR_OK;
}

int32_t MonitorServiceStub::EnableMonitor(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(EnableMonitor());
    return MSERR_OK;
}

int32_t MonitorServiceStub::DisableMonitor(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(DisableMonitor());
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
