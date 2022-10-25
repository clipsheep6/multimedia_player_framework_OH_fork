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
#include <string>
#include <sync_fence.h>
#include <fstream>
#include "aw_common.h"
#include "securec.h"
#include "test_recorder.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::Media::RecorderTestParam;
constexpr uint32_t STUB_STREAM_SIZE = 602;
constexpr uint32_t FRAME_RATE = 30000;
constexpr uint32_t CODEC_BUFFER_WIDTH = 1024;
constexpr uint32_t CODEC_BUFFER_HEIGHT = 25;
constexpr uint32_t YUV_BUFFER_WIDTH = 1280;
constexpr uint32_t YUV_BUFFER_HEIGHT = 720;
constexpr uint32_t STRIDE_ALIGN = 8;
constexpr uint32_t FRAME_DURATION = 40000000;
constexpr uint32_t YUV_BUFFER_SIZE = YUV_BUFFER_WIDTH * YUV_BUFFER_HEIGHT * 3 / 2;
constexpr uint32_t SEC_TO_NS = 1000000000;
// this array contains each buffer size of the stub stream
const uint32_t HIGH_VIDEO_FRAME_SIZE[STUB_STREAM_SIZE] = {
    13571, 321, 72, 472, 68, 76, 79, 509, 90, 677, 88, 956, 99, 347, 77, 452, 681, 81, 1263, 94, 106, 97, 998,
    97, 797, 93, 1343, 150, 116, 117, 926, 1198, 128, 110, 78, 1582, 158, 135, 112, 1588, 165, 132, 128, 1697,
    168, 149, 117, 1938, 170, 141, 142, 1830, 106, 161, 122, 1623, 160, 154, 156, 1998, 230, 177, 139, 1650,
    186, 128, 134, 1214, 122, 1411, 120, 1184, 128, 1591, 195, 145, 105, 1587, 169, 140, 118, 1952, 177, 150,
    161, 1437, 159, 123, 1758, 180, 165, 144, 1936, 214, 191, 175, 2122, 180, 179, 160, 1927, 161, 184, 119,
    1973, 218, 210, 129, 1962, 196, 127, 154, 2308, 173, 127, 1572, 142, 122, 2065, 262, 159, 206, 2251, 269,
    179, 170, 2056, 308, 168, 191, 2090, 303, 191, 110, 1932, 272, 162, 122, 1877, 245, 167, 141, 1908, 294,
    162, 118, 1493, 132, 1782, 273, 184, 133, 1958, 274, 180, 149, 2070, 216, 169, 143, 1882, 224, 149, 139,
    1749, 277, 184, 139, 2141, 197, 170, 140, 2002, 269, 162, 140, 1862, 202, 179, 131, 1868, 214, 164, 140,
    1546, 226, 150, 130, 1707, 162, 146, 1824, 181, 147, 130, 1898, 209, 143, 131, 1805, 180, 148, 106, 1776,
    147, 141, 1572, 177, 130, 105, 1776, 178, 144, 122, 1557, 142, 124, 114, 1436, 143, 126, 1326, 127, 1755,
    169, 127, 105, 1807, 177, 131, 134, 1613, 187, 137, 136, 1314, 134, 118, 2005, 194, 129, 147, 1566, 185,
    132, 131, 1236, 174, 137, 106, 11049, 574, 126, 1242, 188, 130, 119, 1450, 187, 137, 141, 1116, 124, 1848,
    138, 122, 1605, 186, 127, 140, 1798, 170, 124, 121, 1666, 157, 128, 130, 1678, 135, 118, 1804, 169, 135,
    125, 1837, 168, 124, 124, 2049, 180, 122, 128, 1334, 143, 128, 1379, 116, 1884, 149, 122, 150, 1962, 176,
    122, 122, 1197, 139, 1853, 184, 151, 148, 1692, 209, 129, 126, 1736, 149, 135, 104, 1775, 165, 160, 121,
    1653, 163, 123, 112, 1907, 181, 129, 107, 1808, 177, 125, 111, 2405, 166, 144, 114, 1833, 198, 136, 113,
    1960, 206, 139, 116, 1791, 175, 130, 129, 1909, 194, 138, 119, 1807, 160, 156, 124, 1998, 184, 173, 114,
    2069, 181, 127, 139, 2212, 182, 138, 146, 1993, 214, 135, 139, 2286, 194, 137, 120, 2090, 196, 159, 132,
    2294, 194, 148, 137, 2312, 183, 163, 106, 2118, 201, 158, 127, 2291, 187, 144, 116, 2413, 139, 115, 2148,
    178, 122, 103, 2370, 207, 161, 117, 2291, 213, 159, 129, 2244, 243, 157, 133, 2418, 255, 171, 127, 2316,
    185, 160, 132, 2405, 220, 165, 155, 2539, 219, 172, 128, 2433, 199, 154, 119, 1681, 140, 1960, 143, 2682,
    202, 153, 127, 2794, 239, 158, 155, 2643, 229, 172, 125, 2526, 201, 181, 159, 2554, 233, 167, 125, 2809,
    205, 164, 117, 2707, 221, 156, 138, 2922, 240, 160, 146, 2952, 267, 177, 149, 3339, 271, 175, 136, 3006,
    242, 168, 141, 3068, 232, 194, 149, 2760, 214, 208, 143, 2811, 218, 184, 149, 137, 15486, 2116, 235, 167,
    157, 2894, 305, 184, 139, 3090, 345, 179, 155, 3226, 347, 160, 164, 3275, 321, 184, 174, 3240, 269, 166,
    170, 3773, 265, 169, 155, 3023, 301, 188, 161, 3343, 275, 174, 155, 3526, 309, 177, 173, 3546, 307, 183,
    149, 3648, 295, 213, 170, 3568, 305, 198, 166, 3641, 297, 172, 148, 3608, 301, 200, 159, 3693, 322, 209,
    166, 3453, 318, 206, 162, 3696, 341, 200, 176, 3386, 320, 192, 176, 3903, 373, 207, 187, 3305, 361, 200,
    202, 3110, 367, 220, 197, 2357, 332, 196, 201, 1827, 377, 187, 199, 860, 472, 173, 223, 238
};

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

bool TestRecorder::SetVideoSource(RecorderTestParam::VideoRecorderConfig_ &recorderConfig)
{
    int32_t retValue = recorder->SetVideoSource(recorderConfig.vSource, recorderConfig.videoSourceId);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetAudioSource(RecorderTestParam::VideoRecorderConfig_ &recorderConfig)
{
    int32_t retValue = recorder->SetAudioSource(recorderConfig.aSource, recorderConfig.audioSourceId);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetOutputFormat(RecorderTestParam::VideoRecorderConfig_ &recorderConfig)
{
    int32_t retValue = recorder->SetOutputFormat(recorderConfig.outPutFormat);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetAudioEncoder(RecorderTestParam::VideoRecorderConfig_ &recorderConfig)
{
    int32_t retValue = recorder->SetAudioEncoder(recorderConfig.audioSourceId, recorderConfig.audioFormat);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetAudioSampleRate(RecorderTestParam::VideoRecorderConfig_ &recorderConfig)
{
    int32_t retValue = recorder->SetAudioSampleRate(recorderConfig.audioSourceId, recorderConfig.sampleRate);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetAudioChannels(RecorderTestParam::VideoRecorderConfig_ &recorderConfig)
{
    int32_t retValue = recorder->SetAudioChannels(recorderConfig.audioSourceId, recorderConfig.channelCount);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetAudioEncodingBitRate(RecorderTestParam::VideoRecorderConfig_ &recorderConfig)
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

bool TestRecorder::SetMaxDuration(RecorderTestParam::VideoRecorderConfig_ &recorderConfig)
{
    int32_t retValue = recorder->SetMaxDuration(recorderConfig.duration);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetOutputFile(RecorderTestParam::VideoRecorderConfig_ &recorderConfig)
{
    int32_t retValue = recorder->SetOutputFile(recorderConfig.outputFd);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetRecorderCallback(RecorderTestParam::VideoRecorderConfig_ &recorderConfig)
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

bool TestRecorder::Prepare(RecorderTestParam::VideoRecorderConfig_ &recorderConfig)
{
    int32_t retValue = recorder->Prepare();
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::Start(RecorderTestParam::VideoRecorderConfig_ &recorderConfig)
{
    int32_t retValue = recorder->Start();
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::Stop(bool block, RecorderTestParam::VideoRecorderConfig_ &recorderConfig)
{
    int32_t retValue = recorder->Stop(block);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::Reset(RecorderTestParam::VideoRecorderConfig_ &recorderConfig)
{
    int32_t retValue = recorder->Reset();
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::Release(RecorderTestParam::VideoRecorderConfig_ &recorderConfig)
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

bool TestRecorder::SetVideoEncoder(VideoRecorderConfig_ &recorderConfig)
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

bool TestRecorder::SetVideoSize(VideoRecorderConfig_ &recorderConfig)
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

bool TestRecorder::SetVideoFrameRate(VideoRecorderConfig_ &recorderConfig)
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

bool TestRecorder::SetVideoEncodingBitRate(VideoRecorderConfig_ &recorderConfig)
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

bool TestRecorder::SetCaptureRate(VideoRecorderConfig_ &recorderConfig, double fps)
{
    int32_t retValue = recorder->SetCaptureRate(recorderConfig.videoSourceId, fps);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetNextOutputFile(VideoRecorderConfig_ &recorderConfig)
{
    int32_t retValue = recorder->SetNextOutputFile(recorderConfig.outputFd);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetMaxFileSize(int64_t size, VideoRecorderConfig_ &recorderConfig)
{
    int32_t retValue = recorder->SetMaxFileSize(size);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

void TestRecorder::GetSurface(VideoRecorderConfig_ &recorderConfig)
{
    producerSurface = recorder->GetSurface(recorderConfig.videoSourceId);
}

bool TestRecorder::CameraServicesForVideo(VideoRecorderConfig_ &recorderConfig)
{
    RETURN_IF(TestRecorder::SetVideoEncoder(recorderConfig), false);
    RETURN_IF(TestRecorder::SetVideoSize(recorderConfig), false);
    RETURN_IF(TestRecorder::SetVideoFrameRate(recorderConfig), false);
    RETURN_IF(TestRecorder::SetVideoEncodingBitRate(recorderConfig), false);
    return true;
}

bool TestRecorder::CameraServicesForAudio(VideoRecorderConfig_ &recorderConfig)
{
    RETURN_IF(TestRecorder::SetAudioEncoder(recorderConfig), false);
    RETURN_IF(TestRecorder::SetAudioSampleRate(recorderConfig), false);
    RETURN_IF(TestRecorder::SetAudioChannels(recorderConfig), false);
    RETURN_IF(TestRecorder::SetAudioEncodingBitRate(recorderConfig), false);
    return true;
}

bool TestRecorder::SetFileSplitDuration(FileSplitType type, int64_t timestamp,
    uint32_t duration, VideoRecorderConfig_ &recorderConfig)
{
    int32_t retValue = recorder->SetFileSplitDuration(type, timestamp, duration);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::SetParameter(int32_t sourceId, const Format &format, VideoRecorderConfig_ &recorderConfig)
{
    int32_t retValue = recorder->SetParameter(sourceId, format);
    if (retValue != 0) {
        recorder->Release();
        close(recorderConfig.outputFd);
        return false;
    }
    return true;
}

bool TestRecorder::RequesetBuffer(const std::string &recorderType, VideoRecorderConfig_ &recorderConfig)
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
        return false;
    }
    const std::string filePath = "/data/test/media/out_320_240_10s.h264";
    file->open(filePath, std::ios::in | std::ios::binary);
    if (!(file->is_open())) {
        return false;
    }
    return true;
}

uint64_t TestRecorder::GetPts()
{
    struct timespec timestamp = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    uint64_t timeValue = static_cast<uint64_t>(timestamp.tv_sec) * SEC_TO_NS + static_cast<uint64_t>(timestamp.tv_nsec);
    return timeValue;
}

void TestRecorder::HDICreateESBuffer()
{
    const uint32_t *frameLenArray = HIGH_VIDEO_FRAME_SIZE;
    while (nowFrame < STUB_STREAM_SIZE) {
        if (isExit_.load()) {
            break;
        }
        usleep(FRAME_RATE);
        OHOS::sptr<OHOS::SurfaceBuffer> buffer;
        int32_t releaseFence;
        OHOS::SurfaceError retValue = producerSurface->RequestBuffer(buffer, releaseFence, g_esRequestConfig);
        DO_IF(retValue == OHOS::SURFACE_ERROR_NO_BUFFER, continue);
        DO_IF(retValue == SURFACE_ERROR_OK && buffer != nullptr, break);

        sptr<SyncFence> syncFence = new SyncFence(releaseFence);
        syncFence->Wait(100); // 100ms

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
            pts = GetPts();
            isStart_.store(false);
        }

        (void)buffer->GetExtraData()->ExtraSet("dataSize", static_cast<int32_t>(*frameLenArray));
        (void)buffer->GetExtraData()->ExtraSet("timeStamp", pts);
        (void)buffer->GetExtraData()->ExtraSet("isKeyFrame", isKeyFrame);
        ((++nowFrame % 30) == 0) ? (isKeyFrame = 1) : (isKeyFrame = 0);   // keyframe every 30fps
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
    constexpr int32_t countAbstract = 3;
    constexpr int32_t countSplit = 30;
    constexpr int32_t countColor = 255;
    while (nowFrame < STUB_STREAM_SIZE) {
        if (isExit_.load()) {
            break;
        }

        usleep(FRAME_RATE);
        OHOS::sptr<OHOS::SurfaceBuffer> buffer;
        int32_t releaseFence;
        OHOS::SurfaceError retValue = producerSurface->RequestBuffer(buffer, releaseFence, g_yuvRequestConfig);
        if (retValue == OHOS::SURFACE_ERROR_NO_BUFFER) {
            continue;
        }
        if (retValue != SURFACE_ERROR_OK || buffer == nullptr) {
            break;
        }

        sptr<SyncFence> syncFence = new SyncFence(releaseFence);
        syncFence->Wait(100);  // 100ms

        char *tempBuffer = (char *)(buffer->GetVirAddr());
        (void)memset_s(tempBuffer, YUV_BUFFER_SIZE, color, YUV_BUFFER_SIZE);
        for (uint32_t i = 0; i < YUV_BUFFER_SIZE - 1; i += (YUV_BUFFER_SIZE - 1)) {
            if (i >= YUV_BUFFER_SIZE - 1) {
                break;
            }
            tempBuffer[i] = static_cast<unsigned char>(PlayerTestParam::ProduceRandomNumberCrypt() % countColor);
        }

        color = color - countAbstract;

        if (color <= 0) {
            color = 0xFF;
        }

        pts = GetPts();
        (void)buffer->GetExtraData()->ExtraSet("dataSize", static_cast<int32_t>(YUV_BUFFER_SIZE));
        (void)buffer->GetExtraData()->ExtraSet("timeStamp", pts);
        (void)buffer->GetExtraData()->ExtraSet("isKeyFrame", isKeyFrame);
        nowFrame++;
        (nowFrame % countSplit) == 0 ? (isKeyFrame = 1) : (isKeyFrame = 0);
        (void)producerSurface->FlushBuffer(buffer, -1, g_yuvFlushConfig);
    }
}

void TestRecorder::StopBuffer(const std::string &recorderType)
{
    if (recorderType != PURE_AUDIO && camereHDIThread != nullptr) {
        camereHDIThread->join();
    }
}

bool TestRecorder::SetConfig(const std::string &recorderType, VideoRecorderConfig_ &recorderConfig)
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