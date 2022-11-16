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
#include "common_napi.h"
#include "audio_info.h"
#include "task_queue.h"

namespace OHOS {
namespace Media {
/* type AVPlayerState = 'idle' | 'prepared' | 'playing' | 'paused' | 'stopped' | 'released' | 'error' */
namespace AVPlayerState {
const std::string STATE_IDLE = "idle";
const std::string STATE_PREPARED = "prepared";
const std::string STATE_PLAYING = "playing";
const std::string STATE_PAUSED = "paused";
const std::string STATE_STOPPED = "stopped";
const std::string STATE_RELEASED = "released";
const std::string STATE_ERROR = "error";
}

/**
 * on(type: 'stateChange', callback: (state: AVPlayerState, reason: StateChangeReason) => void): void;
 * on(type: 'volumeChange', callback: Callback<number>): void;
 * on(type: 'playComplete', callback: Callback<void>): void;
 * on(type: 'seekDone', callback: Callback<number>): void;
 * on(type: 'speedDone', callback: Callback<number>): void;
 * on(type: 'bitrateDone', callback: Callback<number>): void;
 * on(type: 'timeUpdate', callback: Callback<number>): void;
 * on(type: 'bufferingUpdate', callback: (infoType: BufferingInfoType, value: number) => void): void;
 * on(type: 'startRenderFrame', callback: Callback<void>): void;
 * on(type: 'videoSizeChange', callback: (width: number, height: number) => void): void;
 * on(type: 'audioInterrupt', callback: (info: audio.InterruptEvent) => void): void;
 * on(type: 'availableBitrates', callback: (bitrates: Array<number>) => void): void;
 * on(type: 'error', callback: ErrorCallback): void;
 */
namespace AVPlayerEvent {
const std::string EVENT_STATE_CHANGE = "stateChange";
const std::string EVENT_VOLUME_CHANGE = "volumeChange";
const std::string EVENT_PLAY_COMPLETE = "playComplete";
const std::string EVENT_SEEK_DONE = "seekDone";
const std::string EVENT_SPEED_DONE = "speedDone";
const std::string EVENT_BITRATE_DONE = "bitrateDone";
const std::string EVENT_TIME_UPDATE = "timeUpdate";
const std::string EVENT_BUFFERING_UPDATE = "bufferingUpdate";
const std::string EVENT_START_RENDER_FRAME = "startRenderFrame";
const std::string EVENT_VIDEO_SIZE_CHANGE = "videoSizeChange";
const std::string EVENT_AUDIO_INTERRUPT = "audioInterrupt";
const std::string EVENT_AVAILABLE_BITRATES = "availableBitrates";
const std::string EVENT_ERROR = "error";
}

namespace AVPlayerContextName {
const std::string CONTEXT_NAME_PREPARE = "prepare";
const std::string CONTEXT_NAME_RESET = "reset";
const std::string CONTEXT_NAME_RELEASE = "release";
}

struct AVPlayerFileDescriptor {
    int32_t fd = 0;
    int64_t offset = 0;
    int64_t length = -1;
};

class MediaNapiCall {
public:
    struct EventBase {
        std::weak_ptr<AutoRef> callback;
        std::string callbackName = "unknown";
    };

    struct EventError : public EventBase {
        std::string errorMsg = "unknown";
        MediaServiceExtErrCodeAPI9 errorCode = MSERR_EXT_API9_UNSUPPORT_FORMAT;
    };

    struct EventIntVec : public EventBase {
        std::vector<int32_t> valueVec;
    };

    struct EventDoubleVec : public EventBase {
        std::vector<double> valueVec;
    };

    struct EventPropertyInt : public EventBase {
        std::map<std::string, int32_t> valueVec;
    };

    struct EventPromise {
        std::shared_ptr<struct MediaAsyncContext> context = nullptr;
    };

    static void CallError(napi_env env, MediaNapiCall::EventError *event);
    static void CallSignle(napi_env env, MediaNapiCall::EventBase *event);
    static void CallIntVec(napi_env env, MediaNapiCall::EventIntVec *event);
    static void CallDoubleVec(napi_env env, MediaNapiCall::EventDoubleVec *event);
    static void CallIntArray(napi_env env, MediaNapiCall::EventIntVec *event);
    static void CallPropertyInt(napi_env env, MediaNapiCall::EventPropertyInt *event);
};

class AVPlayerNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);

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
     * play(): void
     */
    static napi_value JsPlay(napi_env env, napi_callback_info info);
    /**
     * pause(): void
     */
    static napi_value JsPause(napi_env env, napi_callback_info info);
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
     * SelectBitrate(bitRate: number): void
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
     * on(type: 'volumeChange', callback: Callback<number>): void;
     * on(type: 'playComplete', callback: Callback<void>): void;
     * on(type: 'seekDone', callback: Callback<number>): void;
     * on(type: 'speedDone', callback: Callback<number>): void;
     * on(type: 'bitrateDone', callback: Callback<number>): void;
     * on(type: 'timeUpdate', callback: Callback<number>): void;
     * on(type: 'bufferingUpdate', callback: (infoType: BufferingInfoType, value: number) => void): void;
     * on(type: 'startRenderFrame', callback: Callback<void>): void;
     * on(type: 'videoSizeChange', callback: (width: number, height: number) => void): void;
     * on(type: 'audioInterrupt', callback: (info: audio.InterruptEvent) => void): void;
     * on(type: 'availableBitrates', callback: (bitrates: Array<number>) => void): void;
     * on(type: 'error', callback: ErrorCallback): void;
    */
    static napi_value JsSetEventCallback(napi_env env, napi_callback_info info);
    AVPlayerNapi();
    ~AVPlayerNapi();
    void SaveCallbackReference(const std::string &callbackName, std::shared_ptr<AutoRef> ref);
    void OnErrorCb(MediaServiceExtErrCodeAPI9 errorCode, const std::string &errorMsg);
    void SavePromiseCallback(std::shared_ptr<MediaAsyncContext> ctx);
    void CompletePromiseCallbackAll();

    static thread_local napi_ref constructor_;
    napi_env env_ = nullptr;
    napi_ref wrapper_ = nullptr;
    std::shared_ptr<Player> player_ = nullptr;
    std::shared_ptr<PlayerCallback> playerCb_ = nullptr;
    // std::string url_ = "";
    // struct AVPlayerFileDescriptor playerFd_;
    // std::vector<Format> audioTrackInfoVec_;
    // std::vector<Format> videoTrackInfoVec_;
    // int32_t videoScaleType_ = 0;
    // OHOS::AudioStandard::InterruptMode interruptMode_ = AudioStandard::InterruptMode::SHARE_MODE;
    std::unique_ptr<TaskQueue> taskQue_;
    std::mutex mutex_;
    std::map<std::string, std::shared_ptr<AutoRef>> refMap_;
};
} // namespace Media
} // namespace OHOS
#endif // AV_PLAYER_NAPI_H