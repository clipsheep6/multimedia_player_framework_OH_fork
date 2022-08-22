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

#include "videoenc_native_mock.h"
#include "avformat_native_mock.h"
#include "avmemory_native_mock.h"
#include "surface_native_mock.h"
#include "media_errors.h"

namespace OHOS {
namespace Media {
void VideoEncNativeMock::OnError(AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)errorType;
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb == nullptr) {
        mockCb->OnError(errorCode);
    }
}

void VideoEncNativeMock::OnStreamChanged(AVCodec *codec, AVFormat *format, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        auto formatMock = std::make_shared<AVFormatNativeMock>(format);
        mockCb->OnStreamChanged(formatMock);
    }
}

void VideoEncNativeMock::OnNeedInputData(AVCodec *codec, uint32_t index, AVMemory *data, void *userData)
{
    (void)userData;
    std::shared_ptr<AVCodecCallbackMock> mockCb = GetCallback(codec);
    if (mockCb != nullptr) {
        mockCb->OnNeedInputData(index, nullptr);
    }
}

void VideoEncNativeMock::OnNewOutputData(AVCodec *codec, uint32_t index, AVMemory *data,
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
        std::shared_ptr<AVMemoryMock> memMock = std::make_shared<AVMemoryNativeMock>(data);
        mockCb->OnNewOutputData(index, memMock, bufferInfo);
    }
}

std::shared_ptr<AVCodecCallbackMock> VideoEncNativeMock::GetCallback(AVCodec *codec)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (mockCbMap_.find(codec) != mockCbMap_.end()) {
        return mockCbMap_.at(codec);
    }
    return nullptr;
}

void VideoEncNativeMock::SetCallback(AVCodec *codec, std::shared_ptr<AVCodecCallbackMock> cb)
{
    std::lock_guard<std::mutex> lock(mutex_);
    mockCbMap_[codec] = cb;
}

void VideoEncNativeMock::DelCallback(AVCodec *codec)
{
    auto it = mockCbMap_.find(codec);
    if (it != mockCbMap_.end()) {
        mockCbMap_.erase(it);
    }
}

int32_t VideoEncNativeMock::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (cb != nullptr && codec_ != nullptr) {
        SetCallback(codec_, cb);
        struct AVCodecAsyncCallback cb;
        cb.onAsyncError = VideoDecNativeMock::OnError;
        cb.onAsyncStreamChanged = VideoDecNativeMock::OnStreamChanged;
        cb.onAsyncNeedInputData = VideoDecNativeMock::OnNeedInputData;
        cb.onAsyncNewOutputData = VideoDecNativeMock::OnNewOutputData;
        return OH_VideoEncoder_SetCallback(codec, cb, NULL);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

std::shared_ptr<SurfaceMock> VideoEncNativeMock::GetInputSurface()
{
    if (codec_ != nullptr) {
        NativeWindow *surface = nullptr;
        (void)OH_VideoEncoder_GetSurface(codec_, &surface);
        if (surface != nullptr) {
            return std::make_shared<SurfaceNativeMock>(surface);
        }
    }
    return nullptr;
}

int32_t VideoEncNativeMock::Configure(std::shared_ptr<FormatMock> format)
{
    if (codec_ != nullptr && format != nullptr) {
        auto formatMock = std::static_pointer_cast<AVFormatNativeMock>(format);
        AVFormat *avFormat = formatMock->GetFormat();
        if (avFormat != nullptr) {
            return OH_VideoEncoder_Configure(codec_, avFormat);
        }
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoEncNativeMock::Prepare()
{
    if (codec_ != nullptr) {
        return OH_VideoEncoder_Prepare(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoEncNativeMock::Start()
{
    if (codec_ != nullptr) {
        return OH_VideoEncoder_Start(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoEncNativeMock::Stop()
{
    if (codec_ != nullptr) {
        return OH_VideoEncoder_Stop(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoEncNativeMock::Flush()
{
    if (codec_ != nullptr) {
        return OH_VideoEncoder_Flush(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoEncNativeMock::Reset()
{
    if (codec_ != nullptr) {
        return OH_VideoEncoder_Reset(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoEncNativeMock::Release()
{
    if (codec_ != nullptr) {
        return OH_VideoEncoder_Release(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoEncNativeMock::NotifyEos()
{
    if (codec_ != nullptr) {
        return OH_VideoEncoder_NotifyEndOfStream(codec_);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

std::shared_ptr<FormatMock> VideoEncNativeMock::GetOutputMediaDescription()
{
    if (codec_ != nullptr) {
        AVFormat *format = OH_VideoEncoder_GetOutputDescription(codec_);
        return std::make_shared<AVFormatNativeMock>(format);
    }
    return nullptr;
}

int32_t VideoEncNativeMock::SetParameter(std::shared_ptr<FormatMock> format)
{
    if (codec_ != nullptr && format != nullptr) {
        auto formatMock = std::static_pointer_cast<AVFormatNativeMock>(format);
        return OH_VideoEncoder_SetParameter(codec_, format->GetFormat());
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}

int32_t VideoEncNativeMock::FreeOutputData(uint32_t index)
{
    if (codec_ != nullptr) {
        return OH_VideoEncoder_FreeOutputData(codec_, index);
    }
    return AV_ERR_OPERATE_NOT_PERMIT;
}
}
}