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

#ifndef GST_SUBTITLE_COMMON_H
#define GST_SUBTITLE_COMMON_H

#include "gst/gst.h"
#include "gst_subtitle_base_parse.h"

gchar *gst_subtitle_str_dup(const gchar *str, gboolean ndup, gsize len);
void gst_subtitle_free_frame(GstSubtitleBaseParse *base, GstSubtitleDecodedFrame *decoded_frame);
void gst_subtitle_typefind(GstTypeFind *tf, const gpointer priv,
    const GstSubtitleFormatDetect detect_caps_pfn);
gsize gst_subtitle_read_line(GstSubtitleBaseParse *base, gchar **out_line);
GstFlowReturn gst_subtitle_push_buffer(GstSubtitleBaseParse *self,
    const GstSubtitleDecodedFrame *decoded_frame);

#endif // GST_SUBTITLE_COMMON_H
