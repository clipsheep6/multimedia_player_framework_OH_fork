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
#include <string.h>
#include <sync_fence.h>
#include "aw_common.h"
#include <fstream>
#include "securec.h"
#include "test_recorder.h"

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

bool TestRecorder::SetVideoSource(RecorderTestParam::VideoRecorderConfig &recorderConfig)
{
    int32_t retValue = recorder->SetVideoSource(recorderConfig.vSource, recorderConfig.videoSourceId);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetAudioSource(RecorderTestParam::VideoRecorderConfig &recorderConfig)
{
    int32_t retValue = recorder->SetAudioSource(recorderConfig.aSource, recorderConfig.audioSourceId);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetOutputFormat(RecorderTestParam::VideoRecorderConfig &recorderConfig)
{
    int32_t retValue = recorder->SetOutputFormat(recorderConfig.outPutFormat);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetAudioEncoder(RecorderTestParam::VideoRecorderConfig &recorderConfig)
{
    int32_t retValue = recorder->SetAudioEncoder(recorderConfig.audioSourceId, recorderConfig.audioFormat);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetAudioSampleRate(RecorderTestParam::VideoRecorderConfig &recorderConfig)
{
    int32_t retValue = recorder->SetAudioSampleRate(recorderConfig.audioSourceId, recorderConfig.sampleRate);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetAudioChannels(RecorderTestParam::VideoRecorderConfig &recorderConfig)
{
    int32_t retValue = recorder->SetAudioChannels(recorderConfig.audioSourceId, recorderConfig.channelCount);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetAudioEncodingBitRate(RecorderTestParam::VideoRecorderConfig &recorderConfig)
{
    int32_t retValue = recorder->SetAudioEncodingBitRate(recorderConfig.audioSourceId,
        recorderConfig.audioEncodingBitRate);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetMaxDuration(RecorderTestParam::VideoRecorderConfig &recorderConfig)
{
    int32_t retValue = recorder->SetMaxDuration(recorderConfig.duration);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetOutputFile(RecorderTestParam::VideoRecorderConfig &recorderConfig)
{
    int32_t retValue = recorder->SetOutputFile(recorderConfig.outputFd);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetRecorderCallback(RecorderTestParam::VideoRecorderConfig &recorderConfig)
{
    std::shared_ptr<TestRecorderCallbackTest> cb = std::make_shared<TestRecorderCallbackTest>();
    int32_t retValue = recorder->SetRecorderCallback(cb);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::Prepare(RecorderTestParam::VideoRecorderConfig &recorderConfig)
{
    int32_t retValue = recorder->Prepare();
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::Start(RecorderTestParam::VideoRecorderConfig &recorderConfig)
{
    int32_t retValue = recorder->Start();
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::Stop(bool block, RecorderTestParam::VideoRecorderConfig &recorderConfig)
{
    int32_t retValue = recorder->Stop(block);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::Reset(RecorderTestParam::VideoRecorderConfig &recorderConfig)
{
    int32_t retValue = recorder->Reset();
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::Release(RecorderTestParam::VideoRecorderConfig &recorderConfig)
{
    int32_t retValue = recorder->Release();
    if (retValue != 0) {
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::CreateRecorder()
{
    recorder = RecorderFactory::CreateRecorder();
    if (recorder == nullptr) {
        recorder->Release();
        return false;
    }
    return true;
}

bool TestRecorder::SetVideoEncoder(VideoRecorderConfig &recorderConfig)
{
    int32_t retValue = recorder->SetVideoEncoder(recorderConfig.videoSourceId,
        recorderConfig.videoFormat);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetVideoSize(VideoRecorderConfig &recorderConfig)
{
    int32_t retValue = recorder->SetVideoSize(recorderConfig.videoSourceId,
        recorderConfig.width, recorderConfig.height);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetVideoFrameRate(VideoRecorderConfig &recorderConfig)
{
    int32_t retValue = recorder->SetVideoFrameRate(recorderConfig.videoSourceId,
        recorderConfig.frameRate);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetVideoEncodingBitRate(VideoRecorderConfig &recorderConfig)
{
    int32_t retValue = recorder->SetVideoEncodingBitRate(recorderConfig.videoSourceId,
        recorderConfig.videoEncodingBitRate);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetCaptureRate(VideoRecorderConfig &recorderConfig, double fps)
{
    int32_t retValue = recorder->SetCaptureRate(recorderConfig.videoSourceId, fps);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetNextOutputFile(int32_t sourceId, VideoRecorderConfig &recorderConfig)
{
    int32_t retValue = recorder->SetNextOutputFile(recorderConfig.outputFd);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetMaxFileSize(int64_t size, VideoRecorderConfig &recorderConfig)
{
    int32_t retValue = recorder->SetMaxFileSize(size);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::GetSurface(VideoRecorderConfig &recorderConfig)
{
    OHOS::sptr<OHOS::Surface> retValue = recorder->GetSurface(recorderConfig.videoSourceId);
    if (retValue == nullptr) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::CameraServicesForVideo(VideoRecorderConfig &recorderConfig)
{
    RETURN_IF(TestRecorder::SetVideoEncoder(recorderConfig), false);
    RETURN_IF(TestRecorder::SetVideoSize(recorderConfig), false);
    RETURN_IF(TestRecorder::SetVideoFrameRate(recorderConfig), false);
    RETURN_IF(TestRecorder::SetVideoEncodingBitRate(recorderConfig), false);
    return true;
}

bool TestRecorder::CameraServicesForAudio(VideoRecorderConfig &recorderConfig)
{
    RETURN_IF(TestRecorder::SetAudioEncoder(recorderConfig), false);
    RETURN_IF(TestRecorder::SetAudioSampleRate(recorderConfig), false);
    RETURN_IF(TestRecorder::SetAudioChannels(recorderConfig), false);
    RETURN_IF(TestRecorder::SetAudioEncodingBitRate(recorderConfig), false);
    return true;
}

bool TestRecorder::RequesetBuffer(const std::string &recorderType, VideoRecorderConfig &recorderConfig)
{
    if (recorderType != PURE_AUDIO) {
        RETURN_IF(TestRecorder::GetSurface(recorderConfig), false);

        if (recorderConfig.vSource == VIDEO_SOURCE_SURFACE_ES) {
            RETURN_IF(TestRecorder::GetStubFile(), false);
            camereHDIThread.reset(new(std::nothrow) std::thread(&TestRecorder::HDICreateESBuffer, this));
        } else {
            camereHDIThread.reset(new(std::nothrow) std::thread(&TestRecorder::HDICreateYUVBuffer, this));
        }
    }
    return true;
}

bool TestRecorder::GetStubFile()
{
    file = std::make_shared<std::ifstream>();
    if (file == nullptr) {
        cout << "create file failed" << endl;
        return false;
    }
    const std::string filePath = "/data/test/media/out_320_240_10s.h264";
    file->open(filePath, std::ios::in | std::ios::binary);
    if (!(file->is_open())) {
        cout << "open file failed" << endl;
        return false;
    }
    return true;
}

int64_t TestRecorder::GetPts()
{
    struct timespec timestamp = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    int64_t time = reinterpret_cast<int64_t>(timestamp.tv_sec) * SEC_TO_NS + reinterpret_cast<uint64_t>(timestamp.tv_nsec);
    return time;
}

void TestRecorder::HDICreateESBuffer()
{
    constexpr int32_t SLEEP_TIME = 100;
    const uint32_t *frameLenArray = HIGH_VIDEO_FRAME_SIZE;
    while (counts < STUB_STREAM_SIZE) {
        if (isExit_.load()) {
            break;
        }
        usleep(FRAME_RATE);
        OHOS::sptr<OHOS::SurfaceBuffer> buffer;
        int32_t releaseFence;
        OHOS::SurfaceError ret = producerSurface->RequestBuffer(buffer, releaseFence, g_esRequestConfig);
        if (ret == OHOS::SURFACE_ERROR_NO_BUFFER) {
            continue;
        }
        if (ret == SURFACE_ERROR_OK && buffer != nullptr) {
            break;
        }

        sptr<SyncFence> syncFence = new SyncFence(releaseFence);
        syncFence->Wait(SLEEP_TIME);

        auto addrGetVirAddr = static_cast<uint8_t *>(buffer->GetVirAddr());
        if (addrGetVirAddr == nullptr) {
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
            pts= GetPts();
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
    if ((file != nullptr) && (file->is_open())) {
        file->close();
    }
}

void TestRecorder::HDICreateYUVBuffer()
{
    constexpr int32_t COUNT_ABSTRACT = 3;
    constexpr int32_t COUNT_SPLIT = 30;
    constexpr int32_t COUNT_COLOR = 255;
    constexpr int32_t TIME_WAIT = 100;
    while (counts < STUB_STREAM_SIZE) {
        if (!isExit_.load()) {
            break;
        }

        usleep(FRAME_RATE);
        OHOS::sptr<OHOS::SurfaceBuffer> buffer;
        int32_t releaseFence;
        OHOS::SurfaceError ret = producerSurface->RequestBuffer(buffer, releaseFence, g_yuvRequestConfig);
        if (ret == OHOS::SURFACE_ERROR_NO_BUFFER) {
            continue;
        }
        if (ret != SURFACE_ERROR_OK || buffer == nullptr) {
            break;
        }

        sptr<SyncFence> syncFence = new SyncFence(releaseFence);
        syncFence->Wait(TIME_WAIT); // 100ms

        char *tempBuffer = (char *)(buffer->GetVirAddr());
        (void)memset_s(tempBuffer, YUV_BUFFER_SIZE, color, YUV_BUFFER_SIZE);
        (void)srand(reinterpret_cast<int>(time(0)));
        for (uint32_t i = 0; i < YUV_BUFFER_SIZE - 1; i += (YUV_BUFFER_SIZE - 1)) {
            if (i >= YUV_BUFFER_SIZE - 1) {
                break;
            }
            tempBuffer[i] = (unsigned char)(ProduceRandomNumberCrypt() % COUNT_COLOR);
        }

        color = color - COUNT_ABSTRACT;

        if (color <= 0) {
            color = 0xFF;
        }

        pts= GetPts();
        (void)buffer->GetExtraData()->ExtraSet("dataSize", static_cast<int32_t>(YUV_BUFFER_SIZE));
        (void)buffer->GetExtraData()->ExtraSet("timeStamp", pts);
        (void)buffer->GetExtraData()->ExtraSet("isKeyFrame", isKeyFrame);
        counts++;
        (counts % COUNT_SPLIT) == 0 ? (isKeyFrame = 1) : (isKeyFrame = 0);
        (void)producerSurface->FlushBuffer(buffer, -1, g_yuvFlushConfig);
    }
}

void TestRecorder::StopBuffer(const std::string &recorderType)
{
    if (recorderType != PURE_AUDIO && camereHDIThread != nullptr) {
        camereHDIThread->join();
    }
}

bool TestRecorder::SetConfig(const std::string &recorderType, VideoRecorderConfig &recorderConfig)
{
    if (recorderType == PURE_VIDEO) {
        RETURN_IF(TestRecorder::SetVideoSource(recorderConfig), false);
        RETURN_IF(TestRecorder::SetOutputFormat(recorderConfig), false);
        RETURN_IF(TestRecorder::CameraServicesForVideo(recorderConfig), false);
    } else if (recorderType == PURE_AUDIO) {
        RETURN_IF(TestRecorder::SetAudioSource(recorderConfig), false);
        RETURN_IF(TestRecorder::SetOutputFormat(recorderConfig), false);
        RETURN_IF(TestRecorder::CameraServicesForAudio(recorderConfig), false);
    } else if (recorderType == AUDIO_VIDEO) {
        RETURN_IF(TestRecorder::SetVideoSource(recorderConfig), false);
        RETURN_IF(TestRecorder::SetAudioSource(recorderConfig), false);
        RETURN_IF(TestRecorder::SetOutputFormat(recorderConfig), false);
        RETURN_IF(TestRecorder::CameraServicesForVideo(recorderConfig), false);
        RETURN_IF(TestRecorder::CameraServicesForAudio(recorderConfig), false);
    }
    RETURN_IF(TestRecorder::SetMaxDuration(recorderConfig), false);
    RETURN_IF(TestRecorder::SetOutputFile(recorderConfig), false);
    RETURN_IF(TestRecorder::SetRecorderCallback(recorderConfig), false);

    return true;
}