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
#include "media_errors.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "common_napi.h"
#include "task_queue.h"

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

constexpr int32_t AVRECORDER_DEFAULT_AUDIO_BIT_RATE = 48000;
constexpr int32_t AVRECORDER_DEFAULT_AUDIO_CHANNELS = 2;
constexpr int32_t AVRECORDER_DEFAULT_AUDIO_SAMPLE_RATE = 48000;
constexpr int32_t AVRECORDER_DEFAULT_VIDEO_BIT_RATE = 48000;
constexpr int32_t AVRECORDER_DEFAULT_FRAME_HEIGHT = -1;
constexpr int32_t AVRECORDER_DEFAULT_FRAME_WIDTH = -1;
constexpr int32_t AVRECORDER_DEFAULT_FRAME_RATE = 30;

/**
 * on(type: 'stateChange', callback: (state: AVPlayerState, reason: StateChangeReason) => void): void
 * on(type: 'error', callback: ErrorCallback): void
 */
namespace AVRecorderEvent {
const std::string EVENT_STATE_CHANGE = "stateChange";
const std::string EVENT_ERROR = "error";
}

class AVRecorderNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);

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
     * start(): void
     */
    static napi_value JsStart(napi_env env, napi_callback_info info);
    /**
     * pause(): void
     */
    static napi_value JsPause(napi_env env, napi_callback_info info);
    /**
     * resume(): void
     */
    static napi_value JsResume(napi_env env, napi_callback_info info);
    /**
     * stop(): void
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
     * getInputSurface(callback: AsyncCallback<string>): void
     * getInputSurface(): Promise<string>
     */
    static napi_value JsGetInputSurface(napi_env env, napi_callback_info info);
    /**
     * on(type: 'stateChange', callback: (state: AVPlayerState, reason: StateChangeReason) => void): void
     * on(type: 'error', callback: ErrorCallback): void
     */
    static napi_value JsSetEventCallback(napi_env env, napi_callback_info info);
    /**
     * readonly state: AVRecorderState;
     */
    static napi_value GetState(napi_env env, napi_callback_info info);

    static AVRecorderNapi* GetAVRecorderNapi(napi_env env, napi_callback_info info);
    AVRecorderNapi();
    ~AVRecorderNapi();

    void ErrorCallback(int32_t errCode, const std::string& param1);
    void StateCallback(const std::string &state);
    void SetCallbackReference(const std::string &callbackName, std::shared_ptr<AutoRef> ref);
    void CancelCallback();
    void RemoveSurface();

    int32_t GetConfig(napi_env env, napi_value args);
    int32_t GetProfile(napi_env env, napi_value args);
    int32_t SetProfile();
    int32_t SetConfiguration();

    struct AVRecorderProfile {
        int32_t audioBitrate = AVRECORDER_DEFAULT_AUDIO_BIT_RATE;
        int32_t audioChannels = AVRECORDER_DEFAULT_AUDIO_CHANNELS;
        int32_t auidoSampleRate = AVRECORDER_DEFAULT_AUDIO_SAMPLE_RATE;
        AudioCodecFormat audioCodecFormat = AudioCodecFormat::AUDIO_DEFAULT;

        int32_t videoBitrate = AVRECORDER_DEFAULT_VIDEO_BIT_RATE;
        int32_t videoFrameWidth = AVRECORDER_DEFAULT_FRAME_HEIGHT;
        int32_t videoFrameHeight = AVRECORDER_DEFAULT_FRAME_WIDTH;
        int32_t videoFrameRate = AVRECORDER_DEFAULT_FRAME_RATE;
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
    };

    AVRecorderConfig config;
    static thread_local napi_ref constructor_;
    napi_env env_ = nullptr;
    napi_ref wrapper_ = nullptr;
    std::shared_ptr<Recorder> recorder_ = nullptr;
    std::shared_ptr<RecorderCallback> recorderCb_ = nullptr;
    std::map<std::string, std::shared_ptr<AutoRef>> eventCbMap_;
    sptr<Surface> surface_ = nullptr;
    int32_t videoSourceID = -1;
    int32_t audioSourceID = -1;
    std::unique_ptr<TaskQueue> taskQue_;
    bool withVideo_ = false;
    bool withAudio_ = false;
};

struct AVRecorderAsyncContext : public MediaAsyncContext {
    explicit AVRecorderAsyncContext(napi_env env) : MediaAsyncContext(env) {}
    ~AVRecorderAsyncContext() = default;

    void AVRecorderSignError(int32_t code, const std::string &param1, const std::string &param2);

    AVRecorderNapi *napi = nullptr;
};


#define CHECK_AND_RETURN_SIGNERROR(cond, ctx, ret, act)                                                     \
    do {                                                                                                    \
        if (!(cond)) {                                                                                      \
            MediaServiceExtErrCodeAPI9 _err = MSErrorToExtErrorAPI9(static_cast<MediaServiceErrCode>(ret)); \
            ctx->AVRecorderSignError(_err, act, "");                                                        \
            return;                                                                                         \
        }                                                                                                   \
    } while (0)
} // namespace Media
} // namespace OHOS
#endif // AV_RECORDER_NAPI_H