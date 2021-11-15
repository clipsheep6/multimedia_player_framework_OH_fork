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

#include "avcodec_service_proxy.h"
#include "avcodec_listener_stub.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecServiceProxy"};
}

namespace OHOS {
namespace Media {
AVCodecServiceProxy::AVCodecServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardAVCodecService>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecServiceProxy::~AVCodecServiceProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVCodecServiceProxy::SetListenerObject(const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    (void)data.WriteRemoteObject(object);
    int ret = Remote()->SendRequest(SET_LISTENER_OBJ, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Set listener obj failed, error: %{public}d", ret);
        return ret;
    }
    return reply.ReadInt32();
}


int32_t AVCodecServiceProxy::Configure(const Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    // todo
    int ret = Remote()->SendRequest(CONFIGURE, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Set listener obj failed, error: %{public}d", ret);
        return ret;
    }
    return reply.ReadInt32();
}

int32_t AVCodecServiceProxy::Prepare()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int ret = Remote()->SendRequest(PREPARE, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("release failed, error: %{public}d", ret);
        return ret;
    }
    return reply.ReadInt32();
}

int32_t AVCodecServiceProxy::Start()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int ret = Remote()->SendRequest(START, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("release failed, error: %{public}d", ret);
        return ret;
    }
    return reply.ReadInt32();
}

int32_t AVCodecServiceProxy::Stop()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int ret = Remote()->SendRequest(STOP, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("release failed, error: %{public}d", ret);
        return ret;
    }
    return reply.ReadInt32();
}

int32_t AVCodecServiceProxy::Flush()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int ret = Remote()->SendRequest(FLUSH, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("release failed, error: %{public}d", ret);
        return ret;
    }
    return reply.ReadInt32();
}

int32_t AVCodecServiceProxy::Reset()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int ret = Remote()->SendRequest(RESET, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("release failed, error: %{public}d", ret);
        return ret;
    }
    return reply.ReadInt32();
}

int32_t AVCodecServiceProxy::Release()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int ret = Remote()->SendRequest(RELEASE, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("release failed, error: %{public}d", ret);
        return ret;
    }
    return reply.ReadInt32();
}

sptr<OHOS::Surface> AVCodecServiceProxy::CreateInputSurface()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int ret = Remote()->SendRequest(CREATE_INPUT_SURFACE, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Create input surface failed, error: %{public}d", ret);
        return nullptr;
    }

    sptr<IRemoteObject> object = reply.ReadRemoteObject();
    if (object == nullptr) {
        MEDIA_LOGE("failed to read surface object");
        return nullptr;
    }

    sptr<IBufferProducer> producer = iface_cast<IBufferProducer>(object);
    if (producer == nullptr) {
        MEDIA_LOGE("failed to convert object to producer");
        return nullptr;
    }

    return OHOS::Surface::CreateSurfaceAsProducer(producer);
}

int32_t AVCodecServiceProxy::SetOutputSurface(sptr<OHOS::Surface> surface)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    // todo
    int ret = Remote()->SendRequest(SET_OUTPUT_SURFACE, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Set output surface failed, error: %{public}d", ret);
        return ret;
    }
    return reply.ReadInt32();
}

std::shared_ptr<AVSharedMemory> AVCodecServiceProxy::GetInputBuffer(uint32_t index)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(index);
    int ret = Remote()->SendRequest(GET_INPUT_BUFFER, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Get input buffer failed, error: %{public}d", ret);
        return nullptr;
    }
    // todo
    return nullptr;
}

int32_t AVCodecServiceProxy::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferType type, AVCodecBufferFlag flag)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    // todo
    int ret = Remote()->SendRequest(QUEUE_INPUT_BUFFER, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Queue input buffer failed, error: %{public}d", ret);
        return ret;
    }
    return reply.ReadInt32();
}

std::shared_ptr<AVSharedMemory> AVCodecServiceProxy::GetOutputBuffer(uint32_t index)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(index);
    int ret = Remote()->SendRequest(GET_OUTPUT_BUFFER, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Get output buffer failed, error: %{public}d", ret);
        return nullptr;
    }
    // todo
    return nullptr;
}

int32_t AVCodecServiceProxy::ReleaseOutputBuffer(uint32_t index, bool render = false)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    // todo
    int ret = Remote()->SendRequest(RELEASE_OUTPUT_BUFFER, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Release output buffer failed, error: %{public}d", ret);
        return ret;
    }
    return reply.ReadInt32();
}

int32_t AVCodecServiceProxy::SetParameter(const Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    // todo
    int ret = Remote()->SendRequest(SET_PARAMETER, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Set parameter failed, error: %{public}d", ret);
        return ret;
    }
    return reply.ReadInt32();
}

int32_t AVCodecServiceProxy::DestroyStub()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int ret = Remote()->SendRequest(DESTROY, data, reply, option);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("destroy failed, error: %{public}d", ret);
        return ret;
    }
    return reply.ReadInt32();
}
} // namespace Media
} // namespace OHOS
