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

#ifndef AV_SCREEN_CAPTURE_NAPI_H
#define AV_SCREEN_CAPTURE_NAPI_H

#include "screen_capture.h"
#include "screen_capture_controller.h"
#include "media_errors.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "common_napi.h"
#include "task_queue.h"

namespace OHOS {
namespace Media {
namespace AVScreenCapturegOpt {
const std::string INIT = "Init";
const std::string REPORT_USER_CHOICE = "ReportAVScreenCaptureUserChoice";
const std::string START_RECORDING = "StartRecording";
const std::string STOP_RECORDING = "StopRecording";
const std::string SET_MIC_ENABLE = "SetMicrophoneEnable";
const std::string RELEASE = "Release";
}
constexpr int32_t AVSCREENCAPTURE_DEFAULT_AUDIO_BIT_RATE = 96000;
constexpr int32_t AVSCREENCAPTURE_DEFAULT_AUDIO_CHANNELS = 2;
constexpr int32_t AVSCREENCAPTURE_DEFAULT_AUDIO_SAMPLE_RATE = 48000;
constexpr int32_t AVSCREENCAPTURE_DEFAULT_VIDEO_FRAME_RATE = 60;
constexpr int32_t AVSCREENCAPTURE_DEFAULT_VIDEO_BIT_RATE = 10000000;
constexpr int32_t AVSCREENCAPTURE_DEFAULT_FRAME_HEIGHT = 0;
constexpr int32_t AVSCREENCAPTURE_DEFAULT_FRAME_WIDTH = 0;
const std::string AVSCREENCAPTURE_DEFAULT_FILE_FORMAT = "mp4";

namespace AVScreenCaptureEvent {
const std::string EVENT_STATE_CHANGE = "stateChange";
const std::string EVENT_ERROR = "error";
}

enum AVScreenCaptureRecorderPreset: int32_t {
    SCREEN_RECORD_PRESET_H264_AAC_MP4 = 0,
    SCREEN_RECORD_PRESET_H265_AAC_MP4 = 1
};

struct AVScreenCaptureAsyncContext;

using RetInfo = std::pair<int32_t, std::string>;

class AVScreenCaptureNapi {
public:
    __attribute__((visibility("default"))) static napi_value Init(napi_env env, napi_value exports);

    using AvScreenCaptureTaskqFunc = RetInfo (AVScreenCaptureNapi::*)();
private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    /**
     * createAVScreenCaptureRecorder(): Promise<AVScreenCaptureRecorder>
     */
    static napi_value JsCreateAVScreenRecorder(napi_env env, napi_callback_info info);
    /**
     * reportAVScreenCaptureUserChoice(sessionId: number, choice: string): Promise<void>
     */
    static napi_value JsReportAVScreenCaptureUserChoice(napi_env env, napi_callback_info info);
    /**
     * init(config: AVScreenCaptureRecordConfig): Promise<void>
     */
    static napi_value JsInit(napi_env env, napi_callback_info info);
    /**
     * startRecording(): Promise<void>
     */
    static napi_value JsStartRecording(napi_env env, napi_callback_info info);
    /**
     * stopRecording(): Promise<void>
     */
    static napi_value JsStopRecording(napi_env env, napi_callback_info info);
    /**
     * setMicrophoneEnabled(enable: boolean): Promise<void>
     */
    static napi_value JsSetMicrophoneEnabled(napi_env env, napi_callback_info info);
    /**
     * release(): Promise<void>
     */
    static napi_value JsRelease(napi_env env, napi_callback_info info);
    /**
     * on(type: 'stateChange', callback: Callback<AVScreenCaptureOnInfoType>): void
     * on(type: 'error', callback: ErrorCallback): void
     */
    static napi_value JsSetEventCallback(napi_env env, napi_callback_info info);
    /**
     * off(type: 'stateChange', callback?: Callback<AVScreenCaptureOnInfoType>): void
     * off(type: 'error', callback?: ErrorCallback): void
     */
    static napi_value JsCancelEventCallback(napi_env env, napi_callback_info info);

    static std::shared_ptr<TaskHandler<RetInfo>> GetPromiseTask(AVScreenCaptureNapi *avnapi, const std::string &opt);
    static napi_value ExecuteByPromise(napi_env env, napi_callback_info info, const std::string &opt);
    static AVScreenCaptureNapi* GetJsInstanceAndArgs(napi_env env, napi_callback_info info,
        size_t &argCount, napi_value *args);
    static std::shared_ptr<TaskHandler<RetInfo>> GetInitTask(
        const std::unique_ptr<AVScreenCaptureAsyncContext> &asyncCtx);
    static std::shared_ptr<TaskHandler<RetInfo>> GetSetMicrophoneEnableTask(
        const std::unique_ptr<AVScreenCaptureAsyncContext> &asyncCtx, const bool enable);
    static int32_t GetPropertyInt32(napi_env env, napi_value configObj, const std::string &type, int32_t &result);
    static VideoCodecFormat GetVideoCodecFormat(const int32_t &preset);
    static int32_t GetAudioInfo(std::unique_ptr<AVScreenCaptureAsyncContext> &asyncCtx, napi_env env, napi_value args);
    static int32_t GetVideoInfo(std::unique_ptr<AVScreenCaptureAsyncContext> &asyncCtx, napi_env env, napi_value args);
    static void AsyncJsReportAVScreenCaptureUserChoice(napi_env env, void *data);

    AVScreenCaptureNapi();
    ~AVScreenCaptureNapi();

    RetInfo StartRecording();
    RetInfo StopRecording();
    RetInfo Release();

    void ErrorCallback(int32_t errCode, const std::string &operate, const std::string &add = "");
    void StateCallback(const AVScreenCaptureStateCode &stateCode);
    void SetCallbackReference(const std::string &callbackName, std::shared_ptr<AutoRef> ref);
    void CancelCallbackReference(const std::string &callbackName);
    void CancelCallback();

    int32_t GetConfig(std::unique_ptr<AVScreenCaptureAsyncContext> &asyncCtx, napi_env env, napi_value args);
    int32_t GetRecorderInfo(std::unique_ptr<AVScreenCaptureAsyncContext> &asyncCtx, napi_env env, napi_value args);

    static thread_local napi_ref constructor_;
    napi_env env_ = nullptr;
    std::shared_ptr<ScreenCapture> screenCapture_ = nullptr;
    std::shared_ptr<ScreenCaptureCallBack> screenCaptureCb_ = nullptr;
    std::map<std::string, std::shared_ptr<AutoRef>> eventCbMap_;
    std::unique_ptr<TaskQueue> taskQue_;
    static std::map<std::string, AvScreenCaptureTaskqFunc> taskQFuncs_;
};

struct AVScreenCaptureAsyncContext : public MediaAsyncContext {
    explicit AVScreenCaptureAsyncContext(napi_env env) : MediaAsyncContext(env) {}
    ~AVScreenCaptureAsyncContext() = default;

    void AVScreenCaptureSignError(int32_t errCode, const std::string &operate,
        const std::string &param, const std::string &add = "");

    AVScreenCaptureNapi *napi = nullptr;
    AVScreenCaptureConfig config_;
    std::shared_ptr<ScreenCaptureController> controller_ = nullptr;
    std::string opt_ = "";
    std::shared_ptr<TaskHandler<RetInfo>> task_ = nullptr;
};

} // namespace Media
} // namespace OHOS
#endif