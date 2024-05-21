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
#include <unistd.h>
#include <securec.h>
#include "media_errors.h"
#include "audio_effect.h"
#include "av_common.h"
#include "meta/video_types.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media::PlayerTestParam;

namespace OHOS {
namespace Media {
void PlayerUnitTest::SetUpTestCase(void)
{
}

void PlayerUnitTest::TearDownTestCase(void)
{
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
        sleep(PLAYING_TIME_2_SEC);
        if (protocol == PlayerTestParam::HLS_PLAY) {
            EXPECT_EQ(MSERR_OK, player_->SelectBitRate(200000));  // 200000:bitrate
            sleep(PLAYING_TIME_2_SEC);
        }
        EXPECT_EQ(MSERR_OK, player_->SetLooping(false));
        EXPECT_EQ(false, player_->IsLooping());
        EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1));
        EXPECT_EQ(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC));
        EXPECT_EQ(MSERR_OK, player_->Stop());
        EXPECT_EQ(MSERR_OK, player_->Reset());
    }
}

void PlayerUnitTest::NoRunPlayFunTest(const std::string &protocol)
{
    int32_t duration = 0;
    if (player_ != nullptr) {
        EXPECT_NE(MSERR_OK, player_->GetDuration(duration));
        EXPECT_NE(MSERR_OK, player_->Play());
        EXPECT_FALSE(player_->IsPlaying());
        EXPECT_NE(MSERR_OK, player_->GetDuration(duration));
        EXPECT_NE(MSERR_OK, player_->Pause());
        int32_t time;
        EXPECT_NE(MSERR_OK, player_->GetCurrentTime(time));
        std::vector<Format> videoTrack;
        std::vector<Format> audioTrack;
        EXPECT_NE(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
        EXPECT_NE(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
        PlaybackRateMode mode;
        player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X);
        player_->GetPlaybackSpeed(mode);
        EXPECT_NE(MSERR_OK, player_->SetLooping(true));
        EXPECT_NE(true, player_->IsLooping());
        EXPECT_NE(MSERR_OK, player_->Seek(duration, SEEK_NEXT_SYNC));
        EXPECT_NE(MSERR_OK, player_->Play());
        sleep(PLAYING_TIME_2_SEC);
        if (protocol == PlayerTestParam::HLS_PLAY) {
            EXPECT_NE(MSERR_OK, player_->SelectBitRate(200000));  // 200000:bitrate
            sleep(PLAYING_TIME_2_SEC);
        }
        EXPECT_NE(MSERR_OK, player_->SetLooping(false));
        EXPECT_EQ(false, player_->IsLooping());
        EXPECT_NE(MSERR_OK, player_->SetVolume(1, 1));
        EXPECT_NE(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC));
        EXPECT_NE(MSERR_OK, player_->Stop());
        EXPECT_NE(MSERR_OK, player_->Reset());
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
 * @tc.name  : Test Player SetMediaSource API
 * @tc.number: Player_SetMediaSource_001
 * @tc.desc  : Test Player SetMediaSource interface
 */
HWTEST_F(PlayerUnitTest, Player_SetMediaSource_001, TestSize.Level0)
{
    std::map<std::string, std::string> header = {
        {"key1", "value1"},
        {"key2", "value2"},
    };
    struct AVPlayStrategy strategy = {1080, 920, 10000, false};
    std::shared_ptr<AVMediaSource> mediaSource = std::make_shared<AVMediaSource>(VIDEO_FILE1, header);
    int32_t ret = player_->SetMediaSource(mediaSource, strategy);
    EXPECT_EQ(MSERR_OK, ret);
}

/**
 * @tc.name  : Test Player SetMediaSource API
 * @tc.number: Player_SetMediaSource_002
 * @tc.desc  : Test Player SetMediaSource interface
 */
HWTEST_F(PlayerUnitTest, Player_SetMediaSource_002, TestSize.Level0)
{
    struct AVPlayStrategy strategy = {1080, 920, 10000, false};
    int32_t ret = player_->SetMediaSource(nullptr, strategy);
    EXPECT_NE(MSERR_OK, ret);
}

/**
 * @tc.name  : Test Player SetMediaSource API
 * @tc.number: Player_SetMediaSource_003
 * @tc.desc  : Test Player SetMediaSource interface
 */
HWTEST_F(PlayerUnitTest, Player_SetMediaSource_003, TestSize.Level0)
{
    std::map<std::string, std::string> header = {
        {"key1", "value1"},
        {"key2", "value2"},
    };
    struct AVPlayStrategy strategy = {1080, 920, 10000, false};
    std::shared_ptr<AVMediaSource> mediaSource = std::make_shared<AVMediaSource>(MEDIA_ROOT + "error.mp4", header);
    int32_t ret = player_->SetMediaSource(mediaSource, strategy);
    EXPECT_EQ(MSERR_OK, ret);
    EXPECT_NE(MSERR_OK, player_->PrepareAsync());
}

/**
 * @tc.name  : Test Player SetMediaSource API
 * @tc.number: Player_SetMediaSource_004
 * @tc.desc  : Test Player SetMediaSource interface
 */
HWTEST_F(PlayerUnitTest, Player_SetMediaSource_004, TestSize.Level0)
{
    std::map<std::string, std::string> header = {
        {"key1", "value1"},
        {"key2", "value2"},
    };
    struct AVPlayStrategy strategy = {1080, 920, 10000, false};
    std::shared_ptr<AVMediaSource> mediaSource = std::make_shared<AVMediaSource>(MEDIA_ROOT + "error.mp4", header);
    int32_t ret = player_->SetMediaSource(mediaSource, strategy);
    EXPECT_EQ(MSERR_OK, ret);
    EXPECT_NE(MSERR_OK, player_->PrepareAsync());
    EXPECT_NE(MSERR_OK, player_->Prepare());
}

/**
 * @tc.name  : Test Player SetMediaSource API
 * @tc.number: Player_SetMediaSource_005
 * @tc.desc  : Test Player SetMediaSource interface
 */
HWTEST_F(PlayerUnitTest, Player_SetMediaSource_005, TestSize.Level0)
{
    std::map<std::string, std::string> header = {
        {"key1", "value1"},
        {"key2", "value2"},
    };
    struct AVPlayStrategy strategy = {1080, 920, 10000, false};
    std::shared_ptr<AVMediaSource> mediaSource = std::make_shared<AVMediaSource>(MEDIA_ROOT + "error.mp4", header);
    int32_t ret = player_->SetMediaSource(mediaSource, strategy);
    EXPECT_EQ(MSERR_OK, ret);
    EXPECT_NE(MSERR_OK, player_->Play());
    EXPECT_EQ(false, player_->IsPlaying());
}

/**
 * @tc.name  : Test Player SetMediaSource API
 * @tc.number: Player_SetMediaSource_006
 * @tc.desc  : Test Player SetMediaSource interface
 */
HWTEST_F(PlayerUnitTest, Player_SetMediaSource_006, TestSize.Level0)
{
    std::map<std::string, std::string> header = {
    };
    struct AVPlayStrategy strategy = {1080, 920, 10000, false};
    std::shared_ptr<AVMediaSource> mediaSource = std::make_shared<AVMediaSource>(MEDIA_ROOT + "error.mp4", header);
    int32_t ret = player_->SetMediaSource(mediaSource, strategy);
    EXPECT_EQ(MSERR_OK, ret);
    EXPECT_NE(MSERR_OK, player_->Play());
    EXPECT_EQ(false, player_->IsPlaying());
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
    system("param set sys.media.player.buffering.enable TRUE");
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
    format.PutIntValue(PlayerKeys::VIDEO_SCALE_TYPE,
        static_cast<int32_t>(Plugins::VideoScaleType::VIDEO_SCALE_TYPE_FIT));
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
    system("param set sys.media.player.buffering.enable FALSE");
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
    format.PutIntValue(PlayerKeys::VIDEO_SCALE_TYPE,
        static_cast<int32_t>(Plugins::VideoScaleType::VIDEO_SCALE_TYPE_FIT));
    EXPECT_NE(MSERR_OK, player_->SetParameter(format));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_NE(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_NE(MSERR_OK, player_->PrepareAsync());
    EXPECT_NE(MSERR_OK, player_->Prepare());
    EXPECT_NE(MSERR_OK, player_->SetParameter(format));
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
 * @tc.number: Player_SetSource_006
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
 * @tc.name  : Test Player SetSource API
 * @tc.number: Player_SetSource_007
 * @tc.desc  : Test Player SetSource interface
 */
HWTEST_F(PlayerUnitTest, Player_SetSource_007, TestSize.Level2)
{
    EXPECT_NE(MSERR_OK, player_->SetSource(INVALID_FILE));
}

/**
 * @tc.name  : Test Player SetSource API
 * @tc.number: Player_SetSource_008
 * @tc.desc  : Test Player SetSource interface
 */
HWTEST_F(PlayerUnitTest, Player_SetSource_008, TestSize.Level2)
{
    int32_t invliadFd = 256; // invliad fd
    EXPECT_NE(MSERR_OK, player_->SetSource(invliadFd, 0, 0));
}

/**
 * @tc.name  : Test Player SetSource API
 * @tc.number: Player_SetSource_009
 * @tc.desc  : Test Player SetSource interface
 */
HWTEST_F(PlayerUnitTest, Player_SetSource_009, TestSize.Level2)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    int32_t ret = player_->PrepareAsync();
    ASSERT_NE(MSERR_OK, player_->SetSource(MEDIA_ROOT + "AVC_Baseline@L1.2_81.0Kbps_320x240.mp4"));
    if (ret == MSERR_OK) {
        PlayFunTest(LOCAL_PLAY);
    }
}

/**
 * @tc.name  : Test Player SetSource API
 * @tc.number: Player_SetSource_010
 * @tc.desc  : Test Player SetSource interface
 */
HWTEST_F(PlayerUnitTest, Player_SetSource_010, TestSize.Level2)
{
    int32_t ret = player_->SetSource("http://domain/H264_MP3.mp4");
    ASSERT_EQ(MSERR_OK, ret);
}

/**
 * @tc.name  : Test Player SetSource API
 * @tc.number: Player_SetSource_011
 * @tc.desc  : Test Player SetSource interface
 */
HWTEST_F(PlayerUnitTest, Player_SetSource_011, TestSize.Level2)
{
    int32_t ret = player_->SetSource("https://domain/H264_MP3.mp4");
    ASSERT_EQ(MSERR_OK, ret);
}

/**
 * @tc.name  : Test Player SetSource API
 * @tc.number: Player_SetSource_012
 * @tc.desc  : Test Player SetSource interface
 */
HWTEST_F(PlayerUnitTest, Player_SetSource_012, TestSize.Level2)
{
    int32_t ret = player_->SetSource(VIDEO_FILE1, 0, 0);
    ASSERT_EQ(MSERR_OK, ret);
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
 * @tc.name  : Test Player Local
 * @tc.number: Player_Local_016
 * @tc.desc  : Test Player Local source
 */
HWTEST_F(PlayerUnitTest, Player_Local_016, TestSize.Level2)
{
    int32_t ret = player_->SetSource(VIDEO_FILE1);
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
 * @tc.number: Player_Local_017
 * @tc.desc  : Test Player Local source
 */
HWTEST_F(PlayerUnitTest, Player_Local_017, TestSize.Level2)
{
    int32_t ret = player_->SetSource(MEDIA_ROOT + "amr_nb_8ksr_7400kbr_1ch.amr");
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
    PlaybackRateMode rateMode;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> renderSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, renderSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(renderSurface));
    EXPECT_EQ(MSERR_OK, player_->SetLooping(true));
    bool ret = player_->IsLooping();
    EXPECT_EQ(true, ret);
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1));
    EXPECT_NE(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X));
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(rateMode));
    EXPECT_NE(SPEED_FORWARD_2_00_X, rateMode);
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(true, player_->IsLooping());
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(rateMode));
    EXPECT_NE(SPEED_FORWARD_2_00_X, rateMode);
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
 * @tc.name  : Test Player Prepare API
 * @tc.number: Player_Prepare_005
 * @tc.desc  : Test Player Play->Prepare
 */
HWTEST_F(PlayerUnitTest, Player_Prepare_005, TestSize.Level2)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_NE(MSERR_OK, player_->Prepare());
}

/**
 * @tc.name  : Test Player SetRenderFirstFrame API
 * @tc.number: Player_SetRenderFirstFrame_001
 * @tc.desc  : Test Player Play->SetRenderFirstFrame
 */
HWTEST_F(PlayerUnitTest, Player_SetRenderFirstFrame_001, TestSize.Level2)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->SetRenderFirstFrame(false));
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
 * @tc.name  : Test Player PrepareAsync API
 * @tc.number: Player_PrepareAsync_005
 * @tc.desc  : Test Player Play->PrepareAsync
 */
HWTEST_F(PlayerUnitTest, Player_PrepareAsync_005, TestSize.Level2)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_NE(MSERR_OK, player_->PrepareAsync());
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
    sleep(PLAYING_TIME_2_SEC);
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
 * @tc.name  : Test Player Play API
 * @tc.number: Player_Play_005
 * @tc.desc  : Test Player is killed Play
 */
HWTEST_F(PlayerUnitTest, Player_Play_005, TestSize.Level2)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    int32_t ret = player_->PrepareAsync();
    EXPECT_EQ(MSERR_OK, player_->Release());
    if (ret == MSERR_OK) {
        NoRunPlayFunTest(LOCAL_PLAY);
    }
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
    sleep(PLAYING_TIME_2_SEC);
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
 * @tc.name  : Test Player Pause API
 * @tc.number: Player_Pause_003
 * @tc.desc  : Test Player Pause interface, Stop -> Pause
 */
HWTEST_F(PlayerUnitTest, Player_Pause_003, TestSize.Level2)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Stop());
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
    EXPECT_EQ(MSERR_OK, player_->Seek(SEEK_TIME_4_SEC, SEEK_NEXT_SYNC));
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Seek(SEEK_TIME_4_SEC, SEEK_CLOSEST));
    int32_t time = 0;
    EXPECT_EQ(MSERR_OK, player_->GetCurrentTime(time));
    EXPECT_NEAR(SEEK_TIME_4_SEC, time, DELTA_TIME);
    EXPECT_EQ(MSERR_OK, player_->Pause());
    EXPECT_EQ(MSERR_OK, player_->Seek(SEEK_TIME_4_SEC, SEEK_NEXT_SYNC));
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
    sleep(PLAYING_TIME_2_SEC);
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
    sleep(PLAYING_TIME_2_SEC);
    EXPECT_EQ(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
    EXPECT_EQ(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
    EXPECT_EQ(MSERR_OK, player_->Stop());
    EXPECT_NE(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
    EXPECT_NE(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
    EXPECT_EQ(MSERR_OK, player_->Reset());
    EXPECT_NE(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
    EXPECT_NE(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
}

/**
 * @tc.name  : Test GetAudioTrackInfo API
 * @tc.number: Player_GetAudioTrackInfo_001
 * @tc.desc  : Test Player GetAudioTrackInfo
 */
HWTEST_F(PlayerUnitTest, Player_GetAudioTrackInfo_001, TestSize.Level2)
{
    std::vector<Format> audioTrack;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    sleep(PLAYING_TIME_2_SEC);
    EXPECT_EQ(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
    EXPECT_EQ(1, audioTrack.size());
    Format audioTrackFormat = audioTrack.front();
    int32_t sampleDepth;
    audioTrackFormat.GetIntValue("sample_depth", sampleDepth);
    EXPECT_EQ(16, sampleDepth);
}

/**
 * @tc.name  : Test SelectTrack and DeselectTrack API
 * @tc.number: Player_SelectTrack_001
 * @tc.desc  : Test Player SelectTrack and DeselectTrack
 */
HWTEST_F(PlayerUnitTest, Player_SelectTrack_001, TestSize.Level0)
{
    bool trackChange = false;
    std::vector<Format> audioTrack;
    std::vector<int32_t> audioTrackIds;
    int32_t currentAudioTrackIndex = -1;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE2));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1));
    sleep(PLAYING_TIME_2_SEC);
    EXPECT_EQ(MSERR_OK, player_->GetCurrentTrack(MediaType::MEDIA_TYPE_AUD, currentAudioTrackIndex));
    EXPECT_EQ(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
    for (Format audioTrackFormat: audioTrack) {
        int32_t trackIndex = -1;
        audioTrackFormat.GetIntValue("track_index", trackIndex);
        audioTrackIds.push_back(trackIndex);
    }
    for (int32_t trackIndex: audioTrackIds) {
        if (trackIndex != currentAudioTrackIndex) {
            trackChange = false;
            EXPECT_EQ(MSERR_OK, player_->SelectTrack(trackIndex, trackChange));
            EXPECT_EQ(trackChange, true);
            EXPECT_EQ(MSERR_OK, player_->GetCurrentTrack(MediaType::MEDIA_TYPE_AUD, currentAudioTrackIndex));
            sleep(PLAYING_TIME_2_SEC);
            trackChange = false;
            EXPECT_EQ(MSERR_OK, player_->DeselectTrack(currentAudioTrackIndex, trackChange));
            EXPECT_EQ(trackChange, true);
            EXPECT_EQ(MSERR_OK, player_->GetCurrentTrack(MediaType::MEDIA_TYPE_AUD, currentAudioTrackIndex));
            sleep(PLAYING_TIME_2_SEC);
        }
    }
    EXPECT_EQ(MSERR_OK, player_->Stop());
}

/**
 * @tc.name  : Test SelectTrack API
 * @tc.number: Player_SelectTrack_002
 * @tc.desc  : Test Player SelectTrack invalid trackId
 */
HWTEST_F(PlayerUnitTest, Player_SelectTrack_002, TestSize.Level0)
{
    bool trackChange = false;
    int32_t currentAudioTrackIndex = -1;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE2));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1));
    sleep(PLAYING_TIME_2_SEC);
    EXPECT_EQ(MSERR_OK, player_->GetCurrentTrack(MediaType::MEDIA_TYPE_AUD, currentAudioTrackIndex));
    EXPECT_NE(MSERR_OK, player_->SelectTrack(currentAudioTrackIndex, trackChange));
    EXPECT_EQ(trackChange, false);
    sleep(PLAYING_TIME_2_SEC);
    EXPECT_EQ(MSERR_OK, player_->Stop());
}

/**
 * @tc.name  : Test DeselectTrack API
 * @tc.number: Player_DeselectTrack_001
 * @tc.desc  : Test Player DeselectTrack invalid trackId
 */
HWTEST_F(PlayerUnitTest, Player_DeselectTrack_001, TestSize.Level0)
{
    std::vector<Format> audioTrack;
    std::vector<int32_t> audioTrackIds;
    int32_t defaultAudioTrackIndex = -1;
    bool trackChange = false;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE2));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1));
    sleep(PLAYING_TIME_2_SEC);
    EXPECT_EQ(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
    EXPECT_GT(audioTrack.size(), 0);
    audioTrack[0].GetIntValue("track_index", defaultAudioTrackIndex);
    EXPECT_GT(defaultAudioTrackIndex, -1);
    EXPECT_NE(MSERR_OK, player_->DeselectTrack(defaultAudioTrackIndex, trackChange));
    EXPECT_EQ(trackChange, false);
    sleep(PLAYING_TIME_2_SEC);
    EXPECT_EQ(MSERR_OK, player_->Stop());
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
    sleep(PLAYING_TIME_2_SEC);
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
    sleep(PLAYING_TIME_2_SEC);
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
    sleep(PLAYING_TIME_2_SEC);
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
    EXPECT_NE(MSERR_OK, player_->SetVolume(1.1, 1.1));
    EXPECT_NE(MSERR_OK, player_->SetVolume(-0.1, 0.1));
    EXPECT_NE(MSERR_OK, player_->SetVolume(0.1, -0.1));
    EXPECT_NE(MSERR_OK, player_->SetVolume(-0.1, -0.1));
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
 * @tc.name  : Test SetVolume API
 * @tc.number: Player_SetVolume_004
 * @tc.desc  : Test Player SetVolume
 */
HWTEST_F(PlayerUnitTest, Player_SetVolume_004, TestSize.Level2)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1e-7, 1));
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1, 1e-7));
    EXPECT_EQ(MSERR_OK, player_->SetVolume(1e-7, 1e-7));
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
    format.PutIntValue(PlayerKeys::VIDEO_SCALE_TYPE,
        static_cast<int32_t>(Plugins::VideoScaleType::VIDEO_SCALE_TYPE_FIT));
    EXPECT_EQ(MSERR_OK, player_->SetParameter(format));
    format.PutIntValue(PlayerKeys::VIDEO_SCALE_TYPE,
        static_cast<int32_t>(Plugins::VideoScaleType::VIDEO_SCALE_TYPE_FIT_CROP));
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
    int32_t rendererFlags = 0;
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
    sleep(PLAYING_TIME_2_SEC);
    EXPECT_EQ(MSERR_OK, player_->Play());
    sleep(PLAYING_TIME_2_SEC);
    EXPECT_EQ(MSERR_OK, player->ReleaseSync());
}

/**
 * @tc.name  : Test SetDataSource API
 * @tc.number: Player_SetDataSource_003
 * @tc.desc  : Test Player SetDataSource
 */
HWTEST_F(PlayerUnitTest, Player_SetDataSource_003, TestSize.Level0)
{
    system("param set sys.media.datasrc.set.copymode TRUE");
    ASSERT_EQ(MSERR_OK, player_->SetDataSrc("/data/test/H264_AAC.mp4", 1894386, true));  // 1894386 file size
    sptr<Surface> renderSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, renderSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(renderSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    sleep(PLAYING_TIME_2_SEC);
    int32_t ret = player_->Seek(SEEK_TIME_2_SEC, SEEK_NEXT_SYNC);
    EXPECT_EQ(MSERR_OK, ret);
    sleep(PLAYING_TIME_2_SEC);
    EXPECT_EQ(MSERR_OK, player_->Pause());
    ret = player_->Play();
    EXPECT_EQ(MSERR_OK, ret);
    sleep(PLAYING_TIME_2_SEC);
    ret = player_->Stop();
    EXPECT_EQ(MSERR_OK, ret);
    EXPECT_EQ(MSERR_OK, player_->Reset());
    ret = player_->Release();
    EXPECT_EQ(MSERR_OK, ret);
    system("param set sys.media.datasrc.set.copymode FALSE");
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
    EXPECT_EQ(MSERR_OK, player_->SelectBitRate(0));
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
    system("param set sys.media.dump.dot.path /data/test/media");
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Pause());
    EXPECT_EQ(MSERR_OK, player_->Play());
}

/**
 * @tc.name  : Test Player Dump Dot
 * @tc.number: Player_Dump_Dot_002
 * @tc.desc  : Test Player Dump Dot
 */
HWTEST_F(PlayerUnitTest, Player_Dump_Dot_002, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "MP4_ROTATE_90.mp4"));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Play());
    system("param set sys.media.dump.dot.path /xx");
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Pause());
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
    system("param set sys.media.dump.codec.vdec ALL");
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
 * @tc.name  : Test Player With Not Performance
 * @tc.number: Player_Not_Performance_001
 * @tc.desc  : Test Player Not Performance
 */
HWTEST_F(PlayerUnitTest, Player_Not_Performance_001, TestSize.Level2)
{
    system("param set sys.media.player.performance.enable FALSE");
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    system("param set sys.media.player.performance.enable TRUE");
}

/**
 * @tc.name  : Test Player Mem Recycle
 * @tc.number: Player_Mem_Recycle_001
 * @tc.desc  : Test Player Mem Recycle
 */
HWTEST_F(PlayerUnitTest, Player_Mem_Recycle_001, TestSize.Level0)
{
    sptr<Surface> renderSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, renderSurface);
    std::vector<std::string> srcVector = {MEDIA_ROOT + "H264_MP3.mp4"};
    for (int32_t i = 0; i < static_cast<int32_t>(srcVector.size()); i++) {
        if (srcVector[i] == MEDIA_ROOT + "H264_MP3.mp4") {
            system("param set sys.media.player.resource.type NetWork");
        }
        ASSERT_EQ(MSERR_OK, player_->SetSource(srcVector[i]));
        EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(renderSurface));
        char text[100]; // 100: text len
        sprintf_s(text, 100, "hidumper -s 1909 -a \"-d %d %d %d\"", 0, 0, 4);
        system(text);
        sprintf_s(text, 100, "hidumper -s 1909 -a \"-d %d %d %d\"", getpid(), 0, 4);
        system(text);
        system("hidumper -s 1909 -a \"-t 6\"");
        system("hidumper -s 1909 -a \"-t 3\"");
        int32_t ret = player_->Prepare();
        EXPECT_EQ(MSERR_OK, ret);
        EXPECT_EQ(MSERR_OK, player_->Reset());
        system("param set sys.media.player.resource.type Local");
    }
}

/**
 * @tc.name  : Test Player Mem Recycle
 * @tc.number: Player_Mem_Recycle_002
 * @tc.desc  : Test Player Mem Recycle
 */
HWTEST_F(PlayerUnitTest, Player_Mem_Recycle_002, TestSize.Level0)
{
    sptr<Surface> vSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, vSurface);
    std::vector<std::string> srcVec = {MEDIA_ROOT + "H264_MP3.mp4"};
    for (uint32_t i = 0; i < srcVec.size(); i++) {
        if (srcVec[i] == MEDIA_ROOT + "H264_MP3.mp4") {
            system("param set sys.media.player.resource.type NetWork");
        }
        ASSERT_EQ(MSERR_OK, player_->SetSource(srcVec[i]));
        EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(vSurface));
        EXPECT_EQ(MSERR_OK, player_->Prepare());
        char str[100]; // 100: str len
        sprintf_s(str, 100, "hidumper -s 1909 -a \"-d %d %d %d\"", getpid(), getuid(), 4);
        system(str);
        system("hidumper -s 1909 -a \"-t 3\"");
        EXPECT_EQ(MSERR_OK, player_->Play());
        EXPECT_EQ(MSERR_OK, player_->Reset());
        system("param set sys.media.player.resource.type Local");
    }
}

/**
 * @tc.name  : Test Player Mem Recycle
 * @tc.number: Player_Mem_Recycle_003
 * @tc.desc  : Test Player Mem Recycle
 */
HWTEST_F(PlayerUnitTest, Player_Mem_Recycle_003, TestSize.Level0)
{
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    std::vector<std::string> srcVec = {MEDIA_ROOT + "H264_MP3.mp4"};
    for (uint32_t i = 0; i < srcVec.size(); i++) {
        if (srcVec[i] == MEDIA_ROOT + "H264_MP3.mp4") {
            system("param set sys.media.player.resource.type NetWork");
        }
        ASSERT_EQ(MSERR_OK, player_->SetSource(srcVec[i]));
        EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
        EXPECT_EQ(MSERR_OK, player_->Prepare());
        EXPECT_EQ(MSERR_OK, player_->Play());
        sleep(15);
        char str[100]; // 100: str len
        sprintf_s(str, 100, "hidumper -s 1909 -a \"-d %d %d %d\"", getpid(), getuid(), 4);
        system(str);
        system("hidumper -s 1909 -a \"-t 3\"");
        EXPECT_EQ(MSERR_OK, player_->Play());
        EXPECT_EQ(MSERR_OK, player_->Reset());
        system("param set sys.media.player.resource.type Local");
    }
}

/**
 * @tc.name  : Test Player Mem Recycle
 * @tc.number: Player_Mem_Recycle_004
 * @tc.desc  : Test Player Mem Recycle
 */
HWTEST_F(PlayerUnitTest, Player_Mem_Recycle_004, TestSize.Level0)
{
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    std::vector<std::string> vec = {MEDIA_ROOT + "H264_MP3.mp4"};
    for (uint32_t i = 0; i < vec.size(); i++) {
        if (vec[i] == MEDIA_ROOT + "H264_MP3.mp4") {
            system("param set sys.media.player.resource.type NetWork");
        }
        const auto ret = MSERR_OK;
        ASSERT_EQ(ret, player_->SetSource(vec[i]));
        EXPECT_EQ(ret, player_->SetVideoSurface(videoSurface));
        EXPECT_EQ(ret, player_->Prepare());
        EXPECT_EQ(ret, player_->Play());
        EXPECT_EQ(ret, player_->Pause());
        char str[100]; // 100: str len
        sprintf_s(str, 100, "hidumper -s 1909 -a \"-d %d %d %d\"", getpid(), getuid(), 4);
        system(str);
        system("hidumper -s 1909 -a \"-t 3\"");
        int32_t currentTime = 0;
        std::vector<Format> videoTrack;
        std::vector<Format> audioTrack;
        int32_t duration = 0;
        PlaybackRateMode mode;
        EXPECT_EQ(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
        EXPECT_EQ(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
        EXPECT_EQ(MSERR_OK, player_->GetCurrentTime(currentTime));
        EXPECT_NE(0, player_->GetVideoWidth());
        EXPECT_NE(0, player_->GetVideoHeight());
        EXPECT_EQ(MSERR_OK, player_->GetDuration(duration));
        EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
        int32_t index;
        EXPECT_EQ(MSERR_OK, player_->GetCurrentTrack(MediaType::MEDIA_TYPE_AUD, index));
        EXPECT_EQ(MSERR_OK, player_->GetCurrentTrack(MediaType::MEDIA_TYPE_VID, index));
        EXPECT_EQ(MSERR_OK, player_->GetCurrentTrack(MediaType::MEDIA_TYPE_SUBTITLE, index));
        EXPECT_NE(MSERR_OK, player_->GetCurrentTrack(100, index));
        EXPECT_EQ(false, player_->IsPlaying());
        EXPECT_EQ(false, player_->IsLooping());
        EXPECT_EQ(MSERR_OK, player_->Seek(1000, SEEK_NEXT_SYNC));
        EXPECT_EQ(MSERR_OK, player_->Reset());
        system("param set sys.media.player.resource.type Local");
    }
}

/**
 * @tc.name  : Test Player Mem Recycle
 * @tc.number: Player_Mem_Recycle_005
 * @tc.desc  : Test Player Mem Recycle
 */
HWTEST_F(PlayerUnitTest, Player_Mem_Recycle_005, TestSize.Level0)
{
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    std::vector<std::string> srcVec = {MEDIA_ROOT + "H264_MP3.mp4"};
    for (uint32_t i = 0; i < srcVec.size(); i++) {
        if (srcVec[i] == MEDIA_ROOT + "H264_MP3.mp4") {
            system("param set sys.media.player.resource.type NetWork");
        }
        const auto result = MSERR_OK;
        ASSERT_EQ(result, player_->SetSource(srcVec[i]));
        EXPECT_EQ(result, player_->SetVideoSurface(videoSurface));
        EXPECT_EQ(result, player_->Prepare());
        EXPECT_EQ(result, player_->Play());
        EXPECT_EQ(result, player_->Pause());
        char str[100]; // 100: str len
        sprintf_s(str, 100, "hidumper -s 1909 -a \"-d %d %d %d\"", getpid(), getuid(), 4);
        system(str);
        system("hidumper -s 1909 -a \"-t 3\"");
        EXPECT_EQ(result, player_->Reset());
        system("param set sys.media.player.resource.type Local");
    }
}

/**
 * @tc.name  : Test Player Mem Recycle
 * @tc.number: Player_Mem_Recycle_006
 * @tc.desc  : Test Player Mem Recycle
 */
HWTEST_F(PlayerUnitTest, Player_Mem_Recycle_006, TestSize.Level0)
{
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    std::vector<std::string> srcVec = {MEDIA_ROOT + "H264_MP3.mp4"};
    for (uint32_t i = 0; i < srcVec.size(); i++) {
        if (srcVec[i] == MEDIA_ROOT + "H264_MP3.mp4") {
            system("param set sys.media.player.resource.type NetWork");
        }
        ASSERT_EQ(MSERR_OK, player_->SetSource(srcVec[i]));
        EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
        EXPECT_EQ(MSERR_OK, player_->Prepare());
        EXPECT_EQ(MSERR_OK, player_->Play());
        EXPECT_EQ(MSERR_OK, player_->Pause());
        char str[100]; // 100: str len
        sprintf_s(str, 100, "hidumper -s 1909 -a \"-d %d %d %d\"", getpid(), getuid(), 4);
        system(str);
        system("hidumper -s 1909 -a \"-t 3\"");
        EXPECT_EQ(MSERR_OK, player_->Reset());
        system("param set sys.media.player.resource.type Local");
    }
}

/**
 * @tc.name  : Test Player Mem Recycle
 * @tc.number: Player_Mem_Recycle_007
 * @tc.desc  : Test Player Mem Recycle
 */
HWTEST_F(PlayerUnitTest, Player_Mem_Recycle_007, TestSize.Level0)
{
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    std::vector<std::string> srcVec = {MEDIA_ROOT + "H264_MP3.mp4"};
    std::vector<PlaybackRateMode> speedMode = {SPEED_FORWARD_2_00_X, SPEED_FORWARD_1_25_X};
    for (uint32_t i = 0; i < srcVec.size(); i++) {
        if (srcVec[i] == MEDIA_ROOT + "H264_MP3.mp4") {
            system("param set sys.media.player.resource.type NetWork");
        }
        ASSERT_EQ(MSERR_OK, player_->SetSource(srcVec[i]));
        EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
        EXPECT_EQ(MSERR_OK, player_->Prepare());
        EXPECT_EQ(MSERR_OK, player_->Play());
        EXPECT_EQ(MSERR_OK, player_->Pause());
        char str[100]; // 100: str len
        sprintf_s(str, 100, "hidumper -s 1909 -a \"-d %d %d %d\"", getpid(), getuid(), 4);
        system(str);
        system("hidumper -s 1909 -a \"-t 3\"");
        EXPECT_EQ(MSERR_OK, player_->SetPlaybackSpeed(speedMode[i]));
        EXPECT_EQ(MSERR_OK, player_->Reset());
        system("param set sys.media.player.resource.type Local");
    }
}

/**
 * @tc.name  : Test Player Mem Recycle
 * @tc.number: Player_Mem_Recycle_008
 * @tc.desc  : Test Player Mem Recycle
 */
HWTEST_F(PlayerUnitTest, Player_Mem_Recycle_008, TestSize.Level0)
{
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    std::vector<std::string> srcVec = {MEDIA_ROOT + "H264_MP3.mp4"};
    for (uint32_t i = 0; i < srcVec.size(); i++) {
        if (srcVec[i] == MEDIA_ROOT + "H264_MP3.mp4") {
            system("param set sys.media.player.resource.type NetWork");
        }
        ASSERT_EQ(MSERR_OK, player_->SetSource(srcVec[i]));
        EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
        EXPECT_EQ(MSERR_OK, player_->Prepare());
        EXPECT_EQ(MSERR_OK, player_->Play());
        EXPECT_EQ(MSERR_OK, player_->Pause());
        EXPECT_EQ(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X));
        EXPECT_EQ(MSERR_OK, player_->SetLooping(true));
        EXPECT_EQ(MSERR_OK, player_->SetVolume(0.5, 0.5));
        char str[100]; // 100: str len
        sprintf_s(str, 100, "hidumper -s 1909 -a \"-d %d %d %d\"", getpid(), getuid(), 4);
        system(str);
        system("hidumper -s 1909 -a \"-t 3\"");
        EXPECT_EQ(MSERR_OK, player_->SetLooping(false));
        EXPECT_EQ(MSERR_OK, player_->Reset());
        system("param set sys.media.player.resource.type Local");
    }
}

/**
 * @tc.name  : Test Player Mem Recycle
 * @tc.number: Player_Mem_Recycle_009
 * @tc.desc  : Test Player Mem Recycle
 */
HWTEST_F(PlayerUnitTest, Player_Mem_Recycle_009, TestSize.Level0)
{
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    std::vector<std::string> srcVec = {MEDIA_ROOT + "H264_MP3.mp4"};
    for (uint32_t i = 0; i < srcVec.size(); i++) {
        if (srcVec[i] == MEDIA_ROOT + "H264_MP3.mp4") {
            system("param set sys.media.player.resource.type NetWork");
        }
        ASSERT_EQ(MSERR_OK, player_->SetSource(srcVec[i]));
        EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
        EXPECT_EQ(MSERR_OK, player_->Prepare());
        EXPECT_EQ(MSERR_OK, player_->Play());
        EXPECT_EQ(MSERR_OK, player_->Pause());
        char str[100]; // 100: str len
        sprintf_s(str, 100, "hidumper -s 1909 -a \"-d %d %d %d\"", getpid(), getuid(), 4);
        system(str);
        system("hidumper -s 1909 -a \"-t 3\"");
        EXPECT_EQ(MSERR_OK, player_->SelectBitRate(0));
        EXPECT_EQ(MSERR_OK, player_->Reset());
        system("param set sys.media.player.resource.type Local");
    }
}

/**
 * @tc.name  : Test Player Mem Recycle
 * @tc.number: Player_Mem_Recycle_010
 * @tc.desc  : Test Player Mem Recycle
 */
HWTEST_F(PlayerUnitTest, Player_Mem_Recycle_010, TestSize.Level0)
{
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    std::vector<std::string> srcVec = {MEDIA_ROOT + "H264_MP3.mp4"};
    for (uint32_t i = 0; i < srcVec.size(); i++) {
        if (srcVec[i] == MEDIA_ROOT + "H264_MP3.mp4") {
            system("param set sys.media.player.resource.type NetWork");
        }
        ASSERT_EQ(MSERR_OK, player_->SetSource(srcVec[i]));
        EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
        EXPECT_EQ(MSERR_OK, player_->Prepare());
        EXPECT_EQ(MSERR_OK, player_->Play());
        EXPECT_EQ(MSERR_OK, player_->Stop());
        char str[100]; // 100: str len
        sprintf_s(str, 100, "hidumper -s 1909 -a \"-d %d %d %d\"", getpid(), getuid(), 4);
        system(str);
        system("hidumper -s 1909 -a \"-t 3\"");
        EXPECT_EQ(MSERR_OK, player_->Reset());
        system("param set sys.media.player.resource.type Local");
    }
}

/**
 * @tc.name  : Test Player Mem Recycle
 * @tc.number: Player_Mem_Recycle_011
 * @tc.desc  : Test Player Mem Recycle
 */
HWTEST_F(PlayerUnitTest, Player_Mem_Recycle_011, TestSize.Level0)
{
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    std::vector<std::string> srcVec = {MEDIA_ROOT + "H264_MP3.mp4"};
    for (uint32_t i = 0; i < srcVec.size(); i++) {
        if (srcVec[i] == MEDIA_ROOT + "H264_MP3.mp4") {
            system("param set sys.media.player.resource.type NetWork");
        }
        ASSERT_EQ(MSERR_OK, player_->SetSource(srcVec[i]));
        EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
        EXPECT_EQ(MSERR_OK, player_->Prepare());
        EXPECT_EQ(MSERR_OK, player_->Play());
        EXPECT_EQ(MSERR_OK, player_->Pause());
        char str[100]; // 100: str len
        sprintf_s(str, 100, "hidumper -s 1909 -a \"-d %d %d %d\"", getpid(), getuid(), 4);
        system(str);
        system("hidumper -s 1909 -a \"-t 2\"");
        EXPECT_EQ(MSERR_OK, player_->Reset());
        system("param set sys.media.player.resource.type Local");
    }
}

/**
 * @tc.name  : Test Player Mem Recycle
 * @tc.number: Player_Mem_Recycle_012
 * @tc.desc  : Test Player Mem Recycle
 */
HWTEST_F(PlayerUnitTest, Player_Mem_Recycle_012, TestSize.Level0)
{
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    std::vector<std::string> srcVec = {MEDIA_ROOT + "H264_MP3.mp4"};
    for (uint32_t i = 0; i < srcVec.size(); i++) {
        if (srcVec[i] == MEDIA_ROOT + "H264_MP3.mp4") {
            system("param set sys.media.player.resource.type NetWork");
        }
        ASSERT_EQ(MSERR_OK, player_->SetSource(srcVec[i]));
        EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
        EXPECT_EQ(MSERR_OK, player_->Prepare());
        EXPECT_EQ(MSERR_OK, player_->Play());
        EXPECT_EQ(MSERR_OK, player_->Pause());
        char str[100]; // 100: str len
        sprintf_s(str, 100, "hidumper -s 1909 -a \"-d %d %d %d\"", getpid(), getuid(), 4);
        system(str);
        system("hidumper -s 1909 -a \"-t 4\"");
        EXPECT_EQ(MSERR_OK, player_->Reset());
        system("param set sys.media.player.resource.type Local");
    }
}

/**
 * @tc.name  : Test Player Mem Recycle
 * @tc.number: Player_Mem_Recycle_013
 * @tc.desc  : Test Player Mem Recycle
 */
HWTEST_F(PlayerUnitTest, Player_Mem_Recycle_013, TestSize.Level0)
{
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    std::vector<std::string> srcVec = {MEDIA_ROOT + "H264_MP3.mp4"};
    for (uint32_t i = 0; i < srcVec.size(); i++) {
        if (srcVec[i] == MEDIA_ROOT + "H264_MP3.mp4") {
            system("param set sys.media.player.resource.type NetWork");
        }
        ASSERT_EQ(MSERR_OK, player_->SetSource(srcVec[i]));
        EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
        EXPECT_EQ(MSERR_OK, player_->Prepare());
        EXPECT_EQ(MSERR_OK, player_->Play());
        EXPECT_EQ(MSERR_OK, player_->Pause());
        char str[100]; // 100: str len
        sprintf_s(str, 100, "hidumper -s 1909 -a \"-d %d %d %d\"", getpid(), getuid(), 4);
        system(str);
        system("hidumper -s 1909 -a \"-f 3\"");
        EXPECT_EQ(MSERR_OK, player_->Reset());
        system("param set sys.media.player.resource.type Local");
        system("killall memmgrservice");
        sleep(1);
    }
}

/**
 * @tc.name  : Test Player Mem Recycle
 * @tc.number: Player_Mem_Recycle_014
 * @tc.desc  : Test Player Mem Recycle
 */
HWTEST_F(PlayerUnitTest, Player_Mem_Recycle_014, TestSize.Level0)
{
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    std::vector<std::string> srcVector = {MEDIA_ROOT + "H264_AAC.mp4"};
    for (uint32_t i = 0; i < srcVector.size(); i++) {
        if (srcVector[i] == MEDIA_ROOT + "H264_AAC.mp4") {
            system("param set sys.media.player.resource.type NetWork");
        }
        ASSERT_EQ(MSERR_OK, player_->SetSource(srcVector[i]));
        EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
        EXPECT_EQ(MSERR_OK, player_->Prepare());
        char str[100]; // 100: str len
        sprintf_s(str, 100, "hidumper -s 1909 -a \"-d %d %d %d\"", getpid(), getuid(), 4);
        system(str);
        sprintf_s(str, 100, "hidumper -s 1909 -a \"-d %d %d %d\"", getpid(), getuid(), 2);
        system(str);
        sleep(130);
        EXPECT_EQ(MSERR_OK, player_->Play());
        EXPECT_EQ(MSERR_OK, player_->Reset());
        system("param set sys.media.player.resource.type Local");
    }
}

/**
 * @tc.name  : Test Player Mem Recycle
 * @tc.number: Player_Mem_Recycle_015
 * @tc.desc  : Test Player Mem Recycle
 */
HWTEST_F(PlayerUnitTest, Player_Mem_Recycle_015, TestSize.Level0)
{
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    std::vector<std::string> srcVec = {MEDIA_ROOT + "H264_AAC.mp4"};
    for (uint32_t i = 0; i < srcVec.size(); i++) {
        if (srcVec[i] == MEDIA_ROOT + "H264_AAC.mp4") {
            system("param set sys.media.player.resource.type NetWork");
        }
        ASSERT_EQ(MSERR_OK, player_->SetSource(srcVec[i]));
        EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
        EXPECT_EQ(MSERR_OK, player_->Prepare());
        char str[100]; // 100: str len
        sprintf_s(str, 100, "hidumper -s 1909 -a \"-d %d %d %d\"", getpid(), getuid(), 4);
        system(str);
        system("hidumper -s 3002 -a \"player\"");
        sleep(70);
        system("hidumper -s 3002 -a \"player\"");
        EXPECT_EQ(MSERR_OK, player_->Play());
        EXPECT_EQ(MSERR_OK, player_->Stop());
        EXPECT_EQ(MSERR_OK, player_->Reset());
        system("param set sys.media.player.resource.type Local");
    }
}

/**
 * @tc.name  : Test Player Mem Recycle
 * @tc.number: Player_Mem_Recycle_016
 * @tc.desc  : Test Player Mem Recycle
 */
HWTEST_F(PlayerUnitTest, Player_Mem_Recycle_016, TestSize.Level0)
{
    int32_t duration = 0;
    std::vector<Format> videoTrack;
    std::vector<Format> audioTrack;
    PlaybackRateMode mode;
    int32_t index = 0;
    EXPECT_EQ(MSERR_OK, player_->SetVolume(0.9, 0.9));
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1, 0, 0));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->SetLooping(true));
    EXPECT_EQ(true, player_->IsLooping());
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    char str[100]; // 100: str len
    sprintf_s(str, 100, "hidumper -s 1909 -a \"-d %d %d %d\"", getpid(), getuid(), 4);
    system(str);
    system("hidumper -s 1909 -a \"-t 4\"");
    EXPECT_EQ(720, player_->GetVideoWidth());
    EXPECT_EQ(480, player_->GetVideoHeight());
    EXPECT_EQ(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
    EXPECT_EQ(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
    EXPECT_EQ(MSERR_OK, player_->GetDuration(duration));
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_EQ(SPEED_FORWARD_1_00_X, mode);
    EXPECT_EQ(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_0_50_X));
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_EQ(SPEED_FORWARD_0_50_X, mode);
    EXPECT_EQ(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X));
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_EQ(SPEED_FORWARD_2_00_X, mode);

    EXPECT_EQ(false, player_->IsPlaying());
    EXPECT_EQ(true, player_->IsLooping());

    EXPECT_EQ(MSERR_OK, player_->GetCurrentTrack(MediaType::MEDIA_TYPE_AUD, index));
    EXPECT_EQ(MSERR_OK, player_->GetCurrentTrack(MediaType::MEDIA_TYPE_VID, index));
    EXPECT_EQ(MSERR_OK, player_->GetCurrentTrack(MediaType::MEDIA_TYPE_SUBTITLE, index));
    EXPECT_NE(MSERR_OK, player_->GetCurrentTrack(100, index));

    EXPECT_EQ(MSERR_OK, player_->GetDuration(duration));
    EXPECT_EQ(MSERR_OK, player_->Seek(duration, PlayerSeekMode::SEEK_PREVIOUS_SYNC));
    EXPECT_EQ(MSERR_OK, player_->Reset());
    EXPECT_EQ(MSERR_OK, player_->Release());
}

/**
 * @tc.name  : Test SetEffect API
 * @tc.number: Player_SetEffect_001
 * @tc.desc  : Test Player SetEffect state machine
 */
HWTEST_F(PlayerUnitTest, Player_SetEffect_001, TestSize.Level0)
{
    Format format;
    const float FLOAT_VALUE = 1.0;
    const double DOUBLE_VALUE = 2.5;
    const std::string STRING_VALUE = "player_test";
    const int32_t INT_VALUE = 1;
    (void)format.PutIntValue(PlayerKeys::AUDIO_EFFECT_MODE, OHOS::AudioStandard::AudioEffectMode::EFFECT_NONE);
    EXPECT_NE(MSERR_OK, player_->SetParameter(format));
    (void)format.PutFloatValue(PlayerKeys::AUDIO_EFFECT_MODE, FLOAT_VALUE);
    EXPECT_NE(MSERR_OK, player_->SetParameter(format));
    (void)format.PutDoubleValue(PlayerKeys::AUDIO_EFFECT_MODE, DOUBLE_VALUE);
    EXPECT_NE(MSERR_OK, player_->SetParameter(format));
    (void)format.PutStringValue(PlayerKeys::AUDIO_EFFECT_MODE, STRING_VALUE);
    EXPECT_NE(MSERR_OK, player_->SetParameter(format));
    (void)format.PutIntValue(PlayerKeys::AUDIO_EFFECT_MODE, INT_VALUE);
    EXPECT_NE(MSERR_OK, player_->SetParameter(format));

    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "01.mp3"));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));

    EXPECT_NE(MSERR_OK, player_->SetParameter(format));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->SetParameter(format));
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(MSERR_OK, player_->SetParameter(format));
    EXPECT_EQ(MSERR_OK, player_->Pause());
    EXPECT_EQ(MSERR_OK, player_->SetParameter(format));
    EXPECT_EQ(MSERR_OK, player_->Stop());
    EXPECT_NE(MSERR_OK, player_->SetParameter(format));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->SetParameter(format));
    EXPECT_EQ(MSERR_OK, player_->Reset());
    EXPECT_NE(MSERR_OK, player_->SetParameter(format));
    EXPECT_EQ(MSERR_OK, player_->Release());
    EXPECT_NE(MSERR_OK, player_->SetParameter(format));
}

/**
 * @tc.name  : Test SetEffect API
 * @tc.number: Player_SetEffect_002
 * @tc.desc  : Test Player SetEffect param
 */
HWTEST_F(PlayerUnitTest, Player_SetEffect_002, TestSize.Level0)
{
    Format format;
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "01.mp3"));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());

    (void)format.PutIntValue(PlayerKeys::AUDIO_EFFECT_MODE, OHOS::AudioStandard::AudioEffectMode::EFFECT_DEFAULT);
    EXPECT_EQ(MSERR_OK, player_->SetParameter(format));
    (void)format.PutIntValue(PlayerKeys::AUDIO_EFFECT_MODE, OHOS::AudioStandard::AudioEffectMode::EFFECT_NONE);
    EXPECT_EQ(MSERR_OK, player_->SetParameter(format));
    (void)format.PutIntValue(PlayerKeys::AUDIO_EFFECT_MODE, 100); // 100 is an invalid parameter.
    EXPECT_NE(MSERR_OK, player_->SetParameter(format));
    (void)format.PutIntValue(PlayerKeys::AUDIO_EFFECT_MODE, -1); // -1 is an invalid parameter.
    EXPECT_NE(MSERR_OK, player_->SetParameter(format));

    EXPECT_EQ(MSERR_OK, player_->Release());
}

/**
 * @tc.name  : Test media error
 * @tc.number: Player_Media_Error
 * @tc.desc  : Test Player Media Error
 */
HWTEST_F(PlayerUnitTest, Player_Media_Error, TestSize.Level0)
{
    for (int32_t code = MSERR_OK; code <= MSERR_EXTEND_START + 1; code++) {
        MSErrorToString(static_cast<MediaServiceErrCode>(code));
        MSErrorToExtErrorString(static_cast<MediaServiceErrCode>(code));
        MSErrorToExtError(static_cast<MediaServiceErrCode>(code));
        MSErrorToExtErrorAPI9String(static_cast<MediaServiceErrCode>(code), "test1", "test2");
        MSErrorToExtErrorAPI9(static_cast<MediaServiceErrCode>(code));
    }

    for (int32_t code = MSERR_EXT_OK; code <= MSERR_EXT_EXTEND_START + 1; code++) {
        MSExtErrorToString(static_cast<MediaServiceExtErrCode>(code));
    }

    for (int32_t code = MSERR_EXT_API9_OK;
        code <= MSERR_EXT_API9_UNSUPPORT_FORMAT + 1; code++) {
        MSExtErrorAPI9ToString(static_cast<MediaServiceExtErrCodeAPI9>(code), "test1", "test2");
        MSExtAVErrorToString(static_cast<MediaServiceExtErrCodeAPI9>(code));
    }
}

/**
 * @tc.name  : Test ChangeSurface
 * @tc.number: Player_ChangeSurface_001
 * @tc.desc  : Test video player change surface in idle state
 */
HWTEST_F(PlayerUnitTest, Player_ChangeSurface_001, TestSize.Level0)
{
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_NE(MSERR_OK, player_->SetVideoSurface(videoSurface));
}

/**
 * @tc.name  : Test ChangeSurface
 * @tc.number: Player_ChangeSurface_002
 * @tc.desc  : Test video player change surface in released state
 */
HWTEST_F(PlayerUnitTest, Player_ChangeSurface_002, TestSize.Level0)
{
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    player_->Release();
    EXPECT_NE(MSERR_OK, player_->SetVideoSurface(videoSurface));
}

/**
 * @tc.name  : Test ChangeSurface
 * @tc.number: Player_ChangeSurface_003
 * @tc.desc  : Test video player change surface in error state
 */
HWTEST_F(PlayerUnitTest, Player_ChangeSurface_003, TestSize.Level0)
{
    int32_t ret = player_->SetSource(MEDIA_ROOT + "error.mp4");
    EXPECT_EQ(MSERR_OK, ret);
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_NE(MSERR_OK, player_->PrepareAsync());
    sleep(PLAYING_TIME_2_SEC);
    sptr<Surface> nextVideoSurface = player_->GetVideoSurfaceNext();
    ASSERT_NE(nullptr, nextVideoSurface);
    EXPECT_NE(MSERR_OK, player_->SetVideoSurface(nextVideoSurface));
}

/**
 * @tc.name  : Test SetPlaybackSpeed API
 * @tc.number: Player_SetPlaybackSpeed_003
 * @tc.desc  : Test Player SetPlaybackSpeed SPEED_FORWARD_0_50_X
 */
HWTEST_F(PlayerUnitTest, Player_SetPlaybackSpeed_003, TestSize.Level0)
{
    PlaybackRateMode mode;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_0_50_X));
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_EQ(SPEED_FORWARD_0_50_X, mode);
}

/**
 * @tc.name  : Test SetPlaybackSpeed API
 * @tc.number: Player_SetPlaybackSpeed_004
 * @tc.desc  : Test Player SetPlaybackSpeed SPEED_FORWARD_1_50_X
 */
HWTEST_F(PlayerUnitTest, Player_SetPlaybackSpeed_004, TestSize.Level0)
{
    PlaybackRateMode mode;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_1_50_X));
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_EQ(SPEED_FORWARD_1_50_X, mode);
}

/**
 * @tc.name  : Test SetPlaybackSpeed API
 * @tc.number: Player_SetPlaybackSpeed_005
 * @tc.desc  : Test Player SetPlaybackSpeed SPEED_FORWARD_1_50_X
 */
HWTEST_F(PlayerUnitTest, Player_SetPlaybackSpeed_005, TestSize.Level0)
{
    PlaybackRateMode mode;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    int32_t speed = -1;
    EXPECT_EQ(MSERR_OK, player_->SetPlaybackSpeed(static_cast<OHOS::Media::PlaybackRateMode>(speed)));
    EXPECT_EQ(MSERR_OK, player_->GetPlaybackSpeed(mode));
    EXPECT_EQ(-1, mode);
}

/**
 * @tc.name  : Test SetSurface API
 * @tc.number: Player_SetSurface_001
 * @tc.desc  : Test Player SetSurface->SetSurface
 */
HWTEST_F(PlayerUnitTest, Player_SetSurface_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    int32_t ret = player_->PrepareAsync();
    if (ret == MSERR_OK) {
        PlayFunTest(LOCAL_PLAY);
    }
}

/**
 * @tc.name  : Test AddSubSource
 * @tc.number: Player_AddSubSource_001
 * @tc.desc  : Test Player AddSubSource state machine
 */
HWTEST_F(PlayerUnitTest, Player_AddSubSource_001, TestSize.Level0)
{
    ASSERT_NE(MSERR_OK, player_->AddSubSource(SUBTITLE_SRT_FIELE, 0, 0));
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "H264_AAC.mp4", 0, 0));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_NE(MSERR_OK, player_->AddSubSource(SUBTITLE_SRT_FIELE));    // Illegal state machine
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_NE(MSERR_OK, player_->AddSubSource(SUBTITLE_SRT_FIELE, 0, 0));
    EXPECT_NE(MSERR_OK, player_->Play());
    EXPECT_NE(SUBTITLE_0_SEC, player_->GetSubtitleText(""));
    EXPECT_NE(MSERR_OK, player_->AddSubSource(SUBTITLE_SRT_FIELE, 0, 0));
    EXPECT_NE(MSERR_OK, player_->Pause());
    EXPECT_NE(MSERR_OK, player_->AddSubSource(SUBTITLE_SRT_FIELE, 0, 0));
    EXPECT_EQ(MSERR_OK, player_->Stop());
    EXPECT_NE(MSERR_OK, player_->AddSubSource(SUBTITLE_SRT_FIELE, 0, 0));    // Illegal state machine
    EXPECT_EQ(MSERR_OK, player_->Reset());
    EXPECT_NE(MSERR_OK, player_->AddSubSource(SUBTITLE_SRT_FIELE, 0, 0));    // Illegal state machine
    EXPECT_EQ(MSERR_OK, player_->Release());
    EXPECT_NE(MSERR_OK, player_->AddSubSource(SUBTITLE_SRT_FIELE, 0, 0));    // Illegal state machine
}

/**
 * @tc.name  : Test AddSubSource
 * @tc.number: Player_AddSubSource_002
 * @tc.desc  : Test Player AddSubSource behavior
 */
HWTEST_F(PlayerUnitTest, Player_AddSubSource_002, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "H264_AAC.mp4", 0, 0));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_NE(MSERR_OK, player_->AddSubSource(SUBTITLE_SRT_FIELE, 0, 0));
    EXPECT_NE(MSERR_OK, player_->Play());
    EXPECT_NE(MSERR_OK, player_->Seek(SEEK_TIME_5_SEC, SEEK_CLOSEST));
    EXPECT_NE(SUBTITLE_5_SEC, player_->GetSubtitleText(""));
    EXPECT_NE(MSERR_OK, player_->Pause());
    EXPECT_NE(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_CLOSEST));
    EXPECT_NE(SUBTITLE_2_SEC, player_->GetSubtitleText(""));
    EXPECT_NE(MSERR_OK, player_->Seek(SEEK_TIME_5_SEC, SEEK_NEXT_SYNC));
    EXPECT_NE(SUBTITLE_8_SEC, player_->GetSubtitleText(""));
    EXPECT_NE(MSERR_OK, player_->Seek(SEEK_TIME_5_SEC, SEEK_PREVIOUS_SYNC));
    EXPECT_NE(SUBTITLE_4_SEC, player_->GetSubtitleText(""));
    int duration = 0;
    EXPECT_EQ(MSERR_OK, player_->GetDuration(duration));
    EXPECT_NE(MSERR_OK, player_->Play());
    EXPECT_NE(MSERR_OK, player_->Seek(duration, SEEK_CLOSEST));
    EXPECT_NE(SUBTITLE_10_SEC, player_->GetSubtitleText(""));
}

/**
 * @tc.name  : Test AddSubSource
 * @tc.number: Player_AddSubSource_003
 * @tc.desc  : Test Player AddSubSource behavior
 */
HWTEST_F(PlayerUnitTest, Player_AddSubSource_003, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "H264_AAC.mp4", 0, 0));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    sleep(PLAYING_TIME_2_SEC);
    EXPECT_NE(MSERR_OK, player_->AddSubSource(SUBTITLE_SRT_FIELE, 0, 0));
    EXPECT_NE(SUBTITLE_2_SEC, player_->GetSubtitleText(""));
    sleep(PLAYING_TIME_1_SEC);
    EXPECT_NE(SUBTITLE_3_SEC, player_->GetSubtitleText(""));
}

/**
 * @tc.name  : Test AddSubSource
 * @tc.number: Player_AddSubSource_004
 * @tc.desc  : Test Player AddSubSource behavior
 */
HWTEST_F(PlayerUnitTest, Player_AddSubSource_004, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "H264_AAC.mp4", 0, 0));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X));
    EXPECT_EQ(MSERR_OK, player_->Play());
    sleep(PLAYING_TIME_2_SEC);
    EXPECT_NE(MSERR_OK, player_->AddSubSource(SUBTITLE_SRT_FIELE, 0, 0));
    EXPECT_NE(SUBTITLE_4_SEC, player_->GetSubtitleText(""));
    sleep(PLAYING_TIME_1_SEC);
    std::string text = player_->GetSubtitleText("");
    EXPECT_NE((SUBTITLE_6_SEC == text), true);
}

/**
 * @tc.name  : Test AddSubSource
 * @tc.number: Player_AddSubSource_005
 * @tc.desc  : Test Player AddSubSource behavior
 */
HWTEST_F(PlayerUnitTest, Player_AddSubSource_005, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "H264_AAC.mp4", 0, 0));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_EQ(MSERR_OK, player_->Play());
    sleep(PLAYING_TIME_2_SEC);
    EXPECT_NE(MSERR_OK, player_->AddSubSource(SUBTITLE_SRT_FIELE, 0, 0));
    EXPECT_NE(SUBTITLE_2_SEC, player_->GetSubtitleText(""));
    EXPECT_EQ(MSERR_OK, player_->SetPlaybackSpeed(SPEED_FORWARD_2_00_X));
    sleep(PLAYING_TIME_1_SEC);
    std::string text = player_->GetSubtitleText("");
    EXPECT_NE((SUBTITLE_3_SEC == text || SUBTITLE_4_SEC == text), true);
    EXPECT_EQ(MSERR_OK, player_->Reset());
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "H264_AAC.mp4", 0, 0));
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->Prepare());
    EXPECT_NE(MSERR_OK, player_->AddSubSource(SUBTITLE_SRT_FIELE, 0, 0));
    EXPECT_NE(MSERR_OK, player_->Play());
    EXPECT_NE(MSERR_OK, player_->Pause());
    EXPECT_NE(MSERR_OK, player_->Seek(SEEK_TIME_2_SEC, SEEK_CLOSEST));
    EXPECT_NE(SUBTITLE_2_SEC, player_->GetSubtitleText(""));
}

/**
 * @tc.name  : Test PlayerServer Stop
 * @tc.number: PlayerServer_Stop_001
 * @tc.desc  : Test PlayerServer Stop on different status
 */
HWTEST_F(PlayerUnitTest, PlayerServer_Stop_001, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "H264_AAC.mp4", 0, 0));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Stop());
}

/**
 * @tc.name  : Test PlayerServer Stop
 * @tc.number: PlayerServer_Stop_002
 * @tc.desc  : Test PlayerServer Stop on different status
 */
HWTEST_F(PlayerUnitTest, PlayerServer_Stop_002, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "H264_AAC.mp4", 0, 0));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Stop());
}

/**
 * @tc.name  : Test PlayerServer Stop
 * @tc.number: PlayerServer_Stop_003
 * @tc.desc  : Test PlayerServer Stop on different status
 */
HWTEST_F(PlayerUnitTest, PlayerServer_Stop_003, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "H264_AAC.mp4", 0, 0));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    int32_t duration = 0;
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_EQ(MSERR_OK, player_->GetDuration(duration));
    EXPECT_EQ(MSERR_OK, player_->Seek(duration, SEEK_NEXT_SYNC));
    EXPECT_TRUE(player_->IsPlaying());
    sleep(PLAYING_TIME_2_SEC);
    EXPECT_EQ(MSERR_OK, player_->Stop());
}

/**
 * @tc.name  : Test PlayerServer Stop
 * @tc.number: PlayerServer_Stop_004
 * @tc.desc  : Test PlayerServer Stop on different status
 */
HWTEST_F(PlayerUnitTest, PlayerServer_Stop_004, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "H264_AAC.mp4", 0, 0));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->Play());
    EXPECT_TRUE(player_->IsPlaying());
    EXPECT_EQ(MSERR_OK, player_->Pause());
    EXPECT_EQ(MSERR_OK, player_->Stop());
}

/**
 * @tc.name  : Test PlayerServer Stop
 * @tc.number: PlayerServer_Stop_005
 * @tc.desc  : Test PlayerServer Stop on different status
 */
HWTEST_F(PlayerUnitTest, PlayerServer_Stop_005, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, player_->SetSource(MEDIA_ROOT + "H264_AAC.mp4", 0, 0));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_NE(MSERR_OK, player_->Stop());
}

/**
 * @tc.name  : Test SetParameter Stop
 * @tc.number: Player_SetParameter_001
 * @tc.desc  : Test Player SetParameter
 */
HWTEST_F(PlayerUnitTest, Player_SetParameter_001, TestSize.Level0)
{
    std::vector<Format> videoTrack;
    std::vector<Format> audioTrack;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->SetLooping(true));
    Format format;
    EXPECT_EQ(MSERR_OK, player_->SetParameter(format));
    EXPECT_EQ(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
    EXPECT_EQ(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
}

/**
 * @tc.name  : Test SetParameter Stop
 * @tc.number: Player_SetParameter_002
 * @tc.desc  : Test Player SetParameter
 */
HWTEST_F(PlayerUnitTest, Player_SetParameter_002, TestSize.Level0)
{
    std::vector<Format> videoTrack;
    std::vector<Format> audioTrack;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->SetLooping(true));
    Format format;
    int32_t contentType = 1;
    int32_t scaleType = 1;
    int32_t streamUsage = 1;
    int32_t rendererFlags = 1;
    int32_t audioInterruptMode = 1;
    format.PutIntValue(PlayerKeys::VIDEO_SCALE_TYPE, scaleType);
    format.PutIntValue(PlayerKeys::CONTENT_TYPE, contentType);
    format.PutIntValue(PlayerKeys::STREAM_USAGE, streamUsage);
    format.PutIntValue(PlayerKeys::RENDERER_FLAG, rendererFlags);
    format.PutIntValue(PlayerKeys::AUDIO_INTERRUPT_MODE, audioInterruptMode);
    EXPECT_EQ(MSERR_OK, player_->SetParameter(format));
    EXPECT_EQ(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
    EXPECT_EQ(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
}

/**
 * @tc.name  : Test SetParameter Stop
 * @tc.number: Player_SetParameter_003
 * @tc.desc  : Test Player SetParameter
 */
HWTEST_F(PlayerUnitTest, Player_SetParameter_003, TestSize.Level0)
{
    Format formatNoScaleType;
    Format formatNoContentType;
    Format formatNoStreamUsage;
    Format formatNoStreamUsageAndContentType;
    Format formatNoInterruptMode;
    std::vector<Format> videoTrack;
    std::vector<Format> audioTrack;
    int32_t contentType = 1;
    int32_t scaleType = 1;
    int32_t streamUsage = 1;
    int32_t rendererFlags = 1;
    int32_t audioInterruptMode = 1;
    ASSERT_EQ(MSERR_OK, player_->SetSource(VIDEO_FILE1));
    sptr<Surface> videoSurface = player_->GetVideoSurface();
    ASSERT_NE(nullptr, videoSurface);
    EXPECT_EQ(MSERR_OK, player_->SetVideoSurface(videoSurface));
    EXPECT_EQ(MSERR_OK, player_->PrepareAsync());
    EXPECT_EQ(MSERR_OK, player_->SetLooping(true));

    formatNoScaleType.PutIntValue(PlayerKeys::CONTENT_TYPE, contentType);
    formatNoScaleType.PutIntValue(PlayerKeys::STREAM_USAGE, streamUsage);
    formatNoScaleType.PutIntValue(PlayerKeys::RENDERER_FLAG, rendererFlags);
    formatNoScaleType.PutIntValue(PlayerKeys::AUDIO_INTERRUPT_MODE, audioInterruptMode);
    EXPECT_EQ(MSERR_OK, player_->SetParameter(formatNoScaleType));

    formatNoContentType.PutIntValue(PlayerKeys::VIDEO_SCALE_TYPE, scaleType);
    formatNoContentType.PutIntValue(PlayerKeys::STREAM_USAGE, streamUsage);
    formatNoContentType.PutIntValue(PlayerKeys::RENDERER_FLAG, rendererFlags);
    formatNoContentType.PutIntValue(PlayerKeys::AUDIO_INTERRUPT_MODE, audioInterruptMode);
    EXPECT_EQ(MSERR_OK, player_->SetParameter(formatNoContentType));

    formatNoStreamUsage.PutIntValue(PlayerKeys::VIDEO_SCALE_TYPE, scaleType);
    formatNoStreamUsage.PutIntValue(PlayerKeys::CONTENT_TYPE, contentType);
    formatNoStreamUsage.PutIntValue(PlayerKeys::RENDERER_FLAG, rendererFlags);
    formatNoStreamUsage.PutIntValue(PlayerKeys::AUDIO_INTERRUPT_MODE, audioInterruptMode);
    EXPECT_EQ(MSERR_OK, player_->SetParameter(formatNoStreamUsage));

    formatNoStreamUsageAndContentType.PutIntValue(PlayerKeys::VIDEO_SCALE_TYPE, scaleType);
    formatNoStreamUsageAndContentType.PutIntValue(PlayerKeys::RENDERER_FLAG, rendererFlags);
    formatNoStreamUsageAndContentType.PutIntValue(PlayerKeys::AUDIO_INTERRUPT_MODE, audioInterruptMode);
    EXPECT_EQ(MSERR_OK, player_->SetParameter(formatNoStreamUsageAndContentType));

    formatNoInterruptMode.PutIntValue(PlayerKeys::VIDEO_SCALE_TYPE, scaleType);
    formatNoInterruptMode.PutIntValue(PlayerKeys::RENDERER_FLAG, rendererFlags);
    formatNoInterruptMode.PutIntValue(PlayerKeys::CONTENT_TYPE, contentType);
    formatNoInterruptMode.PutIntValue(PlayerKeys::STREAM_USAGE, streamUsage);
    EXPECT_EQ(MSERR_OK, player_->SetParameter(formatNoInterruptMode));

    EXPECT_EQ(MSERR_OK, player_->GetVideoTrackInfo(videoTrack));
    EXPECT_EQ(MSERR_OK, player_->GetAudioTrackInfo(audioTrack));
}

} // namespace Media
} // namespace OHOS
