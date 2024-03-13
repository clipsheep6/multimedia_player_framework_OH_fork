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
#ifndef GST_BUFFER_TYPE_META_H_
#define GST_BUFFER_TYPE_META_H_

#include <gst/gst.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#ifndef GST_API_EXPORT
#define GST_API_EXPORT __attribute__((visibility("default")))
#endif
#define GST_BUFFER_TYPE_META_API_TYPE (gst_buffer_type_meta_api_get_type())
#define GST_BUFFER_TYPE_META_INFO (gst_buffer_type_meta_get_info())
typedef struct _GstBufferTypeMeta GstBufferTypeMeta;
typedef struct _GstBufferFdConfig GstBufferFdConfig;
typedef struct _GstBufferHandleConfig GstBufferHandleConfig;

typedef enum {
    FLAGS_READ_WRITE = 0x1,
    FLAGS_READ_ONLY = 0x2,
} AVShmemFlags;

typedef enum {
    BUFFER_FLAG_NONE = 0,
    BUFFER_FLAG_EOS = 1 << 0,
    BUFFER_FLAG_SYNC_FRAME = 1 << 1,
    BUFFER_FLAG_PARTIAL_FRAME = 1 << 2,
    BUFFER_FLAG_CODEC_DATA = 1 << 3,
} BufferFlags;

typedef enum {
    BUFFER_TYPE_VIR,
    BUFFER_TYPE_AVSHMEM,
    BUFFER_TYPE_HANDLE,
} GstBufferType;

struct _GstBufferTypeMeta {
    GstMeta meta;
    uint32_t id;
    GstBufferType type;
    intptr_t buf;
    uint32_t bufLen;
    uint32_t offset;
    uint32_t length;
    uint32_t totalSize;
    int32_t fenceFd;
    uint32_t memFlag;
    uint32_t bufferFlag;
    int32_t pixelFormat;
    uint32_t width;
    uint32_t height;
    gboolean invalidpts;
};

struct _GstBufferFdConfig {
    uint32_t bufLen;
    uint32_t offset;
    uint32_t length;
    uint32_t totalSize;
    uint32_t memFlag;
    uint32_t bufferFlag;
};

struct _GstBufferHandleConfig {
    uint32_t bufLen;
    int32_t fenceFd;
    uint32_t bufferFlag;
    uint32_t length;
    int32_t pixelFormat;
    uint32_t width;
    uint32_t height;
};

GST_API_EXPORT GType gst_buffer_type_meta_api_get_type(void);

GST_API_EXPORT const GstMetaInfo *gst_buffer_type_meta_get_info(void);

GST_API_EXPORT GstBufferTypeMeta *gst_buffer_get_buffer_type_meta(GstBuffer *buffer);

GST_API_EXPORT GstBufferTypeMeta *gst_buffer_add_buffer_vir_meta(GstBuffer *buffer,
    intptr_t buf, uint32_t bufferFlag);

GST_API_EXPORT GstBufferTypeMeta *gst_buffer_add_buffer_handle_meta(GstBuffer *buffer, intptr_t buf,
    GstBufferHandleConfig config);

GST_API_EXPORT GstBufferTypeMeta *gst_buffer_add_buffer_fd_meta(GstBuffer *buffer, intptr_t buf,
    GstBufferFdConfig config);

#ifdef __cplusplus
}
#endif
#endif
