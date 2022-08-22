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

#include "videodec_native_mock.h"
#include "avformat_native_mock.h"
#include "avmemory_native_mock.h"
#include "surface_native_mock.h"

namespace OHOS {
namespace Media {
void VideoDecNativeMock::OnError(AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)errorType;
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb == nullptr) {
        mockCb->OnError(errorCode);
    }
}

void VideoDecNativeMock::OnStreamChanged(AVCodec *codec, AVFormat *format, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        auto formatMock = std::make_shared<AVFormatNativeMock>(format);
        mockCb->OnStreamChanged(formatMock);
    }
}

void VideoDecNativeMock::OnNeedInputData(AVCodec *codec, uint32_t index, AVMemory *data, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        std::shared_ptr<AVMemoryMock> memMock = std::make_shared<AVMemoryNativeMock>(data);
        mockCb->OnNeedInputData(index, memMock);
    }
}

void VideoDecNativeMock::OnNewOutputData(AVCodec *codec, uint32_t index, AVMemory *data,
    AVCodecBufferAttr *attr, void *userData)
{
    (void)data;
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb == nullptr) {
        struct AVCodecBufferAttrMock bufferInfo;
        bufferInfo.pts = attr->pts;
        bufferInfo.size = attr->size;
        bufferInfo.offset = attr->offset;
        bufferInfo.flags = attr->flags;
        mockCb->OnNewOutputData(index, nullptr, bufferInfo);
    }
}

std::shared_ptr<AVCodecCallbackMock> VideoDecNativeMock::GetCallback(AVCodec *codec)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (mockCbMap_.find(codec) != mockCbMap_.end()) {
        return mockCbMap_.at(codec);
    }
    return nullptr;
}

void VideoDecNativeMock::SetCallback(AVCodec *codec, std::shared_ptr<AVCodecCallbackMock> cb)
{
    std::lock_guard<std::mutex> lock(mutex_);
    mockCbMap_[codec] = cb;
}

void VideoDecNativeMock::DelCallback(AVCodec *codec)
{
    auto it = mockCbMap_.find(codec);
    if (it != mockCbMap_.end()) {
        mockCbMap_.erase(it);
    }
}

int32_t VideoDecNativeMock::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (cb != nullptr && codec_ != nullptr) {
        SetCallback(codec_, cb);
        struct AVCodecAsyncCallback cb;
        cb.onAsyncError = VideoDecNativeMock::OnError;
        cb.onAsyncStreamChanged = VideoDecNativeMock::OnStreamChanged;
        cb.onAsyncNeedInputData = VideoDecNativeMock::OnNeedInputData;
        cb.onAsyncNewOutputData = VideoDecNativeMock::OnNewOutputData;
        return OH_VideoDecoder_SetCallback(codec, cb, NULL);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecNativeMock::SetOutputSurface(std::shared_ptr<SurfaceMock> surface)
{
    if (codec_ != nullptr && surface != nullptr) {
        auto surfaceMock = std::static_pointer_cast<SurfaceNativeMock>(surface);
        NativeWindow *nativeWindow = surfaceMock->GetSurface();
        if (nativeWindow != nullptr) {
            return OH_VideoDecoder_SetSurface(codec_, nativeWindow);
        }
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecNativeMock::Configure(std::shared_ptr<FormatMock> format)
{
    if (codec_ != nullptr && format != nullptr) {
        auto formatMock = std::static_pointer_cast<AVFormatNativeMock>(format);
        AVFormat *avFormat = formatMock->GetFormat();
        if (avFormat != nullptr) {
            return OH_VideoDecoder_Configure(codec_, avFormat);
        }
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecNativeMock::Prepare()
{
    if (codec_ != nullptr) {
        return OH_VideoDecoder_Prepare(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecNativeMock::Start()
{
    if (codec_ != nullptr) {
        return OH_VideoDecoder_Start(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecNativeMock::Stop()
{
    if (codec_ != nullptr) {
        return OH_VideoDecoder_Stop(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecNativeMock::Flush()
{
    if (codec_ != nullptr) {
        return OH_VideoDecoder_Flush(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecNativeMock::Reset()
{
    if (codec_ != nullptr) {
        return OH_VideoDecoder_Reset(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecNativeMock::Release()
{
    if (codec_ != nullptr) {
        DelCallback(codec_);
        return OH_VideoDecoder_Destroy(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

std::shared_ptr<FormatMock> VideoDecNativeMock::GetOutputMediaDescription()
{
    if (codec_ != nullptr) {
        AVFormat *format = OH_VideoDecoder_GetOutputDescription(codec_);
        return std::make_shared<AVFormatNativeMock>(format);
    }
    return nullptr;
}

int32_t VideoDecNativeMock::SetParameter(std::shared_ptr<FormatMock> format)
{
    if (codec_ != nullptr && format != nullptr) {
        auto formatMock = std::static_pointer_cast<AVFormatNativeMock>(format);
        return OH_VideoDecoder_SetParameter(codec_, format->GetFormat());
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecNativeMock::PushInputData(uint32_t index, AVCodecBufferAttrMock &attr)
{
    if (codec_ != nullptr) {
        AVCodecBufferAttr info;
        info.pts = attr.pts;
        info.size = attr.size;
        info.offset = attr.offset;
        info.flags = attr.flags;
        return OH_VideoDecoder_PushInputData(codec_, index, info);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecNativeMock::RenderOutputData(uint32_t index)
{
    if (codec_ != nullptr) {
        return OH_VideoDecoder_RenderOutputData(codec_, index);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoDecNativeMock::FreeOutputData(uint32_t index)
{
    if (codec_ != nullptr) {
        return OH_VideoDecoder_FreeOutputData(codec_, index);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}
}
}