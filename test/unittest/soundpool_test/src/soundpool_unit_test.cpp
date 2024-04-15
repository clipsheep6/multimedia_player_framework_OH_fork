/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "soundpool_unit_test.h"
#include "media_errors.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace std;

static const std::string g_fileName[6] = {
    {"/data/test/test_06.ogg"},
    {"/data/test/test_02.mp3"},
    {"/data/test/test_01.mp3"},
    {"/data/test/test_05.ogg"},
    {"/data/test/test_03.mp3"},
    {"/data/test/test_04.mp3"},
};

namespace OHOS {
namespace Media {
void SoundPoolUnitTest::SetUpTestCase(void) {}

void SoundPoolUnitTest::TearDownTestCase(void) {}

void SoundPoolUnitTest::SetUp(void)
{
    soundPool_ = std::make_shared<SoundPoolMock>();
    ASSERT_NE(nullptr, soundPool_);
}

void SoundPoolUnitTest::TearDown(void)
{
    for (auto soundId : soundIDs_) {
        if (soundId != 0) {
            soundId = 0;
        }
    }
    for (auto streamId : streamIDs_) {
        if (streamId != 0) {
            streamId = 0;
        }
    }
    for (auto fd : fds_) {
        if (fd != 0) {
            fd = 0;
        }
    }
    if (loadNum_ != 0 || playNum_ != 0) {
        loadNum_ = 0;
        playNum_ = 0;
    }

    if (soundPool_ != nullptr) {
        int32_t ret = soundPool_->Release();
        soundPool_ = nullptr;
        EXPECT_EQ(MSERR_OK, ret);
    }
    sleep(waitTime1);
}

void SoundPoolUnitTest::create(int maxStreams)
{
    AudioStandard::AudioRendererInfo audioRenderInfo;
    audioRenderInfo.contentType = CONTENT_TYPE_MUSIC;
    audioRenderInfo.streamUsage = STREAM_USAGE_MUSIC;
    audioRenderInfo.rendererFlags = 0;
    if (soundPool_ == nullptr) {
        cout << "create soundpool failed" << endl;
    } else {
        EXPECT_TRUE(soundPool_->CreateSoundPool(maxStreams, audioRenderInfo));
    }
}

void SoundPoolUnitTest::loadUrl(std::string fileName, int32_t loadNum)
{
    fds_[loadNum] = open(fileName.c_str(), O_RDWR);
    if (fds_[loadNum] > 0) {
        std::string url = "fd://" + std::to_string(fds_[loadNum]);
        soundIDs_[loadNum] = soundPool_->Load(url);
    } else {
        cout << "Url open failed, g_fileName " << fileName.c_str() << ", fd: " << fds_[loadNum] << endl;
    }
    EXPECT_GT(soundIDs_[loadNum], 0);
}

void SoundPoolUnitTest::loadFd(std::string fileName, int32_t loadNum)
{
    fds_[loadNum] = open(fileName.c_str(), O_RDONLY);
    if (fds_[loadNum] < 0) {
        cout << "Fd open failed, g_fileName " << fileName.c_str() << ", Fd: " << fds_[loadNum] << endl;
    }
    size_t filesize = soundPool_->GetFileSize(fileName);
    EXPECT_NE(filesize, 0);
    soundIDs_[loadNum] = soundPool_->Load(fds_[loadNum], 0, filesize);
    EXPECT_GT(soundIDs_[loadNum], 0);
}

 /**
 * @tc.name: soundpool_function_036
 * @tc.desc: function test Priority
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_036, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[1], loadNum_);
    loadNum_++;
    loadUrl(g_fileName[1], loadNum_);
    loadNum_++;
    loadUrl(g_fileName[1], loadNum_);
    loadNum_++;
    loadUrl(g_fileName[0], loadNum_);
    sleep(waitTime3);
    struct PlayParams playParameters;
    playParameters.priority = 3;
    if (soundIDs_[0] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[0], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime2);
    }
    playNum_++;
    if (soundIDs_[1] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[1], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime2);
    }
    playNum_++;
    if (soundIDs_[2] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[2], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime2);
    }
    playNum_++;
    playParameters.priority = 5;
    if (soundIDs_[3] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[3], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime2);
    }
    sleep(15);
    cb->ResetHavePlayedSoundNum();
}

 /**
 * @tc.name: soundpool_function_037
 * @tc.desc: function test Priority
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_037, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[1], loadNum_);
    loadNum_++;
    loadUrl(g_fileName[1], loadNum_);
    loadNum_++;
    loadUrl(g_fileName[1], loadNum_);
    loadNum_++;
    loadUrl(g_fileName[0], loadNum_);
    sleep(waitTime3);
    struct PlayParams playParameters;
    playParameters.priority = 3;
    if (soundIDs_[0] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[0], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime2);
    }
    playNum_++;
    if (soundIDs_[1] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[1], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime2);
    }
    playNum_++;
    if (soundIDs_[2] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[2], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime2);
    }
    playNum_++;
    playParameters.priority = 1;
    if (soundIDs_[3] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[3], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime2);
    }
    sleep(15);
    cb->ResetHavePlayedSoundNum();
}

} // namespace Media
} // namespace OHOS