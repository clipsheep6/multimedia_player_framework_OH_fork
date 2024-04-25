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

namespace OHOS {
namespace Media {
using namespace std;
using namespace testing::ext;
using namespace Security::AccessToken;
using Security::AccessToken::AccessTokenID;

const std::string AUDIO_TEST_URI = "ringtone.ogg";
const std::string HAPTIC_TEST_URI = "ringtone.json";
const std::string HAPTIC_TEST_EFFECT_ID = "haptic.clock.timer";

std::shared_ptr<AudioHapticManager> AudioHapticUnitTest::g_audioHapticManager = nullptr;

int32_t AudioHapticUnitTest::g_normalSourceId = -1;
int32_t AudioHapticUnitTest::g_lowLatencySourceId = -1;
int32_t AudioHapticUnitTest::g_effectSourceId = -1;
std::shared_ptr<AudioHapticPlayer> AudioHapticUnitTest::g_normalAudioHapticPlayer = nullptr;
std::shared_ptr<AudioHapticPlayer> AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer = nullptr;
std::shared_ptr<AudioHapticPlayer> AudioHapticUnitTest::g_effectAudioHapticPlayer = nullptr;

AccessTokenID AudioHapticUnitTest::g_tokenId = 0;

void AudioHapticUnitTest::SetUpTestCase(void)
{
    // SetSelfTokenID to foundation for ohos.permission.VIBRATE
    g_tokenId = AccessTokenKit::GetNativeTokenId("foundation");
    ASSERT_NE(0, g_tokenId);
    ASSERT_EQ(0, SetSelfTokenID(g_tokenId));

    g_audioHapticManager = AudioHapticManagerFactory::CreateAudioHapticManager();
    ASSERT_NE(g_audioHapticManager, nullptr);

    // Initilize normal audio haptic player.
    g_normalSourceId = g_audioHapticManager->RegisterSource(AUDIO_TEST_URI, HAPTIC_TEST_URI);
    AudioLatencyMode latencyMode = AudioLatencyMode::AUDIO_LATENCY_MODE_NORMAL;
    g_audioHapticManager->SetAudioLatencyMode(g_normalSourceId, latencyMode);
    AudioStandard::StreamUsage streamUsage = AudioStandard::StreamUsage::STREAM_USAGE_MUSIC;
    g_audioHapticManager->SetStreamUsage(g_normalSourceId, streamUsage);
    AudioHapticPlayerOptions options;
    options.muteAudio = false;
    options.muteHaptics = false;
    g_normalAudioHapticPlayer = g_audioHapticManager->CreatePlayer(g_normalSourceId, options);
    ASSERT_NE(g_normalAudioHapticPlayer, nullptr);

    // Initilize low latency audio haptic player.
    g_lowLatencySourceId = g_audioHapticManager->RegisterSource(AUDIO_TEST_URI, HAPTIC_TEST_URI);
    latencyMode = AudioLatencyMode::AUDIO_LATENCY_MODE_FAST;
    g_audioHapticManager->SetAudioLatencyMode(g_lowLatencySourceId, latencyMode);
    streamUsage = AudioStandard::StreamUsage::STREAM_USAGE_GAME;
    g_audioHapticManager->SetStreamUsage(g_lowLatencySourceId, streamUsage);
    g_lowLatencyAudioHapticPlayer = g_audioHapticManager->CreatePlayer(g_lowLatencySourceId, options);
    ASSERT_NE(g_lowLatencyAudioHapticPlayer, nullptr);

    g_effectSourceId = g_audioHapticManager->RegisterSourceWithEffectId(AUDIO_TEST_URI, HAPTIC_TEST_EFFECT_ID);
    latencyMode = AudioLatencyMode::AUDIO_LATENCY_MODE_FAST;
    g_audioHapticManager->SetAudioLatencyMode(g_effectSourceId, latencyMode);
    streamUsage = AudioStandard::StreamUsage::STREAM_USAGE_NOTIFICATION;
    g_audioHapticManager->SetStreamUsage(g_effectSourceId, streamUsage);
    g_effectAudioHapticPlayer = g_audioHapticManager->CreatePlayer(g_effectSourceId, options);
    ASSERT_NE(g_effectAudioHapticPlayer, nullptr);
}

void AudioHapticUnitTest::TearDownTestCase(void)
{
}

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

    AudioHapticUnitTest::g_audioHapticManager->UnregisterSource(sourceId);
}

/**
 * @tc.name  : Test AudioHapticManager RegisterSourceWithEffectId API
 * @tc.number: AudioHapticManager_RegisterSourceWithEffectId_001
 * @tc.desc  : Test AudioHapticManager RegisterSourceWithEffectId interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticManager_RegisterSourceWithEffectId_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticManager, nullptr);

    std::string audioUri = AUDIO_TEST_URI;
    std::string effectId = HAPTIC_TEST_EFFECT_ID;
    int32_t sourceId = AudioHapticUnitTest::g_audioHapticManager->RegisterSourceWithEffectId(audioUri, effectId);
    EXPECT_NE(-1, sourceId);

    AudioHapticUnitTest::g_audioHapticManager->UnregisterSource(sourceId);
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
 * @tc.name  : Test AudioHapticManager UnregisterSource API
 * @tc.number: AudioHapticManager_UnregisterSource_003
 * @tc.desc  : Test AudioHapticManager UnregisterSource interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticManager_UnregisterSource_003, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticManager, nullptr);

    std::string audioUri = AUDIO_TEST_URI;
    std::string effectId = HAPTIC_TEST_EFFECT_ID;
    int32_t sourceId = AudioHapticUnitTest::g_audioHapticManager->RegisterSourceWithEffectId(audioUri, effectId);
    EXPECT_NE(-1, sourceId);

    int32_t result = AudioHapticUnitTest::g_audioHapticManager->UnregisterSource(sourceId);
    EXPECT_EQ(MSERR_OK, result);
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

    AudioHapticUnitTest::g_audioHapticManager->UnregisterSource(sourceId);
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

    AudioHapticUnitTest::g_audioHapticManager->UnregisterSource(sourceId);
}

/**
 * @tc.name  : Test AudioHapticManager SetAudioLatencyMode API
 * @tc.number: AudioHapticManager_SetAudioLatencyMode_003
 * @tc.desc  : Test AudioHapticManager SetAudioLatencyMode interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticManager_SetAudioLatencyMode_003, TestSize.Level1)
{
    int32_t sourceId = -1;
    AudioLatencyMode latencyMode = AudioLatencyMode::AUDIO_LATENCY_MODE_NORMAL;
    int32_t result = AudioHapticUnitTest::g_audioHapticManager->SetAudioLatencyMode(sourceId, latencyMode);
    EXPECT_NE(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticManager SetAudioLatencyMode API
 * @tc.number: AudioHapticManager_SetAudioLatencyMode_004
 * @tc.desc  : Test AudioHapticManager SetAudioLatencyMode interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticManager_SetAudioLatencyMode_004, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticManager, nullptr);

    std::string audioUri = AUDIO_TEST_URI;
    std::string effectId = HAPTIC_TEST_EFFECT_ID;
    int32_t sourceId = AudioHapticUnitTest::g_audioHapticManager->RegisterSourceWithEffectId(audioUri, effectId);
    EXPECT_NE(-1, sourceId);

    AudioLatencyMode latencyMode = AudioLatencyMode::AUDIO_LATENCY_MODE_NORMAL;
    int32_t result = AudioHapticUnitTest::g_audioHapticManager->SetAudioLatencyMode(sourceId, latencyMode);
    EXPECT_NE(MSERR_OK, result);

    AudioHapticUnitTest::g_audioHapticManager->UnregisterSource(sourceId);
}

/**
 * @tc.name  : Test AudioHapticManager SetAudioLatencyMode API
 * @tc.number: AudioHapticManager_SetAudioLatencyMode_005
 * @tc.desc  : Test AudioHapticManager SetAudioLatencyMode interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticManager_SetAudioLatencyMode_005, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticManager, nullptr);

    std::string audioUri = AUDIO_TEST_URI;
    std::string effectId = HAPTIC_TEST_EFFECT_ID;
    int32_t sourceId = AudioHapticUnitTest::g_audioHapticManager->RegisterSourceWithEffectId(audioUri, effectId);
    EXPECT_NE(-1, sourceId);

    AudioLatencyMode latencyMode = AudioLatencyMode::AUDIO_LATENCY_MODE_FAST;
    int32_t result = AudioHapticUnitTest::g_audioHapticManager->SetAudioLatencyMode(sourceId, latencyMode);
    EXPECT_EQ(MSERR_OK, result);

    AudioHapticUnitTest::g_audioHapticManager->UnregisterSource(sourceId);
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

    AudioHapticUnitTest::g_audioHapticManager->UnregisterSource(sourceId);
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

    AudioHapticUnitTest::g_audioHapticManager->UnregisterSource(sourceId);
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

    AudioHapticUnitTest::g_audioHapticManager->UnregisterSource(sourceId);
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

    AudioHapticUnitTest::g_audioHapticManager->UnregisterSource(sourceId);
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

    AudioHapticUnitTest::g_audioHapticManager->UnregisterSource(sourceId);
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

    AudioHapticUnitTest::g_audioHapticManager->UnregisterSource(sourceId);
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

    AudioHapticUnitTest::g_audioHapticManager->UnregisterSource(sourceId);
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

    AudioHapticUnitTest::g_audioHapticManager->UnregisterSource(sourceId);
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
    EXPECT_NE(AudioHapticUnitTest::g_normalAudioHapticPlayer, nullptr);

    AudioHapticType type = AudioHapticType::AUDIO_HAPTIC_TYPE_AUDIO;
    bool result = AudioHapticUnitTest::g_normalAudioHapticPlayer->IsMuted(type);
    EXPECT_EQ(false, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer IsMuted API
 * @tc.number: AudioHapticPlayer_IsMuted_002
 * @tc.desc  : Test AudioHapticPlayer IsMuted interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_IsMuted_002, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_normalAudioHapticPlayer, nullptr);

    AudioHapticType type = AudioHapticType::AUDIO_HAPTIC_TYPE_HAPTIC;
    bool result = AudioHapticUnitTest::g_normalAudioHapticPlayer->IsMuted(type);
    EXPECT_EQ(false, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetVolume API
 * @tc.number: AudioHapticPlayer_SetVolume_001
 * @tc.desc  : Test AudioHapticPlayer SetVolume interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetVolume_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_normalAudioHapticPlayer, nullptr);

    float volume = 0.0f;
    int32_t result = AudioHapticUnitTest::g_normalAudioHapticPlayer->SetVolume(volume);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetVolume API
 * @tc.number: AudioHapticPlayer_SetVolume_002
 * @tc.desc  : Test AudioHapticPlayer SetVolume interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetVolume_002, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_normalAudioHapticPlayer, nullptr);

    float volume = 0.5f;
    int32_t result = AudioHapticUnitTest::g_normalAudioHapticPlayer->SetVolume(volume);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetVolume API
 * @tc.number: AudioHapticPlayer_SetVolume_003
 * @tc.desc  : Test AudioHapticPlayer SetVolume interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetVolume_003, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_normalAudioHapticPlayer, nullptr);

    float volume = 1.0f;
    int32_t result = AudioHapticUnitTest::g_normalAudioHapticPlayer->SetVolume(volume);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetVolume API
 * @tc.number: AudioHapticPlayer_SetVolume_004
 * @tc.desc  : Test AudioHapticPlayer SetVolume interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetVolume_004, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_normalAudioHapticPlayer, nullptr);

    float volume = -0.5f;
    int32_t result = AudioHapticUnitTest::g_normalAudioHapticPlayer->SetVolume(volume);
    EXPECT_NE(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetVolume API
 * @tc.number: AudioHapticPlayer_SetVolume_005
 * @tc.desc  : Test AudioHapticPlayer SetVolume interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetVolume_005, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_normalAudioHapticPlayer, nullptr);

    float volume = 1.5f;
    int32_t result = AudioHapticUnitTest::g_normalAudioHapticPlayer->SetVolume(volume);
    EXPECT_NE(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetVolume API
 * @tc.number: AudioHapticPlayer_SetVolume_006
 * @tc.desc  : Test AudioHapticPlayer SetVolume interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetVolume_006, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer, nullptr);

    float volume = 0.0f;
    int32_t result = AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer->SetVolume(volume);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetVolume API
 * @tc.number: AudioHapticPlayer_SetVolume_007
 * @tc.desc  : Test AudioHapticPlayer SetVolume interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetVolume_007, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer, nullptr);

    float volume = 0.5f;
    int32_t result = AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer->SetVolume(volume);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetVolume API
 * @tc.number: AudioHapticPlayer_SetVolume_008
 * @tc.desc  : Test AudioHapticPlayer SetVolume interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetVolume_008, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer, nullptr);

    float volume = 1.0f;
    int32_t result = AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer->SetVolume(volume);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetVolume API
 * @tc.number: AudioHapticPlayer_SetVolume_009
 * @tc.desc  : Test AudioHapticPlayer SetVolume interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetVolume_009, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer, nullptr);

    float volume = -0.5f;
    int32_t result = AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer->SetVolume(volume);
    EXPECT_NE(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetVolume API
 * @tc.number: AudioHapticPlayer_SetVolume_010
 * @tc.desc  : Test AudioHapticPlayer SetVolume interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetVolume_010, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer, nullptr);

    float volume = 1.5f;
    int32_t result = AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer->SetVolume(volume);
    EXPECT_NE(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetVolume API
 * @tc.number: AudioHapticPlayer_SetVolume_011
 * @tc.desc  : Test AudioHapticPlayer SetVolume interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetVolume_011, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_effectAudioHapticPlayer, nullptr);

    float volume = 0.0f;
    int32_t result = AudioHapticUnitTest::g_effectAudioHapticPlayer->SetVolume(volume);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetVolume API
 * @tc.number: AudioHapticPlayer_SetVolume_012
 * @tc.desc  : Test AudioHapticPlayer SetVolume interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetVolume_012, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_effectAudioHapticPlayer, nullptr);

    float volume = 0.5f;
    int32_t result = AudioHapticUnitTest::g_effectAudioHapticPlayer->SetVolume(volume);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetVolume API
 * @tc.number: AudioHapticPlayer_SetVolume_013
 * @tc.desc  : Test AudioHapticPlayer SetVolume interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetVolume_013, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_effectAudioHapticPlayer, nullptr);

    float volume = 1.0f;
    int32_t result = AudioHapticUnitTest::g_effectAudioHapticPlayer->SetVolume(volume);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetVolume API
 * @tc.number: AudioHapticPlayer_SetVolume_014
 * @tc.desc  : Test AudioHapticPlayer SetVolume interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetVolume_014, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_effectAudioHapticPlayer, nullptr);

    float volume = -0.5f;
    int32_t result = AudioHapticUnitTest::g_effectAudioHapticPlayer->SetVolume(volume);
    EXPECT_NE(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetVolume API
 * @tc.number: AudioHapticPlayer_SetVolume_015
 * @tc.desc  : Test AudioHapticPlayer SetVolume interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetVolume_015, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_effectAudioHapticPlayer, nullptr);

    float volume = 1.5f;
    int32_t result = AudioHapticUnitTest::g_effectAudioHapticPlayer->SetVolume(volume);
    EXPECT_NE(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetLoop API
 * @tc.number: AudioHapticPlayer_SetLoop_001
 * @tc.desc  : Test AudioHapticPlayer SetLoop interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetLoop_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_normalAudioHapticPlayer, nullptr);

    bool loop = true;
    int32_t result = AudioHapticUnitTest::g_normalAudioHapticPlayer->SetLoop(loop);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetLoop API
 * @tc.number: AudioHapticPlayer_SetLoop_002
 * @tc.desc  : Test AudioHapticPlayer SetLoop interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetLoop_002, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_normalAudioHapticPlayer, nullptr);

    bool loop = false;
    int32_t result = AudioHapticUnitTest::g_normalAudioHapticPlayer->SetLoop(loop);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetLoop API
 * @tc.number: AudioHapticPlayer_SetLoop_003
 * @tc.desc  : Test AudioHapticPlayer SetLoop interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetLoop_003, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer, nullptr);

    bool loop = true;
    int32_t result = AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer->SetLoop(loop);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetLoop API
 * @tc.number: AudioHapticPlayer_SetLoop_004
 * @tc.desc  : Test AudioHapticPlayer SetLoop interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetLoop_004, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer, nullptr);

    bool loop = false;
    int32_t result = AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer->SetLoop(loop);
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetAudioHapticPlayerCallback API
 * @tc.number: AudioHapticPlayer_SetAudioHapticPlayerCallback_001
 * @tc.desc  : Test AudioHapticPlayer SetAudioHapticPlayerCallback interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetAudioHapticPlayerCallback_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_normalAudioHapticPlayer, nullptr);

    std::shared_ptr<AudioHapticPlayerCallback> callback = nullptr;
    int32_t result = AudioHapticUnitTest::g_normalAudioHapticPlayer->SetAudioHapticPlayerCallback(callback);
    EXPECT_NE(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetAudioHapticPlayerCallback API
 * @tc.number: AudioHapticPlayer_SetAudioHapticPlayerCallback_002
 * @tc.desc  : Test AudioHapticPlayer SetAudioHapticPlayerCallback interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetAudioHapticPlayerCallback_002, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer, nullptr);

    std::shared_ptr<AudioHapticPlayerCallback> callback = nullptr;
    int32_t result = AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer->SetAudioHapticPlayerCallback(callback);
    EXPECT_NE(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer GetAudioCurrentTime API
 * @tc.number: AudioHapticPlayer_GetAudioCurrentTime_001
 * @tc.desc  : Test AudioHapticPlayer GetAudioCurrentTime interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_GetAudioCurrentTime_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_normalAudioHapticPlayer, nullptr);

    int32_t result = AudioHapticUnitTest::g_normalAudioHapticPlayer->GetAudioCurrentTime();
    EXPECT_EQ(-1, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer GetAudioCurrentTime API
 * @tc.number: AudioHapticPlayer_GetAudioCurrentTime_002
 * @tc.desc  : Test AudioHapticPlayer GetAudioCurrentTime interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_GetAudioCurrentTime_002, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer, nullptr);

    int32_t result = AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer->GetAudioCurrentTime();
    EXPECT_EQ(-1, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer Prepare API
 * @tc.number: AudioHapticPlayer_Prepare_001
 * @tc.desc  : Test AudioHapticPlayer Prepare interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_Prepare_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_normalAudioHapticPlayer, nullptr);

    int32_t result = MSERR_OK;
    result = AudioHapticUnitTest::g_normalAudioHapticPlayer->Prepare();
    if (result == MSERR_OPEN_FILE_FAILED || result == MSERR_UNSUPPORT_FILE) {
        // The source file is invalid or the path is inaccessible. Return directly.
        EXPECT_NE(MSERR_OK, result);
        return;
    }

    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer Prepare API
 * @tc.number: AudioHapticPlayer_Prepare_002
 * @tc.desc  : Test AudioHapticPlayer Prepare interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_Prepare_002, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer, nullptr);

    int32_t result = MSERR_OK;
    result = AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer->Prepare();
    if (result == MSERR_OPEN_FILE_FAILED || result == MSERR_UNSUPPORT_FILE) {
        // The source file is invalid or the path is inaccessible. Return directly.
        EXPECT_NE(MSERR_OK, result);
        return;
    }

    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer Prepare API
 * @tc.number: AudioHapticPlayer_Prepare_003
 * @tc.desc  : Test AudioHapticPlayer Prepare interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_Prepare_003, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_effectAudioHapticPlayer, nullptr);

    int32_t result = MSERR_OK;
    result = AudioHapticUnitTest::g_effectAudioHapticPlayer->Prepare();
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
    EXPECT_NE(AudioHapticUnitTest::g_normalAudioHapticPlayer, nullptr);

    int32_t result = MSERR_OK;
    result = AudioHapticUnitTest::g_normalAudioHapticPlayer->Prepare();
    if (result == MSERR_OPEN_FILE_FAILED || result == MSERR_UNSUPPORT_FILE) {
        // The source file is invalid or the path is inaccessible. Return directly.
        EXPECT_NE(MSERR_OK, result);
        return;
    }

    result = AudioHapticUnitTest::g_normalAudioHapticPlayer->Start();
    EXPECT_EQ(MSERR_OK, result);

    AudioHapticUnitTest::g_normalAudioHapticPlayer->Stop();
}

/**
 * @tc.name  : Test AudioHapticPlayer Start API
 * @tc.number: AudioHapticPlayer_Start_002
 * @tc.desc  : Test AudioHapticPlayer Start interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_Start_002, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer, nullptr);

    int32_t result = MSERR_OK;
    result = AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer->Prepare();
    if (result == MSERR_OPEN_FILE_FAILED || result == MSERR_UNSUPPORT_FILE) {
        // The source file is invalid or the path is inaccessible. Return directly.
        EXPECT_NE(MSERR_OK, result);
        return;
    }

    result = AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer->Start();
    EXPECT_EQ(MSERR_OK, result);

    AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer->Stop();
}

/**
 * @tc.name  : Test AudioHapticPlayer Start API
 * @tc.number: AudioHapticPlayer_Start_003
 * @tc.desc  : Test AudioHapticPlayer Start interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_Start_003, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_effectAudioHapticPlayer, nullptr);

    int32_t result = MSERR_OK;
    result = AudioHapticUnitTest::g_effectAudioHapticPlayer->Prepare();
    if (result == MSERR_OPEN_FILE_FAILED || result == MSERR_UNSUPPORT_FILE) {
        // The source file is invalid or the path is inaccessible. Return directly.
        EXPECT_NE(MSERR_OK, result);
        return;
    }

    result = AudioHapticUnitTest::g_effectAudioHapticPlayer->Start();
    EXPECT_EQ(MSERR_OK, result);

    AudioHapticUnitTest::g_effectAudioHapticPlayer->Stop();
}

/**
 * @tc.name  : Test AudioHapticPlayer Stop API
 * @tc.number: AudioHapticPlayer_Stop_001
 * @tc.desc  : Test AudioHapticPlayer Stop interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_Stop_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_normalAudioHapticPlayer, nullptr);

    int32_t result = MSERR_OK;
    result = AudioHapticUnitTest::g_normalAudioHapticPlayer->Prepare();
    if (result == MSERR_OPEN_FILE_FAILED || result == MSERR_UNSUPPORT_FILE) {
        // The source file is invalid or the path is inaccessible. Return directly.
        EXPECT_NE(MSERR_OK, result);
        return;
    }
    AudioHapticUnitTest::g_normalAudioHapticPlayer->Start();

    result = AudioHapticUnitTest::g_normalAudioHapticPlayer->Stop();
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer Stop API
 * @tc.number: AudioHapticPlayer_Stop_002
 * @tc.desc  : Test AudioHapticPlayer Stop interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_Stop_002, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer, nullptr);

    int32_t result = MSERR_OK;
    result = AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer->Prepare();
    if (result == MSERR_OPEN_FILE_FAILED || result == MSERR_UNSUPPORT_FILE) {
        // The source file is invalid or the path is inaccessible. Return directly.
        EXPECT_NE(MSERR_OK, result);
        return;
    }
    AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer->Start();

    result = AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer->Stop();
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer Stop API
 * @tc.number: AudioHapticPlayer_Stop_003
 * @tc.desc  : Test AudioHapticPlayer Stop interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_Stop_003, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_effectAudioHapticPlayer, nullptr);

    int32_t result = MSERR_OK;
    result = AudioHapticUnitTest::g_effectAudioHapticPlayer->Prepare();
    if (result == MSERR_OPEN_FILE_FAILED || result == MSERR_UNSUPPORT_FILE) {
        // The source file is invalid or the path is inaccessible. Return directly.
        EXPECT_NE(MSERR_OK, result);
        return;
    }
    AudioHapticUnitTest::g_effectAudioHapticPlayer->Start();

    result = AudioHapticUnitTest::g_effectAudioHapticPlayer->Stop();
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer Release API
 * @tc.number: AudioHapticPlayer_Release_001
 * @tc.desc  : Test AudioHapticPlayer Release interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_Release_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_normalAudioHapticPlayer, nullptr);

    int32_t result = MSERR_OK;
    result = AudioHapticUnitTest::g_normalAudioHapticPlayer->Prepare();
    if (result == MSERR_OPEN_FILE_FAILED || result == MSERR_UNSUPPORT_FILE) {
        // The source file is invalid or the path is inaccessible. Return directly.
        EXPECT_NE(MSERR_OK, result);
        return;
    }
    AudioHapticUnitTest::g_normalAudioHapticPlayer->Start();
    AudioHapticUnitTest::g_normalAudioHapticPlayer->Stop();

    result = AudioHapticUnitTest::g_normalAudioHapticPlayer->Release();
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer Release API
 * @tc.number: AudioHapticPlayer_Release_002
 * @tc.desc  : Test AudioHapticPlayer Release interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_Release_002, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer, nullptr);

    int32_t result = MSERR_OK;
    result = AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer->Prepare();
    if (result == MSERR_OPEN_FILE_FAILED || result == MSERR_UNSUPPORT_FILE) {
        // The source file is invalid or the path is inaccessible. Return directly.
        EXPECT_NE(MSERR_OK, result);
        return;
    }
    AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer->Start();
    AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer->Stop();

    result = AudioHapticUnitTest::g_lowLatencyAudioHapticPlayer->Release();
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer Release API
 * @tc.number: AudioHapticPlayer_Release_003
 * @tc.desc  : Test AudioHapticPlayer Release interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_Release_003, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_effectAudioHapticPlayer, nullptr);

    int32_t result = MSERR_OK;
    result = AudioHapticUnitTest::g_effectAudioHapticPlayer->Prepare();
    if (result == MSERR_OPEN_FILE_FAILED || result == MSERR_UNSUPPORT_FILE) {
        // The source file is invalid or the path is inaccessible. Return directly.
        EXPECT_NE(MSERR_OK, result);
        return;
    }
    AudioHapticUnitTest::g_effectAudioHapticPlayer->Start();
    AudioHapticUnitTest::g_effectAudioHapticPlayer->Stop();

    result = AudioHapticUnitTest::g_effectAudioHapticPlayer->Release();
    EXPECT_EQ(MSERR_OK, result);
}

/**
 * @tc.name  : Test AudioHapticPlayer SetHapticIntensity API
 * @tc.number: AudioHapticPlayer_SetHapticIntensity_001
 * @tc.desc  : Test AudioHapticPlayer SetHapticIntensity interface
 */
HWTEST_F(AudioHapticUnitTest, AudioHapticPlayer_SetHapticIntensity_001, TestSize.Level1)
{
    EXPECT_NE(AudioHapticUnitTest::g_audioHapticManager, nullptr);

    g_effectSourceId = g_audioHapticManager->RegisterSourceWithEffectId(AUDIO_TEST_URI, HAPTIC_TEST_EFFECT_ID);
    EXPECT_NE(-1, g_effectSourceId);
    AudioLatencyMode latencyMode = AudioLatencyMode::AUDIO_LATENCY_MODE_FAST;
    g_audioHapticManager->SetAudioLatencyMode(g_effectSourceId, latencyMode);
    AudioStandard::StreamUsage streamUsage = AudioStandard::StreamUsage::STREAM_USAGE_NOTIFICATION;
    g_audioHapticManager->SetStreamUsage(g_effectSourceId, streamUsage);
    AudioHapticPlayerOptions options;
    options.muteAudio = false;
    options.muteHaptics = false;
    g_effectAudioHapticPlayer = g_audioHapticManager->CreatePlayer(g_effectSourceId, options);
    EXPECT_NE(nullptr, g_effectAudioHapticPlayer);

    int32_t result = g_effectAudioHapticPlayer->Prepare();
    EXPECT_EQ(MSERR_OK, result);

    g_effectAudioHapticPlayer->SetVolume(1.0f);
    g_effectAudioHapticPlayer->SetHapticIntensity(100.0f);
    result = g_effectAudioHapticPlayer->Start();
    EXPECT_EQ(MSERR_OK, result);

    sleep(1);
    g_effectAudioHapticPlayer->Stop();
    g_effectAudioHapticPlayer->SetVolume(0.75f);
    g_effectAudioHapticPlayer->SetHapticIntensity(75.0f);
    result = g_effectAudioHapticPlayer->Start();
    EXPECT_EQ(MSERR_OK, result);

    sleep(1);
    g_effectAudioHapticPlayer->Stop();
    g_effectAudioHapticPlayer->SetVolume(0.5f);
    g_effectAudioHapticPlayer->SetHapticIntensity(50.0f);
    result = g_effectAudioHapticPlayer->Start();
    EXPECT_EQ(MSERR_OK, result);

    sleep(1);
    g_effectAudioHapticPlayer->Stop();
    g_effectAudioHapticPlayer->SetVolume(0.25f);
    g_effectAudioHapticPlayer->SetHapticIntensity(25.0f);
    result = g_effectAudioHapticPlayer->Start();
    EXPECT_EQ(MSERR_OK, result);

    sleep(1);
    g_effectAudioHapticPlayer->Stop();
    g_effectAudioHapticPlayer->SetVolume(0.01f);
    g_effectAudioHapticPlayer->SetHapticIntensity(1.0f);
    result = g_effectAudioHapticPlayer->Start();
    EXPECT_EQ(MSERR_OK, result);

    sleep(1);
    result = g_effectAudioHapticPlayer->Stop();
    EXPECT_EQ(MSERR_OK, result);
    result = g_effectAudioHapticPlayer->Release();
    EXPECT_EQ(MSERR_OK, result);
}
} // namespace Media
} // namespace OHOS
