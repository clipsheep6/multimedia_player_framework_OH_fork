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
void PlayerUnitTest::SetUpTestCase(void)
{
    system("param set debug.media_service.histreamer 0");
}

void PlayerUnitTest::TearDownTestCase(void)
{
    system("param set debug.media_service.histreamer 0");
}

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
        player_->Release();
    }
}

void PlayerUnitTest::PlayFunTest(const std::string &protocol)
{
    int32_t duration = 0;
    if (player_ != nullptr) {
        EXPECT_EQ(MSERR_OK, player_->GetDuration(duration));
        EXPECT_EQ(MSERR_OK, player_->Play());
        EXPECT_TRUE(player_->IsPlaying());
        EXPECT_EQ(MSERR_OK, player_->GetDuration(duration));
        EXPECT_EQ(MSERR_OK, player_->Pause());
        int32_t time;
        EXPECT_EQ(MSERR_OK, player_->GetCurrentTime(time));
        std::vector<Format> videoTrack;
        std::vector<Format> audioTrack;
        EXPECT_EQ(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
        EXPECT_EQ(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
        PlaybackRateMode mode;
        player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X);
        player_->GetPlaybackSpeed(mode);
        EXPECT_EQ(MSERR_OK, player_->SetLooping(true));
        EXPECT_EQ(true, player_->IsLooping());
        EXPECT_EQ(MSERR_OK, player_->Seek(duration, SEEK_NEXT_SYNC));
        EXPECT_EQ(MSERR_OK, player_->Play());
        sleep(PLAYING_TIME);
        if (protocol == PlayerTestParam::HLS_PLAY) {
            EXPECT_EQ(MSERR_OK, player_->SelectBitRate(200000));  // 200000:bitrate
            sleep(PLAYING_TIME);
        }
        EXPECT_EQ(MSERR_OK, player_->SetLooping(false));
        EXPECT_EQ(false, player_->IsLooping());
        EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1));
        EXPECT_EQ(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC));
        EXPECT_EQ(MSERR_OK, player_->Stop());
        EXPECT_EQ(MSERR_OK, player_->Reset());
    }
}

void PlayerUnitTest::GetSetParaFunTest()
{
    if (player_ != nullptr) {
        int32_t duration = 0;
        int32_t time = 0;
        PlaybackRateMode mode;
        std::vector<Format> videoTrack;
        std::vector<Format> audioTrack;
        player_->GetVideoTrackInfo(videoTrack);
        player_->GetAudioTrackInfo(audioTrack);
        player_->GetCurrentTime(time);
        player_->GetDuration(duration);
        player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X);
        player_->GetPlaybackSpeed(mode);
        player_->SetLooping(true);
        player_->IsLooping();
        player_->SetVolume(1, 1);
    }
}

/**
 * @tc.name  : Test Player SetSource API
 * @tc.number: Player_SetSource_001
 * @tc.desc  : Test Player SetSource interface
 */
HWTEST_F(PlayerUnitTest, Player_SetSource_001, TestSize.Level0)
{
    int32_t ret = player_->SetSource(VIDEO_FILE1);
    EXPECT_EQ(MSERR_OK, ret);
}

/**
 * @tc.name  : Test Player SetSource API
 * @tc.number: Player_SetSource_002
 * @tc.desc  : Test Player SetSource interface with invalid path
 */
HWTEST_F(PlayerUnitTest, Player_SetSource_002, TestSize.Level1)
{
    int32_t ret = player_->SetSource(MEDIA_ROOT + "kong.mp4");
    EXPECT_NE(MSERR_OK, ret);
}

/**
 * @tc.name  : Test Player SetSource API
 * @tc.number: Player_SetSource_003
 * @tc.desc  : Test Player SetSource interface with wrong mp3
 */
HWTEST_F(PlayerUnitTest, Player_SetSource_003, TestSize.Level2)
{
    PlaybackRateMode mode;
    int32_t time = 0;
    int32_t duration = 0;
    std::vector<Format> videoTrack;
    std::vector<Format> audioTrack;
    int32_t ret = player_->SetSource(MEDIA_ROOT + "1kb.mp3");
    EXPECT_EQ(MSERR_OK, ret);
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_NE(MSERR_OK, player_->PrepareAsync());
    EXPECT_NE(MSERR_OK, player_->Prepare());
    Format format;
    format.PutIntValue(PlayerKeys::VIDEO_SCALE_TYPE, VideoScaleType::VIDEO_SCALE_TYPE_FIT);
    EXPECT_NE(MSERR_OK, player_->SetParameter(format));
    EXPECT_NE(MSERR_OK, player_->SetVolume(1, 1));
    EXPECT_NE(MSERR_OK, player_->Play());
    EXPECT_EQ(false, player_->IsPlaying());
    EXPECT_NE(MSERR_OK, player_->Pause());
    EXPECT_NE(MSERR_OK, player_->Seek(0, SEEK_CLOSEST));
    EXPECT_NE(MSERR_OK, player_->SetLooping(true));
    EXPECT_EQ(false, player_->IsLooping());
    EXPECT_NE(MSERR_OK, player_->SetVolume(1, 1));
    EXPECT_NE(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X));
    EXPECT_NE(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_NE(MSERR_OK, player_->GetCurrentTime(time));
    EXPECT_NE(MSERR_OK, player_->GetDuration(duration));
    EXPECT_NE(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
    EXPECT_NE(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
    EXPECT_NE(480, player_->GetVideoHeight());
    EXPECT_NE(720, player_->GetVideoWidth());
    EXPECT_NE(MSERR_OK, player_->Stop());
    EXPECT_EQ(MSERR_OK, player_->Reset());
}

/**
 * @tc.name  : Test Player SetSource API
 * @tc.number: Player_SetSource_004
 * @tc.desc  : Test Player SetSource interface with txt
 */
HWTEST_F(PlayerUnitTest, Player_SetSource_004, TestSize.Level2)
{
    int32_t ret = player_->SetSource(MEDIA_ROOT + "error.mp4");
    EXPECT_EQ(MSERR_OK, ret);
    EXPECT_NE(MSERR_OK, player_->PrepareAsync());
}

/**
 * @tc.name  : Test Player SetSource
 * @tc.number: Player_SetSource_005
 * @tc.desc  : Test Player SetSource interface
 */
HWTEST_F(PlayerUnitTest, Player_SetSource_005, TestSize.Level3)
{
    PlaybackRateMode mode;
    int32_t duration = 0;
    std::vector<Format> videoTrack;
    std::vector<Format> audioTrack;
    Format format;
    format.PutIntValue(PlayerKeys::VIDEO_SCALE_TYPE, VideoScaleType::VIDEO_SCALE_TYPE_FIT);
    EXPECT_EQ(MSERR_OK, player_->SetParameter(format));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_NE(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_NE(MSERR_OK, player_->PrepareAsync());
    EXPECT_NE(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->SetParameter(format));
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1));
    EXPECT_NE(MSERR_OK, player_->Play());
    EXPECT_NE(MSERR_OK, player_->Pause());
    EXPECT_NE(MSERR_OK, player_->Seek(0, SEEK_CLOSEST));
    EXPECT_EQ(MSERR_OK, player_->SetLooping(true));
    EXPECT_EQ(true, player_->IsLooping());
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1));
    EXPECT_NE(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X));
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_EQ(SPEED_FORWARD_1_00_X, mode);
    EXPECT_NE(MSERR_OK, player_->GetDuration(duration));
    EXPECT_NE(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
    EXPECT_NE(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
    EXPECT_NE(480, player_->GetVideoHeight());
    EXPECT_NE(720, player_->GetVideoWidth());
    EXPECT_NE(MSERR_OK, player_->Stop());
    EXPECT_NE(MSERR_OK, player_->Reset());
}

/**
 * @tc.name  : Test Player SetSource API
 * @tc.number: Player_SetSource_001
 * @tc.desc  : Test Player SetSource interface
 */
HWTEST_F(PlayerUnitTest, Player_SetSource_006, TestSize.Level2)
{
    int32_t ret = player_->SetSource(VIDEO_FILE1);
    EXPECT_EQ(MSERR_OK, ret);
    EXPECT_EQ(MSERR_OK, player_->Reset());
    EXPECT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
}

/**
 * @tc.name  : Test Player Local
 * @tc.number: Player_Local_001
 * @tc.desc  : Test Player Local source
 */
HWTEST_F(PlayerUnitTest, Player_Local_001, TestSize.Level2)
{
    int32_t ret = player_->SetSource(MEDIA_ROOT + "AVC_Baseline@L1.2_81.0Kbps_320x240.mp4");
    EXPECT_EQ(MSERR_OK, ret);
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    ret = player_->PrepareAsync();
    if (ret == MSERR_OK) {
        PlayFunTest(LOCAL_PLAY);
    }
}

/**
 * @tc.name  : Test Player Local
 * @tc.number: Player_Local_002
 * @tc.desc  : Test Player Local source
 */
HWTEST_F(PlayerUnitTest, Player_Local_002, TestSize.Level2)
{
    int32_t ret = player_->SetSource(MEDIA_ROOT + "ChineseColor_H264_AAC_480p_15fps.mp4");
    EXPECT_EQ(MSERR_OK, ret);
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    ret = player_->PrepareAsync();
    if (ret == MSERR_OK) {
        PlayFunTest(LOCAL_PLAY);
    }
}

/**
 * @tc.name  : Test Player Local
 * @tc.number: Player_Local_003
 * @tc.desc  : Test Player Local source
 */
HWTEST_F(PlayerUnitTest, Player_Local_003, TestSize.Level2)
{
    int32_t ret = player_->SetSource(MEDIA_ROOT + "H264_MP3.mp4");
    EXPECT_EQ(MSERR_OK, ret);
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    ret = player_->PrepareAsync();
    if (ret == MSERR_OK) {
        PlayFunTest(LOCAL_PLAY);
    }
}

/**
 * @tc.name  : Test Player Local
 * @tc.number: Player_Local_004
 * @tc.desc  : Test Player Local source
 */
HWTEST_F(PlayerUnitTest, Player_Local_004, TestSize.Level2)
{
    int32_t ret = player_->SetSource(MEDIA_ROOT + "MPEG2_AAC.mp4");
    EXPECT_EQ(MSERR_OK, ret);
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    ret = player_->PrepareAsync();
    if (ret == MSERR_OK) {
        PlayFunTest(LOCAL_PLAY);
    }
}

/**
 * @tc.name  : Test Player Local
 * @tc.number: Player_Local_005
 * @tc.desc  : Test Player Local source
 */
HWTEST_F(PlayerUnitTest, Player_Local_005, TestSize.Level2)
{
    int32_t ret = player_->SetSource(MEDIA_ROOT + "MPEG2_MP3.mp4");
    EXPECT_EQ(MSERR_OK, ret);
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    ret = player_->PrepareAsync();
    if (ret == MSERR_OK) {
        PlayFunTest(LOCAL_PLAY);
    }
}

/**
 * @tc.name  : Test Player Local
 * @tc.number: Player_Local_006
 * @tc.desc  : Test Player Local source
 */
HWTEST_F(PlayerUnitTest, Player_Local_006, TestSize.Level2)
{
    int32_t ret = player_->SetSource(MEDIA_ROOT + "MPEG4_AAC.mp4");
    EXPECT_EQ(MSERR_OK, ret);
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    ret = player_->PrepareAsync();
    if (ret == MSERR_OK) {
        PlayFunTest(LOCAL_PLAY);
    }
}

/**
 * @tc.name  : Test Player Local
 * @tc.number: Player_Local_007
 * @tc.desc  : Test Player Local source
 */
HWTEST_F(PlayerUnitTest, Player_Local_007, TestSize.Level2)
{
    int32_t ret = player_->SetSource(MEDIA_ROOT + "MPEG4_MP3.mp4");
    EXPECT_EQ(MSERR_OK, ret);
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    ret = player_->PrepareAsync();
    if (ret == MSERR_OK) {
        PlayFunTest(LOCAL_PLAY);
    }
}

/**
 * @tc.name  : Test Player Local
 * @tc.number: Player_Local_008
 * @tc.desc  : Test Player Local source
 */
HWTEST_F(PlayerUnitTest, Player_Local_008, TestSize.Level2)
{
    int32_t ret = player_->SetSource(MEDIA_ROOT + "out_170_170.mp4");
    EXPECT_EQ(MSERR_OK, ret);
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    ret = player_->PrepareAsync();
    if (ret == MSERR_OK) {
        PlayFunTest(LOCAL_PLAY);
    }
}

/**
 * @tc.name  : Test Player Local
 * @tc.number: Player_Local_009
 * @tc.desc  : Test Player Local source
 */
HWTEST_F(PlayerUnitTest, Player_Local_009, TestSize.Level2)
{
    int32_t ret = player_->SetSource(MEDIA_ROOT + "H264_AAC_320x240.mp4");
    EXPECT_EQ(MSERR_OK, ret);
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    ret = player_->PrepareAsync();
    if (ret == MSERR_OK) {
        PlayFunTest(LOCAL_PLAY);
    }
}

/**
 * @tc.name  : Test Player Local
 * @tc.number: Player_Local_010
 * @tc.desc  : Test Player Local source
 */
HWTEST_F(PlayerUnitTest, Player_Local_010, TestSize.Level2)
{
    int32_t ret = player_->SetSource(MEDIA_ROOT + "aac_44100Hz_143kbs_stereo.aac");
    EXPECT_EQ(MSERR_OK, ret);
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    ret = player_->PrepareAsync();
    if (ret == MSERR_OK) {
        PlayFunTest(LOCAL_PLAY);
    }
}

/**
 * @tc.name  : Test Player Local
 * @tc.number: Player_Local_011
 * @tc.desc  : Test Player Local source
 */
HWTEST_F(PlayerUnitTest, Player_Local_011, TestSize.Level2)
{
    int32_t ret = player_->SetSource(MEDIA_ROOT + "mp3_48000Hz_64kbs_mono.mp3");
    EXPECT_EQ(MSERR_OK, ret);
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    ret = player_->PrepareAsync();
    if (ret == MSERR_OK) {
        PlayFunTest(LOCAL_PLAY);
    }
}

/**
 * @tc.name  : Test Player Local
 * @tc.number: Player_Local_013
 * @tc.desc  : Test Player Local source
 */
HWTEST_F(PlayerUnitTest, Player_Local_013, TestSize.Level2)
{
    int32_t ret = player_->SetSource(MEDIA_ROOT + "pcm_s16le_48000Hz_768kbs_mono.wav");
    EXPECT_EQ(MSERR_OK, ret);
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    ret = player_->PrepareAsync();
    if (ret == MSERR_OK) {
        PlayFunTest(LOCAL_PLAY);
    }
}

/**
 * @tc.name  : Test Player Local
 * @tc.number: Player_Local_014
 * @tc.desc  : Test Player Local source
 */
HWTEST_F(PlayerUnitTest, Player_Local_014, TestSize.Level2)
{
    int32_t ret = player_->SetSource(MEDIA_ROOT + "vorbis_48000Hz_80kbs_mono.ogg");
    EXPECT_EQ(MSERR_OK, ret);
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    ret = player_->PrepareAsync();
    if (ret == MSERR_OK) {
        PlayFunTest(LOCAL_PLAY);
    }
}

/**
 * @tc.name  : Test Player Local
 * @tc.number: Player_Local_015
 * @tc.desc  : Test Player Local source
 */
HWTEST_F(PlayerUnitTest, Player_Local_015, TestSize.Level2)
{
    int32_t ret = player_->SetSource(MEDIA_ROOT + "aac_48000Hz_70kbs_mono.m4a");
    EXPECT_EQ(MSERR_OK, ret);
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    ret = player_->PrepareAsync();
    if (ret == MSERR_OK) {
        PlayFunTest(LOCAL_PLAY);
    }
}

/**
 * @tc.name  : Test Player SetPlayerCallback API
 * @tc.number: Player_SetPlayerCallback_001
 * @tc.desc  : Test Player SetPlayerCallback interface
 */
HWTEST_F(PlayerUnitTest, Player_SetPlayerCallback_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    std::shared_ptr<PlayerCallbackTest> callback = std::make_shared<PlayerCallbackTest>();
    ASSERT_NE(nullptr, callback);
    EXPECT_NE(MSERR_OK, player_->SetPlayerCallback(callback));
    player_->Reset();
    player_->SetPlayerCallback(callback);
}

/**
 * @tc.name  : Test Player Prepare API
 * @tc.number: Player_Prepare_001
 * @tc.desc  : Test Player Prepare interface
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
 * @tc.name  : Test Player Prepare API
 * @tc.number: Player_Prepare_002
 * @tc.desc  : Test Player Prepare->Prepare
 */
HWTEST_F(PlayerUnitTest, Player_Prepare_002, TestSize.Level2)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_NE(MSERR_OK, player_->Prepare());
}

/**
 * @tc.name  : Test Player Prepare API
 * @tc.number: Player_Prepare_003
 * @tc.desc  : Test Player SetVolume/SetLooping/SetPlaybackSpeed->Prepare
 */
HWTEST_F(PlayerUnitTest, Player_Prepare_003, TestSize.Level2)
{
    PlaybackRateMode mode;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->SetLooping(true));
    EXPECT_EQ(true, player_->IsLooping());
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1));
    EXPECT_NE(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X));
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_NE(SPEED_FORWARD_2_00_X, mode);
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(true, player_->IsLooping());
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_NE(SPEED_FORWARD_2_00_X, mode);
}

/**
 * @tc.name  : Test Player Prepare API
 * @tc.number: Player_Prepare_004
 * @tc.desc  : Test Player Stop->Prepare
 */
HWTEST_F(PlayerUnitTest, Player_Prepare_004, TestSize.Level2)
{
    PlaybackRateMode mode;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->SetLooping(true));
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1));
    EXPECT_EQ(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X));
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_EQ(mode, SPEED_FORWARD_2_00_X);
    EXPECT_EQ(MSERR_OK, player_->Stop());
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(true, player_->IsLooping());
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_EQ(mode, SPEED_FORWARD_2_00_X);
}

/**
 * @tc.name  : Test Player PrepareAsync API
 * @tc.number: Player_PrepareAsync_001
 * @tc.desc  : Test Player PrepareAsync interface
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
 * @tc.name  : Test Player PrepareAsync API
 * @tc.number: Player_PrepareAsync_002
 * @tc.desc  : Test Player PrepareAsync->PrepareAsync
 */
HWTEST_F(PlayerUnitTest, Player_PrepareAsync_002, TestSize.Level2)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_NE(MSERR_OK, player_->PrepareAsync());
}

/**
 * @tc.name  : Test Player PrepareAsync API
 * @tc.number: Player_PrepareAsync_003
 * @tc.desc  : Test Player SetVolume/SetLooping/SetPlaybackSpeed->PrepareAsync
 */
HWTEST_F(PlayerUnitTest, Player_PrepareAsync_003, TestSize.Level2)
{
    PlaybackRateMode mode;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->SetLooping(true));
    EXPECT_EQ(true, player_->IsLooping());
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1));
    EXPECT_NE(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X));
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_NE(SPEED_FORWARD_2_00_X, mode);
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(true, player_->IsLooping());
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_NE(SPEED_FORWARD_2_00_X, mode);
}

/**
 * @tc.name  : Test Player PrepareAsync API
 * @tc.number: Player_PrepareAsync_004
 * @tc.desc  : Test Player Stop->PrepareAsync
 */
HWTEST_F(PlayerUnitTest, Player_PrepareAsync_004, TestSize.Level2)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Stop());
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
}

/**
 * @tc.name  : Test Player SetVideoSurface API
 * @tc.number: Player_SetVideoSurface_001
 * @tc.desc  : Test Player SetVideoSurface interface
 */
HWTEST_F(PlayerUnitTest, Player_SetVideoSurface_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(player_->GetVideoSurface()));
}

/**
 * @tc.name  : Test Player SetVideoSurface API
 * @tc.number: Player_SetVideoSurface_002
 * @tc.desc  : Test Player PrepareAsync->SetVideoSurface
 */
HWTEST_F(PlayerUnitTest, Player_SetVideoSurface_002, TestSize.Level2)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_NE(MSERR_OK, player_->SetVideoSurface(videoSurface));
}

/**
 * @tc.name  : Test Player SetVideoSurface API
 * @tc.number: Player_SetVideoSurface_003
 * @tc.desc  : Test Player SetVideoSurface interface
 */
HWTEST_F(PlayerUnitTest, Player_SetVideoSurface_003, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_NE(MSERR_OK, player_->SetVideoSurface(videoSurface));
}

/**
 * @tc.name  : Test Player Play API
 * @tc.number: Player_Play_001
 * @tc.desc  : Test Player Play interface
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
    EXPECT_EQ(MSERR_OK, player_->Pause());
    EXPECT_EQ(MSERR_OK, player_->Play());
}

/**
 * @tc.name  : Test Player Play API
 * @tc.number: Player_Play_002
 * @tc.desc  : Test Player Reset->Play
 */
HWTEST_F(PlayerUnitTest, Player_Play_002, TestSize.Level2)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Reset());
    EXPECT_NE(MSERR_OK, player_->Play());
}

/**
 * @tc.name  : Test Player Play API
 * @tc.number: Player_Play_003
 * @tc.desc  : Test Player complete->Play
 */
HWTEST_F(PlayerUnitTest, Player_Play_003, TestSize.Level2)
{
    int32_t duration = 0;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->GetDuration(duration));
    EXPECT_EQ(MSERR_OK, player_->Seek(duration, SEEK_CLOSEST));
    sleep(PLAYING_TIME);
    EXPECT_EQ(MSERR_OK, player_->Play());
}

/**
 * @tc.name  : Test Player Play API
 * @tc.number: Player_Play_004
 * @tc.desc  : Test Player Play->Play
 */
HWTEST_F(PlayerUnitTest, Player_Play_004, TestSize.Level2)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_NE(MSERR_OK, player_->Play());
}

/**
 * @tc.name  : Test Player Stop API
 * @tc.number: Player_Stop_001
 * @tc.desc  : Test Player Stop Play->Stop
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
 * @tc.name  : Test Player Stop API
 * @tc.number: Player_Stop_002
 * @tc.desc  : Test Player Stop Prepare->Stop
 */
HWTEST_F(PlayerUnitTest, Player_Stop_002, TestSize.Level2)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1, 0, 0));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Stop());
}

/**
 * @tc.name  : Test Player Stop API
 * @tc.number: Player_Stop_003
 * @tc.desc  : Test Player Stop complete/stop->Stop
 */
HWTEST_F(PlayerUnitTest, Player_Stop_003, TestSize.Level2)
{
    int32_t duration = 0;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1, 0, 0));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->GetDuration(duration));
    EXPECT_EQ(MSERR_OK, player_->Seek(duration, SEEK_CLOSEST));
    sleep(PLAYING_TIME);
    EXPECT_EQ(MSERR_OK, player_->Stop());
    EXPECT_NE(MSERR_OK, player_->Stop());
}

/**
 * @tc.name  : Test Player Stop API
 * @tc.number: Player_Stop_004
 * @tc.desc  : Test Player Stop Reset->Stop
 */
HWTEST_F(PlayerUnitTest, Player_Stop_004, TestSize.Level2)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1, 0, 0));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(MSERR_OK, player_->Reset());
    EXPECT_NE(MSERR_OK, player_->Stop());
}

/**
 * @tc.name  : Test Player Stop API
 * @tc.number: Player_Stop_005
 * @tc.desc  : Test Player Reset->Stop
 */
HWTEST_F(PlayerUnitTest, Player_Stop_005, TestSize.Level2)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1, 0, 0));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Reset());
    EXPECT_NE(MSERR_OK, player_->Stop());
}

/**
 * @tc.name  : Test Player Pause API
 * @tc.number: Player_Pause_001
 * @tc.desc  : Test Player Pause interface
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
    EXPECT_NE(MSERR_OK, player_->Pause());
}

/**
 * @tc.name  : Test Player Pause API
 * @tc.number: Player_Pause_002
 * @tc.desc  : Test Player Pause interface
 */
HWTEST_F(PlayerUnitTest, Player_Pause_002, TestSize.Level2)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_NE(MSERR_OK, player_->Pause());
}

/**
 * @tc.name  : Test Player Reset API
 * @tc.number: Player_Reset_001
 * @tc.desc  : Test Player Reset interface
 */
HWTEST_F(PlayerUnitTest, Player_Reset_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Reset());
    EXPECT_NE(MSERR_OK, player_->Reset());
}

/**
 * @tc.name  : Test Player Seek API
 * @tc.number: Player_Seek_001
 * @tc.desc  : Test Player Seek interface with valid parameters
 */
HWTEST_F(PlayerUnitTest, Player_Seek_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC));
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_CLOSEST));
    int32_t time = 0;
    EXPECT_EQ(MSERR_OK, player_->GetCurrentTime(time));
    EXPECT_NEAR(SEEK_TIME_2_SEC, time, DELTA_TIME);
    EXPECT_EQ(MSERR_OK, player_->Pause());
    EXPECT_EQ(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC));
}

/**
 * @tc.name  : Test Player Seek API
 * @tc.number: Player_Seek_002
 * @tc.desc  : Test Player Seek interface with seek mode
 */
HWTEST_F(PlayerUnitTest, Player_Seek_002, TestSize.Level1)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Seek(0, SEEK_NEXT_SYNC));
    EXPECT_EQ(MSERR_OK, player_->Seek(0, SEEK_PREVIOUS_SYNC));
    EXPECT_EQ(MSERR_OK, player_->Seek(0, SEEK_CLOSEST_SYNC));
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC));
    EXPECT_EQ(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_PREVIOUS_SYNC));
    EXPECT_EQ(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_CLOSEST_SYNC));
    EXPECT_NE(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, (PlayerSeekMode)5));
}

/**
 * @tc.name  : Test Player Seek API
 * @tc.number: Player_Seek_002
 * @tc.desc  : Test Player Seek out of duration
 */
HWTEST_F(PlayerUnitTest, Player_Seek_003, TestSize.Level2)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Seek(1000000, SEEK_NEXT_SYNC));
}

/**
 * @tc.name  : Test Seek API
 * @tc.number: Player_Seek_004
 * @tc.desc  : Test Player Seek
 */
HWTEST_F(PlayerUnitTest, Player_Seek_004, TestSize.Level2)
{
    int32_t duration = 0;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    EXPECT_NE(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_NE(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC));
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC));
    EXPECT_EQ(MSERR_OK, player_->Pause());
    EXPECT_EQ(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC));
    EXPECT_EQ(MSERR_OK, player_->GetDuration(duration));
    EXPECT_EQ(MSERR_OK, player_->Seek(duration, SEEK_CLOSEST));
    EXPECT_EQ(MSERR_OK, player_->Play());
    sleep(PLAYING_TIME);
    EXPECT_EQ(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC));
    EXPECT_EQ(MSERR_OK, player_->Stop());
    EXPECT_NE(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC));
    EXPECT_EQ(MSERR_OK, player_->Reset());
    EXPECT_NE(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC));
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
 * @tc.name  : Test GetVideoTrackInfo API
 * @tc.number: Player_GetVideoTrackInfo_002
 * @tc.desc  : Test Player GetVideoTrackInfo
 */
HWTEST_F(PlayerUnitTest, Player_GetVideoTrackInfo_002, TestSize.Level2)
{
    std::vector<Format> videoTrack;
    std::vector<Format> audioTrack;
    int32_t duration = 0;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    EXPECT_NE(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
    EXPECT_NE(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_NE(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
    EXPECT_NE(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
    EXPECT_EQ(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
    EXPECT_EQ(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
    EXPECT_EQ(MSERR_OK, player_->Pause());
    EXPECT_EQ(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
    EXPECT_EQ(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
    EXPECT_EQ(MSERR_OK, player_->GetDuration(duration));
    EXPECT_EQ(MSERR_OK, player_->Seek(duration, SEEK_CLOSEST));
    EXPECT_EQ(MSERR_OK, player_->Play());
    sleep(PLAYING_TIME);
    EXPECT_EQ(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
    EXPECT_EQ(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
    EXPECT_EQ(MSERR_OK, player_->Stop());
    EXPECT_EQ(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
    EXPECT_EQ(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
    EXPECT_EQ(MSERR_OK, player_->Reset());
    EXPECT_NE(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
    EXPECT_NE(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
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
 * @tc.name  : Test GetVideoHeight API
 * @tc.number: Player_GetVideoHeight_002
 * @tc.desc  : Test Player GetVideoHeight
 */
HWTEST_F(PlayerUnitTest, Player_GetVideoHeight_002, TestSize.Level2)
{
    int32_t duration = 0;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    EXPECT_NE(480, player_->GetVideoHeight());
    EXPECT_NE(720, player_->GetVideoWidth());
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_NE(480, player_->GetVideoHeight());
    EXPECT_NE(720, player_->GetVideoWidth());
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(480, player_->GetVideoHeight());
    EXPECT_EQ(720, player_->GetVideoWidth());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(480, player_->GetVideoHeight());
    EXPECT_EQ(720, player_->GetVideoWidth());
    EXPECT_EQ(MSERR_OK, player_->Pause());
    EXPECT_EQ(480, player_->GetVideoHeight());
    EXPECT_EQ(720, player_->GetVideoWidth());
    EXPECT_EQ(MSERR_OK, player_->GetDuration(duration));
    EXPECT_EQ(MSERR_OK, player_->Seek(duration, SEEK_CLOSEST));
    EXPECT_EQ(MSERR_OK, player_->Play());
    sleep(PLAYING_TIME);
    EXPECT_EQ(480, player_->GetVideoHeight());
    EXPECT_EQ(720, player_->GetVideoWidth());
    EXPECT_EQ(MSERR_OK, player_->Stop());
    EXPECT_EQ(480, player_->GetVideoHeight());
    EXPECT_EQ(720, player_->GetVideoWidth());
    EXPECT_EQ(MSERR_OK, player_->Reset());
    EXPECT_NE(480, player_->GetVideoHeight());
    EXPECT_NE(720, player_->GetVideoWidth());
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
 * @tc.name  : Test GetDuration API
 * @tc.number: Player_GetDuration_002
 * @tc.desc  : Test Player GetDuration
 */
HWTEST_F(PlayerUnitTest, Player_GetDuration_002, TestSize.Level2)
{
    int32_t duration = 0;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    EXPECT_NE(MSERR_OK, player_->GetDuration(duration));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_NE(MSERR_OK, player_->GetDuration(duration));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->GetDuration(duration));
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(MSERR_OK, player_->GetDuration(duration));
    EXPECT_EQ(MSERR_OK, player_->Pause());
    EXPECT_EQ(MSERR_OK, player_->GetDuration(duration));
    EXPECT_EQ(MSERR_OK, player_->Seek(duration, SEEK_CLOSEST));
    EXPECT_EQ(MSERR_OK, player_->Play());
    sleep(PLAYING_TIME);
    EXPECT_EQ(MSERR_OK, player_->GetDuration(duration));
    EXPECT_EQ(MSERR_OK, player_->Stop());
    EXPECT_EQ(MSERR_OK, player_->GetDuration(duration));
    EXPECT_EQ(MSERR_OK, player_->Reset());
    EXPECT_NE(MSERR_OK, player_->GetDuration(duration));
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
 * @tc.name  : Test SetPlaybackSpeed API
 * @tc.number: Player_SetPlaybackSpeed_002
 * @tc.desc  : Test Player SetPlaybackSpeed
 */
HWTEST_F(PlayerUnitTest, Player_SetPlaybackSpeed_002, TestSize.Level2)
{
    int32_t duration = 0;
    PlaybackRateMode mode;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    EXPECT_NE(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X));
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_NE(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X));
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X));
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X));
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_EQ(MSERR_OK, player_->Pause());
    EXPECT_EQ(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_1_75_X));
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_EQ(MSERR_OK, player_->GetDuration(duration));
    EXPECT_EQ(MSERR_OK, player_->Seek(duration, SEEK_CLOSEST));
    EXPECT_EQ(MSERR_OK, player_->Play());
    sleep(PLAYING_TIME);
    EXPECT_EQ(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_0_75_X));
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_EQ(MSERR_OK, player_->Stop());
    EXPECT_NE(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_1_25_X));
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_EQ(MSERR_OK, player_->Reset());
    EXPECT_NE(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X));
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
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
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1));
}

/**
 * @tc.name  : Test SetVolume API
 * @tc.number: Player_SetVolume_002
 * @tc.desc  : Test Player SetVolume
 */
HWTEST_F(PlayerUnitTest, Player_SetVolume_002, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_NE(MSERR_OK, player_->SetVolume(1.1, 0.1));
    EXPECT_NE(MSERR_OK, player_->SetVolume(0.1, 1.1));
    EXPECT_NE(MSERR_OK, player_->SetVolume(-0.1, 0.1));
    EXPECT_NE(MSERR_OK, player_->SetVolume(0.1, -0.1));
}

/**
 * @tc.name  : Test SetVolume API
 * @tc.number: Player_SetVolume_003
 * @tc.desc  : Test Player SetVolume
 */
HWTEST_F(PlayerUnitTest, Player_SetVolume_003, TestSize.Level2)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1));
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1));
    EXPECT_EQ(MSERR_OK, player_->Pause());
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1));
    EXPECT_EQ(MSERR_OK, player_->Stop());
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1));
    EXPECT_EQ(MSERR_OK, player_->Reset());
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1));
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
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    int32_t contentType = 1;
    int32_t streamUsage = 1;
    int32_t rendererFlags = 1;
    (void)format.PutIntValue(PlayerKeys::CONTENT_TYPE, contentType);
    (void)format.PutIntValue(PlayerKeys::STREAM_USAGE, streamUsage);
    (void)format.PutIntValue(PlayerKeys::RENDERER_FLAG, rendererFlags);
    EXPECT_EQ(MSERR_OK, player_->SetParameter(format));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
}

/**
 * @tc.name  : Test SetInterrupt API
 * @tc.number: Player_SetInterrupt_001
 * @tc.desc  : Test Player SetInterrupt
 */
HWTEST_F(PlayerUnitTest, Player_SetInterrupt_001, TestSize.Level0)
{
    Format format;
    int32_t mode = 1;
    int32_t type = 1;
    std::shared_ptr<PlayerMock> player = nullptr;
    std::shared_ptr<PlayerCallbackTest> callback = nullptr;
    callback = std::make_shared<PlayerCallbackTest>();
    ASSERT_NE(nullptr, callback);
    player = std::make_shared<PlayerMock>(callback);
    ASSERT_NE(nullptr, player);
    EXPECT_TRUE(player->CreatePlayer());
    EXPECT_EQ(MSERR_OK, player->SetPlayerCallback(callback));
    ASSERT_EQ(MSERR_OK, player->SetSource(MEDIA_ROOT + "01.mp3"));
    EXPECT_EQ(MSERR_OK, player->Prepare());

    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    (void)format.PutIntValue(PlayerKeys::AUDIO_INTERRUPT_MODE, mode);
    (void)format.PutIntValue(PlayerKeys::AUDIO_INTERRUPT_TYPE, type);
    EXPECT_EQ(MSERR_OK, player->SetParameter(format));
    EXPECT_EQ(MSERR_OK, player->Play());
    sleep(PLAYING_TIME);
    EXPECT_EQ(MSERR_OK, player_->Play());
    sleep(PLAYING_TIME);
    EXPECT_EQ(MSERR_OK, player->ReleaseSync());
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
 * @tc.name  : Test SetDataSource API
 * @tc.number: Player_SetDataSource_002
 * @tc.desc  : Test Player SetDataSource
 */
HWTEST_F(PlayerUnitTest, Player_SetDataSource_002, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetDataSrc("/data/test/H264_AAC.mp4", -1, false));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_NE(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC));
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
    EXPECT_LE(deltaTime / runTimes, 1000); // less than 1000 ms
}

/**
 * @tc.name  : Test Player Play mp4 with rotation
 * @tc.number: Player_Rotate_001
 * @tc.desc  : Test Player Play interface
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
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "MP4_ROTATE_90.mp4"));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Play());
    system("param set sys.media.dump.dot.path /data/test");
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Pause());
    system("param set sys.media.dump.dot.path /xx");
    EXPECT_EQ(MSERR_OK, player_->Play());
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
    system("param set sys.media.dump.codec.vdec ALL");
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Pause());
}

/**
 * @tc.name  : Test Player Dump
 * @tc.number: Player_HiDump_001
 * @tc.desc  : Test Player Dump
 */
HWTEST_F(PlayerUnitTest, Player_HiDump_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "MP4_ROTATE_90.mp4"));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Play());
    system("hidumper -s 3002");
    system("hidumper -s 3002 -a player");
    system("param set sys.media.dump.codec.vdec INPUT");
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
    system("param set sys.media.dump.frame.enable true");
    system("param set sys.media.set.mute TRUE");
    system("param set sys.media.kpi.avsync.log.enable true");
    system("param set sys.media.kpi.opt.renderdelay.enable true");
    EXPECT_EQ(MSERR_OK, player_->Play());
    system("hidumper -s 3002 -a glibpool");
    system("param set sys.media.dump.codec.vdec OUTPUT");
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Pause());
}

/**
 * @tc.name  : Test Player Dump Log
 * @tc.number: Player_Dump_Log_001
 * @tc.desc  : Test Player Dump Log
 */
HWTEST_F(PlayerUnitTest, Player_Dump_Log_001, TestSize.Level0)
{
    system("mkdir /data/media/log");
    system("chmod 777 -R /data/media");
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    system("param set sys.media.log.level *:l,multiqueue,decodecbin:,tsdemux:D,multiqueue:D,hlsdemux:D,souphttpsrc:W");
    system("param set sys.media.log.level *:l,basesrcmmmbasesrcmmmbasesrcmmmbasesrcmmmbasesrcmmmbasesrcmmmbasesrcmmm" \
        "basesrcmmmbasesrcmmmbasesrcmmmbasesrcmmmbasesrcmmmbasesrcmmm:D");
    system("param set sys.media.dump.frame.enable false");
    system("param set sys.media.set.mute FALSE");
    system("param set sys.media.kpi.avsync.log.enable false");
    system("param set sys.media.kpi.opt.renderdelay.enable false");
    EXPECT_EQ(MSERR_OK, player_->Play());
    system("param set sys.media.dump.codec.vdec NULL");
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Pause());
    system("rm -rf /data/media/log");
}

/**
 * @tc.name  : Test Player Dump gstbuffer
 * @tc.number: Player_Dump_GstBuffer_001
 * @tc.desc  : Test Player Dump gstbuffer
 */
HWTEST_F(PlayerUnitTest, Player_Dump_GstBuffer_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    system("param set sys.media.dump.gstbuffer 1");
    system("param set sys.media.set.mute null");
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Pause());
    system("param set sys.media.dump.gstbuffer 0");
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

    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "01.mp3"));
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
    player_->GetVideoTrackInfo(videoTrack);
    player_->GetAudioTrackInfo(audioTrack);
    int32_t duration = 0;
    player_->GetDuration(duration);
    PlaybackRateMode mode;
    player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X);
    player_->GetPlaybackSpeed(mode);
    EXPECT_EQ(MSERR_OK, player_->SetLooping(true));
    EXPECT_EQ(true, player_->IsLooping());
    EXPECT_EQ(MSERR_OK, player_->SetLooping(false));
    EXPECT_EQ(false, player_->IsLooping());
    player_->SetVolume(1, 1);
    Format format;
    format.PutIntValue(PlayerKeys::VIDEO_SCALE_TYPE, VideoScaleType::VIDEO_SCALE_TYPE_FIT);
    player_->SetParameter(format);
    player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC);
    player_->Stop();
    EXPECT_EQ(MSERR_OK, player_->Reset());
    EXPECT_EQ(MSERR_OK, player_->Release());
    player_ = nullptr;
    system("param set debug.media_service.histreamer 0");
}

/**
 * @tc.name  : Test Player Histreamer
 * @tc.number: Player_Histreamer_002
 * @tc.desc  : Test Player function with Histreamer
 */
HWTEST_F(PlayerUnitTest, Player_Histreamer_002, TestSize.Level0)
{
    EXPECT_EQ(MSERR_OK, player_->Release());
    system("param set debug.media_service.histreamer 1");
    callback_ = std::make_shared<PlayerCallbackTest>();
    ASSERT_NE(nullptr, callback_);
    player_ = std::make_shared<PlayerMock>(callback_);
    ASSERT_NE(nullptr, player_);
    EXPECT_TRUE(player_->CreatePlayer());
    EXPECT_EQ(MSERR_OK, player_->SetPlayerCallback(callback_));

    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "01.mp3", 0, 0));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    GetSetParaFunTest();
    player_->PrepareAsync();
    player_->PrepareAsync();
    GetSetParaFunTest();
    player_->Play();
    player_->Play();
    GetSetParaFunTest();
    player_->Pause();
    player_->Pause();
    GetSetParaFunTest();
    player_->Play();
    player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC);
    player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC);
    player_->Pause();
    player_->Seek(SEEK_TIME_2_SEC, SEEK_PREVIOUS_SYNC);
    player_->Seek(SEEK_TIME_2_SEC, SEEK_CLOSEST_SYNC);
    player_->Stop();
    player_->Stop();
    player_->Pause();
    player_->Play();
    GetSetParaFunTest();
    EXPECT_EQ(MSERR_OK, player_->Reset());
    player_->Pause();
    player_->Play();
    GetSetParaFunTest();
    EXPECT_EQ(MSERR_OK, player_->Release());
    player_ = nullptr;
    system("param set debug.media_service.histreamer 0");
}

/**
 * @tc.name  : Test Histreamer
 * @tc.number: Player_Histreamer_003
 * @tc.desc  : Test Player Histreamer
 */
HWTEST_F(PlayerUnitTest, Player_Histreamer_003, TestSize.Level0)
{
    EXPECT_EQ(MSERR_OK, player_->Release());
    system("param set debug.media_service.histreamer 1");
    callback_ = std::make_shared<PlayerCallbackTest>();
    ASSERT_NE(nullptr, callback_);
    player_ = std::make_shared<PlayerMock>(callback_);
    ASSERT_NE(nullptr, player_);
    EXPECT_TRUE(player_->CreatePlayer());
    EXPECT_EQ(MSERR_OK, player_->SetPlayerCallback(callback_));
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "01.mp3", 0, 0));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_NE(MSERR_OK, player_->SetVolume(1.1, 0.1));
    EXPECT_NE(MSERR_OK, player_->SetVolume(0.1, 1.1));
    EXPECT_NE(MSERR_OK, player_->SetVolume(-0.1, 0.1));
    EXPECT_NE(MSERR_OK, player_->SetVolume(0.1, -0.1));
    player_->Seek(0, SEEK_NEXT_SYNC);
    player_->Seek(0, SEEK_PREVIOUS_SYNC);
    player_->Seek(0, SEEK_CLOSEST_SYNC);
    player_->IsPlaying();
    player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC);
    player_->Seek(SEEK_TIME_2_SEC, SEEK_PREVIOUS_SYNC);
    player_->Seek(SEEK_TIME_2_SEC, SEEK_CLOSEST_SYNC);
    player_->Seek(SEEK_TIME_2_SEC, (PlayerSeekMode)5);
    EXPECT_EQ(MSERR_OK, player_->Release());
    player_ = nullptr;
    system("param set debug.media_service.histreamer 0");
}

/**
 * @tc.name  : Test Histreamer
 * @tc.number: Player_Histreamer_004
 * @tc.desc  : Test Player Histreamer
 */
HWTEST_F(PlayerUnitTest, Player_Histreamer_004, TestSize.Level0)
{
    PlaybackRateMode mode;
    int32_t time = 0;
    int32_t duration = 0;
    std::vector<Format> videoTrack;
    std::vector<Format> audioTrack;
    EXPECT_EQ(MSERR_OK, player_->Release());
    system("param set debug.media_service.histreamer 1");
    callback_ = std::make_shared<PlayerCallbackTest>();
    ASSERT_NE(nullptr, callback_);
    player_ = std::make_shared<PlayerMock>(callback_);
    ASSERT_NE(nullptr, player_);
    EXPECT_TRUE(player_->CreatePlayer());
    EXPECT_EQ(MSERR_OK, player_->SetPlayerCallback(callback_));
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "1kb.mp3", 0, 0));
    EXPECT_NE(MSERR_OK, player_->PrepareAsync());
    EXPECT_NE(MSERR_OK, player_->Prepare());
    Format format;
    format.PutIntValue(PlayerKeys::VIDEO_SCALE_TYPE, VideoScaleType::VIDEO_SCALE_TYPE_FIT);
    EXPECT_NE(MSERR_OK, player_->SetParameter(format));
    player_->SetVolume(1, 1);
    EXPECT_NE(MSERR_OK, player_->Play());
    EXPECT_EQ(false, player_->IsPlaying());
    EXPECT_NE(MSERR_OK, player_->Pause());
    EXPECT_NE(MSERR_OK, player_->Seek(0, SEEK_CLOSEST));
    player_->SetLooping(true);
    player_->IsLooping();
    player_->SetVolume(1, 1);
    player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X);
    player_->GetPlaybackSpeed(mode);
    player_->GetCurrentTime(time);
    player_->GetDuration(duration);
    player_->GetVideoTrackInfo(videoTrack);
    player_->GetAudioTrackInfo(audioTrack);
    player_->GetVideoHeight();
    player_->GetVideoWidth();
    EXPECT_NE(MSERR_OK, player_->Stop());
    EXPECT_EQ(MSERR_OK, player_->Reset());
    EXPECT_EQ(MSERR_OK, player_->Release());
    player_ = nullptr;
    system("param set debug.media_service.histreamer 0");
}
} // namespace Media
} // namespace OHOS
