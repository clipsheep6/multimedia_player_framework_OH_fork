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

using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace std;
using namespace OHOS::Rosen;
using namespace OHOS::Media::ScreenCaptureTestParam;

namespace OHOS {
namespace Media {
void ScreenCaptureUnitTestCallback::OnError(int32_t errorCode)
{
    cout << "Error received, errorCode:" << errorCode << endl;
}

void ScreenCaptureUnitTestCallback::OnAudioBufferAvailable(bool isReady, AudioCaptureSourceType type)
{
    if (isReady) {
        std::shared_ptr<AudioBuffer> audioBuffer = nullptr;
        if (screenCapture_->AcquireAudioBuffer(audioBuffer, type) == MSERR_OK) {
            if (audioBuffer == nullptr) {
                cout << "AcquireAudioBuffer failed, audio buffer empty" << endl;
            }
            cout << "AcquireAudioBuffer, audioBufferLen:" << audioBuffer->length << ", timestampe:"
                << audioBuffer->timestamp << ", audioSourceType:" << audioBuffer->sourcetype << endl;
            DumpAudioBuffer(audioBuffer);
        }
        if (aFlag_ == 1) {
            screenCapture_->ReleaseAudioBuffer(type);
        }
    } else {
        cout << "AcquireAudioBuffer failed" << endl;
    }
}

void ScreenCaptureUnitTestCallback::DumpAudioBuffer(std::shared_ptr<AudioBuffer> audioBuffer)
{
    if ((aFile_ != nullptr) && (audioBuffer->buffer != nullptr)) {
        if (fwrite(audioBuffer->buffer, 1, audioBuffer->length, aFile_) != audioBuffer->length) {
            cout << "error occurred in fwrite:" << strerror(errno) <<endl;
        }
    }
}

void ScreenCaptureUnitTestCallback::OnVideoBufferAvailable(bool isReady)
{
    if (isReady) {
        int32_t fence = 0;
        int64_t timestamp = 0;
        OHOS::Rect damage;
        sptr<OHOS::SurfaceBuffer> surfacebuffer = screenCapture_->AcquireVideoBuffer(fence, timestamp, damage);
        if (surfacebuffer != nullptr) {
            int32_t length = surfacebuffer->GetSize();
            cout << "AcquireVideoBuffer, videoBufferLen:" << surfacebuffer->GetSize() << ", timestamp:"
                << timestamp << ", size:"<< length << endl;
            DumpVideoBuffer(surfacebuffer);
            if (vFlag_ == 1) {
                screenCapture_->ReleaseVideoBuffer();
            }
        } else {
            cout << "AcquireVideoBuffer failed" << endl;
        }
    }
}

void ScreenCaptureUnitTestCallback::DumpVideoBuffer(sptr<OHOS::SurfaceBuffer> surfacebuffer)
{
    if (vFile_ != nullptr) {
        if (fwrite(surfacebuffer->GetVirAddr(), 1, surfacebuffer->GetSize(), vFile_) != surfacebuffer->GetSize()) {
            cout << "error occurred in fwrite:" << strerror(errno) <<endl;
        }
    }
}

void ScreenCaptureUnitTest::SetUpTestCase(void)
{
    system("param set debug.media_service.histreamer 0");
}

void ScreenCaptureUnitTest::TearDownTestCase(void)
{
    system("param set debug.media_service.histreamer 0");
}

void ScreenCaptureUnitTest::SetUp(void)
{
    screenCapture_ = ScreenCaptureMockFactory::CreateScreenCapture();
    ASSERT_NE(nullptr, screenCapture_);
}

void ScreenCaptureUnitTest::TearDown(void)
{
    if (screenCapture_ != nullptr) {
        screenCapture_->Release();
    }
}

int32_t ScreenCaptureUnitTest::SetConfig(AVScreenCaptureConfig &config)
{
    AudioCaptureInfo miccapinfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = SOURCE_DEFAULT
    };

    VideoCaptureInfo videocapinfo = {
        .videoFrameWidth = 720,
        .videoFrameHeight = 1280,
        .videoSource = VIDEO_SOURCE_SURFACE_RGBA
    };

    AudioInfo audioinfo = {
        .micCapInfo = miccapinfo,
    };

    VideoInfo videoinfo = {
        .videoCapInfo = videocapinfo
    };

    config = {
        .captureMode = CAPTURE_HOME_SCREEN,
        .dataType = ORIGINAL_STREAM,
        .audioInfo = audioinfo,
        .videoInfo = videoinfo
    };
    return MSERR_OK;
}

int32_t ScreenCaptureUnitTest::SetConfigFile(AVScreenCaptureConfig &config, RecorderInfo &recorderInfo)
{
    AudioEncInfo audioEncInfo = {
        .audioBitrate = 48000,
        .audioCodecformat = AudioCodecFormat::AAC_LC
    };

    VideoCaptureInfo videoCapInfo = {
        .videoFrameWidth = 720,
        .videoFrameHeight = 1080,
        .videoSource = VideoSourceType::VIDEO_SOURCE_SURFACE_RGBA
    };

    VideoEncInfo videoEncInfo = {
        .videoCodec = VideoCodecFormat::MPEG4,
        .videoBitrate = 2000000,
        .videoFrameRate = 30
    };

    AudioInfo audioInfo = {
        .audioEncInfo = audioEncInfo
    };

    VideoInfo videoInfo = {
        .videoCapInfo = videoCapInfo,
        .videoEncInfo = videoEncInfo
    };

    config = {
        .captureMode = CaptureMode::CAPTURE_HOME_SCREEN,
        .dataType = DataType::CAPTURE_FILE,
        .audioInfo = audioInfo,
        .videoInfo = videoInfo,
        .recorderInfo = recorderInfo
    };
    return MSERR_OK;
}

void ScreenCaptureUnitTest::OpenFile(std::string filename_)
{
    if (snprintf_s(filename, sizeof(filename), sizeof(filename) - 1, "/data/screen_capture/%s.pcm",
        filename_.c_str()) >= 0) {
        aFile = fopen(filename, "w+");
        if (aFile == nullptr) {
            cout << "aFile audio open failed, " << strerror(errno) << endl;
        }
    } else {
        cout << "snprintf audio file failed, " << strerror(errno) << endl;
        return;
    }
    if (snprintf_s(filename, sizeof(filename), sizeof(filename) - 1, "/data/screen_capture/%s.yuv",
        filename_.c_str()) >= 0) {
        vFile = fopen(filename, "w+");
        if (vFile == nullptr) {
            cout << "vFile video open failed, " << strerror(errno) << endl;
        }
    } else {
        cout << "snprintf video file failed, " << strerror(errno) << endl;
        return;
    }
}

void ScreenCaptureUnitTest::CloseFile(void)
{
    if (aFile != nullptr) {
        fclose(aFile);
        aFile = nullptr;
    }
    if (vFile != nullptr) {
        fclose(vFile);
        vFile = nullptr;
    }
}

void ScreenCaptureUnitTest::AudioLoop(void)
{
    int index_ = 200;
    int index_audio_frame = 0;
    while (index_) {
        if (screenCapture_ == nullptr) {
            break;
        }
        std::shared_ptr<AudioBuffer> audioBuffer = nullptr;
        AudioCaptureSourceType type = MIC;
        if (screenCapture_->AcquireAudioBuffer(audioBuffer, type) == MSERR_OK) {
            if (audioBuffer == nullptr) {
                cout << "AcquireAudioBuffer failed, audio buffer is nullptr" << endl;
                continue;
            }
            cout << "index audio:" << index_audio_frame++ << ", AcquireAudioBuffer, audioBufferLen:"
                << audioBuffer->length << ", timestampe:" << audioBuffer->timestamp << ", audioSourceType:"
                << audioBuffer->sourcetype << endl;
            screenCapture_->ReleaseAudioBuffer(type);
        } else {
            cout << "AcquireAudioBuffer failed" << endl;
        }
        index_--;
    }
}

void ScreenCaptureUnitTest::AudioLoopWithoutRelease(void)
{
    int index_ = 200;
    int index_audio_frame = 0;
    while (index_) {
        if (screenCapture_ == nullptr) {
            break;
        }
        std::shared_ptr<AudioBuffer> audioBuffer = nullptr;
        AudioCaptureSourceType type = MIC;
        if (screenCapture_->AcquireAudioBuffer(audioBuffer, type) == MSERR_OK) {
            if (audioBuffer == nullptr) {
                cout << "AcquireAudioBuffer failed, audio buffer is nullptr" << endl;
                continue;
            }
            cout << "index audio:" << index_audio_frame++ << ", AcquireAudioBuffer, audioBufferLen:"
                << audioBuffer->length << ", timestampe:" << audioBuffer->timestamp << ", audioSourceType:"
                << audioBuffer->sourcetype << endl;
        } else {
            cout << "AcquireAudioBuffer failed" << endl;
        }
        index_--;
    }
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
    AVScreenCaptureConfig config_;
    RecorderInfo recorderInfo;
    int32_t outputFd = open((screenCaptureRoot + "screen_capture_get_screen_capture_01.mp4").c_str(),
        O_RDWR | O_CREAT, 0777);
    recorderInfo.url = "fd://" + to_string(outputFd);
    recorderInfo.fileFormat = "mp4";
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::APP_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
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
    AVScreenCaptureConfig config_;
    RecorderInfo recorderInfo;
    int32_t outputFd = open((screenCaptureRoot + "screen_capture_get_screen_capture_02.mp4").c_str(),
        O_RDWR | O_CREAT, 0777);
    recorderInfo.url = "fd://" + to_string(outputFd);
    recorderInfo.fileFormat = "mp4";
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo micCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::MIC
    };
    config_.audioInfo.micCapInfo = micCapInfo;

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
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
    AVScreenCaptureConfig config_;
    RecorderInfo recorderInfo;
    int32_t outputFd = open((screenCaptureRoot + "screen_capture_get_screen_capture_03.mp4").c_str(),
        O_RDWR | O_CREAT, 0777);
    recorderInfo.url = "fd://" + to_string(outputFd);
    recorderInfo.fileFormat = "mp4";
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
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_03 after");
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
    AVScreenCaptureConfig config_;
    RecorderInfo recorderInfo;
    int32_t outputFd = open((screenCaptureRoot + "screen_capture_check_param_01.mp4").c_str(),
        O_RDWR | O_CREAT, 0777);
    recorderInfo.url = "fd://" + to_string(outputFd);
    recorderInfo.fileFormat = "mp4";
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo micCapInfoRateSmall = {
        .audioSampleRate = 0,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::MIC
    };
    config_.audioInfo.micCapInfo = micCapInfoRateSmall;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    AudioCaptureInfo micCapInfoRateBig = {
        .audioSampleRate = 8000000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::MIC
    };
    config_.audioInfo.micCapInfo = micCapInfoRateBig;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    AudioCaptureInfo micCapInfoChannelSmall = {
        .audioSampleRate = 16000,
        .audioChannels = 0,
        .audioSource = AudioCaptureSourceType::MIC
    };
    config_.audioInfo.micCapInfo = micCapInfoChannelSmall;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    AudioCaptureInfo micCapInfoChannelBig = {
        .audioSampleRate = 16000,
        .audioChannels = 200,
        .audioSource = AudioCaptureSourceType::MIC
    };
    config_.audioInfo.micCapInfo = micCapInfoChannelBig;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    AudioCaptureInfo micCapInfoTypeError = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::APP_PLAYBACK
    };
    config_.audioInfo.micCapInfo = micCapInfoTypeError;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
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
    AVScreenCaptureConfig config_;
    RecorderInfo recorderInfo;
    int32_t outputFd = open((screenCaptureRoot + "screen_capture_check_param_02.mp4").c_str(),
        O_RDWR | O_CREAT, 0777);
    recorderInfo.url = "fd://" + to_string(outputFd);
    recorderInfo.fileFormat = "mp4";
    SetConfigFile(config_, recorderInfo);
    AudioEncInfo audioEncInfoBitSmall = {
        .audioBitrate = 0,
        .audioCodecformat = AudioCodecFormat::AAC_LC
    };
    config_.audioInfo.audioEncInfo = audioEncInfoBitSmall;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    AudioEncInfo audioEncInfoBitBig = {
        .audioBitrate = 4800000,
        .audioCodecformat = AudioCodecFormat::AAC_LC
    };
    config_.audioInfo.audioEncInfo = audioEncInfoBitBig;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    AudioEncInfo audioEncInfoFormatError = {
        .audioBitrate = 48000,
        .audioCodecformat = AudioCodecFormat::AUDIO_CODEC_FORMAT_BUTT
    };
    config_.audioInfo.audioEncInfo = audioEncInfoFormatError;
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
    AVScreenCaptureConfig config_;
    RecorderInfo recorderInfo;
    int32_t outputFd = open((screenCaptureRoot + "screen_capture_check_param_03.mp4").c_str(),
        O_RDWR | O_CREAT, 0777);
    recorderInfo.url = "fd://" + to_string(outputFd);
    recorderInfo.fileFormat = "mp4";
    SetConfigFile(config_, recorderInfo);
    VideoCaptureInfo videoCapInfoWidthSmall = {
        .videoFrameWidth = 0,
        .videoFrameHeight = 1080,
        .videoSource = VideoSourceType::VIDEO_SOURCE_SURFACE_RGBA
    };
    config_.videoInfo.videoCapInfo = videoCapInfoWidthSmall;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    VideoCaptureInfo videoCapInfoWidthBig = {
        .videoFrameWidth = 7200,
        .videoFrameHeight = 1080,
        .videoSource = VideoSourceType::VIDEO_SOURCE_SURFACE_RGBA
    };
    config_.videoInfo.videoCapInfo = videoCapInfoWidthBig;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    VideoCaptureInfo videoCapInfoHeightSmall = {
        .videoFrameWidth = 720,
        .videoFrameHeight = 0,
        .videoSource = VideoSourceType::VIDEO_SOURCE_SURFACE_RGBA
    };
    config_.videoInfo.videoCapInfo = videoCapInfoHeightSmall;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    VideoCaptureInfo videoCapInfoHeightBig = {
        .videoFrameWidth = 720,
        .videoFrameHeight = 10800,
        .videoSource = VideoSourceType::VIDEO_SOURCE_SURFACE_RGBA
    };
    config_.videoInfo.videoCapInfo = videoCapInfoHeightBig;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    VideoCaptureInfo videoCapInfoSourceError = {
        .videoFrameWidth = 720,
        .videoFrameHeight = 1080,
        .videoSource = VideoSourceType::VIDEO_SOURCE_BUTT
    };
    config_.videoInfo.videoCapInfo = videoCapInfoSourceError;
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
    AVScreenCaptureConfig config_;
    RecorderInfo recorderInfo;
    int32_t outputFd = open((screenCaptureRoot + "screen_capture_check_param_04.mp4").c_str(),
        O_RDWR | O_CREAT, 0777);
    recorderInfo.url = "fd://" + to_string(outputFd);
    recorderInfo.fileFormat = "mp4";
    SetConfigFile(config_, recorderInfo);
    VideoEncInfo videoEncInfoBitSmall = {
        .videoCodec = VideoCodecFormat::MPEG4,
        .videoBitrate = 0,
        .videoFrameRate = 30
    };
    config_.videoInfo.videoEncInfo = videoEncInfoBitSmall;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    VideoEncInfo videoEncInfoBitBig = {
        .videoCodec = VideoCodecFormat::MPEG4,
        .videoBitrate = 20000000,
        .videoFrameRate = 30
    };
    config_.videoInfo.videoEncInfo = videoEncInfoBitBig;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    VideoEncInfo videoEncInfoRateSmall = {
        .videoCodec = VideoCodecFormat::MPEG4,
        .videoBitrate = 2000000,
        .videoFrameRate = 0
    };
    config_.videoInfo.videoEncInfo = videoEncInfoRateSmall;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    VideoEncInfo videoEncInfoRateBig = {
        .videoCodec = VideoCodecFormat::MPEG4,
        .videoBitrate = 2000000,
        .videoFrameRate = 300
    };
    config_.videoInfo.videoEncInfo = videoEncInfoRateBig;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    VideoEncInfo videoEncInfoCodecError = {
        .videoCodec = VideoCodecFormat::VIDEO_CODEC_FORMAT_BUTT,
        .videoBitrate = 2000000,
        .videoFrameRate = 30
    };
    config_.videoInfo.videoEncInfo = videoEncInfoCodecError;
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
    AVScreenCaptureConfig config_;
    RecorderInfo recorderInfo;
    int32_t outputFd = open((screenCaptureRoot + "screen_capture_check_param_05.mp4").c_str(),
        O_RDWR | O_CREAT, 0777);
    recorderInfo.url = "fd://" + to_string(outputFd);
    recorderInfo.fileFormat = "mp4";
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::APP_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;

    config_.dataType = DataType::ENCODED_STREAM;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    config_.dataType = DataType::INVAILD;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    config_.dataType = DataType::CAPTURE_FILE;
    config_.captureMode = CaptureMode::CAPTURE_INVAILD;
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));
    config_.captureMode = CaptureMode::CAPTURE_SPECIFIED_WINDOW;
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
    AVScreenCaptureConfig config_;
    RecorderInfo recorderInfo;
    int32_t outputFd = open((screenCaptureRoot + "screen_capture_check_param_06.mp4").c_str(),
        O_RDWR | O_CREAT, 0777);
    recorderInfo.url = "fd://" + to_string(outputFd);
    recorderInfo.fileFormat = "avi";
    SetConfigFile(config_, recorderInfo);
    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = AudioCaptureSourceType::APP_PLAYBACK
    };
    config_.audioInfo.innerCapInfo = innerCapInfo;

    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    recorderInfo.fileFormat = "mp4";
    recorderInfo.url = "http://" + to_string(outputFd);
    EXPECT_NE(MSERR_OK, screenCapture_->Init(config_));

    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_check_param_06 after");
}

/**
 * @tc.name: screen_capture_video_configure_0001
 * @tc.desc: init with videoFrameWidth -1
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_video_configure_0001, TestSize.Level2)
{
    AVScreenCaptureConfig config_;
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
    AVScreenCaptureConfig config_;
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
    AVScreenCaptureConfig config_;
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
    AVScreenCaptureConfig config_;
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
    AVScreenCaptureConfig config_;
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
    AVScreenCaptureConfig config_;
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
    AVScreenCaptureConfig config_;
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
    AVScreenCaptureConfig config_;
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
    AVScreenCaptureConfig config_;
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
    AVScreenCaptureConfig config_;
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
    AVScreenCaptureConfig config_;
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
    AVScreenCaptureConfig config_;
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
    AVScreenCaptureConfig config_;
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
    AVScreenCaptureConfig config_;
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
    AVScreenCaptureConfig config_;
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
    AVScreenCaptureConfig config_;
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
    AVScreenCaptureConfig config_;
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoFrameWidth = 160;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 160;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    vFile = fopen("/data/screen_capture/screen_capture_video_size_0001.yuv", "w+");
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
    AVScreenCaptureConfig config_;
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoFrameWidth = 640;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 480;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    vFile = fopen("/data/screen_capture/screen_capture_video_size_0002.yuv", "w+");
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
    AVScreenCaptureConfig config_;
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoFrameWidth = 1920;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 1080;
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    vFile = fopen("/data/screen_capture/screen_capture_video_size_0003.yuv", "w+");
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
    AVScreenCaptureConfig config_;
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    sptr<Display> display = DisplayManager::GetInstance().GetDefaultDisplaySync();
    ASSERT_NE(display, nullptr);
    cout << "get displayinfo: " << endl;
    cout << "width: " << display->GetWidth() << "; height: " << display->GetHeight() << "; density: "
        << display->GetDpi() << "; refreshRate: " << display->GetRefreshRate() << endl;

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
    AVScreenCaptureConfig config_;
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
            cout << "index video:" << index_video_frame++ << "; AcquireVideoBuffer, videoBufferLen:"
                << surfacebuffer->GetSize() << ", timestamp:" << timestamp << ", size:"<< length << endl;
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
    AVScreenCaptureConfig config_;
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
            cout << "index video:" << index_video_frame++ << "; AcquireVideoBuffer, videoBufferLen:"
                << surfacebuffer->GetSize() << ", timestamp:" << timestamp << ", size:"<< length << endl;
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
    AVScreenCaptureConfig config_;
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
    AVScreenCaptureConfig config_;
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
    AVScreenCaptureConfig config_;
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
 * @tc.name: screen_capture_mic_open_close_open
 * @tc.desc: screen capture mic test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_mic_open_close_open, TestSize.Level2)
{
    AVScreenCaptureConfig config_;
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    aFile = fopen("/data/screen_capture/screen_capture_mic_open_close_open.pcm", "w+");
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
    AVScreenCaptureConfig config_;
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    aFile = fopen("/data/screen_capture/screen_capture_mic_close_open_close.pcm", "w+");
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
 * @tc.name: screen_capture_displayId
 * @tc.desc: screen capture displayId test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_displayId, TestSize.Level1)
{
    AVScreenCaptureConfig config_;
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    config_.videoInfo.videoCapInfo.displayId = 10;
    OpenFile("screen_capture_displayId");

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
}

/**
 * @tc.name: screen_capture_taskIDs
 * @tc.desc: screen capture taskIDs test
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_taskIDs, TestSize.Level1)
{
    AVScreenCaptureConfig config_;
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoSource = VIDEO_SOURCE_SURFACE_RGBA;
    int32_t num[] = { 111, 222, 333, 444, 555 };
    list<int32_t> listInt_A(num, num + size(num));

    (config_.videoInfo.videoCapInfo.taskIDs).assign(++listInt_A.begin(), --listInt_A.end());
    cout << "taskIDs: ";
    for (list<int>::iterator it = (config_.videoInfo.videoCapInfo.taskIDs).begin();
        it != (config_.videoInfo.videoCapInfo.taskIDs).end(); it++)
    {
        cout << *it << " ";
    }
    cout << endl;
    OpenFile("screen_capture_taskIDs");

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
}
} // namespace Media
} // namespace OHOS