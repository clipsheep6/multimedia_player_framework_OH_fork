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

#include "avcodec_mock.h"
#include "avformat_native_mock.h"
#include "avmemory_native_mock.h"
#include "surface_native_mock.h"
#include "audiodec_native_mock.h"
#include "audioenc_native_mock.h"
#include "videodec_native_mock.h"
#include "videoenc_native_mock.h"

namespace OHOS {
namespace Media {
std::shared_ptr<VideoDecMock> AVCodecMockFactory::CreateVideoDecMockByMine(const std::string &mime)
{
    AVCodec *videoDec = OH_VideoDecoder_CreateByMime(mime.c_str());
    if (videoDec != nullptr) {
        return std::make_shared<VideoDecNativeMock>(videoDec);
    }
    return nullptr;
}

std::shared_ptr<VideoDecMock> AVCodecMockFactory::CreateVideoDecMockByName(const std::string &name)
{
    AVCodec *videoDec = OH_VideoDecoder_CreateByName(name.c_str());
    if (videoDec != nullptr) {
        return std::make_shared<VideoDecNativeMock>(videoDec);
    }
    return nullptr;
}

std::shared_ptr<VideoEncMock> AVCodecMockFactory::CreateVideoEncMockByMine(const std::string &mime)
{
    AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(mime.c_str());
    if (videoEnc != nullptr) {
        return std::make_shared<VideoEncNativeMock>(videoEnc);
    }
    return nullptr;
}

std::shared_ptr<VideoEncMock> AVCodecMockFactory::CreateVideoEncMockByName(const std::string &name)
{
    AVCodec *videoEnc = OH_VideoEncoder_CreateByName(name.c_str());
    if (videoEnc != nullptr) {
        return std::make_shared<VideoEncNativeMock>(videoEnc);
    }
    return nullptr;
}

std::shared_ptr<AudioDecMock> AVCodecMockFactory::CreateAudioDecMockByMine(const std::string &mime)
{
    return nullptr;
}

std::shared_ptr<AudioDecMock> AVCodecMockFactory::CreateAudioDecMockByName(const std::string &name)
{
    return nullptr;
}

std::shared_ptr<AudioEncMock> AVCodecMockFactory::CreateAudioEncMockByMine(const std::string &mime)
{
    return nullptr;
}

std::shared_ptr<AudioEncMock> AVCodecMockFactory::CreateAudioEncMockByName(const std::string &name)
{
    return nullptr;
}

std::shared_ptr<FormatMock> AVCodecMockFactory::CreateFormat()
{
    return std::make_shared<AVFormatNativeMock>();
}

std::shared_ptr<SurfaceMock> AVCodecMockFactory::CreateSurface()
{
    return nullptr;
}
}
}