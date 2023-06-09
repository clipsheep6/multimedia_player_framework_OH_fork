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

#ifndef NATIVE_SCREEN_CAPTURE_H
#define NATIVE_SCREEN_CAPTURE_H

#include <stdint.h>
#include <stdio.h>
#include "native_screen_capture_errors.h"
#include "screen_capture.h"

#include "surface_buffer_impl.h"
#include "native_screen_capture_magic.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*OH_ScreenCaptureOnError)(OH_ScreenCapture *capture, int32_t errorCode);

typedef void (*OH_ScreenCaptureOnAudioBufferAvailable)(OH_ScreenCapture *capture, bool isReady,
                OHOS::Media::AudioCapSourceType type);

typedef void (*OH_ScreenCaptureOnVideoBufferAvailable)(OH_ScreenCapture *capture, bool isReady);

typedef struct OH_ScreenCaptureCallback {
    OH_ScreenCaptureOnError onError;
    OH_ScreenCaptureOnAudioBufferAvailable onAudioBufferAvailable;
    OH_ScreenCaptureOnVideoBufferAvailable onVideoBufferAvailable;
} OH_ScreenCaptureCallback;

struct OH_ScreenCapture *OH_ScreenCapture_Create();
OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_Init(struct OH_ScreenCapture *capture,
    OHOS::Media::AVScreenCaptureConfig config);
OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_StartScreenCapture(struct OH_ScreenCapture *capture,
    OHOS::Media::DataType type);
OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_StopScreenCapture(struct OH_ScreenCapture *capture);
OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_AcquireAudioBuffer(struct OH_ScreenCapture *capture,
    std::shared_ptr<OHOS::Media::AudioBuffer> &audiobuffer, OHOS::Media::AudioCapSourceType type);
OH_NativeBuffer* OH_ScreenCapture_AcquireVideoBuffer(struct OH_ScreenCapture *capture,
    int32_t &fence, int64_t &timestamp, OHOS::Rect &damage);
OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_ReleaseAudioBuffer(struct OH_ScreenCapture *capture,
    OHOS::Media::AudioCapSourceType type);
OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_ReleaseVideoBuffer(struct OH_ScreenCapture *capture);
OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_SetCallback(struct OH_ScreenCapture *capture,
    struct OH_ScreenCaptureCallback callback);
OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_Realease(struct OH_ScreenCapture *capture);
OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_SetMicrophoneEnable(struct OH_ScreenCapture *capture, bool isMicrophone);

#ifdef __cplusplus
}
#endif

#endif // NATIVE_SCREEN_CAPTURE_H