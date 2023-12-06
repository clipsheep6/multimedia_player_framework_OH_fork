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
#ifndef AUDIO_CHANGEINFO_PROXY_STUB_H
#define AUDIO_CHANGEINFO_PROXY_STUB_H

#include "ipc_types.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "recorder.h"

namespace OHOS {
namespace Media {
class AudioChangeInfoProxyStub : public IRemoteBroker {
public:
    static bool WriteAudioRecordChangeInfoParcel(Parcel &parcel, AudioRecordChangeInfo audioRecordChangeInfo)
    {
        return parcel.WriteInt32(audioRecordChangeInfo.createrUID)
            && parcel.WriteInt32(audioRecordChangeInfo.clientUID)
            && parcel.WriteInt32(audioRecordChangeInfo.sessionId)
            && WriteAudioRecordInfoParcel(parcel, audioRecordChangeInfo.aRecorderInfo)
            && parcel.WriteInt32(static_cast<int32_t>(audioRecordChangeInfo.aRecorderState))
            && WriteDeviceInfoParcel(parcel, audioRecordChangeInfo.inputDeviceInfo)
            && parcel.WriteBool(audioRecordChangeInfo.muted);
    }

    static bool WriteAudioRecordInfoParcel(Parcel &parcel, AudioRecordInfo audiorecordInfo)
    {
        return parcel.WriteInt32(static_cast<int32_t>(audiorecordInfo.inputSource))
            && parcel.WriteInt32(audiorecordInfo.capturerFlags);
    }

    static bool WriteDeviceInfoParcel(Parcel &parcel, DeviceInfo deviceInfo)
    {
        return parcel.WriteInt32(static_cast<int32_t>(deviceInfo.deviceType))
            && parcel.WriteInt32(static_cast<int32_t>(deviceInfo.deviceRole))
            && parcel.WriteInt32(deviceInfo.deviceId)
            && parcel.WriteInt32(deviceInfo.channelMasks)
            && parcel.WriteInt32(deviceInfo.channelIndexMasks)
            && parcel.WriteString(deviceInfo.deviceName)
            && parcel.WriteString(deviceInfo.macAddress)
            && WriteAudioStreamInfoParcel(parcel, deviceInfo.audioStreamInfo)
            && parcel.WriteString(deviceInfo.networkId)
            && parcel.WriteString(deviceInfo.displayName)
            && parcel.WriteInt32(deviceInfo.interruptGroupId)
            && parcel.WriteInt32(deviceInfo.volumeGroupId)
            && parcel.WriteBool(deviceInfo.isLowLatencyDevice);
    }

    static bool WriteAudioStreamInfoParcel(Parcel &parcel, AudioStreamInfo audioStreamInfo)
    {
        return parcel.WriteInt32(audioStreamInfo.samplingRate)
            && parcel.WriteInt32(static_cast<int32_t>(audioStreamInfo.encoding))
            && parcel.WriteInt32(static_cast<int32_t>(audioStreamInfo.format))
            && parcel.WriteInt32(audioStreamInfo.channels);
    }

    static void ReadAudioRecordChangeInfoParcel(Parcel &parcel, AudioRecordChangeInfo &audioRecordChangeInfo)
    {
        audioRecordChangeInfo.createrUID = parcel.ReadInt32();
        audioRecordChangeInfo.clientUID = parcel.ReadInt32();
        audioRecordChangeInfo.sessionId = parcel.ReadInt32();
        ReadAudioRecordInfoParcel(parcel, audioRecordChangeInfo.aRecorderInfo);
        audioRecordChangeInfo.aRecorderState = static_cast<AudioReorderState>(parcel.ReadInt32());
        ReadDeviceInfoParcel(parcel, audioRecordChangeInfo.inputDeviceInfo);
        audioRecordChangeInfo.muted = parcel.ReadBool();
    }

    static void ReadAudioRecordInfoParcel(Parcel &parcel, AudioRecordInfo &audiorecordInfo)
    {
        audiorecordInfo.inputSource = static_cast<AudioSourceType>(parcel.ReadInt32());
        audiorecordInfo.capturerFlags = parcel.ReadInt32();
    }

    static void ReadDeviceInfoParcel(Parcel &parcel, DeviceInfo &deviceInfo)
    {
        deviceInfo.deviceType = static_cast<DeviceType>(parcel.ReadInt32());
        deviceInfo.deviceRole = static_cast<DeviceRole>(parcel.ReadInt32());
        deviceInfo.deviceId = parcel.ReadInt32();
        deviceInfo.channelMasks = parcel.ReadInt32();
        deviceInfo.channelIndexMasks = parcel.ReadInt32();
        deviceInfo.deviceName = parcel.ReadString();
        deviceInfo.macAddress = parcel.ReadString();
        ReadAudioStreamInfoParcel(parcel, deviceInfo.audioStreamInfo);
        deviceInfo.networkId = parcel.ReadString();
        deviceInfo.displayName = parcel.ReadString();
        deviceInfo.interruptGroupId = parcel.ReadInt32();
        deviceInfo.volumeGroupId = parcel.ReadInt32();
        deviceInfo.isLowLatencyDevice = parcel.ReadBool();
    }

    static void ReadAudioStreamInfoParcel(Parcel &parcel, AudioStreamInfo &audioStreamInfo)
    {
        audioStreamInfo.samplingRate = parcel.ReadInt32();
        audioStreamInfo.encoding = static_cast<RecordAudioEncodingType>(parcel.ReadInt32());
        audioStreamInfo.format = static_cast<RecordAudioSampleFormat>(parcel.ReadInt32());
        audioStreamInfo.channels = parcel.ReadInt32();
    }
};
} // namespace Media
} // namespace OHOS
#endif // AUDIO_CHANGEINFO_PROXY_STUB_H
