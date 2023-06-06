/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef NATIVE_SCREEN_CAPTURE_BASE_H
#define NATIVE_SCREEN_CAPTURE_BASE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Nativebuffer of screencapture that from graphics.
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 *
 * @since 10
 * @version 1.0
 */
typedef struct OH_NativeBuffer OH_NativeBuffer;

/**
 * @brief Initialization of screenCapture
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 *
 * @since 10
 * @version 1.0
 */
typedef struct OH_ScreenCapture OH_ScreenCapture;

/**
 * @brief Enumerates audio cap source type.
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 *
 * @since 10
 * @version 1.0
 */
typedef enum OH_AudioCapSourceType {
    /* Invalid audio source */
    OH_SOURCE_INVALID = -1,
    /* Default audio source */
    OH_SOURCE_DEFAULT = 0,
    /* Microphone */
    OH_MIC = 1,
    /* PlayBack */
    OH_PLAYBACK = 2,
} OH_AudioCapSourceType;

/**
 * @brief Enumerates audio codec formats.
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 *
 * @since 10
 * @version 1.0
 */
typedef enum OH_AudioCodecFormat {
    /* Default format */
    OH_AUDIO_DEFAULT = 0,
    /* Advanced Audio Coding Low Complexity (AAC-LC) */
    OH_AAC_LC = 3,
    /* Invalid value */
    OH_AUDIO_CODEC_FORMAT_BUTT,
} OH_AudioCodecFormat;

/**
 * @brief Enumerates video codec formats.
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 *
 * @since 10
 * @version 1.0
 */
typedef enum OH_VideoCodecFormat {
    /* Default format */
    OH_VIDEO_DEFAULT = 0,
    /* H.264 */
    OH_H264 = 2,
    /* MPEG4 */
    OH_MPEG4 = 6,
    /* Invalid format */
    OH_VIDEO_CODEC_FORMAT_BUTT,
} OH_VideoCodecFormat;

/**
 * @brief Enumerates screen capture data type.
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 *
 * @since 10
 * @version 1.0
 */
typedef enum OH_DataType {
    /* YUV/RGBA/PCM, etc. original stream */
    OH_ORIGINAL_STREAM = 0,
    /*  */
    /* h264/AAC, etc. encoded stream */
    OH_ENCODED_STREAM = 1,
    /* mp4 file */
    OH_CAPTURE_FILE = 2,
    OH_INVAILD = -1
} OH_DataType;

/**
 * @brief Enumerates video source types.
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 *
 * @since 10
 * @version 1.0
 */
typedef enum OH_VideoSourceType {
    /** Unsupported App Usage. */
    /** YUV video data provided through {@link Surface} */
    OH_VIDEO_SOURCE_SURFACE_YUV = 0,
    /** Raw encoded data provided through {@link Surface} */
    OH_VIDEO_SOURCE_SURFACE_ES,
    /** RGBA video data provided through {@link Surface} */
    OH_VIDEO_SOURCE_SURFACE_RGBA,
    /** Invalid value */
    OH_VIDEO_SOURCE_BUTT
} OH_VideoSourceType;

/**
 * @brief Enumerates the container format types.
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 *
 * @since 10
 * @version 1.0
 */
typedef enum OH_ContainerFormatType {
    /* Audio format type -- m4a */
     CFT_MPEG_4A = 0,
    /* Video format type -- mp4 */
     CFT_MPEG_4 = 1
} OH_ContainerFormatType;

/**
 * @brief Audio capture info struct
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 *
 * @since 10
 * @version 1.0
 */
typedef struct OH_AudioCapInfo {
    /* Audio capture sample rate info */
    int32_t audioSampleRate = 0;
    /* Audio capture channel info */
    int32_t audioChannels = 0;
    /* Audio capture source type */
    OH_AudioCapSourceType audioSource;
} OH_AudioCapInfo;

/**
 * @brief Audio encoder info
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 *
 * @since 10
 * @version 1.0
 */
typedef struct OH_AudioEncInfo {
    /* Audio encoder bitrate */
    int32_t audioBitrate;
    /* Audio codec format */
    OH_AudioCodecFormat audioCodecformat;
} OH_AudioEncInfo;

/**
 * @brief The audio info of screencapture
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 *
 * @since 10
 * @version 1.0
 */
typedef struct OH_AudioInfo {
    /* Audio capture info of microphone */
    OH_AudioCapInfo micCapInfo;
    /* Audio capture info of inner */
    OH_AudioCapInfo innerCapInfo;
    /* Audio encoder info */
    OH_AudioEncInfo audioEncInfo;
} OH_AudioInfo;

/**
 * @brief Video capture info
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 *
 * @since 10
 * @version 1.0
 */
typedef struct OH_VideoCapInfo {
    /* Display id of screencapture */
    uint64_t displayId;
    /* The task ids of display id */
    int32_t *taskIDs;
    /* Task ids length */
    int32_t taskIDsLen;
    /* Video frame width of screencapture */
    int32_t videoFrameWidth;
    /* Video frame height of screencapture */
    int32_t videoFrameHeight;
    /* Video source type of screencapture */
    OH_VideoSourceType videoSource;
} OH_VideoCapInfo;

/**
 * @brief Videoc encoder info
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 *
 * @since 10
 * @version 1.0
 */
typedef struct OH_VideoEncInfo {
    /* Video encoder format */
    OH_VideoCodecFormat videoCodec;
    /* Video encoder bitrate */
    int32_t videoBitrate;
    /* Video encoder frame rate */
    int32_t videoFrameRate;
} OH_VideoEncInfo;

/**
 * @brief Video info
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 *
 * @since 10
 * @version 1.0
 */
typedef struct OH_VideoInfo {
    /* Video capture info */
    OH_VideoCapInfo videoCapInfo;
    /* Video encoder info */
    OH_VideoEncInfo videoEncInfo;
} OH_VideoInfo;

/**
 * @brief Recorder file info
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 *
 * @since 10
 * @version 1.0
 */
typedef struct OH_RecorderInfo {
    /* Recorder file url */
    char *url;
    /* Recorder file url length */
    uint32_t urlLen;
    /* Recorder file format */
    OH_ContainerFormatType fileFormat;
} OH_RecorderInfo;

/**
 * @brief Audio & Video screencapture config info
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 *
 * @since 10
 * @version 1.0
 */
typedef struct OH_AVScreenCaptureConfig {
    /* Capture mode */
    int captureMode;
    OH_DataType dataType;
    OH_AudioInfo audioInfo;
    OH_VideoInfo videoInfo;
    OH_RecorderInfo recorderInfo;
} OH_AVScreenCaptureConfig;

/**
 * @brief When an error occurs in the running of the OH_ScreenCapture instance, the function pointer will be called
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 * @param capture Pointer to an OH_ScreenCapture instance
 * @param errorCode specific error code
 *
 * @since 10
 * @version 1.0
 */
typedef void (*OH_ScreenCaptureOnError)(OH_ScreenCapture *capture, int32_t errorCode);

/**
 * @brief When audio buffer is available during the operation of OH_ScreenCapture, the function pointer will
 * be called.
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 * @param capture Pointer to an OH_ScreenCapture instance
 * @param isReady Information describing whether audio buffer is available
 * @param type Information describing the audio source type
 *
 * @since 10
 * @version 1.0
 */
typedef void (*OH_ScreenCaptureOnAudioBufferAvailable)(OH_ScreenCapture *capture, bool isReady,
    OH_AudioCapSourceType type);

/**
 * @brief When video buffer is available during the operation of OH_ScreenCapture, the function pointer will
 * be called.
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 * @param capture Pointer to an OH_ScreenCapture instance
 * @param isReady Information describing whether video buffer is available
 *
 * @since 10
 * @version 1.0
 */
typedef void (*OH_ScreenCaptureOnVideoBufferAvailable)(OH_ScreenCapture *capture, bool isReady);

/**
 * @brief A collection of all callback function pointers in OH_ScreenCapture. Register an instance of this
 * structure to the OH_ScreenCapture instance, and process the information reported through the callback to ensure the
 * normal operation of OH_ScreenCapture.
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 * @param onError Monitor OH_ScreenCapture operation errors, refer to {@link OH_ScreenCaptureOnError}
 * @param onAudioBufferAvailable Monitor audio buffer, refer to {@link OH_ScreenCaptureOnAudioBufferAvailable}
 * @param onVideoBufferAvailable Monitor video buffer, refer to {@link OH_ScreenCaptureOnVideoBufferAvailable}
 *
 * @since 10
 * @version 1.0
 */
typedef struct OH_ScreenCaptureCallback {
    OH_ScreenCaptureOnError onError;
    OH_ScreenCaptureOnAudioBufferAvailable onAudioBufferAvailable;
    OH_ScreenCaptureOnVideoBufferAvailable onVideoBufferAvailable;
} OH_ScreenCaptureCallback;

/**
 * @brief ScreenCapture rect info
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 *
 * @since 10
 * @version 1.0
 */
typedef struct OH_Rect {
    /* X-coordinate of screen recording */
    int32_t x;
    /* y-coordinate of screen recording */
    int32_t y;
    /* Width of screen recording */
    int32_t width;
    /* Height of screen recording */
    int32_t height;
} OH_Rect;


/**
 * @brief Audiobuffer struct info
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 *
 * @since 10
 * @version 1.0
 */
typedef struct OH_AudioBuffer {
    /* Audio buffer memory block  */
    uint8_t *buf;
    /* Audio buffer memory block size */
    int32_t size;
    /* Audio buffer timestamp info */
    int64_t timestamp;
    /* Audio capture source type */
    OH_AudioCapSourceType type;
} OH_AudioBuffer;

#ifdef __cplusplus
}
#endif

#endif // NATIVE_SCREEN_CAPTURE_BASE_H