/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "avcodec_list_mock.h"
#include "gtest/gtest.h"
#include "media_errors.h"

using namespace OHOS;
using namespace OHOS::Media;

namespace OHOS {
namespace Media {
AVCodecListMock::AVCodecListMock()
{
}

AVCodecListMock::~AVCodecListMock()
{
}

bool AVCodecListMock::CreateAVCodecList()
{
    avCodecList_ = AVCodecListFactory::CreateAVCodecList();
    return avCodecList_ != nullptr;
}

std::string AVCodecListMock::FindVideoDecoder(const Format &format)
{
    return avCodecList_->FindVideoDecoder(format);
}

std::string AVCodecListMock::FindVideoEncoder(const Format &format)
{
    return avCodecList_->FindVideoEncoder(format);
}

std::string AVCodecListMock::FindAudioDecoder(const Format &format)
{
    return avCodecList_->FindAudioDecoder(format);
}

std::string AVCodecListMock::FindAudioEncoder(const Format &format)
{
    return avCodecList_->FindAudioEncoder(format);
}

std::vector<std::shared_ptr<VideoCaps>> AVCodecListMock::GetVideoDecoderCaps()
{
    return avCodecList_->GetVideoDecoderCaps();
}

std::vector<std::shared_ptr<VideoCaps>> AVCodecListMock::GetVideoEncoderCaps()
{
    return avCodecList_->GetVideoEncoderCaps();
}

std::vector<std::shared_ptr<AudioCaps>> AVCodecListMock::GetAudioDecoderCaps()
{
    return avCodecList_->GetAudioDecoderCaps();
}

std::vector<std::shared_ptr<AudioCaps>> AVCodecListMock::GetAudioEncoderCaps()
{
    return avCodecList_->GetAudioEncoderCaps();
}
} // namespace Media
} // namespace OHOS