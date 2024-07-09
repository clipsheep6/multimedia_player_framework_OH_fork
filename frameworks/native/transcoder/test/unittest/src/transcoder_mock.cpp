/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "transcoder_mock.h"
#include <sync_fence.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
//#include "display_type.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace OHOS::Media::TranscoderTestParam;

void TransCoderCallbackTest::OnError(TransCoderErrorType errorType, int32_t errorCode)
{
    cout << "Error received, errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void TransCoderCallbackTest::OnInfo(int32_t type, int32_t extra)
{
    cout << "Info received, Infotype:" << type << " Infocode:" << extra << endl;
}

bool TranscoderMock::CreateTranscoder()
{
    transcoder_ = TransCoderFactory::CreateTransCoder();
    return transcoder_ != nullptr;
}

int32_t TranscoderMock::SetOutputFormat(OutputFormatType format)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(transcoder_ != nullptr, MSERR_INVALID_OPERATION, "transcoder_ == nullptr");
    return transcoder_->SetOutputFormat(format);
}

int32_t TranscoderMock::SetVideoEncoder(VideoCodecFormat encoder)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(transcoder_ != nullptr, MSERR_INVALID_OPERATION, "transcoder_ == nullptr");
    return transcoder_->SetVideoEncoder(encoder);
}

int32_t TranscoderMock::SetVideoEncodingBitRate(int32_t rate)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(transcoder_ != nullptr, MSERR_INVALID_OPERATION, "transcoder_ == nullptr");
    return transcoder_->SetVideoEncodingBitRate(rate);
}

int32_t TranscoderMock::SetVideoSize(int32_t videoFrameWidth, int32_t videoFrameHeight)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(transcoder_ != nullptr, MSERR_INVALID_OPERATION, "transcoder_ == nullptr");
    return transcoder_->SetVideoSize(videoFrameWidth, videoFrameHeight);
}

int32_t TranscoderMock::SetAudioEncoder(AudioCodecFormat encoder)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(transcoder_ != nullptr, MSERR_INVALID_OPERATION, "transcoder_ == nullptr");
    return transcoder_->SetAudioEncoder(encoder);
}

int32_t TranscoderMock::SetAudioEncodingBitRate(int32_t bitRate)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(transcoder_ != nullptr, MSERR_INVALID_OPERATION, "transcoder_ == nullptr");
    return transcoder_->SetAudioEncodingBitRate(bitRate);
}

int32_t TranscoderMock::SetInputFile(std::string url)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(transcoder_ != nullptr, MSERR_INVALID_OPERATION, "transcoder_ == nullptr");
    return transcoder_->SetInputFile(url);
}

int32_t TranscoderMock::SetInputFile(int32_t fd, int64_t offset, int64_t size)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(transcoder_ != nullptr, MSERR_INVALID_OPERATION, "transcoder_ == nullptr");
    return transcoder_->SetInputFile(fd, offset, size);
}

int32_t TranscoderMock::SetOutputFile(int32_t fd)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(transcoder_ != nullptr, MSERR_INVALID_OPERATION, "transcoder_ == nullptr");
    return transcoder_->SetOutputFile(fd);
}

int32_t TranscoderMock::SetTransCoderCallback(const std::shared_ptr<TransCoderCallback> &callback)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(transcoder_ != nullptr, MSERR_INVALID_OPERATION, "transcoder_ == nullptr");
    return transcoder_->SetTransCoderCallback(callback);
}

int32_t TranscoderMock::Prepare()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(transcoder_ != nullptr, MSERR_INVALID_OPERATION, "transcoder_ == nullptr");
    return transcoder_->Prepare();
}

int32_t TranscoderMock::Start()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(transcoder_ != nullptr, MSERR_INVALID_OPERATION, "transcoder_ == nullptr");
    return transcoder_->Start();
}

int32_t TranscoderMock::Pause()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(transcoder_ != nullptr, MSERR_INVALID_OPERATION, "transcoder_ == nullptr");
    if (!isExit_.load()) {
        isExit_.store(true);
    }
    return transcoder_->Pause();
}

int32_t TranscoderMock::Resume()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(transcoder_ != nullptr, MSERR_INVALID_OPERATION, "transcoder_ == nullptr");
    if (!isExit_.load()) {
        isExit_.store(true);
    }
    return transcoder_->Resume();
}

int32_t TranscoderMock::Cancel()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(transcoder_ != nullptr, MSERR_INVALID_OPERATION, "transcoder_ == nullptr");
    if (!isExit_.load()) {
        isExit_.store(true);
    }
    return transcoder_->Cancel();
}

int32_t TranscoderMock::Release()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(transcoder_ != nullptr, MSERR_INVALID_OPERATION, "transcoder_ == nullptr");
    if (!isExit_.load()) {
        isExit_.store(true);
    }
    return transcoder_->Release();
}