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

#include "gst_subtitle_base_parse.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/time.h>
#include "securec.h"
#include "glib.h"
#include "media_log.h"
#include "gst_subtitle_common.h"
#include "gst_subtitle_base_parse_wrap.h"

GST_DEBUG_CATEGORY_STATIC(gst_subtitle_base_parse_debug_category);
#define GST_CAT_DEFAULT gst_subtitle_base_parse_debug_category

static GstElementClass *g_parentClass = nullptr;

static void gst_subtitle_base_parse_class_init(GstSubtitleBaseParseClass *klass);
static void gst_subtitle_base_parse_init(GstSubtitleBaseParse *base_parse, gpointer gst_class);
static void gst_subtitle_base_parse_dispose(GObject *object);
static gboolean gst_subtitle_base_parse_sink_event(GstPad *pad, GstObject *parent, GstEvent *event);
static GstStateChangeReturn gst_subtitle_base_parse_change_state(GstElement *element, GstStateChange transition);
static GstFlowReturn gst_subtitle_base_parse_chain(GstPad *sinkpad, GstObject *parent, GstBuffer *buf);
static GstFlowReturn default_handle_buffer(GstSubtitleBaseParse *self);
static gboolean default_handle_sink_event(GstSubtitleBaseParse *self, GstEvent *event);
static void gst_subtitle_base_parse_pfn_init(GstSubtitleBaseParseClass *klass);

GType gst_subtitle_base_parse_get_type(void)
{
    static volatile gsize subtitle_base_type = 0;

    if (g_once_init_enter(&subtitle_base_type)) {
        static const GTypeInfo subtitle_base_info = {sizeof(GstSubtitleBaseParseClass),
            nullptr, nullptr, (GClassInitFunc)gst_subtitle_base_parse_class_init, nullptr, nullptr,
            sizeof(GstSubtitleBaseParse), 0, (GInstanceInitFunc)gst_subtitle_base_parse_init, nullptr};

        GType type = g_type_register_static(GST_TYPE_ELEMENT, "GstSubtitleBaseParse", &subtitle_base_info,
            G_TYPE_FLAG_ABSTRACT);
        g_once_init_leave(&subtitle_base_type, type);
    }

    return (GType)subtitle_base_type;
}

/* free subtitle buffer */
static void free_buffer_context(GstSubtitleBaseParse *base_parse)
{
    g_return_if_fail(base_parse != nullptr);
    GstSubtitleBufferContext *buf_ctx = &base_parse->buffer_ctx;

    if (buf_ctx->adapter != nullptr) {
        gst_adapter_clear(buf_ctx->adapter);
        g_object_unref(buf_ctx->adapter);
        buf_ctx->adapter = nullptr;
    }

    if (buf_ctx->text != nullptr) {
        (void)g_string_free(buf_ctx->text, (gboolean)TRUE);
        buf_ctx->text = nullptr;
    }
}

static void gst_subtitle_base_parse_dispose(GObject *object)
{
    g_return_if_fail(object != nullptr);
    GstSubtitleBaseParse *base_parse = static_cast<GstSubtitleBaseParse *>((void *)object);

    GST_DEBUG_OBJECT(base_parse, "cleaning up subtitle parser");

    if (base_parse->segment != nullptr) {
        gst_segment_free(base_parse->segment);
        base_parse->segment = nullptr;
    }

    if (base_parse->event_segment != nullptr) {
        gst_segment_free(base_parse->event_segment);
        base_parse->event_segment = nullptr;
    }

    g_free(base_parse->language);
    base_parse->language = nullptr;

    free_buffer_context(base_parse);
    g_mutex_clear(&base_parse->buffermutex);
    g_mutex_clear(&base_parse->segmentmutex);
    g_mutex_clear(&base_parse->pushmutex);

    free_subinfos_and_streams(base_parse);

    G_OBJECT_CLASS(g_parentClass)->dispose(object);
}

static void gst_subtitle_base_parse_class_init(GstSubtitleBaseParseClass *klass)
{
    GST_DEBUG_CATEGORY_INIT(gst_subtitle_base_parse_debug_category, "subtitlebaseparse", 0, "subtitle base parse");

    GObjectClass *object_class = (GObjectClass *)klass;
    GstElementClass *element_class = (GstElementClass *)klass;

    g_parentClass = static_cast<GstElementClass *>(g_type_class_peek_parent(klass));
    element_class->change_state = gst_subtitle_base_parse_change_state;
    object_class->dispose = gst_subtitle_base_parse_dispose;

    GstPadTemplate *src_pad_template = gst_pad_template_new("text_%u", GST_PAD_SRC, GST_PAD_SOMETIMES, GST_CAPS_ANY);
    if (src_pad_template != nullptr) {
        gst_element_class_add_pad_template((GstElementClass *)klass, src_pad_template);
        GST_INFO("added a SOMETIMES src pad template");
    }
    gst_subtitle_base_parse_pfn_init(klass);
}

static void gst_subtitle_base_parse_pfn_init(GstSubtitleBaseParseClass *klass)
{
    klass->decode_frame_pfn = nullptr;
    klass->get_srcpad_caps_pfn = nullptr;
    klass->read_frame_pfn = nullptr;
    klass->on_seek_pfn = nullptr;
    klass->handle_buffer_pfn = default_handle_buffer;
    klass->on_sink_event_pfn = default_handle_sink_event;
}

/* initialize the subtitle data stream buffer */
static void init_buffer_context(GstSubtitleBaseParse *base_parse)
{
    g_return_if_fail(base_parse != nullptr);

    /* initialize the external subtitle buffer */
    GstSubtitleBufferContext *buf_ctx = &base_parse->buffer_ctx;

    if (memset_s(buf_ctx, sizeof(GstSubtitleBufferContext), 0, sizeof(GstSubtitleBufferContext)) != EOK) {
        GST_ERROR_OBJECT(base_parse, "memset_s failed");
    }

    buf_ctx->adapter = gst_adapter_new();
    g_return_if_fail(buf_ctx->adapter != nullptr);

    buf_ctx->text = g_string_new(nullptr);
    if (buf_ctx->text == nullptr) {
        g_object_unref(buf_ctx->adapter);
        buf_ctx->adapter = nullptr;
        return;
    }

    if (buf_ctx->text->str == nullptr) {
        (void)g_string_free(buf_ctx->text, (gboolean)TRUE);
        buf_ctx->text = nullptr;
        g_object_unref(buf_ctx->adapter);
        buf_ctx->adapter = nullptr;
        return;
    }
}

static void gst_subtitle_base_parse_init(GstSubtitleBaseParse *base_parse, gpointer gst_class)
{
    g_return_if_fail((base_parse != nullptr) && (gst_class != nullptr));

    gint index;

    base_parse->srcpadtmpl = gst_element_class_get_pad_template(GST_ELEMENT_CLASS(gst_class), "text_%u");

    GST_DEBUG_OBJECT(base_parse, "creating sink pad");

    GstPadTemplate *sink_pad_template = gst_element_class_get_pad_template(GST_ELEMENT_CLASS(gst_class), "sink");
    g_return_if_fail(sink_pad_template != nullptr);

    GstPad *sinkpad = gst_pad_new_from_template(sink_pad_template, "sink");
    g_return_if_fail(sinkpad != nullptr);

    GST_DEBUG_OBJECT(base_parse, "setting functions on sink pad");
    gst_pad_set_chain_function(sinkpad, gst_subtitle_base_parse_chain);
    gst_pad_set_event_function(sinkpad, gst_subtitle_base_parse_sink_event);
    base_parse->sinkpad = sinkpad;
    GST_DEBUG_OBJECT(base_parse, "adding sink pad");
    (void)gst_element_add_pad(GST_ELEMENT(base_parse), sinkpad);

    base_parse->flushing = FALSE;
    base_parse->segment = gst_segment_new();
    gst_segment_init((GstSegment *)(base_parse->segment), GST_FORMAT_TIME);
    base_parse->event_segment = gst_segment_new();
    gst_segment_init((GstSegment *)(base_parse->event_segment), GST_FORMAT_TIME);

    base_parse->need_segment = TRUE;
    base_parse->from_internal = FALSE;
    base_parse->recv_eos = FALSE;
    base_parse->first_buffer = FALSE;
    base_parse->stream_id = -1;
    base_parse->language = nullptr;
    init_buffer_context(base_parse);

    base_parse->got_streams = FALSE;
    base_parse->last_seekseq = 0;
    base_parse->stream_num = 0;
    base_parse->pad_num = 0;

    for (index = 0; index < MAX_SUB_STREAM_NUM; index++) {
        base_parse->subinfos[index] = nullptr;
        base_parse->streams[index] = nullptr;
    }

    base_parse->seek_snap_after = FALSE;
    base_parse->switching = FALSE;
    base_parse->first_segment = TRUE;
    g_mutex_init(&base_parse->buffermutex);
    g_mutex_init((GMutex *)&base_parse->pushmutex);
    g_mutex_init(&base_parse->segmentmutex);
}

static GstFlowReturn default_handle_buffer(GstSubtitleBaseParse *self)
{
    GstFlowReturn ret = GST_FLOW_OK;
    GstSubtitleFrame frame;
    GstSubtitleDecodedFrame decoded_frame;

    g_return_val_if_fail(self != nullptr, ret);
    GstSubtitleBaseParseClass *baseclass = GST_SUBTITLE_BASE_PARSE_GET_CLASS(self);
    g_return_val_if_fail(baseclass != nullptr, ret);

    while (!self->flushing) {
        /* determine whether the subclass overrides read_frame_pfn() and decode_frame_pfn() */
        if (baseclass->read_frame_pfn == nullptr) {
            GST_ERROR_OBJECT(self, "have not override ReadFrame function");
            break;
        }
        if (baseclass->decode_frame_pfn == nullptr) {
            GST_ERROR_OBJECT(self, "have not override DecodeFrame function");
            break;
        }

        frame.data = nullptr;
        frame.len = 0;

        g_mutex_lock(&self->buffermutex);
        /* read_frame_pfn: return value == 0 means that a full frame of subtitles cannot be read in current buffer */
        gsize consumed = baseclass->read_frame_pfn(self, &frame);
        g_mutex_unlock(&self->buffermutex);

        if (G_UNLIKELY(consumed == 0)) {
            g_free(frame.data);
            frame.data = nullptr;
            break;
        }

        if (memset_s(&decoded_frame, sizeof(GstSubtitleDecodedFrame), 0, sizeof(GstSubtitleDecodedFrame)) != EOK) {
            GST_ERROR_OBJECT(self, "memset_s failed");
            g_free(frame.data);
            return ret;
        }
        decoded_frame.stream_index = self->stream_id;
        /* decode a frame of subtitles */
        gboolean err_ret = decode_one_frame(baseclass, self, &frame, &decoded_frame);
        if (!err_ret) {
            continue;
        }

        /* encapsulate the decoded frame of subtitles into a gstbuffer, and push it to srcpad */
        ret = gst_subtitle_push_buffer(self, &decoded_frame);
        gst_subtitle_free_frame(self, &decoded_frame);
    }

    return ret;
}

static GstFlowReturn gst_subtitle_base_parse_chain(GstPad *sinkpad, GstObject *parent, GstBuffer *buf)
{
    g_return_val_if_fail((parent != nullptr) && (buf != nullptr), GST_FLOW_NOT_LINKED);

    GstFlowReturn ret = GST_FLOW_OK;
    GstSubtitleBaseParse *self = static_cast<GstSubtitleBaseParse *>((void *)parent);
    GstSubtitleBaseParseClass *baseclass = GST_SUBTITLE_BASE_PARSE_GET_CLASS(self);
    g_return_val_if_fail(baseclass != nullptr, GST_FLOW_NOT_LINKED);

    /* for first buffer, we must determine whether it is internal or external */
    if (!handle_first_frame(sinkpad, buf, self)) {
        gst_buffer_unref(buf);
        return ret;
    }

    g_return_val_if_fail(get_subtitle_streams(baseclass, buf, self), ret);

    if (G_UNLIKELY(!self->has_send_stream_start)) {
        gst_subtitle_push_stream_start_event(self);
        self->has_send_stream_start = TRUE;
    }

    g_return_val_if_fail(chain_set_caps_and_tags(self), GST_FLOW_EOS);

    g_return_val_if_fail(!self->flushing, ret);

    g_return_val_if_fail(chain_push_new_segment_event(ret, self), ret);

    g_return_val_if_fail(baseclass->handle_buffer_pfn != nullptr, ret);
    return baseclass->handle_buffer_pfn(self);
}

static gboolean sink_event_handle_segment_eos(GstEvent *event, GstSubtitleBaseParse *self)
{
    gboolean ret = FALSE;

    self->recv_eos = TRUE;
    GstBuffer *buf = gst_buffer_new_and_alloc(3); // alloc 3 bytes ('\r', '\n', '\0') for copy
    if (buf != nullptr) {
        const gchar term_chars[] = {'\r', '\n', '\0'};
        GST_DEBUG_OBJECT(self, "EOS, Pushing remaining text");
        (void)gst_buffer_fill(buf, 0, term_chars, 3); // copy 3 bytes from term_chars to buf at offset 0
        gst_buffer_set_size(buf, 2); // set the total size 2 of the memory blocks in buf
        GST_BUFFER_OFFSET(buf) = self->offset;
        (void)gst_subtitle_base_parse_chain(self->sinkpad, (GstObject *)self, buf);
    }

    g_return_val_if_fail(self->sinkpad != nullptr, ret);
    ret = gst_pad_event_default(self->sinkpad, (GstObject *)self, event);
    return ret;
}

static gboolean sink_event_handle_segment_event(GstEvent *event, GstSubtitleBaseParse *self)
{
    g_return_val_if_fail((event != nullptr) && (self != nullptr) && (self->segment != nullptr), FALSE);

    const GstSegment *s = nullptr;
    gboolean need_tags = FALSE;

    gst_event_parse_segment(event, &s);
    if ((s != nullptr) && (s->format == GST_FORMAT_TIME)) {
        gst_event_copy_segment(event, self->segment);
        gst_event_copy_segment(event, self->event_segment);
    }
    GST_DEBUG_OBJECT(self, "newsegment (%s)", gst_format_get_name(self->segment->format));

    g_mutex_lock(&self->segmentmutex);
    self->need_segment = TRUE;
    self->switching = FALSE;
    g_mutex_unlock(&self->segmentmutex);

    gst_event_unref(event);
    event = nullptr;
    GST_INFO_OBJECT(self, "subtitle base receive segment event with 0x%06" PRIXPTR ", stream_id: %d, "
        "start: %" G_GUINT64_FORMAT ", stop: %" G_GUINT64_FORMAT, FAKE_POINTER(&self->segment),
        self->stream_id, self->segment->start, self->segment->stop);
    if (self->need_srcpad_caps) {
        if (gst_subtitle_set_caps(self)) {
            self->need_srcpad_caps = FALSE;
            need_tags = TRUE;
        }
    }
    if (need_tags) {
        g_return_val_if_fail(gst_subtitle_set_tags(self), TRUE);
    }
    return TRUE;
}

/* clear subtitle buffer */
static void clear_buffer(GstSubtitleBaseParse *base_parse)
{
    g_return_if_fail(base_parse != nullptr);

    GstSubtitleBufferContext *buf_ctx = &base_parse->buffer_ctx;

    if (buf_ctx->adapter != nullptr) {
        gst_adapter_clear(buf_ctx->adapter);
    }

    if (buf_ctx->text != nullptr) {
        (void)g_string_truncate(buf_ctx->text, 0);
    }
}

static gboolean sink_event_handle_flush_start(GstEvent *event, GstSubtitleBaseParse *self)
{
    g_return_val_if_fail((event != nullptr) && (self != nullptr) && (self->sinkpad != nullptr), FALSE);

    gboolean ret = FALSE;
    GstSubtitleBaseParseClass *baseclass = GST_SUBTITLE_BASE_PARSE_GET_CLASS(self);
    g_return_val_if_fail(baseclass != nullptr, FALSE);

    self->recv_eos = FALSE;
    g_mutex_lock(&self->buffermutex);
    self->flushing = TRUE;
    clear_buffer(self);
    g_mutex_unlock(&self->buffermutex);

    if (self->switching) {
        gst_event_unref(event);
        event = nullptr;
        GstStructure *change_state =
            gst_structure_new("Changestate_flag", "Changestate_flag", G_TYPE_BOOLEAN, FALSE, nullptr);
        g_return_val_if_fail(change_state != nullptr, ret);

        GstEvent *flush_event = gst_event_new_custom(GST_EVENT_FLUSH_START, change_state);
        if (flush_event != nullptr) {
            ret = gst_pad_event_default(self->sinkpad, (GstObject *)self, flush_event);
        }
    } else {
        ret = gst_pad_event_default(self->sinkpad, (GstObject *)self, event);
    }
    GST_INFO_OBJECT(self, "subtitle base flushing streams, stream_id: %d", self->stream_id);

    return ret;
}

static gboolean sink_event_handle_flush_stop(GstEvent *event, GstSubtitleBaseParse *self)
{
    gboolean ret = FALSE;

    g_return_val_if_fail((event != nullptr) && (self != nullptr) && (self->sinkpad != nullptr), FALSE);

    self->flushing = FALSE;

    if (self->switching) {
        gst_event_unref(event);
        event = gst_event_new_flush_stop(FALSE);
    }

    ret = gst_pad_event_default(self->sinkpad, (GstObject *)self, event);

    GST_INFO_OBJECT(self, "subtitle base flushed streams, stream_id: %d", self->stream_id);

    return ret;
}

static void sink_event_handle_caps_event(GstEvent *event, GstSubtitleBaseParse *self)
{
    g_return_if_fail((event != nullptr) && (self != nullptr));

    GstCaps *caps = nullptr;
    gboolean internal = FALSE;

    gst_event_parse_caps(event, &caps);
    g_return_if_fail(caps != nullptr);

    gchar *str_caps = gst_caps_to_string(caps);
    GstStructure *structure = gst_caps_get_structure(caps, 0);
    g_return_if_fail(structure != nullptr);
    if (!gst_structure_get_boolean(structure, "parsed", &internal)) {
        internal = FALSE;
    }
    const gchar *language = gst_structure_get_string(structure, "language");
    if (language != nullptr) {
        g_free(self->language);
        self->language = gst_subtitle_str_dup(language, FALSE, 0);
    }
    self->from_internal = internal;

    g_free(str_caps);
}

static gboolean default_handle_sink_event(GstSubtitleBaseParse *self, GstEvent *event)
{
    gboolean ret = FALSE;

    g_return_val_if_fail((self != nullptr) && (event != nullptr), FALSE);

    GST_INFO_OBJECT(self, "Handling %s event", GST_EVENT_TYPE_NAME(event));
    switch (GST_EVENT_TYPE(event)) {
        case GST_EVENT_EOS: {
            ret = sink_event_handle_segment_eos(event, self);
            break;
        }
        case GST_EVENT_SEGMENT: {
            ret = sink_event_handle_segment_event(event, self);
            break;
        }
        case GST_EVENT_FLUSH_START: {
            ret = sink_event_handle_flush_start(event, self);
            break;
        }
        case GST_EVENT_FLUSH_STOP: {
            ret = sink_event_handle_flush_stop(event, self);
            break;
        }
        case GST_EVENT_CAPS: {
            sink_event_handle_caps_event(event, self);
            g_return_val_if_fail(self->sinkpad != nullptr, ret);
            ret = gst_pad_event_default(self->sinkpad, (GstObject *)self, event);
            break;
        }
        case GST_EVENT_STREAM_START: {
            g_return_val_if_fail(self->sinkpad != nullptr, ret);
            ret = gst_pad_event_default(self->sinkpad, (GstObject *)self, event);
            break;
        }
        default: {
            g_return_val_if_fail(self->sinkpad != nullptr, ret);
            ret = gst_pad_event_default(self->sinkpad, (GstObject *)self, event);
            break;
        }
    }

    return ret;
}

static gboolean gst_subtitle_base_parse_sink_event(GstPad *pad, GstObject *parent, GstEvent *event)
{
    (void)pad;

    GstSubtitleBaseParse *self = static_cast<GstSubtitleBaseParse *>((void *)parent);
    GstSubtitleBaseParseClass *baseclass = GST_SUBTITLE_BASE_PARSE_GET_CLASS(self);
    gboolean ret = baseclass->on_sink_event_pfn(self, event);

    return ret;
}

static GstStateChangeReturn gst_subtitle_base_parse_change_state(GstElement *element, GstStateChange transition)
{
    GstSubtitleBaseParse *self = (GstSubtitleBaseParse *)element;
    GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

    g_return_val_if_fail(self != nullptr, GST_STATE_CHANGE_FAILURE);

    switch (transition) {
        case GST_STATE_CHANGE_READY_TO_PAUSED: {
            /* format detection will init the parser state */
            self->offset = 0;
            self->need_srcpad_caps = TRUE;
            self->first_buffer = TRUE;
            self->recv_eos = FALSE;
            self->first_segment = TRUE;
            break;
        }
        default: {
            break;
        }
    }

    if (g_parentClass != nullptr) {
        GstElementClass *element_class = (GstElementClass *)g_parentClass;
        if (element_class->change_state != nullptr) {
            ret = element_class->change_state(element, transition);
        }
    }

    g_return_val_if_fail(ret != GST_STATE_CHANGE_FAILURE, ret);

    switch (transition) {
        case GST_STATE_CHANGE_PAUSED_TO_READY: {
            self->need_srcpad_caps = TRUE;
            break;
        }
        default: {
            break;
        }
    }

    return ret;
}
