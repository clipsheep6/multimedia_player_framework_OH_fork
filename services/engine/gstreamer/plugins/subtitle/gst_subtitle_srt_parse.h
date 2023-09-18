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

#ifndef GST_SUBTITLE_SRT_PARSE_H
#define GST_SUBTITLE_SRT_PARSE_H

#include "gst_subtitle_base_parse.h"

#define GST_TYPE_SUBTITLE_SRT_PARSE (gst_subtitle_srt_parse_get_type())
#define GST_SUBTITLE_SRT_PARSE_CAST(obj) ((GstSubtitleSrtParse *)(obj))

typedef enum {
    SUBNUM_STATE,
    TIME_STATE,
    SUBTEXT_STATE,
    VALID_STATE
} SrtParseState;

typedef struct {
    GstSubtitleBaseParse parent;
    SrtParseState state;
    GString *buf;
    gint64 last_end;
} GstSubtitleSrtParse;

typedef struct {
    GstSubtitleBaseParseClass parent_class;
} GstSubtitleSrtParseClass;

gboolean gst_subtitle_srt_parse_register(GstPlugin *plugin);
GType gst_subtitle_srt_parse_get_type(void);

#endif // GST_SUBTITLE_SRT_PARSE_H
