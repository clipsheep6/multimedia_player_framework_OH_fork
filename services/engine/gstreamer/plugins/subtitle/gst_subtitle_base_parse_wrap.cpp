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

#include "gst_subtitle_base_parse_wrap.h"
#include <sys/time.h>
#include "securec.h"
#include "scope_guard.h"
#include "media_log.h"
#include "gst_subtitle_common.h"

using namespace OHOS;
using namespace OHOS::Media;

namespace {
    constexpr gsize MAX_BUFFER_SIZE = 100000000;
    constexpr guint64 SEC_TO_MSEC = 1000;
    constexpr guint BOM_OF_UTF_8 = 3;
    constexpr guint FIRST_INDEX_OF_UTF_8 = 0;
    constexpr guint SECOND_INDEX_OF_UTF_8 = 1;
    constexpr guint THIRD_INDEX_OF_UTF_8 = 2;
}

static gboolean src_event_seek_event_handle(GstSeekFlags *flags, gdouble *rate, GstSegment *seeksegment,
    GstSubtitleBaseParse *self, GstEvent *event)
{
    GstFormat format;
    GstSeekType start_type;
    GstSeekType stop_type;
    gint64 start;
    gint64 stop;
    gboolean update = FALSE;

    gst_event_parse_seek(event, rate, &format, flags, &start_type, &start, &stop_type, &stop);
    g_return_val_if_fail(format == GST_FORMAT_TIME, FALSE);

    if (self->segment != nullptr) {
        gst_segment_copy_into((GstSegment *)self->segment, seeksegment);
    }

    if (gst_segment_do_seek((GstSegment *)self->segment, *rate, format, *flags, start_type,
        (guint64)start, stop_type, (guint64)stop, &update)) {
        if ((*flags & GST_SEEK_FLAG_SNAP_NEAREST) == GST_SEEK_FLAG_SNAP_AFTER) {
            self->seek_snap_after = TRUE;
        } else {
            self->seek_snap_after = FALSE;
        }
        GST_INFO_OBJECT(self, "segment after seek: 0x%06" PRIXPTR ", seek_snap_after: %d",
            FAKE_POINTER(&self->segment), self->seek_snap_after);

        if ((self->segment != nullptr) && (self->event_segment != nullptr)) {
            gst_segment_copy_into((GstSegment *)self->segment, (GstSegment *)self->event_segment);
        }
    } else {
        if (self->segment != nullptr) {
            gst_segment_copy_into(seeksegment, (GstSegment *)self->segment);
        }
        GST_INFO_OBJECT(self, "segment fail after seek: 0x%06" PRIXPTR, FAKE_POINTER(&self->segment));
    }

    return TRUE;
}

static gboolean src_event_seek_event(const GstSubtitleBaseParseClass *baseclass,
    GstSubtitleBaseParse *self, GstEvent *event)
{
    GstSeekFlags flags;
    gdouble rate;
    GstSegment seeksegment;

    g_return_val_if_fail(self->last_seekseq != event->seqnum, FALSE);
    self->last_seekseq = event->seqnum;
    if (baseclass->on_seek_pfn != nullptr) {
        baseclass->on_seek_pfn(self, event);
    }

    gboolean err_ret = src_event_seek_event_handle(&flags, &rate, &seeksegment, self, event);
    g_return_val_if_fail(err_ret, FALSE);

    /* external subtitles push seek event upstream, internal subtitles push saved buffer when switched */
    if (!self->from_internal) {
        err_ret = gst_pad_push_event(self->sinkpad, gst_event_new_seek(rate, GST_FORMAT_BYTES, flags,
            GST_SEEK_TYPE_SET, (gint64)0, GST_SEEK_TYPE_NONE, (gint64)0));
        g_return_val_if_fail(!err_ret, TRUE);
        GST_WARNING_OBJECT(self, "seek to 0 bytes failed");
        if (self->segment != nullptr) {
            gst_segment_copy_into(&seeksegment, (GstSegment *)self->segment);
        }
    }

    return TRUE;
}

static GstSubtitleStream *gst_subtitle_base_parse_get_stream_by_pad(const GstSubtitleBaseParse *self,
    const GstPad *pad)
{
    GstSubtitleStream *stream = nullptr;
    gint index = 0;

    while (index < self->stream_num) {
        if (pad == self->streams[index]->pad) {
            stream = self->streams[index];
            break;
        }
        index++;
    }

    return stream;
}

static void gst_subtitle_base_parse_switch_stream(GstSubtitleBaseParse *self, const GstPad *pad, gboolean active)
{
    g_return_if_fail(self != nullptr && pad != nullptr);

    GstSubtitleBaseParseClass *baseclass = GST_SUBTITLE_BASE_PARSE_GET_CLASS(self);
    g_return_if_fail(baseclass != nullptr);

    GstSubtitleStream *stream = gst_subtitle_base_parse_get_stream_by_pad(self, pad);
    g_return_if_fail(stream != nullptr);

    stream->active = active;
    GST_INFO_OBJECT(self, "stream->stream_id: %d, active: %d", stream->stream_id, active);
}

static gboolean gst_subtitle_base_parse_src_event(GstPad *pad, GstObject *parent, GstEvent *event)
{
    g_return_val_if_fail((pad != nullptr) && (parent != nullptr) && (event != nullptr), FALSE);

    GstSubtitleBaseParse *self = static_cast<GstSubtitleBaseParse *>((void *)parent);
    GstSubtitleBaseParseClass *baseclass = GST_SUBTITLE_BASE_PARSE_GET_CLASS(self);
    ON_SCOPE_EXIT(0) {
        gst_event_unref(event);
    };
    g_return_val_if_fail(baseclass != nullptr, FALSE);
    gboolean ret = FALSE;

    GST_INFO_OBJECT(self, "Handling %s event", GST_EVENT_TYPE_NAME(event));
    switch (GST_EVENT_TYPE(event)) {
        case GST_EVENT_SEEK: {
            ret = src_event_seek_event(baseclass, self, event);
            gst_event_unref(event);
            event = nullptr;
            break;
        }
        case GST_EVENT_CUSTOM_UPSTREAM: {
            gboolean active = FALSE;

            const GstStructure *structure = gst_event_get_structure(event);
            if (structure == nullptr || gst_structure_get_name(structure) == nullptr) {
                gst_event_unref(event);
                break;
            }

            if (strcmp(gst_structure_get_name(structure), "select-stream") == 0) {
                if (!gst_structure_get_boolean(structure, "activity", &active)) {
                    active = TRUE;
                }
                gst_subtitle_base_parse_switch_stream(self, pad, active);
                self->switching = TRUE;
                return TRUE;
            } else {
                ret = gst_pad_event_default(pad, parent, event);
            }
            break;
        }
        default: {
            ret = gst_pad_event_default(pad, parent, event);
            break;
        }
    }
    CANCEL_SCOPE_EXIT_GUARD(0);
    return ret;
}

/* push gstbuffer to srcpad */
static GstFlowReturn gst_subtitle_base_push_data(GstSubtitleBaseParse *self, GstPad *pad, GstBuffer *buffer)
{
    GstFlowReturn ret = GST_FLOW_OK;
    guint64 clip_start = 0;
    guint64 clip_end = 0;
    gboolean in_segment = true;

    g_return_val_if_fail(buffer != nullptr, ret);
    ON_SCOPE_EXIT(0) {
        GST_WARNING_OBJECT(self, "Construct GstBuffer failed");
        gst_buffer_unref(buffer);
        buffer = nullptr;
    };
    g_return_val_if_fail(self != nullptr && pad != nullptr, ret);
    CANCEL_SCOPE_EXIT_GUARD(0);

    /*
     * Determine whether the display time interval of the buffer has an intersection with the segment.
     * If has an intersection, adjust the pts and duration of the buffer to the range of the segment.
     */
    if (self->seek_snap_after) {
        guint64 start_time = GST_BUFFER_PTS(buffer);
        guint64 end_time = start_time + GST_BUFFER_DURATION(buffer);
        in_segment = gst_segment_clip((GstSegment *)self->segment, GST_FORMAT_TIME, start_time, end_time,
            &clip_start, &clip_end);
        GST_BUFFER_PTS(buffer) = clip_start;
        GST_BUFFER_DURATION(buffer) = clip_end - clip_start;
    }

    if (in_segment && GST_IS_PAD(pad) && GST_PAD_IS_SRC(pad)) {
        ret = gst_pad_push(pad, buffer);
        if (G_LIKELY(ret != GST_FLOW_OK)) {
            GST_ERROR_OBJECT(self, "Push subtitle buffer failed, ret = %d", ret);
        } else {
            GST_INFO_OBJECT(self, "Push subtitle buffer success, pts = %" GST_TIME_FORMAT
                ", duration = %" GST_TIME_FORMAT, GST_TIME_ARGS(GST_BUFFER_PTS(buffer)),
                GST_TIME_ARGS(GST_BUFFER_DURATION(buffer)));
        }
    } else {
        GST_WARNING_OBJECT(self, "gst_subtitle_base_push_data:invalid time");
        gst_buffer_unref(buffer);
    }
    return ret;
}

gboolean handle_text_subtitle(GstSubtitleBaseParse *self, const GstSubtitleDecodedFrame *decoded_frame,
    GstSubtitleStream *stream, GstFlowReturn *ret)
{
    g_return_val_if_fail((self != nullptr) && (decoded_frame != nullptr)
        && (stream != nullptr) && (ret != nullptr), FALSE);
    g_return_val_if_fail(decoded_frame->len <= MAX_BUFFER_SIZE, FALSE);

    GstPad *pad = stream->pad;
    GstBuffer *buffer = gst_buffer_new_allocate(nullptr, (guint32)decoded_frame->len, nullptr);
    g_return_val_if_fail(buffer != nullptr, FALSE);
    (void)gst_buffer_fill(buffer, 0, decoded_frame->data, (guint32)decoded_frame->len);
    GST_BUFFER_PTS(buffer) = decoded_frame->pts;
    GST_BUFFER_DURATION(buffer) = decoded_frame->duration;

    if (!self->from_internal) {
        *ret = gst_subtitle_base_push_data(self, pad, buffer);
    }

    return TRUE;
}

static guint64 gst_subtitle_base_get_current_position(GstSubtitleBaseParse *self)
{
    g_return_val_if_fail(self != nullptr, 0);

    GstFormat fmt;
    guint64 position = GST_CLOCK_TIME_NONE;
    GstBin *parent_bin = nullptr;

    GstQuery *query = gst_query_new_position(GST_FORMAT_TIME);
    g_return_val_if_fail(query != nullptr, GST_CLOCK_TIME_NONE);
    GstBin *tmp_bin = static_cast<GstBin *>((void *)gst_element_get_parent(self));
    while (tmp_bin != nullptr) {
        parent_bin = tmp_bin;
        tmp_bin = static_cast<GstBin *>((void *)gst_element_get_parent(parent_bin));
        if (tmp_bin == nullptr) {
            break;
        }
        gst_object_unref(parent_bin);
        parent_bin = nullptr;
    }

    ON_SCOPE_EXIT(0) {
        gst_query_unref(query);
    };
    g_return_val_if_fail(parent_bin != nullptr, GST_CLOCK_TIME_NONE);

    gboolean ret = gst_element_query(static_cast<GstElement *>((void *)parent_bin), query);
    if (ret) {
        gst_query_parse_position(query, &fmt, (gint64 *)(&position));
    }

    gst_query_unref(query);
    query = nullptr;
    gst_object_unref(parent_bin);
    return (static_cast<gint64>(position) >= 0) ? position : GST_CLOCK_TIME_NONE;
}

GstSubtitleStream *gst_subtitle_get_stream_by_id(const GstSubtitleBaseParse *self, gint stream_id)
{
    g_return_val_if_fail(self != nullptr, nullptr);

    GstSubtitleStream *stream = nullptr;
    gint index = 0;

    while (index < self->stream_num) {
        if (stream_id == self->streams[index]->stream_id) {
            stream = self->streams[index];
            break;
        }
        index++;
    }

    return stream;
}

void free_subinfos_and_streams(GstSubtitleBaseParse *base_parse)
{
    gint index;
    g_return_if_fail(base_parse != nullptr);

    for (index = 0; index < base_parse->stream_num; index++) {
        if (base_parse->subinfos[index] != nullptr) {
            g_free(base_parse->subinfos[index]->desc);
            base_parse->subinfos[index]->desc = nullptr;
            g_free(base_parse->subinfos[index]);
            base_parse->subinfos[index] = nullptr;
        }

        if (base_parse->streams[index] != nullptr) {
            gst_caps_unref(base_parse->streams[index]->caps);
            base_parse->streams[index]->caps = nullptr;

            gst_tag_list_unref(base_parse->streams[index]->tags);
            base_parse->streams[index]->tags = nullptr;

            g_free(base_parse->streams[index]);
            base_parse->streams[index] = nullptr;
        }
    }
}

static void detect_sub_type_parse_extradata(const GstStructure *structure, GstSubtitleBaseParse *self)
{
    if (gst_structure_get_int(structure, "stream_id", &self->stream_id)) {
        GST_DEBUG_OBJECT(self, "stream_id:%d", self->stream_id);
    }
}

/*
 * Determine whether subtitles are internal or external based on sinkpad caps.
 * TRUE:  determine subtitle type(default: external)
 * FALSE: can not query caps of peer pad
 */
static gboolean gst_subtitle_base_parse_detect_sub_type(GstSubtitleBaseParse *self, GstPad *sinkpad)
{
    gboolean internal = FALSE;
    gboolean ret = FALSE;

    g_return_val_if_fail((self != nullptr) && (sinkpad != nullptr), ret);

    GstPad *peer = gst_pad_get_peer(sinkpad);
    g_return_val_if_fail(G_LIKELY(peer != nullptr), ret);
    GstCaps *caps = gst_pad_query_caps(peer, nullptr);
    g_object_unref(peer);
    peer = nullptr;
    g_return_val_if_fail(G_LIKELY(caps != nullptr), ret);

    GstStructure *structure = gst_caps_get_structure(caps, 0);
    ON_SCOPE_EXIT(0) {
        gst_caps_unref(caps);
    };
    g_return_val_if_fail(G_LIKELY(structure != nullptr), TRUE);

    if (!gst_structure_get_boolean(structure, "parsed", &internal)) {
        self->from_internal = FALSE;
    } else {
        self->from_internal = internal;
    }

    const gchar *language = gst_structure_get_string(structure, "language");
    if (language != nullptr) {
        g_free(self->language);
        self->language = gst_subtitle_str_dup(language, FALSE, 0);
    }

    /* extradata parsing */
    detect_sub_type_parse_extradata(structure, self);

    return TRUE;
}

gboolean handle_first_frame(GstPad *sinkpad, GstBuffer *buf, GstSubtitleBaseParse *self)
{
    g_return_val_if_fail(sinkpad != nullptr && buf != nullptr && self != nullptr, FALSE);

    if (self->first_buffer) {
        g_return_val_if_fail(gst_subtitle_base_parse_detect_sub_type(self, sinkpad), FALSE);
        self->first_buffer = FALSE;
    }

    return TRUE;
}

static gchar *check_utf8_encoding(GstSubtitleBaseParse *self, const gchar *str, gsize len)
{
    g_return_val_if_fail(str != nullptr && len > 0, nullptr);

    /* check if it's utf-8 */
    if (g_utf8_validate(str, (gint)len, nullptr)) {
        if ((len >= BOM_OF_UTF_8) && ((guint8)str[FIRST_INDEX_OF_UTF_8] == 0xEF) &&
            ((guint8)str[SECOND_INDEX_OF_UTF_8] == 0xBB) && ((guint8)str[THIRD_INDEX_OF_UTF_8] == 0xBF)) {
                str += BOM_OF_UTF_8;
                len -= BOM_OF_UTF_8;
        }
        return gst_subtitle_str_dup(str, TRUE, len);
    }

    GST_INFO_OBJECT(self, "invalid utf-8!");
    return gst_subtitle_str_dup(str, TRUE, len);
}

static void fill_buffer_from_adapter(GstSubtitleBaseParse *base_parse, GstSubtitleBufferContext *buf_ctx)
{
    gsize avail = gst_adapter_available(buf_ctx->adapter);
    gconstpointer data = gst_adapter_map(buf_ctx->adapter, avail);

    gchar *input = check_utf8_encoding(base_parse, (const gchar *)data, avail);

    gst_adapter_unmap(buf_ctx->adapter);
    if ((input != nullptr) && (buf_ctx->text != nullptr)) {
        buf_ctx->text = g_string_append(buf_ctx->text, input);
        gst_adapter_flush(buf_ctx->adapter, avail);
        GST_DEBUG_OBJECT(base_parse, "flush adapter size = %" G_GSIZE_FORMAT "", avail);
    }
    g_free(input);
}

/* write @buffer to buffer */
static void gst_subtitle_base_parse_fill_buffer(GstSubtitleBaseParse *base_parse, GstBuffer *buffer)
{
    g_return_if_fail(base_parse != nullptr);

    GstSubtitleBufferContext *buf_ctx = &base_parse->buffer_ctx;
    GstSubtitleBaseParseClass *kclass = GST_SUBTITLE_BASE_PARSE_GET_CLASS(base_parse);
    g_return_if_fail(kclass != nullptr);

    g_mutex_lock(&base_parse->buffermutex);
    /*
     * If it is external subtitle, store @buffer in GstAdapter,
     * then read as many strings as possible from the GstAdapter.
     */
    if (!base_parse->from_internal && buf_ctx->adapter != nullptr) {
        gst_adapter_push(buf_ctx->adapter, buffer);
        fill_buffer_from_adapter(base_parse, buf_ctx);
    }
    g_mutex_unlock(&base_parse->buffermutex);
}

static void get_stream_new_queue(GstSubtitleBaseParse *base_parse, GstSubtitleStream *stream)
{
    if (!base_parse->from_internal) {
        stream->last_result = GST_FLOW_OK;
    }
}

static GstSubtitleStream *gst_subtitle_get_stream_handle(GstSubtitleBaseParse *base_parse,
    const GstSubtitleInfo *info, GstPad *srcpad, GstCaps *caps)
{
    const gchar *language = nullptr;
    const gchar *format = nullptr;
    gboolean activity = FALSE;
    GstSubtitleStream *stream = static_cast<GstSubtitleStream *>(g_malloc0(sizeof(GstSubtitleStream)));
    g_return_val_if_fail(stream != nullptr, nullptr);

    stream->stream_id = info->stream_id;
    stream->pad = srcpad;
    stream->caps = gst_caps_ref(caps);
    GstStructure *caps_structure = gst_caps_get_structure(caps, 0);
    if (caps_structure != nullptr) {
        language = gst_structure_get_string(caps_structure, "language");
        format = gst_structure_get_string(caps_structure, "type");
    }

    if (info->stream_id == base_parse->stream_id) {
        activity = TRUE;
    }

    stream->tags = gst_tag_list_new(GST_TAG_LANGUAGE_CODE, language, GST_TAG_SUBTITLE_FORMAT, format,
        GST_TAG_SUBTITLE_STREAM_ACTIVITY_FLAG, activity ? 1 : 0, GST_TAG_SUBTITLE_TYPE,
        base_parse->from_internal, nullptr);
    GST_DEBUG_OBJECT(base_parse, "subtitle stream: %d, language: %s, format: %s, internal: %d",
        stream->stream_id, language, format, base_parse->from_internal);

    if (!gst_element_add_pad(GST_ELEMENT(base_parse), srcpad)) {
        GST_ERROR_OBJECT(base_parse, "gst_element_add_pad failed");
        g_object_unref(srcpad);
        gst_tag_list_unref(stream->tags);
        g_free(stream);
        return nullptr;
    }

    get_stream_new_queue(base_parse, stream);

    return stream;
}

static void get_stream_srcpad_set(GstSubtitleBaseParse *base_parse, GstPad *srcpad)
{
    gst_pad_set_event_function(srcpad, gst_subtitle_base_parse_src_event);
    base_parse->pad_num++;

    GST_DEBUG_OBJECT(base_parse, "adding src pad %s", GST_PAD_NAME(srcpad));
    gst_pad_use_fixed_caps(srcpad);
    g_return_if_fail(gst_pad_set_active(srcpad, (gboolean)TRUE));
}

GstSubtitleStream *gst_subtitle_get_stream(GstSubtitleBaseParse *base_parse, const GstSubtitleInfo *info)
{
    g_return_val_if_fail((base_parse != nullptr) && (info != nullptr), nullptr);

    GstCaps *caps = nullptr;
    GstSubtitleBaseParseClass *baseclass = GST_SUBTITLE_BASE_PARSE_GET_CLASS(base_parse);
    g_return_val_if_fail((baseclass != nullptr) && (baseclass->get_srcpad_caps_pfn != nullptr), nullptr);

    GString *padname = g_string_new(nullptr);
    g_return_val_if_fail(padname != nullptr, nullptr);
    ON_SCOPE_EXIT(0) {
        (void)g_string_free(padname, (gboolean)TRUE);
    };
    g_return_val_if_fail(padname->str != nullptr, nullptr);
    GstPadTemplate *src_pad_template = base_parse->srcpadtmpl;
    g_return_val_if_fail(src_pad_template != nullptr, nullptr);
    g_string_append_printf(padname, "src_%d", base_parse->pad_num);

    GstPad *srcpad = gst_pad_new_from_template(src_pad_template, padname->str);

    padname = nullptr;
    g_return_val_if_fail(srcpad != nullptr, nullptr);
    get_stream_srcpad_set(base_parse, srcpad);

    caps = baseclass->get_srcpad_caps_pfn(base_parse, info->stream_id);
    g_return_val_if_fail(caps != nullptr, nullptr);
    ON_SCOPE_EXIT(1) {
        gst_caps_unref(caps);
    };
    g_return_val_if_fail(gst_pad_set_caps(srcpad, caps), nullptr);
    return gst_subtitle_get_stream_handle(base_parse, info, srcpad, caps);
}

gboolean get_subtitle_streams(const GstSubtitleBaseParseClass *baseclass,
    GstBuffer *buf, GstSubtitleBaseParse *self)
{
    g_return_val_if_fail((baseclass != nullptr) && (buf != nullptr) && (self != nullptr), FALSE);

    gst_subtitle_base_parse_fill_buffer(self, buf);

    if (!self->got_streams) {
        if (self->subinfos[0] != nullptr) {
            g_free(self->subinfos[0]);
        }

        self->subinfos[0] = static_cast<GstSubtitleInfo *>(g_malloc0(sizeof(GstSubtitleInfo)));
        if (self->subinfos[0] != nullptr) {
            if (!self->from_internal) {
                self->stream_id = 0;
            }
            self->subinfos[0]->stream_id = self->stream_id;
            self->streams[0] = gst_subtitle_get_stream(self, self->subinfos[0]);
            if (self->streams[0] == nullptr) {
                g_free(self->subinfos[0]);
                self->subinfos[0] = nullptr;
            } else {
                self->stream_num++;
            }
        } else {
            GST_WARNING_OBJECT(self, "alloc substream info failed");
        }
        GST_INFO_OBJECT(self, "%s get subtitle streams success, stream_num %d",
            GST_ELEMENT_NAME(self), self->stream_num);
        self->got_streams = TRUE;
    }

    return TRUE;
}

void gst_subtitle_push_stream_start_event(GstSubtitleBaseParse *base_parse)
{
    g_return_if_fail(base_parse != nullptr);
    g_return_if_fail(base_parse->stream_num != 0);

    gint i;
    for (i = 0; i < base_parse->stream_num; i++) {
        g_return_if_fail(base_parse->streams[i] != nullptr);

        gchar *stream_name = g_strdup_printf("%s_stream%d", GST_ELEMENT_NAME(base_parse), i);
        GstEvent *event = gst_event_new_stream_start(stream_name);
        ON_SCOPE_EXIT(0) {
            g_free(stream_name);
        };
        g_return_if_fail(event != nullptr);
        CANCEL_SCOPE_EXIT_GUARD(0);
        const gchar *event_name = gst_event_type_get_name(GST_EVENT_TYPE(event));
        GST_DEBUG_OBJECT(base_parse, "pushing event %s on pad %s",
            event_name, GST_PAD_NAME(base_parse->streams[i]->pad));

        (void)gst_pad_push_event(base_parse->streams[i]->pad, event);
        g_free(stream_name);
    }
}

gboolean gst_subtitle_set_caps(GstSubtitleBaseParse *base_parse)
{
    g_return_val_if_fail(base_parse != nullptr, FALSE);

    gint i;
    if (base_parse->stream_num == 0) {
        return FALSE;
    }
    for (i = 0; i < base_parse->stream_num; i++) {
        g_return_val_if_fail(base_parse->streams[i] != nullptr &&
            gst_pad_set_caps(base_parse->streams[i]->pad, base_parse->streams[i]->caps), FALSE);
        GST_INFO_OBJECT(base_parse, "set caps on pad %s success", GST_PAD_NAME(base_parse->streams[i]->pad));
    }

    return TRUE;
}

gboolean gst_subtitle_set_tags(GstSubtitleBaseParse *base_parse)
{
    g_return_val_if_fail(base_parse != nullptr, FALSE);

    gint i;

    g_return_val_if_fail(base_parse->stream_num != 0, FALSE);
    for (i = 0; i < base_parse->stream_num; i++) {
        g_return_val_if_fail(base_parse->streams[i] != nullptr, FALSE);
        GstEvent *event = gst_event_new_tag(gst_tag_list_ref(base_parse->streams[i]->tags));
        ON_SCOPE_EXIT(0) {
            gst_tag_list_unref(base_parse->streams[i]->tags);
        };
        g_return_val_if_fail(event != nullptr, FALSE);
        GST_DEBUG_OBJECT(base_parse, "new event tag for streams[%d]", i);
        const gchar *event_name = gst_event_type_get_name(GST_EVENT_TYPE(event));
        GST_DEBUG_OBJECT(base_parse, "pushing taglist 0x%06" PRIXPTR " on pad %s",
            FAKE_POINTER(base_parse->streams[i]->tags), GST_PAD_NAME(base_parse->streams[i]->pad));
        g_return_val_if_fail(gst_pad_push_event(base_parse->streams[i]->pad, event), FALSE);
        GST_INFO_OBJECT(base_parse, "pad %s send event %s success", GST_PAD_NAME(base_parse->streams[i]->pad),
            event_name);
        CANCEL_SCOPE_EXIT_GUARD(0);
        gst_tag_list_unref(base_parse->streams[i]->tags);
    }
    return TRUE;
}

gboolean chain_set_caps_and_tags(GstSubtitleBaseParse *self)
{
    g_return_val_if_fail(self != nullptr, FALSE);

    gboolean need_tags = FALSE;

    if (G_UNLIKELY(self->need_srcpad_caps)) {
        g_return_val_if_fail(gst_subtitle_set_caps(self), FALSE);
        self->need_srcpad_caps = FALSE;
        need_tags = TRUE;
    }

    if (G_UNLIKELY(need_tags)) {
        g_return_val_if_fail(gst_subtitle_set_tags(self), TRUE);
        GST_INFO_OBJECT(self, "subtitle set taglist success");
    }

    return TRUE;
}

static gboolean gst_subtitle_send_event(GstSubtitleBaseParse *base_parse, GstEvent *event)
{
    g_return_val_if_fail((base_parse != nullptr) && (event != nullptr), FALSE);

    gint i;
    gboolean ret = TRUE;

    for (i = 0; i < base_parse->stream_num; i++) {
        if (base_parse->streams[i] == nullptr) {
            GST_ERROR_OBJECT(base_parse, "streams[%d] is nullptr", i);
            ret = FALSE;
        } else {
            if (!gst_pad_push_event(base_parse->streams[i]->pad, gst_event_ref(event))) {
                GST_ERROR_OBJECT(base_parse, "pad %s push event failed", GST_PAD_NAME(base_parse->streams[i]->pad));
                ret = FALSE;
            } else {
                GST_INFO_OBJECT(base_parse, "push event %s on pad %s success",
                    gst_event_type_get_name(GST_EVENT_TYPE(event)), GST_PAD_NAME(base_parse->streams[i]->pad));
            }
        }
    }

    gst_event_unref(event);
    return ret;
}

gboolean chain_push_new_segment_event(GstFlowReturn ret, GstSubtitleBaseParse *self)
{
    g_return_val_if_fail(self != nullptr, FALSE);

    g_mutex_lock(&self->segmentmutex);
    ON_SCOPE_EXIT(0) {
        g_mutex_unlock(&self->segmentmutex);
    };
    gboolean result = TRUE;
    if (G_UNLIKELY(self->need_segment)) {
        GST_INFO_OBJECT(self, "begin pushing newsegment event with 0x%06" PRIXPTR, FAKE_POINTER(&self->segment));

        if (self->first_segment) {
            self->event_segment->start = gst_subtitle_base_get_current_position(self);
            GST_WARNING_OBJECT(self, "first segment, segment start set cur position = %" GST_TIME_FORMAT,
                GST_TIME_ARGS(self->event_segment->start));
            self->first_segment = FALSE;
        }
        GstEvent *event = gst_event_new_segment((GstSegment *)self->event_segment);
        g_return_val_if_fail(event != nullptr, FALSE);
        if (gst_subtitle_send_event(self, gst_event_ref(event))) {
            GST_INFO_OBJECT(self, "end pushing newsegment event with 0x%06" PRIXPTR, FAKE_POINTER(&self->segment));
            self->need_segment = FALSE;
        } else {
            result = FALSE;
            GST_WARNING_OBJECT(self, "push segment failed: %d", ret);
        }
        gst_event_unref(event);
        event = nullptr;
    }
    return result;
}

gboolean decode_one_frame(const GstSubtitleBaseParseClass *baseclass, GstSubtitleBaseParse *self,
    GstSubtitleFrame *frame, GstSubtitleDecodedFrame *decoded_frame)
{
    g_return_val_if_fail((baseclass != nullptr) && (baseclass->decode_frame_pfn != nullptr) &&
        (self != nullptr) && (frame != nullptr), FALSE);
    if ((frame->data == nullptr) || (decoded_frame == nullptr)) {
        return FALSE;
    }

    struct timeval decode_start;
    struct timeval decode_end;

    (void)gettimeofday(&decode_start, nullptr);
    gboolean got_frame = baseclass->decode_frame_pfn(self, frame, decoded_frame);

    g_free(frame->data);
    frame->data = nullptr;

    ON_SCOPE_EXIT(0) {
        gst_subtitle_free_frame(self, decoded_frame);
    };
    g_return_val_if_fail(G_LIKELY(got_frame), FALSE);
    g_return_val_if_fail((decoded_frame->pts / GST_SECOND) <= G_MAXLONG, FALSE);
    (void)gettimeofday(&decode_end, nullptr);

    GST_DEBUG_OBJECT(self, "decode subtitle frame use time %" G_GUINT64_FORMAT " us",
        (guint64)(decode_end.tv_sec - decode_start.tv_sec) * SEC_TO_MSEC +
        (guint64)(decode_end.tv_usec - decode_start.tv_usec));
    CANCEL_SCOPE_EXIT_GUARD(0);
    return TRUE;
}
