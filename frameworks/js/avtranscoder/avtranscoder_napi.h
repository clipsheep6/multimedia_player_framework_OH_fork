/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef AV_TRANSCODER_NAPI_H
#define AV_TRANSCODER_NAPI_H

#include "av_common.h"
#include "media_errors.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "common_napi.h"
#include "task_queue.h"
#include "transcoder.h"

namespace OHOS {
namespace Media {
namespace AVTransCoderState {
const std::string STATE_IDLE = "idle";
const std::string STATE_PREPARED = "prepared";
const std::string STATE_STARTED = "started";
const std::string STATE_PAUSED = "paused";
const std::string STATE_CANCELLED = "cancelled";
const std::string STATE_COMPLETED = "completed";
const std::string STATE_RELEASED = "released";
const std::string STATE_ERROR = "error";
}

namespace AVTransCoderOpt {
const std::string PREPARE = "Prepare";
const std::string START = "Start";
const std::string PAUSE = "Pause";
const std::string RESUME = "Resume";
const std::string CANCEL = "Cancel";
const std::string RELEASE = "Release";
const std::string SET_AV_TRANSCODER_CONFIG = "SetAVTransCoderConfig";
}

constexpr int32_t AVTRANSCODER_DEFAULT_AUDIO_BIT_RATE = 48000;
constexpr int32_t AVTRANSCODER_DEFAULT_VIDEO_BIT_RATE = 48000;
constexpr int32_t AVTRANSCODER_DEFAULT_FRAME_HEIGHT = -1;
constexpr int32_t AVTRANSCODER_DEFAULT_FRAME_WIDTH = -1;

const std::map<std::string, std::vector<std::string>> STATE_LIST = {
    {AVTransCoderState::STATE_IDLE, {
        AVTransCoderOpt::PREPARE,
        AVTransCoderOpt::RELEASE
    }},
    {AVTransCoderState::STATE_PREPARED, {
        AVTransCoderOpt::START,
        AVTransCoderOpt::RELEASE
    }},
    {AVTransCoderState::STATE_STARTED, {
        AVTransCoderOpt::START,
        AVTransCoderOpt::PAUSE,
        AVTransCoderOpt::RESUME,
        AVTransCoderOpt::CANCEL,
        AVTransCoderOpt::RELEASE
    }},
    {AVTransCoderState::STATE_PAUSED, {
        AVTransCoderOpt::START,
        AVTransCoderOpt::PAUSE,
        AVTransCoderOpt::RESUME,
        AVTransCoderOpt::CANCEL,
        AVTransCoderOpt::RELEASE
    }},
    {AVTransCoderState::STATE_CANCELLED, {
        AVTransCoderOpt::RELEASE
    }},
    {AVTransCoderState::STATE_COMPLETED, {
        AVTransCoderOpt::RELEASE
    }},
    {AVTransCoderState::STATE_RELEASED, {
        AVTransCoderOpt::RELEASE
    }},
    {AVTransCoderState::STATE_ERROR, {
        AVTransCoderOpt::RELEASE
    }},
};

/**
 * on(type: 'stateChange', callback: (state: AVPlayerState, reason: StateChangeReason) => void): void
 * on(type: 'error', callback: ErrorCallback): void
 * on(type: 'audioCaptureChange', callback: Callback<AudioCaptureChangeInfo>): void
 */
namespace AVTransCoderEvent {
const std::string EVENT_STATE_CHANGE = "stateChange";
const std::string EVENT_ERROR = "error";
const std::string EVENT_PROGRESS_UPDATE = "progressUpdate";
}

struct AVTransCoderAsyncContext;

struct AVTransCoderConfig {
    AudioCodecFormat audioCodecFormat = AudioCodecFormat::AUDIO_DEFAULT;
    int32_t audioBitrate = AVTRANSCODER_DEFAULT_AUDIO_BIT_RATE;
    OutputFormatType fileFormat = OutputFormatType::FORMAT_DEFAULT;
    VideoCodecFormat videoCodecFormat = VideoCodecFormat::VIDEO_DEFAULT;
    int32_t videoBitrate = AVTRANSCODER_DEFAULT_VIDEO_BIT_RATE;
    int32_t videoFrameWidth = AVTRANSCODER_DEFAULT_FRAME_HEIGHT;
    int32_t videoFrameHeight = AVTRANSCODER_DEFAULT_FRAME_WIDTH;
};

using RetInfo = std::pair<int32_t, std::string>;

class AVTransCoderNapi {
public:
    __attribute__((visibility("default"))) static napi_value Init(napi_env env, napi_value exports);

    using AvTransCoderTaskqFunc = RetInfo (AVTransCoderNapi::*)();

private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    /**
     * createAVTransCoder(): Promise<VideoPlayer>
     */
    static napi_value JsCreateAVTransCoder(napi_env env, napi_callback_info info);
    /**
     * prepare(config: AVTransCoderConfig): Promise<void>;
     */
    static napi_value JsPrepare(napi_env env, napi_callback_info info);
    /**
     * start(): Promise<void>;
     */
    static napi_value JsStart(napi_env env, napi_callback_info info);
    /**
     * pause(): Promise<void>;
     */
    static napi_value JsPause(napi_env env, napi_callback_info info);
    /**
     * resume(): Promise<void>;
     */
    static napi_value JsResume(napi_env env, napi_callback_info info);
    /**
     * cancel(): Promise<void>;
     */
    static napi_value JsCancel(napi_env env, napi_callback_info info);
    /**
     * release(): Promise<void>
     */
    static napi_value JsRelease(napi_env env, napi_callback_info info);
    /**
     * on(type: 'stateChange', callback: (state: AVTransCoderState, reason: StateChangeReason) => void): void
     * on(type: 'error', callback: ErrorCallback): void
     * on(type: 'progressUpdate', callback: Callback<number>): void
     */
    static napi_value JsSetEventCallback(napi_env env, napi_callback_info info);
    /**
     * off(type: 'stateChange'): void;
     * off(type: 'error'): void;
     * off(type: 'progressUpdate'): void
     */
    static napi_value JsCancelEventCallback(napi_env env, napi_callback_info info);

    /**
     * srcUrl: string
     */
    static napi_value JsSetSrcUrl(napi_env env, napi_callback_info info);
    static napi_value JsGetSrcUrl(napi_env env, napi_callback_info info);

    static napi_value JsSetSrcFd(napi_env env, napi_callback_info info);
    static napi_value JsGetSrcFd(napi_env env, napi_callback_info info);

    static napi_value JsSetDstFd(napi_env env, napi_callback_info info);
    static napi_value JsGetDstFd(napi_env env, napi_callback_info info);

    static AVTransCoderNapi* GetJsInstanceAndArgs(napi_env env, napi_callback_info info,
        size_t &argCount, napi_value *args);
    static napi_value ExecuteByPromise(napi_env env, napi_callback_info info, const std::string &opt);
    static std::shared_ptr<TaskHandler<RetInfo>> GetPrepareTask(std::unique_ptr<AVTransCoderAsyncContext> &asyncCtx);
    static std::shared_ptr<TaskHandler<RetInfo>> GetPromiseTask(AVTransCoderNapi *avnapi, const std::string &opt);

    static int32_t GetAudioCodecFormat(const std::string &mime, AudioCodecFormat &codecFormat);
    static int32_t GetVideoCodecFormat(const std::string &mime, VideoCodecFormat &codecFormat);
    static int32_t GetOutputFormat(const std::string &extension, OutputFormatType &type);

    AVTransCoderNapi();
    ~AVTransCoderNapi();

    RetInfo Start();
    RetInfo Pause();
    RetInfo Resume();
    RetInfo Cancel();
    RetInfo Release();

    RetInfo SetInputFile(std::string srcUrl);
    RetInfo SetInputFile(int32_t fd, int64_t offset, int64_t size);
    RetInfo SetOutputFile(int32_t fd);

    int32_t CheckStateMachine(const std::string &opt);
    int32_t CheckRepeatOperation(const std::string &opt);

    void ErrorCallback(int32_t errCode, const std::string &operate, const std::string &add = "");
    void StateCallback(const std::string &state);
    void SetCallbackReference(const std::string &callbackName, std::shared_ptr<AutoRef> ref);
    void CancelCallbackReference(const std::string &callbackName);
    void CancelCallback();

    RetInfo Configure(std::shared_ptr<AVTransCoderConfig> config);
    int32_t GetAudioConfig(std::unique_ptr<AVTransCoderAsyncContext> &asyncCtx, napi_env env, napi_value args);
    int32_t GetVideoConfig(std::unique_ptr<AVTransCoderAsyncContext> &asyncCtx, napi_env env, napi_value args);
    int32_t GetConfig(std::unique_ptr<AVTransCoderAsyncContext> &asyncCtx, napi_env env, napi_value args);

    static thread_local napi_ref constructor_;
    napi_env env_ = nullptr;
    std::shared_ptr<TransCoder> transCoder_ = nullptr;
    std::shared_ptr<TransCoderCallback> transCoderCb_ = nullptr;
    std::map<std::string, std::shared_ptr<AutoRef>> eventCbMap_;
    std::unique_ptr<TaskQueue> taskQue_;
    static std::map<std::string, AvTransCoderTaskqFunc> taskQFuncs_;
    bool hasConfiged_ = false;

    std::string srcUrl_ = "";
    struct AVFileDescriptor srcFd_;
    int32_t dstFd_;
};

struct AVTransCoderAsyncContext : public MediaAsyncContext {
    explicit AVTransCoderAsyncContext(napi_env env) : MediaAsyncContext(env) {}
    ~AVTransCoderAsyncContext() = default;

    void AVTransCoderSignError(int32_t errCode, const std::string &operate,
        const std::string &param, const std::string &add = "");

    AVTransCoderNapi *napi = nullptr;
    std::shared_ptr<AVTransCoderConfig> config_ = nullptr;
    std::string opt_ = "";
    std::shared_ptr<TaskHandler<RetInfo>> task_ = nullptr;
};

class MediaJsAVTransCoderConfig : public MediaJsResult {
public:
    explicit MediaJsAVTransCoderConfig(std::shared_ptr<AVTransCoderConfig> value)
        : value_(value)
    {
    }
    ~MediaJsAVTransCoderConfig() = default;

private:
    std::shared_ptr<AVTransCoderConfig> value_ = nullptr;
};

} // namespace Media
} // namespace OHOS
#endif // AV_RECORDER_NAPI_H