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
 * @tc.name: soundpool_function_001
 * @tc.desc: function test Load Url one more time
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_001, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    loadNum_++;
    loadUrl(g_fileName[loadNum_], loadNum_);
    loadNum_++;
    loadUrl(g_fileName[1], loadNum_);
    loadNum_++;
    loadUrl(g_fileName[1], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded url sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 4) {
            cout << "All sound loaded url break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
}

/**
 * @tc.name: soundpool_function_002
 * @tc.desc: function test Load Url with invalid path
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_002, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    // test invalid path
    std::string fileName = "/data/test/test_05.mp3";
    fds_[loadNum_] = open(fileName.c_str(), O_RDWR);
    std::string url = "fd://" + std::to_string(fds_[loadNum_]);
    soundIDs_[loadNum_] = soundPool_->Load(url);
    sleep(waitTime3);
    if (fds_[loadNum_] == -1) {
        cout << "Url open a invalid path: " << fileName.c_str() << endl;
    }
    EXPECT_EQ(soundIDs_[loadNum_], -1);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded url sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 1) {
            cout << "All sound loaded url break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        } else {
            cout << "Sound loaded url error. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            break;
        }
    }
}

/**
 * @tc.name: soundpool_function_003
 * @tc.desc: function test Load Url when no callback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_003, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    // test no callback to load
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
}

/**
 * @tc.name: soundpool_function_004
 * @tc.desc: function test Load Url after play
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_004, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 1) {
            cout << "All sound loaded url break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    if (soundIDs_[loadNum_] > 0) {
        soundPool_->Play(soundIDs_[loadNum_], playParameters);
        sleep(waitTime1);
        loadNum_++;
        loadUrl(g_fileName[loadNum_], loadNum_);
        sleep(waitTime3);
    } else {
        cout << "Get soundId failed, please try to get soundId: " << soundIDs_[loadNum_] << endl;
    }
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}

/**
 * @tc.name: soundpool_function_005
 * @tc.desc: function test Load Fd one more time
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_005, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadFd(g_fileName[loadNum_], loadNum_);
    loadNum_++;
    loadFd(g_fileName[loadNum_], loadNum_);
    loadNum_++;
    loadFd(g_fileName[1], loadNum_);
    loadNum_++;
    loadFd(g_fileName[1], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Fd sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 4) {
            cout << "All sound loaded Fd break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
}

/**
 * @tc.name: soundpool_function_006
 * @tc.desc: function test Load Fd with invalid path
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_006, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    std::string fileName = "/data/test/test_05.mp3";
    fds_[loadNum_] = open(fileName.c_str(), O_RDONLY);
    size_t filesize = soundPool_->GetFileSize(fileName);
    EXPECT_EQ(filesize, 0);
    soundIDs_[loadNum_] = soundPool_->Load(fds_[loadNum_], 0, filesize);
    sleep(waitTime3);
    if (fds_[loadNum_] == -1) {
        cout << "Fd open a invalid path: " << fileName.c_str() << endl;
    }
    EXPECT_EQ(soundIDs_[loadNum_], -1);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Fd sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 1) {
            cout << "All sound loaded Fd break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        } else {
            cout << "Sound loaded error Fd. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            break;
        }
    }
}

/**
 * @tc.name: soundpool_function_007
 * @tc.desc: function test Load Fd when no callback
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_007, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    // test no callback to load
    loadFd(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
}

/**
 * @tc.name: soundpool_function_008
 * @tc.desc: function test Load Fd after paly
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_008, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadFd(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Fd sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 1) {
            cout << "All sound loaded Fd break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        } else {
            cout << "Sound loaded Fd error. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            break;
        }
    }
    struct PlayParams playParameters;
    if (soundIDs_[loadNum_] > 0) {
        soundPool_->Play(soundIDs_[loadNum_], playParameters);
        sleep(waitTime1);
        loadNum_++;
        loadFd(g_fileName[loadNum_], loadNum_);
        sleep(waitTime3);
    } else {
        cout << "Get soundId failed, please try to get soundId: " << soundIDs_[loadNum_] << endl;
    }
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}

/**
 * @tc.name: soundpool_function_009
 * @tc.desc: function test UnLoad
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_009, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    loadNum_++;
    loadUrl(g_fileName[loadNum_], loadNum_);
    loadNum_++;
    loadFd(g_fileName[loadNum_], loadNum_);
    loadNum_++;
    loadFd(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 4) {
            cout << "All sound loaded break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    EXPECT_EQ(MSERR_OK, soundPool_->Unload(soundIDs_[0]));
    EXPECT_EQ(MSERR_OK, soundPool_->Unload(soundIDs_[1]));
    EXPECT_EQ(MSERR_OK, soundPool_->Unload(soundIDs_[2]));
    EXPECT_EQ(MSERR_OK, soundPool_->Unload(soundIDs_[3]));
}

/**
 * @tc.name: soundpool_function_010
 * @tc.desc: function test UnLoad with error path
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_010, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    std::string fileName = "/data/test/test_05.mp3";
    fds_[loadNum_] = open(fileName.c_str(), O_RDWR);
    std::string url = "fd://" + std::to_string(fds_[loadNum_]);
    soundIDs_[loadNum_] = soundPool_->Load(url);
    if (fds_[loadNum_] == -1) {
        cout << "Url open a invalid path: " << fileName.c_str() << endl;
    }
    EXPECT_EQ(soundIDs_[loadNum_], -1);
    loadNum_++;
    fds_[loadNum_] = open(fileName.c_str(), O_RDONLY);
    size_t filesize = soundPool_->GetFileSize(fileName);
    EXPECT_EQ(filesize, 0);
    soundIDs_[loadNum_] = soundPool_->Load(fds_[loadNum_], 0, filesize);
    sleep(waitTime3);
    if (fds_[loadNum_] == -1) {
        cout << "Fd open a invalid path: " << fileName.c_str() << endl;
    }
    EXPECT_EQ(soundIDs_[loadNum_], -1);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 2) {
            cout << "All sound loaded break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        } else {
            cout << "Sound loaded error. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            break;
        }
    }
    // test UnLoad a invalid-path return soundId
    int32_t unload = soundPool_->Unload(soundIDs_[0]);
    EXPECT_EQ(MSERR_NO_MEMORY, unload);
    unload = soundPool_->Unload(soundIDs_[1]);
    EXPECT_EQ(MSERR_NO_MEMORY, unload);
}

/**
 * @tc.name: soundpool_function_011
 * @tc.desc: function test UnLoad with -1/5
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_011, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    // test unload -1
    int32_t unload = soundPool_->Unload(-1);
    EXPECT_EQ(MSERR_NO_MEMORY, unload);
    // test unload 5
    unload = soundPool_->Unload(5);
    EXPECT_EQ(MSERR_NO_MEMORY, unload);
}

/**
 * @tc.name: soundpool_function_012
 * @tc.desc: function test Play with undefault playParameters
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_012, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }

    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Fd sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 1) {
            cout << "All sound loaded Fd break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    playParameters.loop = -1;
    playParameters.rate = 1;
    playParameters.leftVolume = 0.5;
    playParameters.rightVolume = 0.3;
    playParameters.priority = 1;
    playParameters.parallelPlayFlag = true;
    if (soundIDs_[loadNum_] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[loadNum_], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime3);
    }
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}

/**
 * @tc.name: soundpool_function_013
 * @tc.desc: function test Play with default playParameters
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_013, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Fd sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 1) {
            cout << "All sound loaded Fd break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    if (soundIDs_[loadNum_] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[loadNum_], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime3);
        playNum_++;
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[loadNum_], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime3);
        playNum_++;
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[loadNum_], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime3);
        playNum_++;
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[loadNum_], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime3);
    } else {
        cout << "Get soundId failed, please try to get soundId: " << soundIDs_[loadNum_] << endl;
    }
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}

/**
 * @tc.name: soundpool_function_014
 * @tc.desc: function test Play with error soundID
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_014, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    struct PlayParams playParameters;
    streamIDs_[playNum_] = soundPool_->Play(5, playParameters);
    cout << "soundId 5 play, result: " << streamIDs_[playNum_] << endl;
    EXPECT_EQ(streamIDs_[playNum_], -1);
    sleep(waitTime1);
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}

/**
 * @tc.name: soundpool_function_015
 * @tc.desc: function test Play with not load number -1
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_015, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    struct PlayParams playParameters;
    streamIDs_[playNum_] = soundPool_->Play(-1, playParameters);
    cout << "soundId -1 play, result: " << streamIDs_[playNum_] << endl;
    EXPECT_EQ(streamIDs_[playNum_], -1);
    sleep(waitTime1);
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}

/**
 * @tc.name: soundpool_function_016
 * @tc.desc: function test Play with different soundID
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_016, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    loadNum_++;
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Fd sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 2) {
            cout << "All sound loaded Fd break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    streamIDs_[playNum_] = soundPool_->Play(soundIDs_[0], playParameters);
    EXPECT_GT(streamIDs_[playNum_], 0);
    sleep(waitTime1);
    playNum_++;
    streamIDs_[playNum_] = soundPool_->Play(soundIDs_[1], playParameters);
    EXPECT_GT(streamIDs_[playNum_], 0);
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}

/**
 * @tc.name: soundpool_function_017
 * @tc.desc: function test Stop with same streamId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_017, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Fd sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 1) {
            cout << "All sound loaded Fd break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    if (soundIDs_[loadNum_] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[loadNum_], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
    }
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
    if (streamIDs_[playNum_] > 0) {
        int32_t stopResult = soundPool_->Stop(streamIDs_[playNum_]);
        EXPECT_EQ(MSERR_OK, stopResult);
        sleep(waitTime1);
        int32_t stopResult1 = soundPool_->Stop(streamIDs_[playNum_]);
        EXPECT_EQ(MSERR_OK, stopResult1);
    }
}

/**
 * @tc.name: soundpool_function_018
 * @tc.desc: function test Stop with different streamId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_018, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    loadNum_++;
    loadFd(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Fd sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 2) {
            cout << "All sound loaded Fd break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    if (soundIDs_[0] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[0], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
    }
    playNum_++;
    if (soundIDs_[1] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[1], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
    }
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
    if (streamIDs_[0] > 0) {
        EXPECT_EQ(MSERR_OK, soundPool_->Stop(streamIDs_[0]));
    }
    if (streamIDs_[1] > 0) {
        EXPECT_EQ(MSERR_OK, soundPool_->Stop(streamIDs_[1]));
    }
}

/**
 * @tc.name: soundpool_function_019
 * @tc.desc: function test not Stop all streamId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_019, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    loadNum_++;
    loadFd(g_fileName[loadNum_], loadNum_);
    loadNum_++;
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        if (cb->GetHaveLoadedSoundNum() == 3) {
            cout << "All sound loaded Fd break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    if (soundIDs_[0] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[0], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
    }
    playNum_++;
    if (soundIDs_[1] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[1], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
    }
    playNum_++;
    if (soundIDs_[2] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[2], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
    }
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
    EXPECT_EQ(MSERR_OK, soundPool_->Stop(streamIDs_[0]));
    EXPECT_EQ(MSERR_OK, soundPool_->Stop(streamIDs_[2]));
}

/**
 * @tc.name: soundpool_function_020
 * @tc.desc: function test Stop with -1
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_020, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Fd sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 1) {
            cout << "All sound loaded Fd break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    int32_t stopResult = soundPool_->Stop(-1);
    EXPECT_EQ(MSERR_INVALID_OPERATION, stopResult);
}

/**
 * @tc.name: soundpool_function_021
 * @tc.desc: function test SetLoop
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_021, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Url sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 1) {
            cout << "All sound loaded Url break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    playParameters.loop = 0;
    int32_t loop = -1;
    if (soundIDs_[loadNum_] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[loadNum_], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
        int32_t setPool = soundPool_->SetLoop(streamIDs_[playNum_], loop);
        EXPECT_EQ(MSERR_OK, setPool);
        sleep(waitTime1);
    } else {
        cout << "Get soundId failed, please try to get soundId: " << soundIDs_[loadNum_] << endl;
    }
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
    EXPECT_EQ(MSERR_OK, soundPool_->Unload(soundIDs_[loadNum_]));
}

/**
 * @tc.name: soundpool_function_022
 * @tc.desc: function test SetLoop 3
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_022, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Url sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 1) {
            cout << "All sound loaded Url break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    playParameters.loop = 0;
    int32_t loop = 3;
    if (soundIDs_[loadNum_] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[loadNum_], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
        int32_t setPool = soundPool_->SetLoop(streamIDs_[playNum_], loop);
        EXPECT_EQ(MSERR_OK, setPool);
        sleep(waitTime1);
    } else {
        cout << "Get soundId failed, please try to get soundId: " << soundIDs_[loadNum_] << endl;
    }
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}

/**
 * @tc.name: soundpool_function_023
 * @tc.desc: function test SetLoop -2
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_023, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Url sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 1) {
            cout << "All sound loaded Url break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    playParameters.loop = 0;
    int32_t loop = -2;
    if (soundIDs_[loadNum_] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[loadNum_], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
        int32_t setPool = soundPool_->SetLoop(streamIDs_[playNum_], loop);
        EXPECT_EQ(MSERR_OK, setPool);
        sleep(waitTime1);
    } else {
        cout << "Get soundId failed, please try to get soundId: " << soundIDs_[loadNum_] << endl;
    }
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}

/**
 * @tc.name: soundpool_function_024
 * @tc.desc: function test SetLoop -1, streamID is -1
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_024, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Url sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 1) {
            cout << "All sound loaded Url break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    playParameters.loop = 0;
    int32_t loop = -1;
    if (soundIDs_[loadNum_] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[loadNum_], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
        int32_t setPool = soundPool_->SetLoop(-1, loop);
        EXPECT_EQ(MSERR_INVALID_OPERATION, setPool);
        sleep(waitTime1);
    } else {
        cout << "Get soundId failed, please try to get soundId: " << soundIDs_[loadNum_] << endl;
    }
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}

/**
 * @tc.name: soundpool_function_025
 * @tc.desc: function test SetLoop 2 with different streamId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_025, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    loadNum_++;
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Url sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 2) {
            cout << "All sound loaded Url break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    int32_t loop = 2;
    if (soundIDs_[0] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[0], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
        int32_t setPool = soundPool_->SetLoop(streamIDs_[playNum_], loop);
        EXPECT_EQ(MSERR_OK, setPool);
        sleep(waitTime3);
        playNum_++;
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[1], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
        int32_t setPool1 = soundPool_->SetLoop(streamIDs_[playNum_], loop);
        EXPECT_EQ(MSERR_OK, setPool1);
        sleep(waitTime3);
    } else {
        cout << "Get soundId failed, please try to get soundId: " << soundIDs_[loadNum_] << endl;
    }
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}

/**
 * @tc.name: soundpool_function_026
 * @tc.desc: function test SetLoop different loop with different streamId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_026, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    loadNum_++;
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Url sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 2) {
            cout << "All sound loaded Url break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    int32_t loop = 2;
    if (soundIDs_[0] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[0], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
        int32_t setPool = soundPool_->SetLoop(streamIDs_[playNum_], loop);
        EXPECT_EQ(MSERR_OK, setPool);
        sleep(waitTime3);
        playNum_++;
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[1], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
        int32_t setPool1 = soundPool_->SetLoop(streamIDs_[playNum_], -1);
        EXPECT_EQ(MSERR_OK, setPool1);
        sleep(waitTime3);
    } else {
        cout << "Get soundId failed, please try to get soundId: " << soundIDs_[loadNum_] << endl;
    }
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}

/**
 * @tc.name: soundpool_function_027
 * @tc.desc: function test SetRate with different rate
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_027, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Url sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 1) {
            cout << "All sound loaded Url break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    playParameters.loop = -1;
    if (soundIDs_[loadNum_] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[loadNum_], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
        int32_t setRateResult = soundPool_->SetRate(streamIDs_[playNum_], AudioRendererRate::RENDER_RATE_DOUBLE);
        EXPECT_EQ(MSERR_OK, setRateResult);
        sleep(waitTime3);
        int32_t setRateResult1 = soundPool_->SetRate(streamIDs_[playNum_], AudioRendererRate::RENDER_RATE_HALF);
        EXPECT_EQ(MSERR_OK, setRateResult1);
        sleep(waitTime3);
    } else {
        cout << "Get soundId failed, please try to get soundId: " << soundIDs_[loadNum_] << endl;
    }

    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}

/**
 * @tc.name: soundpool_function_028
 * @tc.desc: function test SetRate with different rate and streamID
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_028, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    loadNum_++;
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Url sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 2) {
            cout << "All sound loaded Url break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    if (soundIDs_[0] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[0], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
        int32_t setRateResult = soundPool_->SetRate(streamIDs_[playNum_], AudioRendererRate::RENDER_RATE_DOUBLE);
        EXPECT_EQ(MSERR_OK, setRateResult);
        sleep(waitTime3);
    }
    playNum_++;
    if (soundIDs_[1] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[1], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
        int32_t setRateResult = soundPool_->SetRate(streamIDs_[playNum_], AudioRendererRate::RENDER_RATE_HALF);
        EXPECT_EQ(MSERR_OK, setRateResult);
        sleep(waitTime3);
    }
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}

/**
 * @tc.name: soundpool_function_029
 * @tc.desc: function test SetRate with different streamID and same rate
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_029, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    loadNum_++;
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Url sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 2) {
            cout << "All sound loaded Url break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    playParameters.loop = -1;
    if (soundIDs_[0] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[0], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
        int32_t setRateResult = soundPool_->SetRate(streamIDs_[playNum_], AudioRendererRate::RENDER_RATE_DOUBLE);
        EXPECT_EQ(MSERR_OK, setRateResult);
        sleep(waitTime3);
    }
    playNum_++;
    if (soundIDs_[1] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[1], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
        int32_t setRateResult = soundPool_->SetRate(streamIDs_[playNum_], AudioRendererRate::RENDER_RATE_DOUBLE);
        EXPECT_EQ(MSERR_OK, setRateResult);
        sleep(waitTime3);
    }
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}

/**
 * @tc.name: soundpool_function_030
 * @tc.desc: function test SetVolume 0.5
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_030, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Url sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 1) {
            cout << "All sound loaded Url break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    float leftVolume = 0.5;
    float rightVolume = 0.5;
    if (soundIDs_[loadNum_] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[loadNum_], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
        int32_t setVol = soundPool_->SetVolume(streamIDs_[playNum_], leftVolume, rightVolume);
        EXPECT_EQ(MSERR_OK, setVol);
        sleep(waitTime1);
    } else {
        cout << "Get soundId failed, please try to get soundId: " << soundIDs_[loadNum_] << endl;
    }
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}

/**
 * @tc.name: soundpool_function_031
 * @tc.desc: function test SetVolume deifferent leftVolume
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_031, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Url sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 1) {
            cout << "All sound loaded Url break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    float leftVolume = 0.1;
    if (soundIDs_[loadNum_] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[loadNum_], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
        int32_t setVol = soundPool_->SetVolume(streamIDs_[playNum_], leftVolume, 0.0);
        EXPECT_EQ(MSERR_OK, setVol);
        sleep(waitTime3);
        int32_t setVol1 = soundPool_->SetVolume(streamIDs_[playNum_], 1.0, 0.0);
        EXPECT_EQ(MSERR_OK, setVol1);
        sleep(waitTime1);
    } else {
        cout << "Get soundId failed, please try to get soundId: " << soundIDs_[loadNum_] << endl;
    }
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}

/**
 * @tc.name: soundpool_function_032
 * @tc.desc: function test SetVolume leftVolume -1
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_032, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Url sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 1) {
            cout << "All sound loaded Url break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    float leftVolume = -1;
    if (soundIDs_[loadNum_] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[loadNum_], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
        int32_t setVol = soundPool_->SetVolume(streamIDs_[playNum_], leftVolume, 0.0);
        EXPECT_EQ(MSERR_OK, setVol);
        sleep(waitTime1);
    } else {
        cout << "Get soundId failed, please try to get soundId: " << soundIDs_[loadNum_] << endl;
    }
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}

/**
 * @tc.name: soundpool_function_033
 * @tc.desc: function test SetVolume rightVolume 2.0
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_033, TestSize.Level2)
{
    int maxStreams = 3;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Url sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 1) {
            cout << "All sound loaded Url break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    float rightVolume = 2.0;
    if (soundIDs_[loadNum_] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[loadNum_], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime1);
        int32_t setVol = soundPool_->SetVolume(streamIDs_[playNum_], 0.0, rightVolume);
        EXPECT_EQ(MSERR_OK, setVol);
        sleep(waitTime1);
    } else {
        cout << "Get soundId failed, please try to get soundId: " << soundIDs_[loadNum_] << endl;
    }
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}

/**
 * @tc.name: soundpool_function_034
 * @tc.desc: function test SetPriority 1
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_034, TestSize.Level2)
{
    int maxStreams = 1;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    loadNum_++;
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Url sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 2) {
            cout << "All sound loaded Url break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    playParameters.loop = -1;
    int32_t priority  = 1;
    if (soundIDs_[0] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[0], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime3);
    }
    playNum_++;
    if (soundIDs_[1] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[1], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime3);
        int32_t setPriority = soundPool_->SetPriority(streamIDs_[playNum_], priority);
        EXPECT_EQ(MSERR_OK, setPriority);
        sleep(waitTime1);
    }
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}


 /**
 * @tc.name: soundpool_function_035
 * @tc.desc: function test SetPriority
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SoundPoolUnitTest, soundpool_function_035, TestSize.Level2)
{
    int maxStreams = 1;
    create(maxStreams);
    std::shared_ptr<SoundPoolCallbackTest> cb = std::make_shared<SoundPoolCallbackTest>(soundPool_);
    int32_t ret = soundPool_->SetSoundPoolCallback(cb);
    if (ret != 0) {
        cout << "set callback failed" << endl;
    }
    loadUrl(g_fileName[loadNum_], loadNum_);
    loadNum_++;
    loadUrl(g_fileName[loadNum_], loadNum_);
    sleep(waitTime3);
    if (cb == nullptr) {
        cout << "Invalid cb to get loaded sound num." << endl;
        return;
    }
    while (true) {
        cout << "Have loaded Url sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
        if (cb->GetHaveLoadedSoundNum() == 2) {
            cout << "All sound loaded Url break. loaded sound num = " << cb->GetHaveLoadedSoundNum()  << endl;
            cb->ResetHaveLoadedSoundNum();
            break;
        }
    }
    struct PlayParams playParameters;
    playParameters.priority = 1;
    playParameters.loop = -1;
    int32_t priority  = -1;
    if (soundIDs_[0] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[0], playParameters);
        EXPECT_GT(streamIDs_[playNum_], 0);
        sleep(waitTime3);
    }
    playNum_++;
    if (soundIDs_[1] > 0) {
        streamIDs_[playNum_] = soundPool_->Play(soundIDs_[1], playParameters);
        EXPECT_GT(streamIDs_[1], 0);
        sleep(waitTime3);
    }
    int32_t setPriority = soundPool_->SetPriority(streamIDs_[0], priority);
    sleep(waitTime1);
    EXPECT_EQ(MSERR_OK, setPriority);
    if (cb == nullptr) {
        cout << "Invalid cb to get played sound num." << endl;
        return;
    }
    cb->ResetHavePlayedSoundNum();
}
} // namespace Media
} // namespace OHOS