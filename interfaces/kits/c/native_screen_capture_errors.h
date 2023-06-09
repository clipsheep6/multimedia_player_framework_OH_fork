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

#ifndef NATIVE_SCREEN_CAPTURE_ERRORS_H
#define NATIVE_SCREEN_CAPTURE_ERRORS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Screen capture error code
 * @syscap SystemCapability.Multimedia.Media.Core
 * @since 10
 * @version 1.0
 */

typedef enum OH_SCREEN_CAPTURE_ErrCode {
    /**
     * the operation completed successfully.
     */
    SCREEN_CAPTURE_ERR_OK = 0,
    /**
     * no memory.
     */
    SCREEN_CAPTURE_ERR_NO_MEMORY = 1,
    /**
     * opertation not be permitted.
     */
    SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT = 2,
    /**
     * invalid argument.
     */
    SCREEN_CAPTURE_ERR_INVALID_VAL = 3,
    /**
     * IO error.
     */
    SCREEN_CAPTURE_ERR_IO = 4,
    /**
     * network timeout.
     */
    SCREEN_CAPTURE_ERR_TIMEOUT = 5,
    /**
     * unknown error.
     */
    SCREEN_CAPTURE_ERR_UNKNOWN = 6,
    /**
     * media service died.
     */
    SCREEN_CAPTURE_ERR_SERVICE_DIED = 7,
    /**
     * the state is not support this operation.
     */
    SCREEN_CAPTURE_ERR_INVALID_STATE = 8,
    /**
     * unsupport interface.
     */
    SCREEN_CAPTURE_ERR_UNSUPPORT = 9,
    /**
     * extend err start.
     */
    SCREEN_CAPTURE_ERR_EXTEND_START = 100,
} OH_SCREEN_CAPTURE_ErrCode;

#ifdef __cplusplus
}
#endif

#endif // NATIVE_SCREEN_CAPTURE_ERRORS_H