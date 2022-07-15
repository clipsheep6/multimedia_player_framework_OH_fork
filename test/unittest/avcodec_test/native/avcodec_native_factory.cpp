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
#include "media_errors.h"

namespace OHOS {
namespace Media {
std::shared_ptr<VideoDecMock> AVCodecMockFactory::CreateVideoDecMockByMine(const std::string &mime)
{
    auto videoDec = VideoDecoderFactory::CreateByMime(mime);
    return std::make_shared<VideoDecNativeMock>(videoDec);
}

std::shared_ptr<VideoDecMock> AVCodecMockFactory::CreateVideoDecMockByMine()
{

}

std::shared_ptr<VideoDecMock> AVCodecMockFactory::CreateVideoDecMockByName()
{

}

std::shared_ptr<VideoEncMock> AVCodecMockFactory::CreateVideoEncMockByMine()
{

}

std::shared_ptr<VideoEncMock> AVCodecMockFactory::CreateVideoEncMockByName()
{

}

std::shared_ptr<VideoEncMock> AVCodecMockFactory::CreateVideoEncMock(const std::string &mime)
{
    return nullptr;
}

std::shared_ptr<FormatMock> AVCodecMockFactory::CreateFormat()
{
    return nullptr;
}

std::shared_ptr<SurfaceMock> AVCodecMockFactory::CreateSurface()
{
    return nullptr;
}
}
}