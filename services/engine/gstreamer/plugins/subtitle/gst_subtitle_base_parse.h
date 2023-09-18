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

#ifndef GST_SUBTITLE_BASE_PARSE_H
#define GST_SUBTITLE_BASE_PARSE_H

#include "gst/gst.h"
#include "gst/base/gstadapter.h"

#define MAX_SUB_STREAM_NUM 20

#define GST_TYPE_SUBTITLE_BASE_PARSE (gst_subtitle_base_parse_get_type())
#define GST_SUBTITLE_BASE_PARSE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SUBTITLE_BASE_PARSE, GstSubtitleBaseParse))
#define GST_SUBTITLE_BASE_PARSE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_SUBTITLE_BASE_PARSE, GstSubtitleBaseParseClass))
#define GST_IS_SUBTITLE_BASE_PARSE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SUBTITLE_BASE_PARSE))
#define GST_IS_SUBTITLE_BASE_PARSE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_SUBTITLE_BASE_PARSE))
#define GST_SUBTITLE_BASE_PARSE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_SUBTITLE_BASE_PARSE, GstSubtitleBaseParseClass))

typedef struct {
    guint64 start_time;
    guint64 duration;
} GstSubtitleBaseParseState;

typedef struct {
    guint8 *data;
    gsize len;
    guint64 pts;
    guint64 timestamp;
    guint64 duration;
    gint stream_index;
} GstSubtitleFrame;

typedef struct {
    guint8 *data;
    gsize len;
    guint64 pts;
    guint64 timestamp;
    guint64 duration;
    gint stream_index;
} GstSubtitleDecodedFrame;

typedef struct {
    gint stream_id;
    gchar *desc;
} GstSubtitleInfo;

typedef struct {
    gint stream_id;
    GstPad *pad;
    GstCaps *caps;
    GstTagList *tags;
    gboolean active;
    GstFlowReturn last_result;
} GstSubtitleStream;

typedef struct {
    GstAdapter *adapter; // external subtitles use @adapter for buffering
    GString *text;       // subtitle text string is stored in @text
} GstSubtitleBufferContext;

typedef struct {
    GstElement element;
    GstPad *sinkpad;
    gboolean need_srcpad_caps;
    GstSubtitleBaseParseState state;
    guint64 offset; // seek
    GstSegment *segment;
    GstSegment *event_segment;
    gboolean need_segment;
    gboolean flushing;
    gboolean first_buffer;
    GstSubtitleBufferContext buffer_ctx;
    gboolean from_internal;
    gboolean recv_eos;
    gint stream_id;
    gchar *language;
    gint stream_num;
    GstSubtitleInfo *subinfos[MAX_SUB_STREAM_NUM];
    GstSubtitleStream *streams[MAX_SUB_STREAM_NUM];
    GstPadTemplate *srcpadtmpl;
    gboolean got_streams;
    guint32 last_seekseq;
    gint pad_num;
    gboolean seek_snap_after;
    gboolean switching;
    gboolean has_send_stream_start;
    gboolean first_segment;
    GMutex buffermutex;
    GMutex segmentmutex;
    GMutex pushmutex;
} GstSubtitleBaseParse;

typedef GstCaps *(*GstSubtitleFormatDetect)(const gchar *match_str);
typedef GstCaps *(*GstSubtitleGetSrcCaps)(const GstSubtitleBaseParse *self, gint stream_id);
typedef gsize (*GstSubtitleReadFrame)(GstSubtitleBaseParse *self, GstSubtitleFrame *frame);
typedef gboolean (*GstSubtitleDecodeFrame)(GstSubtitleBaseParse *self, const GstSubtitleFrame *frame,
                                           GstSubtitleDecodedFrame *decoded_frame);
typedef void (*GstSubtitleOnSeek)(GstSubtitleBaseParse *self, const GstEvent *event);
typedef GstFlowReturn (*GstSubtitleHandleBuffer)(GstSubtitleBaseParse *self);
typedef gboolean (*GstSubtitleOnSinkEvent)(GstSubtitleBaseParse *self, GstEvent *event);
typedef void (*GstSubtitleSetVideoBaseTime)(GstSubtitleBaseParse *self, gint64 base_time);

typedef struct {
    GstElementClass parent_class;

    GstSubtitleGetSrcCaps get_srcpad_caps_pfn; // get src pad caps
    GstSubtitleReadFrame read_frame_pfn;
    GstSubtitleDecodeFrame decode_frame_pfn;
    GstSubtitleOnSeek on_seek_pfn;
    GstSubtitleHandleBuffer handle_buffer_pfn;
    GstSubtitleOnSinkEvent on_sink_event_pfn;
} GstSubtitleBaseParseClass;

GType gst_subtitle_base_parse_get_type(void);

#endif // GST_SUBTITLE_BASE_PARSE_H
