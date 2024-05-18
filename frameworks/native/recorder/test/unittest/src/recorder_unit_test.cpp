/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "recorder_unit_test.h"
#include <fcntl.h>
#include <nativetoken_kit.h>
#include <token_setproc.h>
#include <accesstoken_kit.h>
#include "media_errors.h"
#include "media_log.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;
using namespace OHOS::Media::RecorderTestParam;
namespace OHOS {
namespace Media {
// config for video to request buffer from surface
static VideoRecorderConfig g_videoRecorderConfig;

void RecorderUnitTest::SetUpTestCase(void)
{
    vector<string> permission;
    permission.push_back("ohos.permission.MICROPHONE");
    uint64_t tokenId = 0;

    auto perms = std::make_unique<const char* []>(permission.size());
    for (size_t i = 0; i < permission.size(); i++) {
        perms[i] = permission[i].c_str();
    }
    NativeTokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = static_cast<int32_t>(permission.size()),
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms.get(),
        .acls = nullptr,
        .processName = "recorder_unittest",
        .aplStr = "system_basic",
    };
    tokenId = GetAccessTokenId(&infoInstance);
    if (tokenId == 0) {
        MEDIA_LOGE("Get Access Token Id Failed");
        return;
    }
    int ret = SetSelfTokenID(tokenId);
    if (ret != 0) {
        MEDIA_LOGE("Set Acess Token Id failed");
        return;
    }
    ret = Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
    if (ret < 0) {
        MEDIA_LOGE("Reload Native Token Info Failed");
    }
}

void RecorderUnitTest::TearDownTestCase(void) {}

void RecorderUnitTest::SetUp(void)
{
    recorder_ = std::make_shared<RecorderMock>();
    ASSERT_NE(nullptr, recorder_);
    ASSERT_TRUE(recorder_->CreateRecorder());
}

void RecorderUnitTest::TearDown(void)
{
    if (recorder_ != nullptr) {
        recorder_->Release();
    }
}


/**
 * @tc.name: recorder_GetCurrentCapturerChangeInfo_001
 * @tc.desc: recorder_GetCurrentCapturerChangeInfo_001
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_GetCurrentCapturerChangeInfo_001, TestSize.Level0)
{
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT +
    "recorder_GetCurrentCapturerChangeInfo_001.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_AUDIO, g_videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    AudioRecorderChangeInfo changeInfo;
    EXPECT_EQ(MSERR_OK, recorder_->GetCurrentCapturerChangeInfo(changeInfo));
    ASSERT_TRUE(changeInfo.capturerInfo.sourceType == g_videoRecorderConfig.aSource);
    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_SetLocation_001
 * @tc.desc: record video with setLocation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_SetLocation_001, TestSize.Level2)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_video_SetLocation_001.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    recorder_->SetLocation(1, 1);
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_Repeat_001
 * @tc.desc: record video with Repeat
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_Repeat_001, TestSize.Level2)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_video_Repeat_001.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, g_videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_INVALID_OPERATION, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(AUDIO_VIDEO, g_videoRecorderConfig));

    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_INVALID_OPERATION, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Pause());
    EXPECT_EQ(MSERR_INVALID_OPERATION, recorder_->Pause());
    EXPECT_EQ(MSERR_OK, recorder_->Resume());
    EXPECT_EQ(MSERR_INVALID_OPERATION, recorder_->Resume());
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_SetFileSplitDuration_001
 * @tc.desc: record video with SetFileSplitDuration
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_SetFileSplitDuration_001, TestSize.Level2)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT +
            "recorder_video_SetFileSplitDuration_001.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, g_videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(AUDIO_VIDEO, g_videoRecorderConfig));

    EXPECT_EQ(MSERR_OK, recorder_->Start());
    EXPECT_EQ(MSERR_OK, recorder_->Pause());
    recorder_->SetFileSplitDuration(FileSplitType::FILE_SPLIT_POST, -1, 1000);
    EXPECT_EQ(MSERR_OK, recorder_->Resume());
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_SetAudioEncoder_Error_001
 * @tc.desc: record video with SetAudioEncoder
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_SetAudioEncoder_Error_001, TestSize.Level2)
{
    g_videoRecorderConfig.audioSourceId = 0;
    g_videoRecorderConfig.audioFormat = AUDIO_DEFAULT;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT +
            "recorder_video_SetAudioEncoder_Error_001.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_NE(MSERR_OK,
              recorder_->SetAudioEncoder(g_videoRecorderConfig.audioSourceId, g_videoRecorderConfig.audioFormat));
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_GetSurface_Error_001
 * @tc.desc: record video with GetSurface
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_GetSurface_Error_001, TestSize.Level2)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_video_GetSurface_Error_001.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    OHOS::sptr<OHOS::Surface> surface = recorder_->GetSurface(2);
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_SetVideoSourceRepeat_001
 * @tc.desc: record video with SetFileSplitDuration
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_SetVideoSourceRepeat_001, TestSize.Level2)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT +
            "recorder_video_SetVideoSourceRepeat_001.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, g_videoRecorderConfig));
    int32_t videoSourceIdTwo = 0;
    EXPECT_NE(MSERR_OK, recorder_->SetVideoSource(VIDEO_SOURCE_SURFACE_ES, videoSourceIdTwo));
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_SetAudioSourceRepeat_001
 * @tc.desc: record video with SetFileSplitDuration
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_SetAudioSourceRepeat_001, TestSize.Level2)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.aSource = AUDIO_SOURCE_DEFAULT;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT +
            "recorder_video_SetAudioSourceRepeat_001.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, g_videoRecorderConfig));
    int32_t audioSourceIdTwo = 0;
    EXPECT_NE(MSERR_OK, recorder_->SetAudioSource(AUDIO_SOURCE_DEFAULT, audioSourceIdTwo));
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_DrainBufferTrue_001
 * @tc.desc: record video with DrainBufferTrue, stop true
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_DrainBufferTrue_001, TestSize.Level2)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    videoRecorderConfig.videoFormat = H264;
    videoRecorderConfig.outPutFormat = FORMAT_DEFAULT;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_video_DrainBufferTrue_001.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(AUDIO_VIDEO, videoRecorderConfig));

    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(true));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_SetVideoSourceRGBA_001
 * @tc.desc: record video with SetVideoSourceRGBA
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_SetVideoSourceRGBA_001, TestSize.Level2)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_RGBA;
    videoRecorderConfig.videoFormat = H264;
    videoRecorderConfig.outPutFormat = FORMAT_DEFAULT;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_video_SetVideoSourceRGBA_001.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_DrainBufferStarted_001
 * @tc.desc: record video with DrainBufferStarted, stop after pause
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_DrainBufferStarted_001, TestSize.Level2)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    videoRecorderConfig.videoFormat = H264;
    videoRecorderConfig.outPutFormat = FORMAT_DEFAULT;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_video_DrainBufferStarted_001.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(AUDIO_VIDEO, videoRecorderConfig));

    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Pause());
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_configure_001
 * @tc.desc: record with sampleRate -1
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_configure_001, TestSize.Level2)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    videoRecorderConfig.videoFormat = H264;
    videoRecorderConfig.sampleRate = -1;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_configure.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_configure_002
 * @tc.desc: record with channelCount -1
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_configure_002, TestSize.Level2)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    videoRecorderConfig.videoFormat = H264;
    videoRecorderConfig.channelCount = -1;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_configure.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_configure_003
 * @tc.desc: record with audioEncodingBitRate -1
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_configure_003, TestSize.Level2)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    videoRecorderConfig.videoFormat = H264;
    videoRecorderConfig.audioEncodingBitRate = -1;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_configure.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_configure_004
 * @tc.desc: record with videoFormat VIDEO_CODEC_FORMAT_BUTT
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_configure_004, TestSize.Level2)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    videoRecorderConfig.videoFormat = VIDEO_CODEC_FORMAT_BUTT;
    videoRecorderConfig.audioEncodingBitRate = -1;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_configure.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, videoRecorderConfig));
    EXPECT_NE(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_configure_005
 * @tc.desc: record with width -1
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_configure_005, TestSize.Level2)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    videoRecorderConfig.videoFormat = H264;
    videoRecorderConfig.width = -1;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_configure.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, videoRecorderConfig));
    EXPECT_NE(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_configure_006
 * @tc.desc: record with height -1
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_configure_006, TestSize.Level2)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    videoRecorderConfig.videoFormat = H264;
    videoRecorderConfig.height = -1;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_configure.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, videoRecorderConfig));
    EXPECT_NE(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_configure_007
 * @tc.desc: record with frameRate -1
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_configure_007, TestSize.Level2)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    videoRecorderConfig.videoFormat = H264;
    videoRecorderConfig.frameRate = -1;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_configure.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, videoRecorderConfig));
    EXPECT_NE(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_configure_008
 * @tc.desc: record with videoEncodingBitRate -1
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_configure_008, TestSize.Level2)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    videoRecorderConfig.videoFormat = H264;
    videoRecorderConfig.videoEncodingBitRate = -1;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_configure.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_configure_009
 * @tc.desc: record with videoFormat VIDEO_CODEC_FORMAT_BUTT
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_configure_009, TestSize.Level2)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    videoRecorderConfig.videoFormat = VIDEO_CODEC_FORMAT_BUTT;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_configure.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, videoRecorderConfig));
    EXPECT_NE(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_configure_010
 * @tc.desc: record with videoFormat VIDEO_CODEC_FORMAT_BUTT
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_configure_010, TestSize.Level2)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    videoRecorderConfig.videoFormat = H264;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_configure_error.mp4").c_str(), O_RDWR);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, videoRecorderConfig));
    EXPECT_NE(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_configure_011
 * @tc.desc: record with videoFormat FORMAT_BUTT
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_configure_011, TestSize.Level2)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    videoRecorderConfig.videoFormat = H264;
    videoRecorderConfig.outPutFormat = FORMAT_BUTT;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_configure.mp4").c_str(), O_RDWR);

    EXPECT_NE(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, videoRecorderConfig));
    EXPECT_NE(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_configure_012
 * @tc.desc: record with videoFormat FORMAT_DEFAULT
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_configure_012, TestSize.Level2)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    videoRecorderConfig.videoFormat = H264;
    videoRecorderConfig.outPutFormat = FORMAT_DEFAULT;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_configure.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(AUDIO_VIDEO, videoRecorderConfig));

    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Pause());
    EXPECT_EQ(MSERR_OK, recorder_->Resume());
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_configure_013
 * @tc.desc: record
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_configure_013, TestSize.Level2)
{
    EXPECT_NE(MSERR_OK, recorder_->Prepare());

    EXPECT_NE(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_NE(MSERR_OK, recorder_->Pause());
    EXPECT_NE(MSERR_OK, recorder_->Resume());
    EXPECT_NE(MSERR_OK, recorder_->Stop(false));
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_configure_014
 * @tc.desc: record with enableTemporalScale true
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_configure_014, TestSize.Level2)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    videoRecorderConfig.videoFormat = H264;
    videoRecorderConfig.enableTemporalScale = true;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_configure.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_prepare
 * @tc.desc: record prepare
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_prepare, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_prepare.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, g_videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(AUDIO_VIDEO, g_videoRecorderConfig));

    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_yuv_H264
 * @tc.desc: record video with yuv H264
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_yuv_H264, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_video_yuv_H264.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(PURE_VIDEO, g_videoRecorderConfig));

    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    system("hidumper -s 3002 -a recorder");
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_es
 * @tc.desc: record video with es
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_es, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_ES;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_video_es.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(PURE_VIDEO, g_videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Pause());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Resume());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_audio_es
 * @tc.desc: record audio with es
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_audio_es, TestSize.Level0)
{
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_audio_es.m4a").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_AUDIO, g_videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Pause());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Resume());
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_audio_es_0100
 * @tc.desc: record audio with es
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_audio_es_0100, TestSize.Level0)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.outPutFormat = FORMAT_M4A;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_audio_es.m4a").c_str(), O_RDWR);
    ASSERT_TRUE(videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_AUDIO, videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Pause());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Resume());
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_av_yuv_H264
 * @tc.desc: record audio with yuv H264
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_av_yuv_H264, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_av_yuv_H264.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, g_videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(AUDIO_VIDEO, g_videoRecorderConfig));

    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_av_yuv_h264
 * @tc.desc: record audio with yuv h264
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_av_yuv_h264, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_av_yuv_h264.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, g_videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(AUDIO_VIDEO, g_videoRecorderConfig));

    recorder_->Start();
    sleep(RECORDER_TIME);
    recorder_->Stop(false);
    recorder_->StopBuffer(PURE_VIDEO);
    recorder_->Reset();
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_pause_resume
 * @tc.desc: record video, then pause resume
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_pause_resume, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_video_pause_resume.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(PURE_VIDEO, g_videoRecorderConfig));

    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Pause());
    sleep(RECORDER_TIME / 2);
    EXPECT_EQ(MSERR_OK, recorder_->Resume());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_stop_start
 * @tc.desc: record video, then stop start
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_stop_start, TestSize.Level2)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.audioFormat = AUDIO_DEFAULT;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_video_stop_start.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(PURE_VIDEO, g_videoRecorderConfig));

    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    EXPECT_NE(MSERR_OK, recorder_->Start());
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_stop_start
 * @tc.desc: record video, then stop start
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_wrongsize, TestSize.Level2)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_video_wrongsize.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);
    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(PURE_ERROR, g_videoRecorderConfig));
    recorder_->Start();
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    recorder_->Reset();
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_SetOrientationHint_001
 * @tc.desc: record video, SetOrientationHint
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_SetOrientationHint_001, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT +
    "recorder_video_SetOrientationHint_001.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    recorder_->SetLocation(1, 1);
    recorder_->SetOrientationHint(90);
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(PURE_VIDEO, g_videoRecorderConfig));

    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_SetOrientationHint_002
 * @tc.desc: record video, SetOrientationHint
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_SetOrientationHint_002, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT +
        "recorder_video_SetOrientationHint_002.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    recorder_->SetLocation(-91, 0);
    recorder_->SetOrientationHint(720);
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(PURE_VIDEO, g_videoRecorderConfig));

    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_SetOrientationHint_003
 * @tc.desc: record video, SetOrientationHint
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_SetOrientationHint_003, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT +
        "recorder_video_SetOrientationHint_003.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    recorder_->SetLocation(91, 0);
    recorder_->SetOrientationHint(180);
    system("param set sys.media.dump.surfacesrc.enable true");
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(PURE_VIDEO, g_videoRecorderConfig));

    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_SetOrientationHint_004
 * @tc.desc: record video, SetOrientationHint
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_SetOrientationHint_004, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT +
        "recorder_video_SetOrientationHint_004.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    recorder_->SetLocation(1, 181);
    recorder_->SetLocation(1, -181);
    recorder_->SetOrientationHint(270);
    system("param set sys.media.dump.surfacesrc.enable false");
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(PURE_VIDEO, g_videoRecorderConfig));

    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_UpdateRotation_001
 * @tc.desc: record video, Update rotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_UpdateRotation_001, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT +
        "recorder_video_UpdateRotation_001.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    system("param set sys.media.dump.surfacesrc.enable false");
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(PURE_VIDEO, g_videoRecorderConfig));
    recorder_->SetOrientationHint(0);
    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_UpdateRotation_002
 * @tc.desc: record video, Update rotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_UpdateRotation_002, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT +
        "recorder_video_UpdateRotation_002.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    system("param set sys.media.dump.surfacesrc.enable false");
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(PURE_VIDEO, g_videoRecorderConfig));
    recorder_->SetOrientationHint(90);
    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_UpdateRotation_003
 * @tc.desc: record video, Update rotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_UpdateRotation_003, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT +
        "recorder_video_UpdateRotation_003.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    system("param set sys.media.dump.surfacesrc.enable false");
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(PURE_VIDEO, g_videoRecorderConfig));
    recorder_->SetOrientationHint(180);
    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_UpdateRotation_004
 * @tc.desc: record video, Update rotation
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_UpdateRotation_004, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT +
        "recorder_video_UpdateRotation_004.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    system("param set sys.media.dump.surfacesrc.enable false");
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(PURE_VIDEO, g_videoRecorderConfig));
    recorder_->SetOrientationHint(270);
    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_SetCaptureRate_001
 * @tc.desc: record video ,SetCaptureRate
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_SetCaptureRate_001, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_video_SetCaptureRate_001.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);
    recorder_->SetCaptureRate(0, -0.1);
    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_SetMaxFileSize_001
 * @tc.desc: record video ,SetMaxFileSize
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_SetMaxFileSize_001, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_video_SetMaxFileSize_001.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);
    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    recorder_->SetCaptureRate(0, 30);
    recorder_->SetMaxFileSize(-1);
    recorder_->SetMaxFileSize(5000);
    recorder_->SetNextOutputFile(g_videoRecorderConfig.outputFd);
    recorder_->SetFileSplitDuration(FileSplitType::FILE_SPLIT_POST, -1, 1000);
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_SetMaxFileSize_002
 * @tc.desc: record video ,SetMaxFileSize
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_SetMaxFileSize_002, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_video_SetMaxFileSize_002.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);
    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    recorder_->SetMaxFileSize(-1);
    recorder_->SetNextOutputFile(g_videoRecorderConfig.outputFd);
    recorder_->SetFileSplitDuration(FileSplitType::FILE_SPLIT_POST, -1, 1000);
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_SetParameter_001
 * @tc.desc: record video, SetParameter
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_SetParameter_001, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_video_SetParameter_001.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);
    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    Format format;
    format.PutIntValue("SetParameter", 0);
    EXPECT_EQ(MSERR_INVALID_OPERATION, recorder_->SetParameter(0, format));
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_SetDataSource_001
 * @tc.desc: record video, SetDataSource
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_SetDataSource_001, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = H264;
    g_videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_SetDataSource_001.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);
    EXPECT_EQ(MSERR_INVALID_OPERATION,
        recorder_->SetDataSource(DataSourceType::METADATA, g_videoRecorderConfig.videoSourceId));
}

/**
 * @tc.name: recorder_video_SetGenre_001
 * @tc.desc: record video, SetGenre
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_SetGenre_001, TestSize.Level0)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    videoRecorderConfig.videoFormat = H264;
    videoRecorderConfig.outPutFormat = FORMAT_DEFAULT;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_video_SetGenre_001.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, videoRecorderConfig));
    recorder_->SetGenre(videoRecorderConfig.genre);
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(AUDIO_VIDEO, videoRecorderConfig));

    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Pause());
    EXPECT_EQ(MSERR_OK, recorder_->Resume());
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_SetGenre_002
 * @tc.desc: record audio SetGenre
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_SetGenre_002, TestSize.Level0)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.outPutFormat = FORMAT_M4A;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_video_SetGenre_002.m4a").c_str(), O_RDWR);
    ASSERT_TRUE(videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_AUDIO, videoRecorderConfig));
    recorder_->SetGenre(videoRecorderConfig.genre);
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Pause());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Resume());
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_SetCustomInfo_001
 * @tc.desc: record video, SetCustomInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_SetCustomInfo_001, TestSize.Level0)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    videoRecorderConfig.videoFormat = H264;
    videoRecorderConfig.outPutFormat = FORMAT_DEFAULT;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_video_SetCustomInfo_001.mp4").c_str(), O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, videoRecorderConfig));
    Meta customInfo;
    customInfo.SetData("key", "value");
    recorder_->SetUserCustomInfo(customInfo);
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(AUDIO_VIDEO, videoRecorderConfig));

    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Pause());
    EXPECT_EQ(MSERR_OK, recorder_->Resume());
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_SetCustomInfo_002
 * @tc.desc: record audio SetCustomInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_SetCustomInfo_002, TestSize.Level0)
{
    VideoRecorderConfig videoRecorderConfig;
    videoRecorderConfig.outPutFormat = FORMAT_M4A;
    videoRecorderConfig.outputFd = open((RECORDER_ROOT + "recorder_video_SetCustomInfo_002.m4a").c_str(), O_RDWR);
    ASSERT_TRUE(videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_AUDIO, videoRecorderConfig));
    Meta customInfo;
    customInfo.SetData("key", "value");
    recorder_->SetUserCustomInfo(customInfo);
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Pause());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Resume());
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(videoRecorderConfig.outputFd);
}
} // namespace Media
} // namespace OHOS