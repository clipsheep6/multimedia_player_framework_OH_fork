/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "transcoder_unit_test.h"
#include <unistd.h>
#include <fcntl.h>
#include <securec.h>
#include "media_errors.h"
#include "av_common.h"
#include "meta/video_types.h"
#include "media_errors.h"
#include "media_log.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;
using namespace OHOS::Media::TranscoderTestParam;

namespace OHOS {
namespace Media {

void TransCoderUnitTest::SetUpTestCase(void) {}

void TransCoderUnitTest::TearDownTestCase(void) {}

void TransCoderUnitTest::SetUp(void)
{
    transcoder_ = std::make_shared<TranscoderMock>();
    ASSERT_NE(nullptr, transcoder_);
    ASSERT_TRUE(transcoder_->CreateTranscoder());
}

void TransCoderUnitTest::TearDown(void)
{
    if (transcoder_ != nullptr) {
        transcoder_->Release();
    }
}

/**
 * @tc.name: transcoder_PureVideo_001
 * @tc.desc: transcoder pure video AVC_Baseline@L1.2_81.0Kbps_320x240.mp4 with codec format H264
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TransCoderUnitTest, transcoder_PureVideo_001, TestSize.Level2)
{
    int32_t srcFd = open((TRANSCODER_ROOT_SRC + "AVC_Baseline@L1.2_81.0Kbps_320x240.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(srcFd >= 0);
    int64_t offset = TRANSCODER_FILE_OFFSET;
    int64_t size = TRANSCODER_FILE_SIZE;
    EXPECT_EQ(MSERR_OK, transcoder_->SetInputFile(srcFd, offset, size));
    int32_t dstFd = open((TRANSCODER_ROOT_DST + "AVC_Baseline@L1.2_81.0Kbps_320x240_dst.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(dstFd >= 0);
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFile(dstFd));
    std::shared_ptr<TransCoderCallbackTest> cb = std::make_shared<TransCoderCallbackTest>();
    EXPECT_EQ(MSERR_OK, transcoder_->SetTransCoderCallback(cb));
    OutputFormatType format = FORMAT_MPEG_4;
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFormat(format));
    VideoCodecFormat encoder = H264;
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncoder(encoder));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncodingBitRate(TRASCODER_VIDEO_ENCODING_BIT_RATE));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoSize(TRANSCODER_BUFFER_WIDTH, TRANSCODER_BUFFER_HEIGHT));
    EXPECT_EQ(MSERR_OK, transcoder_->Prepare());
    EXPECT_EQ(MSERR_OK, transcoder_->Start());
    EXPECT_EQ(MSERR_OK, transcoder_->Cancel());
    EXPECT_EQ(MSERR_OK, transcoder_->Release());
    close(dstFd);
    close(srcFd);
}

/**
 * @tc.name: transcoder_PureVideo_002
 * @tc.desc: transcoder pure video AVC_Baseline@L1.2_81.0Kbps_320x240.mp4 with codec format h264 pause and resume
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TransCoderUnitTest, transcoder_PureVideo_002, TestSize.Level2)
{
    int32_t srcFd = open((TRANSCODER_ROOT_SRC + "AVC_Baseline@L1.2_81.0Kbps_320x240.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(srcFd >= 0);
    int64_t offset = TRANSCODER_FILE_OFFSET;
    int64_t size = TRANSCODER_FILE_SIZE;
    EXPECT_EQ(MSERR_OK, transcoder_->SetInputFile(srcFd, offset, size));
    int32_t dstFd = open((TRANSCODER_ROOT_DST + "AVC_Baseline@L1.2_81.0Kbps_320x240_dst.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(dstFd >= 0);
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFile(dstFd));
    std::shared_ptr<TransCoderCallbackTest> cb = std::make_shared<TransCoderCallbackTest>();
    EXPECT_EQ(MSERR_OK, transcoder_->SetTransCoderCallback(cb));
    OutputFormatType format = FORMAT_MPEG_4;
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFormat(format));
    VideoCodecFormat encoder = H264;
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncoder(encoder));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncodingBitRate(TRASCODER_VIDEO_ENCODING_BIT_RATE));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoSize(TRANSCODER_BUFFER_WIDTH, TRANSCODER_BUFFER_HEIGHT));
    EXPECT_EQ(MSERR_OK, transcoder_->Prepare());
    EXPECT_EQ(MSERR_OK, transcoder_->Start());
    EXPECT_EQ(MSERR_OK, transcoder_->Pause());
    EXPECT_EQ(MSERR_OK, transcoder_->Resume());
    EXPECT_EQ(MSERR_OK, transcoder_->Cancel());
    EXPECT_EQ(MSERR_OK, transcoder_->Release());
    close(dstFd);
    close(srcFd);
}

/**
 * @tc.name: transcoder_PureVideo_003
 * @tc.desc: transcoder pure video AVC_Baseline@L1.2_81.0Kbps_320x240.mp4 with codec format H265
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TransCoderUnitTest, transcoder_PureVideo_003, TestSize.Level2)
{
    int32_t srcFd = open((TRANSCODER_ROOT_SRC + "AVC_Baseline@L1.2_81.0Kbps_320x240.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(srcFd >= 0);
    int64_t offset = TRANSCODER_FILE_OFFSET;
    int64_t size = TRANSCODER_FILE_SIZE;
    EXPECT_EQ(MSERR_OK, transcoder_->SetInputFile(srcFd, offset, size));
    int32_t dstFd = open((TRANSCODER_ROOT_DST + "AVC_Baseline@L1.2_81.0Kbps_320x240_dst.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(dstFd >= 0);
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFile(dstFd));
    std::shared_ptr<TransCoderCallbackTest> cb = std::make_shared<TransCoderCallbackTest>();
    EXPECT_EQ(MSERR_OK, transcoder_->SetTransCoderCallback(cb));
    OutputFormatType format = FORMAT_MPEG_4;
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFormat(format));
    VideoCodecFormat encoder = H265;
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncoder(encoder));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncodingBitRate(TRASCODER_VIDEO_ENCODING_BIT_RATE));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoSize(TRANSCODER_BUFFER_WIDTH, TRANSCODER_BUFFER_HEIGHT));
    EXPECT_EQ(MSERR_OK, transcoder_->Prepare());
    EXPECT_EQ(MSERR_OK, transcoder_->Start());
    EXPECT_EQ(MSERR_OK, transcoder_->Cancel());
    EXPECT_EQ(MSERR_OK, transcoder_->Release());
    close(dstFd);
    close(srcFd);
}

/**
 * @tc.name: transcoder_PureVideo_004
 * @tc.desc: transcoder pure video AVC_Baseline@L1.2_81.0Kbps_320x240.mp4 with codec format h265 pause and resume
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TransCoderUnitTest, transcoder_PureVideo_004, TestSize.Level2)
{
    int32_t srcFd = open((TRANSCODER_ROOT_SRC + "AVC_Baseline@L1.2_81.0Kbps_320x240.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(srcFd >= 0);
    int64_t offset = TRANSCODER_FILE_OFFSET;
    int64_t size = TRANSCODER_FILE_SIZE;
    EXPECT_EQ(MSERR_OK, transcoder_->SetInputFile(srcFd, offset, size));
    int32_t dstFd = open((TRANSCODER_ROOT_DST + "AVC_Baseline@L1.2_81.0Kbps_320x240_dst.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(dstFd >= 0);
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFile(dstFd));
    std::shared_ptr<TransCoderCallbackTest> cb = std::make_shared<TransCoderCallbackTest>();
    EXPECT_EQ(MSERR_OK, transcoder_->SetTransCoderCallback(cb));
    OutputFormatType format = FORMAT_MPEG_4;
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFormat(format));
    VideoCodecFormat encoder = H265;
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncoder(encoder));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncodingBitRate(TRASCODER_VIDEO_ENCODING_BIT_RATE));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoSize(TRANSCODER_BUFFER_WIDTH, TRANSCODER_BUFFER_HEIGHT));
    EXPECT_EQ(MSERR_OK, transcoder_->Prepare());
    EXPECT_EQ(MSERR_OK, transcoder_->Start());
    EXPECT_EQ(MSERR_OK, transcoder_->Pause());
    EXPECT_EQ(MSERR_OK, transcoder_->Resume());
    EXPECT_EQ(MSERR_OK, transcoder_->Cancel());
    EXPECT_EQ(MSERR_OK, transcoder_->Release());
    close(dstFd);
    close(srcFd);
}

/**
 * @tc.name: transcoder_PureAudio_001
 * @tc.desc: transcoder pure audio 01.mp3
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TransCoderUnitTest, transcoder_PureAudio_001, TestSize.Level2)
{
    int32_t srcFd = open((TRANSCODER_ROOT_SRC + "01.mp3").c_str(), O_RDWR);
    ASSERT_TRUE(srcFd >= 0);
    int64_t offset = TRANSCODER_FILE_OFFSET;
    int64_t size = TRANSCODER_FILE_SIZE;
    EXPECT_EQ(MSERR_OK, transcoder_->SetInputFile(srcFd, offset, size));
    int32_t dstFd = open((TRANSCODER_ROOT_DST + "01_dst.mp3").c_str(), O_RDWR);
    ASSERT_TRUE(dstFd >= 0);
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFile(dstFd));
    std::shared_ptr<TransCoderCallbackTest> cb = std::make_shared<TransCoderCallbackTest>();
    EXPECT_EQ(MSERR_OK, transcoder_->SetTransCoderCallback(cb));
    OutputFormatType format = FORMAT_M4A;
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFormat(format));
    AudioCodecFormat encoder = AAC_LC;
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncoder(encoder));
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncodingBitRate(TRASCODER_AUDIO_ENCODING_BIT_RATE));
    EXPECT_EQ(MSERR_OK, transcoder_->Prepare());
    EXPECT_EQ(MSERR_OK, transcoder_->Start());
    EXPECT_EQ(MSERR_OK, transcoder_->Cancel());
    EXPECT_EQ(MSERR_OK, transcoder_->Release());
    close(dstFd);
    close(srcFd);
}

/**
 * @tc.name: transcoder_PureAudio_002
 * @tc.desc: transcoder pure audio 01.mp3 with pause resume
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TransCoderUnitTest, transcoder_PureAudio_002, TestSize.Level2)
{
    int32_t srcFd = open((TRANSCODER_ROOT_SRC + "01.mp3").c_str(), O_RDWR);
    ASSERT_TRUE(srcFd >= 0);
    int64_t offset = TRANSCODER_FILE_OFFSET;
    int64_t size = TRANSCODER_FILE_SIZE;
    EXPECT_EQ(MSERR_OK, transcoder_->SetInputFile(srcFd, offset, size));
    int32_t dstFd = open((TRANSCODER_ROOT_DST + "01_dst.mp3").c_str(), O_RDWR);
    ASSERT_TRUE(dstFd >= 0);
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFile(dstFd));
    std::shared_ptr<TransCoderCallbackTest> cb = std::make_shared<TransCoderCallbackTest>();
    EXPECT_EQ(MSERR_OK, transcoder_->SetTransCoderCallback(cb));
    OutputFormatType format = FORMAT_M4A;
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFormat(format));
    AudioCodecFormat encoder = AAC_LC;
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncoder(encoder));
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncodingBitRate(TRASCODER_AUDIO_ENCODING_BIT_RATE));
    EXPECT_EQ(MSERR_OK, transcoder_->Prepare());
    EXPECT_EQ(MSERR_OK, transcoder_->Start());
    EXPECT_EQ(MSERR_OK, transcoder_->Pause());
    EXPECT_EQ(MSERR_OK, transcoder_->Resume());
    EXPECT_EQ(MSERR_OK, transcoder_->Cancel());
    EXPECT_EQ(MSERR_OK, transcoder_->Release());
    close(dstFd);
    close(srcFd);
}

/**
 * @tc.name: transcoder_AudioVideo_001
 * @tc.desc: transcoder audio and video ChineseColor_H264_AAC_480p_15fps.mp4 with codec format H264
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TransCoderUnitTest, transcoder_AudioVideo_001, TestSize.Level2)
{
    int32_t srcFd = open((TRANSCODER_ROOT_SRC + "ChineseColor_H264_AAC_480p_15fps.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(srcFd >= 0);
    int64_t offset = TRANSCODER_FILE_OFFSET;
    int64_t size = TRANSCODER_FILE_SIZE;
    EXPECT_EQ(MSERR_OK, transcoder_->SetInputFile(srcFd, offset, size));
    int32_t dstFd = open((TRANSCODER_ROOT_DST + "ChineseColor_H264_AAC_480p_15fps_dst.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(dstFd >= 0);
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFile(dstFd));
    std::shared_ptr<TransCoderCallbackTest> cb = std::make_shared<TransCoderCallbackTest>();
    EXPECT_EQ(MSERR_OK, transcoder_->SetTransCoderCallback(cb));
    OutputFormatType format = FORMAT_MPEG_4;
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFormat(format));
    AudioCodecFormat encoderAudio = AAC_LC;
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncoder(encoderAudio));
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncodingBitRate(TRASCODER_AUDIO_ENCODING_BIT_RATE));
    VideoCodecFormat encoder = H264;
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncoder(encoder));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncodingBitRate(TRASCODER_VIDEO_ENCODING_BIT_RATE));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoSize(TRANSCODER_BUFFER_WIDTH, TRANSCODER_BUFFER_HEIGHT));
    EXPECT_EQ(MSERR_OK, transcoder_->Prepare());
    EXPECT_EQ(MSERR_OK, transcoder_->Start());
    EXPECT_EQ(MSERR_OK, transcoder_->Cancel());
    EXPECT_EQ(MSERR_OK, transcoder_->Release());
    close(dstFd);
    close(srcFd);
}

/**
 * @tc.name: transcoder_AudioVideo_002
 * @tc.desc: transcoder audio and video AVC_Baseline@L1.2_81.0Kbps_320x240.mp4 with codec format h264 pause and resume
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TransCoderUnitTest, transcoder_AudioVideo_002, TestSize.Level2)
{
    int32_t srcFd = open((TRANSCODER_ROOT_SRC + "ChineseColor_H264_AAC_480p_15fps.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(srcFd >= 0);
    int64_t offset = TRANSCODER_FILE_OFFSET;
    int64_t size = TRANSCODER_FILE_SIZE;
    EXPECT_EQ(MSERR_OK, transcoder_->SetInputFile(srcFd, offset, size));
    int32_t dstFd = open((TRANSCODER_ROOT_DST + "ChineseColor_H264_AAC_480p_15fps_dst.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(dstFd >= 0);
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFile(dstFd));
    std::shared_ptr<TransCoderCallbackTest> cb = std::make_shared<TransCoderCallbackTest>();
    EXPECT_EQ(MSERR_OK, transcoder_->SetTransCoderCallback(cb));
    OutputFormatType format = FORMAT_MPEG_4;
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFormat(format));
    AudioCodecFormat encoderAudio = AAC_LC;
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncoder(encoderAudio));
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncodingBitRate(TRASCODER_AUDIO_ENCODING_BIT_RATE));
    VideoCodecFormat encoder = H264;
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncoder(encoder));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncodingBitRate(TRASCODER_VIDEO_ENCODING_BIT_RATE));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoSize(TRANSCODER_BUFFER_WIDTH, TRANSCODER_BUFFER_HEIGHT));
    EXPECT_EQ(MSERR_OK, transcoder_->Prepare());
    EXPECT_EQ(MSERR_OK, transcoder_->Start());
    EXPECT_EQ(MSERR_OK, transcoder_->Pause());
    EXPECT_EQ(MSERR_OK, transcoder_->Resume());
    EXPECT_EQ(MSERR_OK, transcoder_->Cancel());
    EXPECT_EQ(MSERR_OK, transcoder_->Release());
    close(dstFd);
    close(srcFd);
}

/**
 * @tc.name: transcoder_AudioVideo_003
 * @tc.desc: transcoder audio and video ChineseColor_H264_AAC_480p_15fps.mp4 with codec format H265
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TransCoderUnitTest, transcoder_AudioVideo_003, TestSize.Level2)
{
    int32_t srcFd = open((TRANSCODER_ROOT_SRC + "ChineseColor_H264_AAC_480p_15fps.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(srcFd >= 0);
    int64_t offset = TRANSCODER_FILE_OFFSET;
    int64_t size = TRANSCODER_FILE_SIZE;
    EXPECT_EQ(MSERR_OK, transcoder_->SetInputFile(srcFd, offset, size));
    int32_t dstFd = open((TRANSCODER_ROOT_DST + "ChineseColor_H264_AAC_480p_15fps_dst.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(dstFd >= 0);
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFile(dstFd));
    std::shared_ptr<TransCoderCallbackTest> cb = std::make_shared<TransCoderCallbackTest>();
    EXPECT_EQ(MSERR_OK, transcoder_->SetTransCoderCallback(cb));
    OutputFormatType format = FORMAT_MPEG_4;
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFormat(format));
    AudioCodecFormat encoderAudio = AAC_LC;
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncoder(encoderAudio));
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncodingBitRate(TRASCODER_AUDIO_ENCODING_BIT_RATE));
    VideoCodecFormat encoder = H265;
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncoder(encoder));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncodingBitRate(TRASCODER_VIDEO_ENCODING_BIT_RATE));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoSize(TRANSCODER_BUFFER_WIDTH, TRANSCODER_BUFFER_HEIGHT));
    EXPECT_EQ(MSERR_OK, transcoder_->Prepare());
    EXPECT_EQ(MSERR_OK, transcoder_->Start());
    EXPECT_EQ(MSERR_OK, transcoder_->Cancel());
    EXPECT_EQ(MSERR_OK, transcoder_->Release());
    close(dstFd);
    close(srcFd);
}

/**
 * @tc.name: transcoder_AudioVideo_004
 * @tc.desc: transcoder audio and video ChineseColor_H264_AAC_480p_15fps.mp4 with codec format h265 pause and resume
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TransCoderUnitTest, transcoder_AudioVideo_004, TestSize.Level2)
{
    int32_t srcFd = open((TRANSCODER_ROOT_SRC + "ChineseColor_H264_AAC_480p_15fps.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(srcFd >= 0);
    int64_t offset = TRANSCODER_FILE_OFFSET;
    int64_t size = TRANSCODER_FILE_SIZE;
    EXPECT_EQ(MSERR_OK, transcoder_->SetInputFile(srcFd, offset, size));
    int32_t dstFd = open((TRANSCODER_ROOT_DST + "ChineseColor_H264_AAC_480p_15fps_dst.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(dstFd >= 0);
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFile(dstFd));
    std::shared_ptr<TransCoderCallbackTest> cb = std::make_shared<TransCoderCallbackTest>();
    EXPECT_EQ(MSERR_OK, transcoder_->SetTransCoderCallback(cb));
    OutputFormatType format = FORMAT_MPEG_4;
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFormat(format));
    AudioCodecFormat encoderAudio = AAC_LC;
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncoder(encoderAudio));
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncodingBitRate(TRASCODER_AUDIO_ENCODING_BIT_RATE));
    VideoCodecFormat encoder = H265;
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncoder(encoder));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncodingBitRate(TRASCODER_VIDEO_ENCODING_BIT_RATE));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoSize(TRANSCODER_BUFFER_WIDTH, TRANSCODER_BUFFER_HEIGHT));
    EXPECT_EQ(MSERR_OK, transcoder_->Prepare());
    EXPECT_EQ(MSERR_OK, transcoder_->Start());
    EXPECT_EQ(MSERR_OK, transcoder_->Pause());
    EXPECT_EQ(MSERR_OK, transcoder_->Resume());
    EXPECT_EQ(MSERR_OK, transcoder_->Cancel());
    EXPECT_EQ(MSERR_OK, transcoder_->Release());
    close(dstFd);
    close(srcFd);
}

/**
 * @tc.name: transcoder_PureAudioAbnormal_case_001
 * @tc.desc: transcoder pure audio 01.mp3 with cancle before prepare
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TransCoderUnitTest, transcoder_PureAudioAbnormal_case_001, TestSize.Level2)
{
    int32_t srcFd = open((TRANSCODER_ROOT_SRC + "01.mp3").c_str(), O_RDWR);
    ASSERT_TRUE(srcFd >= 0);
    int64_t offset = TRANSCODER_FILE_OFFSET;
    int64_t size = TRANSCODER_FILE_SIZE;
    EXPECT_EQ(MSERR_OK, transcoder_->SetInputFile(srcFd, offset, size));
    int32_t dstFd = open((TRANSCODER_ROOT_DST + "01_dst.mp3").c_str(), O_RDWR);
    ASSERT_TRUE(dstFd >= 0);
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFile(dstFd));
    std::shared_ptr<TransCoderCallbackTest> cb = std::make_shared<TransCoderCallbackTest>();
    EXPECT_EQ(MSERR_OK, transcoder_->SetTransCoderCallback(cb));
    OutputFormatType format = FORMAT_M4A;
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFormat(format));
    AudioCodecFormat encoder = AAC_LC;
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncoder(encoder));
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncodingBitRate(TRASCODER_AUDIO_ENCODING_BIT_RATE));
    EXPECT_EQ(ERROR_WRONG_STATE, transcoder_->Cancel());
    EXPECT_EQ(MSERR_OK, transcoder_->Release());
    close(dstFd);
    close(srcFd);
}

/**
 * @tc.name: transcoder_PureAudioAbnormal_case_002
 * @tc.desc: transcoder pure audio 01.mp3 with pause before prepare
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TransCoderUnitTest, transcoder_PureAudioAbnormal_case_002, TestSize.Level2)
{
    int32_t srcFd = open((TRANSCODER_ROOT_SRC + "01.mp3").c_str(), O_RDWR);
    ASSERT_TRUE(srcFd >= 0);
    int64_t offset = TRANSCODER_FILE_OFFSET;
    int64_t size = TRANSCODER_FILE_SIZE;
    EXPECT_EQ(MSERR_OK, transcoder_->SetInputFile(srcFd, offset, size));
    int32_t dstFd = open((TRANSCODER_ROOT_DST + "01_dst.mp3").c_str(), O_RDWR);
    ASSERT_TRUE(dstFd >= 0);
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFile(dstFd));
    std::shared_ptr<TransCoderCallbackTest> cb = std::make_shared<TransCoderCallbackTest>();
    EXPECT_EQ(MSERR_OK, transcoder_->SetTransCoderCallback(cb));
    OutputFormatType format = FORMAT_M4A;
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFormat(format));
    AudioCodecFormat encoder = AAC_LC;
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncoder(encoder));
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncodingBitRate(TRASCODER_AUDIO_ENCODING_BIT_RATE));
    EXPECT_EQ(MSERR_INVALID_OPERATION, transcoder_->Pause());
    EXPECT_EQ(MSERR_OK, transcoder_->Release());
    close(dstFd);
    close(srcFd);
}

/**
 * @tc.name: transcoder_PureAudioAbnormal_case_003
 * @tc.desc: transcoder pure audio 01.mp3 with resume before prepare
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TransCoderUnitTest, transcoder_PureAudioAbnormal_case_003, TestSize.Level2)
{
    int32_t srcFd = open((TRANSCODER_ROOT_SRC + "01.mp3").c_str(), O_RDWR);
    ASSERT_TRUE(srcFd >= 0);
    int64_t offset = TRANSCODER_FILE_OFFSET;
    int64_t size = TRANSCODER_FILE_SIZE;
    EXPECT_EQ(MSERR_OK, transcoder_->SetInputFile(srcFd, offset, size));
    int32_t dstFd = open((TRANSCODER_ROOT_DST + "01_dst.mp3").c_str(), O_RDWR);
    ASSERT_TRUE(dstFd >= 0);
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFile(dstFd));
    std::shared_ptr<TransCoderCallbackTest> cb = std::make_shared<TransCoderCallbackTest>();
    EXPECT_EQ(MSERR_OK, transcoder_->SetTransCoderCallback(cb));
    OutputFormatType format = FORMAT_M4A;
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFormat(format));
    AudioCodecFormat encoder = AAC_LC;
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncoder(encoder));
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncodingBitRate(TRASCODER_AUDIO_ENCODING_BIT_RATE));
    EXPECT_EQ(MSERR_INVALID_OPERATION, transcoder_->Resume());
    EXPECT_EQ(MSERR_OK, transcoder_->Release());
    close(dstFd);
    close(srcFd);
}

/**
 * @tc.name: transcoder_PureVideoAbnormal_case_001
 * @tc.desc: transcoder pure video AVC_Baseline@L1.2_81.0Kbps_320x240.mp4 with cancel before prepare
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TransCoderUnitTest, transcoder_PureVideoAbnormal_case_001, TestSize.Level2)
{
    int32_t srcFd = open((TRANSCODER_ROOT_SRC + "AVC_Baseline@L1.2_81.0Kbps_320x240.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(srcFd >= 0);
    int64_t offset = TRANSCODER_FILE_OFFSET;
    int64_t size = TRANSCODER_FILE_SIZE;
    EXPECT_EQ(MSERR_OK, transcoder_->SetInputFile(srcFd, offset, size));
    int32_t dstFd = open((TRANSCODER_ROOT_DST + "AVC_Baseline@L1.2_81.0Kbps_320x240_dst.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(dstFd >= 0);
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFile(dstFd));
    std::shared_ptr<TransCoderCallbackTest> cb = std::make_shared<TransCoderCallbackTest>();
    EXPECT_EQ(MSERR_OK, transcoder_->SetTransCoderCallback(cb));
    OutputFormatType format = FORMAT_MPEG_4;
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFormat(format));
    VideoCodecFormat encoder = H264;
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncoder(encoder));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncodingBitRate(TRASCODER_VIDEO_ENCODING_BIT_RATE));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoSize(TRANSCODER_BUFFER_WIDTH, TRANSCODER_BUFFER_HEIGHT));
    EXPECT_EQ(ERROR_WRONG_STATE, transcoder_->Cancel());
    EXPECT_EQ(MSERR_OK, transcoder_->Release());
    close(dstFd);
    close(srcFd);
}

/**
 * @tc.name: transcoder_PureVideoAbnormal_case_002
 * @tc.desc: transcoder pure video AVC_Baseline@L1.2_81.0Kbps_320x240.mp4 with cancel before prepare
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TransCoderUnitTest, transcoder_PureVideoAbnormal_case_002, TestSize.Level2)
{
    int32_t srcFd = open((TRANSCODER_ROOT_SRC + "AVC_Baseline@L1.2_81.0Kbps_320x240.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(srcFd >= 0);
    int64_t offset = TRANSCODER_FILE_OFFSET;
    int64_t size = TRANSCODER_FILE_SIZE;
    EXPECT_EQ(MSERR_OK, transcoder_->SetInputFile(srcFd, offset, size));
    int32_t dstFd = open((TRANSCODER_ROOT_DST + "AVC_Baseline@L1.2_81.0Kbps_320x240_dst.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(dstFd >= 0);
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFile(dstFd));
    std::shared_ptr<TransCoderCallbackTest> cb = std::make_shared<TransCoderCallbackTest>();
    EXPECT_EQ(MSERR_OK, transcoder_->SetTransCoderCallback(cb));
    OutputFormatType format = FORMAT_MPEG_4;
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFormat(format));
    VideoCodecFormat encoder = H264;
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncoder(encoder));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncodingBitRate(TRASCODER_VIDEO_ENCODING_BIT_RATE));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoSize(TRANSCODER_BUFFER_WIDTH, TRANSCODER_BUFFER_HEIGHT));
    EXPECT_EQ(MSERR_INVALID_OPERATION, transcoder_->Pause());
    EXPECT_EQ(MSERR_OK, transcoder_->Release());
    close(dstFd);
    close(srcFd);
}

/**
 * @tc.name: transcoder_PureVideoAbnormal_case_003
 * @tc.desc: transcoder pure video AVC_Baseline@L1.2_81.0Kbps_320x240.mp4 with cancel before prepare
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TransCoderUnitTest, transcoder_PureVideoAbnormal_case_003, TestSize.Level2)
{
    int32_t srcFd = open((TRANSCODER_ROOT_SRC + "AVC_Baseline@L1.2_81.0Kbps_320x240.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(srcFd >= 0);
    int64_t offset = TRANSCODER_FILE_OFFSET;
    int64_t size = TRANSCODER_FILE_SIZE;
    EXPECT_EQ(MSERR_OK, transcoder_->SetInputFile(srcFd, offset, size));
    int32_t dstFd = open((TRANSCODER_ROOT_DST + "AVC_Baseline@L1.2_81.0Kbps_320x240_dst.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(dstFd >= 0);
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFile(dstFd));
    std::shared_ptr<TransCoderCallbackTest> cb = std::make_shared<TransCoderCallbackTest>();
    EXPECT_EQ(MSERR_OK, transcoder_->SetTransCoderCallback(cb));
    OutputFormatType format = FORMAT_MPEG_4;
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFormat(format));
    VideoCodecFormat encoder = H264;
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncoder(encoder));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncodingBitRate(TRASCODER_VIDEO_ENCODING_BIT_RATE));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoSize(TRANSCODER_BUFFER_WIDTH, TRANSCODER_BUFFER_HEIGHT));
    EXPECT_EQ(MSERR_INVALID_OPERATION, transcoder_->Resume());
    EXPECT_EQ(MSERR_OK, transcoder_->Release());
    close(dstFd);
    close(srcFd);
}

/**
 * @tc.name: transcoder_AudioVideoAbnormal_case_001
 * @tc.desc: transcoder audio and video ChineseColor_H264_AAC_480p_15fps.mp4 with cancel before prepare
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TransCoderUnitTest, transcoder_AudioVideoAbnormal_case_001, TestSize.Level2)
{
    int32_t srcFd = open((TRANSCODER_ROOT_SRC + "ChineseColor_H264_AAC_480p_15fps.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(srcFd >= 0);
    int64_t offset = TRANSCODER_FILE_OFFSET;
    int64_t size = TRANSCODER_FILE_SIZE;
    EXPECT_EQ(MSERR_OK, transcoder_->SetInputFile(srcFd, offset, size));
    int32_t dstFd = open((TRANSCODER_ROOT_DST + "ChineseColor_H264_AAC_480p_15fps_dst.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(dstFd >= 0);
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFile(dstFd));
    std::shared_ptr<TransCoderCallbackTest> cb = std::make_shared<TransCoderCallbackTest>();
    EXPECT_EQ(MSERR_OK, transcoder_->SetTransCoderCallback(cb));
    OutputFormatType format = FORMAT_MPEG_4;
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFormat(format));
    AudioCodecFormat encoderAudio = AAC_LC;
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncoder(encoderAudio));
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncodingBitRate(TRASCODER_AUDIO_ENCODING_BIT_RATE));
    VideoCodecFormat encoder = H264;
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncoder(encoder));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncodingBitRate(TRASCODER_VIDEO_ENCODING_BIT_RATE));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoSize(TRANSCODER_BUFFER_WIDTH, TRANSCODER_BUFFER_HEIGHT));
    EXPECT_EQ(ERROR_WRONG_STATE, transcoder_->Cancel());
    EXPECT_EQ(MSERR_OK, transcoder_->Release());
    close(dstFd);
    close(srcFd);
}

/**
 * @tc.name: transcoder_AudioVideoAbnormal_case_002
 * @tc.desc: transcoder audio and video ChineseColor_H264_AAC_480p_15fps.mp4 with cancel before prepare
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TransCoderUnitTest, transcoder_AudioVideoAbnormal_case_002, TestSize.Level2)
{
    int32_t srcFd = open((TRANSCODER_ROOT_SRC + "ChineseColor_H264_AAC_480p_15fps.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(srcFd >= 0);
    int64_t offset = TRANSCODER_FILE_OFFSET;
    int64_t size = TRANSCODER_FILE_SIZE;
    EXPECT_EQ(MSERR_OK, transcoder_->SetInputFile(srcFd, offset, size));
    int32_t dstFd = open((TRANSCODER_ROOT_DST + "ChineseColor_H264_AAC_480p_15fps_dst.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(dstFd >= 0);
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFile(dstFd));
    std::shared_ptr<TransCoderCallbackTest> cb = std::make_shared<TransCoderCallbackTest>();
    EXPECT_EQ(MSERR_OK, transcoder_->SetTransCoderCallback(cb));
    OutputFormatType format = FORMAT_MPEG_4;
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFormat(format));
    AudioCodecFormat encoderAudio = AAC_LC;
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncoder(encoderAudio));
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncodingBitRate(TRASCODER_AUDIO_ENCODING_BIT_RATE));
    VideoCodecFormat encoder = H264;
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncoder(encoder));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncodingBitRate(TRASCODER_VIDEO_ENCODING_BIT_RATE));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoSize(TRANSCODER_BUFFER_WIDTH, TRANSCODER_BUFFER_HEIGHT));
    EXPECT_EQ(MSERR_INVALID_OPERATION, transcoder_->Pause());
    EXPECT_EQ(MSERR_OK, transcoder_->Release());
    close(dstFd);
    close(srcFd);
}

/**
 * @tc.name: transcoder_AudioVideoAbnormal_case_003
 * @tc.desc: transcoder audio and video ChineseColor_H264_AAC_480p_15fps.mp4 with cancel before prepare
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TransCoderUnitTest, transcoder_AudioVideoAbnormal_case_003, TestSize.Level2)
{
    int32_t srcFd = open((TRANSCODER_ROOT_SRC + "ChineseColor_H264_AAC_480p_15fps.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(srcFd >= 0);
    int64_t offset = TRANSCODER_FILE_OFFSET;
    int64_t size = TRANSCODER_FILE_SIZE;
    EXPECT_EQ(MSERR_OK, transcoder_->SetInputFile(srcFd, offset, size));
    int32_t dstFd = open((TRANSCODER_ROOT_DST + "ChineseColor_H264_AAC_480p_15fps_dst.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(dstFd >= 0);
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFile(dstFd));
    std::shared_ptr<TransCoderCallbackTest> cb = std::make_shared<TransCoderCallbackTest>();
    EXPECT_EQ(MSERR_OK, transcoder_->SetTransCoderCallback(cb));
    OutputFormatType format = FORMAT_MPEG_4;
    EXPECT_EQ(MSERR_OK, transcoder_->SetOutputFormat(format));
    AudioCodecFormat encoderAudio = AAC_LC;
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncoder(encoderAudio));
    EXPECT_EQ(MSERR_OK, transcoder_->SetAudioEncodingBitRate(TRASCODER_AUDIO_ENCODING_BIT_RATE));
    VideoCodecFormat encoder = H264;
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncoder(encoder));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoEncodingBitRate(TRASCODER_VIDEO_ENCODING_BIT_RATE));
    EXPECT_EQ(MSERR_OK, transcoder_->SetVideoSize(TRANSCODER_BUFFER_WIDTH, TRANSCODER_BUFFER_HEIGHT));
    EXPECT_EQ(MSERR_INVALID_OPERATION, transcoder_->Resume());
    EXPECT_EQ(MSERR_OK, transcoder_->Release());
    close(dstFd);
    close(srcFd);
}
} // namespace Media
} // namespace OHOS