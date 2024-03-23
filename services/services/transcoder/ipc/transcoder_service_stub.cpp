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

#include "transcoder_service_stub.h"
#include <unistd.h>
#include "transcoder_listener_proxy.h"
#include "media_server_manager.h"
#include "media_log.h"
#include "media_errors.h"
#include "ipc_skeleton.h"
#include "media_permission.h"
#include "accesstoken_kit.h"
#include "media_dfx.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "TransCoderServiceStub"};
}

namespace OHOS {
namespace Media {
sptr<TransCoderServiceStub> TransCoderServiceStub::Create()
{
    sptr<TransCoderServiceStub> transCoderStub = new(std::nothrow) TransCoderServiceStub();
    CHECK_AND_RETURN_RET_LOG(transCoderStub != nullptr, nullptr, "failed to new TransCoderServiceStub");

    int32_t ret = transCoderStub->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to transcoder stub init");
    StatisticEventWriteBundleName("create", "TransCoderServiceStub");
    return transCoderStub;
}

TransCoderServiceStub::TransCoderServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

TransCoderServiceStub::~TransCoderServiceStub()
{
    (void)CancellationMonitor(pid_);
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t TransCoderServiceStub::Init()
{
    transCoderServer_ = TransCoderServer::Create();
    CHECK_AND_RETURN_RET_LOG(transCoderServer_ != nullptr, MSERR_NO_MEMORY, "failed to create TransCoderServer");

    recFuncs_[SET_LISTENER_OBJ] = &TransCoderServiceStub::SetListenerObject;
    recFuncs_[SET_VIDEO_ENCODER] = &TransCoderServiceStub::SetVideoEncoder;
    recFuncs_[SET_VIDEO_SIZE] = &TransCoderServiceStub::SetVideoSize;
    recFuncs_[SET_VIDEO_ENCODING_BIT_RATE] = &TransCoderServiceStub::SetVideoEncodingBitRate;
    recFuncs_[SET_AUDIO_ENCODER] = &TransCoderServiceStub::SetAudioEncoder;
    recFuncs_[SET_AUDIO_ENCODING_BIT_RATE] = &TransCoderServiceStub::SetAudioEncodingBitRate;
    recFuncs_[SET_OUTPUT_FORMAT] = &TransCoderServiceStub::SetOutputFormat;
    recFuncs_[SET_INPUT_FILE] = &TransCoderServiceStub::SetInputFile;
    recFuncs_[SET_OUTPUT_FILE] = &TransCoderServiceStub::SetOutputFile;
    recFuncs_[PREPARE] = &TransCoderServiceStub::Prepare;
    recFuncs_[START] = &TransCoderServiceStub::Start;
    recFuncs_[PAUSE] = &TransCoderServiceStub::Pause;
    recFuncs_[RESUME] = &TransCoderServiceStub::Resume;
    recFuncs_[CANCEL] = &TransCoderServiceStub::Cancel;
    recFuncs_[RELEASE] = &TransCoderServiceStub::Release;
    recFuncs_[DESTROY] = &TransCoderServiceStub::DestroyStub;

    pid_ = IPCSkeleton::GetCallingPid();
    (void)RegisterMonitor(pid_);
    return MSERR_OK;
}

int32_t TransCoderServiceStub::DestroyStub()
{
    transCoderServer_ = nullptr;
    MediaServerManager::GetInstance().DestroyStubObject(MediaServerManager::TRANSCODER, AsObject());
    return MSERR_OK;
}

int TransCoderServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    MEDIA_LOGI("Stub: OnRemoteRequest of code: %{public}d is received", code);

    auto remoteDescriptor = data.ReadInterfaceToken();
    CHECK_AND_RETURN_RET_LOG(TransCoderServiceStub::GetDescriptor() == remoteDescriptor,
        MSERR_INVALID_OPERATION, "Invalid descriptor");

    auto itFunc = recFuncs_.find(code);
    if (itFunc != recFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            std::lock_guard<std::mutex> lock(mutex_);
            (void)IpcRecovery(false);
            int32_t ret = (this->*memberFunc)(data, reply);
            if (ret != MSERR_OK) {
                MEDIA_LOGE("calling memberFunc is failed.");
            }
            return MSERR_OK;
        }
    }
    MEDIA_LOGW("TransCoderServiceStub: no member func supporting, applying default process");

    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t TransCoderServiceStub::SetListenerObject(const sptr<IRemoteObject> &object)
{
    CHECK_AND_RETURN_RET_LOG(object != nullptr, MSERR_NO_MEMORY, "set listener object is nullptr");

    sptr<IStandardTransCoderListener> listener = iface_cast<IStandardTransCoderListener>(object);
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, MSERR_NO_MEMORY, "failed to convert IStandardTransCoderListener");

    std::shared_ptr<TransCoderCallback> callback = std::make_shared<TransCoderListenerCallback>(listener);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_NO_MEMORY, "failed to new TransCoderListenerCallback");

    CHECK_AND_RETURN_RET_LOG(transCoderServer_ != nullptr, MSERR_NO_MEMORY, "transCoder server is nullptr");
    (void)transCoderServer_->SetTransCoderCallback(callback);
    return MSERR_OK;
}

int32_t TransCoderServiceStub::SetVideoEncoder(VideoCodecFormat encoder)
{
    CHECK_AND_RETURN_RET_LOG(transCoderServer_ != nullptr, MSERR_NO_MEMORY, "transcoder server is nullptr");
    return transCoderServer_->SetVideoEncoder(encoder);
}

int32_t TransCoderServiceStub::SetVideoSize(int32_t width, int32_t height)
{
    CHECK_AND_RETURN_RET_LOG(transCoderServer_ != nullptr, MSERR_NO_MEMORY, "transcoder server is nullptr");
    return transCoderServer_->SetVideoSize(width, height);
}

int32_t TransCoderServiceStub::SetVideoEncodingBitRate(int32_t rate)
{
    CHECK_AND_RETURN_RET_LOG(transCoderServer_ != nullptr, MSERR_NO_MEMORY, "transcoder server is nullptr");
    return transCoderServer_->SetVideoEncodingBitRate(rate);
}

int32_t TransCoderServiceStub::SetAudioEncoder(AudioCodecFormat encoder)
{
    CHECK_AND_RETURN_RET_LOG(transCoderServer_ != nullptr, MSERR_NO_MEMORY, "transcoder server is nullptr");
    return transCoderServer_->SetAudioEncoder(encoder);
}

int32_t TransCoderServiceStub::SetAudioEncodingBitRate(int32_t bitRate)
{
    CHECK_AND_RETURN_RET_LOG(transCoderServer_ != nullptr, MSERR_NO_MEMORY, "transcoder server is nullptr");
    return transCoderServer_->SetAudioEncodingBitRate(bitRate);
}

int32_t TransCoderServiceStub::SetOutputFormat(OutputFormatType format)
{
    CHECK_AND_RETURN_RET_LOG(transCoderServer_ != nullptr, MSERR_NO_MEMORY, "transcoder server is nullptr");
    return transCoderServer_->SetOutputFormat(format);
}

int32_t TransCoderServiceStub::SetInputFile(int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(transCoderServer_ != nullptr, MSERR_NO_MEMORY, "transcoder server is nullptr");
    return transCoderServer_->SetInputFile(fd);
}

int32_t TransCoderServiceStub::SetOutputFile(int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(transCoderServer_ != nullptr, MSERR_NO_MEMORY, "transcoder server is nullptr");
    return transCoderServer_->SetOutputFile(fd);
}

int32_t TransCoderServiceStub::Prepare()
{
    CHECK_AND_RETURN_RET_LOG(transCoderServer_ != nullptr, MSERR_NO_MEMORY, "transcoder server is nullptr");
    return transCoderServer_->Prepare();
}

int32_t TransCoderServiceStub::Start()
{
    CHECK_AND_RETURN_RET_LOG(transCoderServer_ != nullptr, MSERR_NO_MEMORY, "transcoder server is nullptr");
    return transCoderServer_->Start();
}

int32_t TransCoderServiceStub::Pause()
{
    CHECK_AND_RETURN_RET_LOG(transCoderServer_ != nullptr, MSERR_NO_MEMORY, "transcoder server is nullptr");
    return transCoderServer_->Pause();
}

int32_t TransCoderServiceStub::Resume()
{
    CHECK_AND_RETURN_RET_LOG(transCoderServer_ != nullptr, MSERR_NO_MEMORY, "transcoder server is nullptr");
    return transCoderServer_->Resume();
}

int32_t TransCoderServiceStub::Cancel()
{
    CHECK_AND_RETURN_RET_LOG(transCoderServer_ != nullptr, MSERR_NO_MEMORY, "transcoder server is nullptr");
    return transCoderServer_->Cancel();
}

int32_t TransCoderServiceStub::Release()
{
    CHECK_AND_RETURN_RET_LOG(transCoderServer_ != nullptr, MSERR_NO_MEMORY, "transcoder server is nullptr");
    return transCoderServer_->Release();
}

int32_t TransCoderServiceStub::DoIpcAbnormality()
{
    MEDIA_LOGI("Enter DoIpcAbnormality.");
    SetIpcAlarmedFlag();
    return MSERR_OK;
}

int32_t TransCoderServiceStub::DoIpcRecovery(bool fromMonitor)
{
    MEDIA_LOGI("Enter DoIpcRecovery %{public}d.", fromMonitor);
    UnSetIpcAlarmedFlag();
    return MSERR_OK;
}

int32_t TransCoderServiceStub::SetListenerObject(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    reply.WriteInt32(SetListenerObject(object));
    return MSERR_OK;
}

int32_t TransCoderServiceStub::SetVideoEncoder(MessageParcel &data, MessageParcel &reply)
{
    int32_t encoder = data.ReadInt32();
    VideoCodecFormat codecFormat = static_cast<VideoCodecFormat>(encoder);
    reply.WriteInt32(SetVideoEncoder(codecFormat));
    return MSERR_OK;
}

int32_t TransCoderServiceStub::SetVideoSize(MessageParcel &data, MessageParcel &reply)
{
    int32_t width = data.ReadInt32();
    int32_t height = data.ReadInt32();
    reply.WriteInt32(SetVideoSize(width, height));
    return MSERR_OK;
}

int32_t TransCoderServiceStub::SetVideoEncodingBitRate(MessageParcel &data, MessageParcel &reply)
{
    int32_t rate = data.ReadInt32();
    reply.WriteInt32(SetVideoEncodingBitRate(rate));
    return MSERR_OK;
}

int32_t TransCoderServiceStub::SetAudioEncoder(MessageParcel &data, MessageParcel &reply)
{;
    int32_t format = data.ReadInt32();
    AudioCodecFormat encoderFormat = static_cast<AudioCodecFormat>(format);
    reply.WriteInt32(SetAudioEncoder(encoderFormat));
    return MSERR_OK;
}

int32_t TransCoderServiceStub::SetAudioEncodingBitRate(MessageParcel &data, MessageParcel &reply)
{
    int32_t bitRate = data.ReadInt32();
    reply.WriteInt32(SetAudioEncodingBitRate(bitRate));
    return MSERR_OK;
}

int32_t TransCoderServiceStub::SetOutputFormat(MessageParcel &data, MessageParcel &reply)
{
    int32_t type = data.ReadInt32();
    OutputFormatType formatType = static_cast<OutputFormatType>(type);
    reply.WriteInt32(SetOutputFormat(formatType));
    return MSERR_OK;
}

int32_t TransCoderServiceStub::SetInputFile(MessageParcel &data, MessageParcel &reply)
{
    int32_t fd = data.ReadFileDescriptor();
    reply.WriteInt32(SetInputFile(fd));
    (void)::close(fd);
    return MSERR_OK;
}

int32_t TransCoderServiceStub::SetOutputFile(MessageParcel &data, MessageParcel &reply)
{
    int32_t fd = data.ReadFileDescriptor();
    reply.WriteInt32(SetOutputFile(fd));
    (void)::close(fd);
    return MSERR_OK;
}

int32_t TransCoderServiceStub::Prepare(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Prepare());
    return MSERR_OK;
}

int32_t TransCoderServiceStub::Start(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Start());
    return MSERR_OK;
}

int32_t TransCoderServiceStub::Pause(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Pause());
    return MSERR_OK;
}

int32_t TransCoderServiceStub::Resume(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Resume());
    return MSERR_OK;
}

int32_t TransCoderServiceStub::Cancel(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Cancel());
    return MSERR_OK;
}

int32_t TransCoderServiceStub::Release(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Release());
    return MSERR_OK;
}

int32_t TransCoderServiceStub::DestroyStub(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(DestroyStub());
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS
