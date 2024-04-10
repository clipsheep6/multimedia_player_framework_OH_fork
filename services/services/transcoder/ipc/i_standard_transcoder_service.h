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

#ifndef I_STANDARD_TRANSCODER_SERVICE_H
#define I_STANDARD_TRANSCODER_SERVICE_H

#include "ipc_types.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "transcoder.h"

namespace OHOS {
namespace Media {
class IStandardTransCoderService : public IRemoteBroker {
public:
    virtual ~IStandardTransCoderService() = default;
    virtual int32_t SetListenerObject(const sptr<IRemoteObject> &object) = 0;
    virtual int32_t SetVideoEncoder(VideoCodecFormat encoder) = 0;
    virtual int32_t SetVideoSize(int32_t width, int32_t height) = 0;
    virtual int32_t SetVideoEncodingBitRate(int32_t rate) = 0;
    virtual int32_t SetAudioEncoder(AudioCodecFormat encoder) = 0;
    virtual int32_t SetAudioEncodingBitRate(int32_t bitRate) = 0;
    virtual int32_t SetOutputFormat(OutputFormatType format) = 0;
    virtual int32_t SetInputFile(int32_t fd) = 0;
    virtual int32_t SetOutputFile(int32_t fd) = 0;
    virtual int32_t Prepare() = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Pause() = 0;
    virtual int32_t Resume() = 0;
    virtual int32_t Cancel() = 0;
    virtual int32_t Release() = 0;
    virtual int32_t DestroyStub() = 0;

    /**
     * IPC code ID
     */
    enum RecorderServiceMsg {
        SET_LISTENER_OBJ = 0,
        SET_VIDEO_ENCODER,
        SET_VIDEO_SIZE,
        SET_VIDEO_ENCODING_BIT_RATE,
        SET_AUDIO_ENCODER,
        SET_AUDIO_ENCODING_BIT_RATE,
        SET_OUTPUT_FORMAT,
        SET_INPUT_FILE,
        SET_OUTPUT_FILE,
        PREPARE,
        START,
        PAUSE,
        RESUME,
        CANCEL,
        RELEASE,
        DESTROY,
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardTransCoderService");
};
} // namespace Media
} // namespace OHOS
#endif // I_STANDARD_RECORDER_SERVICE_H
