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

#include <fcntl.h>
#include <iostream>
#include <string>
#include "media_errors.h"
#include "screen_capture_unit_test.h"
#include "ability_manager_client.h"
#include "accesstoken_kit.h"
#include "token_setproc.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace std;
using namespace OHOS::Rosen;
using namespace OHOS::Media::ScreenCaptureTestParam;

namespace OHOS {
namespace Media {

/**
 * @tc.name: screen_capture_save_file_mix_01
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_save_file_mix_01, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_mix_01 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_save_file_mix_01.mp4", recorderInfo);
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
        .audioSource = AudioCaptureSourceType::APP_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_mix_01 after");
}

/**
 * @tc.name: screen_capture_save_file_mix_02
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_save_file_mix_02, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_mix_02 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_save_file_mix_02.mp4", recorderInfo);
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
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_mix_02 after");
}

/**
 * @tc.name: screen_capture_save_file_mix_03
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_save_file_mix_03, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_mix_03 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_save_file_mix_03.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo micCapInfo = {
        .audioSampleRate = 48000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::SOURCE_DEFAULT
    };
    config_.audioInfo.micCapInfo = micCapInfo;
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 48000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::APP_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_mix_03 after");
}

/**
 * @tc.name: screen_capture_save_file_mix_04
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_save_file_mix_04, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_mix_04 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_save_file_mix_04.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo micCapInfo = {
        .audioSampleRate = 48000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::SOURCE_DEFAULT
    };
    config_.audioInfo.micCapInfo = micCapInfo;
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 48000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::ALL_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_mix_04 after");
}

/**
* @tc.name: screen_capture_report_user_choice_01
* @tc.desc: do screencapture
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(ScreenCaptureUnitTest, screen_capture_report_user_choice_01, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_report_user_choice_01 before");
    std::shared_ptr<ScreenCaptureController> controller =
        ScreenCaptureControllerFactory::CreateScreenCaptureController();
    int32_t sessionId = 0;
    std::string choice = "false";
    controller->ReportAVScreenCaptureUserChoice(sessionId, choice);
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_report_user_choice_01 after");
}

/**
 * @tc.name: screen_capture_specified_window_file_01
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_specified_window_file_01, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_file_01 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_specified_window_file_01.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::ALL_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;
    config_.captureMode = CaptureMode::CAPTURE_SPECIFIED_WINDOW;
    std::shared_ptr<OHOS::AAFwk::AbilityManagerClient> client_ = OHOS::AAFwk::AbilityManagerClient::GetInstance();
    std::string deviceId = "";
    std::vector<OHOS::AAFwk::MissionInfo> missionInfos;
    auto result = client_->GetMissionInfos(deviceId, 20, missionInfos);
    MEDIA_LOGI("screen_capture_specified_window_file_01 missionInfos size:%{public}s, result:%{public}d",
        std::to_string(missionInfos.size()).c_str(), result);
    for (OHOS::AAFwk::MissionInfo info : missionInfos) {
        MEDIA_LOGI("screen_capture_specified_window_file_01 missionId : %{public}d", info.id);
    }
    if (missionInfos.size() > 0) {
        config_.videoInfo.videoCapInfo.taskIDs.push_back(missionInfos[0].id);
    } else {
        MEDIA_LOGE("screen_capture_specified_window_file_01 GetMissionInfos failed");
    }

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_file_01 after");
}

/**
 * @tc.name: screen_capture_specified_window_file_02
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_specified_window_file_02, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_file_02 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_specified_window_file_02.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::ALL_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;
    config_.captureMode = CaptureMode::CAPTURE_SPECIFIED_WINDOW;
    std::shared_ptr<OHOS::AAFwk::AbilityManagerClient> client_ = OHOS::AAFwk::AbilityManagerClient::GetInstance();
    std::string deviceId = "";
    std::vector<OHOS::AAFwk::MissionInfo> missionInfos;
    auto result = client_->GetMissionInfos(deviceId, 20, missionInfos);
    MEDIA_LOGI("screen_capture_specified_window_file_02 missionInfos size:%{public}s, result:%{public}d",
        std::to_string(missionInfos.size()).c_str(), result);
    for (OHOS::AAFwk::MissionInfo info : missionInfos) {
        MEDIA_LOGI("screen_capture_specified_window_file_02 missionId : %{public}d", info.id);
    }
    if (missionInfos.size() > 0) {
        config_.videoInfo.videoCapInfo.taskIDs.push_back(missionInfos[(missionInfos.size()-1)/2].id);
    } else {
        MEDIA_LOGE("screen_capture_specified_window_file_02 GetMissionInfos failed");
    }

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_file_02 after");
}

/**
 * @tc.name: screen_capture_specified_window_file_03
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_specified_window_file_03, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_file_03 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_specified_window_file_03.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::ALL_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;
    config_.captureMode = CaptureMode::CAPTURE_SPECIFIED_WINDOW;
    std::shared_ptr<OHOS::AAFwk::AbilityManagerClient> client_ = OHOS::AAFwk::AbilityManagerClient::GetInstance();
    std::string deviceId = "";
    std::vector<OHOS::AAFwk::MissionInfo> missionInfos;
    auto result = client_->GetMissionInfos(deviceId, 20, missionInfos);
    MEDIA_LOGI("screen_capture_specified_window_file_03 missionInfos size:%{public}s, result:%{public}d",
        std::to_string(missionInfos.size()).c_str(), result);
    for (OHOS::AAFwk::MissionInfo info : missionInfos) {
        MEDIA_LOGI("screen_capture_specified_window_file_03 missionId : %{public}d", info.id);
    }
    if (missionInfos.size() > 0) {
        config_.videoInfo.videoCapInfo.taskIDs.push_back(missionInfos[missionInfos.size()-1].id);
    } else {
        MEDIA_LOGE("screen_capture_specified_window_file_03 GetMissionInfos failed");
    }

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_file_03 after");
}

/**
 * @tc.name: screen_capture_specified_window_file_04
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_specified_window_file_04, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_file_04 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_specified_window_file_04.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::ALL_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;
    config_.captureMode = CaptureMode::CAPTURE_SPECIFIED_WINDOW;
    std::shared_ptr<OHOS::AAFwk::AbilityManagerClient> client_ = OHOS::AAFwk::AbilityManagerClient::GetInstance();
    std::string deviceId = "";
    std::vector<OHOS::AAFwk::MissionInfo> missionInfos;
    auto result = client_->GetMissionInfos(deviceId, 20, missionInfos);
    MEDIA_LOGI("screen_capture_specified_window_file_04 missionInfos size:%{public}s, result:%{public}d",
        std::to_string(missionInfos.size()).c_str(), result);
    if (missionInfos.size() > 0) {
        for (OHOS::AAFwk::MissionInfo info : missionInfos) {
            MEDIA_LOGI("screen_capture_specified_window_file_04 missionId:%{public}d", info.id);
            config_.videoInfo.videoCapInfo.taskIDs.push_back(info.id);
        }
    } else {
        MEDIA_LOGE("screen_capture_specified_window_file_04 GetMissionInfos failed");
    }

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_file_04 after");
}

/**
 * @tc.name: screen_capture_specified_window
 * @tc.desc: open microphone
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_specified_window, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window before");
    SetConfig(config_);
    config_.captureMode = CaptureMode::CAPTURE_SPECIFIED_WINDOW;
    std::shared_ptr<OHOS::AAFwk::AbilityManagerClient> client_ = OHOS::AAFwk::AbilityManagerClient::GetInstance();
    std::string deviceId = "";
    std::vector<OHOS::AAFwk::MissionInfo> missionInfos;
    auto result = client_->GetMissionInfos(deviceId, 10, missionInfos);
    MEDIA_LOGI("screen_capture_specified_window missionInfos size:%{public}s, result:%{public}d",
        std::to_string(missionInfos.size()).c_str(), result);
    if (missionInfos.size() > 0) {
        for (OHOS::AAFwk::MissionInfo info : missionInfos) {
            MEDIA_LOGI("screen_capture_specified_window missionId:%{public}d", info.id);
            config_.videoInfo.videoCapInfo.taskIDs.push_back(info.id);
        }
    } else {
        MEDIA_LOGE("screen_capture_specified_window GetMissionInfos failed");
    }
    OpenFile("screen_capture_specified_window");

    aFlag = 1;
    vFlag = 1;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window after");
}

/**
 * @tc.name: screen_capture_specified_window_Rotation
 * @tc.desc: open microphone
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_specified_window_Rotation, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_Rotation before");
    SetConfig(config_);
    config_.captureMode = CaptureMode::CAPTURE_SPECIFIED_WINDOW;
    std::shared_ptr<OHOS::AAFwk::AbilityManagerClient> client_ = OHOS::AAFwk::AbilityManagerClient::GetInstance();
    std::string deviceId = "";
    std::vector<OHOS::AAFwk::MissionInfo> missionInfos;
    auto result = client_->GetMissionInfos(deviceId, 10, missionInfos);
    MEDIA_LOGI("screen_capture_specified_window_Rotation missionInfos size:%{public}s, result:%{public}d",
        std::to_string(missionInfos.size()).c_str(), result);
    if (missionInfos.size() > 0) {
        for (OHOS::AAFwk::MissionInfo info : missionInfos) {
            MEDIA_LOGI("screen_capture_specified_window_Rotation missionId:%{public}d", info.id);
            config_.videoInfo.videoCapInfo.taskIDs.push_back(info.id);
        }
    } else {
        MEDIA_LOGE("screen_capture_specified_window_Rotation GetMissionInfos failed");
    }
    OpenFile("screen_capture_specified_window_Rotation");

    aFlag = 1;
    vFlag = 1;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    bool canvasRotation = true;
    EXPECT_EQ(MSERR_OK, screenCapture_->SetCanvasRotation(canvasRotation));
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->SetCanvasRotation(canvasRotation));
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_Rotation after");
}

/**
 * @tc.name: screen_capture_save_file_01
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_save_file_01, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_01 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_get_screen_capture_01.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::APP_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_01 after");
}

/**
 * @tc.name: screen_capture_save_file_02
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_save_file_02, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_02 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_get_screen_capture_02.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::APP_PLAYBACK
    };
    AudioCaptureInfo micCapInfoInvalidChannels = {
        .audioSampleRate = 16000,
        .audioChannels = 0,
        .audioSource = AudioCaptureSourceType::MIC
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;
    config_.audioInfo.micCapInfo = micCapInfoInvalidChannels;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    AudioCaptureInfo micCapInfoInvalidSampleRate = {
        .audioSampleRate = 0,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::MIC
    };
    SetRecorderInfo("screen_capture_get_screen_capture_02.mp4", recorderInfo);
    config_.recorderInfo = recorderInfo;
    config_.audioInfo.micCapInfo = micCapInfoInvalidSampleRate;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    AudioCaptureInfo micCapInfoIgnored = {
        .audioSampleRate = 0,
        .audioChannels = 0,
        .audioSource = AudioCaptureSourceType::MIC
    };
    SetRecorderInfo("screen_capture_get_screen_capture_02.mp4", recorderInfo);
    config_.recorderInfo = recorderInfo;
    config_.audioInfo.micCapInfo = micCapInfoIgnored;
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_02 after");
}

/**
 * @tc.name: screen_capture_save_file_03
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_save_file_03, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_03 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_get_screen_capture_03.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo micCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::MIC
    };
    config_.audioInfo.micCapInfo = micCapInfo;
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::APP_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_03 after");
}

/**
 * @tc.name: screen_capture_save_file_04
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_save_file_04, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_04 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_get_screen_capture_04.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::APP_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_NE(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_NE(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_04 after");
}

/**
 * @tc.name: screen_capture_save_file_05
 * @tc.desc: open microphone
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_save_file_05, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_05 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    OpenFile("screen_capture_get_screen_capture_05");

    aFlag = 1;
    vFlag = 1;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_NE(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_NE(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_05 after");
}

/**
 * @tc.name: screen_capture_save_file_Rotation
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_save_file_Rotation, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_Rotation before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    OpenFile("screen_capture_save_file_Rotation");

    aFlag = 1;
    vFlag = 1;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    bool canvasRotation = true;
    EXPECT_EQ(MSERR_OK, screenCapture_->SetCanvasRotation(canvasRotation));
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_NE(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_NE(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_Rotation after");
}

/**
 * @tc.name: screen_capture_save_file_Rotation_01
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_save_file_Rotation_01, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_Rotation_01 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_get_screen_capture_Rotation_01.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo innerCapInfo = {
            .audioSampleRate = 16000,
            .audioChannels = 2,
            .audioSource = AudioCaptureSourceType::APP_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;
    config_.videoInfo.videoCapInfo.videoFrameWidth = 2720;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 1260;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    bool canvasRotation = true;
    EXPECT_EQ(MSERR_OK, screenCapture_->SetCanvasRotation(canvasRotation));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetCanvasRotation(canvasRotation));
    canvasRotation = false;
    EXPECT_EQ(MSERR_OK, screenCapture_->SetCanvasRotation(canvasRotation));
    sleep(RECORDER_TIME);
    canvasRotation = true;
    EXPECT_EQ(MSERR_OK, screenCapture_->SetCanvasRotation(canvasRotation));
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_Rotation_01 after");
}

/**
 * @tc.name: screen_capture_save_file_Rotation_02
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_save_file_Rotation_02, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_Rotation_02 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_get_screen_capture_Rotation_02.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo innerCapInfo = {
            .audioSampleRate = 16000,
            .audioChannels = 2,
            .audioSource = AudioCaptureSourceType::APP_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;
    config_.videoInfo.videoCapInfo.videoFrameWidth = 2720;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 1260;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    bool canvasRotation = true;
    EXPECT_EQ(MSERR_OK, screenCapture_->SetCanvasRotation(canvasRotation));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->SetCanvasRotation(canvasRotation));
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_Rotation_02 after");
}

/**
 * @tc.name: screen_capture_save_file_Rotation_03
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_save_file_Rotation_03, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_Rotation_03 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_get_screen_capture_Rotation_03.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo innerCapInfo = {
            .audioSampleRate = 16000,
            .audioChannels = 2,
            .audioSource = AudioCaptureSourceType::APP_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;
    config_.videoInfo.videoCapInfo.videoFrameWidth = 2720;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 1260;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    bool canvasRotation = false;
    EXPECT_EQ(MSERR_OK, screenCapture_->SetCanvasRotation(canvasRotation));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->SetCanvasRotation(canvasRotation));
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_Rotation_03 after");
}

/**
 * @tc.name: screen_capture_specified_screen_file_01
 * @tc.desc: do screencapture with specified screen
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_specified_screen_file_01, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_screen_file_01 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_specified_screen_file_01.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::APP_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;
    config_.captureMode = CaptureMode::CAPTURE_SPECIFIED_SCREEN;

    std::vector<sptr<Screen>> screens;
    DMError ret = ScreenManager::GetInstance().GetAllScreens(screens);
    MEDIA_LOGI("screen_capture_specified_screen_file_01 screens size:%{public}s, ret:%{public}d",
        std::to_string(screens.size()).c_str(), ret);
    if (screens.size() > 0) {
        MEDIA_LOGI("screen_capture_specified_screen_file_01 screens id(size-1):%{public}s",
            std::to_string(screens[screens.size() - 1]->GetId()).c_str());
        MEDIA_LOGI("screen_capture_specified_screen_file_01 screens id(size-1):%{public}s",
            std::to_string(screens[0]->GetId()).c_str());
        config_.videoInfo.videoCapInfo.displayId = screens[0]->GetId();
    } else {
        MEDIA_LOGE("screen_capture_specified_screen_file_01 GetAllScreens failed");
    }

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_screen_file_01 after");
}

/**
 * @tc.name: screen_capture_specified_screen_file_02
 * @tc.desc: do screencapture with specified screen
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_specified_screen_file_02, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_screen_file_02 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_specified_screen_file_02.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 48000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::APP_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;
    config_.captureMode = CaptureMode::CAPTURE_SPECIFIED_SCREEN;

    std::vector<sptr<Screen>> screens;
    DMError ret = ScreenManager::GetInstance().GetAllScreens(screens);
    MEDIA_LOGI("screen_capture_specified_screen_file_02 screens size:%{public}s, ret:%{public}d",
        std::to_string(screens.size()).c_str(), ret);
    if (screens.size() > 0) {
        MEDIA_LOGI("screen_capture_specified_screen_file_02 screens id(size-1):%{public}s",
            std::to_string(screens[screens.size() - 1]->GetId()).c_str());
        MEDIA_LOGI("screen_capture_specified_screen_file_02 screens id(size-1):%{public}s",
            std::to_string(screens[0]->GetId()).c_str());
        config_.videoInfo.videoCapInfo.displayId = screens[0]->GetId();
    } else {
        MEDIA_LOGE("screen_capture_specified_screen_file_02 GetAllScreens failed");
    }

    config_.videoInfo.videoCapInfo.displayId = screens[0]->GetId();

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_screen_file_02 after");
}

/**
 * @tc.name: screen_capture_specified_screen_file_03
 * @tc.desc: do screencapture with specified screen
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_specified_screen_file_03, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_screen_file_03 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_specified_screen_file_03.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo micCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::MIC
    };
    config_.audioInfo.micCapInfo = micCapInfo;
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::APP_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;
    config_.captureMode = CaptureMode::CAPTURE_SPECIFIED_SCREEN;

    std::vector<sptr<Screen>> screens;
    DMError ret = ScreenManager::GetInstance().GetAllScreens(screens);
    MEDIA_LOGI("screen_capture_specified_screen_file_03 screens size:%{public}s, ret:%{public}d",
        std::to_string(screens.size()).c_str(), ret);
    if (screens.size() > 0) {
        MEDIA_LOGI("screen_capture_specified_screen_file_03 screens id(size-1):%{public}s",
            std::to_string(screens[screens.size() - 1]->GetId()).c_str());
        MEDIA_LOGI("screen_capture_specified_screen_file_03 screens id(size-1):%{public}s",
            std::to_string(screens[0]->GetId()).c_str());
        config_.videoInfo.videoCapInfo.displayId = screens[0]->GetId();
    } else {
        MEDIA_LOGE("screen_capture_specified_screen_file_03 GetAllScreens failed");
    }

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_screen_file_03 after");
}

/**
 * @tc.name: screen_capture_specified_screen_01
 * @tc.desc: screen capture specified screen test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_specified_screen_01, TestSize.Level1)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_screen_01 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    std::vector<sptr<Screen>> screens;
    DMError ret = ScreenManager::GetInstance().GetAllScreens(screens);
    MEDIA_LOGI("screen_capture_specified_screen_01 screens size:%{public}s, ret:%{public}d",
        std::to_string(screens.size()).c_str(), ret);
    if (screens.size() > 0) {
        MEDIA_LOGI("screen_capture_specified_screen_01 screens id(size-1):%{public}s",
            std::to_string(screens[screens.size() - 1]->GetId()).c_str());
        MEDIA_LOGI("screen_capture_specified_screen_01 screens id(size-1):%{public}s",
            std::to_string(screens[0]->GetId()).c_str());
        config_.videoInfo.videoCapInfo.displayId = screens[0]->GetId();
    } else {
        MEDIA_LOGE("screen_capture_specified_screen_01 GetAllScreens failed");
    }
    OpenFile("screen_capture_specified_screen_01");

    aFlag = 1;
    vFlag = 1;
    bool isMicrophone = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(3);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_screen_01 after");
}

/**
 * @tc.name: screen_capture_check_param_01
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_check_param_01, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_check_param_01 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_check_param_01.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo micCapInfoRateSmall = {
        .audioSampleRate = 0,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::MIC
    };
    config_.audioInfo.micCapInfo = micCapInfoRateSmall;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    SetRecorderInfo("screen_capture_check_param_01.mp4", recorderInfo);
    config_.recorderInfo = recorderInfo;
    config_.audioInfo.micCapInfo.audioSampleRate = 8000000;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    SetRecorderInfo("screen_capture_check_param_01.mp4", recorderInfo);
    config_.recorderInfo = recorderInfo;
    config_.audioInfo.micCapInfo.audioSampleRate = 16000;
    config_.audioInfo.micCapInfo.audioChannels = 0;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    SetRecorderInfo("screen_capture_check_param_01.mp4", recorderInfo);
    config_.recorderInfo = recorderInfo;
    config_.audioInfo.micCapInfo.audioChannels = 200;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    SetRecorderInfo("screen_capture_check_param_01.mp4", recorderInfo);
    config_.recorderInfo = recorderInfo;
    config_.audioInfo.micCapInfo.audioChannels = 2;
    config_.audioInfo.micCapInfo.audioSource = AudioCaptureSourceType::APP_PLAYBACK;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    SetRecorderInfo("screen_capture_check_param_01.mp4", recorderInfo);
    config_.recorderInfo = recorderInfo;
    config_.audioInfo.micCapInfo.audioSource = AudioCaptureSourceType::SOURCE_INVALID;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_check_param_01 after");
}

/**
 * @tc.name: screen_capture_check_param_02
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_check_param_02, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_check_param_02 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_check_param_02.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo micCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::MIC
    };
    config_.audioInfo.micCapInfo = micCapInfo;
    AudioEncInfo audioEncInfoBitSmall = {
        .audioBitrate = 0,
        .audioCodecformat = AudioCodecFormat::AAC_LC
    };
    config_.audioInfo.audioEncInfo = audioEncInfoBitSmall;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    SetRecorderInfo("screen_capture_check_param_02.mp4", recorderInfo);
    config_.recorderInfo = recorderInfo;
    config_.audioInfo.audioEncInfo.audioBitrate = 4800000;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    SetRecorderInfo("screen_capture_check_param_02.mp4", recorderInfo);
    config_.recorderInfo = recorderInfo;
    config_.audioInfo.audioEncInfo.audioBitrate = 48000;
    config_.audioInfo.audioEncInfo.audioCodecformat = AudioCodecFormat::AUDIO_CODEC_FORMAT_BUTT;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_check_param_02 after");
}

/**
 * @tc.name: screen_capture_check_param_03
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_check_param_03, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_check_param_03 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_check_param_03.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo micCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::MIC
    };
    config_.audioInfo.micCapInfo = micCapInfo;
    VideoCaptureInfo videoCapInfoWidthSmall = {
        .videoFrameWidth = 0,
        .videoFrameHeight = 1080,
        .videoSource = VideoSourceType::VIDEO_SOURCE_SURFACE_RGBA
    };
    config_.videoInfo.videoCapInfo = videoCapInfoWidthSmall;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    SetRecorderInfo("screen_capture_check_param_03.mp4", recorderInfo);
    config_.recorderInfo = recorderInfo;
    config_.videoInfo.videoCapInfo.videoFrameWidth = 720;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 0;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    SetRecorderInfo("screen_capture_check_param_03.mp4", recorderInfo);
    config_.recorderInfo = recorderInfo;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 1080;
    config_.videoInfo.videoCapInfo.videoSource = VideoSourceType::VIDEO_SOURCE_BUTT;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_check_param_03 after");
}

/**
 * @tc.name: screen_capture_check_param_04
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_check_param_04, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_check_param_04 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_check_param_04.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo micCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::MIC
    };
    config_.audioInfo.micCapInfo = micCapInfo;
    VideoEncInfo videoEncInfoBitSmall = {
        .videoCodec = VideoCodecFormat::MPEG4,
        .videoBitrate = 0,
        .videoFrameRate = 30
    };
    config_.videoInfo.videoEncInfo = videoEncInfoBitSmall;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    SetRecorderInfo("screen_capture_check_param_04.mp4", recorderInfo);
    config_.recorderInfo = recorderInfo;
    config_.videoInfo.videoEncInfo.videoBitrate = 30000001;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    SetRecorderInfo("screen_capture_check_param_04.mp4", recorderInfo);
    config_.recorderInfo = recorderInfo;
    config_.videoInfo.videoEncInfo.videoBitrate = 2000000;
    config_.videoInfo.videoEncInfo.videoFrameRate = 0;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    SetRecorderInfo("screen_capture_check_param_04.mp4", recorderInfo);
    config_.recorderInfo = recorderInfo;
    config_.videoInfo.videoEncInfo.videoFrameRate = 300;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    SetRecorderInfo("screen_capture_check_param_04.mp4", recorderInfo);
    config_.recorderInfo = recorderInfo;
    config_.videoInfo.videoEncInfo.videoFrameRate = 30;
    config_.videoInfo.videoEncInfo.videoCodec = VideoCodecFormat::VIDEO_CODEC_FORMAT_BUTT;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_check_param_04 after");
}

/**
 * @tc.name: screen_capture_check_param_05
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_check_param_05, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_check_param_05 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_check_param_05.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::APP_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;
    config_.dataType = DataType::ENCODED_STREAM;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    SetRecorderInfo("screen_capture_check_param_05.mp4", recorderInfo);
    config_.recorderInfo = recorderInfo;
    config_.dataType = DataType::INVAILD;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    SetRecorderInfo("screen_capture_check_param_05.mp4", recorderInfo);
    config_.recorderInfo = recorderInfo;
    config_.dataType = DataType::CAPTURE_FILE;
    config_.captureMode = CaptureMode::CAPTURE_INVAILD;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_check_param_05 after");
}

/**
 * @tc.name: screen_capture_check_param_06
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_check_param_06, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_check_param_06 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_check_param_06.mp4", recorderInfo);
    recorderInfo.fileFormat = "avi";
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::APP_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;

    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_check_param_06 after");
}

/**
 * @tc.name: screen_capture_check_param_07
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_check_param_07, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_check_param_07 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_check_param_07.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::APP_PLAYBACK
    };
    AudioCaptureInfo micCapInfo = {
        .audioSampleRate = 48000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::MIC
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;
    config_.audioInfo.micCapInfo = micCapInfo;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    SetRecorderInfo("screen_capture_check_param_07.mp4", recorderInfo);
    config_.recorderInfo = recorderInfo;
    config_.audioInfo.micCapInfo.audioSampleRate = 16000;
    config_.audioInfo.micCapInfo.audioChannels = 1;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    SetRecorderInfo("screen_capture_check_param_07.mp4", recorderInfo);
    config_.recorderInfo = recorderInfo;
    config_.audioInfo.micCapInfo.audioSampleRate = 48000;
    config_.audioInfo.micCapInfo.audioChannels = 1;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_check_param_07 after");
}

/**
 * @tc.name: screen_capture_video_configure_0001
 * @tc.desc: init with videoFrameWidth -1
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_video_configure_0001, TestSize.Level2)
{
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoFrameWidth = -1;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    bool isMicrophone = false;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
}

/**
 * @tc.name: screen_capture_video_configure_0002
 * @tc.desc: init with videoFrameHeight -1
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_video_configure_0002, TestSize.Level2)
{
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoFrameHeight = -1;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    bool isMicrophone = false;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
}

/**
 * @tc.name: screen_capture_video_configure_0003
 * @tc.desc: init with videoSource yuv
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_video_configure_0003, TestSize.Level2)
{
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_YUV;

    bool isMicrophone = false;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
}

/**
 * @tc.name: screen_capture_video_configure_0004
 * @tc.desc: init with videoSource es
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_video_configure_0004, TestSize.Level0)
{
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_ES;

    bool isMicrophone = false;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
}

/**
 * @tc.name: screen_capture_without_audio_data
 * @tc.desc: close microphone
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_without_audio_data, TestSize.Level2)
{
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    OpenFile("screen_capture_without_audio_data");

    aFlag = 0;
    vFlag = 1;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    bool isMicrophone = false;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
}

/**
 * @tc.name: screen_capture_audio_configure_0001
 * @tc.desc: init with audioSampleRate -1
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_audio_configure_0001, TestSize.Level2)
{
    SetConfig(config_);
    config_.audioInfo.micCapInfo.audioSampleRate = -1;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
}

/**
 * @tc.name: screen_capture_audio_configure_0002
 * @tc.desc: init with audioChannels -1
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_audio_configure_0002, TestSize.Level2)
{
    SetConfig(config_);
    config_.audioInfo.micCapInfo.audioChannels = -1;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
}

/**
 * @tc.name: screen_capture_audio_configure_0003
 * @tc.desc: init with audioSource SOURCE_INVALID
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_audio_configure_0003, TestSize.Level2)
{
    SetConfig(config_);
    config_.audioInfo.micCapInfo.audioSource = SOURCE_INVALID;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
}

/**
 * @tc.name: screen_capture_avconfigure
 * @tc.desc: init with both audioinfo and videoinfo invaild
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_avconfigure, TestSize.Level2)
{
    SetConfig(config_);
    config_.audioInfo.micCapInfo.audioSource = SOURCE_INVALID;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_YUV;

    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
}

/**
 * @tc.name: screen_capture_with_audio_data
 * @tc.desc: open microphone
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_with_audio_data, TestSize.Level2)
{
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    OpenFile("screen_capture_with_audio_data");

    aFlag = 1;
    vFlag = 1;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
}

/**
 * @tc.name: screen_capture_captureMode_0001
 * @tc.desc: screen capture with captureMode -1
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_captureMode_0001, TestSize.Level2)
{
    SetConfig(config_);
    config_.captureMode = static_cast<CaptureMode>(-1);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
}

/**
 * @tc.name: screen_capture_captureMode_0002
 * @tc.desc: screen capture with captureMode 5
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_captureMode_0002, TestSize.Level2)
{
    SetConfig(config_);
    config_.captureMode = static_cast<CaptureMode>(5);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
}

/**
 * @tc.name: screen_capture_init_datatype_0001
 * @tc.desc: screen capture init with ENCODED_STREAM
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_init_datatype_0001, TestSize.Level2)
{
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    config_.dataType = ENCODED_STREAM;

    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
}

/**
 * @tc.name: screen_capture_init_datatype_0002
 * @tc.desc: screen capture init with CAPTURE_FILE
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_init_datatype_0002, TestSize.Level2)
{
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    config_.dataType = CAPTURE_FILE;

    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
}

/**
 * @tc.name: screen_capture_init_datatype_0003
 * @tc.desc: screen capture init with INVAILD
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_init_datatype_0003, TestSize.Level2)
{
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    config_.dataType = INVAILD;

    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
}

/**
 * @tc.name: screen_capture_audioSampleRate_48000
 * @tc.desc: screen capture with audioSampleRate 48000
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_audioSampleRate_48000, TestSize.Level2)
{
    SetConfig(config_);
    config_.audioInfo.micCapInfo.audioSampleRate = 48000;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    aFlag = 1;
    vFlag = 1;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
}

/**
 * @tc.name: screen_capture_video_size_0001
 * @tc.desc: screen capture with 160x160
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_video_size_0001, TestSize.Level2)
{
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoFrameWidth = 160;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 160;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    std::string name = SCREEN_CAPTURE_ROOT_DIR + "screen_capture_video_size_0001.yuv";
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
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
}

/**
 * @tc.name: screen_capture_video_size_0002
 * @tc.desc: screen capture with 640x480
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_video_size_0002, TestSize.Level2)
{
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoFrameWidth = 640;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 480;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    std::string name = SCREEN_CAPTURE_ROOT_DIR + "screen_capture_video_size_0002.yuv";
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
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
}

/**
 * @tc.name: screen_capture_video_size_0003
 * @tc.desc: screen capture with 1920x1080
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_video_size_0003, TestSize.Level2)
{
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoFrameWidth = 1920;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 1080;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    std::string name = SCREEN_CAPTURE_ROOT_DIR + "screen_capture_video_size_0003.yuv";
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
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
}

/**
 * @tc.name: screen_capture_from_display
 * @tc.desc: screen capture from display
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_from_display, TestSize.Level0)
{
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
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
}

/**
 * @tc.name: screen_capture_buffertest_0001
 * @tc.desc: screen capture buffer test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_buffertest_0001, TestSize.Level2)
{
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
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
}

/**
 * @tc.name: screen_capture_buffertest_0002
 * @tc.desc: screen capture buffer test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_buffertest_0002, TestSize.Level2)
{
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    int index = 200;
    int index_video_frame = 0;
    audioLoop_ = std::make_unique<std::thread>(&ScreenCaptureUnitTest::AudioLoop, this);
    while (index) {
        int32_t fence = 0;
        int64_t timestamp = 0;
        OHOS::Rect damage;
        sptr<OHOS::SurfaceBuffer> surfacebuffer = screenCapture_->AcquireVideoBuffer(fence, timestamp, damage);
        if (surfacebuffer != nullptr) {
            int32_t length = surfacebuffer->GetSize();
            cout << "index video:" << index_video_frame++ << "; AcquireVideoBuffer, videoBufferLen:" <<
                surfacebuffer->GetSize() << ", timestamp:" << timestamp << ", size:"<< length << endl;
            screenCapture_->ReleaseVideoBuffer();
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
}

/**
 * @tc.name: screen_capture_buffertest_0003
 * @tc.desc: screen capture buffer test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_buffertest_0003, TestSize.Level2)
{
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    aFlag = 0;
    vFlag = 1;
    bool isMicrophone = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(15);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
}

/**
 * @tc.name: screen_capture_buffertest_0004
 * @tc.desc: screen capture buffer test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_buffertest_0004, TestSize.Level2)
{
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    aFlag = 1;
    vFlag = 0;
    bool isMicrophone = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(10);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
}

/**
 * @tc.name: screen_capture_buffertest_0005
 * @tc.desc: screen capture buffer test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_buffertest_0005, TestSize.Level2)
{
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
    sleep(10);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
}

/**
 * @tc.name: screen_capture_buffertest_Rotation
 * @tc.desc: screen capture buffer test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_buffertest_Rotation, TestSize.Level2)
{
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    aFlag = 0;
    vFlag = 0;
    bool isMicrophone = true;
    bool canvasRotation = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetCanvasRotation(canvasRotation));
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->SetCanvasRotation(canvasRotation));
    sleep(10);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
}

/**
 * @tc.name: screen_capture_mic_open_close_open
 * @tc.desc: screen capture mic test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_mic_open_close_open, TestSize.Level2)
{
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    std::string name = SCREEN_CAPTURE_ROOT_DIR + "screen_capture_mic_open_close_open.pcm";
    aFile = fopen(name.c_str(), "w+");
    if (aFile == nullptr) {
        cout << "aFile audio open failed, " << strerror(errno) << endl;
    }

    aFlag = 1;
    vFlag = 1;
    bool isMicrophone = true;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(5);
    isMicrophone = false;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    sleep(3);
    isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    sleep(3);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
}

/**
 * @tc.name: screen_capture_mic_close_open_close
 * @tc.desc: screen capture mic test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_mic_close_open_close, TestSize.Level2)
{
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;

    std::string name = SCREEN_CAPTURE_ROOT_DIR + "screen_capture_mic_close_open_close.pcm";
    aFile = fopen(name.c_str(), "w+");
    if (aFile == nullptr) {
        cout << "aFile audio open failed, " << strerror(errno) << endl;
    }

    aFlag = 1;
    vFlag = 1;
    bool isMicrophone = false;
    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_, aFile, vFile, aFlag, vFlag);
    ASSERT_NE(nullptr, screenCaptureCb_);
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_));
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(5);
    isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    sleep(3);
    isMicrophone = false;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    sleep(3);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
}

/**
 * @tc.name: screen_capture_with_surface_01
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_with_surface_01, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_01 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoFrameWidth = 720;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 1280;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_01 CreateRecorder");
    std::shared_ptr<Recorder> recorder = nullptr;
    recorder = RecorderFactory::CreateRecorder();
    int32_t videoSourceId = 0;
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSource(VIDEO_SOURCE_SURFACE_RGBA, videoSourceId));
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFormat(OutputFormatType::FORMAT_MPEG_4));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncoder(videoSourceId, VideoCodecFormat::H264));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSize(videoSourceId, 720, 1280));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoFrameRate(videoSourceId, 30));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncodingBitRate(videoSourceId, 2000000));
    OpenFileFd("screen_capture_with_surface_01.mp4");
    ASSERT_TRUE(outputFd_ >= 0);
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFile(outputFd_));
    EXPECT_EQ(MSERR_OK, recorder->Prepare());
    sptr<OHOS::Surface> consumer = recorder->GetSurface(videoSourceId);
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCaptureWithSurface(consumer));
    EXPECT_EQ(MSERR_OK, recorder->Start());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_01 recorder Start");
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder->Stop(true));
    EXPECT_EQ(MSERR_OK, recorder->Reset());
    EXPECT_EQ(MSERR_OK, recorder->Release());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_01 after");
}

/**
 * @tc.name: screen_capture_with_surface_02
 * @tc.desc: do screencapture
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_with_surface_02, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_02 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoFrameWidth = 480;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 640;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_02 CreateRecorder");
    std::shared_ptr<Recorder> recorder = nullptr;
    recorder = RecorderFactory::CreateRecorder();
    int32_t videoSourceId = 0;
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSource(VIDEO_SOURCE_SURFACE_RGBA, videoSourceId));
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFormat(OutputFormatType::FORMAT_MPEG_4));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncoder(videoSourceId, VideoCodecFormat::H264));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSize(videoSourceId, 480, 640));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoFrameRate(videoSourceId, 30));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncodingBitRate(videoSourceId, 2000000));
    OpenFileFd("screen_capture_with_surface_02.mp4");
    ASSERT_TRUE(outputFd_ >= 0);
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFile(outputFd_));
    EXPECT_EQ(MSERR_OK, recorder->Prepare());
    sptr<OHOS::Surface> consumer = recorder->GetSurface(videoSourceId);
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCaptureWithSurface(consumer));
    EXPECT_EQ(MSERR_OK, recorder->Start());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_02 recorder Start");
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder->Stop(true));
    EXPECT_EQ(MSERR_OK, recorder->Reset());
    EXPECT_EQ(MSERR_OK, recorder->Release());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_02 after");
}
} // namespace Media
} // namespace OHOS