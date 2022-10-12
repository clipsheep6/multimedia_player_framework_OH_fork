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

#include "player_unit_test.h"
#include "media_errors.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media::PlayerTestParam;

namespace OHOS {
namespace Media {
void PlayerUnitTest::SetUpTestCase(void) {}

void PlayerUnitTest::TearDownTestCase(void) {}

void PlayerUnitTest::SetUp(void)
{
    callback_ = std::make_shared<PlayerCallbackTest>();
    ASSERT_NE(nullptr, callback_);
    player_ = std::make_shared<PlayerMock>(callback_);
    ASSERT_NE(nullptr, player_);
    EXPECT_TRUE(player_->CreatePlayer());
    EXPECT_EQ(MSERR_OK, player_->SetPlayerCallback(callback_));
}

void PlayerUnitTest::TearDown(void)
{
    if (player_ != nullptr) {
        EXPECT_EQ(MSERR_OK, player_->Release());
    }
}

/**
 * @tc.name  : Test PlayerSetSource API
 * @tc.number: Player_SetSource_001
 * @tc.desc  : Test PlayerSetSource interface 
 */
HWTEST_F(PlayerUnitTest, Player_SetSource_001, TestSize.Level0)
{
    int32_t ret = player_->SetSource(VIDEO_FILE1);
    EXPECT_EQ(MSERR_OK, ret);
}

/**
 * @tc.name  : Test PlayerSetSource API
 * @tc.number: Player_SetSource_002
 * @tc.desc  : Test PlayerSetSource interface with invalid path
 */
HWTEST_F(PlayerUnitTest, Player_SetSource_002, TestSize.Level1)
{
    int32_t ret = player_->SetSource(MEDIA_ROOT + "kong.mp4");
    EXPECT_NE(MSERR_OK, ret);
}

/**
 * @tc.name  : Test PlayerSetSource API
 * @tc.number: Player_SetSource_003
 * @tc.desc  : Test PlayerSetSource interface with wrong mp3
 */
HWTEST_F(PlayerUnitTest, Player_SetSource_003, TestSize.Level2)
{
    int32_t ret = player_->SetSource(MEDIA_ROOT + "1kb.mp3");
    EXPECT_EQ(MSERR_OK, ret);
    EXPECT_NE(MSERR_OK, player_->PrepareAsync());
}

/**
 * @tc.name  : Test PlayerPrepare API
 * @tc.number: Player_Prepare_001
 * @tc.desc  : Test PlayerPrepare interface
 */
HWTEST_F(PlayerUnitTest, Player_Prepare_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
}

/**
 * @tc.name  : Test PlayerPrepareAsync API
 * @tc.number: Player_PrepareAsync_001
 * @tc.desc  : Test PlayerPrepareAsync interface
 */
HWTEST_F(PlayerUnitTest, Player_PrepareAsync_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
}

/**
 * @tc.name  : Test PlayerSetVideoSurface API
 * @tc.number: Player_SetVideoSurface_001
 * @tc.desc  : Test PlayerSetVideoSurface interface
 */
HWTEST_F(PlayerUnitTest, Player_SetVideoSurface_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(player_->GetVideoSurface()));
}

/**
 * @tc.name  : Test PlayerPlay API
 * @tc.number: Player_Play_001
 * @tc.desc  : Test PlayerPlay interface
 */
HWTEST_F(PlayerUnitTest, Player_Play_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
}

/**
 * @tc.name  : Test PlayerStop API
 * @tc.number: Player_PreStop_001
 * @tc.desc  : Test PlayerStop interface
 */
HWTEST_F(PlayerUnitTest, Player_Stop_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1, 0, 0));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Stop());
}

/**
 * @tc.name  : Test PlayerPause API
 * @tc.number: Player_Pause_001
 * @tc.desc  : Test PlayerPause interface
 */
HWTEST_F(PlayerUnitTest, Player_Pause_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Pause());
    EXPECT_FALSE(player_->IsPlaying());
}

/**
 * @tc.name  : Test PlayerSeek API
 * @tc.number: Player_Seek_001
 * @tc.desc  : Test PlayerSeek interface with valid parameters
 */
HWTEST_F(PlayerUnitTest, Player_Seek_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_CLOSEST));
    int32_t time;
    EXPECT_EQ(MSERR_OK, player_->GetCurrentTime(time));
    EXPECT_NEAR(SEEK_TIME_2_SEC, time, DELTA_TIME);
}

/**
 * @tc.name  : Test GetVideoTrackInfo API
 * @tc.number: Player_GetVideoTrackInfo_001
 * @tc.desc  : Test Player GetVideoTrackInfo
 */
HWTEST_F(PlayerUnitTest, Player_GetVideoTrackInfo_001, TestSize.Level0)
{
    std::vector<Format> videoTrack;
    std::vector<Format> audioTrack;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
    EXPECT_EQ(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
}

/**
 * @tc.name  : Test GetVideoHeight API
 * @tc.number: Player_GetVideoHeight_001
 * @tc.desc  : Test Player GetVideoHeight
 */
HWTEST_F(PlayerUnitTest, Player_GetVideoHeight_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(480, player_->GetVideoHeight());
    EXPECT_EQ(720, player_->GetVideoWidth());
}

/**
 * @tc.name  : Test GetDuration API
 * @tc.number: Player_GetDuration_001
 * @tc.desc  : Test Player GetDuration
 */
HWTEST_F(PlayerUnitTest, Player_GetDuration_001, TestSize.Level0)
{
    int32_t duration = 0;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(MSERR_OK, player_->GetDuration(duration));
    EXPECT_NEAR(10000, duration, DELTA_TIME); // duration 10000ms
}

/**
 * @tc.name  : Test SetPlaybackSpeed API
 * @tc.number: Player_SetPlaybackSpeed_001
 * @tc.desc  : Test Player SetPlaybackSpeed
 */
HWTEST_F(PlayerUnitTest, Player_SetPlaybackSpeed_001, TestSize.Level0)
{
    PlaybackRateMode mode;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X));
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_EQ(SPEED_FORWARD_2_00_X, mode);
}

/**
 * @tc.name  : Test SetLooping API
 * @tc.number: Player_SetLooping_001
 * @tc.desc  : Test Player SetLooping
 */
HWTEST_F(PlayerUnitTest, Player_SetLooping_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(MSERR_OK, player_->SetLooping(true));
    EXPECT_EQ(true, player_->IsLooping());
    EXPECT_EQ(MSERR_OK, player_->SetLooping(false));
    EXPECT_EQ(false, player_->IsLooping());
}

/**
 * @tc.name  : Test SetVolume API
 * @tc.number: Player_SetVolume_001
 * @tc.desc  : Test Player SetVolume
 */
HWTEST_F(PlayerUnitTest, Player_SetVolume_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(MSERR_OK, player_->SetVolume(0.1, 0.1));
}

/**
 * @tc.name  : Test SetVideoScaleType API
 * @tc.number: Player_SetVideoScaleType_001
 * @tc.desc  : Test Player SetVideoScaleType
 */
HWTEST_F(PlayerUnitTest, Player_SetVideoScaleType_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    Format format;
    format.PutIntValue(PlayerKeys::VIDEO_SCALE_TYPE, VideoScaleType::VIDEO_SCALE_TYPE_FIT);
    EXPECT_EQ(MSERR_OK, player_->SetParameter(format));
    format.PutIntValue(PlayerKeys::VIDEO_SCALE_TYPE, VideoScaleType::VIDEO_SCALE_TYPE_FIT_CROP);
    EXPECT_EQ(MSERR_OK, player_->SetParameter(format));
}

/**
 * @tc.name  : Test SetRendererInfo API
 * @tc.number: Player_SetRendererInfo_001
 * @tc.desc  : Test Player SetRendererInfo
 */
HWTEST_F(PlayerUnitTest, Player_SetRendererInfo_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    Format format;
    int32_t contentType = 1;
    int32_t streamUsage = 1;
    int32_t rendererFlags = 1;
    (void)format.PutIntValue(PlayerKeys::CONTENT_TYPE, contentType);
    (void)format.PutIntValue(PlayerKeys::STREAM_USAGE, streamUsage);
    (void)format.PutIntValue(PlayerKeys::RENDERER_FLAG, rendererFlags);
    EXPECT_EQ(MSERR_OK, player_->SetParameter(format));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
}

/**
 * @tc.name  : Test SetRendererInfo API
 * @tc.number: Player_SetRendererInfo_001
 * @tc.desc  : Test Player SetRendererInfo
 */
HWTEST_F(PlayerUnitTest, Player_SetAudio_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    Format format;
    int32_t contentType = 1;
    int32_t streamUsage = 1;
    int32_t rendererFlags = 1;
    (void)format.PutIntValue(PlayerKeys::CONTENT_TYPE, contentType);
    (void)format.PutIntValue(PlayerKeys::STREAM_USAGE, streamUsage);
    (void)format.PutIntValue(PlayerKeys::RENDERER_FLAG, rendererFlags);
    EXPECT_EQ(MSERR_OK, player_->SetParameter(format));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
}

/**
 * @tc.name  : Test SetDataSource API
 * @tc.number: Player_SetDataSource_001
 * @tc.desc  : Test Player SetDataSource
 */
HWTEST_F(PlayerUnitTest, Player_SetDataSource_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetDataSrc("/data/test/H264_AAC.mp4", -1, true));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC));
}

/**
 * @tc.name  : Test Player SelectBitRate API
 * @tc.number: Player_SelectBitRate_001
 * @tc.desc  : Test Player SelectBitRate interface
 */
HWTEST_F(PlayerUnitTest, Player_SelectBitRate_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_NE(MSERR_OK, player_->SelectBitRate(0));
}

/**
 * @tc.name: Player_Performance_Prepared_001
 * @tc.desc: test player start
 * @tc.type: PERFORMANCE
 * @tc.require: issueI5NYBJ
 */
HWTEST_F(PlayerUnitTest, Player_Performance_Prepared_001, TestSize.Level0)
{
    struct timeval startTime = {};
    struct timeval finishTime = {};
    int32_t runTimes = 10;
    float timeConv = 1000;
    float deltaTime = 0;
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    for (int32_t i = 0; i < runTimes; i++) {
        EXPECT_EQ(MSERR_OK, gettimeofday(&startTime, nullptr));
        ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
        EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
        EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
        EXPECT_EQ(MSERR_OK, gettimeofday(&finishTime, nullptr));
        EXPECT_EQ(MSERR_OK, player_->Play());
        deltaTime += (finishTime.tv_sec - startTime.tv_sec) * timeConv +
            (finishTime.tv_usec - startTime.tv_usec) / timeConv;
        EXPECT_EQ(MSERR_OK, player_->Reset());
    }
    EXPECT_LE(deltaTime / runTimes, 300); // less than 300ms
}

/**
 * @tc.name  : Test Player Play mp4 with rotation
 * @tc.number: Player_Rotate_001
 * @tc.desc  : Test PlayerPlay interface
 */
HWTEST_F(PlayerUnitTest, Player_Rotate_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "MP4_ROTATE_90.mp4"));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
}

/**
 * @tc.name  : Test Player Dump Dot
 * @tc.number: Player_Dump_Dot_001
 * @tc.desc  : Test Player Dump Dot
 */
HWTEST_F(PlayerUnitTest, Player_Dump_Dot_001, TestSize.Level0)
{
    system("setenforce 0");
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "MP4_ROTATE_90.mp4"));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Play());
    system("param set sys.media.dump.dot.path /data/test/media");
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Pause());
    system("param set sys.media.dump.dot.path /xx");
    EXPECT_EQ(MSERR_OK, player_->Play());
    system("setenforce 1");
}

/**
 * @tc.name  : Test Player Dump GlibMem
 * @tc.number: Player_Dump_GlibMem_001
 * @tc.desc  : Test Player Dump GlibMem
 */
HWTEST_F(PlayerUnitTest, Player_Dump_GlibMem_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "MP4_ROTATE_90.mp4"));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Play());
    system("hidumper -s 3002 -a glibmem");
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Pause());
}

/**
 * @tc.name  : Test Player Dump GlibPool
 * @tc.number: Player_Dump_GlibPool_001
 * @tc.desc  : Test Player Dump GlibPool
 */
HWTEST_F(PlayerUnitTest, Player_Dump_GlibPool_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "MP4_ROTATE_90.mp4"));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Play());
    system("hidumper -s 3002 -a glibpool");
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Pause());
}

/**
 * @tc.name  : Test Player Histreamer
 * @tc.number: Player_Histreamer_001
 * @tc.desc  : Test Player function with Histreamer 
 */
HWTEST_F(PlayerUnitTest, Player_Histreamer_001, TestSize.Level0)
{
    EXPECT_EQ(MSERR_OK, player_->Release());
    system("param set debug.media_service.histreamer 1");
    callback_ = std::make_shared<PlayerCallbackTest>();
    ASSERT_NE(nullptr, callback_);
    player_ = std::make_shared<PlayerMock>(callback_);
    ASSERT_NE(nullptr, player_);
    EXPECT_TRUE(player_->CreatePlayer());
    EXPECT_EQ(MSERR_OK, player_->SetPlayerCallback(callback_));

    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Pause());
    int32_t time;
    EXPECT_EQ(MSERR_OK, player_->GetCurrentTime(time));
    std::vector<Format> videoTrack;
    std::vector<Format> audioTrack;
    EXPECT_EQ(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
    EXPECT_EQ(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
    int32_t duration = 0;
    EXPECT_EQ(MSERR_OK, player_->GetDuration(duration));
    EXPECT_NEAR(10000, duration, DELTA_TIME); // duration 10000ms
    PlaybackRateMode mode;
    player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X);
    player_->GetPlaybackSpeed(mode);
    EXPECT_EQ(MSERR_OK, player_->SetLooping(true));
    EXPECT_EQ(true, player_->IsLooping());
    EXPECT_EQ(MSERR_OK, player_->SetLooping(false));
    EXPECT_EQ(false, player_->IsLooping());
    EXPECT_EQ(MSERR_OK, player_->SetVolume(0.1, 0.1));
    Format format;
    format.PutIntValue(PlayerKeys::VIDEO_SCALE_TYPE, VideoScaleType::VIDEO_SCALE_TYPE_FIT);
    player_->SetParameter(format);
    EXPECT_EQ(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC));
    EXPECT_EQ(MSERR_OK, player_->Stop());
    EXPECT_EQ(MSERR_OK, player_->Reset());
    EXPECT_EQ(MSERR_OK, player_->Release());
    player_ = nullptr;
    system("param set debug.media_service.histreamer 0");
}
} // namespace Media
} // namespace OHOS
