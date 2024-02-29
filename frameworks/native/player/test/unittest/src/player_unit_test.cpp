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
} // namespace Media
} // namespace OHOS
