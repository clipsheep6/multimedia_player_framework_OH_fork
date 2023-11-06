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

#ifndef HELPER_LISTENER_PROXY_H
#define HELPER_LISTENER_PROXY_H

#include "i_standard_helper_listener.h"
#include "media_death_recipient.h"
#include "avmetadatahelper_server.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class HelperListenerCallback : public HelperCallback, public NoCopyable {
public:
    explicit HelperListenerCallback(const sptr<IStandardHelperListener> &listener);
    virtual ~HelperListenerCallback();

    void OnError(int32_t errorCode, const std::string &errorMsg) override;
    void OnInfo(HelperOnInfoType type, int32_t extra, const Format &infoBody = {}) override;

private:
    sptr<IStandardHelperListener> listener_ = nullptr;
};

class HelperListenerProxy : public IRemoteProxy<IStandardHelperListener>, public NoCopyable {
public:
    explicit HelperListenerProxy(const sptr<IRemoteObject> &impl);
    virtual ~HelperListenerProxy();

    void OnError(int32_t errorCode, const std::string &errorMsg) override;
    void OnInfo(HelperOnInfoType type, int32_t extra, const Format &infoBody = {}) override;

private:
    int32_t SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option);
    static inline BrokerDelegator<HelperListenerProxy> delegator_;
};
} // namespace Media
} // namespace OHOS
#endif // HELPER_LISTENER_PROXY_H
