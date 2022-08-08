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

#include "avcodec_venc_demo.h"
#include <iostream>
#include <fstream>
#include <sync_fence.h>
#include "securec.h"
#include "demo_log.h"
#include "display_type.h"
#include "media_errors.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
namespace {
    constexpr uint32_t DEFAULT_WIDTH = 480;
    constexpr uint32_t DEFAULT_HEIGHT = 360;
    constexpr uint32_t DEFAULT_FRAME_RATE = 30;
    constexpr uint32_t YUV_BUFFER_SIZE = 259200; // 480 * 360 * 3 / 2
    constexpr uint32_t STRIDE_ALIGN = 8;
    constexpr int32_t FAST_PRODUCER = 50; // 50 fps producer, used to test max_encoder_fps property
    constexpr int32_t SLOW_PRODUCER = 20; // 20 fps producer, used to test repeat_frame_after property
    constexpr uint32_t REPEAT_FRAME_AFTER_MS = 50;
    constexpr uint32_t DEFAULT_FRAME_COUNT = 50;
}

static BufferFlushConfig g_flushConfig = {
    .damage = {
        .x = 0,
        .y = 0,
        .w = DEFAULT_WIDTH,
        .h = DEFAULT_HEIGHT
    },
    .timestamp = 0
};

static BufferRequestConfig g_request = {
    .width = DEFAULT_WIDTH,
    .height = DEFAULT_HEIGHT,
    .strideAlignment = STRIDE_ALIGN,
    .format = PIXEL_FMT_YCRCB_420_SP,
    .usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA,
    .timeout = 0
};

int32_t VEncDemo::String2Int(const string &str)
{
    int32_t ret = 0;
    if (str == "\n") {
        std::cout << "Enter enter, convert enter to 0" << endl;
        ret = 0;
    } else {
        ret = atoi(str.c_str());
    }
    return ret;
}

void VEncDemo::RunCase(bool enableProp)
{
    DEMO_CHECK_AND_RETURN_LOG(CreateVenc() == MSERR_OK, "Fatal: CreateVenc fail");

    std::cout << "Enter profile: " << endl;
    cout << "profile baseline : 0" << endl;
    cout << "profile high : 4" << endl;
    cout << "profile main : 8" << endl;
    std::string proflie = "";
    (void)getline(cin, proflie);
    std::cout << "Enter bitmode: " << endl;
    cout << "bitmode CBR : 0" << endl;
    cout << "bitmode VBR : 1" << endl;
    std::string bitmode = "";
    (void)getline(cin, bitmode);

    int32_t pro = String2Int(proflie);
    int32_t bmode = String2Int(bitmode);

    Format format;
    format.PutIntValue("width", DEFAULT_WIDTH);
    format.PutIntValue("height", DEFAULT_HEIGHT);
    format.PutIntValue("pixel_format", NV21);
    format.PutIntValue("frame_rate", DEFAULT_FRAME_RATE);
    format.PutIntValue("video_encode_bitrate_mode", bmode);
    format.PutIntValue("codec_profile", pro);
    DEMO_CHECK_AND_RETURN_LOG(Configure(format) == MSERR_OK, "Fatal: Configure fail");
    surface_ = GetVideoSurface();
    DEMO_CHECK_AND_RETURN_LOG(surface_ != nullptr, "Fatal: GetVideoSurface fail");
    DEMO_CHECK_AND_RETURN_LOG(Prepare() == MSERR_OK, "Fatal: Prepare fail");
    DEMO_CHECK_AND_RETURN_LOG(Start() == MSERR_OK, "Fatal: Start fail");

    if (enableProp) {
        DEMO_CHECK_AND_RETURN_LOG(SetParameter(0, DEFAULT_FRAME_RATE, REPEAT_FRAME_AFTER_MS) == MSERR_OK,
            "Fatal: SetParameter fail");
        GenerateData(DEFAULT_FRAME_COUNT, FAST_PRODUCER);
        GenerateData(DEFAULT_FRAME_COUNT, SLOW_PRODUCER);
        DEMO_CHECK_AND_RETURN_LOG(SetParameter(1, 0, 0) == MSERR_OK, "Fatal: Set suspend fail");
        GenerateData(DEFAULT_FRAME_COUNT, DEFAULT_FRAME_RATE);
    } else {
        GenerateData(DEFAULT_FRAME_COUNT, DEFAULT_FRAME_RATE);
    }
    DEMO_CHECK_AND_RETURN_LOG(Stop() == MSERR_OK, "Fatal: Stop fail");
    DEMO_CHECK_AND_RETURN_LOG(Release() == MSERR_OK, "Fatal: Release fail");
}

void VEncDemo::GenerateData(uint32_t count, uint32_t fps)
{
    if (fps == 0) {
        return;
    }
    constexpr uint32_t secToUs = 1000000;
    uint32_t intervalUs = secToUs / fps;
    uint32_t frameCount = 0;
    while (frameCount <= count) {
        usleep(intervalUs);

        sptr<OHOS::SurfaceBuffer> buffer = nullptr;
        int32_t fence = -1;
        if (surface_->RequestBuffer(buffer, fence, g_request) != SURFACE_ERROR_OK) {
            continue;
        }
        DEMO_CHECK_AND_BREAK_LOG(buffer != nullptr, "Fatal: SurfaceBuffer is nullptr");

        sptr<SyncFence> tempFence = new SyncFence(fence);
        tempFence->Wait(100); // 100ms

        auto addr = static_cast<uint8_t *>(buffer->GetVirAddr());
        if (addr == nullptr) {
            cout << "Fatal: SurfaceBuffer address is nullptr" << endl;
            (void)surface_->CancelBuffer(buffer);
            break;
        }
        DEMO_CHECK_AND_BREAK_LOG(memset_s(addr, buffer->GetSize(), 0xFF, YUV_BUFFER_SIZE) == EOK, "Fatal");

        const sptr<OHOS::BufferExtraData>& extraData = buffer->GetExtraData();
        DEMO_CHECK_AND_BREAK_LOG(extraData != nullptr, "Fatal: SurfaceBuffer is nullptr");
        (void)extraData->ExtraSet("timeStamp", timestampNs_);
        timestampNs_ += static_cast<int64_t>(intervalUs * 1000); // us to ns

        (void)surface_->FlushBuffer(buffer, -1, g_flushConfig);
        cout << "Generate input buffer success, timestamp: " << timestampNs_ << endl;
        frameCount++;
    }
}

int32_t VEncDemo::CreateVenc()
{
    std::cout << "Enter create type: " << endl;
    cout << "selectt CreateByName: 1" << endl;
    cout << "select CreateByMime: 2" << endl;
    string createType = "";
    (void)getline(cin, createType);
    if (createType.compare("1") == 0) {
        codername = "openh264enc";
        std::cout << "Enter media coder name: " << endl;
        cout << "selectt openh264enc: 1" << endl;
        cout << "select avenc_mpeg4: 2" << endl;
        string encodeMode = "";
        (void)getline(cin, encodeMode);
        if (encodeMode.compare("1") == 0) {
            cout << "select openh264enc" << endl;
        } else {
            cout << "select avenc_mpeg4"<< endl;
            codername = "avenc_mpeg4";
        }
        venc_ = VideoEncoderFactory::CreateByName(codername);
        DEMO_CHECK_AND_RETURN_RET_LOG(venc_ != nullptr, MSERR_UNKNOWN, "Fatal: CreateByName fail");
    } else {
        venc_ = VideoEncoderFactory::CreateByMime("video/mp4v-es");
        DEMO_CHECK_AND_RETURN_RET_LOG(venc_ != nullptr, MSERR_UNKNOWN, "Fatal: CreateByMime fail");
    }

    signal_ = make_shared<VEncSignal>();
    DEMO_CHECK_AND_RETURN_RET_LOG(signal_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");

    cb_ = make_shared<VEncDemoCallback>(signal_);
    DEMO_CHECK_AND_RETURN_RET_LOG(cb_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");
    DEMO_CHECK_AND_RETURN_RET_LOG(venc_->SetCallback(cb_) == MSERR_OK, MSERR_UNKNOWN, "Fatal: SetCallback fail");

    return MSERR_OK;
}

int32_t VEncDemo::Configure(const Format &format)
{
    return venc_->Configure(format);
}

int32_t VEncDemo::Prepare()
{
    return venc_->Prepare();
}

int32_t VEncDemo::Start()
{
    isRunning_.store(true);
    readLoop_ = make_unique<thread>(&VEncDemo::LoopFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(readLoop_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");
    return venc_->Start();
}

int32_t VEncDemo::SetParameter(int32_t suspend, int32_t maxFps, int32_t repeatMs)
{
    Format format;
    format.PutIntValue("suspend_input_surface", suspend);
    format.PutIntValue("max_encoder_fps", maxFps);
    format.PutIntValue("repeat_frame_after", repeatMs);
    return venc_->SetParameter(format);
}

int32_t VEncDemo::Stop()
{
    isRunning_.store(false);

    if (readLoop_ != nullptr && readLoop_->joinable()) {
        unique_lock<mutex> queueLock(signal_->mutex_);
        signal_->bufferQueue_.push(0);
        signal_->sizeQueue_.push(0);
        signal_->cond_.notify_all();
        queueLock.unlock();
        readLoop_->join();
        readLoop_.reset();
    }

    return venc_->Stop();
}

int32_t VEncDemo::Flush()
{
    return venc_->Flush();
}

int32_t VEncDemo::Reset()
{
    return venc_->Reset();
}

int32_t VEncDemo::Release()
{
    return venc_->Release();
}

sptr<Surface> VEncDemo::GetVideoSurface()
{
    return venc_->CreateInputSurface();
}

void VEncDemo::LoopFunc()
{
    std::ofstream ofs;
    if (codername.compare("openh264enc") == 0) {
            ofs.open("/data/media/avc.h264", ios::out| ios::app);
        } else {
            ofs.open("/data/media/mpeg4.mpeg4", ios::out| ios::app);
    }
    if (!ofs.is_open()) {
            std::cout << "open file failed" << std::endl;
    }
    while (true) {
        if (!isRunning_.load()) {
            break;
        }

        unique_lock<mutex> lock(signal_->mutex_);
        signal_->cond_.wait(lock, [this](){ return signal_->bufferQueue_.size() > 0; });

        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = signal_->bufferQueue_.front();
        uint32_t size = signal_->sizeQueue_.front();
        auto buffer = venc_->GetOutputBuffer(index);
        ofs.write(reinterpret_cast<char *>(buffer->GetBase()), size);
        if (!buffer) {
            cout << "Fatal: GetOutputBuffer fail, exit" << endl;
            break;
        }

        if (venc_->ReleaseOutputBuffer(index) != MSERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail, exit" << endl;
            break;
        }
        signal_->bufferQueue_.pop();
        signal_->sizeQueue_.pop();
    }
    ofs.close();
}

VEncDemoCallback::VEncDemoCallback(shared_ptr<VEncSignal> signal)
    : signal_(signal)
{
}

void VEncDemoCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    cout << "Error received, errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void VEncDemoCallback::OnOutputFormatChanged(const Format &format)
{
    cout << "OnOutputFormatChanged received" << endl;
}

void VEncDemoCallback::OnInputBufferAvailable(uint32_t index)
{
    (void)index;
}

void VEncDemoCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    cout << "OnOutputBufferAvailable received, index:" << index << " timestamp:" << info.presentationTimeUs << endl;
    cout << "OnOutputBufferAvailable received, index:" << index << " size:" << info.size << endl;
    unique_lock<mutex> lock(signal_->mutex_);
    signal_->bufferQueue_.push(index);
    signal_->sizeQueue_.push(info.size);
    signal_->cond_.notify_all();
}
