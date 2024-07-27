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

#include "screen_capture_monitor_client.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SCREENCAPTURE, "ScreenCaptureMonitorClient"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<ScreenCaptureMonitorClient> ScreenCaptureMonitorClient::Create(
    const sptr<IStandardScreenCaptureMonitorService> &ipcProxy)
{
    CHECK_AND_RETURN_RET_LOG(ipcProxy != nullptr, nullptr, "ipcProxy is nullptr..");
    std::shared_ptr<ScreenCaptureMonitorClient> scmClient = std::make_shared<ScreenCaptureMonitorClient>(ipcProxy);
    CHECK_AND_RETURN_RET_LOG(scmClient != nullptr, nullptr, "failed to new ScreenCaptureMonitorClient..");
    int32_t ret = scmClient->CreateListenerObject();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to create listener object..");
    return scmClient;
}

ScreenCaptureMonitorClient::ScreenCaptureMonitorClient(const sptr<IStandardScreenCaptureMonitorService> &ipcProxy)
    : screenCaptureMonitorProxy_(ipcProxy)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

ScreenCaptureMonitorClient::~ScreenCaptureMonitorClient()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (screenCaptureMonitorProxy_ != nullptr) {
            (void)screenCaptureMonitorProxy_->DestroyStub();
            screenCaptureMonitorProxy_ = nullptr;
        }
        screenCaptureMonitorClientCallbacks_.clear();
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void ScreenCaptureMonitorClient::MediaServerDied()
{
    std::lock_guard<std::mutex> lock(mutex_);
    screenCaptureMonitorProxy_ = nullptr;
    listenerStub_ = nullptr;
}

int32_t ScreenCaptureMonitorClient::CreateListenerObject()
{
    std::lock_guard<std::mutex> lock(mutex_);
    listenerStub_ = new(std::nothrow) ScreenCaptureMonitorListenerStub();
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, MSERR_NO_MEMORY,
        "failed to new ScreenCaptureMonitorListenerStub object");
    CHECK_AND_RETURN_RET_LOG(screenCaptureMonitorProxy_ != nullptr, MSERR_NO_MEMORY,
        "ScreenCaptureMonitor service does not exist.");
    sptr<IRemoteObject> object = listenerStub_->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_NO_MEMORY, "listener object is nullptr.");
    MEDIA_LOGD("SetListenerObject");
    listenerStubIPCExist_ = true;
    return screenCaptureMonitorProxy_->SetListenerObject(object);
}

int32_t ScreenCaptureMonitorClient::CloseListenerObject()
{
    listenerStubIPCExist_ = false;
    MEDIA_LOGD("CloseListenerObject");
    return screenCaptureMonitorProxy_->CloseListenerObject();
}

int32_t ScreenCaptureMonitorClient::IsScreenCaptureWorking()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(screenCaptureMonitorProxy_ != nullptr, MSERR_NO_MEMORY,
        "ScreenCaptureMonitor service does not exist.");
    return screenCaptureMonitorProxy_->IsScreenCaptureWorking();
}

void ScreenCaptureMonitorClient::RegisterScreenCaptureMonitorListener(
    sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> listener)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!listenerStubIPCExist_) {
        CreateListenerObject();
    }
    CHECK_AND_RETURN_LOG(listener != nullptr, "input param listener is nullptr.");
    CHECK_AND_RETURN_LOG(listenerStub_ != nullptr, "listenerStub_ is nullptr.");
    MEDIA_LOGD("RegisterScreenCaptureMonitorListener");
    screenCaptureMonitorClientCallbacks_.insert(listener);
    listenerStub_->RegisterScreenCaptureMonitorListener(listener);
}

void ScreenCaptureMonitorClient::UnregisterScreenCaptureMonitorListener(
    sptr<ScreenCaptureMonitor::ScreenCaptureMonitorListener> listener)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(listener != nullptr, "input param listener is nullptr.");
    CHECK_AND_RETURN_LOG(listenerStub_ != nullptr, "listenerStub_ is nullptr.");
    MEDIA_LOGD("UnregisterScreenCaptureMonitorListener");
    listenerStub_->UnregisterScreenCaptureMonitorListener(listener);
    screenCaptureMonitorClientCallbacks_.erase(listener);
    if (listenerStubIPCExist_ && screenCaptureMonitorClientCallbacks_.size() == 0) {
        MEDIA_LOGD("No listener left,CloseListenerObject");
        CloseListenerObject();
    }
}
} // namespace Media
} // namespace OHOS