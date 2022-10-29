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
 * @tc.name: video_codec_creat_0100
 * @tc.desc: video create
 * @tc.type: FUNC
 * @tc.require: issueI5OX06 issueI5P8N0
 */
HWTEST_F(VCodecUnitTest, video_codec_creat_0100, TestSize.Level0)
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
    string frame_rate = "frame_rate";
    string max_input_size = "max_input_size";
    string rotation_angle = "rotation_angle";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
    (void)format->PutIntValue(rotation_angle.c_str(), 20); // invalid rotation_angle 20
    (void)format->PutIntValue(max_input_size.c_str(), -1); // invalid max input size -1
    videoEnc_->Configure(format);
    videoDec_->Configure(format);
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
    string frame_rate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
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
 * @tc.name: video_codec_0100
 * @tc.desc: video decodec h264->mpeg4
 * @tc.type: FUNC
 * @tc.require: issueI5OX06 issueI5P8N0
 */
HWTEST_F(VCodecUnitTest, video_codec_0100, TestSize.Level0)
{
    ASSERT_TRUE(CreateVideoCodecByMime("video/avc", "video/avc"));
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frame_rate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
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
    EXPECT_EQ(MSERR_OK, videoDec_->Stop());
    EXPECT_EQ(MSERR_OK, videoEnc_->Stop());
    format->Destroy();
}

/**
 * @tc.name: video_codec_0200
 * @tc.desc: video codec h265->h265
 * @tc.type: FUNC
 * @tc.require: issueI5OOKN issueI5OOKW issueI5OX06 issueI5P8N0
 */
HWTEST_F(VCodecUnitTest, video_codec_0200, TestSize.Level0)
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
    string frame_rate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
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
    string frame_rate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
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
    string frame_rate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
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
    string frame_rate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
    videoDec_->SetSource(H264_SRC_PATH, ES_H264, ES_LENGTH_H264);
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));
    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
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
    string frame_rate = "frame_rate";
    string suspend_input_surface = "suspend_input_surface";
    string max_encoder_fps = "max_encoder_fps";
    string repeat_frame_after = "repeat_frame_after";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
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
    (void)format->PutIntValue(suspend_input_surface.c_str(), 0); // set suspend_input_surface value 0
    (void)format->PutIntValue(max_encoder_fps.c_str(), DEFAULT_FRAME_RATE);
    (void)format->PutIntValue(repeat_frame_after.c_str(), 1); // set repeat_frame_after 1ms
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
    string frame_rate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
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
    string frame_rate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
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
 * @tc.name: video_codec_format_mpeg2_0100
 * @tc.desc: video format decoder-mpeg2 encoder-mpeg4
 * @tc.type: FUNC
 * @tc.require: issueI5OX06 issueI5P8N0
 */
HWTEST_F(VCodecUnitTest, video_codec_format_mpeg2_0100, TestSize.Level0)
{
    ASSERT_TRUE(CreateVideoCodecByMime("video/mpeg2", "video/mp4v-es"));
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frame_rate = "frame_rate";
    string video_encode_bitrate_mode = "video_encode_bitrate_mode";
    string max_input_size = "max_input_size";
    string i_frame_interval = "i_frame_interval";
    string codec_profile = "codec_profile";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
    (void)format->PutIntValue(max_input_size.c_str(), 150000); // max input size 15000
    (void)format->PutIntValue(codec_profile.c_str(), 0); // AVC_PROFILE_BASELINE
    videoDec_->SetSource(H264_SRC_PATH, ES_H264, ES_LENGTH_H264);
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    (void)format->PutIntValue(video_encode_bitrate_mode.c_str(), 0); // CBR
    (void)format->PutIntValue(i_frame_interval.c_str(), 1); // i_frame_interval 1ms
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));

    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
}

/**
 * @tc.name: video_codec_format_mpeg4_0100
 * @tc.desc: video format decoder-mpeg4 encoder-mpeg4
 * @tc.type: FUNC
 * @tc.require: issueI5OX06 issueI5P8N0
 */
HWTEST_F(VCodecUnitTest, video_codec_format_mpeg4_0100, TestSize.Level0)
{
    ASSERT_TRUE(CreateVideoCodecByMime("video/mp4v-es", "video/mp4v-es"));
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frame_rate = "frame_rate";
    string video_encode_bitrate_mode = "video_encode_bitrate_mode";
    string rotation_angle = "rotation_angle";
    string max_input_size = "max_input_size";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
    (void)format->PutIntValue(rotation_angle.c_str(), 90); // rotation_angle 90
    (void)format->PutIntValue(max_input_size.c_str(), 4000000); // max input size 4000000 invalid
    videoDec_->SetSource(H264_SRC_PATH, ES_H264, ES_LENGTH_H264);
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    (void)format->PutIntValue(video_encode_bitrate_mode.c_str(), 1); // VBR
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));

    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
}

/**
 * @tc.name: video_codec_format_h263_0100
 * @tc.desc: video format decoder-h263 encoder-mpeg4
 * @tc.type: FUNC
 * @tc.require: issueI5OX06 issueI5P8N0
 */
HWTEST_F(VCodecUnitTest, video_codec_format_h263_0100, TestSize.Level0)
{
    ASSERT_TRUE(CreateVideoCodecByMime("video/h263", "video/mp4v-es"));
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frame_rate = "frame_rate";
    string video_encode_bitrate_mode = "video_encode_bitrate_mode";

    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
    videoDec_->SetSource(H264_SRC_PATH, ES_H264, ES_LENGTH_H264);
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    (void)format->PutIntValue(video_encode_bitrate_mode.c_str(), 2); //CQ
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));
    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
}