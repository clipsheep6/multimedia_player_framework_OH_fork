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

#ifndef SCREEN_CAPTURE_H
#define SCREEN_CAPTURE_H

#include <cstdint>
#include <memory>
#include <list>
#include <set>
#include "avcodec_info.h"
#include "surface.h"
#include "recorder.h"

namespace OHOS {
namespace Media {
namespace ScreenCaptureState {
const std::string STATE_IDLE = "idle";
const std::string STATE_STARTED = "started";
const std::string STATE_STOPPED = "stopped";
const std::string STATE_ERROR = "error";
}

namespace ScreenCaptureEvent {
const std::string EVENT_STATE_CHANGE = "stateChange";
const std::string EVENT_ERROR = "error";
}

enum ScreenCaptureErrorType : int32_t {
    SCREEN_CAPTURE_ERROR_INTERNAL,
    SCREEN_CAPTURE_ERROR_EXTEND_START = 0X10000,
};

enum AVScreenCaptureErrorCode {
    SCREEN_CAPTURE_ERR_BASE = 0,
    SCREEN_CAPTURE_ERR_OK = SCREEN_CAPTURE_ERR_BASE,
    SCREEN_CAPTURE_ERR_NO_MEMORY = SCREEN_CAPTURE_ERR_BASE + 1,
    SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT = SCREEN_CAPTURE_ERR_BASE + 2,
    SCREEN_CAPTURE_ERR_INVALID_VAL = SCREEN_CAPTURE_ERR_BASE + 3,
    SCREEN_CAPTURE_ERR_IO = SCREEN_CAPTURE_ERR_BASE + 4,
    SCREEN_CAPTURE_ERR_TIMEOUT = SCREEN_CAPTURE_ERR_BASE + 5,
    SCREEN_CAPTURE_ERR_UNKNOWN = SCREEN_CAPTURE_ERR_BASE + 6,
    SCREEN_CAPTURE_ERR_SERVICE_DIED = SCREEN_CAPTURE_ERR_BASE + 7,
    SCREEN_CAPTURE_ERR_INVALID_STATE = SCREEN_CAPTURE_ERR_BASE + 8,
    SCREEN_CAPTURE_ERR_UNSUPPORT = SCREEN_CAPTURE_ERR_BASE + 9,
    SCREEN_CAPTURE_ERR_EXTEND_START = SCREEN_CAPTURE_ERR_BASE + 100,
};

enum AudioCaptureSourceType : int32_t {
    /** Invalid audio source */
    SOURCE_INVALID = -1,
    /** Default audio source */
    SOURCE_DEFAULT = 0,
    /** Microphone */
    MIC = 1,
    /** all PlayBack **/
    ALL_PLAYBACK = 2,
    /** app PlayBack **/
    APP_PLAYBACK = 3
};

enum DataType {
    ORIGINAL_STREAM = 0,
    ENCODED_STREAM = 1,
    CAPTURE_FILE = 2,
    INVAILD = -1
};

enum CaptureMode : int32_t {
    /* capture home screen */
    CAPTURE_HOME_SCREEN = 0,
    /* capture a specified screen */
    CAPTURE_SPECIFIED_SCREEN = 1,
    /* capture a specified window */
    CAPTURE_SPECIFIED_WINDOW = 2,
    CAPTURE_INVAILD = -1
};

enum AVScreenCaptureStateCode {
    /* Screen capture state INVALID */
    SCREEN_CAPTURE_STATE_INVLID = -1,
    /* Screen capture started by user */
    SCREEN_CAPTURE_STATE_STARTED = 0,
    /* Screen capture canceled by user */
    SCREEN_CAPTURE_STATE_CANCELED = 1,
    /* ScreenCapture stopped by user */
    SCREEN_CAPTURE_STATE_STOPPED_BY_USER = 2,
    /* ScreenCapture interrupted by other screen capture */
    SCREEN_CAPTURE_STATE_INTERRUPTED_BY_OTHER = 3,
    /* ScreenCapture stopped by SIM call */
    SCREEN_CAPTURE_STATE_STOPPED_BY_CALL = 4,
    /* Microphone is temporarily unavailable */
    SCREEN_CAPTURE_STATE_MIC_UNAVAILABLE = 5,
    /* Microphone is muted by user */
    SCREEN_CAPTURE_STATE_MIC_MUTED_BY_USER = 6,
    /* Microphone is unmuted by user */
    SCREEN_CAPTURE_STATE_MIC_UNMUTED_BY_USER = 7,
    /* Current captured screen has private window */
    SCREEN_CAPTURE_STATE_ENTER_PRIVATE_SCENE = 8,
    /* Private window disappeared on current captured screen*/
    SCREEN_CAPTURE_STATE_EXIT_PRIVATE_SCENE = 9,
};

enum AVScreenCaptureBufferType {
    /* Buffer of video data from screen */
    SCREEN_CAPTURE_BUFFERTYPE_INVALID = -1,
    /* Buffer of video data from screen */
    SCREEN_CAPTURE_BUFFERTYPE_VIDEO = 0,
    /* Buffer of audio data from inner capture */
    SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER = 1,
    /* Buffer of audio data from microphone */
    SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC = 2,
};

enum AVScreenCaptureFilterableAudioContent {
    /* Audio content of notification sound */
    SCREEN_CAPTURE_NOTIFICATION_AUDIO = 0,
    /* Audio content of the sound of the app itself */
    SCREEN_CAPTURE_CURRENT_APP_AUDIO = 1,
};

enum AVScreenCaptureParamValidationState : int32_t {
    VALIDATION_IGNORE,
    VALIDATION_VALID,
    VALIDATION_INVALID,
};

struct ScreenCaptureContentFilter {
    std::set<AVScreenCaptureFilterableAudioContent> filteredAudioContents;
    std::vector<int32_t> windowIDsVec;
};

struct AudioCaptureInfo {
    int32_t audioSampleRate = 0;
    int32_t audioChannels = 0;
    AudioCaptureSourceType audioSource = AudioCaptureSourceType::SOURCE_DEFAULT;
    AVScreenCaptureParamValidationState state = AVScreenCaptureParamValidationState::VALIDATION_IGNORE;
};

struct AudioEncInfo {
    int32_t audioBitrate = 0;
    AudioCodecFormat audioCodecformat = AudioCodecFormat::AUDIO_CODEC_FORMAT_BUTT;
    AVScreenCaptureParamValidationState state = AVScreenCaptureParamValidationState::VALIDATION_IGNORE;
};

struct AudioInfo {
    AudioCaptureInfo micCapInfo;
    AudioCaptureInfo innerCapInfo;
    AudioEncInfo audioEncInfo;
};

struct VideoCaptureInfo {
    uint64_t displayId = -1;
    std::list<int32_t> taskIDs;
    int32_t videoFrameWidth = 0;
    int32_t videoFrameHeight = 0;
    VideoSourceType videoSource = VideoSourceType::VIDEO_SOURCE_BUTT;
    AVScreenCaptureParamValidationState state = AVScreenCaptureParamValidationState::VALIDATION_IGNORE;
};

struct VideoEncInfo {
    VideoCodecFormat videoCodec = VideoCodecFormat::VIDEO_CODEC_FORMAT_BUTT;
    int32_t videoBitrate = 0;
    int32_t videoFrameRate = 0;
    AVScreenCaptureParamValidationState state = AVScreenCaptureParamValidationState::VALIDATION_IGNORE;
};

struct VideoInfo {
    VideoCaptureInfo videoCapInfo;
    VideoEncInfo videoEncInfo;
};

struct RecorderInfo {
    std::string url;
    std::string fileFormat;
};

struct AVScreenCaptureConfig {
    CaptureMode captureMode = CaptureMode::CAPTURE_INVAILD;
    DataType dataType = DataType::INVAILD;
    AudioInfo audioInfo;
    VideoInfo videoInfo;
    RecorderInfo recorderInfo;
};

struct AudioBuffer {
    AudioBuffer(uint8_t *buf, int32_t size, int64_t timestamp, AudioCaptureSourceType type)
        : buffer(std::move(buf)), length(size), timestamp(timestamp), sourcetype(type)
    {
    }
    ~AudioBuffer()
    {
        if (buffer != nullptr) {
            free(buffer);
            buffer = nullptr;
        }
        length = 0;
        timestamp = 0;
    }
    uint8_t *buffer;
    int32_t length;
    int64_t timestamp;
    AudioCaptureSourceType sourcetype;
};

class ScreenCaptureCallBack {
public:
    virtual ~ScreenCaptureCallBack() = default;

    /**
     * @brief Called when an error occurs during recording. This callback is used to report recording errors.
     *
     * @param errorType Indicates the error type. For details, see {@link RecorderErrorType}.
     * @param errorCode Indicates the error code.
     * @since 1.0
     * @version 1.0
     */
    virtual void OnError(ScreenCaptureErrorType errorType, int32_t errorCode) = 0;

    virtual void OnAudioBufferAvailable(bool isReady, AudioCaptureSourceType type) = 0;

    virtual void OnVideoBufferAvailable(bool isReady) = 0;

    virtual void OnStateChange(AVScreenCaptureStateCode stateCode)
    {
        (void)stateCode;
        return;
    }
};

class ScreenCapture {
public:
    virtual ~ScreenCapture() = default;
    virtual int32_t Init(AVScreenCaptureConfig config) = 0;
    virtual int32_t SetMicrophoneEnabled(bool isMicrophone) = 0;
    virtual int32_t SetCanvasRotation(bool canvasRotation) = 0;
    virtual int32_t StartScreenCapture() = 0;
    virtual int32_t StartScreenCaptureWithSurface(sptr<Surface> surface) = 0;
    virtual int32_t StopScreenCapture() = 0;
    virtual int32_t StartScreenRecording() = 0;
    virtual int32_t StopScreenRecording() = 0;
    virtual int32_t AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audiobuffer, AudioCaptureSourceType type) = 0;
    virtual sptr<OHOS::SurfaceBuffer> AcquireVideoBuffer(int32_t &fence, int64_t &timestamp, Rect &damage) = 0;
    virtual int32_t ReleaseAudioBuffer(AudioCaptureSourceType type) = 0;
    virtual int32_t ReleaseVideoBuffer() = 0;
    virtual int32_t Release() = 0;
    virtual int32_t SetScreenCaptureCallback(const std::shared_ptr<ScreenCaptureCallBack> &callback) = 0;
    virtual int32_t ExcludeContent(ScreenCaptureContentFilter &contentFilter) = 0;
    virtual int32_t SetPrivacyAuthorityEnabled() = 0;
};

class __attribute__((visibility("default"))) ScreenCaptureFactory {
public:
#ifdef UNSUPPORT_SCREEN_CAPTURE
    static std::shared_ptr<ScreenCapture> CreateScreenCapture()
    {
        return nullptr;
    }
#else
    static std::shared_ptr<ScreenCapture> CreateScreenCapture();
#endif

private:
    ScreenCaptureFactory() = default;
    ~ScreenCaptureFactory() = default;
};
} // namespace Media
} // namespace OHOS
#endif // SCREEN_CAPTURE_H