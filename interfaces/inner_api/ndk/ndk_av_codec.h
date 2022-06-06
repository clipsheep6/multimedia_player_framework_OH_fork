/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef NDK_VIDEO_CODEC_H
#define NDK_VIDEO_CODEC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "ndk_av_errors.h"
#include "ndk_av_format.h"
#include "ndk_av_memory.h"

typedef struct NativeWindow NativeWindow;
typedef struct AVCodec AVCodec;

typedef enum AVCodecBufferFlags {
    AVCODEC_BUFFER_FLAGS_NONE = 0,
    /* This signals the end of stream */
    AVCODEC_BUFFER_FLAGS_EOS = 1 << 0,
    /* This indicates that the buffer contains the data for a sync frame */
    AVCODEC_BUFFER_FLAGS_SYNC_FRAME = 1 << 1,
    /* This indicates that the buffer only contains part of a frame */
    AVCODEC_BUFFER_FLAGS_PARTIAL_FRAME = 1 << 2,
    /* This indicated that the buffer contains codec specific data */
    AVCODEC_BUFFER_FLAGS_CODEC_DATA = 1 << 3,
} AVCodecBufferFlags;

typedef struct AVCodecBufferAttr {
    /* The presentation timestamp in microseconds for the buffer */
    int64_t presentationTimeUs;
    /* The amount of data (in bytes) in the buffer */
    int32_t size;
    /* The start-offset of the data in the buffer */
    int32_t offset;
    /**
     * @brief the flags associated with the sample, this
     * maybe be a combination of multiple {@link AVCodecBufferFlags}.
     */
    uint32_t flags;
} AVCodecBufferAttr;

/**
 * @brief Called when an error occurred.
 * @syscap SystemCapability.Multimedia.Media.Codec
 * @param errorCode Error code.
 * @param userData specified data
 * @since 3.2
 * @version 3.2
 */
typedef void (* AVCodecOnAsyncError)(struct AVCodec *codec, int32_t errorCode, void *userData);

/**
 * @brief Called when the output format has changed.
 * @syscap SystemCapability.Multimedia.Media.Codec
 * @param format The new output format.
 * @param userData specified data
 * @since 3.2
 * @version 3.2
 */
typedef void (* AVCodecOnAsyncFormatChanged)(struct AVCodec *codec, struct AVFormat *format, void *userData);

/**
 * @brief Called when an input buffer becomes available.
 * @syscap SystemCapability.Multimedia.Media.Codec
 * @param index The index of the available input buffer.
 * @param userData specified data
 * @since 3.2
 * @version 3.2
 */
typedef void (* AVCodecOnAsyncInputAvailable)(struct AVCodec *codec, uint32_t index, void *userData);

/**
 * @brief Called when an out buffer becomes available.
 * @syscap SystemCapability.Multimedia.Media.Codec
 * @param index The index of the available output buffer.
 * @param attr attr of the available output buffer. For details, see {@link AVCodecBufferAttr}
 * @param userData specified data
 * @since 3.2
 * @version 3.2
 */
typedef void (* AVCodecOnAsyncOutputAvailable)(struct AVCodec *codec, uint32_t index, struct AVCodecBufferAttr *attr, void *userData);

/**
 * @brief Register callback function, Respond to codec reporting events
 * @syscap SystemCapability.Multimedia.Media.Codec
 * @param onAsyncError AsyncCallback Function when an error occurred.
 * @param onAsyncFormatChanged AsyncCallback Function when the output format has changed.
 * @param onAsyncInputAvailable AsyncCallback Function when an input buffer becomes available.
 * @param onAsyncOutputAvailable AsyncCallback Function when an out buffer becomes available.
 * @since  3.2
 * @version 3.2
 */
typedef struct AVCodecOnAsyncCallback {
    AVCodecOnAsyncError onAsyncError;
    AVCodecOnAsyncFormatChanged onAsyncFormatChanged;
    AVCodecOnAsyncInputAvailable onAsyncInputAvailable;
    AVCodecOnAsyncOutputAvailable onAsyncOutputAvailable;
} AVCodecOnAsyncCallback;

// Video Decoder
AVCodec* OH_AVCODEC_CreateVideoDecoderByMime(const char *mime);
AVCodec* OH_AVCODEC_CreateVideoDecoderByName(const char *name);
void OH_AVCODEC_DestroyVideoDecoder(AVCodec *codec);
/**
 * @brief Set an asynchronous callback for actionable AVCodec events.
 * @syscap SystemCapability.Multimedia.Media.Codec
 * @param codec Encapsulate AVCodecVideoDecoder structure instance pointer
 * @param callback Refer to the definition of AVCodecOnAsyncCallback on how each callback function is called and what are specified.
 * @param userData The specified userdata is the pointer used when those callback functions are called.
 * @since  3.2
 * @version 3.2
 */
AVErrCode OH_AVCODEC_VideoDecoderSetCallback(AVCodec *codec, AVCodecOnAsyncCallback *callback, void *userData);
AVErrCode OH_AVCODEC_VideoDecoderConfigure(AVCodec *codec, AVFormat *format);
AVErrCode OH_AVCODEC_VideoDecoderPrepare(AVCodec *codec);
AVErrCode OH_AVCODEC_VideoDecoderStart(AVCodec *codec);
AVErrCode OH_AVCODEC_VideoDecoderStop(AVCodec *codec);
AVErrCode OH_AVCODEC_VideoDecoderFlush(AVCodec *codec);
AVErrCode OH_AVCODEC_VideoDecoderReset(AVCodec *codec);
AVErrCode OH_AVCODEC_VideoDecoderRelease(AVCodec *codec);
AVErrCode OH_AVCODEC_VideoDecoderSetOutputSurface(AVCodec *codec, NativeWindow *window);
AVMemory* OH_AVCODEC_VideoDecoderGetInputBuffer(AVCodec *codec, uint32_t index);
AVErrCode OH_AVCODEC_VideoDecoderQueueInputBuffer(AVCodec *codec, uint32_t index, AVCodecBufferAttr attr);
AVErrCode OH_AVCODEC_VideoDecoderGetOutputFormat(AVCodec *codec, AVFormat *format);
// VideoCapsObject* OH_AVCODEC_VideoDecoderGetVideoDecoderCaps(struct AVCodec *codec);
AVErrCode OH_AVCODEC_VideoDecoderReleaseOutputBuffer(AVCodec *codec, uint32_t index, bool render);
AVErrCode OH_AVCODEC_VideoDecoderSetParameter(AVCodec *codec, AVFormat *format);

// Video Encoder
AVCodec* OH_AVCODEC_CreateVideoEncoderByMime(const char *mime);
AVCodec* OH_AVCODEC_CreateVideoEncoderByName(const char *name);
void OH_AVCODEC_DestroyVideoEnoder(AVCodec *codec);
AVErrCode OH_AVCODEC_VideoEncoderSetCallback(AVCodec *codec, AVCodecOnAsyncCallback *callback, void *userData);
AVErrCode OH_AVCODEC_VideoEncoderConfigure(AVCodec *codec, AVFormat *format);
AVErrCode OH_AVCODEC_VideoEncoderPrepare(AVCodec *codec);
AVErrCode OH_AVCODEC_VideoEncoderStart(AVCodec *codec);
AVErrCode OH_AVCODEC_VideoEncoderStop(AVCodec *codec);
AVErrCode OH_AVCODEC_VideoEncoderFlush(AVCodec *codec);
AVErrCode OH_AVCODEC_VideoEncoderReset(AVCodec *codec);
AVErrCode OH_AVCODEC_VideoEncoderRelease(AVCodec *codec);
AVErrCode OH_AVCODEC_VideoEncoderCreateInputSurface(AVCodec *codec, NativeWindow **window);
AVErrCode OH_AVCODEC_VideoEncoderQueueInputBuffer(AVCodec *codec, uint32_t index, AVCodecBufferAttr attr);
AVMemory* OH_AVCODEC_VideoEncoderGetOutputBuffer(AVCodec *codec, uint32_t index);
AVErrCode OH_AVCODEC_VideoEncoderReleaseOutputBuffer(AVCodec *codec, uint32_t index, bool render);
AVErrCode OH_AVCODEC_VideoEncoderSetParameter(AVCodec *codec, AVFormat *format);
AVErrCode OH_AVCODEC_VideoEncoderGetOutputFormat(AVCodec *codec, AVFormat *format);

// Audio Decoder
AVCodec* OH_AVCODEC_CreateAudioDecoderByMime(const char *mime);
AVCodec* OH_AVCODEC_CreateAudioDecoderByName(const char *name);
void OH_AVCODEC_DestroyAudioDecoder(AVCodec *codec);
AVErrCode OH_AVCODEC_AudioDecoderSetCallback(AVCodec *codec, AVCodecOnAsyncCallback *callback, void *userData);
AVErrCode OH_AVCODEC_AudioDecoderConfigure(AVCodec *codec, AVFormat *format);
AVErrCode OH_AVCODEC_AudioDecoderPrepare(AVCodec *codec);
AVErrCode OH_AVCODEC_AudioDecoderStart(AVCodec *codec);
AVErrCode OH_AVCODEC_AudioDecoderStop(AVCodec *codec);
AVErrCode OH_AVCODEC_AudioDecoderFlush(AVCodec *codec);
AVErrCode OH_AVCODEC_AudioDecoderReset(AVCodec *codec);
AVErrCode OH_AVCODEC_AudioDecoderRelease(AVCodec *codec);
AVMemory* OH_AVCODEC_AudioDecoderGetInputBuffer(AVCodec *codec, uint32_t index);
AVMemory* OH_AVCODEC_AudioDecoderGetOutputBuffer(AVCodec *codec, uint32_t index);
AVErrCode OH_AVCODEC_AudioDecoderQueueInputBuffer(AVCodec *codec, uint32_t index, AVCodecBufferAttr attr);
AVErrCode OH_AVCODEC_AudioDecoderReleaseOutputBuffer(AVCodec *codec, uint32_t index);
AVErrCode OH_AVCODEC_AudioDecoderSetParameter(AVCodec *codec, AVFormat *format);
AVErrCode OH_AVCODEC_AudioDecoderGetOutputFormat(AVCodec *codec, AVFormat *format);

// Audio Encoder
AVCodec* OH_AVCODEC_CreateAudioEncoderByMime(const char *mime);
AVCodec* OH_AVCODEC_CreateAudioEncoderByName(const char *name);
void OH_AVCODEC_DestroyAudioEncoder(AVCodec *codec);
AVErrCode OH_AVCODEC_AudioEncoderSetCallback(AVCodec *codec, AVCodecOnAsyncCallback *callback, void *userData);
AVErrCode OH_AVCODEC_AudioEncoderConfigure(AVCodec *codec, AVFormat *format);
AVErrCode OH_AVCODEC_AudioEncoderPrepare(AVCodec *codec);
AVErrCode OH_AVCODEC_AudioEncoderStart(AVCodec *codec);
AVErrCode OH_AVCODEC_AudioEncoderStop(AVCodec *codec);
AVErrCode OH_AVCODEC_AudioEncoderFlush(AVCodec *codec);
AVErrCode OH_AVCODEC_AudioEncoderReset(AVCodec *codec);
AVErrCode OH_AVCODEC_AudioEncoderRelease(AVCodec *codec);
AVMemory* OH_AVCODEC_AudioEncoderGetInputBuffer(AVCodec *codec, uint32_t index);
AVMemory* OH_AVCODEC_AudioEncoderGetOutputBuffer(AVCodec *codec, uint32_t index);
AVErrCode OH_AVCODEC_AudioEncoderQueueInputBuffer(AVCodec *codec, uint32_t index, AVCodecBufferAttr attr);
AVErrCode OH_AVCODEC_AudioEncoderReleaseOutputBuffer(AVCodec *codec, uint32_t index);
AVErrCode OH_AVCODEC_AudioEncoderSetParameter(AVCodec *codec, AVFormat *format);
AVErrCode OH_AVCODEC_AudioEncoderGetOutputFormat(AVCodec *codec, AVFormat *format);

#ifdef __cplusplus
}
#endif

#endif // NDK_VIDEO_CODEC_H