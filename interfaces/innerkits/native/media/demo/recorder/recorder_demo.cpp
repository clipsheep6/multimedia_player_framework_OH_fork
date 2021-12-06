/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License\n");
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

#include "recorder_demo.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "display_type.h"
#include "securec.h"
#include "demo_log.h"
#include "media_errors.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
namespace {
    constexpr uint32_t YUV_STREAM_SIZE = 120; // constexpr uint32_t STUB_STREAM_SIZE = 451;
    constexpr uint32_t FRAME_RATE = 30000;
    constexpr uint32_t CODEC_BUFFER_WIDTH = 480; // 1024;
    constexpr uint32_t CODEC_BUFFER_HEIGHT = 360; // 25;
    constexpr uint32_t STRIDE_ALIGN = 8;
    constexpr uint32_t FRAME_DURATION = 40000000;
    constexpr uint32_t RECORDER_TIME = 5;
    const string PURE_VIDEO = "1";
    const string PURE_AUDIO = "2";
    const string AUDIO_VIDEO = "3";
    constexpr uint32_t YUV_BUFFER_SIZE = 259200; // 480 * 360 * 3 / 2
}

void RecorderCallbackDemo::OnError(RecorderErrorType errorType, int32_t errorCode)
{
    cout << "Error received, errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void RecorderCallbackDemo::OnInfo(int32_t type, int32_t extra)
{
    cout << "Info received, Infotype:" << type << " Infocode:" << extra << endl;
}

// config for video to request buffer from surface
static VideoRecorderConfig g_videoRecorderConfig;

// config for audio to request buffer from surface
static AudioRecorderConfig g_audioRecorderConfig;

// config for surface buffer flush to the queue
static OHOS::BufferFlushConfig g_flushConfig = {
    .damage = {
        .x = 0,
        .y = 0,
        .w = CODEC_BUFFER_WIDTH,
        .h = CODEC_BUFFER_HEIGHT
    },
    .timestamp = 0
};

// config for surface buffer request from the queue
static OHOS::BufferRequestConfig g_requestConfig = {
    .width = CODEC_BUFFER_WIDTH,
    .height = CODEC_BUFFER_HEIGHT,
    .strideAlignment = STRIDE_ALIGN,
    .format = PIXEL_FMT_YCBCR_420_SP, // PIXEL_FMT_RGBA_8888,
    .usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA,
    .timeout = 0
};

int32_t RecorderDemo::GetStubFile()
{
    file_ = std::make_shared<std::ifstream>();
    DEMO_CHECK_AND_RETURN_RET_LOG(file_ != nullptr, MSERR_INVALID_OPERATION, "create file failed");
    const std::string filePath = "/data/yuv480x360.yuv"; // "/data/h264_1280_720.h264";
    file_->open(filePath, std::ios::in | std::ios::binary);
    DEMO_CHECK_AND_RETURN_RET_LOG(file_->is_open(), MSERR_INVALID_OPERATION, "open file failed");

    return MSERR_OK;
}

void RecorderDemo::HDICreateBuffer()
{
    // camera hdi loop to requeset buffer // const uint32_t *frameLenArray = HIGH_VIDEO_FRAME_SIZE;
    while (count_ < YUV_STREAM_SIZE) {
        DEMO_CHECK_AND_BREAK_LOG(!isExit_.load(), "close camera hdi thread");
        usleep(FRAME_RATE);
        OHOS::sptr<OHOS::SurfaceBuffer> buffer;
        int32_t releaseFence;
        OHOS::SurfaceError ret = producerSurface_->RequestBuffer(buffer, releaseFence, g_requestConfig);
        DEMO_CHECK_AND_CONTINUE_LOG(ret != OHOS::SURFACE_ERROR_NO_BUFFER, "surface loop full, no buffer now");
        DEMO_CHECK_AND_BREAK_LOG(ret == SURFACE_ERROR_OK && buffer != nullptr, "RequestBuffer failed");

        auto addr = static_cast<uint8_t *>(buffer->GetVirAddr());
        if (addr == nullptr) {
            cout << "GetVirAddr failed" << endl;
            (void)producerSurface_->CancelBuffer(buffer);
            break;
        } // char *tempBuffer = static_cast<char *>(malloc(sizeof(char) * (*frameLenArray) + 1));

        char *tempBuffer = static_cast<char *>(malloc(sizeof(char) * YUV_BUFFER_SIZE));
        if (tempBuffer == nullptr) {
            (void)producerSurface_->CancelBuffer(buffer);
            break;
        }
        (void)file_->read(tempBuffer, YUV_BUFFER_SIZE);

        errno_t mRet = memcpy_s(addr, YUV_BUFFER_SIZE, tempBuffer, YUV_BUFFER_SIZE);
        if (mRet != EOK) {
            (void)producerSurface_->CancelBuffer(buffer);
            free(tempBuffer);
            break;
        }

        (void)buffer->ExtraSet("dataSize", static_cast<int32_t>(YUV_BUFFER_SIZE));
        (void)buffer->ExtraSet("timeStamp", pts_);
        (void)buffer->ExtraSet("isKeyFrame", isKeyFrame_);
        count_++;
        (count_ % 30) == 0 ? (isKeyFrame_ = 1) : (isKeyFrame_ = 0); // keyframe every 30fps
        pts_ += FRAME_DURATION;
        (void)producerSurface_->FlushBuffer(buffer, -1, g_flushConfig);
        free(tempBuffer);
    }
    cout << "exit camera hdi loop" << endl;
    if ((file_ != nullptr) && (file_->is_open())) {
        file_->close();
    }
}

int32_t RecorderDemo::CameraServicesForVideo() const
{
    int32_t ret = recorder_->SetVideoEncoder(g_videoRecorderConfig.videoSourceId,
        g_videoRecorderConfig.videoFormat);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetVideoEncoder failed ");

    ret = recorder_->SetVideoSize(g_videoRecorderConfig.videoSourceId,
        g_videoRecorderConfig.width, g_videoRecorderConfig.height);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetVideoSize failed ");

    ret = recorder_->SetVideoFrameRate(g_videoRecorderConfig.videoSourceId, g_videoRecorderConfig.frameRate);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetVideoFrameRate failed ");

    ret = recorder_->SetVideoEncodingBitRate(g_videoRecorderConfig.videoSourceId,
        g_videoRecorderConfig.videoEncodingBitRate);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetVideoEncodingBitRate failed ");

    recorder_->SetOrientationHint(g_videoRecorderConfig.rotation);
    return MSERR_OK;
}

int32_t RecorderDemo::CameraServicesForAudio() const
{
    int32_t ret = recorder_->SetAudioEncoder(g_videoRecorderConfig.audioSourceId, g_videoRecorderConfig.audioFormat);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetAudioEncoder failed ");

    ret = recorder_->SetAudioSampleRate(g_videoRecorderConfig.audioSourceId, g_videoRecorderConfig.sampleRate);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetAudioSampleRate failed ");

    ret = recorder_->SetAudioChannels(g_videoRecorderConfig.audioSourceId, g_videoRecorderConfig.channelCount);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetAudioChannels failed ");

    ret = recorder_->SetAudioEncodingBitRate(g_videoRecorderConfig.audioSourceId,
        g_videoRecorderConfig.audioEncodingBitRate);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetAudioEncodingBitRate failed ");

    return MSERR_OK;
}

int32_t RecorderDemo::SetFormat(const std::string &recorderType) const
{
    int32_t ret;
    if (recorderType == PURE_VIDEO) {
        ret = recorder_->SetVideoSource(g_videoRecorderConfig.vSource, g_videoRecorderConfig.videoSourceId);
        DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetVideoSource failed ");
        ret = recorder_->SetOutputFormat(g_videoRecorderConfig.outPutFormat);
        DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetOutputFormat failed ");
        ret = CameraServicesForVideo();
        DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "CameraServices failed ");
    } else if (recorderType == PURE_AUDIO) {
        ret = recorder_->SetAudioSource(g_videoRecorderConfig.aSource, g_videoRecorderConfig.audioSourceId);
        DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetAudioSource failed ");
        ret = recorder_->SetOutputFormat(g_audioRecorderConfig.outPutFormat);
        DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetOutputFormat failed ");
        ret = CameraServicesForAudio();
        DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "CameraServicesForAudio failed ");
    } else if (recorderType == AUDIO_VIDEO) {
        ret = recorder_->SetVideoSource(g_videoRecorderConfig.vSource, g_videoRecorderConfig.videoSourceId);
        DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetVideoSource failed ");
        ret = recorder_->SetAudioSource(g_videoRecorderConfig.aSource, g_videoRecorderConfig.audioSourceId);
        DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetAudioSource failed ");
        ret = recorder_->SetOutputFormat(g_videoRecorderConfig.outPutFormat);
        DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetOutputFormat failed ");
        ret = CameraServicesForVideo();
        DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "CameraServicesForVideo failed ");
        ret = CameraServicesForAudio();
        DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "CameraServicesForAudio failed ");
    }

    ret = recorder_->SetMaxDuration(g_videoRecorderConfig.duration);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetMaxDuration failed ");

    recorder_->SetLocation(g_videoRecorderConfig.latitude, g_videoRecorderConfig.longitude);

    ret = recorder_->SetOutputPath(g_videoRecorderConfig.outPath);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetOutputPath failed ");
    std::shared_ptr<RecorderCallbackDemo> cb = std::make_shared<RecorderCallbackDemo>();
    ret = recorder_->SetRecorderCallback(cb);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetRecorderCallback failed ");

    cout << "set format finished" << endl;
    return MSERR_OK;
}

void RecorderDemo::RunCase()
{
    recorder_ = OHOS::Media::RecorderFactory::CreateRecorder();
    if (recorder_ == nullptr) {
        cout << "recorder_ is null" << endl;
        return;
    }

    string recorderType;
    cout << "recorder pure video audio or audio/video " << endl;
    cout << "pure video enter  :  1" << endl;
    cout << "pure audio enter  :  2" << endl;
    cout << "audio/video enter :  3" << endl;
    (void)getline(cin, recorderType);

    int32_t ret = SetFormat(recorderType);
    DEMO_CHECK_AND_RETURN_LOG(ret == MSERR_OK, "SetFormat failed ");

    ret = recorder_->Prepare();
    DEMO_CHECK_AND_RETURN_LOG(ret == MSERR_OK, "Prepare failed ");
    cout << "Prepare finished" << endl;

    if (recorderType != PURE_AUDIO) {
        producerSurface_ = recorder_->GetSurface(g_videoRecorderConfig.videoSourceId);
        DEMO_CHECK_AND_RETURN_LOG(producerSurface_ != nullptr, "GetSurface failed ");

        ret = GetStubFile();
        DEMO_CHECK_AND_RETURN_LOG(ret == MSERR_OK, "GetStubFile failed ");

        camereHDIThread_.reset(new(std::nothrow) std::thread(&RecorderDemo::HDICreateBuffer, this));
    }

    ret = recorder_->Start();
    DEMO_CHECK_AND_RETURN_LOG(ret == MSERR_OK, "Start failed ");
    cout << "start recordering" << endl;
    sleep(RECORDER_TIME);

    isExit_.store(true);
    ret = recorder_->Stop(false);
    DEMO_CHECK_AND_RETURN_LOG(ret == MSERR_OK, "Stop failed ");
    cout << "stop recordering" << endl;
    if (recorderType != PURE_AUDIO && camereHDIThread_ != nullptr) {
        camereHDIThread_->join();
    }
    ret = recorder_->Reset();
    DEMO_CHECK_AND_RETURN_LOG(ret == MSERR_OK, "Reset failed ");
    ret = recorder_->Release();
    DEMO_CHECK_AND_RETURN_LOG(ret == MSERR_OK, "Release failed ");
}
