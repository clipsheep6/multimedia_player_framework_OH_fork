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

#ifndef TRANSCODER_SERVICE_CLIENT_H
#define TRANSCODER_SERVICE_CLIENT_H

#include <thread>
#include "i_transcoder_service.h"
#include "i_standard_transcoder_service.h"
#include "transcoder_listener_stub.h"
#include "monitor_client_object.h"

namespace OHOS {
namespace Media {
class TransCoderClient : public ITransCoderService, public MonitorClientObject {
public:
    static std::shared_ptr<TransCoderClient> Create(const sptr<IStandardTransCoderService> &ipcProxy);
    explicit TransCoderClient(const sptr<IStandardTransCoderService> &ipcProxy);
    ~TransCoderClient();
    // ITransCoderService override
    int32_t SetVideoEncoder(VideoCodecFormat encoder) override;
    int32_t SetVideoSize(int32_t width, int32_t height) override;
    int32_t SetVideoEncodingBitRate(int32_t rate) override;
    int32_t SetAudioEncoder(AudioCodecFormat encoder) override;
    int32_t SetAudioEncodingBitRate(int32_t bitRate) override;
    int32_t SetOutputFormat(OutputFormatType format) override;
    int32_t SetInputFile(int32_t fd, int64_t offset, int64_t size) override;
    int32_t SetOutputFile(int32_t fd) override;
    int32_t SetTransCoderCallback(const std::shared_ptr<TransCoderCallback> &callback) override;
    int32_t Prepare() override;
    int32_t Start() override;
    int32_t Pause() override;
    int32_t Resume() override;
    int32_t Cancel() override;
    int32_t Release() override;
    // TransCoderClient
    void MediaServerDied();

private:
    int32_t CreateListenerObject();
    int32_t ExecuteWhen(int32_t ret, bool ok);

    sptr<IStandardTransCoderService> transCoderProxy_ = nullptr;
    sptr<TransCoderListenerStub> listenerStub_ = nullptr;
    std::shared_ptr<TransCoderCallback> callback_ = nullptr;
    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS
#endif // TRANSCODER_SERVICE_CLIENT_H
