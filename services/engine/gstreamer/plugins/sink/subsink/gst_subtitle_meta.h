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
#ifndef GST_SUBTITLE_META_H
#define GST_SUBTITLE_META_H

#include "gst/gst.h"
#include "gio/gio.h"

G_BEGIN_DECLS

typedef struct {
    const char *text;
    const char *color;
    int size;
    int style;
    int weight;
    int decorationType;
} GstSubtitleMeta;

__attribute__((visibility("default"))) GType gst_subtitle_meta_api_get_type(void);
#define GST_SUBTITLE_META_API_TYPE (gst_subtitle_meta_api_get_type())
#define GST_BUFFER_GET_SUBTITLE_META(b) \
    ((GstSubtitleMeta *)gst_buffer_get_meta((b), GST_SUBTITLE_META_API_TYPE))

G_END_DECLS

#endif