/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "screen_capture_controller_proxy.h"
#include "media_log.h"
#include "media_errors.h"
#include "avsharedmemory_ipc.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ScreenCaptureControllerProxy"};
}

namespace OHOS {
namespace Media {

ScreenCaptureControllerProxy::ScreenCaptureControllerProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardScreenCaptureController>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

ScreenCaptureControllerProxy::~ScreenCaptureControllerProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t ScreenCaptureControllerProxy::DestroyStub()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureControllerProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    int error = Remote()->SendRequest(DESTROY, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "DestroyStub failed, error: %{public}d", error);

    return reply.ReadInt32();
}

int32_t ScreenCaptureControllerProxy::ReportAVScreenCaptureUserChoice(int32_t sessionId, std::string choice)
{
    MEDIA_LOGI("ScreenCaptureControllerProxy::ReportAVScreenCaptureUserChoice start");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(ScreenCaptureControllerProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write descriptor!");

    token = data.WriteInt32(sessionId);
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write sessionId!");

    token = data.WriteString(choice);
    CHECK_AND_RETURN_RET_LOG(token, MSERR_INVALID_OPERATION, "Failed to write choice!");

    int error = Remote()->SendRequest(REPORT_USER_CHOICE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, MSERR_INVALID_OPERATION,
        "ReportAVScreenCaptureUserChoice failed, error: %{public}d", error);

    return reply.ReadInt32();
}


} // namespace Media
} // namespace OHOS