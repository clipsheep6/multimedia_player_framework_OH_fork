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

enum AudioCapSourceType : int32_t {
    /** Invalid audio source */
    SOURCE_INVALID = -1,
    /** Default audio source */
    SOURCE_DEFAULT = 0,
    /** Microphone */
    MIC = 1,
    /**PlayBack**/
    PLAYBACK = 2,
};

enum DataType {
    ORIGINAL_STREAM = 0,
    ENCODED_STREAM = 1,
    CAPTURE_FILE = 2,
    INVAILD = -1
};

struct AudioCapInfo {
    int32_t audioSampleRate = 0;
    int32_t audioChannels = 0;
    AudioCapSourceType audioSource;
};

struct AudioEncInfo {
    int32_t audioBitrate;
    AudioCodecFormat audioCodecformat;
};

struct AudioInfo {
    AudioCapInfo micCapInfo;
    AudioCapInfo innerCapInfo;
    AudioEncInfo audioEncInfo;
};

struct VideoCapInfo {
    uint64_t displayId;
    std::list<int32_t> taskIDs;
    int32_t videoFrameWidth;
    int32_t videoFrameHeight;
    VideoSourceType videoSource;
};

struct VideoEncInfo {
    VideoCodecFormat videoCodec;
    int32_t videoBitrate;
    int32_t videoFrameRate;
};

struct VideoInfo {
    VideoCapInfo videoCapInfo;
    VideoEncInfo videoEncInfo;
};

struct RecorderInfo {
    std::string url;
    std::string fileFormat;
};

struct AVScreenCaptureConfig {
    int captureMode;
    DataType dataType;
    AudioInfo audioInfo;
    VideoInfo videoInfo;
    RecorderInfo recorderInfo;
};

struct AudioBuffer {
    AudioBuffer(uint8_t *buf, int32_t size, int64_t timestamp, AudioCapSourceType type)
        : buffer(std::move(buf)), length(size), timestamp(timestamp), sourcetype(type)
    {
    }
    ~AudioBuffer()
    {
        if (buffer != nullptr) {
            buffer = nullptr;
        }
        length = 0;
        timestamp = 0;
    }
    uint8_t *buffer;
    int32_t length;
    int64_t timestamp;
    AudioCapSourceType sourcetype;
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

    virtual void OnAudioBufferAvailable(bool isReady, AudioCapSourceType type) = 0;

    virtual void OnVideoBufferAvailable(bool isReady) = 0;
};

class ScreenCapture {
public:
    virtual ~ScreenCapture() = default;
    virtual int32_t Init(AVScreenCaptureConfig config) = 0;
    virtual int32_t SetMicrophoneEnable(bool isMicrophone) = 0;
    virtual int32_t StartScreenCapture(DataType type) = 0;
    virtual int32_t StopScreenCapture() = 0;
    virtual int32_t AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audiobuffer, AudioCapSourceType type) = 0;
    virtual sptr<OHOS::SurfaceBuffer> AcquireVideoBuffer(int32_t &fence, int64_t &timestamp, Rect &damage) = 0;
    virtual int32_t ReleaseAudioBuffer(AudioCapSourceType type) = 0;
    virtual int32_t ReleaseVideoBuffer() = 0;
    virtual int32_t Release() = 0;
    virtual int32_t SetScreenCaptureCallback(const std::shared_ptr<ScreenCaptureCallBack> &callback) = 0;
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