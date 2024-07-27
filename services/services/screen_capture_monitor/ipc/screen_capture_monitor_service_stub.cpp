/*
* Copyright (C) 2024 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "screen_capture_monitor_service_stub.h"
#include <unistd.h>
#include "screen_capture_monitor_listener_proxy.h"
#include "media_server_manager.h"
#include "media_log.h"
#include "media_errors.h"
#include "ipc_skeleton.h"
#include "media_permission.h"
#include "accesstoken_kit.h"
#include "media_dfx.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SCREENCAPTURE, "ScreenCaptureMonitorServiceStub"};
}

namespace OHOS {
namespace Media {
sptr<ScreenCaptureMonitorServiceStub> ScreenCaptureMonitorServiceStub::Create()
{
    sptr<ScreenCaptureMonitorServiceStub> screenCaptureMonitorStub =
        new(std::nothrow) ScreenCaptureMonitorServiceStub();
    CHECK_AND_RETURN_RET_LOG(screenCaptureMonitorStub != nullptr, nullptr,
        "failed to new ScreenCaptureMonitorServiceStub");

    int32_t ret = screenCaptureMonitorStub->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to screen capture monitor stub init");
    StatisticEventWriteBundleName("create", "ScreenCaptureMonitorServiceStub");
    return screenCaptureMonitorStub;
}

ScreenCaptureMonitorServiceStub::ScreenCaptureMonitorServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

ScreenCaptureMonitorServiceStub::~ScreenCaptureMonitorServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t ScreenCaptureMonitorServiceStub::Init()
{
    screenCaptureMonitorServer_ = ScreenCaptureMonitorServer::GetInstance();
    CHECK_AND_RETURN_RET_LOG(screenCaptureMonitorServer_ != nullptr, MSERR_NO_MEMORY,
        "failed to create ScreenCaptureMonitorServer");
    screenCaptureMonitorStubFuncs_[SET_LISTENER_OBJ] = &ScreenCaptureMonitorServiceStub::SetListenerObject;
    screenCaptureMonitorStubFuncs_[IS_SCREEN_CAPTURE_WORKING] =
        &ScreenCaptureMonitorServiceStub::IsScreenCaptureWorking;
    screenCaptureMonitorStubFuncs_[DESTROY] = &ScreenCaptureMonitorServiceStub::DestroyStub;
    screenCaptureMonitorStubFuncs_[CLOSE_LISTENER_OBJ] = &ScreenCaptureMonitorServiceStub::CloseListenerObject;

    return MSERR_OK;
}

int32_t ScreenCaptureMonitorServiceStub::DestroyStub()
{
    CloseListenerObject();
    MediaServerManager::GetInstance().DestroyStubObject(MediaServerManager::SCREEN_CAPTURE_MONITOR, AsObject());
    return MSERR_OK;
}

int ScreenCaptureMonitorServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    MEDIA_LOGI("Stub: OnRemoteRequest of code: %{public}d is received", code);
    auto remoteDescriptor = data.ReadInterfaceToken();
    CHECK_AND_RETURN_RET_LOG(ScreenCaptureMonitorServiceStub::GetDescriptor() == remoteDescriptor,
        MSERR_INVALID_OPERATION, "Invalid descriptor");
    auto itFunc = screenCaptureMonitorStubFuncs_.find(code);
    if (itFunc != screenCaptureMonitorStubFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            std::lock_guard<std::mutex> lock(mutex_);
            int32_t ret = (this->*memberFunc)(data, reply);
            if (ret != MSERR_OK) {
                MEDIA_LOGE("calling memberFunc is failed.");
            }
        return MSERR_OK;
    }
}
MEDIA_LOGW("ScreenCaptureMonitorServiceStub: no member func supporting, applying default process");
return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t ScreenCaptureMonitorServiceStub::SetListenerObject(const sptr<IRemoteObject> &object)
{
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_NO_MEMORY, "set listener object is nullptr");
    sptr<IStandardScreenCaptureMonitorListener> listener = iface_cast<IStandardScreenCaptureMonitorListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, MSERR_NO_MEMORY,
        "failed to convert IStandardScreenCaptureMonitorListener");
    sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> callback =
        new ScreenCaptureMonitorListenerCallback(listener);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_NO_MEMORY,
        "failed to new ScreenCaptureMonitorListenerCallback");
    CHECK_AND_RETURN_RET_LOG(screenCaptureMonitorServer_ != nullptr, MSERR_NO_MEMORY,
        "screen capture monitor server is nullptr");
    screenCaptureMonitorCallback_ = callback;
    (void)screenCaptureMonitorServer_->SetScreenCaptureMonitorCallback(callback);
    return MSERR_OK;
}

int32_t ScreenCaptureMonitorServiceStub::CloseListenerObject()
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureMonitorCallback_ != nullptr, MSERR_OK,
        "screenCaptureMonitorCallback_ is nullptr");
    (void)screenCaptureMonitorServer_->RemoveScreenCaptureMonitorCallback(screenCaptureMonitorCallback_);
    screenCaptureMonitorCallback_ = nullptr;
    return MSERR_OK;
}

int32_t ScreenCaptureMonitorServiceStub::IsScreenCaptureWorking()
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureMonitorServer_ != nullptr, MSERR_NO_MEMORY,
        "screen capture monitor server is nullptr");
    return screenCaptureMonitorServer_->IsScreenCaptureWorking();
}

int32_t ScreenCaptureMonitorServiceStub::SetListenerObject(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    reply.WriteInt32(SetListenerObject(object));
    return MSERR_OK;
}

int32_t ScreenCaptureMonitorServiceStub::CloseListenerObject(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(CloseListenerObject());
    return MSERR_OK;
}

int32_t ScreenCaptureMonitorServiceStub::IsScreenCaptureWorking(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(IsScreenCaptureWorking());
    return MSERR_OK;
}

int32_t ScreenCaptureMonitorServiceStub::DestroyStub(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(DestroyStub());
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS