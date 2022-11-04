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

#include "gtest/gtest.h"
#include "media_errors.h"
#include "vcodec_unit_test.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace OHOS::Media::VCodecTestParam;

void VCodecUnitTest::SetUpTestCase(void) {}

void VCodecUnitTest::TearDownTestCase(void) {}

void VCodecUnitTest::SetUp(void)
{
    createCodecSuccess_ = false;
    std::shared_ptr<VDecSignal> vdecSignal = std::make_shared<VDecSignal>();
    vdecCallback_ = std::make_shared<VDecCallbackTest>(vdecSignal);
    ASSERT_NE(nullptr, vdecCallback_);
    videoDec_ = std::make_shared<VDecMock>(vdecSignal);

    std::shared_ptr<VEncSignal> vencSignal = std::make_shared<VEncSignal>();
    vencCallback_ = std::make_shared<VEncCallbackTest>(vencSignal);
    ASSERT_NE(nullptr, vencCallback_);
    videoEnc_ = std::make_shared<VEncMock>(vencSignal);

    testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    string prefix = "/data/test/media/";
    string fileName = testInfo_->name();
    string suffix = ".es";
    videoEnc_->SetOutPath(prefix + fileName + suffix);
}

bool VCodecUnitTest::CreateVideoCodecByMime(const std::string &decMime, const std::string &encMime)
{
    if (videoDec_->CreateVideoDecMockByMime(decMime) == false ||
        videoEnc_->CreateVideoEncMockByMime(encMime) == false ||
        videoDec_->SetCallback(vdecCallback_) != MSERR_OK ||
        videoEnc_->SetCallback(vencCallback_) != MSERR_OK) {
        return false;
    }
    createCodecSuccess_ = true;
    return true;
}

bool VCodecUnitTest::CreateVideoCodecByName(const std::string &decName, const std::string &encName)
{
    if (videoDec_->CreateVideoDecMockByName(decName) == false ||
        videoEnc_->CreateVideoEncMockByName(encName) == false ||
        videoDec_->SetCallback(vdecCallback_) != MSERR_OK ||
        videoEnc_->SetCallback(vencCallback_) != MSERR_OK) {
        return false;
    }
    createCodecSuccess_ = true;
    return true;
}

void VCodecUnitTest::TearDown(void)
{
    if (videoDec_ != nullptr && createCodecSuccess_) {
        EXPECT_EQ(MSERR_OK, videoDec_->Reset());
        EXPECT_EQ(MSERR_OK, videoDec_->Release());
    }

    if (videoEnc_ != nullptr && createCodecSuccess_) {
        EXPECT_EQ(MSERR_OK, videoEnc_->Reset());
        EXPECT_EQ(MSERR_OK, videoEnc_->Release());
    }
}

/**
 * @tc.name: video_codec_create_0100
 * @tc.desc: video create
 * @tc.type: FUNC
 * @tc.require: issueI5OX06 issueI5P8N0
 */
HWTEST_F(VCodecUnitTest, video_codec_create_0100, TestSize.Level0)
{
    ASSERT_TRUE(CreateVideoCodecByName("avdec_h264", "avenc_mpeg4"));
}

/**
 * @tc.name: video_codec_Configure_0100
 * @tc.desc: video codec Configure
 * @tc.type: FUNC
 * @tc.require: issueI5OX06 issueI5P8N0
 */
HWTEST_F(VCodecUnitTest, video_codec_Configure_0100, TestSize.Level0)
{
    ASSERT_TRUE(CreateVideoCodecByMime("video/avc", "video/avc"));
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frameRate = "frame_rate";
    string maxInputSize = "max_input_size";
    string rotationAngle = "rotation_angle";
    string videoEncodeBitrateMode = "video_encode_bitrate_mode";
    string iFrameInterval = "i_frame_interval";
    string codecQuality = "codec_quality";
    string codecProfile = "codec_profile";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frameRate.c_str(), DEFAULT_FRAME_RATE);
    (void)format->PutIntValue(rotationAngle.c_str(), 0); // set rotation_angle 0
    (void)format->PutIntValue(maxInputSize.c_str(), 15000); // set max input size 15000
    EXPECT_EQ(MSERR_OK, videoDec_->Configure(format));
    (void)format->PutIntValue(videoEncodeBitrateMode.c_str(), 0); // CBR
    (void)format->PutIntValue(iFrameInterval.c_str(), 1); // i_frame_interval 1ms
    (void)format->PutIntValue(codecQuality.c_str(), 0); // set codec_quality 0
    (void)format->PutIntValue(codecProfile.c_str(), 0); // AVC_PROFILE_BASELINE
    EXPECT_EQ(MSERR_OK, videoEnc_->Configure(format));
    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
    format->Destroy();
}

/**
 * @tc.name: video_codec_start_0100
 * @tc.desc: video decodec start
 * @tc.type: FUNC
 * @tc.require: issueI5OX06 issueI5P8N0
 */
HWTEST_F(VCodecUnitTest, video_codec_start_0100, TestSize.Level0)
{
    ASSERT_TRUE(CreateVideoCodecByMime("video/avc", "video/avc"));
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frameRate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frameRate.c_str(), DEFAULT_FRAME_RATE);
    videoDec_->SetSource(H264_SRC_PATH, ES_H264, ES_LENGTH_H264);
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));

    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
    EXPECT_EQ(MSERR_OK, videoDec_->Start());
    EXPECT_EQ(MSERR_OK, videoEnc_->Start());
    format->Destroy();
}

/**
 * @tc.name: video_codec_format_h264_h264_0100
 * @tc.desc: video decodec h264->h264
 * @tc.type: FUNC
 * @tc.require: issueI5OX06 issueI5P8N0
 */
HWTEST_F(VCodecUnitTest, video_codec_format_h264_h264_0100, TestSize.Level0)
{
    ASSERT_TRUE(CreateVideoCodecByMime("video/avc", "video/avc"));
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frameRate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frameRate.c_str(), DEFAULT_FRAME_RATE);
    videoDec_->SetSource(H264_SRC_PATH, ES_H264, ES_LENGTH_H264);
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));

    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
    EXPECT_EQ(MSERR_OK, videoDec_->Start());
    EXPECT_EQ(MSERR_OK, videoEnc_->Start());
    system("hidumper -s 3002 -a codec");
    sleep(10); // start run 10s
    EXPECT_EQ(MSERR_OK, videoDec_->Stop());
    EXPECT_EQ(MSERR_OK, videoEnc_->Stop());
    format->Destroy();
}

/**
 * @tc.name: video_codec_format_h265_h265_0100
 * @tc.desc: video codec h265->h265
 * @tc.type: FUNC
 * @tc.require: issueI5OOKN issueI5OOKW issueI5OX06 issueI5P8N0
 */
HWTEST_F(VCodecUnitTest, video_codec_format_h265_h265_0100, TestSize.Level0)
{
    if (!CreateVideoCodecByName("OMX_hisi_video_decoder_hevc", "OMX_hisi_video_encoder_hevc")) {
        std::cout << "This device does not support hard hevc" << std::endl;
        createCodecSuccess_ = false;
        return;
    }
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frameRate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frameRate.c_str(), DEFAULT_FRAME_RATE);
    videoDec_->SetSource(H265_SRC_PATH, ES_H265, ES_LENGTH_H265);
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));

    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
    EXPECT_EQ(MSERR_OK, videoDec_->Start());
    EXPECT_EQ(MSERR_OK, videoEnc_->Start());
    sleep(10); // start run 10s
    EXPECT_EQ(MSERR_OK, videoDec_->Stop());
    EXPECT_EQ(MSERR_OK, videoEnc_->Stop());
    format->Destroy();
}

/**
 * @tc.name: video_decode_Flush_0100
 * @tc.desc: video decodec flush
 * @tc.type: FUNC
 * @tc.require: issueI5OX06 issueI5P8N0
 */
HWTEST_F(VCodecUnitTest, video_decode_Flush_0100, TestSize.Level0)
{
    ASSERT_TRUE(CreateVideoCodecByMime("video/avc", "video/avc"));
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frameRate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frameRate.c_str(), DEFAULT_FRAME_RATE);
    videoDec_->SetSource(H264_SRC_PATH, ES_H264, ES_LENGTH_H264);
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));

    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
    EXPECT_EQ(MSERR_OK, videoDec_->Start());
    EXPECT_EQ(MSERR_OK, videoEnc_->Start());
    sleep(3); // start run 3s
    EXPECT_EQ(MSERR_OK, videoDec_->Flush());
    sleep(7); // start run 7s
    EXPECT_EQ(MSERR_OK, videoDec_->Stop());
    EXPECT_EQ(MSERR_OK, videoEnc_->Stop());
    format->Destroy();
}

/**
 * @tc.name: video_encode_Flush_0100
 * @tc.desc: video encodec flush
 * @tc.type: FUNC
 * @tc.require: issueI5NYCP issueI5OX06 issueI5P8N0
 */
HWTEST_F(VCodecUnitTest, video_encode_Flush_0100, TestSize.Level0)
{
    ASSERT_TRUE(CreateVideoCodecByMime("video/avc", "video/avc"));
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frameRate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frameRate.c_str(), DEFAULT_FRAME_RATE);
    videoDec_->SetSource(H264_SRC_PATH, ES_H264, ES_LENGTH_H264);
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));

    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
    EXPECT_EQ(MSERR_OK, videoDec_->Start());
    EXPECT_EQ(MSERR_OK, videoEnc_->Start());
    sleep(3); // start run 3s
    EXPECT_EQ(MSERR_OK, videoEnc_->Flush());
    sleep(7); // start run 7s
    EXPECT_EQ(MSERR_OK, videoDec_->Stop());
    EXPECT_EQ(MSERR_OK, videoEnc_->Stop());
    format->Destroy();
}


/**
 * @tc.name: video_codec_abnormal_0100
 * @tc.desc: video codec abnormal func
 * @tc.type: FUNC
 * @tc.require: issueI5NYCP issueI5OX06 issueI5P8N0
 */
HWTEST_F(VCodecUnitTest, video_codec_abnormal_0100, TestSize.Level0)
{
    ASSERT_TRUE(CreateVideoCodecByMime("video/avc", "video/avc"));
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frameRate = "frame_rate";
    string maxInputSize = "max_input_size";
    string rotationAngle = "rotation_angle";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frameRate.c_str(), DEFAULT_FRAME_RATE);
    (void)format->PutIntValue(rotationAngle.c_str(), 20); // invalid rotation_angle 20
    (void)format->PutIntValue(maxInputSize.c_str(), -1); // invalid max input size -1
    videoDec_->SetSource(H264_SRC_PATH, ES_H264, ES_LENGTH_H264);
    videoEnc_->Configure(format);
    videoDec_->Configure(format);
    videoDec_->Prepare();
    videoEnc_->Prepare();
    EXPECT_EQ(MSERR_OK, videoDec_->Reset());
    EXPECT_EQ(MSERR_OK, videoEnc_->Reset());
    ASSERT_NE(MSERR_OK, videoDec_->Start());
    ASSERT_NE(MSERR_OK, videoEnc_->Start());
    ASSERT_NE(MSERR_OK, videoDec_->Flush());
    ASSERT_NE(MSERR_OK, videoEnc_->Flush());
    ASSERT_NE(MSERR_OK, videoDec_->Stop());
    ASSERT_NE(MSERR_OK, videoEnc_->Stop());
    format->Destroy();
}

/**
 * @tc.name: video_codec_SetParameter_0100
 * @tc.desc: video codec SetParameter
 * @tc.type: FUNC
 * @tc.require: issueI5OX06 issueI5P8N0
 */
HWTEST_F(VCodecUnitTest, video_codec_SetParameter_0100, TestSize.Level0)
{
    ASSERT_TRUE(CreateVideoCodecByMime("video/avc", "video/avc"));
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frameRate = "frame_rate";
    string suspendInputSurface = "suspend_input_surface";
    string maxEncoderFps = "max_encoder_fps";
    string repeatFrameAfter = "repeat_frame_after";
    string reqIFrame = "req_i_frame";
    string bitrate = "bitrate";
    string vendorCustom = "vendor.custom";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frameRate.c_str(), DEFAULT_FRAME_RATE);
    videoDec_->SetSource(H264_SRC_PATH, ES_H264, ES_LENGTH_H264);
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    (void)format->PutIntValue(suspendInputSurface.c_str(), 0); // set suspend_input_surface value 0
    (void)format->PutIntValue(maxEncoderFps.c_str(), DEFAULT_FRAME_RATE);
    (void)format->PutIntValue(repeatFrameAfter.c_str(), 1); // set repeat_frame_after 1ms
    (void)format->PutIntValue(reqIFrame.c_str(), 0); // set request i frame false
    (void)format->PutIntValue(bitrate.c_str(), 1000000); // set bitrate 1Mbps
    uint8_t *addr = nullptr;
    size_t size = 0;
    (void)format->PutBuffer(vendorCustom, addr, size);
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));
    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
    EXPECT_EQ(MSERR_OK, videoDec_->Start());
    EXPECT_EQ(MSERR_OK, videoEnc_->Start());
    EXPECT_EQ(MSERR_OK, videoEnc_->SetParameter(format));
    EXPECT_EQ(MSERR_OK, videoDec_->SetParameter(format));
    sleep(5); // start run 5s
    EXPECT_EQ(MSERR_OK, videoDec_->Stop());
    EXPECT_EQ(MSERR_OK, videoEnc_->Stop());
    format->Destroy();
}

/**
 * @tc.name: video_codec_GetOutputMediaDescription_0100
 * @tc.desc: video codec GetOutputMediaDescription
 * @tc.type: FUNC
 * @tc.require: issueI5NYCP issueI5OX06 issueI5P8N0
 */
HWTEST_F(VCodecUnitTest, video_codec_GetOutputMediaDescription_0100, TestSize.Level0)
{
    ASSERT_TRUE(CreateVideoCodecByMime("video/avc", "video/avc"));
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frameRate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frameRate.c_str(), DEFAULT_FRAME_RATE);
    videoDec_->SetSource(H264_SRC_PATH, ES_H264, ES_LENGTH_H264);
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));
    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
    EXPECT_EQ(MSERR_OK, videoDec_->Start());
    EXPECT_EQ(MSERR_OK, videoEnc_->Start());
    sleep(2); // start run 2s
    EXPECT_NE(nullptr, videoDec_->GetOutputMediaDescription());
    EXPECT_NE(nullptr, videoEnc_->GetOutputMediaDescription());
    EXPECT_EQ(MSERR_OK, videoDec_->Stop());
    EXPECT_EQ(MSERR_OK, videoEnc_->Stop());
    format->Destroy();
}

/**
 * @tc.name: video_NotifyEos_0100
 * @tc.desc: video encodec NotifyEos
 * @tc.type: FUNC
 * @tc.require: issueI5NYCF issueI5OX06 issueI5P8N0
*/
HWTEST_F(VCodecUnitTest, video_NotifyEos_0100, TestSize.Level0)
{
    ASSERT_TRUE(CreateVideoCodecByMime("video/avc", "video/avc"));
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frameRate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frameRate.c_str(), DEFAULT_FRAME_RATE);
    videoDec_->SetSource(H264_SRC_PATH, ES_H264, ES_LENGTH_H264);
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));

    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
    EXPECT_EQ(MSERR_OK, videoDec_->Start());
    EXPECT_EQ(MSERR_OK, videoEnc_->Start());
    sleep(10); // start run 10s
    EXPECT_EQ(MSERR_OK, videoEnc_->NotifyEos());
    EXPECT_EQ(MSERR_OK, videoDec_->Stop());
    EXPECT_EQ(MSERR_OK, videoEnc_->Stop());
    format->Destroy();
}

/**
 * @tc.name: video_codec_format_none_0100
 * @tc.desc: video format none
 * @tc.type: FUNC
 * @tc.require: issueI5OX06 issueI5P8N0
*/
HWTEST_F(VCodecUnitTest, video_codec_format_none_0100, TestSize.Level0)
{
    CreateVideoCodecByMime("", "");
    videoDec_->Release();
    videoEnc_->Release();
}

/**
 * @tc.name: video_codec_format_mpeg2_mpeg4_0100
 * @tc.desc: video format decoder-mpeg2 encoder-mpeg4
 * @tc.type: FUNC
 * @tc.require: issueI5OX06 issueI5P8N0
 */
HWTEST_F(VCodecUnitTest, video_codec_format_mpeg2_mpeg4_0100, TestSize.Level0)
{
    ASSERT_TRUE(CreateVideoCodecByMime("video/mpeg2", "video/mp4v-es"));
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frameRate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), 720); // set width 720
    (void)format->PutIntValue(height.c_str(), 480); // set height 480
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frameRate.c_str(), DEFAULT_FRAME_RATE);
    videoDec_->SetSource(MPEG2_SRC_PATH, ES_MPEG2, ES_LENGTH_MPEG2);
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));
    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
    EXPECT_EQ(MSERR_OK, videoDec_->Start());
    EXPECT_EQ(MSERR_OK, videoEnc_->Start());
    sleep(5); // start run 5s
    EXPECT_EQ(MSERR_OK, videoDec_->Stop());
    EXPECT_EQ(MSERR_OK, videoEnc_->Stop());
    format->Destroy();
}

/**
 * @tc.name: video_codec_format_mpeg4_mpeg4_0100
 * @tc.desc: video format decoder-mpeg4 encoder-mpeg4
 * @tc.type: FUNC
 * @tc.require: issueI5OX06 issueI5P8N0
 */
HWTEST_F(VCodecUnitTest, video_codec_format_mpeg4_mpeg4_0100, TestSize.Level0)
{
    ASSERT_TRUE(CreateVideoCodecByMime("video/mp4v-es", "video/mp4v-es"));
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frameRate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frameRate.c_str(), DEFAULT_FRAME_RATE);
    videoDec_->SetSource(MPEG4_SRC_PATH, ES_MPEG4, ES_LENGTH_MPEG4);
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));

    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
    EXPECT_EQ(MSERR_OK, videoDec_->Start());
    EXPECT_EQ(MSERR_OK, videoEnc_->Start());
    sleep(5); // start run 5s
    EXPECT_EQ(MSERR_OK, videoDec_->Stop());
    EXPECT_EQ(MSERR_OK, videoEnc_->Stop());
    format->Destroy();
}