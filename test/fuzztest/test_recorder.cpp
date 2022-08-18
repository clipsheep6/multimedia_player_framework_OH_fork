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

#include <iostream>
#include <cstring.h>
#include <sync_fence.h>
#include "securec.h"
#include "test_recorder.h"
#include <fstream>

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::Media::RecorderTestParam;

static OHOS::BufferFlushConfig g_esFlushConfig = {
    .damage = {
        .x = 0,
        .y = 0,
        .w = CODEC_BUFFER_WIDTH,
        .h = CODEC_BUFFER_HEIGHT
    },
    .timestamp = 0
};

static OHOS::BufferRequestConfig g_esRequestConfig = {
    .width = CODEC_BUFFER_WIDTH,
    .height = CODEC_BUFFER_HEIGHT,
    .strideAlignment = STRIDE_ALIGN,
    .format = PIXEL_FMT_RGBA_8888,
    .usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA,
    .timeout = 0
};

static OHOS::BufferFlushConfig g_yuvFlushConfig = {
    .damage = {
        .x = 0,
        .y = 0,
        .w = YUV_BUFFER_WIDTH,
        .h = YUV_BUFFER_HEIGHT
    },
    .timestamp = 0
};

// config for surface buffer request from the queue
static OHOS::BufferRequestConfig g_yuvRequestConfig = {
    .width = YUV_BUFFER_WIDTH,
    .height = YUV_BUFFER_HEIGHT,
    .strideAlignment = STRIDE_ALIGN,
    .format = PIXEL_FMT_YCRCB_420_SP,
    .usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA,
    .timeout = 0
};

void TestRecorderCallbackTest::OnError(RecorderErrorType errorType, int32_t errorCode)
{
    cout << "Error received, errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void TestRecorderCallbackTest::OnInfo(int32_t type, int32_t extra)
{
    cout << "Info received, Infotype:" << type << " Infocode:" << extra << endl;
}

int32_t TestRecorderCallbackTest::GetErrorCode()
{
    return errorCode_;
};

TestRecorder::TestRecorder()
{
}

TestRecorder::~TestRecorder()
{
}

int32_t TestRecorder::CameraServicesForVideo(VideoRecorderConfig &recorderConfig) const
{
    int32_t ret = recorder->SetVideoEncoder(recorderConfig.videoSourceId, recorderConfig.videoFormat);
    if (ret != 0) {
        cout << "SetVideoEncoder fail!!! " << endl;
        return -1;
    }

    ret = recorder->SetVideoSize(recorderConfig.videoSourceId,
        recorderConfig.width, recorderConfig.height);
    if (ret != 0) {
        cout << "SetVideoSize fail!!! " << endl;
        return -1;
    }

    ret = recorder->SetVideoFrameRate(recorderConfig.videoSourceId, recorderConfig.frameRate);
    if (ret != 0) {
        cout << "SetVideoFrameRate fail!!! " << endl;
        return -1;
    }

    ret = recorder->SetVideoEncodingBitRate(recorderConfig.videoSourceId,
        recorderConfig.videoEncodingBitRate);
    if (ret != 0) {
        cout << "SetVideoEncodingBitRate fail!!! " << endl;
        return -1;
    }
    return 0;
}

int32_t TestRecorder::CameraServicesForAudio(VideoRecorderConfig &recorderConfig) const
{
    int32_t ret = recorder->SetAudioEncoder(recorderConfig.audioSourceId, recorderConfig.audioFormat);
    if (ret != 0) {
        cout << "SetAudioEncoder fail!!! " << endl;
        return -1;
    }
    
    ret = recorder->SetAudioSampleRate(recorderConfig.audioSourceId, recorderConfig.sampleRate);
    if (ret != 0) {
        cout << "SetAudioSampleRate fail!!! " << endl;
        return -1;
    }

    ret = recorder->SetAudioChannels(recorderConfig.audioSourceId, recorderConfig.channelCount);
    if (ret != 0) {
        cout << "SetAudioChannels fail!!! " << endl;
        return -1;
    }

    ret = recorder->SetAudioEncodingBitRate(recorderConfig.audioSourceId, recorderConfig.audioEncodingBitRate);
    if (ret != 0) {
        cout << "SetAudioEncodingBitRate fail!!! " << endl;
        return -1;
    }

    return 0;
}

int32_t TestRecorder::RequesetBuffer(const std::string &recorderType, VideoRecorderConfig &recorderConfig)
{
    if (recorderType != PURE_AUDIO) {
        producerSurface = recorder->GetSurface(recorderConfig.videoSourceId);
        if (producerSurface == nullptr) {
            cout << "GetSurface fail!!! " << endl;
            return -1;
        }

        if (recorderConfig.vSource == VIDEO_SOURCE_SURFACE_ES) {
            cout << "es source stream, get from file" << endl;
            int32_t ret = GetStubFile();
            if(ret != 0) {
                cout << "GetStubFile fail!!! " << endl;
                return -1;
            }
            camereHDIThread.reset(new(std::nothrow) std::thread(&TestRecorder::HDICreateESBuffer, this));
        } else {
            camereHDIThread.reset(new(std::nothrow) std::thread(&TestRecorder::HDICreateYUVBuffer, this));
        }
    }
    return 0;
}

int32_t TestRecorder::GetStubFile()
{
    file = std::make_shared<std::ifstream>();
    if (file == nullptr) {
        cout << "create file failed" << endl;
        return -1;
    }
    const std::string filePath = "/data/test/media/out_320_240_10s.h264";
    file->open(filePath, std::ios::in | std::ios::binary);
    if (!(file->is_open())) {
        cout << "open file failed" << endl;
        return -1;
    }
    return 0;
}

uint64_t TestRecorder::GetPts()
{
    struct timespec timestamp = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    uint64_t time = (uint64_t)timestamp.tv_sec * SEC_TO_NS + (uint64_t)timestamp.tv_nsec;
    return time;
}

void TestRecorder::HDICreateESBuffer()
{
    const uint32_t *frameLenArray = HIGH_VIDEO_FRAME_SIZE;
    while (counts < STUB_STREAM_SIZE) {
        FUZZTEST_CHECK_DO(isExit_.load(), "close camera hdi thread", break);
        usleep(FRAME_RATE);
        OHOS::sptr<OHOS::SurfaceBuffer> buffer;
        int32_t releaseFence;
        OHOS::SurfaceError ret = producerSurface->RequestBuffer(buffer, releaseFence, g_esRequestConfig);
        FUZZTEST_CHECK_DO(ret == OHOS::SURFACE_ERROR_NO_BUFFER, "surface loop full, no buffer now", continue);
        FUZZTEST_CHECK_DO(!(ret == SURFACE_ERROR_OK && buffer != nullptr), "RequestBuffer failed", break);

        sptr<SyncFence> syncFence = new SyncFence(releaseFence);
        syncFence->Wait(100);

        auto addrGetVirAddr = static_cast<uint8_t *>(buffer->GetVirAddr());
        if (addrGetVirAddr == nullptr) {
            cout << "GetVirAddr failed" << endl;
            (void)producerSurface->CancelBuffer(buffer);
            break;
        }
        char *tempBuffer = static_cast<char *>(malloc(sizeof(char) * (*frameLenArray) + 1));
        if (tempBuffer == nullptr) {
            (void)producerSurface->CancelBuffer(buffer);
            break;
        }
        (void)file->read(tempBuffer, *frameLenArray);
        if (*frameLenArray > buffer->GetSize()) {
            free(tempBuffer);
            (void)producerSurface->CancelBuffer(buffer);
            break;
        }
        (void)memcpy_s(addrGetVirAddr, *frameLenArray, tempBuffer, *frameLenArray);

        if (isStart_.load()) {
            pts= (int64_t)(GetPts());
            isStart_.store(false);
        }

        (void)buffer->GetExtraData()->ExtraSet("dataSize", static_cast<int32_t>(*frameLenArray));
        (void)buffer->GetExtraData()->ExtraSet("timeStamp", pts);
        (void)buffer->GetExtraData()->ExtraSet("isKeyFrame", isKeyFrame);
        counts++;
        (counts % 30) == 0 ? (isKeyFrame = 1) : (isKeyFrame = 0); // keyframe every 30fps
        pts += FRAME_DURATION;
        (void)producerSurface->FlushBuffer(buffer, -1, g_esFlushConfig);
        frameLenArray++;
        free(tempBuffer);
    }
    cout << "exit camera hdi loop" << endl;
    if ((file != nullptr) && (file->is_open())) {
        file->close();
    }
}

void TestRecorder::HDICreateYUVBuffer()
{    
    while (counts < STUB_STREAM_SIZE) {
        if (!isExit_.load()) {
            cout << "close camera hdi thread" << endl;
        }

        usleep(FRAME_RATE);
        OHOS::sptr<OHOS::SurfaceBuffer> buffer;
        int32_t releaseFence;
        OHOS::SurfaceError ret = producerSurface->RequestBuffer(buffer, releaseFence, g_yuvRequestConfig);
        FUZZTEST_CHECK_DO(ret == OHOS::SURFACE_ERROR_NO_BUFFER, "surface loop full, no buffer now", continue);
        FUZZTEST_CHECK_DO(ret != SURFACE_ERROR_OK || buffer == nullptr, "RequestBuffer failed", break);

        sptr<SyncFence> syncFence = new SyncFence(releaseFence);
        syncFence->Wait(100); // 100ms

        char *tempBuffer = (char *)(buffer->GetVirAddr());
        (void)memset_s(tempBuffer, YUV_BUFFER_SIZE, color, YUV_BUFFER_SIZE);

        (void)srand((int)time(0));
        for (uint32_t i = 0; i < YUV_BUFFER_SIZE - 1; i += (YUV_BUFFER_SIZE - 1)) {  // 100 is the steps between noise
            if (i >= YUV_BUFFER_SIZE - 1) {
                break;
            }
            tempBuffer[i] = (unsigned char)(rand() % 255); // 255 is the size of yuv, add noise
        }

        color = color - 3; // 3 is the step of the pic change

        if (color <= 0) {
            color = 0xFF;
        }

        pts= (int64_t)(GetPts());
        (void)buffer->GetExtraData()->ExtraSet("dataSize", static_cast<int32_t>(YUV_BUFFER_SIZE));
        (void)buffer->GetExtraData()->ExtraSet("timeStamp", pts);
        (void)buffer->GetExtraData()->ExtraSet("isKeyFrame", isKeyFrame);
        counts++;
        (counts % 30) == 0 ? (isKeyFrame = 1) : (isKeyFrame = 0); // keyframe every 30fps
        (void)producerSurface->FlushBuffer(buffer, -1, g_yuvFlushConfig);
    }
    cout << "exit camera hdi loop" << endl;
}

void TestRecorder::StopBuffer(const std::string &recorderType)
{
    if (recorderType != PURE_AUDIO && camereHDIThread != nullptr) {
        camereHDIThread->join();
    }
}

int32_t TestRecorder::SetConfig(const std::string &recorderType, VideoRecorderConfig &recorderConfig) const
{
    int32_t retValue = 0;
    if (recorderType == PURE_VIDEO) {
        retValue = recorder->SetVideoSource(recorderConfig.vSource, recorderConfig.videoSourceId);
        FUZZTEST_CHECK(retValue != 0, "SetVideoSource fail!!!", retValue, retValue = -1, retValue = -1)
        retValue = recorder->SetOutputFormat(recorderConfig.outPutFormat);
        FUZZTEST_CHECK(retValue != 0, "SetOutputFormat fail!!!", retValue, retValue = -1, retValue = -1)
        retValue = CameraServicesForVideo(recorderConfig);
        FUZZTEST_CHECK(retValue != 0, "CameraServicesForVideo fail!!!", retValue, retValue = -1, retValue = -1)
    } else if (recorderType == PURE_AUDIO) {
        retValue = recorder->SetAudioSource(recorderConfig.aSource, recorderConfig.audioSourceId);
        FUZZTEST_CHECK(retValue != 0, "SetAudioSource fail!!!", retValue, retValue = -1, retValue = -1)
        retValue = recorder->SetOutputFormat(recorderConfig.outPutFormat);
        FUZZTEST_CHECK(retValue != 0, "SetOutputFormat fail!!!", retValue, retValue = -1, retValue = -1)
        retValue = CameraServicesForAudio(recorderConfig);
        FUZZTEST_CHECK(retValue != 0, "CameraServicesForAudio fail!!!", retValue, retValue = -1, retValue = -1)
    } else if (recorderType == AUDIO_VIDEO) {
        retValue = recorder->SetVideoSource(recorderConfig.vSource, recorderConfig.videoSourceId);
        FUZZTEST_CHECK(retValue != 0, "SetVideoSource fail!!!", retValue, retValue = -1, retValue = -1)
        retValue = recorder->SetAudioSource(recorderConfig.aSource, recorderConfig.audioSourceId);
        FUZZTEST_CHECK(retValue != 0, "SetAudioSource fail!!!", retValue, retValue = -1, retValue = -1)
        retValue = recorder->SetOutputFormat(recorderConfig.outPutFormat);
        FUZZTEST_CHECK(retValue != 0, "SetOutputFormat fail!!!", retValue, retValue = -1, retValue = -1)
        retValue = CameraServicesForVideo(recorderConfig);
        FUZZTEST_CHECK(retValue != 0, "CameraServicesForVideo fail!!!", retValue, retValue = -1, retValue = -1)
        retValue = CameraServicesForAudio(recorderConfig);
        FUZZTEST_CHECK(retValue != 0, "CameraServicesForAudio fail!!!", retValue, retValue = -1, retValue = -1)
    }

    retValue = recorder->SetMaxDuration(recorderConfig.duration);
    FUZZTEST_CHECK(retValue != 0, "SetMaxDuration fail!!!", retValue, retValue = -1, retValue = -1)

    retValue = recorder->SetOutputFile(recorderConfig.outputFd);
    FUZZTEST_CHECK(retValue != 0, "SetOutputFile fail!!!", retValue, retValue = -1, retValue = -1)

    std::shared_ptr<TestRecorderCallbackTest> cb = std::make_shared<TestRecorderCallbackTest>();
    retValue += recorder->SetRecorderCallback(cb);
    FUZZTEST_CHECK(retValue != 0, "SetRecorderCallback fail!!!", retValue, retValue = -1, retValue = -1)
    cout << "set format finished" << endl;
    return retValue;
}