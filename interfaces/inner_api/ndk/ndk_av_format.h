/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef NDK_AV_FORMAT_H
#define NDK_AV_FORMAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct AVFormat AVFormat; 

/**
 * @brief
 *
 * @since 3.2
 * @version 3.2
 */
enum AVPixelFormat {
    /**
     * yuv 420 planar.
     */
    AV_FORMT_YUVI420 = 1,
    /**
     *  NV12. yuv 420 semiplanar.
     */
    AV_FORMT_NV12 = 2,
    /**
     *  NV21. yvu 420 semiplanar.
     */
    AV_FORMT_NV21 = 3,
    /**
     * format from surface.
     */
    AV_FORMT_SURFACE_FORMAT = 4,
    /**
     * RGBA.
     */
    AV_FORMT_RGBA = 5,
};

struct AVFormat* OH_AV_CreateFormat(void);
void OH_AV_DestroyFormat(struct AVFormat *format);
bool OH_AV_FormatPutIntValue(struct AVFormat *format, const char *key, int32_t value);
bool OH_AV_FormatPutLongValue(struct AVFormat *format, const char *key, int64_t value);
bool OH_AV_FormatPutFloatValue(struct AVFormat *format, const char *key, float value);
bool OH_AV_FormatPutDoubleValue(struct AVFormat *format, const char *key, double value);
bool OH_AV_FormatPutStringValue(struct AVFormat *format, const char *key, const char *value);
bool OH_AV_FormatPutBuffer(struct AVFormat *format, const char *key, const uint8_t *addr, size_t size);
bool OH_AV_FormatGetIntValue(struct AVFormat *format, const char *key, int32_t *out);
bool OH_AV_FormatGetLongValue(struct AVFormat *format, const char *key, int64_t *out);
bool OH_AV_FormatGetFloatValue(struct AVFormat *format, const char *key, float *out);
bool OH_AV_FormatGetDoubleValue(struct AVFormat *format, const char *key, double *out);
bool OH_AV_FormatGetStringValue(struct AVFormat *format, const char *key, const char **out);

/**
 * @brief The returned data is owned by the format and remains valid as long as the named entry is part of the format.
 * @syscap SystemCapability.Multimedia.Media.Core
 * @param format Encapsulate Format structure instance pointer
 * @param key
 * @param addr 生命周期是format持有，伴随Format销毁，如果调用者需要长期持有，必须进行内存拷贝
 * @param size buffer size
 * @since  3.2
 * @version 3.2
 */
bool OH_AV_FormatGetBuffer(struct AVFormat *format, const char *key, uint8_t **addr, size_t *size);

#ifdef __cplusplus
}
#endif

#endif // NDK_AV_FORMAT_H