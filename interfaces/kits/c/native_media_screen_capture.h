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
#include "native_screen_capture_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a screen capture
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 * @return Returns a pointer to an OH_ScreenCapture instance
 * @since 10
 * @version 1.0
 */
struct OH_ScreenCapture *OH_ScreenCapture_Create(void);

/**
 * @brief To init the screen capture, typically, you need to configure the description information of the audio
 * and video, which can be extracted from the container. This interface must be called before StartScreenCapture
 * called.
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 * @param capture Pointer to an OH_ScreenCapture instance
 * @param config Information describing the audio and video config
 * @return Returns SCREEN_CAPTURE_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_SCREEN_CAPTURE_ErrCode}
 * @since 10
 * @version 1.0
 */
OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_Init(struct OH_ScreenCapture *capture,
    OH_AVScreenCaptureConfig config);

/**
 * @brief Start the screen capture
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 * @param capture Pointer to an OH_ScreenCapture instance
 * @param type Information describing the data type of the capture
 * @return Returns SCREEN_CAPTURE_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_SCREEN_CAPTURE_ErrCode}
 * @since 10
 * @version 1.0
 */
OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_StartScreenCapture(struct OH_ScreenCapture *capture,
    OH_DataType type);

/**
 * @brief Stop the screen capture
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 * @param capture Pointer to an OH_ScreenCapture instance
 * @return Returns SCREEN_CAPTURE_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_SCREEN_CAPTURE_ErrCode}
 * @since 10
 * @version 1.0
 */
OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_StopScreenCapture(struct OH_ScreenCapture *capture);

/**
 * @brief Acquire the audio buffer for the screen capture
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 * @param capture Pointer to an OH_ScreenCapture instance
 * @param audiobuffer Information describing the audio buffer of the capture
 * @param type Information describing the audio source type
 * @return Returns SCREEN_CAPTURE_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_SCREEN_CAPTURE_ErrCode}
 * @since 10
 * @version 1.0
 */
OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_AcquireAudioBuffer(struct OH_ScreenCapture *capture,
    OH_AudioBuffer **audiobuffer, OH_AudioCapSourceType type);

/**
 * @brief Acquire the video buffer for the screen capture
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 * @param capture Pointer to an OH_ScreenCapture instance
 * @param fence A processing state of display buffer
 * @param timestamp Information about the video buffer
 * @param region Information about the video buffer
 * @return Returns a pointer to an OH_NativeBuffer instance
 * @since 10
 * @version 1.0
 */
OH_NativeBuffer* OH_ScreenCapture_AcquireVideoBuffer(struct OH_ScreenCapture *capture,
    int32_t *fence, int64_t *timestamp, struct OH_Rect *region);

/**
 * @brief Release the audio buffer for the screen capture
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 * @param capture Pointer to an OH_ScreenCapture instance
 * @param type Information describing the audio source type
 * @return Returns SCREEN_CAPTURE_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_SCREEN_CAPTURE_ErrCode}
 * @since 10
 * @version 1.0
 */
OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_ReleaseAudioBuffer(struct OH_ScreenCapture *capture,
    OH_AudioCapSourceType type);

/**
 * @brief Release the video buffer for the screen capture
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 * @param capture Pointer to an OH_ScreenCapture instance
 * @return Returns SCREEN_CAPTURE_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_SCREEN_CAPTURE_ErrCode}
 * @since 10
 * @version 1.0
 */
OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_ReleaseVideoBuffer(struct OH_ScreenCapture *capture);

/**
 * @brief Set the callback function so that your application
 * can respond to the events generated by the screen capture. This interface must be called before Init is called.
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 * @param capture Pointer to an OH_ScreenCapture instance
 * @param callback A collection of all callback functions, see {@link OH_ScreenCaptureCallback}
 * @return Returns SCREEN_CAPTURE_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_SCREEN_CAPTURE_ErrCode}
 * @since 10
 * @version 1.0
 */
OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_SetCallback(struct OH_ScreenCapture *capture,
    struct OH_ScreenCaptureCallback callback);

/**
 * @brief Release the screen capture
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 * @param capture Pointer to an OH_ScreenCapture instance
 * @return Returns SCREEN_CAPTURE_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_SCREEN_CAPTURE_ErrCode}
 * @since 10
 * @version 1.0
 */
OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_Realease(struct OH_ScreenCapture *capture);

/**
 * @brief Controls the switch of the microphone, which is turned on by default
 * @syscap SystemCapability.Multimedia.Media.ScreenCapture
 * @param capture Pointer to an OH_ScreenCapture instance
 * @param isMicrophone The switch of the microphone
 * @return Returns SCREEN_CAPTURE_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_SCREEN_CAPTURE_ErrCode}
 * @since 10
 * @version 1.0
 */
OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_SetMicrophoneEnable(struct OH_ScreenCapture *capture, bool isMicrophone);

#ifdef __cplusplus
}
#endif

#endif // NATIVE_SCREEN_CAPTURE_H