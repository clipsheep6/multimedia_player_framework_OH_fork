/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "videodec_native_sample.h"
#include "videoenc_native_sample.h"
#include <sync_fence.h>
#include "audio_info.h"
#include "av_common.h"
#include "avcodec_video_decoder.h"
#include "avcodec_video_encoder.h"


// #include "window.h"
// #include "window_option.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;

namespace {

    constexpr uint32_t DEFAULT_WIDTH = 320;
    constexpr uint32_t DEFAULT_HEIGHT = 240;
    constexpr uint32_t DEFAULT_FRAME_RATE = 60;
    constexpr uint32_t FRAME_DURATION_US = 16670;
    const string MIME_TYPE = "video/mp4v-es";
    constexpr bool NEED_DUMP = true;

    const char * VENC_OUT_DIR = "/data/media/venc_out.es";
    const char * INP_DIR = "/data/media/out_320_240_10s.h264";
    const uint32_t ES[] = // H264_FRAME_SIZE_240
      { 2106, 11465, 321, 72, 472, 68, 76, 79, 509, 90, 677, 88, 956, 99, 347, 77, 452, 681, 81, 1263, 94, 106, 97,
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
        1798, 170, 124, 121, 1666, 157, 128, 130, 1678, 135, 118, 1804, 169, 135, 125, 1837, 168, 124, 124,
        2049, 180, 122, 128, 1334, 143, 128, 1379, 116, 1884, 149, 122, 150, 1962, 176, 122, 122, 1197, 139,
        1853, 184, 151, 148, 1692, 209, 129, 126, 1736, 149, 135, 104, 1775, 165, 160, 121, 1653, 163, 123,
        112, 1907, 181, 129, 107, 1808, 177, 125, 111, 2405, 166, 144, 114, 1833, 198, 136, 113, 1960, 206,
        139, 116, 1791, 175, 130, 129, 1909, 194, 138, 119, 1807, 160, 156, 124, 1998, 184, 173, 114, 2069, 181,
        127, 139, 2212, 182, 138, 146, 1993, 214, 135, 139, 2286, 194, 137, 120, 2090, 196, 159, 132, 2294, 194,
        148, 137, 2312, 183, 163, 106, 2118, 201, 158, 127, 2291, 187, 144, 116, 2413, 139, 115, 2148, 178, 122,
        103, 2370, 207, 161, 117, 2291, 213, 159, 129, 2244, 243, 157, 133, 2418, 255, 171, 127, 2316, 185, 160,
        132, 2405, 220, 165, 155, 2539, 219, 172, 128, 2433, 199, 154, 119, 1681, 140, 1960, 143, 2682, 202, 153,
        127, 2794, 239, 158, 155, 2643, 229, 172, 125, 2526, 201, 181, 159, 2554, 233, 167, 125, 2809, 205, 164,
        117, 2707, 221, 156, 138, 2922, 240, 160, 146, 2952, 267, 177, 149, 3339, 271, 175, 136, 3006, 242, 168,
        141, 3068, 232, 194, 149, 2760, 214, 208, 143, 2811, 218, 184, 149, 137, 15486, 2116, 235, 167, 157, 2894,
        305, 184, 139, 3090, 345, 179, 155, 3226, 347, 160, 164, 3275, 321, 184, 174, 3240, 269, 166, 170, 3773,
        265, 169, 155, 3023, 301, 188, 161, 3343, 275, 174, 155, 3526, 309, 177, 173, 3546, 307, 183, 149, 3648,
        295, 213, 170, 3568, 305, 198, 166, 3641, 297, 172, 148, 3608, 301, 200, 159, 3693, 322, 209, 166, 3453,
        318, 206, 162, 3696, 341, 200, 176, 3386, 320, 192, 176, 3903, 373, 207, 187, 3305, 361, 200, 202, 3110,
        367, 220, 197, 2357, 332, 196, 201, 1827, 377, 187, 199, 860, 472, 173, 223, 238};
        constexpr uint32_t ES_LENGTH = sizeof(ES) / sizeof(uint32_t);
}

// vdec
void VdecAsyncError(AVCodec *codec, int32_t errorCode, void *userData)
{
    cout << "VDec Error errorCode=" << errorCode << endl;
}

void VdecAsyncStreamChanged(AVCodec *codec, AVFormat *format, void *userData)
{
    cout << "VDec Format Changed" << endl;
}

void VdecAsyncNeedInputData(AVCodec *codec, uint32_t index, AVMemory *data, void *userData)
{
    VIDCodecSignal* signal_ = static_cast<VIDCodecSignal *>(userData);
    unique_lock<mutex> lock(signal_->vdecInpMutex_);
    if (signal_->isVdecFlushing_.load()) {
        cout << "isVdecFlushing_ return, VDec InputAvailable, index = " << index << endl;
        return;
    }
    cout << "VDec InputAvailable, index = " << index << endl;

    signal_->vdecInpIndexQueue_.push(index);
    signal_->vdecInpBufferQueue_.push(data);
    signal_->vdecInCond_.notify_all();
}

void VdecAsyncNewOutputData(AVCodec *codec, uint32_t index, AVMemory *data, AVCodecBufferAttr *attr, void *userData)
{
    VIDCodecSignal* signal_ = static_cast<VIDCodecSignal *>(userData);
    unique_lock<mutex> lock(signal_->vdecOutMutex_);
    if (signal_->isVdecFlushing_.load()) {
        cout << "isVdecFlushing_ return, VDec OutputAvailable, index = " << index  << endl;
        return;
    }
    cout << "VDec OutputAvailable, index = " << index  << endl;

    signal_->vdecOutIndexQueue_.push(index);
    // signal_->vdecOutBufferQueue_.push(data);
    signal_->vdecOutSizeQueue_.push(attr->size);
    signal_->vdecOutFlagQueue_.push(attr->flags);
    signal_->vdecOutCond_.notify_all();
}

// venc
void VEncAsyncError(AVCodec *codec, int32_t errorCode, void *userData)
{
    cout << "VEnc Error errorCode=" << errorCode << endl;
}

void VEncAsyncStreamChanged(AVCodec *codec, AVFormat *format, void *userData)
{
    cout << "VEnc Format Changed" << endl;
}

void VEncAsyncNeedInputData(AVCodec *codec, uint32_t index, AVMemory *data, void *userData)
{
    (void)codec;
}

void VEncAsyncNewOutputData(AVCodec *codec, uint32_t index, AVMemory *data, AVCodecBufferAttr *attr, void *userData)
{
    VIDCodecSignal* signal_ = static_cast<VIDCodecSignal *>(userData);
    unique_lock<mutex> lock(signal_->vencOutMutex_);
    if (signal_->isVencFlushing_.load()) {
        cout << "isVdecFlushing_ return, VEnc OutputAvailable, index = " << index << endl;
        return;
    }
    cout << "VEnc OutputAvailable, index = " << index << endl;
    signal_->vencOutIndexQueue_.push(index);
    signal_->vencOutSizeQueue_.push(attr->size);
    signal_->vencOutFlagQueue_.push(attr->flags);
    signal_->vencOutBufferQueue_.push(data);
    signal_->vencOutCond_.notify_all();
}

VCodecNdkSample::~VCodecNdkSample()
{
    VDecRelease();
    VEncRelease();
    delete signal_;
    signal_ = nullptr;
}

void VCodecNdkSample::RunVideoCodec(void)
{
    vdec_ = CreateVideoDecoder();
    venc_ = CreateVideoEncoder();
    NATIVE_CHECK_AND_RETURN_LOG(vdec_ != nullptr, "Fatal: CreateVideoDecoder fail");
    NATIVE_CHECK_AND_RETURN_LOG(venc_ != nullptr, "Fatal: CreateVideoEncoder fail");
    cout << "Create Codec success" << endl;
    
    struct AVFormat *decFormat = CreateDecFormat();
    struct AVFormat *encFormat = CreateEncFormat();
    NATIVE_CHECK_AND_RETURN_LOG(decFormat != nullptr, "Fatal: CreateDecFormat fail");
    NATIVE_CHECK_AND_RETURN_LOG(encFormat != nullptr, "Fatal: CreateEncFormat fail");
    cout << "Create AVFormat success" << endl;

    NATIVE_CHECK_AND_RETURN_LOG(VDecConfigure(decFormat) == AV_ERR_OK, "Fatal: VDecConfigure fail");
    NATIVE_CHECK_AND_RETURN_LOG(VEncConfigure(encFormat) == AV_ERR_OK, "Fatal: VEncConfigure fail");
    cout << "Configure success" << endl;


    NATIVE_CHECK_AND_RETURN_LOG(CreateVencSurface() == AV_ERR_OK && surface_ != nullptr, "Fatal: CreateVencSurface fail");
    NATIVE_CHECK_AND_RETURN_LOG(SetVdecSurface() == AV_ERR_OK && surface_ != nullptr, "Fatal: SetVdecSurface fail");
    cout << "Surface set success" << endl;

    NATIVE_CHECK_AND_RETURN_LOG(VDecPrepare() == AV_ERR_OK, "Fatal: VDecPrepare fail");
    NATIVE_CHECK_AND_RETURN_LOG(VEncPrepare() == AV_ERR_OK, "Fatal: VEncPrepare fail");
    cout << "Prepare success" << endl;

    NATIVE_CHECK_AND_RETURN_LOG(VDecStart() == AV_ERR_OK, "Fatal: VDecStart fail");
    NATIVE_CHECK_AND_RETURN_LOG(VEncStart() == AV_ERR_OK, "Fatal: VEncStart fail");
    cout << "Start success" << endl;
    // usleep(80000); // start run 80ms
    while (vdecFrameCount_ < ES_LENGTH / 4) {};
    cout << "Go VDec flush" << endl;
    NATIVE_CHECK_AND_RETURN_LOG(VDecFlush() == AV_ERR_OK, "Fatal: VDecFlush fail");
    cout << "Flush VDec success" << endl << "Restart:" << endl;
    NATIVE_CHECK_AND_RETURN_LOG(VDecStart() == AV_ERR_OK, "Fatal: ReStart vdec fail");
    cout << "restart vdec success" << endl;
    // usleep(300000); // start run 80ms
    // cout << "Go VEnc flush" << endl;
    // NATIVE_CHECK_AND_RETURN_LOG(VEncFlush() == AV_ERR_OK, "Fatal: VEncFlush fail");
    // cout << "Flush VEnc success" << endl << "Restart:" << endl;
    // NATIVE_CHECK_AND_RETURN_LOG(OH_VideoEncoder_Start(venc_) == AV_ERR_OK, "Fatal: ReStart venc fail");


    while (signal_->isVdecEOS_.load() == false || signal_->isVencEOS_.load() == false) {};

    cout << "Running end" << endl;
    NATIVE_CHECK_AND_RETURN_LOG(VDecStop() == AV_ERR_OK, "Fatal: VDecStop fail");
    NATIVE_CHECK_AND_RETURN_LOG(VEncStop() == AV_ERR_OK, "Fatal: VEncStop fail");
    cout << "Stop success" << endl;

    NATIVE_CHECK_AND_RETURN_LOG(VDecRelease() == AV_ERR_OK, "Fatal: VDecRelease fail");
    NATIVE_CHECK_AND_RETURN_LOG(VEncRelease() == AV_ERR_OK, "Fatal: VEncRelease fail");
    cout << "Release success" << endl;
}

struct AVCodec* VCodecNdkSample::CreateVideoDecoder(void)
{
    struct AVCodec *codec = OH_VideoDecoder_CreateByMime("video/avc");
    // struct AVCodec *codec = OH_VideoDecoder_CreateByMime(MIME_TYPE.c_str());
    NATIVE_CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "Fatal: OH_VideoDecoder_CreateByMime");
    if (signal_ == nullptr) {
        signal_ = new VIDCodecSignal();
        NATIVE_CHECK_AND_RETURN_RET_LOG(signal_ != nullptr, nullptr, "Fatal: No Memory");
    }

    vdec_cb_.onError = VdecAsyncError;
    vdec_cb_.onStreamChanged = VdecAsyncStreamChanged;
    vdec_cb_.onNeedInputData = VdecAsyncNeedInputData;
    vdec_cb_.onNeedOutputData = VdecAsyncNewOutputData;
    int32_t ret = OH_VideoDecoder_SetCallback(codec, vdec_cb_, static_cast<void *>(signal_));
    NATIVE_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, NULL, "Fatal: OH_VideoDecoder_SetCallback");
    return codec;
}

struct AVCodec* VCodecNdkSample::CreateVideoEncoder(void)
{
    struct AVCodec *codec = OH_VideoEncoder_CreateByMime(MIME_TYPE.c_str());
    NATIVE_CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "Fatal: OH_VideoEncoder_CreateByMime");

    if (signal_ == nullptr) {
        signal_ = new VIDCodecSignal();
        NATIVE_CHECK_AND_RETURN_RET_LOG(signal_ != nullptr, nullptr, "Fatal: No Memory");
    }
    venc_cb_.onError = VEncAsyncError;
    venc_cb_.onStreamChanged = VEncAsyncStreamChanged;
    venc_cb_.onNeedInputData = VEncAsyncNeedInputData;
    venc_cb_.onNeedOutputData = VEncAsyncNewOutputData;
    int32_t ret = OH_VideoEncoder_SetCallback(codec, venc_cb_, static_cast<void *>(signal_));
    NATIVE_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, NULL, "Fatal: OH_VideoEncoder_SetCallback");
    return codec;
}

struct AVFormat *VCodecNdkSample::CreateEncFormat(void)
{
    struct AVFormat *format = OH_AVFormat_Create();
    string width = "width";
    string height = "height";
    string pixel_format = "pixel_format";
    string frame_rate = "frame_rate";

    if (format != NULL) {
        (void)OH_AVFormat_SetIntValue(format, width.c_str(), DEFAULT_WIDTH);
        (void)OH_AVFormat_SetIntValue(format, height.c_str(), DEFAULT_HEIGHT);
        (void)OH_AVFormat_SetIntValue(format, pixel_format.c_str(), NV21);
        (void)OH_AVFormat_SetIntValue(format, frame_rate.c_str(), DEFAULT_FRAME_RATE);
    }
    return format;
}

struct AVFormat *VCodecNdkSample::CreateDecFormat(void)
{
    struct AVFormat *format = OH_AVFormat_Create();
    string width = "width";
    string height = "height";
    string pixel_format = "pixel_format";
    string frame_rate = "frame_rate";

    if (format != NULL) {
        (void)OH_AVFormat_SetIntValue(format, width.c_str(), DEFAULT_WIDTH);
        (void)OH_AVFormat_SetIntValue(format, height.c_str(), DEFAULT_HEIGHT);
        (void)OH_AVFormat_SetIntValue(format, pixel_format.c_str(), NV21);
        (void)OH_AVFormat_SetIntValue(format, frame_rate.c_str(), DEFAULT_FRAME_RATE);
    }
    return format;
}

struct VEncObject : public AVCodec {
    explicit VEncObject(const std::shared_ptr<AVCodecVideoEncoder> &encoder)
        : AVCodec(AVMagic::MEDIA_MAGIC_VIDEO_ENCODER), videoEncoder_(encoder) {}
    ~VEncObject() = default;

    const std::shared_ptr<AVCodecVideoEncoder> videoEncoder_;
};

int32_t VCodecNdkSample::CreateVencSurface()
{
    struct VEncObject *videoEncObj = reinterpret_cast<VEncObject *>(venc_);
    NATIVE_CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoEncoder is nullptr!");

    surface_ = videoEncObj->videoEncoder_->CreateInputSurface();
    NATIVE_CHECK_AND_RETURN_RET_LOG(surface_ != nullptr, AV_ERR_OPERATE_NOT_PERMIT, "videoEncoder CreateInputSurface failed!");
    return AV_ERR_OK;
    // return OH_VideoDecoder_GetInputSurface(vdec_, window);
}

struct VDecObject : public AVCodec {
    explicit VDecObject(const std::shared_ptr<AVCodecVideoDecoder> &decoder)
        : AVCodec(AVMagic::MEDIA_MAGIC_VIDEO_DECODER), videoDecoder_(decoder) {}
    ~VDecObject() = default;

    const std::shared_ptr<AVCodecVideoDecoder> videoDecoder_;
};

int32_t VCodecNdkSample::SetVdecSurface()
{
    struct VDecObject *videoDecObj = reinterpret_cast<VDecObject *>(vdec_);
    NATIVE_CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoDecoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->SetOutputSurface(surface_);
    NATIVE_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder SetOutputSurface failed!");
    return ret;
}

int32_t VCodecNdkSample::VDecConfigure(struct AVFormat *format)
{
    return OH_VideoDecoder_Configure(vdec_, format);
}

int32_t VCodecNdkSample::VEncConfigure(struct AVFormat *format)
{
    return OH_VideoEncoder_Configure(venc_, format);
}

int32_t VCodecNdkSample::VDecPrepare()
{
    return OH_VideoDecoder_Prepare(vdec_);
}

int32_t VCodecNdkSample::VEncPrepare()
{
    return OH_VideoEncoder_Prepare(venc_);
}

int32_t VCodecNdkSample::VDecStart()
{
    unique_lock<mutex> outLock(signal_->vdecOutMutex_);
    unique_lock<mutex> inLock(signal_->vdecInpMutex_);
    isVdecRunning_.store(true);
    signal_->isVdecEOS_.store(false);
    signal_->isVdecStop_.store(false);
    outLock.unlock();
    inLock.unlock();

    if (testFile_ == nullptr) {
        testFile_ = std::make_unique<std::ifstream>();
        NATIVE_CHECK_AND_RETURN_RET_LOG(testFile_ != nullptr, AV_ERR_UNKNOWN, "Fatal: No memory");
        testFile_->open(INP_DIR, std::ios::in | std::ios::binary);
    }
    if (readLoop_ == nullptr) {
        readLoop_ = make_unique<thread>(&VCodecNdkSample::VDecInpLoopFunc, this);
        NATIVE_CHECK_AND_RETURN_RET_LOG(readLoop_ != nullptr, AV_ERR_UNKNOWN, "Fatal: No memory");
    }

    if (vdecOutLoop_ == nullptr) {
        vdecOutLoop_ = make_unique<thread>(&VCodecNdkSample::VDecOutLoopFunc, this);
        NATIVE_CHECK_AND_RETURN_RET_LOG(vdecOutLoop_ != nullptr, AV_ERR_UNKNOWN, "Fatal: No memory");
    }

    return OH_VideoDecoder_Start(vdec_);
}

int32_t VCodecNdkSample::VEncStart()
{
    unique_lock<mutex> outLock(signal_->vencOutMutex_);
    isVencRunning_.store(true);
    signal_->isVencEOS_.store(false);
    outLock.unlock();

    if (vencOutLoop_ == nullptr) {
        vencOutLoop_ = make_unique<thread>(&VCodecNdkSample::VEncOutLoopFunc, this);
        NATIVE_CHECK_AND_RETURN_RET_LOG(vencOutLoop_ != nullptr, AV_ERR_UNKNOWN, "Fatal: No memory");
    }

    return OH_VideoEncoder_Start(venc_);
}

int32_t VCodecNdkSample::VDecStop()
{
    unique_lock<mutex> outLock0(signal_->vdecOutMutex_);
    unique_lock<mutex> inLock0(signal_->vdecInpMutex_);
    signal_->isVdecStop_.store(true);
    outLock0.unlock();
    inLock0.unlock();
    cout << "vdec stopping .."<< endl;
    int32_t ret = OH_VideoDecoder_Stop(vdec_);

    unique_lock<mutex> outLock(signal_->vdecOutMutex_);
    unique_lock<mutex> inLock(signal_->vdecInpMutex_);

    ClearIntQueue(signal_->vdecOutIndexQueue_);
    ClearIntQueue(signal_->vdecOutSizeQueue_);
    // ClearBufferQueue(signal_->vdecOutBufferQueue_);
    ClearIntQueue(signal_->vdecInpIndexQueue_);
    ClearBufferQueue(signal_->vdecInpBufferQueue_);

    return ret;
}

int32_t VCodecNdkSample::VEncStop()
{
    unique_lock<mutex> outLock0(signal_->vencOutMutex_);
    signal_->isVdecStop_.store(true);
    outLock0.unlock();
    cout << "venc stopping .."<< endl;
    int32_t ret = OH_VideoEncoder_Stop(venc_);

    unique_lock<mutex> outLock(signal_->vencOutMutex_);

    ClearIntQueue(signal_->vencOutIndexQueue_);
    ClearIntQueue(signal_->vencOutSizeQueue_);
    signal_->isVencStop_.store(false);

    return ret;
}

void VCodecNdkSample::ClearIntQueue(std::queue<uint32_t> &q)
{
    std::queue<uint32_t> empty;
    swap(empty, q);
}

void VCodecNdkSample::ClearBufferQueue(std::queue<AVMemory *> &q)
{    
    std::queue<AVMemory *> empty;
    swap(empty, q);
}

int32_t VCodecNdkSample::VDecFlush()
{
    unique_lock<mutex> outLock0(signal_->vdecOutMutex_);
    unique_lock<mutex> inLock0(signal_->vdecInpMutex_);
    signal_->isVdecFlushing_.store(true);
    outLock0.unlock();
    inLock0.unlock();
    cout << "vdec flushing .."<< endl;
    int32_t ret = OH_VideoDecoder_Flush(vdec_);

    unique_lock<mutex> outLock(signal_->vdecOutMutex_);
    unique_lock<mutex> inLock(signal_->vdecInpMutex_);

    ClearIntQueue(signal_->vdecOutIndexQueue_);
    ClearIntQueue(signal_->vdecOutSizeQueue_);
    // ClearBufferQueue(signal_->vdecOutBufferQueue_);
    ClearIntQueue(signal_->vdecInpIndexQueue_);
    ClearBufferQueue(signal_->vdecInpBufferQueue_);
    signal_->isVdecFlushing_.store(false);

    return ret;
}

int32_t VCodecNdkSample::VEncFlush()
{
    unique_lock<mutex> outLock0(signal_->vencOutMutex_);
    signal_->isVencFlushing_.store(true);
    outLock0.unlock();
    int32_t ret = OH_VideoEncoder_Flush(venc_);
    unique_lock<mutex> outLock(signal_->vencOutMutex_);
    // ClearIntQueue(signal_->vencInpIndexQueue_);
    ClearIntQueue(signal_->vencOutIndexQueue_);
    ClearIntQueue(signal_->vencOutSizeQueue_);
    // ClearIntQueue(signal_->vencOutFlagQueue_);
    ClearBufferQueue(signal_->vencOutBufferQueue_);
    signal_->isVencFlushing_.store(false);
    return ret;
}

int32_t VCodecNdkSample::VDecReset()
{
    unique_lock<mutex> outLock0(signal_->vdecOutMutex_);
    unique_lock<mutex> inLock0(signal_->vdecInpMutex_);
    signal_->isVdecStop_.store(true);
    outLock0.unlock();
    inLock0.unlock();
    cout << "vdec reseting .."<< endl;
    int32_t ret = OH_VideoDecoder_Reset(vdec_);

    unique_lock<mutex> outLock(signal_->vdecOutMutex_);
    unique_lock<mutex> inLock(signal_->vdecInpMutex_);

    ClearIntQueue(signal_->vdecOutIndexQueue_);
    ClearIntQueue(signal_->vdecOutSizeQueue_);
    // ClearBufferQueue(signal_->vdecOutBufferQueue_);
    ClearIntQueue(signal_->vdecInpIndexQueue_);
    ClearBufferQueue(signal_->vdecInpBufferQueue_);


    return ret;
}


int32_t VCodecNdkSample::VEncReset()
{
    unique_lock<mutex> outLock0(signal_->vencOutMutex_);
    signal_->isVdecStop_.store(true);
    outLock0.unlock();
    cout << "venc reseting .."<< endl;
    int32_t ret = OH_VideoEncoder_Reset(venc_);

    unique_lock<mutex> outLock(signal_->vencOutMutex_);

    ClearIntQueue(signal_->vencOutIndexQueue_);
    ClearIntQueue(signal_->vencOutSizeQueue_);
    signal_->isVencStop_.store(false);

    return ret;
}

int32_t VCodecNdkSample::VDecRelease()
{
    isVdecRunning_.store(false);
    if (readLoop_ != nullptr && readLoop_->joinable()) {
        unique_lock<mutex> queueLock(signal_->vdecInpMutex_);
        signal_->vdecInpIndexQueue_.push(10000);
        signal_->vdecInCond_.notify_all();
        queueLock.unlock();
        readLoop_->join();
        readLoop_.reset();
    }
    if (vdecOutLoop_ != nullptr && vdecOutLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->vdecOutMutex_);
        signal_->vdecOutIndexQueue_.push(10000);
        signal_->vdecOutCond_.notify_all();
        lock.unlock();
        vdecOutLoop_->join();
        vdecOutLoop_.reset();
    }
    AVErrCode ret = AV_ERR_OK;
    if (vdec_ != nullptr) {
        ret = OH_VideoDecoder_Destroy(vdec_);
        vdec_ = (ret == AV_ERR_OK) ? nullptr : vdec_;
    }
    return ret;
}

int32_t VCodecNdkSample::VEncRelease()
{
    isVencRunning_.store(false);
    if (vencOutLoop_ != nullptr && vencOutLoop_->joinable()) {
        unique_lock<mutex> queueLock(signal_->vencOutMutex_);
        signal_->vencOutIndexQueue_.push(10000);
        signal_->vencOutCond_.notify_all();
        queueLock.unlock();
        vencOutLoop_->join();
        vencOutLoop_.reset();
    }
    AVErrCode ret = AV_ERR_OK;
    if (venc_ != nullptr) {
        ret = OH_VideoEncoder_Destroy(venc_);
        venc_ = (ret == AV_ERR_OK) ? nullptr : venc_;
    }
    AVErrCode ret = AV_ERR_OK;
    if (venc_ != nullptr) {
        ret = OH_VideoEncoder_Destroy(venc_);
        venc_ = (ret == AV_ERR_OK) ? nullptr : venc_;
    }
    return ret;
}

int32_t VCodecNdkSample::VDecSetParameter(AVFormat *format)
{
    return OH_VideoDecoder_SetParameter(vdec_, format);
}

int32_t VCodecNdkSample::VEncSetParameter(AVFormat *format)
{
    return OH_VideoEncoder_SetParameter(venc_, format);
}

void VCodecNdkSample::VDecInpLoopFunc()
{
    while (true) {
        if (!isVdecRunning_.load()) {
            break;
        }

        unique_lock<mutex> lock(signal_->vdecInpMutex_);
        signal_->vdecInCond_.wait(lock, [this]() {
            return signal_->vdecInpIndexQueue_.size() > 0;
        });
        if (!isVdecRunning_.load()) {
            break;
        }
        uint32_t index = signal_->vdecInpIndexQueue_.front();

        if (signal_->isVdecFlushing_.load() || signal_->isVdecStop_.load() || signal_->isVdecEOS_.load()) {
            signal_->vdecInpIndexQueue_.pop();
            signal_->vdecInpBufferQueue_.pop();
            signal_->vdecInpSizeQueue_.pop();
            cout << "isVdecFlushing_ VDecInpLoopFunc pop" << endl;
            continue;
        }
        AVMemory *buffer = reinterpret_cast<AVMemory *>(signal_->vdecInpBufferQueue_.front());
        NATIVE_CHECK_AND_RETURN_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");
        NATIVE_CHECK_AND_RETURN_LOG(testFile_ != nullptr && testFile_->is_open(), "Fatal: open file fail");

        uint32_t bufferSize = 0;

        if (vdecFrameCount_ < ES_LENGTH) {
            bufferSize =  ES[vdecFrameCount_]; // replace with the actual size
            char *fileBuffer = (char *)malloc(sizeof(char) * bufferSize + 1);
            NATIVE_CHECK_AND_RETURN_LOG(fileBuffer != nullptr, "Fatal: malloc fail.");
            (void)testFile_->read(fileBuffer, bufferSize);
            if (testFile_->eof()) {
                cout << "Finish" << endl;
                free(fileBuffer);
                break;
            }

            if (memcpy_s(OH_AVMemory_GetAddr(buffer), OH_AVMemory_GetSize(buffer), fileBuffer, bufferSize) != EOK) {
                cout << "Fatal: memcpy fail" << endl;
                free(fileBuffer);
                break;
            }
            free(fileBuffer);
        }
        struct AVCodecBufferAttr attr;
        attr.offset = 0;
        if (vdecFrameCount_ == ES_LENGTH) {
            attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
            attr.size = 0;
            attr.pts = 0;
            cout << "EOS Frame, frameCount = " << vdecFrameCount_ << endl;
            signal_->isVdecEOS_.store(true);
            if (testFile_ != nullptr && testFile_->is_open()) {
                testFile_->close();
                if (testFile_ != nullptr) {
                    testFile_ = nullptr;
                }
            }
        } else {
            if (isFirstDecFrame_) {
                attr.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
                isFirstDecFrame_ = false;
            } else {
                attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
            }
            attr.size = bufferSize;
            attr.pts = timestamp_;
        }
        if (OH_VideoDecoder_PushInputData(vdec_, index, attr) != AV_ERR_OK) {
            cout << "Fatal: OH_VideoDecoder_PushInputData fail, exit" << endl;
        } 
        // else {
        cout << "OH_VideoDecoder_PushInputData success , vdecFrameCount_ = " << vdecFrameCount_ << ", attr.size = " << attr.size<< endl;
        // }
        
        signal_->vdecInpIndexQueue_.pop();
        // signal_->sizeQueue_.pop();
        signal_->vdecInpBufferQueue_.pop();

        timestamp_ += FRAME_DURATION_US;
        vdecFrameCount_++;

        if (buffer == nullptr) {
            cout << "Fatal: GetOutputBuffer fail, exit" << endl;
            break;
        }

    }
    cout <<"EXIT VDecInpLoopFunc ... " << endl; 
}

void VCodecNdkSample::VDecOutLoopFunc()
{
    while (true) {
        if (!isVdecRunning_.load()) {
            break;
        }
        unique_lock<mutex> lock(signal_->vdecOutMutex_);
        signal_->vdecOutCond_.wait(lock, [this]() {
            return signal_->vdecOutIndexQueue_.size() > 0;
        });
        if (!isVdecRunning_.load()) {
            break;
        }
        uint32_t index = signal_->vdecOutIndexQueue_.front();
        if (signal_->isVdecFlushing_.load() || signal_->isVdecStop_.load()) {
            signal_->vdecOutIndexQueue_.pop();
            signal_->vdecOutSizeQueue_.pop();
            signal_->vdecOutFlagQueue_.pop();

            cout << "isVdecFlushing_ or isVdecStop_VDecOutLoopFunc pop" << endl;
            continue;
        }

        bool flag = signal_->vdecOutFlagQueue_.front();
        if (flag == AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "VDEC out EOS: OH_VideoEncoder_NotifyEndOfStream" << endl;
            if (OH_VideoEncoder_NotifyEndOfStream(venc_) != AV_ERR_OK) {
                cout << "OH_VideoEncoder_NotifyEndOfStream fail" << endl;
            }
            if (OH_VideoDecoder_FreeOutputData(vdec_, index) != AV_ERR_OK) {
                cout << "Fatal: OH_VideoDecoder_FreeOutputData fail, index = " << index << endl;
            }
            signal_->isVdecEOS_.store(false);
        } else {
            // Return the buffer corresponding to the index to the surface
            if (OH_VideoDecoder_RenderOutputData(vdec_, index) != AV_ERR_OK) {
                cout << "Fatal: ReleaseOutputBuffer fail" << endl;
                break;
            }
        }
        signal_->vdecOutIndexQueue_.pop();
        signal_->vdecOutSizeQueue_.pop();
        signal_->vdecOutFlagQueue_.pop();
    }
    cout <<"EXIT VDecOutLoopFunc ... " << endl; 
}

void VCodecNdkSample::VEncOutLoopFunc()
{
    while (true) {
        if (!isVencRunning_.load()) {
            break;
        }

        unique_lock<mutex> lock(signal_->vencOutMutex_);
        signal_->vencOutCond_.wait(lock, [this]() {
            return signal_->vencOutIndexQueue_.size() > 0;
        });

        if (!isVencRunning_.load()) {
            break;
        }

        uint32_t index = signal_->vencOutIndexQueue_.front();
        auto buffer = signal_->vencOutBufferQueue_.front();
        uint32_t size = signal_->vencOutSizeQueue_.front();

        if (signal_->isVencFlushing_.load()) {
            signal_->vencOutIndexQueue_.pop();
            signal_->vencOutBufferQueue_.pop();
            signal_->vencOutSizeQueue_.pop();
            signal_->vencOutFlagQueue_.pop();
            cout << "isVencFlushing_ VEncOutLoopFunc pop" << endl;
            continue;
        }
        cout << "GetOutputBuffer size : " << size << " vencFrameCount_ =  " << vencFrameCount_ << endl;
        bool flag = signal_->vencOutFlagQueue_.front();
        if (flag == AVCODEC_BUFFER_FLAGS_EOS || buffer == nullptr) {
            signal_->isVencEOS_.store(false);
            cout << "VENC EOS: size :" << size << " vencFrameCount_ =  " << vencFrameCount_ << endl;
            break;
        }
        vencFrameCount_ ++;
        if (NEED_DUMP) {
            FILE *outFile;
            outFile = fopen(VENC_OUT_DIR, "a");
            if (outFile == nullptr) {
                cout << "dump data fail" << endl;
            } else {
                fwrite(OH_AVMemory_GetAddr(buffer), 1, size, outFile);
            }
            fclose(outFile);
        }

        if (OH_VideoEncoder_FreeOutputData(venc_, index) != AV_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail, exit" << endl;
            break;
        }

        signal_->vencOutIndexQueue_.pop();
        signal_->vencOutSizeQueue_.pop();
        signal_->vencOutFlagQueue_.pop();
        signal_->vencOutBufferQueue_.pop();
    }
    cout <<"EXIT VEncOutLoopFunc ... " << endl; 
}