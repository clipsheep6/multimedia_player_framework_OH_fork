/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
#include <native_buffer/native_buffer.h>
#include <unistd.h>
#include <string>
#include <thread>
#include <cstdio>
#include <fcntl.h>
#include "napi/native_api.h"
#include "iostream"
#include "hilog/log.h"
#include "/multimedia/player_framework/native_avscreen_capture.h"
#include "/multimedia/player_framework/native_avscreen_capture_base.h"
#include "multimedia/player_framework/native_avscreen_capture_errors.h"

using namespace std;
#undef LOG_DOMAIN
#undef LOG_TAG
#define LOG_DOMAIN 0x3200
#define LOG_TAG "AVScreenCaptureDemo"

static struct OH_AVScreenCapture *g_capture;
struct OH_AVScreenCaptureCallback callback_;
static FILE *g_micFile = nullptr;
static FILE *g_vFile = nullptr;
static FILE *g_innerFile = nullptr;
static char g_filename[100] = {0};
static bool g_capStartFlag = false;
static int32_t g_aIndex = 0;
static int32_t g_innerIndex = 0;
static int32_t g_vIndex = 0;
static auto g_startTime = 0;
static auto g_endtTime = 0;

struct CaptureConfig {
    int32_t micSampleRate;
    int32_t micChannel;
    int32_t innerSampleRate;
    int32_t innerChannel;
    int32_t width;
    int32_t height;
    bool isMic;
};

OH_AVScreenCapture *AVScreenCaptureCreate()
{
    g_capture = OH_AVScreenCapture_Create();
    if (g_capture == nullptr) {
        OH_LOG_ERROR(LOG_APP, "AVScreenCapture_Create capture_ is null.");
    }
    return g_capture;
}
void SetConfig(OH_AVScreenCaptureConfig &config)
{
    char name[30] = "fd://11";
    OH_AudioCaptureInfo miccapinfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = OH_MIC
    };
    OH_VideoCaptureInfo videocapinfo = {
        .videoFrameWidth = 720,
        .videoFrameHeight = 1280,
        .videoSource = OH_VIDEO_SOURCE_SURFACE_RGBA
    };
    OH_AudioInfo audioinfo = {
        .micCapInfo = miccapinfo,
    };
    OH_VideoInfo videoinfo = {
        .videoCapInfo = videocapinfo
    };
    OH_RecorderInfo recorderinfo = {
        .url = name
    };
    config = {
        .captureMode = OH_CAPTURE_HOME_SCREEN,
        .dataType = OH_ORIGINAL_STREAM,
        .audioInfo = audioinfo,
        .videoInfo = videoinfo,
        .recorderInfo = recorderinfo
    };
}

void OpenFile(string fileName)
{
    int32_t ret = snprintf(g_filename, sizeof(g_filename), "data/storage/el2/base/files/MIC_%s.pcm", fileName.c_str());
    if (ret >= 0) {
        g_micFile = fopen(g_filename, "w+");
        if (g_micFile == nullptr) {
            OH_LOG_ERROR(LOG_APP, "OpenFile g_micFile audio open failed. %{public}s", strerror(errno));
        }
    } else {
        OH_LOG_ERROR(LOG_APP, "OpenFile g_micFile audio snprintf_s failed.");
    }
    ret = snprintf(g_filename, sizeof(g_filename), "data/storage/el2/base/files/INNER_%s.pcm", fileName.c_str());
    if (ret >= 0) {
        g_innerFile = fopen(g_filename, "w+");
        if (g_innerFile == nullptr) {
            OH_LOG_ERROR(LOG_APP, "OpenFile g_innerFile audio open failed. %{public}s", strerror(errno));
        }
    } else {
        OH_LOG_ERROR(LOG_APP, "OpenFile g_innerFile audio snprintf_s failed.");
    }
    ret = snprintf(g_filename, sizeof(g_filename), "data/storage/el2/base/files/VIDEO_%s.pcm", fileName.c_str());
    if (ret >= 0) {
        g_vFile = fopen(g_filename, "w+");
        if (g_vFile == nullptr) {
            OH_LOG_ERROR(LOG_APP, "OpenFile vFile video open failed. %{public}s", strerror(errno));
        }
    } else {
        OH_LOG_ERROR(LOG_APP, "OpenFile vFile video snprintf_s failed.");
    }
}

void CloseFile(void)
{
    if (g_micFile != nullptr) {
        fclose(g_micFile);
        g_micFile = nullptr;
    }
    if (g_innerFile != nullptr) {
        fclose(g_innerFile);
        g_innerFile = nullptr;
    }
    if (g_vFile != nullptr) {
        fclose(g_vFile);
        g_vFile = nullptr;
    }
    if (g_micFile != nullptr || g_innerFile != nullptr || g_vFile != nullptr) {
        OH_LOG_ERROR(LOG_APP, "CloseFile Fail.");
    }
}

static napi_value GetScreenCaptureParam(napi_env env, napi_value captureConfigValue, CaptureConfig *config)
{
    napi_value value = nullptr;
    napi_get_named_property(env, captureConfigValue, "micSampleRate", &value);
    napi_get_value_int32(env, value, &config->micSampleRate);

    napi_get_named_property(env, captureConfigValue, "micChannel", &value);
    napi_get_value_int32(env, value, &config->micChannel);

    napi_get_named_property(env, captureConfigValue, "innerSampleRate", &value);
    napi_get_value_int32(env, value, &config->innerSampleRate);

    napi_get_named_property(env, captureConfigValue, "innerChannel", &value);
    napi_get_value_int32(env, value, &config->innerChannel);

    napi_get_named_property(env, captureConfigValue, "width", &value);
    napi_get_value_int32(env, value, &config->width);

    napi_get_named_property(env, captureConfigValue, "height", &value);
    napi_get_value_int32(env, value, &config->height);

    napi_get_named_property(env, captureConfigValue, "isMic", &value);
    napi_get_value_bool(env, value, &config->isMic);
    OH_LOG_INFO(LOG_APP, "get micSampleRate %{public}d, micChannel %{public}d, innerSampleRate %{public}d, innerChannel"
        "%{public}d, width %{public}d, height %{public}d, isMic %{public}d", config->micSampleRate, config->micChannel,
        config->innerSampleRate, config->innerChannel, config->width, config->height, config->isMic);
    return 0;
}

static void CheckAudioFile(OH_AudioBuffer *audioBuffer, OH_AudioCaptureSourceType type)
{
    if ((g_micFile != nullptr) && (audioBuffer->buf != nullptr) && (type == OH_MIC)) {
        int32_t micsize = fwrite(audioBuffer->buf, 1, audioBuffer->size, g_micFile);
            if (micsize != audioBuffer->size) {
            OH_LOG_ERROR(LOG_APP, "OnAudioBufferAvailable error occurred in fwrite "
                "g_micFile:%{public}s\n", strerror(errno));
        }
        free(audioBuffer->buf);
        audioBuffer->buf = nullptr;
        g_aIndex++;
    } else if ((g_innerFile != nullptr) && (audioBuffer->buf != nullptr) && (type == OH_ALL_PLAYBACK)) {
        int32_t innersize = fwrite(audioBuffer->buf, 1, audioBuffer->size, g_innerFile);
        if (innersize != audioBuffer->size) {
            OH_LOG_ERROR(LOG_APP, "OnAudioBufferAvailable error occurred in fwrite "
                "g_innerFile:%{public}s\n", strerror(errno));
        }
        free(audioBuffer->buf);
        audioBuffer->buf = nullptr;
        g_innerIndex++;
    }
}

static void OnAudioBufferAvailable(OH_AVScreenCapture *capture, bool isReady, OH_AudioCaptureSourceType type)
{
    if (isReady) {
        OH_AudioBuffer *audioBuffer = (OH_AudioBuffer *)malloc(sizeof(OH_AudioBuffer));
        if (audioBuffer == nullptr) {
            OH_LOG_ERROR(LOG_APP, "OnAudioBufferAvailable audio buffer is nullptr");
        }
        if (OH_AVScreenCapture_AcquireAudioBuffer(capture, &audioBuffer, type) == AV_SCREEN_CAPTURE_ERR_OK) {
            if (audioBuffer == nullptr) {
                OH_LOG_ERROR(LOG_APP, "OnAudioBufferAvailable AcquireAudioBuffer failed, audio buffer empty");
            }
            int32_t count = (g_aIndex + g_innerIndex) % 30;
            if (count == 0) {
                OH_LOG_INFO(LOG_APP, "OnAudioBufferAvailable AcquireAudioBuffer, audioBufferLen:%{public}d, "
                    "timestampe:%{public}lld,audioSourceType:%{public}d\n",
                    audioBuffer->size, audioBuffer->timestamp, audioBuffer->type);
            }
            CheckAudioFile(audioBuffer, type);
            free(audioBuffer);
            audioBuffer = nullptr;
        }
        OH_AVScreenCapture_ReleaseAudioBuffer(capture, type);
    } else {
        OH_LOG_ERROR(LOG_APP, "OnAudioBufferAvailable AcquireAudioBuffer failed.\n");
    }
}

static void CheckVideoFile(OH_NativeBuffer *nativebuffer, int32_t length)
{
    if (g_vFile != nullptr) {
        void *buf = nullptr;
        OH_NativeBuffer_Map(nativebuffer, &buf);
        if (buf != nullptr) {
            if (fwrite(buf, 1, length, g_vFile) != length) {
                OH_LOG_ERROR(LOG_APP, "OnVideoBufferAvailable error occurred in fwrite "
                    "g_vFile: %{public}s", strerror(errno));
            }
        }
    }
}

static void OnVideoBufferAvailable(OH_AVScreenCapture *capture, bool isReady)
{
    if (isReady) {
        int32_t fence = 0;
        int64_t timestamp = 0;
        OH_Rect damage;
        OH_NativeBuffer_Config config;
        OH_NativeBuffer *nativebuffer = OH_AVScreenCapture_AcquireVideoBuffer(capture, &fence, &timestamp, &damage);
        if (nativebuffer != nullptr) {
            OH_NativeBuffer_GetConfig(nativebuffer, &config);
            int32_t length = config.height * config.width *4;
            int32_t count = g_vIndex % 30;
            if (count == 0) {
                OH_LOG_INFO(LOG_APP, "OnVideoBufferAvailable AcquireVideoBuffer, videoBufferLen:%{public}d, "
                    "timestampe:%{public}lld\n", length, timestamp);
            }
            CheckVideoFile(nativebuffer, length);
            OH_NativeBuffer_Unreference(nativebuffer);
            OH_AVScreenCapture_ReleaseVideoBuffer(capture);
        } else {
            OH_LOG_ERROR(LOG_APP, "OnVideoBufferAvailable AcquireVideoBuffer failed.");
        }
        g_vIndex++;
    }
}

static void SetScreenCaptureParam(OH_AVScreenCaptureConfig &config, CaptureConfig &configInner)
{
    config.audioInfo.micCapInfo.audioSampleRate = configInner.micSampleRate;
    config.audioInfo.micCapInfo.audioChannels = configInner.micChannel;
    config.audioInfo.innerCapInfo.audioSampleRate = configInner.innerSampleRate;
    config.audioInfo.innerCapInfo.audioChannels = configInner.innerChannel;
    if (configInner.innerSampleRate == 0 || configInner.innerChannel == 0) {
        OH_LOG_ERROR(LOG_APP, "SetScreenCaptureParam the inner param is error, skip set inner");
    } else {
        config.audioInfo.innerCapInfo.audioSource = OH_ALL_PLAYBACK;
    }
    config.videoInfo.videoCapInfo.videoFrameWidth = configInner.width;
    config.videoInfo.videoCapInfo.videoFrameHeight = configInner.height;
    OH_LOG_INFO(LOG_APP, "set micSampleRate %{public}d, micChannel %{public}d, innerSampleRate %{public}d, "
        "innerChannel %{public}d, width %{public}d, height %{public}d, isMic %{public}d",
        configInner.micSampleRate, configInner.micChannel, configInner.innerSampleRate, configInner.innerChannel,
        configInner.width, configInner.height, configInner.isMic);
}

static napi_value StartScreenCapture(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    napi_value res;
    struct OH_AVScreenCapture* capture = AVScreenCaptureCreate();
    OH_AVScreenCaptureConfig config;
    CaptureConfig configInner;
    SetConfig(config);
    GetScreenCaptureParam(env, args[0], &configInner);
    SetScreenCaptureParam(config, configInner);
    string filenameStr = "mic" + to_string(configInner.micSampleRate) + "_inner" +
        to_string(configInner.innerSampleRate) + "_" +to_string(configInner.width) +
        "x" + to_string(configInner.height);
    OpenFile(filenameStr);
    OH_AVSCREEN_CAPTURE_ErrCode result = OH_AVScreenCapture_SetMicrophoneEnabled(capture, configInner.isMic);
    if (result != AV_SCREEN_CAPTURE_ERR_BASE) {
        napi_create_int32(env, result, &res);
        return res;
    }
    OH_AVScreenCaptureCallback callback;
    callback.onAudioBufferAvailable = OnAudioBufferAvailable;
    callback.onVideoBufferAvailable = OnVideoBufferAvailable;
    result = OH_AVScreenCapture_SetCallback(capture, callback);
    if (result != AV_SCREEN_CAPTURE_ERR_BASE) {
        napi_create_int32(env, result, &res);
        return res;
    }
    result = OH_AVScreenCapture_Init(capture, config);
    if (result != AV_SCREEN_CAPTURE_ERR_BASE) {
        napi_create_int32(env, result, &res);
        return res;
    }
    g_startTime = 0;
    g_endtTime = 0;
    g_aIndex = 0;
    g_innerIndex = 0;
    g_vIndex = 0;
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    g_startTime = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    OH_LOG_INFO(LOG_APP, "StartScreenCapture g_startTime:%{public}d", g_startTime);
    result = OH_AVScreenCapture_StartScreenCapture(capture);
    if (result != AV_SCREEN_CAPTURE_ERR_BASE) {
        OH_LOG_ERROR(LOG_APP, "OH_AVScreenCapture_StartScreenCapture result:%{public}d", result);
        napi_create_int32(env, result, &res);
        return res;
    }
    napi_create_int32(env, result, &res);
    return res;
}

static napi_value StopScreenCapture(napi_env env, napi_callback_info info)
{
    OH_AVSCREEN_CAPTURE_ErrCode result = AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT;
    napi_value res;
    if (g_capture == nullptr) {
        OH_LOG_ERROR(LOG_APP, "StopScreenCapture g_capture is null.");
        napi_create_int32(env, result, &res);
        return res;
    }
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    g_endtTime = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    OH_LOG_INFO(LOG_APP, "StopScreenCapture g_endtTime:%{public}d", g_endtTime);
    result = OH_AVScreenCapture_StopScreenCapture(g_capture);
    if (result != AV_SCREEN_CAPTURE_ERR_BASE) {
        OH_LOG_ERROR(LOG_APP, "OH_AVScreenCapture_StopScreenCapture result:%{public}d", result);
        napi_create_int32(env, result, &res);
        return res;
    }
    result = OH_AVScreenCapture_Release(g_capture);
    if (result != AV_SCREEN_CAPTURE_ERR_BASE) {
        OH_LOG_ERROR(LOG_APP, "OH_AVScreenCapture_Release:%{public}d", result);
        napi_create_int32(env, result, &res);
        return res;
    }
    CloseFile();
    napi_create_int32(env, result, &res);
    return res;
}

static napi_value GetFrameRate(napi_env env, napi_callback_info info)
{
    auto toalTime = g_endtTime - g_startTime;
    OH_LOG_INFO(LOG_APP, "GetFrameRate toalTime: %{public}d", toalTime);
    int32_t aFrameRate = g_aIndex / toalTime;
    OH_LOG_INFO(LOG_APP, "GetFrameRate g_aIndex: %{public}d, aFrameRate: %{public}d", g_aIndex, aFrameRate);
    int32_t innerFrameRate = g_innerIndex / toalTime;
    OH_LOG_INFO(LOG_APP, "GetFrameRate g_innerIndex: %{public}d, innerFrameRate: %{public}d",
        g_innerIndex, innerFrameRate);
    int32_t vFrameRate = g_vIndex / toalTime;
    OH_LOG_INFO(LOG_APP, "GetFrameRate g_vIndex: %{public}d, vFrameRate: %{public}d", g_vIndex, vFrameRate);
    napi_value obj;
    napi_create_object(env, &obj);
    
    napi_value keyNapi = nullptr;
    napi_status status = napi_create_string_utf8(env, "audioframerate", NAPI_AUTO_LENGTH, &keyNapi);
    napi_value valueNapi = nullptr;
    status = napi_create_int32(env, aFrameRate, &valueNapi);
    napi_set_property(env, obj, keyNapi, valueNapi);
    
    status = napi_create_string_utf8(env, "innerframerate", NAPI_AUTO_LENGTH, &keyNapi);
    valueNapi = nullptr;
    status = napi_create_int32(env, innerFrameRate, &valueNapi);
    napi_set_property(env, obj, keyNapi, valueNapi);
    
    status = napi_create_string_utf8(env, "videoframerate", NAPI_AUTO_LENGTH, &keyNapi);
    valueNapi = nullptr;
    status = napi_create_int32(env, vFrameRate, &valueNapi);
    napi_set_property(env, obj, keyNapi, valueNapi);
    return obj;
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        { "startScreenCapture", nullptr, StartScreenCapture, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "stopScreenCapture", nullptr, StopScreenCapture, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "getFrameRate", nullptr, GetFrameRate, nullptr, nullptr, nullptr, napi_default, nullptr }
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
EXTERN_C_END

static napi_module demoModule = {
    .nm_version =1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "screencapturer",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void)
{
    napi_module_register(&demoModule);
}
