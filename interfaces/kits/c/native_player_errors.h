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

#ifndef NATIVE_AVSCREEN_CAPTURE_ERRORS_H
#define NATIVE_AVSCREEN_CAPTURE_ERRORS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Screen capture error code
 * @syscap SystemCapability.Multimedia.Media.AVScreenCapture
 * @since 10
 * @version 1.0
 */

typedef enum OH_PLAYER_ErrCode {
    /**
     * basic error mask for screen recording.
     */
    PLAYER_ERR_BASE = 0,
    /**
     * the operation completed successfully.
     */
    PLAYER_ERR_OK = PLAYER_ERR_BASE,
    /**
     * no memory.
     */
    PLAYER_ERR_NO_MEMORY = PLAYER_ERR_BASE + 1,
    /**
     * opertation not be permitted.
     */
    PLAYER_ERR_OPERATE_NOT_PERMIT = PLAYER_ERR_BASE + 2,
    /**
     * invalid argument.
     */
    PLAYER_ERR_INVALID_VAL = PLAYER_ERR_BASE + 3,
    /**
     * IO error.
     */
    PLAYER_ERR_IO = PLAYER_ERR_BASE + 4,
    /**
     * network timeout.
     */
    PLAYER_ERR_TIMEOUT = PLAYER_ERR_BASE + 5,
    /**
     * unknown error.
     */
    PLAYER_ERR_UNKNOWN = PLAYER_ERR_BASE + 6,
    /**
     * media service died.
     */
    PLAYER_ERR_SERVICE_DIED = PLAYER_ERR_BASE + 7,
    /**
     * the state is not support this operation.
     */
    PLAYER_ERR_INVALID_STATE = PLAYER_ERR_BASE + 8,
    /**
     * unsupport interface.
     */
    PLAYER_ERR_UNSUPPORT = PLAYER_ERR_BASE + 9,
    /**
     * extend err start.
     */
    PLAYER_ERR_EXTEND_START = PLAYER_ERR_BASE + 100,
} OH_PLAYER_ErrCode;

#ifdef __cplusplus
}
#endif

#endif // NATIVE_AVSCREEN_CAPTURE_ERRORS_H