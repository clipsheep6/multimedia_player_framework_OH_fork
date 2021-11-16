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

#define GST_BUFFER_TYPE_META_API_TYPE (gst_buffer_type_meta_api_get_type())
#define GST_BUFFER_TYPE_META_INFO (gst_buffer_type_meta_get_info())
typedef struct _GstBufferTypeMeta GstBufferTypeMeta;

typedef enum {
    FLAGS_READ_WRITE = 0x1,
    FLAGS_READ_ONLY = 0x2,
} Flags;

typedef enum {
    BUFFER_TYPE_FD,
    BUFFER_TYPE_HANDLE,
} BufferType;

struct _GstBufferTypeMeta {
    GstMeta meta;
    BufferType type;
    intptr_t buf;
    uint32_t offset;
    uint32_t length;
    uint32_t totalSize;
    int32_t fenceFd;
    Flags flag;
};

GType gst_buffer_type_meta_api_get_type(void);

const GstMetaInfo *gst_buffer_type_meta_get_info(void);

__attribute__((visibility("default")))
GstBufferTypeMeta *gst_buffer_get_buffer_type_meta(GstBuffer *buffer);

__attribute__((visibility("default")))
GstBufferTypeMeta *gst_buffer_add_buffer_handle_meta(GstBuffer *buffer, intptr_t buf, int32_t fenceFd);

__attribute__((visibility("default")))
GstBufferTypeMeta *gst_buffer_add_buffer_fd_meta(GstBuffer *buffer, intptr_t buf, uint32_t offset,
                                            uint32_t length, uint32_t totalSize, Flags flag);

#ifdef __cplusplus
}
#endif
#endif
