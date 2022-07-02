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

#ifndef NDK_AV_CODEC_H
#define NDK_AV_CODEC_H

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

/**
 * @brief AVCodec MIME types.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
const char * const AVCODEC_MIME_TYPE_VIDEO_AVC = "video/avc";
const char * const AVCODEC_MIME_TYPE_AUDIO_AAC = "audio/mp4a-latm";


/**
 * @brief Enumerates AVCodec Buffer Flags.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
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

/**
 * @brief Enumerates AVCodec Buffer Flags.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
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
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @param codec AVCodec句柄指针
 * @param errorCode Error code.
 * @param userData specified data
 * @since 9
 * @version 1.0
 */
typedef void (* AVCodecOnAsyncError)(AVCodec *codec, int32_t errorCode, void *userData);

/**
 * @brief Called when the output format has changed. format生命周期仅维持在StreamChanged事件中，禁止结束后访问
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @param codec AVCodec句柄指针
 * @param format AVFormat句柄指针
 * @param userData specified data
 * @since 9
 * @version 1.0
 */
typedef void (* AVCodecOnAsyncStreamChanged)(AVCodec *codec, AVFormat *format, void *userData);

/**
 * @brief Called when an input buffer becomes available.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @param codec AVCodec句柄指针
 * @param index The index of the available input buffer.
 * @param userData specified data
 * @since 9
 * @version 1.0
 */
typedef void (* AVCodecOnAsyncNeedInputData)(AVCodec *codec, uint32_t index, AVMemory *data, void *userData);

/**
 * @brief Called when an out buffer becomes available. attr生命周期仅维持在NewOutputData事件中，禁止结束后访问
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @param codec AVCodec句柄指针
 * @param index The index of the available output buffer.
 * @param attr attr of the available output buffer. For details, see {@link AVCodecBufferAttr}
 * @param userData specified data
 * @since 9
 * @version 1.0
 */
typedef void (* AVCodecOnAsyncNewOutputData)(AVCodec *codec, uint32_t index, AVMemory *data,
    AVCodecBufferAttr *attr, void *userData);

/**
 * @brief Register callback function, Respond to codec reporting events
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @param onAsyncError 监听编解码错误码，当回调产生时，编解码出现异常状况
 * @param onAsyncStreamChanged 监听编解码流信息，当回调产生时，流信息变更
 * @param onAsyncNeedInputData 监听编解码需要输入数据，当回调产生时，调用者可通过GetInputData获得InputData
 * @param onAsyncNewOutputData 监听编解码产生输出数据，当回调产生时，调用者方可调用GetOutputData获取OutputData
 * @since 9
 * @version 1.0
 */
typedef struct AVCodecOnAsyncCallback {
    AVCodecOnAsyncError onAsyncError;
    AVCodecOnAsyncStreamChanged onAsyncStreamChanged;
    AVCodecOnAsyncNeedInputData onAsyncNeedInputData;
    AVCodecOnAsyncNewOutputData onAsyncNewOutputData;
} AVCodecOnAsyncCallback;


/**
 * @brief 通过mime创建一个视频解码句柄指针
 * @syscap SystemCapability.Multimedia.Media.VideoDecoder
 * @param mime 编解码器类型 see AVCODEC_MIME_TYPE
 * @return 返回一个AVCodec句柄指针
 * @since 9
 * @version 1.0
 */
AVCodec* OH_AVCODEC_CreateVideoDecoderByMime(const char *mime);


/**
 * @brief 通过name创建一个视频解码句柄指针
 * @syscap SystemCapability.Multimedia.Media.VideoDecoder
 * @param name 查询Xml能力 或者 芯片手册，必须确保解码器name正确
 * @return 返回一个AVCodec句柄指针
 * @since 9
 * @version 1.0
 */
AVCodec* OH_AVCODEC_CreateVideoDecoderByName(const char *name);

/**
 * @brief 设置解码器到Release状态，清空解码器资源，销毁解码器
 * @syscap SystemCapability.Multimedia.Media.VideoDecoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_DestroyVideoDecoder(AVCodec *codec);

/**
 * @brief 设置一个异步回调函数，用以响应编解码产生的事件，监听编解码工作状态，输入/输出Data情况，要求在Configure/Prepare前设置
 * @syscap SystemCapability.Multimedia.Media.VideoDecoder
 * @param codec AVCodec句柄指针
 * @param callback Refer to the definition of AVCodecOnAsyncCallback on how each callback function is called and what are specified.
 * @param userData The specified userdata is the pointer used when those callback functions are called.
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoDecoderSetCallback(AVCodec *codec, AVCodecOnAsyncCallback callback, void *userData);

/**
 * @brief 设置输出Window和Surface，提供解码输出和窗口渲染
 * @syscap SystemCapability.Multimedia.Media.VideoDecoder
 * @param codec AVCodec句柄指针
 * @param window NativeWindow句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoDecoderSetOutputSurface(AVCodec *codec, NativeWindow *window);

/**
 * @brief 设置解码器参数
 * @syscap SystemCapability.Multimedia.Media.VideoDecoder
 * @param codec AVCodec句柄指针
 * @param format AVFormat句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoDecoderConfigure(AVCodec *codec, AVFormat *format);

/**
 * @brief 设置解码器到Prepared状态
 * @syscap SystemCapability.Multimedia.Media.VideoDecoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoDecoderPrepare(AVCodec *codec);

/**
 * @brief 设置解码器到Started状态，等待监听事件，通过QueueInputBuffer送入SPS/PTS/ES数据
 * @syscap SystemCapability.Multimedia.Media.VideoDecoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoDecoderStart(AVCodec *codec);

/**
 * @brief 设置解码器到Stoped状态，停止解码，可通过Start重新进入Started状态，但需重新输入SPS/PTS数据
 * @syscap SystemCapability.Multimedia.Media.VideoDecoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoDecoderStop(AVCodec *codec);

/**
 * @brief 清空解码器队列缓存中的任务
 * @syscap SystemCapability.Multimedia.Media.VideoDecoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoDecoderFlush(AVCodec *codec);

/**
 * @brief 设置解码器到Reset状态，清空解码器的工作参数，如需再次使用，需通过Configure和Prepare重新进入Prepared状态
 * @syscap SystemCapability.Multimedia.Media.VideoDecoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoDecoderReset(AVCodec *codec);

/**
 * @brief 获取输出AVFormat格式，可以通过AVFormat的KEY获取PixelFormat、Width/Hight等参数
 * @syscap SystemCapability.Multimedia.Media.VideoDecoder
 * @param codec AVCodec句柄指针
 * @return 返回AVFormat句柄指针，生命周期随下一次GetOutputMediaDescription刷新，或伴随AVCodec销毁;
 * @since 9
 * @version 1.0
 */
AVFormat* OH_AVCODEC_VideoDecoderGetOutputMediaDescription(AVCodec *codec);

/**
 * @brief 向解码器设置动态参数，只能在Started状态下调用，注意：错误的参数变更，可能会导致解码失败
 * @syscap SystemCapability.Multimedia.Media.VideoDecoder
 * @param codec AVCodec句柄指针
 * @param format AVFormat句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoDecoderSetParameter(AVCodec *codec, AVFormat *format);

/**
 * @brief 当GetInputBuffer返回的AVMemory被写入SPS/PTS/ES数据后，可通过QueueInputBuffer通知解码器读取数据
 * @syscap SystemCapability.Multimedia.Media.VideoDecoder
 * @param codec AVCodec句柄指针
 * @param index AVCodecOnAsyncNeedInputData事件提供的索引值
 * @param attr 描述InputBuffer的属性信息
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoDecoderPushInputData(AVCodec *codec, uint32_t index, AVCodecBufferAttr attr);

/**
 * @brief 当监听到AVCodecOnAsyncNewOutputData事件时，可通过事件提供的索引值，将OutputBuffer送给surface
 * @syscap SystemCapability.Multimedia.Media.VideoDecoder
 * @param codec AVCodec句柄指针
 * @param index OutputBuffer索引值
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoDecoderRenderOutputData(AVCodec *codec, uint32_t index);

/**
 * @brief 当监听到AVCodecOnAsyncNewOutputData事件时，可通过事件提供的索引值，释放OutputBuffer
 * @syscap SystemCapability.Multimedia.Media.VideoDecoder
 * @param codec AVCodec句柄指针
 * @param index OutputBuffer索引值
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoDecoderFreeOutputData(AVCodec *codec, uint32_t index);

// Video Encoder
/**
 * @brief 通过mime创建一个视频编码句柄指针
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param mime 编解码器类型 see AVCODEC_MIME_TYPE
 * @return 返回一个AVCodec句柄指针
 * @since 9
 * @version 1.0
 */
AVCodec* OH_AVCODEC_CreateVideoEncoderByMime(const char *mime);

/**
 * @brief 通过name创建一个视频编码句柄指针
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param name 查询Xml能力 或者 芯片手册，必须确保解码器name正确
 * @return 返回一个AVCodec句柄指针
 * @since 9
 * @version 1.0
 */
AVCodec* OH_AVCODEC_CreateVideoEncoderByName(const char *name);

/**
 * @brief 设置编码器到Release状态，清空编码器资源，销毁编码器
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_DestroyVideoEncoder(AVCodec *codec);

/**
 * @brief 设置一个异步回调函数，用以响应编解码产生的事件，监听编解码工作状态，输入/输出Data情况，要求在Configure/Prepare前设置
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec AVCodec句柄指针
 * @param callback Refer to the definition of AVCodecOnAsyncCallback on how each callback function is called and what are specified.
 * @param userData The specified userdata is the pointer used when those callback functions are called.
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoEncoderSetCallback(AVCodec *codec, AVCodecOnAsyncCallback callback, void *userData);

/**
 * @brief 设置编码器参数
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec AVCodec句柄指针
 * @param format AVFormat句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoEncoderConfigure(AVCodec *codec, AVFormat *format);

/**
 * @brief 设置编码器到Prepared状态
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoEncoderPrepare(AVCodec *codec);

/**
 * @brief 设置编码器到Started状态，等待监听事件，通过QueueInputBuffer送入SPS/PTS/ES数据
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoEncoderStart(AVCodec *codec);

/**
 * @brief 设置编码器到Stoped状态，停止编码，可通过Start重新进入Started状态，但需重新输入SPS/PTS数据
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoEncoderStop(AVCodec *codec);

/**
 * @brief 清空编码器队列缓存中的任务
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoEncoderFlush(AVCodec *codec);

/**
 * @brief 设置编码器到Reset状态，清空编码器的工作参数，如需再次使用，需通过Configure和Prepare重新进入Prepared状态
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoEncoderReset(AVCodec *codec);

/**
 * @brief 向编码器设置动态参数，只能在Started状态下调用，注意：错误的参数变更，可能会导致编码失败
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec AVCodec句柄指针
 * @param format AVFormat句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoEncoderSetParameter(AVCodec *codec, AVFormat *format);

/**
 * @brief 获取输出AVFormat格式，可以通过AVFormat的KEY获取PixelFormat、Width/Hight等参数
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec AVCodec句柄指针
 * @return 返回AVFormat句柄指针，生命周期随下一次GetOutputMediaDescription刷新，或伴随AVCodec销毁;
 * @since 9
 * @version 1.0
 */
AVFormat* OH_AVCODEC_VideoEncoderGetOutputMediaDescription(AVCodec *codec);

/**
 * @brief Obtains the surface from encoder.
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec AVCodec句柄指针
 * @param window NativeWindow句柄指针的指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoEncoderGetInputSurface(AVCodec *codec, NativeWindow **window);

/**
 * @brief 当监听到AVCodecOnAsyncNewOutputData事件时，可通过事件提供的索引值，释放OutputBuffer
 * @syscap SystemCapability.Multimedia.Media.VideoEncoder
 * @param codec AVCodec句柄指针
 * @param index OutputBuffer索引值
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_VideoEncoderFreeOutputData(AVCodec *codec, uint32_t index);

// Audio Decoder
/**
 * @brief 通过mime创建一个音频解码码句柄指针
 * @syscap SystemCapability.Multimedia.Media.AudioDecoder
 * @param mime 编解码器类型 see AVCODEC_MIME_TYPE
 * @return 返回一个AVCodec句柄指针
 * @since 9
 * @version 1.0
 */
AVCodec* OH_AVCODEC_CreateAudioDecoderByMime(const char *mime);

/**
 * @brief 通过name创建一个音频解码句柄指针
 * @syscap SystemCapability.Multimedia.Media.AudioDecoder
 * @param name 查询Xml能力 或者 芯片手册，必须确保解码器name正确
 * @return 返回一个AVCodec句柄指针
 * @since 9
 * @version 1.0
 */
AVCodec* OH_AVCODEC_CreateAudioDecoderByName(const char *name);

/**
 * @brief 设置解码器到Release状态，清空解码器资源，销毁解码器
 * @syscap SystemCapability.Multimedia.Media.AudioDecoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_DestroyAudioDecoder(AVCodec *codec);

/**
 * @brief 设置一个异步回调函数，用以响应编解码产生的事件，监听编解码工作状态，输入/输出Data情况，要求在Configure/Prepare前设置
 * @syscap SystemCapability.Multimedia.Media.AudioDecoder
 * @param codec AVCodec句柄指针
 * @param callback Refer to the definition of AVCodecOnAsyncCallback on how each callback function is called and what are specified.
 * @param userData The specified userdata is the pointer used when those callback functions are called.
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_AudioDecoderSetCallback(AVCodec *codec, AVCodecOnAsyncCallback callback, void *userData);

/**
 * @brief 设置解码器参数
 * @syscap SystemCapability.Multimedia.Media.AudioDecoder
 * @param codec AVCodec句柄指针
 * @param format AVFormat句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_AudioDecoderConfigure(AVCodec *codec, AVFormat *format);

/**
 * @brief 设置解码器到Prepared状态
 * @syscap SystemCapability.Multimedia.Media.AudioDecoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_AudioDecoderPrepare(AVCodec *codec);

/**
 * @brief 设置解码器到Started状态，等待监听事件，通过QueueInputBuffer送入ES数据
 * @syscap SystemCapability.Multimedia.Media.AudioDecoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_AudioDecoderStart(AVCodec *codec);

/**
 * @brief 设置解码器到Stoped状态，停止解码，可通过Start重新进入Started状态，但需重新输入ES数据
 * @syscap SystemCapability.Multimedia.Media.AudioDecoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_AudioDecoderStop(AVCodec *codec);

/**
 * @brief 清空解码器队列缓存中的任务
 * @syscap SystemCapability.Multimedia.Media.AudioDecoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_AudioDecoderFlush(AVCodec *codec);

/**
 * @brief 设置解码器到Reset状态，清空解码器的工作参数，如需再次使用，需通过Configure和Prepare重新进入Prepared状态
 * @syscap SystemCapability.Multimedia.Media.AudioDecoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */

AVErrCode OH_AVCODEC_AudioDecoderReset(AVCodec *codec);

/**
 * @brief 向解码器设置动态参数，只能在Started状态下调用，注意：错误的参数变更，可能会导致解码失败
 * @syscap SystemCapability.Multimedia.Media.AudioDecoder
 * @param codec AVCodec句柄指针
 * @param format AVFormat句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_AudioDecoderSetParameter(AVCodec *codec, AVFormat *format);

/**
 * @brief 获取输出AVFormat格式，可以通过AVFormat的KEY获取SampleFormat、channel、SampleRate等参数
 * @syscap SystemCapability.Multimedia.Media.AudioDecoder
 * @param codec AVCodec句柄指针
 * @return 返回AVFormat句柄指针，生命周期随下一次GetOutputMediaDescription刷新，或伴随AVCodec销毁;
 * @since 9
 * @version 1.0
 */
AVFormat* OH_AVCODEC_AudioDecoderGetOutputMediaDescription(AVCodec *codec);

/**
 * @brief 当GetInputBuffer返回的AVMemory被写入ES数据后，可通过QueueInputBuffer通知解码器读取数据
 * @syscap SystemCapability.Multimedia.Media.AudioDecoder
 * @param codec AVCodec句柄指针
 * @param index AVCodecOnAsyncNeedInputData事件提供的索引值
 * @param attr 描述InputBuffer的属性信息
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_AudioDecoderPushInputData(AVCodec *codec, uint32_t index, AVCodecBufferAttr attr);

/**
 * @brief 当监听到AVCodecOnAsyncNewOutputData事件时，可通过事件提供的索引值，释放OutputBuffer
 * @syscap SystemCapability.Multimedia.Media.AudioDecoder
 * @param codec AVCodec句柄指针
 * @param index OutputBuffer索引值
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_AudioDecoderFreeOutputData(AVCodec *codec, uint32_t index);

// Audio Encoder
/**
 * @brief 通过mime创建一个音频编码句柄指针
 * @syscap SystemCapability.Multimedia.Media.AudioEncoder
 * @param mime 编解码器类型 see AVCODEC_MIME_TYPE
 * @return 返回一个AVCodec句柄指针
 * @since 9
 * @version 1.0
 */
AVCodec* OH_AVCODEC_CreateAudioEncoderByMime(const char *mime);

/**
 * @brief 通过name创建一个音频编码句柄指针
 * @syscap SystemCapability.Multimedia.Media.AudioEncoder
 * @param name 查询Xml能力 或者 芯片手册，必须确保解码器name正确
 * @return 返回一个AVCodec句柄指针
 * @since 9
 * @version 1.0
 */
AVCodec* OH_AVCODEC_CreateAudioEncoderByName(const char *name);

/**
 * @brief 设置编码器到Release状态，清空编码器资源，销毁编码器
 * @syscap SystemCapability.Multimedia.Media.AudioEncoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_DestroyAudioEncoder(AVCodec *codec);

/**
 * @brief 设置一个异步回调函数，用以响应编解码产生的事件，监听编解码工作状态，输入/输出Data情况，要求在Configure/Prepare前设置
 * @syscap SystemCapability.Multimedia.Media.AudioEncoder
 * @param codec AVCodec句柄指针
 * @param callback Refer to the definition of AVCodecOnAsyncCallback on how each callback function is called and what are specified.
 * @param userData The specified userdata is the pointer used when those callback functions are called.
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_AudioEncoderSetCallback(AVCodec *codec, AVCodecOnAsyncCallback callback, void *userData);

/**
 * @brief 设置编码器参数
 * @syscap SystemCapability.Multimedia.Media.AudioEncoder
 * @param codec AVCodec句柄指针
 * @param format AVFormat句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_AudioEncoderConfigure(AVCodec *codec, AVFormat *format);

/**
 * @brief 设置编码器到Prepared状态
 * @syscap SystemCapability.Multimedia.Media.AudioEncoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_AudioEncoderPrepare(AVCodec *codec);

/**
 * @brief 设置编码器到Started状态，等待监听事件，通过QueueInputBuffer送入ES数据
 * @syscap SystemCapability.Multimedia.Media.AudioEncoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_AudioEncoderStart(AVCodec *codec);

/**
 * @brief 设置编码器到Stoped状态，停止编码，可通过Start重新进入Started状态，但需重新输入ES数据
 * @syscap SystemCapability.Multimedia.Media.AudioEncoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_AudioEncoderStop(AVCodec *codec);

/**
 * @brief 清空编码器队列缓存中的任务
 * @syscap SystemCapability.Multimedia.Media.AudioEncoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_AudioEncoderFlush(AVCodec *codec);

/**
 * @brief 设置编码器到Reset状态，清空编码器的工作参数，如需再次使用，需通过Configure和Prepare重新进入Prepared状态
 * @syscap SystemCapability.Multimedia.Media.AudioEncoder
 * @param codec AVCodec句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_AudioEncoderReset(AVCodec *codec);

/**
 * @brief 向编码器设置动态参数，只能在Started状态下调用，注意：错误的参数变更，可能会导致编码失败
 * @syscap SystemCapability.Multimedia.Media.AudioEncoder
 * @param codec AVCodec句柄指针
 * @param format AVFormat句柄指针
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_AudioEncoderSetParameter(AVCodec *codec, AVFormat *format);

/**
 * @brief 获取输出AVFormat格式，可以通过AVFormat的KEY获取SampleFormat、channel、SampleRate等参数
 * @syscap SystemCapability.Multimedia.Media.AudioEncoder
 * @param codec AVCodec句柄指针
 * @return 返回AVFormat句柄指针，生命周期随下一次GetOutputMediaDescription刷新，或伴随AVCodec销毁;
 * @since 9
 * @version 1.0
 */
AVFormat* OH_AVCODEC_AudioEncoderGetOutputMediaDescription(AVCodec *codec);

/**
 * @brief 当GetInputBuffer返回的AVMemory被写入ES数据后，可通过QueueInputBuffer通知解码器读取数据
 * @syscap SystemCapability.Multimedia.Media.AudioEncoder
 * @param codec AVCodec句柄指针
 * @param index AVCodecOnAsyncNeedInputData事件提供的索引值
 * @param attr 描述InputBuffer的属性信息
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_AudioEncoderPushInputData(AVCodec *codec, uint32_t index, AVCodecBufferAttr attr);

/**
 * @brief 当监听到AVCodecOnAsyncNewOutputData事件时，可通过事件提供的索引值，释放OutputBuffer
 * @syscap SystemCapability.Multimedia.Media.AudioEncoder
 * @param codec AVCodec句柄指针
 * @param index OutputBuffer索引值
 * @return AVErrCode 错误码
 * @since 9
 * @version 1.0
 */
AVErrCode OH_AVCODEC_AudioEncoderFreeOutputData(AVCodec *codec, uint32_t index);

#ifdef __cplusplus
}
#endif

#endif // NDK_AV_CODEC_H