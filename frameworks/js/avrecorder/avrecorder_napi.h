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

#ifndef AV_RECORDER_NAPI_H
#define AV_RECORDER_NAPI_H

#include "recorder.h"
#include "av_common.h"
#include "media_errors.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "common_napi.h"
#include "task_queue.h"
#include "recorder_profiles.h"

namespace OHOS {
namespace Media {
/* type AVRecorderState = 'idle' | 'prepared' | 'started' | 'paused' | 'stopped' | 'released' | 'error'; */
namespace AVRecorderState {
const std::string STATE_IDLE = "idle";
const std::string STATE_PREPARED = "prepared";
const std::string STATE_STARTED = "started";
const std::string STATE_PAUSED = "paused";
const std::string STATE_STOPPED = "stopped";
const std::string STATE_RELEASED = "released";
const std::string STATE_ERROR = "error";
}

namespace AVRecordergOpt {
const std::string PREPARE = "Prepare";
const std::string SET_ORIENTATION_HINT = "SetOrientationHint";
const std::string GETINPUTSURFACE = "GetInputSurface";
const std::string START = "Start";
const std::string PAUSE = "Pause";
const std::string RESUME = "Resume";
const std::string STOP = "Stop";
const std::string RESET = "Reset";
const std::string RELEASE = "Release";
const std::string GET_AV_RECORDER_PROFILE = "GetAVRecorderProfile";
const std::string SET_AV_RECORDER_CONFIG = "SetAVRecorderConfig";
const std::string GET_AV_RECORDER_CONFIG = "GetAVRecorderConfig";
const std::string GET_CURRENT_AUDIO_CAPTURER_INFO = "GetCurrentAudioCapturerInfo";
const std::string GET_MAX_AMPLITUDE = "GetMaxAmplitude";
const std::string GET_ENCODER_INFO = "GetEncoderInfo";
}

constexpr int32_t AVRECORDER_DEFAULT_AUDIO_BIT_RATE = 48000;
constexpr int32_t AVRECORDER_DEFAULT_AUDIO_CHANNELS = 2;
constexpr int32_t AVRECORDER_DEFAULT_AUDIO_SAMPLE_RATE = 48000;
constexpr int32_t AVRECORDER_DEFAULT_VIDEO_BIT_RATE = 48000;
constexpr int32_t AVRECORDER_DEFAULT_FRAME_HEIGHT = -1;
constexpr int32_t AVRECORDER_DEFAULT_FRAME_WIDTH = -1;
constexpr int32_t AVRECORDER_DEFAULT_FRAME_RATE = 30;

const std::map<std::string, std::vector<std::string>> stateCtrlList = {
    {AVRecorderState::STATE_IDLE, {
        AVRecordergOpt::PREPARE,
        AVRecordergOpt::RESET,
        AVRecordergOpt::RELEASE,
        AVRecordergOpt::GET_AV_RECORDER_PROFILE,
        AVRecordergOpt::SET_AV_RECORDER_CONFIG,
        AVRecordergOpt::GET_ENCODER_INFO
    }},
    {AVRecorderState::STATE_PREPARED, {
        AVRecordergOpt::SET_ORIENTATION_HINT,
        AVRecordergOpt::GETINPUTSURFACE,
        AVRecordergOpt::START,
        AVRecordergOpt::RESET,
        AVRecordergOpt::RELEASE,
        AVRecordergOpt::GET_CURRENT_AUDIO_CAPTURER_INFO,
        AVRecordergOpt::GET_MAX_AMPLITUDE,
        AVRecordergOpt::GET_ENCODER_INFO,
        AVRecordergOpt::GET_AV_RECORDER_CONFIG
    }},
    {AVRecorderState::STATE_STARTED, {
        AVRecordergOpt::START,
        AVRecordergOpt::RESUME,
        AVRecordergOpt::PAUSE,
        AVRecordergOpt::STOP,
        AVRecordergOpt::RESET,
        AVRecordergOpt::RELEASE,
        AVRecordergOpt::GET_CURRENT_AUDIO_CAPTURER_INFO,
        AVRecordergOpt::GET_MAX_AMPLITUDE,
        AVRecordergOpt::GET_ENCODER_INFO,
        AVRecordergOpt::GET_AV_RECORDER_CONFIG
    }},
    {AVRecorderState::STATE_PAUSED, {
        AVRecordergOpt::PAUSE,
        AVRecordergOpt::RESUME,
        AVRecordergOpt::STOP,
        AVRecordergOpt::RESET,
        AVRecordergOpt::RELEASE,
        AVRecordergOpt::GET_CURRENT_AUDIO_CAPTURER_INFO,
        AVRecordergOpt::GET_MAX_AMPLITUDE,
        AVRecordergOpt::GET_ENCODER_INFO,
        AVRecordergOpt::GET_AV_RECORDER_CONFIG
    }},
    {AVRecorderState::STATE_STOPPED, {
        AVRecordergOpt::STOP,
        AVRecordergOpt::PREPARE,
        AVRecordergOpt::RESET,
        AVRecordergOpt::RELEASE,
        AVRecordergOpt::GET_ENCODER_INFO,
        AVRecordergOpt::GET_AV_RECORDER_CONFIG
    }},
    {AVRecorderState::STATE_RELEASED, {
        AVRecordergOpt::RELEASE
    }},
    {AVRecorderState::STATE_ERROR, {
        AVRecordergOpt::RESET,
        AVRecordergOpt::RELEASE
    }},
};

/**
 * on(type: 'stateChange', callback: (state: AVPlayerState, reason: StateChangeReason) => void): void
 * on(type: 'error', callback: ErrorCallback): void
 * on(type: 'audioCaptureChange', callback: Callback<AudioCaptureChangeInfo>): void
 */
namespace AVRecorderEvent {
const std::string EVENT_STATE_CHANGE = "stateChange";
const std::string EVENT_ERROR = "error";
const std::string EVENT_AUDIO_CAPTURE_CHANGE = "audioCapturerChange";
}

struct AVRecorderAsyncContext;
struct AVRecorderProfile {
    int32_t audioBitrate = AVRECORDER_DEFAULT_AUDIO_BIT_RATE;
    int32_t audioChannels = AVRECORDER_DEFAULT_AUDIO_CHANNELS;
    int32_t auidoSampleRate = AVRECORDER_DEFAULT_AUDIO_SAMPLE_RATE;
    AudioCodecFormat audioCodecFormat = AudioCodecFormat::AUDIO_DEFAULT;

    int32_t videoBitrate = AVRECORDER_DEFAULT_VIDEO_BIT_RATE;
    int32_t videoFrameWidth = AVRECORDER_DEFAULT_FRAME_HEIGHT;
    int32_t videoFrameHeight = AVRECORDER_DEFAULT_FRAME_WIDTH;
    int32_t videoFrameRate = AVRECORDER_DEFAULT_FRAME_RATE;
    bool isHdr = false;
    bool enableTemporalScale = false;
    VideoCodecFormat videoCodecFormat = VideoCodecFormat::VIDEO_DEFAULT;

    OutputFormatType fileFormat = OutputFormatType::FORMAT_DEFAULT;
};

struct AVRecorderConfig {
    AudioSourceType audioSourceType; // source type;
    VideoSourceType videoSourceType;
    AVRecorderProfile profile;
    std::string url;
    int32_t rotation = 0; // Optional
    Location location; // Optional
    AVMetadata metadata; // Optional
    bool withVideo = false;
    bool withAudio = false;
    bool withLocation = false;
};

using RetInfo = std::pair<int32_t, std::string>;

class AVRecorderNapi {
public:
    __attribute__((visibility("default"))) static napi_value Init(napi_env env, napi_value exports);

    using AvRecorderTaskqFunc = RetInfo (AVRecorderNapi::*)();

private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    /**
     * createAVRecorder(callback: AsyncCallback<VideoPlayer>): void
     * createAVRecorder(): Promise<VideoPlayer>
     */
    static napi_value JsCreateAVRecorder(napi_env env, napi_callback_info info);
    /**
     * prepare(config: AVRecorderConfig, callback: AsyncCallback<void>): void;
     * prepare(config: AVRecorderConfig): Promise<void>;
     */
    static napi_value JsPrepare(napi_env env, napi_callback_info info);
    /**
     * setOrientationHint(config: AVRecorderConfig, callback: AsyncCallback<void>): void;
     * setOrientationHint(config: AVRecorderConfig): Promise<void>;
     */
    static napi_value JsSetOrientationHint(napi_env env, napi_callback_info info);
    /**
     * getInputSurface(callback: AsyncCallback<string>): void
     * getInputSurface(): Promise<string>
     */
    static napi_value JsGetInputSurface(napi_env env, napi_callback_info info);
    /**
     * start(callback: AsyncCallback<void>): void;
     * start(): Promise<void>;
     */
    static napi_value JsStart(napi_env env, napi_callback_info info);
    /**
     * pause(callback: AsyncCallback<void>): void;
     * pause(): Promise<void>;
     */
    static napi_value JsPause(napi_env env, napi_callback_info info);
    /**
     * resume(callback: AsyncCallback<void>): void;
     * resume(): Promise<void>;
     */
    static napi_value JsResume(napi_env env, napi_callback_info info);
    /**
     * stop(callback: AsyncCallback<void>): void;
     * stop(): Promise<void>;
     */
    static napi_value JsStop(napi_env env, napi_callback_info info);
    /**
     * reset(callback: AsyncCallback<void>): void
     * reset(): Promise<void>
     */
    static napi_value JsReset(napi_env env, napi_callback_info info);
    /**
     * release(callback: AsyncCallback<void>): void
     * release(): Promise<void>
     */
    static napi_value JsRelease(napi_env env, napi_callback_info info);
    /**
     * on(type: 'stateChange', callback: (state: AVPlayerState, reason: StateChangeReason) => void): void
     * on(type: 'error', callback: ErrorCallback): void
     * on(type: 'audioCaptureChange', callback: Callback<AudioCaptureChangeInfo>): void
     */
    static napi_value JsSetEventCallback(napi_env env, napi_callback_info info);
    /**
     * off(type: 'stateChange'): void;
     * off(type: 'error'): void;
     * off(type: 'audioCaptureChange'): void
     */
    static napi_value JsCancelEventCallback(napi_env env, napi_callback_info info);
    /**
     * readonly state: AVRecorderState;
     */
    static napi_value JsGetState(napi_env env, napi_callback_info info);
    /**
     * getVideoRecorderProfile(sourceId: number, qualityLevel: VideoRecorderQualityLevel,
     *     callback: AsyncCallback<AVRecorderProfile>);
     * getVideoRecorderProfile(sourceId: number, qualityLevel: VideoRecorderQualityLevel): Promise<AVRecorderProfile>;
    */
    static napi_value JsGetAVRecorderProfile(napi_env env, napi_callback_info info);
    /**
     * setAVRecorderConfig(config: AVRecorderConfig, callback: AsyncCallback<void>): void;
     * setAVRecorderConfig(config: AVRecorderConfig): Promise<void>;
    */
    static napi_value JsSetAVRecorderConfig(napi_env env, napi_callback_info info);
    /**
     * getAVRecorderConfig(callback: AsyncCallback<AVRecorderConfig>);
     * getAVRecorderConfig(): Promise<AVRecorderConfig>;
    */
    static napi_value JsGetAVRecorderConfig(napi_env env, napi_callback_info info);
    /**
     * getCurrentAudioCapturerInfo(callback: AsyncCallback<audio.AudioCapturerChangeInfo>): void;
     * getCurrentAudioCapturerInfo(): Promise<audio.AudioCapturerChangeInfo>;
    */
    static napi_value JsGetCurrentAudioCapturerInfo(napi_env env, napi_callback_info info);
    /**
     * getAudioCapturerMaxAmplitude(callback: AsyncCallback<number>): void;
     * getAudioCapturerMaxAmplitude(): Promise<number>;
    */
    static napi_value JsGetAudioCapturerMaxAmplitude(napi_env env,  napi_callback_info info);
    /**
     * getAvailableEncoder(callback: AsyncCallback<Array<EncoderInfo>>): void;
     * getAvailableEncoder(): Promise<Array<EncoderInfo>>;
    */
    static napi_value JsGetAvailableEncoder(napi_env env,  napi_callback_info info);

    static AVRecorderNapi* GetJsInstanceAndArgs(napi_env env, napi_callback_info info,
        size_t &argCount, napi_value *args);
    static std::shared_ptr<TaskHandler<RetInfo>> GetPrepareTask(std::unique_ptr<AVRecorderAsyncContext> &asyncCtx);
    static std::shared_ptr<TaskHandler<RetInfo>> GetSetOrientationHintTask(
        const std::unique_ptr<AVRecorderAsyncContext> &asyncCtx);
    static std::shared_ptr<TaskHandler<RetInfo>> GetPromiseTask(AVRecorderNapi *avnapi, const std::string &opt);
    static std::shared_ptr<TaskHandler<RetInfo>> GetAVRecorderProfileTask(
        const std::unique_ptr<AVRecorderAsyncContext> &asyncCtx);
    static std::shared_ptr<TaskHandler<RetInfo>> SetAVRecorderConfigTask(
        std::unique_ptr<AVRecorderAsyncContext> &asyncCtx);
    static napi_value ExecuteByPromise(napi_env env, napi_callback_info info, const std::string &opt);
    static std::shared_ptr<TaskHandler<RetInfo>> GetAVRecorderConfigTask(
        const std::unique_ptr<AVRecorderAsyncContext> &asyncCtx);
    static std::shared_ptr<TaskHandler<RetInfo>> GetCurrentCapturerChangeInfoTask(
        const std::unique_ptr<AVRecorderAsyncContext> &asyncCtx);
    static std::shared_ptr<TaskHandler<RetInfo>> GetMaxAmplitudeTask(
        const std::unique_ptr<AVRecorderAsyncContext> &asyncCtx);
    static std::shared_ptr<TaskHandler<RetInfo>> GetEncoderInfoTask(
        const std::unique_ptr<AVRecorderAsyncContext> &asyncCtx);
    static int32_t GetAudioCodecFormat(const std::string &mime, AudioCodecFormat &codecFormat);
    static int32_t GetVideoCodecFormat(const std::string &mime, VideoCodecFormat &codecFormat);
    static int32_t GetOutputFormat(const std::string &extension, OutputFormatType &type);

    static int32_t GetPropertyInt32(napi_env env, napi_value configObj, const std::string &type, int32_t &result,
        bool &getValue);

    static int32_t GetAVRecorderProfile(std::shared_ptr<AVRecorderProfile> &profile,
        const int32_t sourceId, const int32_t qualityLevel);
    static void MediaProfileLog(bool isVideo, AVRecorderProfile &profile);

    AVRecorderNapi();
    ~AVRecorderNapi();

    RetInfo GetInputSurface();
    RetInfo Start();
    RetInfo Pause();
    RetInfo Resume();
    RetInfo Stop();
    RetInfo Reset();
    RetInfo Release();
    RetInfo GetVideoRecorderProfile();

    int32_t GetAVRecorderConfig(std::shared_ptr<AVRecorderConfig> &config);
    int32_t GetCurrentCapturerChangeInfo(AudioRecorderChangeInfo &changeInfo);
    int32_t GetMaxAmplitude(int32_t &maxAmplitude);
    int32_t GetEncoderInfo(std::vector<EncoderCapabilityData> &encoderInfo);

    void ErrorCallback(int32_t errCode, const std::string &operate, const std::string &add = "");
    void StateCallback(const std::string &state);
    void SetCallbackReference(const std::string &callbackName, std::shared_ptr<AutoRef> ref);
    void CancelCallbackReference(const std::string &callbackName);
    void CancelCallback();
    void RemoveSurface();

    int32_t CheckStateMachine(const std::string &opt);
    int32_t CheckRepeatOperation(const std::string &opt);
    int32_t GetSourceType(std::unique_ptr<AVRecorderAsyncContext> &asyncCtx, napi_env env, napi_value args);
    int32_t GetAudioProfile(std::unique_ptr<AVRecorderAsyncContext> &asyncCtx, napi_env env, napi_value item,
        AVRecorderProfile &profile);
    int32_t GetVideoProfile(std::unique_ptr<AVRecorderAsyncContext> &asyncCtx, napi_env env, napi_value item,
        AVRecorderProfile &profile);
    int32_t GetProfile(std::unique_ptr<AVRecorderAsyncContext> &asyncCtx, napi_env env, napi_value args);
    int32_t GetConfig(std::unique_ptr<AVRecorderAsyncContext> &asyncCtx, napi_env env, napi_value args);
    int32_t GetRotation(std::unique_ptr<AVRecorderAsyncContext> &asyncCtx, napi_env env, napi_value args);
    int32_t GetAVMetaData(std::unique_ptr<AVRecorderAsyncContext> &asyncCtx, napi_env env, napi_value args);
    bool GetLocation(std::unique_ptr<AVRecorderAsyncContext> &asyncCtx, napi_env env, napi_value args);
    int32_t GetSourceIdAndQuality(std::unique_ptr<AVRecorderAsyncContext> &asyncCtx, napi_env env,
        napi_value sourceIdArgs, napi_value qualityArgs, const std::string &opt);
    RetInfo SetProfile(std::shared_ptr<AVRecorderConfig> config);
    RetInfo Configure(std::shared_ptr<AVRecorderConfig> config);

    static thread_local napi_ref constructor_;
    napi_env env_ = nullptr;
    std::shared_ptr<Recorder> recorder_ = nullptr;
    std::shared_ptr<RecorderCallback> recorderCb_ = nullptr;
    std::map<std::string, std::shared_ptr<AutoRef>> eventCbMap_;
    std::unique_ptr<TaskQueue> taskQue_;
    static std::map<std::string, AvRecorderTaskqFunc> taskQFuncs_;
    sptr<Surface> surface_ = nullptr;
    int32_t videoSourceID_ = -1;
    int32_t audioSourceID_ = -1;
    bool withVideo_ = false;
    bool getVideoInputSurface_ = false;
    int32_t sourceId_ = -1;
    int32_t qualityLevel_ = -1;
    bool hasConfiged_ = false;
};

struct AVRecorderAsyncContext : public MediaAsyncContext {
    explicit AVRecorderAsyncContext(napi_env env) : MediaAsyncContext(env) {}
    ~AVRecorderAsyncContext() = default;

    void AVRecorderSignError(int32_t errCode, const std::string &operate,
        const std::string &param, const std::string &add = "");

    AVRecorderNapi *napi = nullptr;
    std::shared_ptr<AVRecorderConfig> config_ = nullptr;
    std::string opt_ = "";
    std::shared_ptr<TaskHandler<RetInfo>> task_ = nullptr;
    std::shared_ptr<AVRecorderProfile> profile_ = nullptr;
    AudioRecorderChangeInfo changeInfo_;
    int32_t maxAmplitude_ = 0;
    std::vector<EncoderCapabilityData> encoderInfo_;
};

class MediaJsResultExtensionMethod {
public:
    static int32_t SetAudioCodecFormat(AudioCodecFormat &codecFormat, std::string &mime);
    static int32_t SetVideoCodecFormat(VideoCodecFormat &codecFormat, std::string &mime);
    static int32_t SetFileFormat(OutputFormatType &type, std::string &extension);
};
class MediaJsAVRecorderProfile : public MediaJsResult {
public:
    explicit MediaJsAVRecorderProfile(std::shared_ptr<AVRecorderProfile> value)
        : value_(value)
    {
    }
    ~MediaJsAVRecorderProfile() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;

private:
    std::shared_ptr<AVRecorderProfile> value_ = nullptr;
};
class MediaJsAVRecorderConfig : public MediaJsResult {
public:
    explicit MediaJsAVRecorderConfig(std::shared_ptr<AVRecorderConfig> value)
        : value_(value)
    {
    }
    ~MediaJsAVRecorderConfig() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;
    napi_status audioToSet(napi_env env, napi_value &profile, napi_value &result);
    napi_status videoToSet(napi_env env, napi_value &profile, napi_value &result);
    napi_status locationToSet(napi_env env, napi_value &location, napi_value &result);

private:
    std::shared_ptr<AVRecorderConfig> value_ = nullptr;
};
class MediaJsEncoderInfo : public MediaJsResult {
public:
    explicit MediaJsEncoderInfo(const std::vector<EncoderCapabilityData> encoderInfo)
        : encoderInfo_(encoderInfo)
    {
    }
    ~MediaJsEncoderInfo() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;
    napi_status GetAudioEncoderInfo(
        napi_env env, EncoderCapabilityData encoderCapData, napi_value &result, uint32_t position);
    napi_status GetVideoEncoderInfo(
        napi_env env, EncoderCapabilityData encoderCapData, napi_value &result, uint32_t position);
private:
    std::vector<EncoderCapabilityData> encoderInfo_;
};
} // namespace Media
} // namespace OHOS
#endif // AV_RECORDER_NAPI_H