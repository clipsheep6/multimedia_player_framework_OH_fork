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

#include "videoenc_ndk_sample.h"
#include <sync_fence.h>
#include "audio_info.h"
#include "av_common.h"
#include "avcodec_video_encoder.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;

namespace {

    constexpr uint32_t DEFAULT_WIDTH = 480;
    constexpr uint32_t DEFAULT_HEIGHT = 360;
    constexpr uint32_t DEFAULT_FRAME_RATE = 30;
    constexpr uint32_t YUV_BUFFER_SIZE = 259200; // 480 * 360 * 3 / 2
    constexpr uint32_t STRIDE_ALIGN = 8;
    constexpr uint32_t DEFAULT_FRAME_COUNT = 50;
    const string MIME_TYPE = "video/mp4v-es";
    constexpr bool NEED_DUMP = true;
    const char * OUT_DIR = "/data/media/venc_out.es";

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

void VencAsyncError(AVCodec *codec, int32_t errorCode, void *userData)
{
    cout << "Error errorCode=" << errorCode << endl;
}

void VencAsyncStreamChanged(AVCodec *codec, AVFormat *format, void *userData)
{
    cout << "Format Changed" << endl;
}

void VencAsyncNeedInputData(AVCodec *codec, uint32_t index, AVMemory *data, void *userData)
{
    (void)codec;
}

void VencAsyncNewOutputData(AVCodec *codec, uint32_t index, AVMemory *data, AVCodecBufferAttr *attr, void *userData)
{
    VEncSignal* signal_ = static_cast<VEncSignal *>(userData);
    cout << "OutputAvailable, index = " << index << endl;
    unique_lock<mutex> lock(signal_->mutex_);
    signal_->indexQueue_.push(index);
    signal_->sizeQueue_.push(attr->size);
    signal_->outBufferQueue_.push(data);
    signal_->cond_.notify_all();
}

VEncNdkSample::~VEncNdkSample()
{
    Release();

    delete signal_;
    signal_ = nullptr;
}

void VEncNdkSample::RunVideoEnc(void)
{
    venc_ = CreateVideoEncoder();
    NDK_CHECK_AND_RETURN_LOG(venc_ != nullptr, "Fatal: CreateVideoEncoder fail");
    struct AVFormat *format = CreateFormat();
    NDK_CHECK_AND_RETURN_LOG(format != nullptr, "Fatal: CreateFormat fail");

    NDK_CHECK_AND_RETURN_LOG(Configure(format) == AV_ERR_OK, "Fatal: Configure fail");
    NDK_CHECK_AND_RETURN_LOG(GetSurface() == AV_ERR_OK && surface_ != nullptr, "Fatal: GetSurface fail");

    NDK_CHECK_AND_RETURN_LOG(Prepare() == AV_ERR_OK, "Fatal: Prepare fail");
    NDK_CHECK_AND_RETURN_LOG(Start() == AV_ERR_OK, "Fatal: Start fail");

    GenerateData(DEFAULT_FRAME_COUNT, DEFAULT_FRAME_RATE);
    NDK_CHECK_AND_RETURN_LOG(Stop() == AV_ERR_OK, "Fatal: Start fail");
    NDK_CHECK_AND_RETURN_LOG(Release() == AV_ERR_OK, "Fatal: Release fail");
}

void VEncNdkSample::GenerateData(uint32_t count, uint32_t fps)
{
    if (fps == 0) {
        return;
    }
    constexpr uint32_t secToUs = 1000000;
    uint32_t intervalUs = secToUs / fps;
    frameCount_ = 0;
    while (frameCount_ <= count) {
        usleep(intervalUs);

        sptr<OHOS::SurfaceBuffer> buffer = nullptr;
        int32_t fence = -1;
        if (surface_->RequestBuffer(buffer, fence, g_request) != SURFACE_ERROR_OK) {
            continue;
        }
        NDK_CHECK_AND_BREAK_LOG(buffer != nullptr, "Fatal: SurfaceBuffer is nullptr");

        sptr<SyncFence> tempFence = new SyncFence(fence);
        tempFence->Wait(100); // 100ms

        auto addr = static_cast<uint8_t *>(buffer->GetVirAddr());
        if (addr == nullptr) {
            cout << "Fatal: SurfaceBuffer address is nullptr" << endl;
            (void)surface_->CancelBuffer(buffer);
            break;
        }
        NDK_CHECK_AND_BREAK_LOG(memset_s(addr, buffer->GetSize(), 0xFF, YUV_BUFFER_SIZE) == EOK, "Fatal"); // 纯白色0xff

        const sptr<OHOS::BufferExtraData>& extraData = buffer->GetExtraData();
        NDK_CHECK_AND_BREAK_LOG(extraData != nullptr, "Fatal: SurfaceBuffer is nullptr");
        (void)extraData->ExtraSet("timeStamp", timestampNs_);
        timestampNs_ += static_cast<int64_t>(intervalUs); // us to ns

        (void)surface_->FlushBuffer(buffer, -1, g_flushConfig);
        cout << "Generate input buffer success, timestamp: " << timestampNs_ << endl;
        frameCount_++;
    }
}

struct AVCodec* VEncNdkSample::CreateVideoEncoder(void)
{
    struct AVCodec *codec = OH_AVCODEC_CreateVideoEncoderByMime(MIME_TYPE.c_str());
    NDK_CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "Fatal: OH_AVCODEC_CreateVideoEncoderByMime");

    signal_ = new VEncSignal();
    NDK_CHECK_AND_RETURN_RET_LOG(signal_ != nullptr, nullptr, "Fatal: No Memory");

    cb_.onAsyncError = VencAsyncError;
    cb_.onAsyncStreamChanged = VencAsyncStreamChanged;
    cb_.onAsyncNeedInputData = VencAsyncNeedInputData;
    cb_.onAsyncNewOutputData = VencAsyncNewOutputData;
    int32_t ret = OH_AVCODEC_VideoEncoderSetCallback(codec, cb_, static_cast<void *>(signal_));
    NDK_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, NULL, "Fatal: OH_AVCODEC_VideoEncoderSetCallback");
    return codec;
}

struct AVFormat* VEncNdkSample::CreateFormat(void)
{
    struct AVFormat *format = OH_AV_CreateFormat();
    string width = "width";
    string height = "height";
    string pixel_format = "pixel_format";
    string frame_rate = "frame_rate";

    if (format != NULL) {
        (void)OH_AV_FormatPutIntValue(format, width.c_str(), DEFAULT_WIDTH);
        (void)OH_AV_FormatPutIntValue(format, height.c_str(), DEFAULT_HEIGHT);
        (void)OH_AV_FormatPutIntValue(format, pixel_format.c_str(), NV21);
        (void)OH_AV_FormatPutIntValue(format, frame_rate.c_str(), DEFAULT_FRAME_RATE);
    }
    return format;
}

struct VEncObject : public AVCodec {
    explicit VEncObject(const std::shared_ptr<AVCodecVideoEncoder> &encoder)
        : AVCodec(AVMagic::MEDIA_MAGIC_VIDEO_ENCODER), videoEncoder_(encoder) {}
    ~VEncObject() = default;

    const std::shared_ptr<AVCodecVideoEncoder> videoEncoder_;
};

int32_t VEncNdkSample::GetSurface()
{
    struct VEncObject *videoEncObj = reinterpret_cast<VEncObject *>(venc_);
    NDK_CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoEncoder is nullptr!");

    surface_ = videoEncObj->videoEncoder_->CreateInputSurface();
    NDK_CHECK_AND_RETURN_RET_LOG(surface_ != nullptr, AV_ERR_OPERATE_NOT_PERMIT, "videoEncoder CreateInputSurface failed!");
    return AV_ERR_OK;
    // return OH_AVCODEC_VideoEncoderGetInputSurface(venc_, window);
}

int32_t VEncNdkSample::Configure(struct AVFormat *format)
{
    return OH_AVCODEC_VideoEncoderConfigure(venc_, format);
}


int32_t VEncNdkSample::Prepare()
{
    return OH_AVCODEC_VideoEncoderPrepare(venc_);
}

int32_t VEncNdkSample::Start()
{
    isRunning_.store(true);
    readLoop_ = make_unique<thread>(&VEncNdkSample::LoopFunc, this);
    NDK_CHECK_AND_RETURN_RET_LOG(readLoop_ != nullptr, AV_ERR_UNKNOWN, "Fatal: No memory");

    return OH_AVCODEC_VideoEncoderStart(venc_);
}

int32_t VEncNdkSample::Stop()
{
    isRunning_.store(false);
    if (readLoop_ != nullptr && readLoop_->joinable()) {
        unique_lock<mutex> queueLock(signal_->mutex_);
        signal_->indexQueue_.push(0);
        signal_->cond_.notify_all();
        queueLock.unlock();
        readLoop_->join();
        readLoop_.reset();
    }

    return OH_AVCODEC_VideoEncoderStop(venc_);
}

int32_t VEncNdkSample::Flush()
{
    return OH_AVCODEC_VideoEncoderFlush(venc_);
}

int32_t VEncNdkSample::Reset()
{
    return OH_AVCODEC_VideoEncoderReset(venc_);
}

int32_t VEncNdkSample::Release()
{
    AVErrCode ret = AV_ERR_OK;
    if (venc_ != nullptr) {
        ret = OH_AVCODEC_DestroyVideoEncoder(venc_);
        venc_ = (ret == AV_ERR_OK) ? nullptr : venc_;
    }
    return ret;
}

int32_t VEncNdkSample::SetParameter(AVFormat *format)
{
    return OH_AVCODEC_VideoEncoderSetParameter(venc_, format);
}

void VEncNdkSample::LoopFunc()
{
    while (true) {
        if (!isRunning_.load()) {
            break;
        }

        unique_lock<mutex> lock(signal_->mutex_);
        signal_->cond_.wait(lock, [this](){ return signal_->indexQueue_.size() > 0; });

        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = signal_->indexQueue_.front();
        auto buffer = signal_->outBufferQueue_.front();
        uint32_t size = signal_->sizeQueue_.front();

        if (buffer == nullptr) {
            cout << "Fatal: GetOutputBuffer fail, exit" << endl;
            break;
        }
        cout << "GetOutputBuffer size : " << size << " frameCount_ =  " << frameCount_ << endl;

        if (NEED_DUMP) {
            FILE *outFile;
            outFile = fopen(OUT_DIR, "a");
            if (outFile == nullptr) {
                cout << "dump data fail" << endl;
            } else {
                fwrite(OH_AV_MemoryGetAddr(buffer), 1, size, outFile);
            }
            fclose(outFile);
        }

        if (OH_AVCODEC_VideoEncoderFreeOutputData(venc_, index) != AV_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail, exit" << endl;
            break;
        }
        signal_->indexQueue_.pop();
        signal_->sizeQueue_.pop();
        signal_->outBufferQueue_.pop();
    }
}
