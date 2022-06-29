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
 * @brief Enumerates AVPixel Format.
 * @syscap SystemCapability.Multimedia.Media.Core
 * @since 3.2
 * @version 3.2
 */
enum AVPixelFormat {
    /**
     * yuv 420 planar.
     */
    AV_PIXEL_FORMT_YUVI420 = 1,
    /**
     *  NV12. yuv 420 semiplanar.
     */
    AV_PIXEL_FORMT_NV12 = 2,
    /**
     *  NV21. yvu 420 semiplanar.
     */
    AV_PIXEL_FORMT_NV21 = 3,
    /**
     * format from surface.
     */
    AV_PIXEL_FORMT_SURFACE_FORMAT = 4,
    /**
     * RGBA.
     */
    AV_PIXEL_FORMT_RGBA = 5,
};

/**
 * @brief 创建一个AVFormat句柄指针，用以读写数据
 * @syscap SystemCapability.Multimedia.Media.Core
 * @return 返回值为AVFormat句柄指针
 * @since  3.2
 * @version 3.2
 */
struct AVFormat* OH_AV_CreateFormat(void);

/**
 * @brief 销毁指定AVFormat句柄资源
 * @syscap SystemCapability.Multimedia.Media.Core
 * @param format AVFormat句柄指针
 * @return void
 * @since  3.2
 * @version 3.2
 */
void OH_AV_DestroyFormat(struct AVFormat *format);

/**
 * @brief 拷贝AVFormat句柄资源
 * @syscap SystemCapability.Multimedia.Media.Core
 * @param to 接收数据的AVFormat句柄指针
 * @param from 被拷贝数据的AVFormat句柄指针
 * @return 返回值为TRUE 成功，FALSE 失败
 * @since  3.2
 * @version 3.2
 */
bool OH_AV_FormatCopy(struct AVFormat *to, struct AVFormat *from);

/**
 * @brief 向AVFormat写入Int数据
 * @syscap SystemCapability.Multimedia.Media.Core
 * @param format AVFormat句柄指针
 * @param key 写入数据的键值
 * @param value 写入的数据
 * @return 返回值为TRUE 成功，FALSE 失败
 * @since  3.2
 * @version 3.2
 */
bool OH_AV_FormatPutIntValue(struct AVFormat *format, const char *key, int32_t value);

/**
 * @brief 向AVFormat写入Long数据
 * @syscap SystemCapability.Multimedia.Media.Core
 * @param format AVFormat句柄指针
 * @param key 写入数据的键值
 * @param value 写入的数据
 * @return 返回值为TRUE 成功，FALSE 失败
 * @since  3.2
 * @version 3.2
 */
bool OH_AV_FormatPutLongValue(struct AVFormat *format, const char *key, int64_t value);

/**
 * @brief 向AVFormat写入Float数据
 * @syscap SystemCapability.Multimedia.Media.Core
 * @param format AVFormat句柄指针
 * @param key 写入数据的键值
 * @param value 写入的数据
 * @return 返回值为TRUE 成功，FALSE 失败
 * @since  3.2
 * @version 3.2
 */
bool OH_AV_FormatPutFloatValue(struct AVFormat *format, const char *key, float value);

/**
 * @brief 向AVFormat写入Double数据
 * @syscap SystemCapability.Multimedia.Media.Core
 * @param format AVFormat句柄指针
 * @param key 写入数据的键值
 * @param value 写入的数据
 * @return 返回值为TRUE 成功，FALSE 失败
 * @since  3.2
 * @version 3.2
 */
bool OH_AV_FormatPutDoubleValue(struct AVFormat *format, const char *key, double value);

/**
 * @brief 向AVFormat写入String数据
 * @syscap SystemCapability.Multimedia.Media.Core
 * @param format AVFormat句柄指针
 * @param key 写入数据的键值
 * @param value 写入的数据
 * @return 返回值为TRUE 成功，FALSE 失败
 * @since  3.2
 * @version 3.2
 */
bool OH_AV_FormatPutStringValue(struct AVFormat *format, const char *key, const char *value);

/**
 * @brief 向AVFormat写入一块指定长度的数据
 * @syscap SystemCapability.Multimedia.Media.Core
 * @param format AVFormat句柄指针
 * @param key 写入数据的键值
 * @param addr 写入的数据地址
 * @param size 写入的数据长度
 * @return 返回值为TRUE 成功，FALSE 失败
 * @since  3.2
 * @version 3.2
 */
bool OH_AV_FormatPutBuffer(struct AVFormat *format, const char *key, const uint8_t *addr, size_t size);

/**
 * @brief 从AVFormat读取Int数据
 * @syscap SystemCapability.Multimedia.Media.Core
 * @param format AVFormat句柄指针
 * @param key 读取数据的键值
 * @param out 读取的数据
 * @return 返回值为TRUE 成功，FALSE 失败
 * @since  3.2
 * @version 3.2
 */
bool OH_AV_FormatGetIntValue(struct AVFormat *format, const char *key, int32_t *out);

/**
 * @brief 从AVFormat读取Long数据
 * @syscap SystemCapability.Multimedia.Media.Core
 * @param format AVFormat句柄指针
 * @param key 读取数据的键值
 * @param out 读取的数据
 * @return 返回值为TRUE 成功，FALSE 失败
 * @since  3.2
 * @version 3.2
 */
bool OH_AV_FormatGetLongValue(struct AVFormat *format, const char *key, int64_t *out);

/**
 * @brief 从AVFormat读取Float数据
 * @syscap SystemCapability.Multimedia.Media.Core
 * @param format AVFormat句柄指针
 * @param key 读取数据的键值
 * @param out 读取的数据
 * @return 返回值为TRUE 成功，FALSE 失败
 * @since  3.2
 * @version 3.2
 */
bool OH_AV_FormatGetFloatValue(struct AVFormat *format, const char *key, float *out);

/**
 * @brief 从AVFormat读取Double数据
 * @syscap SystemCapability.Multimedia.Media.Core
 * @param format AVFormat句柄指针
 * @param key 读取数据的键值
 * @param out 读取的数据
 * @return 返回值为TRUE 成功，FALSE 失败
 * @since  3.2
 * @version 3.2
 */
bool OH_AV_FormatGetDoubleValue(struct AVFormat *format, const char *key, double *out);

/**
 * @brief 从AVFormat读取Double数据
 * @syscap SystemCapability.Multimedia.Media.Core
 * @param format AVFormat句柄指针
 * @param key 读取数据的键值
 * @param out 读取的字符串指针，指向的数据生命周期伴随GetString更新，伴随Format销毁，如果调用者需要长期持有，必须进行内存拷贝
 * @return 返回值为TRUE 成功，FALSE 失败
 * @since  3.2
 * @version 3.2
 */
bool OH_AV_FormatGetStringValue(struct AVFormat *format, const char *key, const char **out);

/**
 * @brief The returned data is owned by the format and remains valid as long as the named entry is part of the format.
 * @syscap SystemCapability.Multimedia.Media.Core
 * @param format AVFormat句柄指针
 * @param key 读写数据的键值
 * @param addr 生命周期是format持有，伴随Format销毁，如果调用者需要长期持有，必须进行内存拷贝
 * @param size 读写数据的长度
 * @return 返回值为TRUE 成功，FALSE 失败
 * @since  3.2
 * @version 3.2
 */
bool OH_AV_FormatGetBuffer(struct AVFormat *format, const char *key, uint8_t **addr, size_t *size);


/**
 * @brief The returned data is owned by the format and remains valid as long as the named entry is part of the format.
 * @syscap SystemCapability.Multimedia.Media.Core
 * @param format AVFormat句柄指针
 * @return 返回由键值和数据组成的字符串
 * @since  3.2
 * @version 3.2
 */
const char * OH_AV_FormatDumpInfo(struct AVFormat *format);

#ifdef __cplusplus
}
#endif

#endif // NDK_AV_FORMAT_H