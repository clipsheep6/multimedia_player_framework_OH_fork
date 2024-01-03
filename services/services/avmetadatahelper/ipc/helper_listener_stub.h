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

#ifndef HELPER_LISTENER_STUB_H
#define HELPER_LISTENER_STUB_H

#include "i_standard_helper_listener.h"
#include "avmetadatahelper.h"
#include "meta/format.h"

namespace OHOS {
namespace Media {
class HelperListenerStub : public IRemoteStub<IStandardHelperListener> {
public:
    HelperListenerStub();
    virtual ~HelperListenerStub();
    // IStandardHelperListener override
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    void OnError(HelperErrorType errorType, int32_t errorCode) override;
    void OnError(int32_t errorCode, const std::string &errorMsg) override;
    void OnInfo(HelperOnInfoType type, int32_t extra, const Format &infoBody = {}) override;

    // HelperListenerStub
    void SetHelperCallback(const std::weak_ptr<HelperCallback> &callback);

private:
    std::weak_ptr<HelperCallback> callback_;
};
} // namespace Media
} // namespace OHOS
#endif // HELPER_LISTENER_STUB_H
