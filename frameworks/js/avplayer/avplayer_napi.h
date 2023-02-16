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

#ifndef AV_PLAYER_NAPI_H
#define AV_PLAYER_NAPI_H

#include "player.h"
#include "media_errors.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "avplayer_callback.h"
#include "common_napi.h"
#include "audio_info.h"
#include "task_queue.h"

namespace OHOS {
namespace Media {
namespace AVPlayerState {
const std::string STATE_IDLE = "idle";
const std::string STATE_INITIALIZED = "initialized";
const std::string STATE_PREPARED = "prepared";
const std::string STATE_PLAYING = "playing";
const std::string STATE_PAUSED = "paused";
const std::string STATE_STOPPED = "stopped";
const std::string STATE_RELEASED = "released";
const std::string STATE_ERROR = "error";
const std::string STATE_COMPLETED = "completed";
}

namespace AVPlayerEvent {
const std::string EVENT_STATE_CHANGE = "stateChange";
const std::string EVENT_VOLUME_CHANGE = "volumeChange";
const std::string EVENT_END_OF_STREAM = "endOfStream";
const std::string EVENT_SEEK_DONE = "seekDone";
const std::string EVENT_SPEED_DONE = "speedDone";
const std::string EVENT_BITRATE_DONE = "bitrateDone";
const std::string EVENT_TIME_UPDATE = "timeUpdate";
const std::string EVENT_DURATION_UPDATE = "durationUpdate";
const std::string EVENT_BUFFERING_UPDATE = "bufferingUpdate";
const std::string EVENT_START_RENDER_FRAME = "startRenderFrame";
const std::string EVENT_VIDEO_SIZE_CHANGE = "videoSizeChange";
const std::string EVENT_AUDIO_INTERRUPT = "audioInterrupt";
const std::string EVENT_AVAILABLE_BITRATES = "availableBitrates";
const std::string EVENT_ERROR = "error";
}

using TaskRet = std::pair<int32_t, std::string>;

class AVPlayerNapi : public AVPlayerNotify {
public:
    __attribute__((visibility("default"))) static napi_value Init(napi_env env, napi_value exports);

private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);
    /**
     * createAVPlayer(callback: AsyncCallback<VideoPlayer>): void
     * createAVPlayer(): Promise<VideoPlayer>
     */
    static napi_value JsCreateAVPlayer(napi_env env, napi_callback_info info);
    /**
     * prepare(callback: AsyncCallback<void>): void
     * prepare(): Promise<void>
     */
    static napi_value JsPrepare(napi_env env, napi_callback_info info);
    /**
     * play(callback: AsyncCallback<void>): void
     * play(): Promise<void>
     */
    static napi_value JsPlay(napi_env env, napi_callback_info info);
    /**
     * pause(callback: AsyncCallback<void>): void
     * pause(): Promise<void>
     */
    static napi_value JsPause(napi_env env, napi_callback_info info);
    /**
     * stop(callback: AsyncCallback<void>): void
     * stop(): Promise<void>
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
     * seek(timeMs: number, mode?:SeekMode): void
     */
    static napi_value JsSeek(napi_env env, napi_callback_info info);
    /**
     * setSpeed(speed: number): void
     */
    static napi_value JsSetSpeed(napi_env env, napi_callback_info info);
    /**
     * setVolume(vol: number): void
     */
    static napi_value JsSetVolume(napi_env env, napi_callback_info info);
    /**
     * selectBitrate(bitRate: number): void
     */
    static napi_value JsSelectBitrate(napi_env env, napi_callback_info info);
    /**
     * url: string
     */
    static napi_value JsSetUrl(napi_env env, napi_callback_info info);
    static napi_value JsGetUrl(napi_env env, napi_callback_info info);
    /**
     * fdSrc: AVFileDescriptor
     */
    static napi_value JsGetAVFileDescriptor(napi_env env, napi_callback_info info);
    static napi_value JsSetAVFileDescriptor(napi_env env, napi_callback_info info);
    /**
     * surfaceId?: string
     */
    static napi_value JsSetSurfaceID(napi_env env, napi_callback_info info);
    static napi_value JsGetSurfaceID(napi_env env, napi_callback_info info);
    /**
     * loop: boolenan
     */
    static napi_value JsSetLoop(napi_env env, napi_callback_info info);
    static napi_value JsGetLoop(napi_env env, napi_callback_info info);
    /**
     * videoScaleType?: VideoScaleType
     */
    static napi_value JsSetVideoScaleType(napi_env env, napi_callback_info info);
    static napi_value JsGetVideoScaleType(napi_env env, napi_callback_info info);
    /**
     * audioInterruptMode?: audio.AudioInterruptMode
     */
    static napi_value JsGetAudioInterruptMode(napi_env env, napi_callback_info info);
    static napi_value JsSetAudioInterruptMode(napi_env env, napi_callback_info info);
    /**
     * readonly currentTime: number
     */
    static napi_value JsGetCurrentTime(napi_env env, napi_callback_info info);
    /**
     * readonly duration: number
     */
    static napi_value JsGetDuration(napi_env env, napi_callback_info info);
    /**
     * readonly state: AVPlayState
     */
    static napi_value JsGetState(napi_env env, napi_callback_info info);
    /**
     * readonly width: number
     */
    static napi_value JsGetWidth(napi_env env, napi_callback_info info);
    /**
     * readonly height: number
     */
    static napi_value JsGetHeight(napi_env env, napi_callback_info info);
    /**
     * getTrackDescription(callback:AsyncCallback<Array<MediaDescription>>): void
     * getTrackDescription(): Promise<Array<MediaDescription>>
     */
    static napi_value JsGetTrackDescription(napi_env env, napi_callback_info info);
    /**
    * on(type: 'stateChange', callback: (state: AVPlayerState, reason: StateChangeReason) => void): void;
    * off(type: 'stateChange'): void;
    * on(type: 'volumeChange', callback: Callback<number>): void;
    * off(type: 'volumeChange'): void;
    * on(type: 'endOfStream', callback: Callback<void>): void;
    * off(type: 'endOfStream'): void;
    * on(type: 'seekDone', callback: Callback<number>): void;
    * off(type: 'seekDone'): void;
    * on(type: 'speedDone', callback: Callback<number>): void;
    * off(type: 'speedDone'): void;
    * on(type: 'bitrateDone', callback: Callback<number>): void;
    * off(type: 'bitrateDone'): void;
    * on(type: 'timeUpdate', callback: Callback<number>): void;
    * off(type: 'timeUpdate'): void;
    * on(type: 'durationUpdate', callback: Callback<number>): void;
    * off(type: 'durationUpdate'): void;
    * on(type: 'bufferingUpdate', callback: (infoType: BufferingInfoType, value: number) => void): void;
    * off(type: 'bufferingUpdate'): void;
    * on(type: 'startRenderFrame', callback: Callback<void>): void;
    * off(type: 'startRenderFrame'): void;
    * on(type: 'videoSizeChange', callback: (width: number, height: number) => void): void;
    * off(type: 'videoSizeChange'): void;
    * on(type: 'audioInterrupt', callback: (info: audio.InterruptEvent) => void): void;
    * off(type: 'audioInterrupt'): void;
    * on(type: 'availableBitrates', callback: (bitrates: Array<number>) => void): void;
    * off(type: 'availableBitrates'): void;
    * on(type: 'error', callback: ErrorCallback): void;
    * off(type: 'error'): void;
    */
    static napi_value JsSetOnCallback(napi_env env, napi_callback_info info);
    static napi_value JsClearOnCallback(napi_env env, napi_callback_info info);

    static AVPlayerNapi* GetJsInstance(napi_env env, napi_callback_info info);
    static AVPlayerNapi* GetJsInstanceWithParameter(napi_env env, napi_callback_info info,
        size_t &argc, napi_value *argv);
    AVPlayerNapi();
    ~AVPlayerNapi() override;
    void SaveCallbackReference(const std::string &callbackName, std::shared_ptr<AutoRef> ref);
    void ClearCallbackReference();
    void ClearCallbackReference(const std::string &callbackName);
    void StartListenCurrentResource();
    void PauseListenCurrentResource();
    void OnErrorCb(MediaServiceExtErrCodeAPI9 errorCode, const std::string &errorMsg);
    void SetSource(std::string url);
    void SetSurface(const std::string &surfaceStr);
    void ResetUserParameters();

    std::shared_ptr<TaskHandler<TaskRet>> PrepareTask();
    std::shared_ptr<TaskHandler<TaskRet>> PlayTask();
    std::shared_ptr<TaskHandler<TaskRet>> PauseTask();
    std::shared_ptr<TaskHandler<TaskRet>> StopTask();
    std::shared_ptr<TaskHandler<TaskRet>> ResetTask();
    std::shared_ptr<TaskHandler<TaskRet>> ReleaseTask();
    std::string GetCurrentState();
    bool IsControllable();

    void NotifyDuration(int32_t duration) override;
    void NotifyPosition(int32_t position) override;
    void NotifyState(PlayerStates state) override;
    void NotifyVideoSize(int32_t width, int32_t height) override;

    struct AVPlayerContext : public MediaAsyncContext {
        explicit AVPlayerContext(napi_env env) : MediaAsyncContext(env) {}
        ~AVPlayerContext() = default;
        void CheckTaskResult()
        {
            if (asyncTask != nullptr) {
                auto result = asyncTask->GetResult();
                if (result.HasResult() && result.Value().first != MSERR_EXT_API9_OK) {
                    SignError(result.Value().first, result.Value().second);
                }
            }
        }
        std::shared_ptr<TaskHandler<TaskRet>> asyncTask = nullptr;
        AVPlayerNapi *napi = nullptr;
    };
    static thread_local napi_ref constructor_;
    napi_env env_ = nullptr;
    std::shared_ptr<Player> player_ = nullptr;
    std::shared_ptr<AVPlayerCallback> playerCb_ = nullptr;
    std::atomic<bool> isReleased_ = false;
    std::string url_ = "";
    struct AVFileDescriptor fileDescriptor_;
    std::string surface_ = "";
    bool loop_ = false;
    int32_t videoScaleType_ = 0;
    std::vector<Format> trackInfoVec_;
    OHOS::AudioStandard::InterruptMode interruptMode_ = AudioStandard::InterruptMode::SHARE_MODE;
    std::unique_ptr<TaskQueue> taskQue_;
    std::mutex mutex_;
    std::mutex taskMutex_;
    std::map<std::string, std::shared_ptr<AutoRef>> refMap_;
    PlayerStates state_ = PLAYER_IDLE;
    std::condition_variable preparingCond_;
    std::condition_variable stateChangeCond_;
    std::condition_variable resettingCond_;
    int32_t width_ = 0;
    int32_t height_ = 0;
    int32_t position_ = -1;
    int32_t duration_ = -1;
};
} // namespace Media
} // namespace OHOS
#endif // AV_PLAYER_NAPI_H