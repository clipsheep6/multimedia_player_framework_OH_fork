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

#ifndef TRANSCODER_MOCK_H
#define TRANSCODER_MOCK_H
#include <atomic>
#include <thread>
#include <string>
#include "gtest/gtest.h"
#include "media_errors.h"
#include "unittest_log.h"
#include "transcoder.h"
#include "surface.h"
#include "securec.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
namespace TranscoderTestParam {
    constexpr uint32_t TRASCODER_AUDIO_ENCODING_BIT_RATE = 20000;
    constexpr uint32_t TRASCODER_VIDEO_ENCODING_BIT_RATE = 30000;
    constexpr uint32_t TRANSCODER_BUFFER_WIDTH = 1920;
    constexpr uint32_t TRANSCODER_BUFFER_HEIGHT = 1080;
    constexpr int32_t ERROR_WRONG_STATE = -5;
    constexpr uint64_t TRANSCODER_FILE_OFFSET = 37508084;
    constexpr uint64_t TRANSCODER_FILE_SIZE = 2735029;
    const std::string TRANSCODER_ROOT_SRC = "/data/test/media/transcoder_src/";
    const std::string TRANSCODER_ROOT_DST = "/data/test/media/transcoder_dst/";
} // namespace TranscoderTestParam
class TranscoderMock {
public:
    TranscoderMock() = default;
    ~TranscoderMock() = default;
    bool CreateTranscoder();
    int32_t SetOutputFormat(OutputFormatType format);
    int32_t SetVideoEncoder(VideoCodecFormat encoder);
    int32_t SetVideoEncodingBitRate(int32_t rate);
    int32_t SetVideoSize(int32_t videoFrameWidth, int32_t videoFrameHeight);
    int32_t SetAudioEncoder(AudioCodecFormat encoder);
    int32_t SetAudioEncodingBitRate(int32_t bitRate);
    int32_t SetInputFile(std::string url);
    int32_t SetInputFile(int32_t fd, int64_t offset, int64_t size);
    int32_t SetOutputFile(int32_t fd);
    int32_t SetTransCoderCallback(const std::shared_ptr<TransCoderCallback> &callback);
    int32_t Prepare();
    int32_t Start();
    int32_t Pause();
    int32_t Resume();
    int32_t Cancel();
    int32_t Release();
private:
    std::shared_ptr<TransCoder> transcoder_ = nullptr;
    std::atomic<bool> isExit_ { false };
};

class TransCoderCallbackTest : public TransCoderCallback, public NoCopyable {
public:
    ~TransCoderCallbackTest() {}
    void OnError(TransCoderErrorType errorType, int32_t errorCode) override;
    void OnInfo(int32_t type, int32_t extra) override;
};
}
}
#endif