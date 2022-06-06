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

#include <stdio.h>
#include <unistd.h>
#include "avcodec_ndk_sample.h"
#include "ndk_sample_log.h"
#include "ndk_av_codec.h"
// #include "external_window.h" // 头文件封装不纯粹，嵌套依赖

const uint32_t DEFAULT_WIDTH = 480;
const uint32_t DEFAULT_HEIGHT = 360;
const uint32_t DEFAULT_FRAME_RATE = 30;
const uint32_t MAX_INPUT_BUFFER_SIZE = 30000;
const uint32_t FRAME_DURATION_US = 33000;
const uint32_t DEFAULT_FRAME_COUNT = 1;
struct AVCodecOnAsyncCallback g_callback;

void AsyncErrorCallBack(struct AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)printf("Error errorCode=%d\n", errorCode);
}

void AsyncFormatChanged(struct AVCodec *codec, struct AVFormat *format, void *userData)
{
    (void)printf("Format Changed\n");
}

void AsyncInputAvailable(struct AVCodec *codec, uint32_t index, void *userData)
{
    (void)printf("InputAvailable\n");
}

void AsyncOutputAvailable(struct AVCodec *codec, uint32_t index, struct AVCodecBufferAttr *attr, void *userData)
{
    (void)printf("OutputAvailable\n");
}

struct AVCodec* CreateVideoDecoder(void)
{
    struct AVCodec *codec = OH_AVCODEC_CreateVideoDecoderByMime("video/avc");
    NDK_CHECK_AND_RETURN_RET_LOG(codec != NULL, NULL, "Fatal: OH_AVCODEC_CreateVideoDecoderByMime");
    
    g_callback.onAsyncError = AsyncErrorCallBack;
    g_callback.onAsyncFormatChanged = AsyncFormatChanged;
    g_callback.onAsyncInputAvailable = AsyncInputAvailable;
    g_callback.onAsyncOutputAvailable = AsyncOutputAvailable;
    int32_t ret = OH_AVCODEC_VideoDecoderSetCallback(codec, &g_callback, NULL);
    NDK_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, NULL, "Fatal: OH_AVCODEC_CreateVideoDecoderByMime");
    return codec;
}

void DestroyVideoDecoder(struct AVCodec* codec)
{
    if (codec != NULL) {
        return OH_AVCODEC_DestroyVideoDecoder(codec);
    }
}

struct AVFormat* CreateFormat(void)
{
    struct AVFormat *format = OH_AV_CreateFormat();
    char *width = "width";
    char *height = "height";
    char *fmt = "pixel_format";
    char *rate = "frame_rate";
    char *max = "max_input_size";
    if (format != NULL) {
        (void)OH_AV_FormatPutIntValue(format, width, DEFAULT_WIDTH);
        (void)OH_AV_FormatPutIntValue(format, height, DEFAULT_HEIGHT);
        (void)OH_AV_FormatPutIntValue(format, fmt, AV_PIXEL_FORMT_NV21);
        (void)OH_AV_FormatPutIntValue(format, rate, DEFAULT_FRAME_RATE);
        (void)OH_AV_FormatPutIntValue(format, max, MAX_INPUT_BUFFER_SIZE);
    }
    return format;
}

void DestroyFormat(struct AVFormat* format)
{
    if (format != NULL) {
        return OH_AV_DestroyFormat(format);
    }
}

struct NativeWindow* CreateNaitveWindow(void)
{
    // 此处缺少创建suface的NDK接口，导致NativeWindow无法创建，缺少窗口参数设置指导
    // return OH_NativeWindow_CreateNativeWindow(NULL);
    return NULL;
}

void DestroyNaitveWindow(struct NativeWindow *window)
{
    if (window != NULL) {
        // return OH_NativeWindow_DestroyNativeWindow(window);
    }
}

int32_t Configure(struct AVCodec *codec, struct AVFormat *format)
{
    return OH_AVCODEC_VideoDecoderConfigure(codec, format);
}

int32_t Prepare(struct AVCodec *codec)
{
    return OH_AVCODEC_VideoDecoderPrepare(codec);
}

int32_t Start(struct AVCodec *codec)
{
    // isRunning_.store(true);

    // testFile_ = std::make_unique<std::ifstream>();
    // DEMO_CHECK_AND_RETURN_RET_LOG(testFile_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");
    // testFile_->open("/data/media/video.es", std::ios::in | std::ios::binary);

    // inputLoop_ = make_unique<thread>(&VDecDemo::InputFunc, this);
    // DEMO_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");

    // outputLoop_ = make_unique<thread>(&VDecDemo::OutputFunc, this);
    // DEMO_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");

    return OH_AVCODEC_VideoDecoderStart(codec);
}

int32_t Stop(struct AVCodec *codec)
{
    // isRunning_.store(false);

    // if (inputLoop_ != nullptr && inputLoop_->joinable()) {
    //     unique_lock<mutex> lock(signal_->inMutex_);
    //     signal_->inQueue_.push(0);
    //     signal_->inCond_.notify_all();
    //     lock.unlock();
    //     inputLoop_->join();
    //     inputLoop_.reset();
    // }

    // if (outputLoop_ != nullptr && outputLoop_->joinable()) {
    //     unique_lock<mutex> lock(signal_->outMutex_);
    //     signal_->outQueue_.push(0);
    //     signal_->outCond_.notify_all();
    //     lock.unlock();
    //     outputLoop_->join();
    //     outputLoop_.reset();
    // }

    return OH_AVCODEC_VideoDecoderStop(codec);
}

int32_t Flush(struct AVCodec *codec)
{
    return OH_AVCODEC_VideoDecoderFlush(codec);
}

int32_t Reset(struct AVCodec *codec)
{
    return OH_AVCODEC_VideoDecoderReset(codec);
}

int32_t Release(struct AVCodec *codec)
{
    return OH_AVCODEC_VideoDecoderRelease(codec);
}

int32_t SetSurface(struct AVCodec *codec, struct NativeWindow *window)
{
    return OH_AVCODEC_VideoDecoderSetOutputSurface(codec, window);
}

void RunVideoDec(void)
{
    (void)printf("video dec sample in \n");
    struct AVCodec *codec = CreateVideoDecoder();
    struct AVFormat *format = CreateFormat();
    struct NativeWindow *window = CreateNaitveWindow();
    if (codec == NULL || format == NULL || window == NULL) {
        (void)printf("failed to CreateVideoDecoder or CreateFormat\n");
        goto ERR_RET;
    }

    (void)Configure(codec, format);
    (void)SetSurface(codec, window);
    (void)Prepare(codec);
    (void)Start(codec);
    sleep(3); // start run 3s
    (void)Stop(codec);
    (void)Release(codec);

ERR_RET:
    DestroyNaitveWindow(window);
    DestroyFormat(format);
    DestroyVideoDecoder(codec);
}