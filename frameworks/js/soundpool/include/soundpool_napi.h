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

#ifndef SOUNDPOOL_NAPI_H
#define SOUNDPOOL_NAPI_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "common_napi.h"
#include "media_dfx.h"
#include "soundpool_callback_napi.h"

namespace OHOS {
namespace Media {
using RetInfo = std::pair<int32_t, std::string>;
const int PARAM0 = 0;
const int PARAM1 = 1;
const int PARAM2 = 2;
const int PARAM3 = 3;
const int PARAM4 = 4;
const int MAX_PARAM = 5;

struct SoundPoolAsyncContext;

class SoundPoolNapi {
public:
    __attribute__((visibility("default"))) static napi_value Init(napi_env env, napi_value exports);

private:
    SoundPoolNapi() {}
    ~SoundPoolNapi();
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalize);

    /**
     * createSoundPool(maxStreams: number, audioRenderInfo: audio.AudioRendererInfo,
     *     callback: AsyncCallback<SoundPool>): void
     *
     * createSoundPool(maxStreams: number, audioRenderInfo: audio.AudioRendererInfo): Promise<SoundPool>
     */
    static napi_value JsCreateSoundPool(napi_env env, napi_callback_info info);

    /**
     * load(uri: string, callback: AsyncCallback<number>): void
     * load(uri: string): Promise<number>
     *
     * or
     *
     * load(fd: number, offset: number, length: number, callback: AsyncCallback<number>): void
     * load(fd: number, offset: number, length: number): Promise<number>
     */
    static napi_value JsLoad(napi_env env, napi_callback_info info);

    /**
     * play(soundID: number, params?: PlayParameters): Promise<number>
     * play(soundID: number, callback: AsyncCallback<number>): void
     * play(soundID: number, params: PlayParameters, callback: AsyncCallback<number>): void
     */
    static napi_value JsPlay(napi_env env, napi_callback_info info);

    /**
     * stop(streamID: number, callback: AsyncCallback<void>): void
     * stop(streamID: number): Promise<void>
     */
    static napi_value JsStop(napi_env env, napi_callback_info info);

    /**
     * setLoop(streamID: number, loop: number, callback: AsyncCallback<void>): void
     * setLoop(streamID: number, loop: number): Promise<void>
     */
    static napi_value JsSetLoop(napi_env env, napi_callback_info info);

    /**
     * setPriority(streamID: number, priority: number, callback: AsyncCallback<void>): void
     * setPriority(streamID: number, priority: number): Promise<void>
     */
    static napi_value JsSetPriority(napi_env env, napi_callback_info info);

    /**
     * setRate(streamID: number, rate: audio.AudioRendererRate, callback: AsyncCallback<void>): void
     * setRate(streamID: number, rate: audio.AudioRendererRate): Promise<void>
     */
    static napi_value JsSetRate(napi_env env, napi_callback_info info);

    /**
     * setVolume(streamID: number, leftVolume: number, rightVolume: number, callback: AsyncCallback<void>): void
     * setVolume(streamID: number, leftVolume: number, rightVolume: number): Promise<void>
     */
    static napi_value JsSetVolume(napi_env env, napi_callback_info info);

    /**
     * unload(soundID: number, callback: AsyncCallback<void>): void
     * unload(soundID: number): Promise<void>
     */
    static napi_value JsUnload(napi_env env, napi_callback_info info);

    /**
     * release(callback: AsyncCallback<void>): void
     * release(): Promise<void>
     */
    static napi_value JsRelease(napi_env env, napi_callback_info info);

    /**
     *
     * on(type: 'loadCompleted', callback: Callback<number>): void
     * off(type: 'loadCompleted'): void
     *
     * on(type: 'playFinished', callback: Callback<void>): void
     * off(type: 'playFinished'): void
     *
     * on(type: 'error', callback: ErrorCallback): void
     * off(type: 'error'): void
     *
     */
    static napi_value JsSetOnCallback(napi_env env, napi_callback_info info);
    static napi_value JsClearOnCallback(napi_env env, napi_callback_info info);
    // Use to get soundpool instance.
    static SoundPoolNapi* GetJsInstanceAndArgs(napi_env env, napi_callback_info info,
        size_t &argCount, napi_value *args);
    static napi_status GetJsInstanceWithParameter(napi_env env, napi_value *argv);
    int32_t ParserLoadOptionFromJs(std::unique_ptr<SoundPoolAsyncContext> &asyncCtx,
        napi_env env, napi_value *argv, size_t argCount);
    int32_t ParserPlayOptionFromJs(std::unique_ptr<SoundPoolAsyncContext> &asyncCtx,
        napi_env env, napi_value *argv, size_t argCount);
    int32_t ParserRateOptionFromJs(std::unique_ptr<SoundPoolAsyncContext> &asyncCtx,
    napi_env env, napi_value *argv);
    int32_t ParserVolumeOptionFromJs(std::unique_ptr<SoundPoolAsyncContext> &asyncCtx,
    napi_env env, napi_value *argv);
    bool GetPropertyBool(napi_env env, napi_value configObj, const std::string &type, bool &result);

    void ErrorCallback(int32_t errCode, const std::string &operate, const std::string &add = "");
    void SetCallbackReference(const std::string &callbackName, std::shared_ptr<AutoRef> ref);
    void CancelCallbackReference(const std::string &callbackName);
    void CancelCallback();

    static thread_local napi_ref constructor_;
    static int32_t maxStreams;
    static AudioStandard::AudioRendererInfo rendererInfo;

    napi_env env_ = nullptr;
    std::shared_ptr<ISoundPool> soundPool_;
    std::shared_ptr<ISoundPoolCallback> callbackNapi_;
    std::map<std::string, std::shared_ptr<AutoRef>> eventCbMap_;
};

struct SoundPoolAsyncContext : public MediaAsyncContext {
    explicit SoundPoolAsyncContext(napi_env env) : MediaAsyncContext(env) {}
    ~SoundPoolAsyncContext() = default;
    void SoundPoolAsyncSignError(int32_t errCode, const std::string &operate,
        const std::string &param, const std::string &add = "");
    SoundPoolNapi *napi = nullptr;
    std::string url_ = "";
    int32_t fd_ = 0;
    int64_t offset_ = 0;
    int64_t length_ = 0;
    int32_t soundId_ = 0;
    PlayParams playParameters_;
    int32_t streamId_ = 0;
    int32_t loop_ = 0;
    int32_t priority_ = 0;
    float leftVolume_ = 0.0f;
    float rightVolume_ = 0.0f;
    AudioStandard::AudioRendererRate renderRate_ = AudioStandard::AudioRendererRate::RENDER_RATE_NORMAL;
};
} // namespace Media
} // namespace OHOS
#endif // SOUNDPOOL_NAPI_H