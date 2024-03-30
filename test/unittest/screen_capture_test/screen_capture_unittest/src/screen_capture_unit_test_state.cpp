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

using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace std;
using namespace OHOS::Rosen;
using namespace OHOS::Media::ScreenCaptureTestParam;

namespace OHOS {
namespace Media {
namespace {
    constexpr int32_t FLIE_CREATE_FLAGS = 0777;
}
void ScreenCaptureUnitTestCallback::OnError(int32_t errorCode)
{
    ASSERT_FALSE(screenCapture_->IsErrorCallBackEnabled());
    cout << "Error received, errorCode:" << errorCode << endl;
}

void ScreenCaptureUnitTestCallback::OnAudioBufferAvailable(bool isReady, AudioCaptureSourceType type)
{
    cout << "OnAudioBufferAvailable S, isReady:" << isReady << endl;
    ASSERT_FALSE(screenCapture_->IsDataCallBackEnabled());
    if (!isReady) {
        cout << "OnAudioBufferAvailable isReady false E" << endl;
        return;
    }
    std::shared_ptr<AudioBuffer> audioBuffer = nullptr;
    if (screenCapture_->AcquireAudioBuffer(audioBuffer, type) == MSERR_OK) {
        if (audioBuffer == nullptr || audioBuffer->buffer == nullptr) {
            cout << "AcquireAudioBuffer failed, audio buffer empty, PLEASE CHECK IF IT IS OK!!!" <<
                "audioBuffer:" << (audioBuffer == nullptr) << endl;
            return;
        }
        cout << "AcquireAudioBuffer, audioBufferLen:" << audioBuffer->length <<
            ", timestampe:" << audioBuffer->timestamp << ", audioSourceType:" << audioBuffer->sourcetype << endl;
        DumpAudioBuffer(audioBuffer);
    }
    if (!screenCapture_->IsStateChangeCallBackEnabled()) {
        if (aFlag_ == 1) {
            cout << "OnAudioBufferAvailable ReleaseAudioBuffer" << endl;
            screenCapture_->ReleaseAudioBuffer(type);
        }
    } else {
        cout << "OnAudioBufferAvailable ReleaseAudioBuffer" << endl;
        screenCapture_->ReleaseAudioBuffer(type);
    }
    cout << "OnAudioBufferAvailable isReady true E" << endl;
}

void ScreenCaptureUnitTestCallback::DumpAudioBuffer(std::shared_ptr<AudioBuffer> audioBuffer)
{
    if ((audioBuffer == nullptr) || (audioBuffer->buffer == nullptr)) {
        cout << "DumpAudioBuffer audioBuffer or audioBuffer->buffer is nullptr:" << (audioBuffer == nullptr) << endl;
        return;
    }
    if (!screenCapture_->IsStateChangeCallBackEnabled()) {
        if (aFile_ == nullptr) {
            cout << "DumpAudioBuffer aFile_ is nullptr" << endl;
            return;
        }
        if (fwrite(audioBuffer->buffer, 1, audioBuffer->length, aFile_) != static_cast<size_t>(audioBuffer->length)) {
            cout << "DumpAudioBuffer error occurred in fwrite:" << strerror(errno) <<endl;
        }
        return;
    }

    AudioCaptureSourceType type = audioBuffer->sourcetype;
    AVScreenCaptureBufferType bufferType = AVScreenCaptureBufferType::SCREEN_CAPTURE_BUFFERTYPE_INVALID;
    if (type == AudioCaptureSourceType::SOURCE_DEFAULT || type == AudioCaptureSourceType::MIC) {
        bufferType = AVScreenCaptureBufferType::SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC;
        DumpBuffer(micAudioFile_, audioBuffer->buffer, audioBuffer->length, audioBuffer->timestamp, bufferType);
        return;
    }
    if (type == AudioCaptureSourceType::ALL_PLAYBACK || type == AudioCaptureSourceType::APP_PLAYBACK) {
        bufferType = AVScreenCaptureBufferType::SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER;
        DumpBuffer(innerAudioFile_, audioBuffer->buffer, audioBuffer->length, audioBuffer->timestamp, bufferType);
        return;
    }
    cout << "DumpAudioBuffer invalid bufferType:" << bufferType << ", type:" << type << endl;
}

void ScreenCaptureUnitTestCallback::OnVideoBufferAvailable(bool isReady)
{
    cout << "OnVideoBufferAvailable S, isReady:" << isReady << endl;
    ASSERT_FALSE(screenCapture_->IsDataCallBackEnabled());
    if (!isReady) {
        cout << "OnVideoBufferAvailable isReady false E" << endl;
        return;
    }
    cout << "OnVideoBufferAvailable isReady true" << endl;
    int32_t fence = 0;
    int64_t timestamp = 0;
    OHOS::Rect damage;
    sptr<OHOS::SurfaceBuffer> surfacebuffer = screenCapture_->AcquireVideoBuffer(fence, timestamp, damage);
    if (surfacebuffer == nullptr) {
        cout << "OnVideoBufferAvailable isReady true, AcquireVideoBuffer failed" << endl;
        return;
    }
    int32_t length = surfacebuffer->GetSize();
    cout << "AcquireVideoBuffer, videoBufferLen:" << surfacebuffer->GetSize() <<
        ", timestamp:" << timestamp << ", size:"<< length << endl;
    DumpVideoBuffer(surfacebuffer, timestamp);
    if (!screenCapture_->IsStateChangeCallBackEnabled()) {
        if (vFlag_ == 1) {
            cout << "OnVideoBufferAvailable ReleaseVideoBuffer" << endl;
            screenCapture_->ReleaseVideoBuffer();
        }
    } else {
        cout << "OnAudioBufferAvailable ReleaseVideoBuffer" << endl;
        screenCapture_->ReleaseVideoBuffer();
    }
    cout << "OnVideoBufferAvailable isReady true E" << endl;
}

void ScreenCaptureUnitTestCallback::DumpVideoBuffer(sptr<OHOS::SurfaceBuffer> surfacebuffer, int64_t timestamp)
{
    if ((surfacebuffer == nullptr) || (surfacebuffer->GetVirAddr() == nullptr)) {
        cout << "DumpVideoBuffer surfacebuffer or surfacebuffer->GetVirAddr() is nullptr:" <<
            (surfacebuffer == nullptr) << endl;
        return;
    }
    if (!screenCapture_->IsStateChangeCallBackEnabled()) {
        if (vFile_ == nullptr) {
            cout << "DumpVideoBuffer vFile_ is nullptr" << endl;
            return;
        }
        if (fwrite(surfacebuffer->GetVirAddr(), 1, surfacebuffer->GetSize(), vFile_) != surfacebuffer->GetSize()) {
            cout << "DumpVideoBuffer error occurred in fwrite:" << strerror(errno) <<endl;
        }
        return;
    }
    DumpBuffer(videoFile_, reinterpret_cast<uint8_t *>(surfacebuffer->GetVirAddr()), surfacebuffer->GetSize(),
        timestamp, AVScreenCaptureBufferType::SCREEN_CAPTURE_BUFFERTYPE_VIDEO);
}

void ScreenCaptureUnitTestCallback::OnError(int32_t errorCode, void *userData)
{
    ASSERT_TRUE(screenCapture_->IsErrorCallBackEnabled());
    cout << "Error received, errorCode:" << errorCode << ", userData:" << userData << endl;
}

void ScreenCaptureUnitTestCallback::OnStateChange(AVScreenCaptureStateCode stateCode)
{
    MEDIA_LOGI("ScreenCaptureUnitTestCallback::OnStateChange stateCode:%{public}d", stateCode);
    screenCaptureState_ = stateCode;
    cout << "OnStateChange received, stateCode:" << stateCode << endl;
}

void ScreenCaptureUnitTestCallback::OnBufferAvailable(std::shared_ptr<AVBuffer> buffer,
    AVScreenCaptureBufferType bufferType, int64_t timestamp)
{
    ASSERT_TRUE(screenCapture_->IsDataCallBackEnabled());
    MEDIA_LOGD("ScreenCaptureUnitTestCallback::OnBufferAvailable bufferType:%{public}d, timestamp:%{public}" PRId64,
        bufferType, timestamp);
    cout << "OnBufferAvailable received, bufferType:" << bufferType << ", timestamp:" << timestamp << endl;
    if (buffer == nullptr || buffer->memory_ == nullptr) {
        return;
    }
    switch (bufferType) {
        case AVScreenCaptureBufferType::SCREEN_CAPTURE_BUFFERTYPE_VIDEO: {
            sptr<OHOS::SurfaceBuffer> surfaceBuffer = buffer->memory_->GetSurfaceBuffer();
            if (surfaceBuffer == nullptr || surfaceBuffer->GetVirAddr() == nullptr) {
                MEDIA_LOGW("OnBufferAvailable videoBuffer received is nullptr, PLEASE CHECK IF IT IS OK!!!"
                    "bufferType:%{public}d, timestamp:%{public}" PRId64, bufferType, timestamp);
                cout << "OnBufferAvailable videoBuffer received is nullptr, PLEASE CHECK IF IT IS OK!!!" <<
                    "bufferType:" << bufferType << ", timestamp:" << timestamp << (surfaceBuffer == nullptr) <<endl;
                return;
            }
            ProcessVideoBuffer(surfaceBuffer, timestamp);
            break;
        }
        case AVScreenCaptureBufferType::SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER: // fall-through
        case AVScreenCaptureBufferType::SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC: {
            uint8_t *auduioBuffer = buffer->memory_->GetAddr();
            if (auduioBuffer == nullptr) {
                MEDIA_LOGW("OnBufferAvailable audioBuffer received is nullptr, PLEASE CHECK IF IT IS OK!!!"
                    "bufferType:%{public}d, timestamp:%{public}" PRId64, bufferType, timestamp);
                cout << "OnBufferAvailable audioBuffer received is nullptr, PLEASE CHECK IF IT IS OK!!!" <<
                    "bufferType:" << bufferType << ", timestamp:" << timestamp << endl;
                return;
            }
            ProcessAudioBuffer(auduioBuffer, buffer->memory_->GetSize(), timestamp, bufferType);
            break;
        }
        default:
            cout << "OnBufferAvailable received invalid bufferType:" << bufferType <<
                ", timestamp:" << timestamp << endl;
            break;
    }
}

void ScreenCaptureUnitTestCallback::DumpBuffer(FILE *file, uint8_t *buffer, int32_t size, int64_t timestamp,
    AVScreenCaptureBufferType bufferType)
{
    cout << "DumpBuffer, bufferLen:" << size << ", bufferType:" << bufferType << ", timestamp:" << timestamp << endl;
    if ((file == nullptr) || (buffer == nullptr)) {
        cout << "DumpBuffer failed, file:" << (file == nullptr) << ", buffer:" << (buffer == nullptr) <<
            ", bufferLen:" << size << ", bufferType:" << bufferType << ", timestamp:" << timestamp << endl;
        return;
    }
    if (fwrite(buffer, 1, size, file) != static_cast<size_t>(size)) {
        cout << "error occurred in fwrite:" << strerror(errno) << ", bufferType:" << bufferType <<
            ", timestamp:" << timestamp << endl;
    }
}

void ScreenCaptureUnitTestCallback::CheckDataCallbackVideo(int32_t flag)
{
    if (flag != 1) {
        return;
    }
    if (screenCapture_->IsDataCallBackEnabled()) {
        int32_t fence = 0;
        int64_t timestamp = 0;
        OHOS::Rect damage;
        sptr<OHOS::SurfaceBuffer> surfacebuffer = screenCapture_->AcquireVideoBuffer(fence, timestamp, damage);
        EXPECT_EQ(nullptr, surfacebuffer);
        EXPECT_NE(MSERR_OK, screenCapture_->ReleaseVideoBuffer());
    }
}

void ScreenCaptureUnitTestCallback::ProcessVideoBuffer(sptr<OHOS::SurfaceBuffer> surfacebuffer, int64_t timestamp)
{
    int32_t size = surfacebuffer->GetSize();
    DumpBuffer(videoFile_, reinterpret_cast<uint8_t *>(surfacebuffer->GetVirAddr()), size, timestamp);
    CheckDataCallbackVideo(videoFlag_);
}

void ScreenCaptureUnitTestCallback::CheckDataCallbackAudio(AudioCaptureSourceType type, int32_t flag)
{
    if (flag != 1) {
        return;
    }
    if (screenCapture_->IsDataCallBackEnabled()) {
        std::shared_ptr<AudioBuffer> audioBuffer = nullptr;
        EXPECT_NE(MSERR_OK, screenCapture_->AcquireAudioBuffer(audioBuffer, type));
        EXPECT_NE(MSERR_OK, screenCapture_->ReleaseAudioBuffer(type));
    }
}

void ScreenCaptureUnitTestCallback::ProcessAudioBuffer(uint8_t *buffer, int32_t size, int64_t timestamp,
    AVScreenCaptureBufferType bufferType)
{
    if (bufferType == AVScreenCaptureBufferType::SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER) {
        DumpBuffer(innerAudioFile_, buffer, size, timestamp, bufferType);
        CheckDataCallbackAudio(AudioCaptureSourceType::ALL_PLAYBACK, innerAudioFlag_);
        return;
    }
    if (bufferType == AVScreenCaptureBufferType::SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC) {
        DumpBuffer(micAudioFile_, buffer, size, timestamp, bufferType);
        CheckDataCallbackAudio(AudioCaptureSourceType::MIC, micAudioFlag_);
        return;
    }
    cout << "ProcessAudioBuffer invalid bufferType:" << bufferType << endl;
}

void ScreenCaptureUnitTestCallback::InitCaptureTrackInfo(FILE *file, int32_t flag, AVScreenCaptureBufferType bufferType)
{
    if (bufferType == AVScreenCaptureBufferType::SCREEN_CAPTURE_BUFFERTYPE_VIDEO) {
        videoFile_ = file;
        videoFlag_ = flag;
    }
    if (bufferType == AVScreenCaptureBufferType::SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER) {
        innerAudioFile_ = file;
        innerAudioFlag_ = flag;
    }
    if (bufferType == AVScreenCaptureBufferType::SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC) {
        micAudioFile_ = file;
        micAudioFlag_ = flag;
    }
}

const std::string ScreenCaptureUnitTest::SCREEN_CAPTURE_ROOT_DIR = "/data/test/media/";

void ScreenCaptureUnitTest::SetUpTestCase(void) {}

void ScreenCaptureUnitTest::TearDownTestCase(void) {}

void ScreenCaptureUnitTest::SetUp(void)
{
    screenCapture_ = ScreenCaptureMockFactory::CreateScreenCapture();
    ASSERT_NE(nullptr, screenCapture_);
}

void ScreenCaptureUnitTest::TearDown(void)
{
    if (screenCaptureCb_ != nullptr) {
        screenCaptureCb_.reset();
        screenCaptureCb_ = nullptr;
    }
    if (screenCapture_ != nullptr) {
        screenCapture_->Release();
        screenCapture_ = nullptr;
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

    VideoEncInfo videoEncInfo = {
        .videoCodec = VideoCodecFormat::H264,
        .videoBitrate = 2000000,
        .videoFrameRate = 30
    };

    AudioInfo audioinfo = {
        .micCapInfo = miccapinfo,
    };

    VideoInfo videoinfo = {
        .videoCapInfo = videocapinfo,
        .videoEncInfo = videoEncInfo
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
        .videoCodec = VideoCodecFormat::H264,
        .videoBitrate = 2000000,
        .videoFrameRate = 30
    };

    AudioCaptureInfo innerCapInfo = {
        .audioSampleRate = 0,
        .audioChannels = 0,
        .audioSource = AudioCaptureSourceType::SOURCE_DEFAULT
    };

    AudioCaptureInfo micCapInfo = {
        .audioSampleRate = 0,
        .audioChannels = 0,
        .audioSource = AudioCaptureSourceType::SOURCE_DEFAULT
    };

    AudioInfo audioInfo = {
        .micCapInfo = micCapInfo,
        .innerCapInfo = innerCapInfo,
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

void ScreenCaptureUnitTest::OpenFile(std::string name, bool isInnerAudioEnabled, bool isMicAudioEnabled,
    bool isVideoEnabled)
{
    if (isInnerAudioEnabled) {
        std::string nameFormt = SCREEN_CAPTURE_ROOT_DIR + "%s_inner.pcm";
        if (snprintf_s(fileName, sizeof(fileName), sizeof(fileName) - 1, nameFormt.c_str(),
            name.c_str()) >= 0) {
            innerAudioFile_ = fopen(fileName, "w+");
            if (innerAudioFile_ == nullptr) {
                cout << "micAudioFile_ open failed, " << strerror(errno) << endl;
            }
        } else {
            cout << "snprintf micAudioFile_ failed, " << strerror(errno) << endl;
            return;
        }
    }
    if (isMicAudioEnabled) {
        std::string nameFormt = SCREEN_CAPTURE_ROOT_DIR + "%s_mic.pcm";
        if (snprintf_s(fileName, sizeof(fileName), sizeof(fileName) - 1, nameFormt.c_str(),
            name.c_str()) >= 0) {
            micAudioFile_ = fopen(fileName, "w+");
            if (micAudioFile_ == nullptr) {
                cout << "micAudioFile_ open failed, " << strerror(errno) << endl;
            }
        } else {
            cout << "snprintf micAudioFile_ failed, " << strerror(errno) << endl;
            return;
        }
    }
    if (isVideoEnabled) {
        std::string nameFormt = SCREEN_CAPTURE_ROOT_DIR + "%s.yuv";
        if (snprintf_s(fileName, sizeof(fileName), sizeof(fileName) - 1, nameFormt.c_str(),
            name.c_str()) >= 0) {
            videoFile_ = fopen(fileName, "w+");
            if (videoFile_ == nullptr) {
                cout << "videoFile_ open failed, " << strerror(errno) << endl;
            }
        } else {
            cout << "snprintf videoFile_ failed, " << strerror(errno) << endl;
            return;
        }
    }
}

int32_t ScreenCaptureUnitTest::SetRecorderInfo(std::string name, RecorderInfo &recorderInfo)
{
    OpenFileFd(name);
    recorderInfo.url = "fd://" + to_string(outputFd_);
    recorderInfo.fileFormat = "mp4";
    return MSERR_OK;
}

void ScreenCaptureUnitTest::OpenFileFd(std::string name)
{
    CloseFile();
    outputFd_ = open((SCREEN_CAPTURE_ROOT_DIR + name).c_str(), O_RDWR | O_CREAT, FLIE_CREATE_FLAGS);
}

void ScreenCaptureUnitTest::OpenFile(std::string name)
{
    CloseFile();
    std::string nameFormt = SCREEN_CAPTURE_ROOT_DIR + "%s.pcm";
    if (snprintf_s(fileName, sizeof(fileName), sizeof(fileName) - 1, nameFormt.c_str(),
        name.c_str()) >= 0) {
        aFile = fopen(fileName, "w+");
        if (aFile == nullptr) {
            cout << "aFile audio open failed, " << strerror(errno) << endl;
        }
    } else {
        cout << "snprintf audio file failed, " << strerror(errno) << endl;
        return;
    }
    nameFormt = SCREEN_CAPTURE_ROOT_DIR + "%s.yuv";
    if (snprintf_s(fileName, sizeof(fileName), sizeof(fileName) - 1, nameFormt.c_str(),
        name.c_str()) >= 0) {
        vFile = fopen(fileName, "w+");
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
    if (videoFile_ != nullptr) {
        fclose(videoFile_);
        videoFile_ = nullptr;
    }
    if (innerAudioFile_ != nullptr) {
        fclose(innerAudioFile_);
        innerAudioFile_ = nullptr;
    }
    if (micAudioFile_ != nullptr) {
        fclose(micAudioFile_);
        micAudioFile_ = nullptr;
    }
    if (outputFd_ != -1) {
        (void)::close(outputFd_);
        outputFd_ = -1;
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
            cout << "index audio:" << index_audio_frame++ << ", AcquireAudioBuffer, audioBufferLen:" <<
                audioBuffer->length << ", timestampe:" << audioBuffer->timestamp << ", audioSourceType:" <<
                audioBuffer->sourcetype << endl;
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
            cout << "index audio:" << index_audio_frame++ << ", AcquireAudioBuffer, audioBufferLen:" <<
                audioBuffer->length << ", timestampe:" << audioBuffer->timestamp << ", audioSourceType:" <<
                audioBuffer->sourcetype << endl;
        } else {
            cout << "AcquireAudioBuffer failed" << endl;
        }
        index_--;
    }
}

/**
 * @tc.name: screen_capture_specified_window_cb_01
 * @tc.desc: open microphone, capture screen, inner audio, mic audio
 * @tc.type: FUNC
 * @tc.require: play something audio resource to get inner audio
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_specified_window_cb_01, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_cb_01 before");
    SetConfig(config_);
    config_.audioInfo.innerCapInfo.audioSampleRate = 16000;
    config_.audioInfo.innerCapInfo.audioChannels = 2;
    config_.audioInfo.innerCapInfo.audioSource = AudioCaptureSourceType::ALL_PLAYBACK;
    config_.captureMode = CaptureMode::CAPTURE_SPECIFIED_WINDOW;
    std::shared_ptr<OHOS::AAFwk::AbilityManagerClient> client_ = OHOS::AAFwk::AbilityManagerClient::GetInstance();
    std::string deviceId = "";
    std::vector<OHOS::AAFwk::MissionInfo> missionInfos;
    auto result = client_->GetMissionInfos(deviceId, 10, missionInfos);
    MEDIA_LOGI("screen_capture_specified_window_cb_01 missionInfos size:%{public}s, result:%{public}d",
        std::to_string(missionInfos.size()).c_str(), result);
    if (missionInfos.size() > 0) {
        for (OHOS::AAFwk::MissionInfo info : missionInfos) {
            MEDIA_LOGI("screen_capture_specified_window_cb_01 missionId:%{public}d", info.id);
            config_.videoInfo.videoCapInfo.taskIDs.push_back(info.id);
        }
    } else {
        MEDIA_LOGE("screen_capture_specified_window_cb_01 GetMissionInfos failed");
    }

    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_);
    ASSERT_NE(nullptr, screenCaptureCb_);
    std::string name = "screen_capture_specified_window_cb_01";
    // track enalbed: inner: true, mic: true, video: true
    OpenFile(name, true, true, true);
    // check track aquire & release: inner: 1, mic: 1, video: 1
    screenCaptureCb_->InitCaptureTrackInfo(innerAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER);
    screenCaptureCb_->InitCaptureTrackInfo(micAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC);
    screenCaptureCb_->InitCaptureTrackInfo(videoFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_VIDEO);
    bool isMicrophone = true; // Enable Mic
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    // callback enabled: errorCallback: true, dataCallback: true, stateChangeCallback: true
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_, true, true, true));

    screenCapture_->SetMicrophoneEnabled(true); // Enable Mic
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED, screenCaptureCb_->GetScreenCaptureState());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_cb_01 after");
}

/**
 * @tc.name: screen_capture_specified_window_cb_02
 * @tc.desc: diable microphone, capture screen, inner audio, mic audio
 * @tc.type: FUNC
 * @tc.require: play something audio resource to get inner audio
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_specified_window_cb_02, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_cb_02 before");
    SetConfig(config_);
    config_.audioInfo.innerCapInfo.audioSampleRate = 16000;
    config_.audioInfo.innerCapInfo.audioChannels = 2;
    config_.audioInfo.innerCapInfo.audioSource = AudioCaptureSourceType::ALL_PLAYBACK;
    config_.captureMode = CaptureMode::CAPTURE_SPECIFIED_WINDOW;
    std::shared_ptr<OHOS::AAFwk::AbilityManagerClient> client_ = OHOS::AAFwk::AbilityManagerClient::GetInstance();
    std::string deviceId = "";
    std::vector<OHOS::AAFwk::MissionInfo> missionInfos;
    auto result = client_->GetMissionInfos(deviceId, 10, missionInfos);
    MEDIA_LOGI("screen_capture_specified_window_cb_02 missionInfos size:%{public}s, result:%{public}d",
        std::to_string(missionInfos.size()).c_str(), result);
    if (missionInfos.size() > 0) {
        for (OHOS::AAFwk::MissionInfo info : missionInfos) {
            MEDIA_LOGI("screen_capture_specified_window_cb_02 missionId:%{public}d", info.id);
            config_.videoInfo.videoCapInfo.taskIDs.push_back(info.id);
        }
    } else {
        MEDIA_LOGE("screen_capture_specified_window_cb_02 GetMissionInfos failed");
    }

    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_);
    ASSERT_NE(nullptr, screenCaptureCb_);
    std::string name = "screen_capture_specified_window_cb_02";
    // track enalbed: inner: true, mic: true, video: true
    OpenFile(name, true, true, true);
    // check track aquire & release: inner: 1, mic: 1, video: 1
    screenCaptureCb_->InitCaptureTrackInfo(innerAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER);
    screenCaptureCb_->InitCaptureTrackInfo(micAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC);
    screenCaptureCb_->InitCaptureTrackInfo(videoFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_VIDEO);
    // callback enabled: errorCallback: true, dataCallback: true, stateChangeCallback: true
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_, true, true, true));

    screenCapture_->SetMicrophoneEnabled(false); // Enable Mic
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED, screenCaptureCb_->GetScreenCaptureState());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_cb_02 after");
}

/**
 * @tc.name: screen_capture_specified_window_cb_03
 * @tc.desc: Enable microphone, capture screen, mic audio
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_specified_window_cb_03, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_cb_03 before");
    SetConfig(config_);
    config_.captureMode = CaptureMode::CAPTURE_SPECIFIED_WINDOW;
    std::shared_ptr<OHOS::AAFwk::AbilityManagerClient> client_ = OHOS::AAFwk::AbilityManagerClient::GetInstance();
    std::string deviceId = "";
    std::vector<OHOS::AAFwk::MissionInfo> missionInfos;
    auto result = client_->GetMissionInfos(deviceId, 10, missionInfos);
    MEDIA_LOGI("screen_capture_specified_window_cb_03 missionInfos size:%{public}s, result:%{public}d",
        std::to_string(missionInfos.size()).c_str(), result);
    if (missionInfos.size() > 0) {
        for (OHOS::AAFwk::MissionInfo info : missionInfos) {
            MEDIA_LOGI("screen_capture_specified_window_cb_03 missionId:%{public}d", info.id);
            config_.videoInfo.videoCapInfo.taskIDs.push_back(info.id);
        }
    } else {
        MEDIA_LOGE("screen_capture_specified_window_cb_03 GetMissionInfos failed");
    }

    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_);
    ASSERT_NE(nullptr, screenCaptureCb_);
    std::string name = "screen_capture_specified_window_cb_03";
    // track enalbed: inner: false, mic: true, video: true
    OpenFile(name, false, true, true);
    // check track aquire & release: inner: 1, mic: 1, video: 1
    screenCaptureCb_->InitCaptureTrackInfo(innerAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER);
    screenCaptureCb_->InitCaptureTrackInfo(micAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC);
    screenCaptureCb_->InitCaptureTrackInfo(videoFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_VIDEO);
    // callback enabled: errorCallback: true, dataCallback: true, stateChangeCallback: true
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_, true, true, true));

    screenCapture_->SetMicrophoneEnabled(true); // Enable Mic
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED, screenCaptureCb_->GetScreenCaptureState());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_cb_03 after");
}

/**
 * @tc.name: screen_capture_specified_window_cb_04
 * @tc.desc: Enable microphone, capture screen, inner audio
 * @tc.type: FUNC
 * @tc.require: play something audio resource to get inner audio
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_specified_window_cb_04, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_cb_04 before");
    SetConfig(config_);
    config_.audioInfo.micCapInfo.audioSampleRate = 0;
    config_.audioInfo.micCapInfo.audioChannels = 0;
    config_.audioInfo.micCapInfo.audioSource = AudioCaptureSourceType::SOURCE_DEFAULT;
    config_.audioInfo.innerCapInfo.audioSampleRate = 16000;
    config_.audioInfo.innerCapInfo.audioChannels = 2;
    config_.audioInfo.innerCapInfo.audioSource = AudioCaptureSourceType::ALL_PLAYBACK;
    config_.captureMode = CaptureMode::CAPTURE_SPECIFIED_WINDOW;
    std::shared_ptr<OHOS::AAFwk::AbilityManagerClient> client_ = OHOS::AAFwk::AbilityManagerClient::GetInstance();
    std::string deviceId = "";
    std::vector<OHOS::AAFwk::MissionInfo> missionInfos;
    auto result = client_->GetMissionInfos(deviceId, 10, missionInfos);
    MEDIA_LOGI("screen_capture_specified_window_cb_04 missionInfos size:%{public}s, result:%{public}d",
        std::to_string(missionInfos.size()).c_str(), result);
    if (missionInfos.size() > 0) {
        for (OHOS::AAFwk::MissionInfo info : missionInfos) {
            MEDIA_LOGI("screen_capture_specified_window_cb_04 missionId:%{public}d", info.id);
            config_.videoInfo.videoCapInfo.taskIDs.push_back(info.id);
        }
    } else {
        MEDIA_LOGE("screen_capture_specified_window_cb_04 GetMissionInfos failed");
    }

    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_);
    ASSERT_NE(nullptr, screenCaptureCb_);
    std::string name = "screen_capture_specified_window_cb_04";
    // track enalbed: inner: true, mic: false, video: true
    OpenFile(name, true, false, true);
    // check track aquire & release: inner: 1, mic: 1, video: 1
    screenCaptureCb_->InitCaptureTrackInfo(innerAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER);
    screenCaptureCb_->InitCaptureTrackInfo(micAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC);
    screenCaptureCb_->InitCaptureTrackInfo(videoFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_VIDEO);
    // callback enabled: errorCallback: true, dataCallback: true, stateChangeCallback: true
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_, true, true, true));

    screenCapture_->SetMicrophoneEnabled(true); // Enable Mic
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED, screenCaptureCb_->GetScreenCaptureState());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_cb_04 after");
}

/**
 * @tc.name: screen_capture_specified_window_cb_05
 * @tc.desc: Enable microphone, capture inner audio, mic audio
 * @tc.type: FUNC
 * @tc.require: play something audio resource to get inner audio
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_specified_window_cb_05, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_cb_05 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoFrameWidth = 0;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 0;
    config_.audioInfo.innerCapInfo.audioSampleRate = 16000;
    config_.audioInfo.innerCapInfo.audioChannels = 2;
    config_.audioInfo.innerCapInfo.audioSource = AudioCaptureSourceType::ALL_PLAYBACK;
    config_.captureMode = CaptureMode::CAPTURE_SPECIFIED_WINDOW;
    std::shared_ptr<OHOS::AAFwk::AbilityManagerClient> client_ = OHOS::AAFwk::AbilityManagerClient::GetInstance();
    std::string deviceId = "";
    std::vector<OHOS::AAFwk::MissionInfo> missionInfos;
    auto result = client_->GetMissionInfos(deviceId, 10, missionInfos);
    MEDIA_LOGI("screen_capture_specified_window_cb_05 missionInfos size:%{public}s, result:%{public}d",
        std::to_string(missionInfos.size()).c_str(), result);
    if (missionInfos.size() > 0) {
        for (OHOS::AAFwk::MissionInfo info : missionInfos) {
            MEDIA_LOGI("screen_capture_specified_window_cb_05 missionId:%{public}d", info.id);
            config_.videoInfo.videoCapInfo.taskIDs.push_back(info.id);
        }
    } else {
        MEDIA_LOGE("screen_capture_specified_window_cb_05 GetMissionInfos failed");
    }

    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_);
    ASSERT_NE(nullptr, screenCaptureCb_);
    std::string name = "screen_capture_specified_window_cb_05";
    // track enalbed: inner: true, mic: true, video: false
    OpenFile(name, true, true, false);
    // check track aquire & release: inner: 1, mic: 1, video: 1
    screenCaptureCb_->InitCaptureTrackInfo(innerAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER);
    screenCaptureCb_->InitCaptureTrackInfo(micAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC);
    screenCaptureCb_->InitCaptureTrackInfo(videoFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_VIDEO);
    // callback enabled: errorCallback: true, dataCallback: true, stateChangeCallback: true
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_, true, true, true));

    screenCapture_->SetMicrophoneEnabled(true); // Enable Mic
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED, screenCaptureCb_->GetScreenCaptureState());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_cb_05 after");
}

/**
 * @tc.name: screen_capture_specified_window_cb_06
 * @tc.desc: Enable microphone, capture inner audio only
 * @tc.type: FUNC
 * @tc.require: play something audio resource to get inner audio
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_specified_window_cb_06, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_cb_06 before");
    SetConfig(config_);
    config_.videoInfo.videoCapInfo.videoFrameWidth = 0;
    config_.videoInfo.videoCapInfo.videoFrameHeight = 0;
    config_.audioInfo.micCapInfo.audioSampleRate = 0;
    config_.audioInfo.micCapInfo.audioChannels = 0;
    config_.audioInfo.micCapInfo.audioSource = AudioCaptureSourceType::SOURCE_DEFAULT;
    config_.audioInfo.innerCapInfo.audioSampleRate = 16000;
    config_.audioInfo.innerCapInfo.audioChannels = 2;
    config_.audioInfo.innerCapInfo.audioSource = AudioCaptureSourceType::ALL_PLAYBACK;
    config_.captureMode = CaptureMode::CAPTURE_SPECIFIED_WINDOW;
    std::shared_ptr<OHOS::AAFwk::AbilityManagerClient> client_ = OHOS::AAFwk::AbilityManagerClient::GetInstance();
    std::string deviceId = "";
    std::vector<OHOS::AAFwk::MissionInfo> missionInfos;
    auto result = client_->GetMissionInfos(deviceId, 10, missionInfos);
    MEDIA_LOGI("screen_capture_specified_window_cb_06 missionInfos size:%{public}s, result:%{public}d",
        std::to_string(missionInfos.size()).c_str(), result);
    if (missionInfos.size() > 0) {
        for (OHOS::AAFwk::MissionInfo info : missionInfos) {
            MEDIA_LOGI("screen_capture_specified_window_cb_06 missionId:%{public}d", info.id);
            config_.videoInfo.videoCapInfo.taskIDs.push_back(info.id);
        }
    } else {
        MEDIA_LOGE("screen_capture_specified_window_cb_06 GetMissionInfos failed");
    }

    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_);
    ASSERT_NE(nullptr, screenCaptureCb_);
    std::string name = "screen_capture_specified_window_cb_06";
    // track enalbed: inner: true, mic: false, video: false
    OpenFile(name, true, false, false);
    // check track aquire & release: inner: 1, mic: 1, video: 1
    screenCaptureCb_->InitCaptureTrackInfo(innerAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER);
    screenCaptureCb_->InitCaptureTrackInfo(micAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC);
    screenCaptureCb_->InitCaptureTrackInfo(videoFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_VIDEO);
    // callback enabled: errorCallback: true, dataCallback: true, stateChangeCallback: true
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_, true, true, true));

    screenCapture_->SetMicrophoneEnabled(true); // Enable Mic
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED, screenCaptureCb_->GetScreenCaptureState());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_cb_06 after");
}

/**
 * @tc.name: screen_capture_specified_window_cb_07
 * @tc.desc: Enable microphone, capture screen only
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_specified_window_cb_07, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_cb_07 before");
    SetConfig(config_);
    config_.audioInfo.micCapInfo.audioSampleRate = 0;
    config_.audioInfo.micCapInfo.audioChannels = 0;
    config_.audioInfo.micCapInfo.audioSource = AudioCaptureSourceType::SOURCE_DEFAULT;
    config_.captureMode = CaptureMode::CAPTURE_SPECIFIED_WINDOW;
    std::shared_ptr<OHOS::AAFwk::AbilityManagerClient> client_ = OHOS::AAFwk::AbilityManagerClient::GetInstance();
    std::string deviceId = "";
    std::vector<OHOS::AAFwk::MissionInfo> missionInfos;
    auto result = client_->GetMissionInfos(deviceId, 10, missionInfos);
    MEDIA_LOGI("screen_capture_specified_window_cb_07 missionInfos size:%{public}s, result:%{public}d",
        std::to_string(missionInfos.size()).c_str(), result);
    if (missionInfos.size() > 0) {
        for (OHOS::AAFwk::MissionInfo info : missionInfos) {
            MEDIA_LOGI("screen_capture_specified_window_cb_07 missionId:%{public}d", info.id);
            config_.videoInfo.videoCapInfo.taskIDs.push_back(info.id);
        }
    } else {
        MEDIA_LOGE("screen_capture_specified_window_cb_07 GetMissionInfos failed");
    }

    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_);
    ASSERT_NE(nullptr, screenCaptureCb_);
    std::string name = "screen_capture_specified_window_cb_07";
    // track enalbed: inner: false, mic: false, video: true
    OpenFile(name, false, false, true);
    // check track aquire & release: inner: 1, mic: 1, video: 1
    screenCaptureCb_->InitCaptureTrackInfo(innerAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER);
    screenCaptureCb_->InitCaptureTrackInfo(micAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC);
    screenCaptureCb_->InitCaptureTrackInfo(videoFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_VIDEO);
    // callback enabled: errorCallback: true, dataCallback: true, stateChangeCallback: true
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_, true, true, true));

    screenCapture_->SetMicrophoneEnabled(true); // Enable Mic
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCapture());
    sleep(RECORDER_TIME);
    EXPECT_EQ(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED, screenCaptureCb_->GetScreenCaptureState());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_specified_window_cb_07 after");
}

/**
 * @tc.name: screen_capture_save_file_cb_01
 * @tc.desc: do screencapture capture file mode, capture inner audio and mic audio
 * @tc.type: FUNC
 * @tc.require: play something audio resource to get inner audio
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_save_file_cb_01, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_cb_01 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_save_file_cb_01.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    config_.audioInfo.innerCapInfo.audioSampleRate = 16000;
    config_.audioInfo.innerCapInfo.audioChannels = 2;
    config_.audioInfo.innerCapInfo.audioSource = AudioCaptureSourceType::ALL_PLAYBACK;

    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_);
    ASSERT_NE(nullptr, screenCaptureCb_);
    // callback enabled: errorCallback: true, dataCallback: false, stateChangeCallback: true
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_, true, false, true));

    screenCapture_->SetMicrophoneEnabled(true); // Enable Mic
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED, screenCaptureCb_->GetScreenCaptureState());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_cb_01 after");
}

/**
 * @tc.name: screen_capture_save_file_cb_02
 * @tc.desc: do screencapture capture file mode, capture inner audio
 * @tc.type: FUNC
 * @tc.require: play something audio resource to get inner audio
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_save_file_cb_02, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_cb_02 before");
    RecorderInfo recorderInfo;
    SetRecorderInfo("screen_capture_save_file_cb_02.mp4", recorderInfo);
    SetConfigFile(config_, recorderInfo);
    config_.audioInfo.micCapInfo.audioSampleRate = 0;
    config_.audioInfo.micCapInfo.audioChannels = 0;
    config_.audioInfo.micCapInfo.audioSource = AudioCaptureSourceType::SOURCE_DEFAULT;
    config_.audioInfo.innerCapInfo.audioSampleRate = 16000;
    config_.audioInfo.innerCapInfo.audioChannels = 2;
    config_.audioInfo.innerCapInfo.audioSource = AudioCaptureSourceType::ALL_PLAYBACK;

    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_);
    ASSERT_NE(nullptr, screenCaptureCb_);
    // callback enabled: errorCallback: true, dataCallback: false, stateChangeCallback: true
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_, true, false, true));

    screenCapture_->SetMicrophoneEnabled(true); // Enable Mic
    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenRecording());
    sleep(RECORDER_TIME);
    EXPECT_EQ(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED, screenCaptureCb_->GetScreenCaptureState());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenRecording());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_save_file_cb_02 after");
}

/**
 * @tc.name: screen_capture_with_surface_cb_01
 * @tc.desc: do screencapture capture surface mode, capture screen, inner audio and mic audio
 * @tc.type: FUNC
 * @tc.require: play something audio resource to get inner audio
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_with_surface_cb_01, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_cb_01 before");
    SetConfig(config_);
    config_.audioInfo.innerCapInfo.audioSampleRate = 16000;
    config_.audioInfo.innerCapInfo.audioChannels = 2;
    config_.audioInfo.innerCapInfo.audioSource = AudioCaptureSourceType::ALL_PLAYBACK;
    bool isMicrophone = true;
    screenCapture_->SetMicrophoneEnabled(isMicrophone);
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_cb_01 CreateRecorder");
    std::shared_ptr<Recorder> recorder = nullptr;
    recorder = RecorderFactory::CreateRecorder();
    int32_t videoSourceId = 0;
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSource(VIDEO_SOURCE_SURFACE_RGBA, videoSourceId));
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFormat(OutputFormatType::FORMAT_MPEG_4));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncoder(videoSourceId, VideoCodecFormat::H264));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSize(videoSourceId, 720, 1080));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoFrameRate(videoSourceId, 30));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncodingBitRate(videoSourceId, 2000000));
    OpenFileFd("screen_capture_with_surface_cb_01.mp4");
    ASSERT_TRUE(outputFd_ >= 0);
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFile(outputFd_));
    EXPECT_EQ(MSERR_OK, recorder->Prepare());
    sptr<OHOS::Surface> consumer = recorder->GetSurface(videoSourceId);

    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_);
    ASSERT_NE(nullptr, screenCaptureCb_);

    std::string name = "screen_capture_with_surface_cb_01";
    // track enalbed: inner: true, mic: true, video: true(surface mode)
    OpenFile(name, true, true, false);
    // check track aquire & release: inner: 1, mic: 1, video: 1
    screenCaptureCb_->InitCaptureTrackInfo(innerAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER);
    screenCaptureCb_->InitCaptureTrackInfo(micAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC);
    screenCaptureCb_->InitCaptureTrackInfo(videoFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_VIDEO);
    // callback enabled: errorCallback: true, dataCallback: true, stateChangeCallback: true
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_, true, true, true));

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCaptureWithSurface(consumer));
    EXPECT_EQ(MSERR_OK, recorder->Start());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_cb_01 recorder Start");
    sleep(RECORDER_TIME);
    EXPECT_EQ(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED, screenCaptureCb_->GetScreenCaptureState());
    EXPECT_EQ(MSERR_OK, recorder->Stop(true));
    EXPECT_EQ(MSERR_OK, recorder->Reset());
    EXPECT_EQ(MSERR_OK, recorder->Release());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_cb_01 after");
}

/**
 * @tc.name: screen_capture_with_surface_cb_02
 * @tc.desc: do screencapture capture surface mode, capture screen and inner audio
 * @tc.type: FUNC
 * @tc.require: play something audio resource to get inner audio
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_with_surface_cb_02, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_cb_02 before");
    SetConfig(config_);
    config_.audioInfo.micCapInfo.audioSampleRate = 0;
    config_.audioInfo.micCapInfo.audioChannels = 0;
    config_.audioInfo.micCapInfo.audioSource = AudioCaptureSourceType::SOURCE_DEFAULT;
    config_.audioInfo.innerCapInfo.audioSampleRate = 16000;
    config_.audioInfo.innerCapInfo.audioChannels = 2;
    config_.audioInfo.innerCapInfo.audioSource = AudioCaptureSourceType::ALL_PLAYBACK;
    screenCapture_->SetMicrophoneEnabled(true);
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_cb_02 CreateRecorder");
    std::shared_ptr<Recorder> recorder = nullptr;
    recorder = RecorderFactory::CreateRecorder();
    int32_t videoSourceId = 0;
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSource(VIDEO_SOURCE_SURFACE_RGBA, videoSourceId));
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFormat(OutputFormatType::FORMAT_MPEG_4));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncoder(videoSourceId, VideoCodecFormat::H264));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSize(videoSourceId, 720, 1080));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoFrameRate(videoSourceId, 30));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncodingBitRate(videoSourceId, 2000000));
    OpenFileFd("screen_capture_with_surface_cb_02.mp4");
    ASSERT_TRUE(outputFd_ >= 0);
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFile(outputFd_));
    EXPECT_EQ(MSERR_OK, recorder->Prepare());
    sptr<OHOS::Surface> consumer = recorder->GetSurface(videoSourceId);

    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_);
    ASSERT_NE(nullptr, screenCaptureCb_);

    std::string name = "screen_capture_with_surface_cb_02";
    // track enalbed: inner: true, mic: false, video: true(surface mode)
    OpenFile(name, true, false, false);
    // check track aquire & release: inner: 1, mic: 1, video: 1
    screenCaptureCb_->InitCaptureTrackInfo(innerAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER);
    screenCaptureCb_->InitCaptureTrackInfo(micAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC);
    screenCaptureCb_->InitCaptureTrackInfo(videoFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_VIDEO);
    // callback enabled: errorCallback: true, dataCallback: true, stateChangeCallback: true
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_, true, true, true));

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCaptureWithSurface(consumer));
    EXPECT_EQ(MSERR_OK, recorder->Start());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_cb_02 recorder Start");
    sleep(RECORDER_TIME);
    EXPECT_EQ(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED, screenCaptureCb_->GetScreenCaptureState());
    EXPECT_EQ(MSERR_OK, recorder->Stop(true));
    EXPECT_EQ(MSERR_OK, recorder->Reset());
    EXPECT_EQ(MSERR_OK, recorder->Release());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_cb_02 after");
}

/**
 * @tc.name: screen_capture_with_surface_cb_03
 * @tc.desc: do screencapture capture surface mode, capture screen and mic audio
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_with_surface_cb_03, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_cb_03 before");
    SetConfig(config_);
    screenCapture_->SetMicrophoneEnabled(true);
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_cb_03 CreateRecorder");
    std::shared_ptr<Recorder> recorder = RecorderFactory::CreateRecorder();
    int32_t videoSourceId = 0;
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSource(VIDEO_SOURCE_SURFACE_RGBA, videoSourceId));
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFormat(OutputFormatType::FORMAT_MPEG_4));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncoder(videoSourceId, VideoCodecFormat::H264));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSize(videoSourceId, 720, 1080));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoFrameRate(videoSourceId, 30));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncodingBitRate(videoSourceId, 2000000));
    OpenFileFd("screen_capture_with_surface_cb_03.mp4");
    ASSERT_TRUE(outputFd_ >= 0);
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFile(outputFd_));
    EXPECT_EQ(MSERR_OK, recorder->Prepare());
    sptr<OHOS::Surface> consumer = recorder->GetSurface(videoSourceId);

    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_);
    ASSERT_NE(nullptr, screenCaptureCb_);

    std::string name = "screen_capture_with_surface_cb_03";
    // track enalbed: inner: false, mic: true, video: true(surface mode)
    OpenFile(name, false, true, false);
    // check track aquire & release: inner: 1, mic: 1, video: 1
    screenCaptureCb_->InitCaptureTrackInfo(innerAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER);
    screenCaptureCb_->InitCaptureTrackInfo(micAudioFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC);
    screenCaptureCb_->InitCaptureTrackInfo(videoFile_, 1, SCREEN_CAPTURE_BUFFERTYPE_VIDEO);
    // callback enabled: errorCallback: true, dataCallback: true, stateChangeCallback: true
    EXPECT_EQ(MSERR_OK, screenCapture_->SetScreenCaptureCallback(screenCaptureCb_, true, true, true));

    EXPECT_EQ(MSERR_OK, screenCapture_->Init(config_));
    EXPECT_EQ(MSERR_OK, screenCapture_->StartScreenCaptureWithSurface(consumer));
    EXPECT_EQ(MSERR_OK, recorder->Start());
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_cb_03 recorder Start");
    sleep(RECORDER_TIME);
    EXPECT_EQ(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED, screenCaptureCb_->GetScreenCaptureState());
    EXPECT_EQ(MSERR_OK, recorder->Stop(true));
    EXPECT_EQ(MSERR_OK, recorder->Reset());
    EXPECT_EQ(MSERR_OK, recorder->Release());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_cb_03 after");
}

/**
 * @tc.name: screen_capture_with_surface_cb_04
 * @tc.desc: do screencapture capture surface mode, capture screen only
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ScreenCaptureUnitTest, screen_capture_with_surface_cb_04, TestSize.Level2)
{
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_cb_04 before");
    SetConfig(config_);
    config_.audioInfo.micCapInfo.audioSampleRate = 0;
    config_.audioInfo.micCapInfo.audioChannels = 0;
    config_.audioInfo.micCapInfo.audioSource = AudioCaptureSourceType::SOURCE_DEFAULT;
    screenCapture_->SetMicrophoneEnabled(true);
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_cb_04 CreateRecorder");
    std::shared_ptr<Recorder> recorder = RecorderFactory::CreateRecorder();
    int32_t videoSourceId = 0;
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSource(VIDEO_SOURCE_SURFACE_RGBA, videoSourceId));
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFormat(OutputFormatType::FORMAT_MPEG_4));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncoder(videoSourceId, VideoCodecFormat::H264));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoSize(videoSourceId, 720, 1080));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoFrameRate(videoSourceId, 30));
    EXPECT_EQ(MSERR_OK, recorder->SetVideoEncodingBitRate(videoSourceId, 2000000));
    OpenFileFd("screen_capture_with_surface_cb_04.mp4");
    ASSERT_TRUE(outputFd_ >= 0);
    EXPECT_EQ(MSERR_OK, recorder->SetOutputFile(outputFd_));
    EXPECT_EQ(MSERR_OK, recorder->Prepare());
    sptr<OHOS::Surface> consumer = recorder->GetSurface(videoSourceId);

    screenCaptureCb_ = std::make_shared<ScreenCaptureUnitTestCallback>(screenCapture_);
    ASSERT_NE(nullptr, screenCaptureCb_);

    std::string name = "screen_capture_with_surface_cb_04";
    // track enalbed: inner: false, mic: false, video: true(surface mode)
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
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_cb_04 recorder Start");
    sleep(RECORDER_TIME);
    EXPECT_EQ(AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_STARTED, screenCaptureCb_->GetScreenCaptureState());
    EXPECT_EQ(MSERR_OK, recorder->Stop(true));
    EXPECT_EQ(MSERR_OK, recorder->Reset());
    EXPECT_EQ(MSERR_OK, recorder->Release());
    EXPECT_EQ(MSERR_OK, screenCapture_->StopScreenCapture());
    EXPECT_EQ(MSERR_OK, screenCapture_->Release());
    CloseFile();
    MEDIA_LOGI("ScreenCaptureUnitTest screen_capture_with_surface_cb_04 after");
}
} // namespace Media
} // namespace OHOS