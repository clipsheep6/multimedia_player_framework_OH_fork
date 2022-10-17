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

namespace OHOS {
namespace Media {
namespace AVRecorderState {
const std::string STATE_IDLE = "idle";
const std::string STATE_PREPARED = "prepared";
const std::string STATE_STARTED = "started";
const std::string STATE_PAUSED = "paused";
const std::string STATE_STOPPED = "stopped";
const std::string STATE_RELEASED = "released";
const std::string STATE_ERROR = "error";
};

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
     * on(type: 'stateChanged', callback: (state: AVPlayerState, reason: StateChangeReason) => void): void
     * on(type: 'error', callback: ErrorCallback): void
     */
    static napi_value JsSetEventCallback(napi_env env, napi_callback_info info);
    AVRecorderNapi();
    ~AVRecorderNapi();

    static thread_local napi_ref constructor_;
    napi_env env_ = nullptr;
    napi_ref wrapper_ = nullptr;
    std::shared_ptr<Recorder> recorder_ = nullptr;
    std::shared_ptr<RecorderCallback> recorderCb_ = nullptr;
    std::map<std::string, std::shared_ptr<AutoRef>> evnetCbMap_;
    sptr<Surface> surface_ = nullptr;
    int32_t videoSourceID = -1;
    int32_t audioSourceID = -1;
    std::string currentState_ = VideoRecorderState::STATE_IDLE;
};
} // namespace Media
} // namespace OHOS
#endif // AV_RECORDER_NAPI_H