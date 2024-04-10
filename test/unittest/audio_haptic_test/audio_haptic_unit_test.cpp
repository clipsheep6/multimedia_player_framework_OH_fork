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

#include "audio_haptic_unit_test.h"

#include "media_errors.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
const std::string AUDIO_TEST_URI = "data/test/ringtone.ogg";
const std::string HAPTIC_TEST_URI = "data/test/ringtone.json";

std::shared_ptr<AudioHapticManager> AudioHapticUnitTest::g_audioHapticManager = nullptr;
int32_t AudioHapticUnitTest::g_sourceId = -1;
std::shared_ptr<AudioHapticPlayer> AudioHapticUnitTest::g_audioHapticPlayer = nullptr;

void AudioHapticUnitTest::SetUpTestCase(void)
{
    g_audioHapticManager = AudioHapticManagerFactory::CreateAudioHapticManager();

    g_sourceId = g_audioHapticManager->RegisterSource(AUDIO_TEST_URI, HAPTIC_TEST_URI);

    AudioLatencyMode latencyMode = AudioLatencyMode::AUDIO_LATENCY_MODE_NORMAL;
    g_audioHapticManager->SetAudioLatencyMode(g_sourceId, latencyMode);
    AudioStandard::StreamUsage streamUsage = AudioStandard::StreamUsage::STREAM_USAGE_MUSIC;
    g_audioHapticManager->SetStreamUsage(g_sourceId, streamUsage);
    AudioHapticPlayerOptions options;
    options.muteAudio = false;
    options.muteHaptics = false;
    g_audioHapticPlayer = g_audioHapticManager->CreatePlayer(g_sourceId, options);
}

void AudioHapticUnitTest::TearDownTestCase(void) {}

void AudioHapticUnitTest::SetUp(void) {}

void AudioHapticUnitTest::TearDown(void) {}

/**
 * @tc.name  : Test AudioHapticManager RegisterSource API
 * @tc.number: AudioHapticManager_RegisterSource_001
 * @tc.desc  : Test AudioHapticManager RegisterSource interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticManager_RegisterSource_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticManager, nullptr);

    std::string audioUri = AUDIO_TEST_URI;
    std::string hapticUri = HAPTIC_TEST_URI;
    int32_t sourceId = AudioHapticUnitTest::g_audioHapticManager->RegisterSource(audioUri, hapticUri);
    EXPECT_NE(-1, sourceId);
}

/**
 * @tc.name  : Test AudioHapticManager UnregisterSource API
 * @tc.number: AudioHapticManager_UnregisterSource_001
 * @tc.desc  : Test AudioHapticManager UnregisterSource interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticManager_UnregisterSource_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticManager, nullptr);

    std::string audioUri = AUDIO_TEST_URI;
    std::string hapticUri = HAPTIC_TEST_URI;
    int32_t sourceId = AudioHapticUnitTest::g_audioHapticManager->RegisterSource(audioUri, hapticUri);
    EXPECT_NE(-1, sourceId);

    int32_t result = AudioHapticUnitTest::g_audioHapticManager->UnregisterSource(sourceId);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticManager UnregisterSource API
 * @tc.number: AudioHapticManager_UnregisterSource_002
 * @tc.desc  : Test AudioHapticManager UnregisterSource interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticManager_UnregisterSource_002, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticManager, nullptr);

    int32_t sourceId = -1;
    int32_t result = AudioHapticUnitTest::g_audioHapticManager->UnregisterSource(sourceId);
    EXPECT_NE(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticManager SetAudioLatencyMode API
 * @tc.number: AudioHapticManager_SetAudioLatencyMode_001
 * @tc.desc  : Test AudioHapticManager SetAudioLatencyMode interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticManager_SetAudioLatencyMode_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticManager, nullptr);

    std::string audioUri = AUDIO_TEST_URI;
    std::string hapticUri = HAPTIC_TEST_URI;
    int32_t sourceId = AudioHapticUnitTest::g_audioHapticManager->RegisterSource(audioUri, hapticUri);
    EXPECT_NE(-1, sourceId);

    AudioLatencyMode latencyMode = AudioLatencyMode::AUDIO_LATENCY_MODE_FAST;
    int32_t result = AudioHapticUnitTest::g_audioHapticManager->SetAudioLatencyMode(sourceId, latencyMode);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticManager SetAudioLatencyMode API
 * @tc.number: AudioHapticManager_SetAudioLatencyMode_002
 * @tc.desc  : Test AudioHapticManager SetAudioLatencyMode interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticManager_SetAudioLatencyMode_002, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticManager, nullptr);

    std::string audioUri = AUDIO_TEST_URI;
    std::string hapticUri = HAPTIC_TEST_URI;
    int32_t sourceId = AudioHapticUnitTest::g_audioHapticManager->RegisterSource(audioUri, hapticUri);
    EXPECT_NE(-1, sourceId);

    AudioLatencyMode latencyMode = AudioLatencyMode::AUDIO_LATENCY_MODE_NORMAL;
    int32_t result = AudioHapticUnitTest::g_audioHapticManager->SetAudioLatencyMode(sourceId, latencyMode);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticManager SetStreamUsage API
 * @tc.number: AudioHapticManager_SetStreamUsage_001
 * @tc.desc  : Test AudioHapticManager SetStreamUsage interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticManager_SetStreamUsage_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticManager, nullptr);

    std::string audioUri = AUDIO_TEST_URI;
    std::string hapticUri = HAPTIC_TEST_URI;
    int32_t sourceId = AudioHapticUnitTest::g_audioHapticManager->RegisterSource(audioUri, hapticUri);
    EXPECT_NE(-1, sourceId);

    int32_t result = MSERR_OK;
    AudioLatencyMode latencyMode = AudioLatencyMode::AUDIO_LATENCY_MODE_NORMAL;
    result = AudioHapticUnitTest::g_audioHapticManager->SetAudioLatencyMode(sourceId, latencyMode);
    EXPECT_EQ(MSERR_OK, result);

    AudioStandard::StreamUsage streamUsage = AudioStandard::StreamUsage::STREAM_USAGE_MUSIC;
    result = AudioHapticUnitTest::g_audioHapticManager->SetStreamUsage(sourceId, streamUsage);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticManager SetStreamUsage API
 * @tc.number: AudioHapticManager_SetStreamUsage_002
 * @tc.desc  : Test AudioHapticManager SetStreamUsage interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticManager_SetStreamUsage_002, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticManager, nullptr);

    std::string audioUri = AUDIO_TEST_URI;
    std::string hapticUri = HAPTIC_TEST_URI;
    int32_t sourceId = AudioHapticUnitTest::g_audioHapticManager->RegisterSource(audioUri, hapticUri);
    EXPECT_NE(-1, sourceId);

    int32_t result = MSERR_OK;
    AudioLatencyMode latencyMode = AudioLatencyMode::AUDIO_LATENCY_MODE_NORMAL;
    result = AudioHapticUnitTest::g_audioHapticManager->SetAudioLatencyMode(sourceId, latencyMode);
    EXPECT_EQ(MSERR_OK, result);

    AudioStandard::StreamUsage streamUsage = AudioStandard::StreamUsage::STREAM_USAGE_RINGTONE;
    result = AudioHapticUnitTest::g_audioHapticManager->SetStreamUsage(sourceId, streamUsage);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticManager SetStreamUsage API
 * @tc.number: AudioHapticManager_SetStreamUsage_003
 * @tc.desc  : Test AudioHapticManager SetStreamUsage interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticManager_SetStreamUsage_003, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticManager, nullptr);

    std::string audioUri = AUDIO_TEST_URI;
    std::string hapticUri = HAPTIC_TEST_URI;
    int32_t sourceId = AudioHapticUnitTest::g_audioHapticManager->RegisterSource(audioUri, hapticUri);
    EXPECT_NE(-1, sourceId);

    int32_t result = MSERR_OK;
    AudioLatencyMode latencyMode = AudioLatencyMode::AUDIO_LATENCY_MODE_NORMAL;
    result = AudioHapticUnitTest::g_audioHapticManager->SetAudioLatencyMode(sourceId, latencyMode);
    EXPECT_EQ(MSERR_OK, result);

    AudioStandard::StreamUsage streamUsage = AudioStandard::StreamUsage::STREAM_USAGE_NOTIFICATION;
    result = AudioHapticUnitTest::g_audioHapticManager->SetStreamUsage(sourceId, streamUsage);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticManager SetStreamUsage API
 * @tc.number: AudioHapticManager_SetStreamUsage_004
 * @tc.desc  : Test AudioHapticManager SetStreamUsage interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticManager_SetStreamUsage_004, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticManager, nullptr);

    std::string audioUri = AUDIO_TEST_URI;
    std::string hapticUri = HAPTIC_TEST_URI;
    int32_t sourceId = AudioHapticUnitTest::g_audioHapticManager->RegisterSource(audioUri, hapticUri);
    EXPECT_NE(-1, sourceId);

    int32_t result = MSERR_OK;
    AudioLatencyMode latencyMode = AudioLatencyMode::AUDIO_LATENCY_MODE_NORMAL;
    result = AudioHapticUnitTest::g_audioHapticManager->SetAudioLatencyMode(sourceId, latencyMode);
    EXPECT_EQ(MSERR_OK, result);

    AudioStandard::StreamUsage streamUsage = AudioStandard::StreamUsage::STREAM_USAGE_UNKNOWN;
    result = AudioHapticUnitTest::g_audioHapticManager->SetStreamUsage(sourceId, streamUsage);
    EXPECT_NE(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticManager SetStreamUsage API
 * @tc.number: AudioHapticManager_SetStreamUsage_005
 * @tc.desc  : Test AudioHapticManager SetStreamUsage interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticManager_SetStreamUsage_005, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticManager, nullptr);

    std::string audioUri = AUDIO_TEST_URI;
    std::string hapticUri = HAPTIC_TEST_URI;
    int32_t sourceId = -1;
    int32_t result = MSERR_OK;

    AudioStandard::StreamUsage streamUsage = AudioStandard::StreamUsage::STREAM_USAGE_MUSIC;
    result = AudioHapticUnitTest::g_audioHapticManager->SetStreamUsage(sourceId, streamUsage);
    EXPECT_NE(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticManager CreatePlayer API
 * @tc.number: AudioHapticManager_CreatePlayer_001
 * @tc.desc  : Test AudioHapticManager CreatePlayer interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticManager_CreatePlayer_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticManager, nullptr);

    std::string audioUri = AUDIO_TEST_URI;
    std::string hapticUri = HAPTIC_TEST_URI;
    int32_t sourceId = AudioHapticUnitTest::g_audioHapticManager->RegisterSource(audioUri, hapticUri);
    EXPECT_NE(-1, sourceId);

    int32_t result = MSERR_OK;
    AudioLatencyMode latencyMode = AudioLatencyMode::AUDIO_LATENCY_MODE_NORMAL;
    result = AudioHapticUnitTest::g_audioHapticManager->SetAudioLatencyMode(sourceId, latencyMode);
    EXPECT_EQ(MSERR_OK, result);

    AudioStandard::StreamUsage streamUsage = AudioStandard::StreamUsage::STREAM_USAGE_MUSIC;
    result = AudioHapticUnitTest::g_audioHapticManager->SetStreamUsage(sourceId, streamUsage);
    EXPECT_EQ(MSERR_OK, result);

    AudioHapticPlayerOptions options;
    options.muteAudio = false;
    options.muteHaptics = false;
    std::shared_ptr<AudioHapticPlayer> audioHapticPlayer =
        AudioHapticUnitTest::g_audioHapticManager->CreatePlayer(sourceId, options);
    EXPECT_NE(nullptr, audioHapticPlayer);
}

/**
 * @tc.name  : Test AudioHapticManager CreatePlayer API
 * @tc.number: AudioHapticManager_CreatePlayer_002
 * @tc.desc  : Test AudioHapticManager CreatePlayer interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticManager_CreatePlayer_002, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticManager, nullptr);

    std::string audioUri = AUDIO_TEST_URI;
    std::string hapticUri = HAPTIC_TEST_URI;
    int32_t sourceId = AudioHapticUnitTest::g_audioHapticManager->RegisterSource(audioUri, hapticUri);
    EXPECT_NE(-1, sourceId);

    int32_t result = MSERR_OK;
    AudioLatencyMode latencyMode = AudioLatencyMode::AUDIO_LATENCY_MODE_NORMAL;
    result = AudioHapticUnitTest::g_audioHapticManager->SetAudioLatencyMode(sourceId, latencyMode);
    EXPECT_EQ(MSERR_OK, result);

    AudioStandard::StreamUsage streamUsage = AudioStandard::StreamUsage::STREAM_USAGE_MUSIC;
    result = AudioHapticUnitTest::g_audioHapticManager->SetStreamUsage(sourceId, streamUsage);
    EXPECT_EQ(MSERR_OK, result);

    AudioHapticPlayerOptions options;
    options.muteAudio = true;
    options.muteHaptics = false;
    std::shared_ptr<AudioHapticPlayer> audioHapticPlayer =
        AudioHapticUnitTest::g_audioHapticManager->CreatePlayer(sourceId, options);
    EXPECT_NE(nullptr, audioHapticPlayer);
}

/**
 * @tc.name  : Test AudioHapticManager CreatePlayer API
 * @tc.number: AudioHapticManager_CreatePlayer_003
 * @tc.desc  : Test AudioHapticManager CreatePlayer interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticManager_CreatePlayer_003, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticManager, nullptr);

    std::string audioUri = AUDIO_TEST_URI;
    std::string hapticUri = HAPTIC_TEST_URI;
    int32_t sourceId = AudioHapticUnitTest::g_audioHapticManager->RegisterSource(audioUri, hapticUri);
    EXPECT_NE(-1, sourceId);

    int32_t result = MSERR_OK;
    AudioLatencyMode latencyMode = AudioLatencyMode::AUDIO_LATENCY_MODE_NORMAL;
    result = AudioHapticUnitTest::g_audioHapticManager->SetAudioLatencyMode(sourceId, latencyMode);
    EXPECT_EQ(MSERR_OK, result);

    AudioStandard::StreamUsage streamUsage = AudioStandard::StreamUsage::STREAM_USAGE_MUSIC;
    result = AudioHapticUnitTest::g_audioHapticManager->SetStreamUsage(sourceId, streamUsage);
    EXPECT_EQ(MSERR_OK, result);

    AudioHapticPlayerOptions options;
    options.muteAudio = false;
    options.muteHaptics = true;
    std::shared_ptr<AudioHapticPlayer> audioHapticPlayer =
        AudioHapticUnitTest::g_audioHapticManager->CreatePlayer(sourceId, options);
    EXPECT_NE(nullptr, audioHapticPlayer);
}

/**
 * @tc.name  : Test AudioHapticManager CreatePlayer API
 * @tc.number: AudioHapticManager_CreatePlayer_004
 * @tc.desc  : Test AudioHapticManager CreatePlayer interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticManager_CreatePlayer_004, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticManager, nullptr);

    std::string audioUri = AUDIO_TEST_URI;
    std::string hapticUri = HAPTIC_TEST_URI;
    int32_t sourceId = AudioHapticUnitTest::g_audioHapticManager->RegisterSource(audioUri, hapticUri);
    EXPECT_NE(-1, sourceId);

    int32_t result = MSERR_OK;
    AudioLatencyMode latencyMode = AudioLatencyMode::AUDIO_LATENCY_MODE_NORMAL;
    result = AudioHapticUnitTest::g_audioHapticManager->SetAudioLatencyMode(sourceId, latencyMode);
    EXPECT_EQ(MSERR_OK, result);

    AudioStandard::StreamUsage streamUsage = AudioStandard::StreamUsage::STREAM_USAGE_MUSIC;
    result = AudioHapticUnitTest::g_audioHapticManager->SetStreamUsage(sourceId, streamUsage);
    EXPECT_EQ(MSERR_OK, result);

    AudioHapticPlayerOptions options;
    options.muteAudio = true;
    options.muteHaptics = true;
    std::shared_ptr<AudioHapticPlayer> audioHapticPlayer =
        AudioHapticUnitTest::g_audioHapticManager->CreatePlayer(sourceId, options);
    EXPECT_NE(nullptr, audioHapticPlayer);
}

/**
 * @tc.name  : Test AudioHapticManager CreatePlayer API
 * @tc.number: AudioHapticManager_CreatePlayer_005
 * @tc.desc  : Test AudioHapticManager CreatePlayer interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticManager_CreatePlayer_005, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticManager, nullptr);

    AudioHapticPlayerOptions options;
    options.muteAudio = false;
    options.muteHaptics = false;
    std::shared_ptr<AudioHapticPlayer> audioHapticPlayer =
        AudioHapticUnitTest::g_audioHapticManager->CreatePlayer(-1, options);
    EXPECT_EQ(nullptr, audioHapticPlayer);
}

/**
 * @tc.name  : Test AudioHapticPlayer IsMuted API
 * @tc.number: AudioHapticPlayer_IsMuted_001
 * @tc.desc  : Test AudioHapticPlayer IsMuted interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_IsMuted_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_sourceId, -1);
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticPlayer, nullptr);

    AudioHapticType type = AudioHapticType::AUDIO_HAPTIC_TYPE_AUDIO;
    bool result = AudioHapticUnitTest::g_audioHapticPlayer->IsMuted(type);
    EXPECT_EQ(false, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer IsMuted API
 * @tc.number: AudioHapticPlayer_IsMuted_002
 * @tc.desc  : Test AudioHapticPlayer IsMuted interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_IsMuted_002, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_sourceId, -1);
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticPlayer, nullptr);

    AudioHapticType type = AudioHapticType::AUDIO_HAPTIC_TYPE_HAPTIC;
    bool result = AudioHapticUnitTest::g_audioHapticPlayer->IsMuted(type);
    EXPECT_EQ(false, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetVolume API
 * @tc.number: AudioHapticPlayer_SetVolume_001
 * @tc.desc  : Test AudioHapticPlayer SetVolume interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetVolume_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_sourceId, -1);
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticPlayer, nullptr);

    float volume = 0.0f;
    int32_t result = AudioHapticUnitTest::g_audioHapticPlayer->SetVolume(volume);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetVolume API
 * @tc.number: AudioHapticPlayer_SetVolume_002
 * @tc.desc  : Test AudioHapticPlayer SetVolume interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetVolume_002, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_sourceId, -1);
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticPlayer, nullptr);

    float volume = 0.5f;
    int32_t result = AudioHapticUnitTest::g_audioHapticPlayer->SetVolume(volume);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetVolume API
 * @tc.number: AudioHapticPlayer_SetVolume_003
 * @tc.desc  : Test AudioHapticPlayer SetVolume interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetVolume_003, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_sourceId, -1);
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticPlayer, nullptr);

    float volume = 1.0f;
    int32_t result = AudioHapticUnitTest::g_audioHapticPlayer->SetVolume(volume);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetVolume API
 * @tc.number: AudioHapticPlayer_SetVolume_004
 * @tc.desc  : Test AudioHapticPlayer SetVolume interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetVolume_004, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_sourceId, -1);
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticPlayer, nullptr);

    float volume = -0.5f;
    int32_t result = AudioHapticUnitTest::g_audioHapticPlayer->SetVolume(volume);
    EXPECT_NE(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetVolume API
 * @tc.number: AudioHapticPlayer_SetVolume_005
 * @tc.desc  : Test AudioHapticPlayer SetVolume interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetVolume_005, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_sourceId, -1);
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticPlayer, nullptr);

    float volume = 1.5f;
    int32_t result = AudioHapticUnitTest::g_audioHapticPlayer->SetVolume(volume);
    EXPECT_NE(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetLoop API
 * @tc.number: AudioHapticPlayer_SetLoop_001
 * @tc.desc  : Test AudioHapticPlayer SetLoop interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetLoop_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_sourceId, -1);
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticPlayer, nullptr);

    bool loop = true;
    int32_t result = AudioHapticUnitTest::g_audioHapticPlayer->SetLoop(loop);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetLoop API
 * @tc.number: AudioHapticPlayer_SetLoop_002
 * @tc.desc  : Test AudioHapticPlayer SetLoop interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetLoop_002, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_sourceId, -1);
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticPlayer, nullptr);

    bool loop = false;
    int32_t result = AudioHapticUnitTest::g_audioHapticPlayer->SetLoop(loop);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetAudioHapticPlayerCallback API
 * @tc.number: AudioHapticPlayer_SetAudioHapticPlayerCallback_001
 * @tc.desc  : Test AudioHapticPlayer SetAudioHapticPlayerCallback interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetAudioHapticPlayerCallback_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_sourceId, -1);
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticPlayer, nullptr);

    std::shared_ptr<AudioHapticPlayerCallback> callback = nullptr;
    int32_t result = AudioHapticUnitTest::g_audioHapticPlayer->SetAudioHapticPlayerCallback(callback);
    EXPECT_NE(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer GetAudioCurrentTime API
 * @tc.number: AudioHapticPlayer_GetAudioCurrentTime_001
 * @tc.desc  : Test AudioHapticPlayer GetAudioCurrentTime interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_GetAudioCurrentTime_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_sourceId, -1);
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticPlayer, nullptr);

    int32_t result = AudioHapticUnitTest::g_audioHapticPlayer->GetAudioCurrentTime();
    EXPECT_EQ(-1, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer Prepare API
 * @tc.number: AudioHapticPlayer_Prepare_001
 * @tc.desc  : Test AudioHapticPlayer Prepare interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_Prepare_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_sourceId, -1);
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticPlayer, nullptr);

    int32_t result = MSERR_OK;
    result = AudioHapticUnitTest::g_audioHapticPlayer->Prepare();
    if (result == MSERR_OPEN_FILE_FAILED || result == MSERR_UNSUPPORT_FILE) {
        // The source file is invalid or the path is inaccessible. Return directly.
        EXPECT_NE(MSERR_OK, result);
        return;
    }

    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer Start API
 * @tc.number: AudioHapticPlayer_Start_001
 * @tc.desc  : Test AudioHapticPlayer Start interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_Start_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_sourceId, -1);
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticPlayer, nullptr);

    int32_t result = MSERR_OK;
    result = AudioHapticUnitTest::g_audioHapticPlayer->Prepare();
    if (result == MSERR_OPEN_FILE_FAILED || result == MSERR_UNSUPPORT_FILE) {
        // The source file is invalid or the path is inaccessible. Return directly.
        EXPECT_NE(MSERR_OK, result);
        return;
    }

    result = AudioHapticUnitTest::g_audioHapticPlayer->Start();
    EXPECT_EQ(MSERR_OK, result);

    AudioHapticUnitTest::g_audioHapticPlayer->Stop();
}

/**
 * @tc.name  : Test AudioHapticPlayer Stop API
 * @tc.number: AudioHapticPlayer_Stop_001
 * @tc.desc  : Test AudioHapticPlayer Stop interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_Stop_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_sourceId, -1);
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticPlayer, nullptr);

    int32_t result = MSERR_OK;
    result = AudioHapticUnitTest::g_audioHapticPlayer->Prepare();
    if (result == MSERR_OPEN_FILE_FAILED || result == MSERR_UNSUPPORT_FILE) {
        // The source file is invalid or the path is inaccessible. Return directly.
        EXPECT_NE(MSERR_OK, result);
        return;
    }
    AudioHapticUnitTest::g_audioHapticPlayer->Start();

    result = AudioHapticUnitTest::g_audioHapticPlayer->Stop();
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer Release API
 * @tc.number: AudioHapticPlayer_Release_001
 * @tc.desc  : Test AudioHapticPlayer Release interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_Release_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_sourceId, -1);
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticPlayer, nullptr);

    int32_t result = MSERR_OK;
    result = AudioHapticUnitTest::g_audioHapticPlayer->Prepare();
    if (result == MSERR_OPEN_FILE_FAILED || result == MSERR_UNSUPPORT_FILE) {
        // The source file is invalid or the path is inaccessible. Return directly.
        EXPECT_NE(MSERR_OK, result);
        return;
    }
    AudioHapticUnitTest::g_audioHapticPlayer->Start();
    AudioHapticUnitTest::g_audioHapticPlayer->Stop();

    result = AudioHapticUnitTest::g_audioHapticPlayer->Release();
    EXPECT_EQ(MSERR_OK, result);
}
} // namespace Media
} // namespace OHOS
