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

#ifndef TRANSCODER_LISTENER_STUB_H
#define TRANSCODER_LISTENER_STUB_H

#include "i_standard_transcoder_listener.h"
#include "transcoder.h"
#include "monitor_client_object.h"

namespace OHOS {
namespace Media {
class TransCoderListenerStub : public IRemoteStub<IStandardTransCoderListener> {
public:
    TransCoderListenerStub();
    virtual ~TransCoderListenerStub();
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    void OnError(int32_t errorCode, const std::string &errorMsg) override;
    void OnInfo(int32_t type, int32_t extra) override;
    void SetTransCoderCallback(const std::shared_ptr<TransCoderCallback> &callback);
    void SetMonitor(const std::weak_ptr<MonitorClientObject> &monitor);

private:
    std::shared_ptr<TransCoderCallback> callback_ = nullptr;
    std::weak_ptr<MonitorClientObject> monitor_;
};
} // namespace Media
} // namespace OHOS
#endif // TRANSCODER_LISTENER_STUB_H
