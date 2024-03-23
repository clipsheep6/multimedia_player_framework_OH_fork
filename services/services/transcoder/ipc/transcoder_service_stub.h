/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef TRANSCODER_SERVICE_STUB_H
#define TRANSCODER_SERVICE_STUB_H

#include <map>
#include <set>
#include "i_standard_transcoder_service.h"
#include "i_standard_transcoder_listener.h"
#include "media_death_recipient.h"
#include "transcoder_server.h"
#include "nocopyable.h"
#include "monitor_server_object.h"

namespace OHOS {
namespace Media {
class TransCoderServiceStub : public IRemoteStub<IStandardTransCoderService>,
    public MonitorServerObject, public NoCopyable {
public:
    static sptr<TransCoderServiceStub> Create();
    virtual ~TransCoderServiceStub();

    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    using TransCoderStubFunc = int32_t(TransCoderServiceStub::*)(MessageParcel &data, MessageParcel &reply);
    int32_t SetListenerObject(const sptr<IRemoteObject> &object) override;
    int32_t SetVideoEncoder(VideoCodecFormat encoder) override;
    int32_t SetVideoSize(int32_t width, int32_t height) override;
    int32_t SetVideoEncodingBitRate(int32_t rate) override;
    int32_t SetAudioEncoder(AudioCodecFormat encoder) override;
    int32_t SetAudioEncodingBitRate(int32_t bitRate) override;
    int32_t SetOutputFormat(OutputFormatType format) override;
    int32_t SetInputFile(int32_t fd) override;
    int32_t SetOutputFile(int32_t fd) override;
    int32_t Prepare() override;
    int32_t Start() override;
    int32_t Pause() override;
    int32_t Resume() override;
    int32_t Cancel() override;
    int32_t Release() override;
    int32_t DestroyStub() override;

    // MonitorServerObject override
    int32_t DoIpcAbnormality() override;
    int32_t DoIpcRecovery(bool fromMonitor) override;

private:
    TransCoderServiceStub();
    int32_t Init();
    int32_t SetListenerObject(MessageParcel &data, MessageParcel &reply);
    int32_t SetVideoEncoder(MessageParcel &data, MessageParcel &reply);
    int32_t SetVideoSize(MessageParcel &data, MessageParcel &reply);
    int32_t SetVideoEncodingBitRate(MessageParcel &data, MessageParcel &reply);
    int32_t SetAudioEncoder(MessageParcel &data, MessageParcel &reply);
    int32_t SetAudioEncodingBitRate(MessageParcel &data, MessageParcel &reply);
    int32_t SetOutputFormat(MessageParcel &data, MessageParcel &reply);
    int32_t SetInputFile(MessageParcel &data, MessageParcel &reply);
    int32_t SetOutputFile(MessageParcel &data, MessageParcel &reply);
    int32_t Prepare(MessageParcel &data, MessageParcel &reply);
    int32_t Start(MessageParcel &data, MessageParcel &reply);
    int32_t Pause(MessageParcel &data, MessageParcel &reply);
    int32_t Resume(MessageParcel &data, MessageParcel &reply);
    int32_t Cancel(MessageParcel &data, MessageParcel &reply);
    int32_t Release(MessageParcel &data, MessageParcel &reply);
    int32_t DestroyStub(MessageParcel &data, MessageParcel &reply);

    std::shared_ptr<ITransCoderService> transCoderServer_ = nullptr;
    std::map<uint32_t, TransCoderStubFunc> recFuncs_;
    std::mutex mutex_;
    int32_t pid_;
};
} // namespace Media
} // namespace OHOS
#endif // RECORDER_SERVICE_STUB_H
