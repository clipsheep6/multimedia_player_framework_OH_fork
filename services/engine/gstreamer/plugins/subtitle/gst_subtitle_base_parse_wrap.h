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

#ifndef GST_SUBTITLE_BASE_PARSE_WRAP_H
#define GST_SUBTITLE_BASE_PARSE_WRAP_H

#include "gst_subtitle_base_parse.h"

gboolean handle_text_subtitle(GstSubtitleBaseParse *self, const GstSubtitleDecodedFrame *decoded_frame,
    GstSubtitleStream *stream, GstFlowReturn *ret);
GstSubtitleStream *gst_subtitle_get_stream_by_id(const GstSubtitleBaseParse *self, gint stream_id);
void free_subinfos_and_streams(GstSubtitleBaseParse *base_parse);
gboolean handle_first_frame(GstPad *sinkpad, GstBuffer *buf, GstSubtitleBaseParse *self);
GstSubtitleStream *gst_subtitle_get_stream(GstSubtitleBaseParse *base_parse, const GstSubtitleInfo *info);
gboolean get_subtitle_streams(const GstSubtitleBaseParseClass *baseclass,
    GstBuffer *buf, GstSubtitleBaseParse *self);
void gst_subtitle_push_stream_start_event(GstSubtitleBaseParse *base_parse);
gboolean gst_subtitle_set_caps(GstSubtitleBaseParse *base_parse);
gboolean gst_subtitle_set_tags(GstSubtitleBaseParse *base_parse);
gboolean chain_set_caps_and_tags(GstSubtitleBaseParse *self);
gboolean chain_push_new_segment_event(GstFlowReturn ret, GstSubtitleBaseParse *self);
gboolean decode_one_frame(const GstSubtitleBaseParseClass *baseclass, GstSubtitleBaseParse *self,
    GstSubtitleFrame *frame, GstSubtitleDecodedFrame *decoded_frame);

#endif // GST_SUBTITLE_BASE_PARSE_WRAP_H
