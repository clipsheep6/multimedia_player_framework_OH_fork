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

#include "buffer_type_meta.h"

static gboolean gst_buffer_type_meta_init(GstMeta *meta, gpointer params, GstBuffer *buffer)
{
    GstBufferTypeMeta *bufferMeta = (GstBufferTypeMeta *)meta;

    bufferMeta->type = BUFFER_TYPE_FD;
    bufferMeta->buf = (intptr_t)0;
    bufferMeta->offset = 0;
    bufferMeta->length = 0;
    bufferMeta->totalSize = 0;
    bufferMeta->fenceFd = -1;
    bufferMeta->flag = FLAGS_READ_WRITE;

    return TRUE;
}

GType gst_buffer_type_meta_api_get_type(void)
{
    static volatile GType type = 0;
    static const gchar *tags[] ={ GST_META_TAG_MEMORY_STR, NULL};
    if (g_once_init_enter (&type)) {
        GType _type = gst_meta_api_type_register ("GstBufferTypeMetaAPI", tags);
        g_once_init_leave (&type, _type);
    }
    return type;
}

static gboolean gst_buffer_type_meta_transform (GstBuffer * dest, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
    GstBufferTypeMeta *dMeta, *sMeta;

    sMeta = (GstBufferTypeMeta *)meta;

    if (GST_META_TRANSFORM_IS_COPY (type)) {
        GstMetaTransformCopy *copy = data;

        if (!copy->region) {
            dMeta = (GstBufferTypeMeta *)gst_buffer_add_meta(dest, GST_BUFFER_TYPE_META_INFO, NULL);
            if (!dMeta) {
                return FALSE;
            }
            dMeta->type = sMeta->type;
            dMeta->buf = sMeta->buf;
            dMeta->offset = sMeta->offset;
            dMeta->length = sMeta->length;
            dMeta->totalSize = sMeta->totalSize;
            dMeta->fenceFd = sMeta->fenceFd;
            dMeta->flag = sMeta->flag;
        }
    } else {
        return FALSE;
    }
    return TRUE;
}

const GstMetaInfo *gst_buffer_type_meta_get_info(void)
{
    static const GstMetaInfo *buffer_type_meta_info = NULL;

    if (g_once_init_enter ((GstMetaInfo **)&buffer_type_meta_info)) {
        const GstMetaInfo *meta = gst_meta_register(GST_BUFFER_TYPE_META_API_TYPE, "GstBufferTypeMeta",
            sizeof(GstBufferTypeMeta), (GstMetaInitFunction)gst_buffer_type_meta_init,
            (GstMetaFreeFunction)NULL, gst_buffer_type_meta_transform);
        g_once_init_leave ((GstMetaInfo **)&buffer_type_meta_info, (GstMetaInfo *)meta);
    }
    return buffer_type_meta_info;
}

GstBufferTypeMeta *gst_buffer_get_buffer_type_meta(GstBuffer *buffer)
{
    gpointer state = NULL;
    GstBufferTypeMeta *bufferMeta = NULL;
    GstMeta *meta;
    const GstMetaInfo *info = GST_BUFFER_TYPE_META_INFO;

    while ((meta = gst_buffer_iterate_meta(buffer, &state))) {
        if (meta->info->api == info->api) {
            bufferMeta = (GstBufferTypeMeta *)meta;
            return bufferMeta;
        }
    }
    return bufferMeta;
}

GstBufferTypeMeta *gst_buffer_add_buffer_handle_meta(GstBuffer *buffer, intptr_t buf, int32_t fenceFd)
{
    GstBufferTypeMeta *bufferMeta = NULL;

    bufferMeta = (GstBufferTypeMeta *)gst_buffer_add_meta(buffer, GST_BUFFER_TYPE_META_INFO, NULL);
    g_return_val_if_fail(bufferMeta != NULL, bufferMeta);

    bufferMeta->type = BUFFER_TYPE_HANDLE;
    bufferMeta->buf = buf;
    bufferMeta->fenceFd = fenceFd;
    return bufferMeta;
}

GstBufferTypeMeta *gst_buffer_add_buffer_fd_meta(GstBuffer *buffer, intptr_t buf, uint32_t offset,
                                            uint32_t length, uint32_t totalSize, Flags flag)
{
    GstBufferTypeMeta *bufferMeta = NULL;

    bufferMeta = (GstBufferTypeMeta *)gst_buffer_add_meta(buffer, GST_BUFFER_TYPE_META_INFO, NULL);
    g_return_val_if_fail(bufferMeta != NULL, bufferMeta);

    bufferMeta->type = BUFFER_TYPE_FD;
    bufferMeta->buf = buf;
    bufferMeta->offset = offset;
    bufferMeta->length = length;
    bufferMeta->totalSize = totalSize;
    bufferMeta->flag = flag;
    return bufferMeta;
}