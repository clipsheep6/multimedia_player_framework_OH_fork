/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "avcodec_vdec_demo.h"
#include <iostream>
#include <unistd.h>
#include "securec.h"
#include "demo_log.h"
#include "media_errors.h"
#include "ui/rs_surface_node.h"
#include "window.h"
#include "window_option.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
namespace {
    constexpr uint32_t DEFAULT_WIDTH = 320;
    constexpr uint32_t DEFAULT_HEIGHT = 240;
    constexpr uint32_t DEFAULT_FRAME_RATE = 30;
    constexpr uint32_t MAX_INPUT_BUFFER_SIZE = 30000;
    constexpr uint32_t FRAME_DURATION_US = 33000;
    constexpr uint32_t DEFAULT_FRAME_COUNT = 290;
    constexpr uint32_t input[] = { 2106, 11465, 321, 72, 472, 68, 76, 79, 509, 90, 677, 88, 956, 99, 347, 77, 452, 681, 81, 1263, 94, 106, 97,
        998, 97, 797, 93, 1343, 150, 116, 117, 926, 1198, 128, 110, 78, 1582, 158, 135, 112, 1588, 165, 132,
        128, 1697, 168, 149, 117, 1938, 170, 141, 142, 1830, 106, 161, 122, 1623, 160, 154, 156, 1998, 230,
        177, 139, 1650, 186, 128, 134, 1214, 122, 1411, 120, 1184, 128, 1591, 195, 145, 105, 1587, 169, 140,
        118, 1952, 177, 150, 161, 1437, 159, 123, 1758, 180, 165, 144, 1936, 214, 191, 175, 2122, 180, 179,
        160, 1927, 161, 184, 119, 1973, 218, 210, 129, 1962, 196, 127, 154, 2308, 173, 127, 1572, 142, 122,
        2065, 262, 159, 206, 2251, 269, 179, 170, 2056, 308, 168, 191, 2090, 303, 191, 110, 1932, 272, 162,
        122, 1877, 245, 167, 141, 1908, 294, 162, 118, 1493, 132, 1782, 273, 184, 133, 1958, 274, 180, 149,
        2070, 216, 169, 143, 1882, 224, 149, 139, 1749, 277, 184, 139, 2141, 197, 170, 140, 2002, 269, 162,
        140, 1862, 202, 179, 131, 1868, 214, 164, 140, 1546, 226, 150, 130, 1707, 162, 146, 1824, 181, 147,
        130, 1898, 209, 143, 131, 1805, 180, 148, 106, 1776, 147, 141, 1572, 177, 130, 105, 1776, 178, 144,
        122, 1557, 142, 124, 114, 1436, 143, 126, 1326, 127, 1755, 169, 127, 105, 1807, 177, 131, 134, 1613,
        187, 137, 136, 1314, 134, 118, 2005, 194, 129, 147, 1566, 185, 132, 131, 1236, 174, 137, 106, 11049,
        574, 126, 1242, 188, 130, 119, 1450, 187, 137, 141, 1116, 124, 1848, 138, 122, 1605, 186, 127, 140,
        1798, 170, 124, 121, 1666, 157, 128, 130, 1678, 135, 118, 1804, 169, 135, 125, 1837, 168, 124, 124};
}

void VDecDemo::RunCase()
{
    DEMO_CHECK_AND_RETURN_LOG(CreateVdec() == MSERR_OK, "Fatal: CreateVdec fail");

    Format format;
    format.PutIntValue("width", DEFAULT_WIDTH);
    format.PutIntValue("height", DEFAULT_HEIGHT);
    format.PutIntValue("pixel_format", NV12);
    format.PutIntValue("frame_rate", DEFAULT_FRAME_RATE);
    format.PutIntValue("max_input_size", MAX_INPUT_BUFFER_SIZE);
    DEMO_CHECK_AND_RETURN_LOG(Configure(format) == MSERR_OK, "Fatal: Configure fail");

    DEMO_CHECK_AND_RETURN_LOG(SetSurface() == MSERR_OK, "Fatal: SetSurface fail");
    DEMO_CHECK_AND_RETURN_LOG(Prepare() == MSERR_OK, "Fatal: Prepare fail");
    DEMO_CHECK_AND_RETURN_LOG(Start() == MSERR_OK, "Fatal: Start fail");
    sleep(3); // start run 3s
    DEMO_CHECK_AND_RETURN_LOG(Stop() == MSERR_OK, "Fatal: Stop fail");
    DEMO_CHECK_AND_RETURN_LOG(Release() == MSERR_OK, "Fatal: Release fail");
}

int32_t VDecDemo::CreateVdec()
{
    vdec_ = VideoDecoderFactory::CreateByMime("video/avc");
    DEMO_CHECK_AND_RETURN_RET_LOG(vdec_ != nullptr, MSERR_UNKNOWN, "Fatal: CreateByMime fail");

    signal_ = make_shared<VDecSignal>();
    DEMO_CHECK_AND_RETURN_RET_LOG(signal_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");

    cb_ = make_unique<VDecDemoCallback>(signal_);
    DEMO_CHECK_AND_RETURN_RET_LOG(cb_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");
    DEMO_CHECK_AND_RETURN_RET_LOG(vdec_->SetCallback(cb_) == MSERR_OK, MSERR_UNKNOWN, "Fatal: SetCallback fail");

    return MSERR_OK;
}

int32_t VDecDemo::Configure(const Format &format)
{
    return vdec_->Configure(format);
}

int32_t VDecDemo::Prepare()
{
    return vdec_->Prepare();
}

int32_t VDecDemo::Start()
{
    isRunning_.store(true);

    testFile_ = std::make_unique<std::ifstream>();
    DEMO_CHECK_AND_RETURN_RET_LOG(testFile_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");
    testFile_->open("/data/media/video.es", std::ios::in | std::ios::binary);

    inputLoop_ = make_unique<thread>(&VDecDemo::InputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");

    outputLoop_ = make_unique<thread>(&VDecDemo::OutputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");

    return vdec_->Start();
}

int32_t VDecDemo::Stop()
{
    isRunning_.store(false);

    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inQueue_.push(0);
        signal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
        inputLoop_.reset();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outQueue_.push(0);
        signal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
        outputLoop_.reset();
    }

    return vdec_->Stop();
}

int32_t VDecDemo::Flush()
{
    return vdec_->Flush();
}

int32_t VDecDemo::Reset()
{
    return vdec_->Reset();
}

int32_t VDecDemo::Release()
{
    return vdec_->Release();
}

int32_t VDecDemo::SetSurface()
{
    sptr<Rosen::WindowOption> option = new Rosen::WindowOption();
    option->SetWindowRect({0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT});
    option->SetWindowType(Rosen::WindowType::WINDOW_TYPE_APP_LAUNCHING);
    option->SetWindowMode(Rosen::WindowMode::WINDOW_MODE_FLOATING);
    sptr<Rosen::Window> window = Rosen::Window::Create("avcodec video decoder window", option);
    DEMO_CHECK_AND_RETURN_RET_LOG(window != nullptr && window->GetSurfaceNode() != nullptr, MSERR_UNKNOWN, "Fatal");

    sptr<Surface> surface = window->GetSurfaceNode()->GetSurface();
    window->Show();
    DEMO_CHECK_AND_RETURN_RET_LOG(surface != nullptr, MSERR_UNKNOWN, "Fatal: get surface fail");
    return vdec_->SetOutputSurface(surface);
}

void VDecDemo::InputFunc()
{
    while (true) {
        if (!isRunning_.load()) {
            break;
        }

        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this](){ return signal_->inQueue_.size() > 0; });

        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = signal_->inQueue_.front();
        auto buffer = vdec_->GetInputBuffer(index);
        DEMO_CHECK_AND_BREAK_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");
        DEMO_CHECK_AND_BREAK_LOG(testFile_ != nullptr && testFile_->is_open(), "Fatal: open file fail");

        uint32_t bufferSize = input[frameCount_]; // replace with the actual size
        char *fileBuffer = (char *)malloc(sizeof(char) * bufferSize + 1);
        DEMO_CHECK_AND_BREAK_LOG(fileBuffer != nullptr, "Fatal: malloc fail");

        (void)testFile_->read(fileBuffer, bufferSize);
        if (memcpy_s(buffer->GetBase(), buffer->GetSize(), fileBuffer, bufferSize) != EOK) {
            free(fileBuffer);
            cout << "Fatal: memcpy fail" << endl;
            break;
        }

        AVCodecBufferInfo info;
        info.size = bufferSize;
        info.offset = 0;
        info.presentationTimeUs = timeStamp_;

        int32_t ret = MSERR_OK;
        if (isFirstFrame_) {
            ret = vdec_->QueueInputBuffer(index, info, AVCODEC_BUFFER_FLAG_CODEC_DATA);
            isFirstFrame_ = false;
        } else {
            ret = vdec_->QueueInputBuffer(index, info, AVCODEC_BUFFER_FLAG_NONE);
        }

        free(fileBuffer);
        timeStamp_ += FRAME_DURATION_US;
        signal_->inQueue_.pop();

        frameCount_++;
        if (frameCount_ == DEFAULT_FRAME_COUNT) {
            cout << "Finish decode, exit" << endl;
            break;
        }

        if (ret != MSERR_OK) {
            cout << "Fatal error, exit" << endl;
            break;
        }
    }
}

void VDecDemo::OutputFunc()
{
    while (true) {
        if (!isRunning_.load()) {
            break;
        }

        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this](){ return signal_->outQueue_.size() > 0; });

        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = signal_->outQueue_.front();
        if (vdec_->ReleaseOutputBuffer(index, true) != MSERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
            break;
        }

        signal_->outQueue_.pop();
    }
}

VDecDemoCallback::VDecDemoCallback(shared_ptr<VDecSignal> signal)
    : signal_(signal)
{
}

void VDecDemoCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    cout << "Error received, errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void VDecDemoCallback::OnOutputFormatChanged(const Format &format)
{
    cout << "OnOutputFormatChanged received" << endl;
}

void VDecDemoCallback::OnInputBufferAvailable(uint32_t index)
{
    cout << "OnInputBufferAvailable received, index:" << index << endl;
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inQueue_.push(index);
    signal_->inCond_.notify_all();
}

void VDecDemoCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    cout << "OnOutputBufferAvailable received, index:" << index << endl;
    unique_lock<mutex> lock(signal_->outMutex_);
    signal_->outQueue_.push(index);
    signal_->outCond_.notify_all();
}