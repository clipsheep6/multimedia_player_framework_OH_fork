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

#include <fcntl.h>
#include <iostream>
#include <string>
#include "media_errors.h"
#include "screen_capture_unit_test.h"
#include "ability_manager_client.h"
#include "accesstoken_kit.h"
#include "token_setproc.h"
#include <nativetoken_kit.h>

using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace std;
using namespace OHOS::Rosen;
using namespace OHOS::Media::ScreenCaptureTestParam;

namespace OHOS {
namespace Media {
/**
 * @tc.name: screen_capture_exclude_content_001
 * @tc.desc: screen capture exclude window content test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_001, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_001 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    aFlag = 0;
    vFlag = 0;
    bool isMicrophone = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(10);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_001 before");
}

/**
 * @tc.name: screen_capture_exclude_content_002
 * @tc.desc: screen capture exclude audio content test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_002, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_002 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    aFlag = 0;
    vFlag = 0;
    bool isMicrophone = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(10);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_002 before");
}

/**
 * @tc.name: screen_capture_exclude_content_003
 * @tc.desc: screen capture exclude audio content test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_003, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_003 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    aFlag = 0;
    vFlag = 0;
    bool isMicrophone = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(10);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_003 before");
}

/**
 * @tc.name: screen_capture_exclude_content_004
 * @tc.desc: screen capture exclude window and audio content test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_004, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_004 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    aFlag = 0;
    vFlag = 0;
    bool isMicrophone = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(10);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_004 before");
}

/**
 * @tc.name: screen_capture_exclude_content_005
 * @tc.desc: screen capture exclude audio content
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_005, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_005 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    aFlag = 0;
    vFlag = 0;
    bool isMicrophone = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(10);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_005 before");
}

/**
 * @tc.name: screen_capture_exclude_content_006
 * @tc.desc: screen capture exclude audio content
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_006, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_006 before");
    SetConfig(config_);
    AudioCaptureInfo micCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::SOURCE_DEFAULT
    };
    config_.audioInfo.micCapInfo = micCapInfo;
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 0,
        .audioChannels = 0,
        .audioSource = AudioCaptureSourceType::ALL_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    aFlag = 0;
    vFlag = 0;
    bool isMicrophone = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(10);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_006 before");
}

/**
 * @tc.name: screen_capture_exclude_content_007
 * @tc.desc: screen capture exclude audio content
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_007, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_007 before");
    SetConfig(config_);
    AudioCaptureInfo micCapInfo = {
        .audioSampleRate = 0,
        .audioChannels = 0,
        .audioSource = AudioCaptureSourceType::SOURCE_DEFAULT
    };
    config_.audioInfo.micCapInfo = micCapInfo;
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 0,
        .audioChannels = 0,
        .audioSource = AudioCaptureSourceType::ALL_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    aFlag = 0;
    vFlag = 0;
    bool isMicrophone = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(10);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_007 before");
}

/**
 * @tc.name: screen_capture_exclude_content_008
 * @tc.desc: screen capture exclude window and audio content test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_008, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_008 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    aFlag = 0;
    vFlag = 0;
    bool isMicrophone = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(10);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_008 before");
}

/**
 * @tc.name: screen_capture_exclude_content_009
 * @tc.desc: screen capture exclude audio content
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_009, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_009 before");
    SetConfig(config_);
    AudioCaptureInfo micCapInfo = {
        .audioSampleRate = 0,
        .audioChannels = 0,
        .audioSource = AudioCaptureSourceType::SOURCE_DEFAULT
    };
    config_.audioInfo.micCapInfo = micCapInfo;
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 0,
        .audioChannels = 0,
        .audioSource = AudioCaptureSourceType::ALL_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    aFlag = 0;
    vFlag = 0;
    bool isMicrophone = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(10);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_009 before");
}

/**
 * @tc.name: screen_capture_exclude_content_010
 * @tc.desc: screen capture exclude window content test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_010, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_010 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    aFlag = 0;
    vFlag = 0;
    bool isMicrophone = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds1 = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds1[0], static_cast<int32_t>(windowIds1.size())));
    vector<int> windowIds2 = {2, 4, 6};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds2[0], static_cast<int32_t>(windowIds2.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(10);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_010 before");
}

/**
 * @tc.name: screen_capture_exclude_content_011
 * @tc.desc: screen capture exclude window content test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_011, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_011 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    aFlag = 0;
    vFlag = 0;
    bool isMicrophone = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds1 = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds1[0], static_cast<int32_t>(windowIds1.size())));
    vector<int> windowIds2 = {1, 4, 6};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds2[0], static_cast<int32_t>(windowIds2.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(10);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_011 before");
}

/**
 * @tc.name: screen_capture_exclude_content_012
 * @tc.desc: screen capture exclude window content test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_012, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_012 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    aFlag = 0;
    vFlag = 0;
    bool isMicrophone = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds1 = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds1[0], static_cast<int32_t>(windowIds1.size())));
    vector<int> windowIds2 = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds2[0], static_cast<int32_t>(windowIds2.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(10);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_012 before");
}

/**
 * @tc.name: screen_capture_exclude_content_save_file_01
 * @tc.desc: screen capture exclude window content test with save file
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_save_file_01, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_save_file_01 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_exclude_content_save_file_01.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo micCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::SOURCE_DEFAULT
    };
    config_.audioInfo.micCapInfo = micCapInfo;
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::ALL_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_save_file_01 after");
}

/**
 * @tc.name: screen_capture_exclude_content_save_file_02
 * @tc.desc: screen capture exclude audio content test with save file
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_save_file_02, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_save_file_02 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_exclude_content_save_file_02.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo micCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::SOURCE_DEFAULT
    };
    config_.audioInfo.micCapInfo = micCapInfo;
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::ALL_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_save_file_02 after");
}

/**
 * @tc.name: screen_capture_exclude_content_save_file_03
 * @tc.desc: screen capture exclude audio content test with save file
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_save_file_03, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_save_file_03 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_exclude_content_save_file_03.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo micCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::SOURCE_DEFAULT
    };
    config_.audioInfo.micCapInfo = micCapInfo;
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::ALL_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_save_file_03 after");
}

/**
 * @tc.name: screen_capture_exclude_content_save_file_04
 * @tc.desc: screen capture exclude window and audio content test with save file
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_save_file_04, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_save_file_04 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_exclude_content_save_file_04.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo micCapInfo = {
        .audioSampleRate = 0,
        .audioChannels = 0,
        .audioSource = AudioCaptureSourceType::SOURCE_DEFAULT
    };
    config_.audioInfo.micCapInfo = micCapInfo;
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::ALL_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_save_file_04 after");
}

/**
 * @tc.name: screen_capture_exclude_content_save_file_05
 * @tc.desc: screen capture exclude window and audio content test with save file and without innerCap audio
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_save_file_05, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_save_file_05 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_exclude_content_save_file_05.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo micCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::SOURCE_DEFAULT
    };
    config_.audioInfo.micCapInfo = micCapInfo;
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::ALL_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_save_file_05 after");
}

/**
 * @tc.name: screen_capture_exclude_content_save_file_06
 * @tc.desc: screen capture exclude window content test with save file
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_save_file_06, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_save_file_06 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_exclude_content_save_file_06.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo micCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::SOURCE_DEFAULT
    };
    config_.audioInfo.micCapInfo = micCapInfo;
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::ALL_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds1 = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds1[0], static_cast<int32_t>(windowIds1.size())));
    vector<int> windowIds2 = {2, 4, 6};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds2[0], static_cast<int32_t>(windowIds2.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_save_file_06 after");
}

/**
 * @tc.name: screen_capture_exclude_content_with_surface_01
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_with_surface_01, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_01 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoFrameWidth = 720;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 1280;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_01 CreateRecorder");
    std::shared_ptr<Recorder> recorder = nullptr;
    recorder = RecorderFactory::CreateRecorder();
    int32_t videoSourceId = 0;
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSource(VIDEO_SOURCE_SURFACE_RGBA, videoSourceId));
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFormat(OutputFormatType::FORMAT_MPEG_4));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncoder(videoSourceId, VideoCodecFormat::H264));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSize(videoSourceId, 720, 1280));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoFrameRate(videoSourceId, 30));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncodingBitRate(videoSourceId, 2000000));
    OpenFileFd("screen_capture_exclude_content_with_surface_01.mp4");
    ASSERT_TRUE(outputFd_ >= 0);
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFile(outputFd_));
    EXPECT_EQ(MSERR_OK, recorder->Prepare());
    sptr<OHOS::Surface> consumer = recorder->GetSurface(videoSourceId);
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCaptureWithSurface(consumer));
    EXPECT_EQ(MSERR_OK, recorder->Start());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_01 recorder Start");
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder->Stop(true));
    EXPECT_EQ(MSERR_OK, recorder->Reset());
    EXPECT_EQ(MSERR_OK, recorder->Release());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_01 after");
}

/**
 * @tc.name: screen_capture_exclude_content_with_surface_02
 * @tc.desc: do screencapture without audio and exclude window and audio content
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_with_surface_02, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_02 before");
    SetConfig(config_);
    config_.audioInfo.micCapInfo.audioSampleRate = 0;
    config_.audioInfo.micCapInfo.audioChannels = 0;
    config_.audioInfo.micCapInfo.audioSource = AudioCaptureSourceType::SOURCE_DEFAULT;
    config_.audioInfo.innerCapInfo.audioSampleRate = 0;
    config_.audioInfo.innerCapInfo.audioChannels = 0;
    config_.audioInfo.innerCapInfo.audioSource = AudioCaptureSourceType::ALL_PLAYBACK;
    config_.videoInfo.videoCapInfo.videoFrameWidth = 720;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 1280;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_02 CreateRecorder");
    std::shared_ptr<Recorder> recorder = nullptr;
    recorder = RecorderFactory::CreateRecorder();
    int32_t videoSourceId = 0;
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSource(VIDEO_SOURCE_SURFACE_RGBA, videoSourceId));
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFormat(OutputFormatType::FORMAT_MPEG_4));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncoder(videoSourceId, VideoCodecFormat::H264));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSize(videoSourceId, 720, 1280));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoFrameRate(videoSourceId, 30));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncodingBitRate(videoSourceId, 2000000));
    OpenFileFd("screen_capture_exclude_content_with_surface_02.mp4");
    ASSERT_TRUE(outputFd_ >= 0);
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFile(outputFd_));
    EXPECT_EQ(MSERR_OK, recorder->Prepare());
    sptr<OHOS::Surface> consumer = recorder->GetSurface(videoSourceId);
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCaptureWithSurface(consumer));
    EXPECT_EQ(MSERR_OK, recorder->Start());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_02 recorder Start");
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder->Stop(true));
    EXPECT_EQ(MSERR_OK, recorder->Reset());
    EXPECT_EQ(MSERR_OK, recorder->Release());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_02 after");
}

/**
 * @tc.name: screen_capture_exclude_content_with_surface_03
 * @tc.desc: do screencapture exclude windows 2 times
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_with_surface_03, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_03 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoFrameWidth = 720;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 1280;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_03 CreateRecorder");
    std::shared_ptr<Recorder> recorder = nullptr;
    recorder = RecorderFactory::CreateRecorder();
    int32_t videoSourceId = 0;
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSource(VIDEO_SOURCE_SURFACE_RGBA, videoSourceId));
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFormat(OutputFormatType::FORMAT_MPEG_4));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncoder(videoSourceId, VideoCodecFormat::H264));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSize(videoSourceId, 720, 1280));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoFrameRate(videoSourceId, 30));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncodingBitRate(videoSourceId, 2000000));
    OpenFileFd("screen_capture_exclude_content_with_surface_03.mp4");
    ASSERT_TRUE(outputFd_ >= 0);
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFile(outputFd_));
    EXPECT_EQ(MSERR_OK, recorder->Prepare());
    sptr<OHOS::Surface> consumer = recorder->GetSurface(videoSourceId);
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds1 = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds1[0], static_cast<int32_t>(windowIds1.size())));
    vector<int> windowIds2 = {2, 4, 6};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds2[0], static_cast<int32_t>(windowIds2.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCaptureWithSurface(consumer));
    EXPECT_EQ(MSERR_OK, recorder->Start());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_03 recorder Start");
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder->Stop(true));
    EXPECT_EQ(MSERR_OK, recorder->Reset());
    EXPECT_EQ(MSERR_OK, recorder->Release());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_03 after");
}

/**
 * @tc.name: screen_capture_exclude_content_after_start_001
 * @tc.desc: screen capture exclude window content test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_after_start_001, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_after_start_001 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    aFlag = 0;
    vFlag = 0;
    bool isMicrophone = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    sleep(10);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_after_start_001 after");
}

/**
 * @tc.name: screen_capture_exclude_content_after_start_002
 * @tc.desc: screen capture exclude audio content test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_after_start_002, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_after_start_002 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    aFlag = 0;
    vFlag = 0;
    bool isMicrophone = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    sleep(10);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_after_start_002 after");
}

/**
 * @tc.name: screen_capture_exclude_content_after_start_003
 * @tc.desc: screen capture exclude audio content test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_after_start_003, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_after_start_003 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    aFlag = 0;
    vFlag = 0;
    bool isMicrophone = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    sleep(10);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_after_start_003 after");
}

/**
 * @tc.name: screen_capture_exclude_content_after_start_004
 * @tc.desc: screen capture exclude window content test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_after_start_004, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_after_start_004 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    aFlag = 0;
    vFlag = 0;
    bool isMicrophone = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    sleep(10);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_after_start_004 after");
}

/**
 * @tc.name: screen_capture_exclude_content_video_size_0001
 * @tc.desc: screen capture with 160x160
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_video_size_0001, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_video_size_0001 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoFrameWidth = 160;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 160;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    std::string name = SCREEN_CAPTURE_ROOT_DIR + "screen_capture_exclude_content_video_size_0001.yuv";
    vFile = fopen(name.c_str(), "w+");
    if (vFile == nullptr) {
        cout << "vFile video open failed, " << strerror(errno) << endl;
    }

    aFlag = 1;
    vFlag = 1;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_video_size_0001 after");
}

/**
 * @tc.name: screen_capture_exclude_content_video_size_0002
 * @tc.desc: screen capture with 640x480
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_video_size_0002, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_video_size_0002 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoFrameWidth = 640;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 480;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    std::string name = SCREEN_CAPTURE_ROOT_DIR + "screen_capture_exclude_content_video_size_0002.yuv";
    vFile = fopen(name.c_str(), "w+");
    if (vFile == nullptr) {
        cout << "vFile video open failed, " << strerror(errno) << endl;
    }

    aFlag = 1;
    vFlag = 1;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_video_size_0002 after");
}

/**
 * @tc.name: screen_capture_exclude_content_video_size_0003
 * @tc.desc: screen capture with 160x160
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_video_size_0003, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_video_size_0003 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoFrameWidth = 160;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 160;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    std::string name = SCREEN_CAPTURE_ROOT_DIR + "screen_capture_exclude_content_video_size_0003.yuv";
    vFile = fopen(name.c_str(), "w+");
    if (vFile == nullptr) {
        cout << "vFile video open failed, " << strerror(errno) << endl;
    }

    aFlag = 1;
    vFlag = 1;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_video_size_0003 after");
}

/**
 * @tc.name: screen_capture_exclude_content_video_size_0004
 * @tc.desc: screen capture with 160x160
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_video_size_0004, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_video_size_0004 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoFrameWidth = 160;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 160;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    std::string name = SCREEN_CAPTURE_ROOT_DIR + "screen_capture_exclude_content_video_size_0004.yuv";
    vFile = fopen(name.c_str(), "w+");
    if (vFile == nullptr) {
        cout << "vFile video open failed, " << strerror(errno) << endl;
    }

    aFlag = 1;
    vFlag = 1;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_video_size_0004 after");
}

/**
 * @tc.name: screen_capture_exclude_content_video_size_0005
 * @tc.desc: screen capture with 160x160
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_video_size_0005, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_video_size_0005 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoFrameWidth = 160;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 160;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    std::string name = SCREEN_CAPTURE_ROOT_DIR + "screen_capture_exclude_content_video_size_0005.yuv";
    vFile = fopen(name.c_str(), "w+");
    if (vFile == nullptr) {
        cout << "vFile video open failed, " << strerror(errno) << endl;
    }

    aFlag = 1;
    vFlag = 1;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds1 = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds1[0], static_cast<int32_t>(windowIds1.size())));
    vector<int> windowIds2 = {2, 4, 6};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds2[0], static_cast<int32_t>(windowIds2.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_video_size_0005 after");
}

/**
 * @tc.name: screen_capture_exclude_content_from_display_001
 * @tc.desc: screen capture exclude content from display
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_from_display_001, TestSize.Level0)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_from_display_001 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    sptr<Display> display = DisplayManager::GetInstance().GetDefaultDisplaySync();
    ASSERT_NE(display, nullptr);
    cout << "get displayinfo: " << endl;
    cout << "width: " << display->GetWidth() << "; height: " << display->GetHeight() << "; density: " <<
        display->GetDpi() << "; refreshRate: " << display->GetRefreshRate() << endl;

    config_.videoInfo.videoCapInfo.videoFrameWidth = display->GetWidth();
    config_.videoInfo.videoCapInfo.videoFrameHeight = display->GetHeight();

    aFlag = 1;
    vFlag = 1;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_from_display_001 after");
}

/**
 * @tc.name: screen_capture_exclude_content_from_display_002
 * @tc.desc: screen capture exclude content from display
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_from_display_002, TestSize.Level0)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_from_display_002 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    sptr<Display> display = DisplayManager::GetInstance().GetDefaultDisplaySync();
    ASSERT_NE(display, nullptr);
    cout << "get displayinfo: " << endl;
    cout << "width: " << display->GetWidth() << "; height: " << display->GetHeight() << "; density: " <<
        display->GetDpi() << "; refreshRate: " << display->GetRefreshRate() << endl;

    config_.videoInfo.videoCapInfo.videoFrameWidth = display->GetWidth();
    config_.videoInfo.videoCapInfo.videoFrameHeight = display->GetHeight();

    aFlag = 1;
    vFlag = 1;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_from_display_002 after");
}

/**
 * @tc.name: screen_capture_exclude_content_from_display_003
 * @tc.desc: screen capture exclude content from display
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_from_display_003, TestSize.Level0)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_from_display_003 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    sptr<Display> display = DisplayManager::GetInstance().GetDefaultDisplaySync();
    ASSERT_NE(display, nullptr);
    cout << "get displayinfo: " << endl;
    cout << "width: " << display->GetWidth() << "; height: " << display->GetHeight() << "; density: " <<
        display->GetDpi() << "; refreshRate: " << display->GetRefreshRate() << endl;

    config_.videoInfo.videoCapInfo.videoFrameWidth = display->GetWidth();
    config_.videoInfo.videoCapInfo.videoFrameHeight = display->GetHeight();

    aFlag = 1;
    vFlag = 1;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_from_display_003 after");
}

/**
 * @tc.name: screen_capture_exclude_content_from_display_004
 * @tc.desc: screen capture exclude content from display
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_from_display_004, TestSize.Level0)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_from_display_004 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    sptr<Display> display = DisplayManager::GetInstance().GetDefaultDisplaySync();
    ASSERT_NE(display, nullptr);
    cout << "get displayinfo: " << endl;
    cout << "width: " << display->GetWidth() << "; height: " << display->GetHeight() << "; density: " <<
        display->GetDpi() << "; refreshRate: " << display->GetRefreshRate() << endl;

    config_.videoInfo.videoCapInfo.videoFrameWidth = display->GetWidth();
    config_.videoInfo.videoCapInfo.videoFrameHeight = display->GetHeight();

    aFlag = 1;
    vFlag = 1;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_from_display_004 after");
}

/**
 * @tc.name: screen_capture_exclude_content_from_display_005
 * @tc.desc: screen capture exclude content from display
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_from_display_005, TestSize.Level0)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_from_display_005 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    sptr<Display> display = DisplayManager::GetInstance().GetDefaultDisplaySync();
    ASSERT_NE(display, nullptr);
    cout << "get displayinfo: " << endl;
    cout << "width: " << display->GetWidth() << "; height: " << display->GetHeight() << "; density: " <<
        display->GetDpi() << "; refreshRate: " << display->GetRefreshRate() << endl;

    config_.videoInfo.videoCapInfo.videoFrameWidth = display->GetWidth();
    config_.videoInfo.videoCapInfo.videoFrameHeight = display->GetHeight();

    aFlag = 1;
    vFlag = 1;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds1 = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds1[0], static_cast<int32_t>(windowIds1.size())));
    vector<int> windowIds2 = {2, 4, 6};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds2[0], static_cast<int32_t>(windowIds2.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_from_display_005 after");
}

/**
 * @tc.name: screen_capture_exclude_content_buffertest_001
 * @tc.desc: screen capture buffer test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_buffertest_001, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_buffertest_001 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    int index = 200;
    int index_video_frame = 0;
    audioLoop_ = std::make_unique<std::thread>(&ScreenCaptureUnitTest::AudioLoopWithoutRelease, this);
    while (index) {
        int32_t fence = 0;
        int64_t timestamp = 0;
        OHOS::Rect damage;
        sptr<OHOS::SurfaceBuffer> surfacebuffer = screenCapture_->AcquireVideoBuffer(fence, timestamp, damage);
        if (surfacebuffer != nullptr) {
            int32_t length = surfacebuffer->GetSize();
            cout << "index video:" << index_video_frame++ << "; AcquireVideoBuffer, videoBufferLen:" <<
                surfacebuffer->GetSize() << ", timestamp:" << timestamp << ", size:"<< length << endl;
        } else {
            cout << "AcquireVideoBuffer failed" << endl;
        }
        index--;
    }
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    if (audioLoop_ != nullptr && audioLoop_->joinable()) {
        audioLoop_->join();
        audioLoop_.reset();
        audioLoop_ = nullptr;
    }
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_buffertest_001 after");
}

/**
 * @tc.name: screen_capture_exclude_content_buffertest_002
 * @tc.desc: screen capture buffer test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_buffertest_002, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_buffertest_002 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    int index = 200;
    int index_video_frame = 0;
    audioLoop_ = std::make_unique<std::thread>(&ScreenCaptureUnitTest::AudioLoopWithoutRelease, this);
    while (index) {
        int32_t fence = 0;
        int64_t timestamp = 0;
        OHOS::Rect damage;
        sptr<OHOS::SurfaceBuffer> surfacebuffer = screenCapture_->AcquireVideoBuffer(fence, timestamp, damage);
        if (surfacebuffer != nullptr) {
            int32_t length = surfacebuffer->GetSize();
            cout << "index video:" << index_video_frame++ << "; AcquireVideoBuffer, videoBufferLen:" <<
                surfacebuffer->GetSize() << ", timestamp:" << timestamp << ", size:"<< length << endl;
        } else {
            cout << "AcquireVideoBuffer failed" << endl;
        }
        index--;
    }
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    if (audioLoop_ != nullptr && audioLoop_->joinable()) {
        audioLoop_->join();
        audioLoop_.reset();
        audioLoop_ = nullptr;
    }
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_buffertest_002 after");
}

/**
 * @tc.name: screen_capture_exclude_content_buffertest_003
 * @tc.desc: screen capture buffer test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_buffertest_003, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_buffertest_003 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    int index = 200;
    int index_video_frame = 0;
    audioLoop_ = std::make_unique<std::thread>(&ScreenCaptureUnitTest::AudioLoopWithoutRelease, this);
    while (index) {
        int32_t fence = 0;
        int64_t timestamp = 0;
        OHOS::Rect damage;
        sptr<OHOS::SurfaceBuffer> surfacebuffer = screenCapture_->AcquireVideoBuffer(fence, timestamp, damage);
        if (surfacebuffer != nullptr) {
            int32_t length = surfacebuffer->GetSize();
            cout << "index video:" << index_video_frame++ << "; AcquireVideoBuffer, videoBufferLen:" <<
                surfacebuffer->GetSize() << ", timestamp:" << timestamp << ", size:"<< length << endl;
        } else {
            cout << "AcquireVideoBuffer failed" << endl;
        }
        index--;
    }
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    if (audioLoop_ != nullptr && audioLoop_->joinable()) {
        audioLoop_->join();
        audioLoop_.reset();
        audioLoop_ = nullptr;
    }
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_buffertest_003 after");
}

/**
 * @tc.name: screen_capture_exclude_content_buffertest_004
 * @tc.desc: screen capture buffer test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_buffertest_004, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_buffertest_004 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    int index = 200;
    int index_video_frame = 0;
    audioLoop_ = std::make_unique<std::thread>(&ScreenCaptureUnitTest::AudioLoopWithoutRelease, this);
    while (index) {
        int32_t fence = 0;
        int64_t timestamp = 0;
        OHOS::Rect damage;
        sptr<OHOS::SurfaceBuffer> surfacebuffer = screenCapture_->AcquireVideoBuffer(fence, timestamp, damage);
        if (surfacebuffer != nullptr) {
            int32_t length = surfacebuffer->GetSize();
            cout << "index video:" << index_video_frame++ << "; AcquireVideoBuffer, videoBufferLen:" <<
                surfacebuffer->GetSize() << ", timestamp:" << timestamp << ", size:"<< length << endl;
        } else {
            cout << "AcquireVideoBuffer failed" << endl;
        }
        index--;
    }
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    if (audioLoop_ != nullptr && audioLoop_->joinable()) {
        audioLoop_->join();
        audioLoop_.reset();
        audioLoop_ = nullptr;
    }
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_buffertest_004 after");
}

/**
 * @tc.name: screen_capture_exclude_content_buffertest_005
 * @tc.desc: screen capture buffer test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_buffertest_005, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_buffertest_005 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds1 = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds1[0], static_cast<int32_t>(windowIds1.size())));
    vector<int> windowIds2 = {2, 4, 6};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds2[0], static_cast<int32_t>(windowIds2.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    int index = 200;
    int index_video_frame = 0;
    audioLoop_ = std::make_unique<std::thread>(&ScreenCaptureUnitTest::AudioLoopWithoutRelease, this);
    while (index) {
        int32_t fence = 0;
        int64_t timestamp = 0;
        OHOS::Rect damage;
        sptr<OHOS::SurfaceBuffer> surfacebuffer = screenCapture_->AcquireVideoBuffer(fence, timestamp, damage);
        if (surfacebuffer != nullptr) {
            int32_t length = surfacebuffer->GetSize();
            cout << "index video:" << index_video_frame++ << "; AcquireVideoBuffer, videoBufferLen:" <<
                surfacebuffer->GetSize() << ", timestamp:" << timestamp << ", size:"<< length << endl;
        } else {
            cout << "AcquireVideoBuffer failed" << endl;
        }
        index--;
    }
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    if (audioLoop_ != nullptr && audioLoop_->joinable()) {
        audioLoop_->join();
        audioLoop_.reset();
        audioLoop_ = nullptr;
    }
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_buffertest_005 after");
}

/**
 * @tc.name: screen_capture_exclude_content_with_surface_cb_001
 * @tc.desc: do screencapture capture surface mode, capture screen, inner audio and mic audio, exclude some windows
 * @tc.type: FUNC
 * @tc.require: play something audio resource to get inner audio
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_with_surface_cb_001, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_001 before");
    SetConfig(config_);
    config_.audioInfo.innerCapInfo.audioSampleRate = 16000;
    config_.audioInfo.innerCapInfo.audioChannels = 2;
    config_.audioInfo.innerCapInfo.audioSource = AudioCaptureSourceType::ALL_PLAYBACK;
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_001 CreateRecorder");
    std::shared_ptr<Recorder> recorder = nullptr;
    recorder = RecorderFactory::CreateRecorder();
    int32_t videoSourceId = 0;
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSource(VIDEO_SOURCE_SURFACE_RGBA, videoSourceId));
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFormat(OutputFormatType::FORMAT_MPEG_4));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncoder(videoSourceId, VideoCodecFormat::H264));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSize(videoSourceId, 720, 1080));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoFrameRate(videoSourceId, 30));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncodingBitRate(videoSourceId, 2000000));
    OpenFileFd("screen_capture_exclude_content_with_surface_cb_001.mp4");
    ASSERT_TRUE(outputFd_ >= 0);
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFile(outputFd_));
    EXPECT_EQ(MSERR_OK, recorder->Prepare());
    sptr<OHOS::Surface> consumer = recorder->GetSurface(videoSourceId);

    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_);
    ASSERT_NE(nullptr, screenCaptureCb_);

    std::string name = "screen_capture_exclude_content_with_surface_cb_001";
    // track enalbed: inner: true, mic: true, video: true(surface mode)
    OpenFile(name, true, true, false);
    // check track aquire & release: inner: 1, mic: 1, video: 1
    screenCaptureCb_->InitCaptureTrackInfo(innerAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER);
    screenCaptureCb_->InitCaptureTrackInfo(micAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC);
    screenCaptureCb_->InitCaptureTrackInfo(videoFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_VIDEO);
    // callback enabled: errorCallback: true, dataCallback: true, stateChangeCallback: true
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_, true, true, true));

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCaptureWithSurface(consumer));
    EXPECT_EQ(MSERR_OK, recorder->Start());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_001 recorder Start");
    sleep(RECORDER_TIME);
    EXPECT_EQ(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED, screenCaptureCb_->GetScreenCaptureState());
    EXPECT_EQ(MSERR_OK, recorder->Stop(true));
    EXPECT_EQ(MSERR_OK, recorder->Reset());
    EXPECT_EQ(MSERR_OK, recorder->Release());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_001 after");
}

/**
 * @tc.name: screen_capture_exclude_content_with_surface_cb_002
 * @tc.desc: do screencapture capture surface mode, capture screen, inner audio and mic audio, exclude app audio
 * @tc.type: FUNC
 * @tc.require: play something audio resource to get inner audio
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_with_surface_cb_002, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_002 before");
    SetConfig(config_);
    config_.audioInfo.innerCapInfo.audioSampleRate = 16000;
    config_.audioInfo.innerCapInfo.audioChannels = 2;
    config_.audioInfo.innerCapInfo.audioSource = AudioCaptureSourceType::ALL_PLAYBACK;
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_002 CreateRecorder");
    std::shared_ptr<Recorder> recorder = nullptr;
    recorder = RecorderFactory::CreateRecorder();
    int32_t videoSourceId = 0;
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSource(VIDEO_SOURCE_SURFACE_RGBA, videoSourceId));
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFormat(OutputFormatType::FORMAT_MPEG_4));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncoder(videoSourceId, VideoCodecFormat::H264));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSize(videoSourceId, 720, 1080));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoFrameRate(videoSourceId, 30));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncodingBitRate(videoSourceId, 2000000));
    OpenFileFd("screen_capture_exclude_content_with_surface_cb_002.mp4");
    ASSERT_TRUE(outputFd_ >= 0);
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFile(outputFd_));
    EXPECT_EQ(MSERR_OK, recorder->Prepare());
    sptr<OHOS::Surface> consumer = recorder->GetSurface(videoSourceId);

    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_);
    ASSERT_NE(nullptr, screenCaptureCb_);

    std::string name = "screen_capture_exclude_content_with_surface_cb_002";
    // track enalbed: inner: true, mic: true, video: true(surface mode)
    OpenFile(name, true, true, false);
    // check track aquire & release: inner: 1, mic: 1, video: 1
    screenCaptureCb_->InitCaptureTrackInfo(innerAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER);
    screenCaptureCb_->InitCaptureTrackInfo(micAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC);
    screenCaptureCb_->InitCaptureTrackInfo(videoFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_VIDEO);
    // callback enabled: errorCallback: true, dataCallback: true, stateChangeCallback: true
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_, true, true, true));

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCaptureWithSurface(consumer));
    EXPECT_EQ(MSERR_OK, recorder->Start());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_002 recorder Start");
    sleep(RECORDER_TIME);
    EXPECT_EQ(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED, screenCaptureCb_->GetScreenCaptureState());
    EXPECT_EQ(MSERR_OK, recorder->Stop(true));
    EXPECT_EQ(MSERR_OK, recorder->Reset());
    EXPECT_EQ(MSERR_OK, recorder->Release());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_002 after");
}

/**
 * @tc.name: screen_capture_exclude_content_with_surface_cb_003
 * @tc.desc: surface mode, capture screen, inner audio and mic audio, exclude app and notice audio and sone windows
 * @tc.type: FUNC
 * @tc.require: play something audio resource to get inner audio
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_with_surface_cb_003, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_003 before");
    SetConfig(config_);
    config_.audioInfo.innerCapInfo.audioSampleRate = 16000;
    config_.audioInfo.innerCapInfo.audioChannels = 2;
    config_.audioInfo.innerCapInfo.audioSource = AudioCaptureSourceType::ALL_PLAYBACK;
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_003 CreateRecorder");
    std::shared_ptr<Recorder> recorder = nullptr;
    recorder = RecorderFactory::CreateRecorder();
    int32_t videoSourceId = 0;
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSource(VIDEO_SOURCE_SURFACE_RGBA, videoSourceId));
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFormat(OutputFormatType::FORMAT_MPEG_4));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncoder(videoSourceId, VideoCodecFormat::H264));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSize(videoSourceId, 720, 1080));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoFrameRate(videoSourceId, 30));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncodingBitRate(videoSourceId, 2000000));
    OpenFileFd("screen_capture_exclude_content_with_surface_cb_003.mp4");
    ASSERT_TRUE(outputFd_ >= 0);
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFile(outputFd_));
    EXPECT_EQ(MSERR_OK, recorder->Prepare());
    sptr<OHOS::Surface> consumer = recorder->GetSurface(videoSourceId);

    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_);
    ASSERT_NE(nullptr, screenCaptureCb_);

    std::string name = "screen_capture_exclude_content_with_surface_cb_003";
    // track enalbed: inner: true, mic: true, video: true(surface mode)
    OpenFile(name, true, true, false);
    // check track aquire & release: inner: 1, mic: 1, video: 1
    screenCaptureCb_->InitCaptureTrackInfo(innerAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER);
    screenCaptureCb_->InitCaptureTrackInfo(micAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC);
    screenCaptureCb_->InitCaptureTrackInfo(videoFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_VIDEO);
    // callback enabled: errorCallback: true, dataCallback: true, stateChangeCallback: true
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_, true, true, true));

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCaptureWithSurface(consumer));
    EXPECT_EQ(MSERR_OK, recorder->Start());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_003 recorder Start");
    sleep(RECORDER_TIME);
    EXPECT_EQ(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED, screenCaptureCb_->GetScreenCaptureState());
    EXPECT_EQ(MSERR_OK, recorder->Stop(true));
    EXPECT_EQ(MSERR_OK, recorder->Reset());
    EXPECT_EQ(MSERR_OK, recorder->Release());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_003 after");
}

/**
 * @tc.name: screen_capture_exclude_content_with_surface_cb_004
 * @tc.desc: surface mode, capture screen and inner audio, exclude app and notice audio and sone windows
 * @tc.type: FUNC
 * @tc.require: play something audio resource to get inner audio
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_with_surface_cb_004, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_004 before");
    SetConfig(config_);
    config_.audioInfo.micCapInfo.audioSampleRate = 0;
    config_.audioInfo.micCapInfo.audioChannels = 0;
    config_.audioInfo.micCapInfo.audioSource = AudioCaptureSourceType::SOURCE_DEFAULT;
    config_.audioInfo.innerCapInfo.audioSampleRate = 16000;
    config_.audioInfo.innerCapInfo.audioChannels = 2;
    config_.audioInfo.innerCapInfo.audioSource = AudioCaptureSourceType::ALL_PLAYBACK;
    screenCapture_->SetMicrophoneEnabled(true);
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_004 CreateRecorder");
    std::shared_ptr<Recorder> recorder = nullptr;
    recorder = RecorderFactory::CreateRecorder();
    int32_t videoSourceId = 0;
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSource(VIDEO_SOURCE_SURFACE_RGBA, videoSourceId));
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFormat(OutputFormatType::FORMAT_MPEG_4));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncoder(videoSourceId, VideoCodecFormat::H264));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSize(videoSourceId, 720, 1080));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoFrameRate(videoSourceId, 30));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncodingBitRate(videoSourceId, 2000000));
    OpenFileFd("screen_capture_exclude_content_with_surface_cb_004.mp4");
    ASSERT_TRUE(outputFd_ >= 0);
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFile(outputFd_));
    EXPECT_EQ(MSERR_OK, recorder->Prepare());
    sptr<OHOS::Surface> consumer = recorder->GetSurface(videoSourceId);
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_);
    ASSERT_NE(nullptr, screenCaptureCb_);
    std::string name = "screen_capture_exclude_content_with_surface_cb_004";
    // track enalbed: inner: true, mic: false, video: true(surface mode)
    OpenFile(name, true, false, false);
    // check track aquire & release: inner: 1, mic: 1, video: 1
    screenCaptureCb_->InitCaptureTrackInfo(innerAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER);
    screenCaptureCb_->InitCaptureTrackInfo(micAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC);
    screenCaptureCb_->InitCaptureTrackInfo(videoFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_VIDEO);
    // callback enabled: errorCallback: true, dataCallback: true, stateChangeCallback: true
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_, true, true, true));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCaptureWithSurface(consumer));
    EXPECT_EQ(MSERR_OK, recorder->Start());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_004 recorder Start");
    sleep(RECORDER_TIME);
    EXPECT_EQ(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED, screenCaptureCb_->GetScreenCaptureState());
    EXPECT_EQ(MSERR_OK, recorder->Stop(true));
    EXPECT_EQ(MSERR_OK, recorder->Reset());
    EXPECT_EQ(MSERR_OK, recorder->Release());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_004 after");
}

/**
 * @tc.name: screen_capture_exclude_content_with_surface_cb_005
 * @tc.desc: surface mode, capture screen and mic audio, exclude app and notice audio and sone windows
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_with_surface_cb_005, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_005 before");
    SetConfig(config_);
    screenCapture_->SetMicrophoneEnabled(true);
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_005 CreateRecorder");
    std::shared_ptr<Recorder> recorder = RecorderFactory::CreateRecorder();
    int32_t videoSourceId = 0;
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSource(VIDEO_SOURCE_SURFACE_RGBA, videoSourceId));
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFormat(OutputFormatType::FORMAT_MPEG_4));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncoder(videoSourceId, VideoCodecFormat::H264));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSize(videoSourceId, 720, 1080));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoFrameRate(videoSourceId, 30));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncodingBitRate(videoSourceId, 2000000));
    OpenFileFd("screen_capture_exclude_content_with_surface_cb_005.mp4");
    ASSERT_TRUE(outputFd_ >= 0);
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFile(outputFd_));
    EXPECT_EQ(MSERR_OK, recorder->Prepare());
    sptr<OHOS::Surface> consumer = recorder->GetSurface(videoSourceId);

    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_);
    ASSERT_NE(nullptr, screenCaptureCb_);

    std::string name = "screen_capture_exclude_content_with_surface_cb_005";
    // track enalbed: inner: false, mic: true, video: true(surface mode)
    OpenFile(name, false, true, false);
    // check track aquire & release: inner: 1, mic: 1, video: 1
    screenCaptureCb_->InitCaptureTrackInfo(innerAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER);
    screenCaptureCb_->InitCaptureTrackInfo(micAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC);
    screenCaptureCb_->InitCaptureTrackInfo(videoFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_VIDEO);
    // callback enabled: errorCallback: true, dataCallback: true, stateChangeCallback: true
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_, true, true, true));

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCaptureWithSurface(consumer));
    EXPECT_EQ(MSERR_OK, recorder->Start());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_005 recorder Start");
    sleep(RECORDER_TIME);
    EXPECT_EQ(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED, screenCaptureCb_->GetScreenCaptureState());
    EXPECT_EQ(MSERR_OK, recorder->Stop(true));
    EXPECT_EQ(MSERR_OK, recorder->Reset());
    EXPECT_EQ(MSERR_OK, recorder->Release());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_005 after");
}

/**
 * @tc.name: screen_capture_exclude_content_with_surface_cb_006
 * @tc.desc: surface mode, capture screen only, exclude app and notice audio and sone windowss
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_with_surface_cb_006, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_006 before");
    SetConfig(config_);
    config_.audioInfo.micCapInfo.audioSampleRate = 0;
    config_.audioInfo.micCapInfo.audioChannels = 0;
    config_.audioInfo.micCapInfo.audioSource = AudioCaptureSourceType::SOURCE_DEFAULT;
    screenCapture_->SetMicrophoneEnabled(true);
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_006 CreateRecorder");
    std::shared_ptr<Recorder> recorder = RecorderFactory::CreateRecorder();
    int32_t videoSourceId = 0;
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSource(VIDEO_SOURCE_SURFACE_RGBA, videoSourceId));
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFormat(OutputFormatType::FORMAT_MPEG_4));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncoder(videoSourceId, VideoCodecFormat::H264));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSize(videoSourceId, 720, 1080));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoFrameRate(videoSourceId, 30));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncodingBitRate(videoSourceId, 2000000));
    OpenFileFd("screen_capture_exclude_content_with_surface_cb_006.mp4");
    ASSERT_TRUE(outputFd_ >= 0);
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFile(outputFd_));
    EXPECT_EQ(MSERR_OK, recorder->Prepare());
    sptr<OHOS::Surface> consumer = recorder->GetSurface(videoSourceId);

    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_);
    ASSERT_NE(nullptr, screenCaptureCb_);

    std::string name = "screen_capture_exclude_content_with_surface_cb_006";
    // track enalbed: inner: false, mic: false, video: false
    OpenFile(name, false, false, false);
    // check track aquire & release: inner: 1, mic: 1, video: 1
    screenCaptureCb_->InitCaptureTrackInfo(innerAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER);
    screenCaptureCb_->InitCaptureTrackInfo(micAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC);
    screenCaptureCb_->InitCaptureTrackInfo(videoFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_VIDEO);
    // callback enabled: errorCallback: true, dataCallback: true, stateChangeCallback: true
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_, true, true, true));

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCaptureWithSurface(consumer));
    EXPECT_EQ(MSERR_OK, recorder->Start());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_006 recorder Start");
    sleep(RECORDER_TIME);
    EXPECT_EQ(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED, screenCaptureCb_->GetScreenCaptureState());
    EXPECT_EQ(MSERR_OK, recorder->Stop(true));
    EXPECT_EQ(MSERR_OK, recorder->Reset());
    EXPECT_EQ(MSERR_OK, recorder->Release());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_006 after");
}

/**
 * @tc.name: screen_capture_exclude_content_with_surface_cb_007
 * @tc.desc: surface mode, capture screen, inner audio and mic audio, exclude app and notice audio and sone windows
 * @tc.type: FUNC
 * @tc.require: play something audio resource to get inner audio
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_exclude_content_with_surface_cb_007, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_007 before");
    SetConfig(config_);
    config_.audioInfo.innerCapInfo.audioSampleRate = 16000;
    config_.audioInfo.innerCapInfo.audioChannels = 2;
    config_.audioInfo.innerCapInfo.audioSource = AudioCaptureSourceType::ALL_PLAYBACK;
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_007 CreateRecorder");
    std::shared_ptr<Recorder> recorder = nullptr;
    recorder = RecorderFactory::CreateRecorder();
    int32_t videoSourceId = 0;
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSource(VIDEO_SOURCE_SURFACE_RGBA, videoSourceId));
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFormat(OutputFormatType::FORMAT_MPEG_4));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncoder(videoSourceId, VideoCodecFormat::H264));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSize(videoSourceId, 720, 1080));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoFrameRate(videoSourceId, 30));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncodingBitRate(videoSourceId, 2000000));
    OpenFileFd("screen_capture_exclude_content_with_surface_cb_007.mp4");
    ASSERT_TRUE(outputFd_ >= 0);
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFile(outputFd_));
    EXPECT_EQ(MSERR_OK, recorder->Prepare());
    sptr<OHOS::Surface> consumer = recorder->GetSurface(videoSourceId);

    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_);
    ASSERT_NE(nullptr, screenCaptureCb_);

    std::string name = "screen_capture_exclude_content_with_surface_cb_007";
    // track enalbed: inner: true, mic: true, video: true(surface mode)
    OpenFile(name, true, true, false);
    // check track aquire & release: inner: 1, mic: 1, video: 1
    screenCaptureCb_->InitCaptureTrackInfo(innerAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER);
    screenCaptureCb_->InitCaptureTrackInfo(micAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC);
    screenCaptureCb_->InitCaptureTrackInfo(videoFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_VIDEO);
    // callback enabled: errorCallback: true, dataCallback: true, stateChangeCallback: true
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_, true, true, true));

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    vector<int> windowIds = {1, 3, 5};
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeWindowContent(&windowIds[0], static_cast<int32_t>(windowIds.size())));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_NOTIFICATION_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->ExcludeAudioContent(SCREEN_CAPTURE_CURRENT_APP_AUDIO));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCaptureWithSurface(consumer));
    EXPECT_EQ(MSERR_OK, recorder->Start());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_007 recorder Start");
    sleep(RECORDER_TIME);
    EXPECT_EQ(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED, screenCaptureCb_->GetScreenCaptureState());
    EXPECT_EQ(MSERR_OK, recorder->Stop(true));
    EXPECT_EQ(MSERR_OK, recorder->Reset());
    EXPECT_EQ(MSERR_OK, recorder->Release());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_exclude_content_with_surface_cb_007 after");
}
} // namespace Media
} // namespace OHOS