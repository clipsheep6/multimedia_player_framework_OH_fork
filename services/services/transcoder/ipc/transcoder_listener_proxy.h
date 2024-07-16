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

#ifndef TRANSCODER_LISTENER_PROXY_H
#define TRANSCODER_LISTENER_PROXY_H

#include "i_standard_transcoder_listener.h"
#include "media_death_recipient.h"
#include "transcoder.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class TransCoderListenerCallback : public TransCoderCallback, public NoCopyable {
public:
    explicit TransCoderListenerCallback(const sptr<IStandardTransCoderListener> &listener);
    virtual ~TransCoderListenerCallback();

    void OnError(int32_t errorCode, const std::string &errorMsg) override;
    void OnInfo(int32_t type, int32_t extra) override;

private:
    sptr<IStandardTransCoderListener> listener_ = nullptr;
};

class TransCoderListenerProxy : public IRemoteProxy<IStandardTransCoderListener>, public NoCopyable {
public:
    explicit TransCoderListenerProxy(const sptr<IRemoteObject> &impl);
    virtual ~TransCoderListenerProxy();

    void OnError(int32_t errorCode, const std::string &errorMsg) override;
    void OnInfo(int32_t type, int32_t extra) override;

private:
    static inline BrokerDelegator<TransCoderListenerProxy> delegator_;
};
}
} // namespace OHOS
#endif // TransCoder_LISTENER_PROXY_H
