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

#include "gst_subtitle_srt_parse.h"
#include <cstring>
#include "securec.h"
#include "gst/gst.h"
#include "gst_subtitle_common.h"

namespace {
    constexpr int32_t COLOR_VALUE_LEN = 7;
    constexpr gsize MAX_BUFFER_SIZE = 100000000;
    constexpr gsize MSEC_CONTAIN_BITS = 3;
    constexpr guint SRT_MAX_OPEN_TAGS_NUM = 32;
}

#define PARENT_CLASS gst_subtitle_srt_parse_parent_class
G_DEFINE_TYPE(GstSubtitleSrtParse, gst_subtitle_srt_parse, GST_TYPE_SUBTITLE_BASE_PARSE);

static GstStaticCaps g_srtCaps = GST_STATIC_CAPS("application/x-subtitle-srt");
#define SRT_CAPS (gst_static_caps_get(&g_srtCaps))

static GstStaticPadTemplate g_sinkTempl = GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
    GST_STATIC_CAPS("application/x-subtitle-srt"));

static GstStaticPadTemplate g_srcTempl = GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
    GST_STATIC_CAPS("text/x-raw, format={pango-markup,utf8}"));

static gboolean gst_subtitle_srt_parse_decode_frame(GstSubtitleBaseParse *base, const GstSubtitleFrame *frame,
    GstSubtitleDecodedFrame *decoded_frame);
static gsize gst_subtitle_srt_parse_read_frame(GstSubtitleBaseParse *base, GstSubtitleFrame *frame);
static GstCaps *gst_subtitle_srt_parse_get_src_caps(const GstSubtitleBaseParse *base, gint stream_id);
static void gst_subtitle_srt_parse_seek(GstSubtitleBaseParse *base, const GstEvent *event);
static void gst_subtitle_srt_parse_type_find(GstTypeFind *tf, gpointer priv);

static void gst_subtitle_srt_parse_dispose(GObject *object)
{
    GstSubtitleSrtParse *parse = GST_SUBTITLE_SRT_PARSE_CAST(object);
    g_return_if_fail(parse != nullptr);

    GST_INFO_OBJECT(parse, "srt parse dispose in");
    if (parse->buf != nullptr) {
        g_warn_if_fail(g_string_free(parse->buf, (gboolean)TRUE) == nullptr);
        parse->buf = nullptr;
    }

    G_OBJECT_CLASS(PARENT_CLASS)->dispose(object);
    GST_INFO_OBJECT(parse, "srt parse dispose out");
}

static void gst_subtitle_srt_parse_class_init(GstSubtitleSrtParseClass *kclass)
{
    g_return_if_fail(kclass != nullptr);

    GstElementClass *element_class = GST_ELEMENT_CLASS(kclass);
    GObjectClass *object_class = G_OBJECT_CLASS(kclass);
    GstSubtitleBaseParseClass *baseclass = GST_SUBTITLE_BASE_PARSE_CLASS(kclass);

    gst_element_class_set_static_metadata(element_class, "srt test based on baseclass", "Codec/Parser/Subtitle",
        "Parses srt subtitle", "OpenHarmony");
    gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&g_sinkTempl));
    gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&g_srcTempl));

    baseclass->decode_frame_pfn = gst_subtitle_srt_parse_decode_frame;
    baseclass->read_frame_pfn = gst_subtitle_srt_parse_read_frame;
    baseclass->get_srcpad_caps_pfn = gst_subtitle_srt_parse_get_src_caps;
    baseclass->on_seek_pfn = gst_subtitle_srt_parse_seek;

    object_class->dispose = gst_subtitle_srt_parse_dispose;
}

static void gst_subtitle_srt_parse_init(GstSubtitleSrtParse *parse)
{
    g_return_if_fail(parse != nullptr);

    GST_INFO_OBJECT(parse, "gst_subtitle_srt_parse_init in");

    parse->state = SUBNUM_STATE;
    if (parse->buf != nullptr) {
        g_warn_if_fail(g_string_truncate(parse->buf, 0) != nullptr);
    } else {
        parse->buf = g_string_new(nullptr);
        if (parse->buf == nullptr) {
            GST_WARNING_OBJECT(parse, "g_string_new failed");
        } else {
            if (parse->buf->str == nullptr) {
                (void)g_string_free(parse->buf, (gboolean)TRUE);
                parse->buf = nullptr;
            }
        }
    }
    parse->last_end = 0;

    GST_INFO_OBJECT(parse, "gst_subtitle_srt_parse_init out");
}

static gboolean gst_subtitle_srt_parse_decode_frame(GstSubtitleBaseParse *base, const GstSubtitleFrame *frame,
    GstSubtitleDecodedFrame *decoded_frame)
{
    GstSubtitleSrtParse *parse = GST_SUBTITLE_SRT_PARSE_CAST(base);

    g_return_val_if_fail((parse != nullptr) && (decoded_frame != nullptr), FALSE);

    g_return_val_if_fail((frame->len != 0) && (frame->len <= MAX_BUFFER_SIZE), FALSE);

    decoded_frame->data = static_cast<guint8 *>(g_malloc((gsize)frame->len + 1));
    g_return_val_if_fail(decoded_frame->data != nullptr, FALSE);

    if (memcpy_s(decoded_frame->data, frame->len + 1, frame->data, frame->len) != EOK) {
        GST_WARNING_OBJECT(parse, "srt memory copy failed");
        g_free(decoded_frame->data);
        decoded_frame->data = nullptr;
        return FALSE;
    }

    decoded_frame->data[frame->len] = 0;
    decoded_frame->duration = base->state.duration;
    decoded_frame->pts = base->state.start_time;
    decoded_frame->timestamp = decoded_frame->pts;
    decoded_frame->len = frame->len;
    parse->last_end = static_cast<gint64>(decoded_frame->pts + decoded_frame->duration);

    return TRUE;
}

static void srt_trailing_newlines(gchar *text, gsize len)
{
    if (text != nullptr) {
        while ((len > 1) && (text[len - 1] == '\n')) {
            text[len - 1] = '\0';
            --len;
        }
    }
}

static void srt_fix_up_markup_handle_slash_tag(const gchar **next_tag, guint *num_open_tags, const gchar *open_tags)
{
    ++(*next_tag);
    g_return_if_fail((*num_open_tags != 0) && (open_tags[*num_open_tags - 1] == **next_tag));
    /* it's all good, closing tag which is open */
    --(*num_open_tags);
}

static const gchar *string_token(const gchar *string, const gchar *delimiter, gchar **first)
{
    g_return_val_if_fail((string != nullptr) && (delimiter != nullptr) && (first != nullptr), nullptr);

    const gchar *next = static_cast<const gchar *>(strstr(string, delimiter));
    if (next != nullptr) {
        *first = g_strndup(string, (gsize)(next - string));
    } else {
        *first = g_strdup(string);
    }

    return next;
}

static gboolean srt_fix_up_markup_handle_f_tag(const gchar *next_tag)
{
    gchar *name = nullptr;
    gchar *attr_name = nullptr;
    gchar *attr_value = nullptr;
    gboolean ret = TRUE;

    const gchar *next = string_token(next_tag, " ", &name);
    if (next != nullptr) {
        next = string_token(next + 1, "=", &attr_name);
    }
    if ((next != nullptr) && (string_token(next + 1, ">", &attr_value) == nullptr)) {
        GST_WARNING("string_token failed");
    }
    if ((attr_name != nullptr) && (g_ascii_strcasecmp("color", attr_name) == 0)) {
        if (attr_value != nullptr && string_token(attr_value + 1, "\"", &attr_value)) {
            int32_t len = static_cast<int32_t>(strlen(attr_value));
            if ((*attr_value != '#') || (len != COLOR_VALUE_LEN)) {
                ret = FALSE;
                goto BEACH;
            }
        }
    } else {
        ret = FALSE;
        goto BEACH;
    }
BEACH:
    g_free(name);
    g_free(attr_name);
    g_free(attr_value);
    return ret;
}

static gboolean srt_remove_tag(gchar *des, const gchar *src)
{
    g_return_val_if_fail((des != nullptr) && (src != nullptr), FALSE);
    g_return_val_if_fail(memmove_s(des, strlen(des) + 1, src, strlen(src) + 1) == EOK, FALSE);
    return TRUE;
}

/* delete <...> */
static void srt_remove_tags(gchar *text)
{
    gchar *pos = nullptr;
    gchar *gt = nullptr;
    gboolean pos_bool = FALSE;

    for (pos = text; (pos != nullptr) && (*pos != '\0'); pos += (pos_bool ? 0 : 1)) {
        pos_bool = FALSE;
        gt = strstr(pos + 1, ">");
        if ((strncmp(pos, "<", (size_t)1) == 0) && (gt != nullptr)) {
            if (srt_remove_tag(pos, gt + strlen(">"))) {
                pos_bool = TRUE;
            }
        }
    }
}

static gboolean srt_fix_up_markup_handle(const gchar *next_tag, guint *num_open_tags,
    gchar **text, gchar *open_tags, guint open_tags_len)
{
    switch (*next_tag) {
        case '/': {
            srt_fix_up_markup_handle_slash_tag(&next_tag, num_open_tags, open_tags);
            break;
        }
        case 'i':
        case 'I':
        case 'b':
        case 'B':
        case 'u':
        case 'U':
        case 's': {
            if (*num_open_tags >= open_tags_len) {
                /* something dodgy is going on, stop parsing */
                return FALSE;
            }
            open_tags[*num_open_tags] = *next_tag;
            ++(*num_open_tags);
            break;
        }
        case 'f': {
            if (*num_open_tags >= open_tags_len) {
                /* something dodgy is going on, stop parsing */
                return FALSE;
            }
            if (srt_fix_up_markup_handle_f_tag(next_tag) == FALSE) {
                srt_remove_tags(*text);
                return FALSE;
            }
            open_tags[*num_open_tags] = *next_tag;
            ++(*num_open_tags);
            break;
        }
        default: {
            GST_ERROR("unexpected tag '%c' (%s)", *next_tag, next_tag);
            g_free(*text);
            (*text) = nullptr;
            return FALSE;
        }
    }

    return TRUE;
}

static GString *srt_fix_up_markup_add_tag(guint num_open_tags, const gchar *open_tags, const gchar *text)
{
    GString *s = g_string_new(text);
    g_return_val_if_fail(s != nullptr, nullptr);
    if (s->str == nullptr) {
        (void)g_string_free(s, (gboolean)TRUE);
        return nullptr;
    }

    while ((num_open_tags > 0) && (num_open_tags < SRT_MAX_OPEN_TAGS_NUM)) {
        GST_LOG("adding missing closing tag '%c'", open_tags[num_open_tags - 1]);
        g_warn_if_fail(g_string_append_c(s, '<') != nullptr);
        g_warn_if_fail(g_string_append_c(s, '/') != nullptr);
        if ((open_tags[num_open_tags - 1] == 'f') || (open_tags[num_open_tags - 1] == 's')) {
            g_warn_if_fail(g_string_append(s, "font") != nullptr);
        } else {
            g_warn_if_fail(g_string_append_c(s, open_tags[num_open_tags - 1]) != nullptr);
        }
        g_warn_if_fail(g_string_append_c(s, '>') != nullptr);
        --num_open_tags;
    }

    return s;
}

/* if only have the start tag, add the end tag */
static void srt_fix_up_markup(gchar **text)
{
    g_return_if_fail((text != nullptr) && (*text != nullptr));

    gchar open_tags[SRT_MAX_OPEN_TAGS_NUM] = {0};
    guint num_open_tags = 0;
    gchar *cur = *text;

    while ((*cur != '\0') && (num_open_tags < SRT_MAX_OPEN_TAGS_NUM)) {
        gchar *next_tag = strchr(cur, '<');
        if (next_tag == nullptr) {
            break;
        }
        ++next_tag;
        g_return_if_fail(srt_fix_up_markup_handle(next_tag, &num_open_tags,
            text, open_tags, SRT_MAX_OPEN_TAGS_NUM) != FALSE);
        cur = next_tag;
    }

    srt_remove_tags(*text);
    g_return_if_fail(num_open_tags != 0);

    GString *s = srt_fix_up_markup_add_tag(num_open_tags, open_tags, *text);
    g_return_if_fail(s != nullptr);
    g_free(*text);
    *text = g_string_free(s, FALSE);
}

static void srt_parse_calibrate_time(gchar *srt_str, guint len)
{
    /*
     * convert ' ' to '0'
     * or
     * convert '.' to ','
     * for example, "hh:mm:ss. 1 " is converted to "hh:mm:ss,010"
     */
    guint i;
    for (i = 0; i < len; i++) {
        if (srt_str[i] == ' ') {
            srt_str[i] = '0';
        } else if (srt_str[i] == '.') {
            srt_str[i] = ',';
        }
    }
}

static gboolean parse_timestamp(const gchar *srt_str, GstClockTime *timestamp)
{
    guint hour;
    guint min;
    guint sec;
    guint msec;

    if (sscanf_s(srt_str, "%u:%u:%u,%u", &hour, &min, &sec, &msec) != 4) { // 4 shows hour:min:sec,msec
        GST_WARNING("failed to parse subrip timestamp string '%s'", srt_str);
        return FALSE;
    }
    g_return_val_if_fail((hour <= 23) && // hour [0,23]
                         (min <= 59) &&  // min [0,59]
                         (sec <= 59) &&  // sec [0,59]
                         (msec <= 999),  // msec [0,999]
                         FALSE);

    guint64 hour_time = static_cast<guint64>(hour * 3600) * GST_SECOND; // 1 hour = 3600 secs
    guint64 min_time = static_cast<guint64>(min * 60) * GST_SECOND; // 1 min = 60 secs
    guint64 sec_time = static_cast<guint64>(sec) * GST_SECOND;
    guint64 msec_time = static_cast<guint64>(msec) * GST_MSECOND;
    *timestamp = hour_time + min_time + sec_time + msec_time;

    return TRUE;
}

static gboolean srt_parse_time(const gchar *ts_string, GstClockTime *timestamp)
{
    gchar srt_str[128] = {0}; // 128 shows the length of srt_str
    while (*ts_string == ' ') {
        ++ts_string;
    }

    g_return_val_if_fail(g_strlcpy(srt_str, ts_string, (gsize)sizeof(srt_str)) != 0, FALSE);
    gchar *end = strstr(srt_str, "-->");
    if (end != nullptr) {
        *end = '\0';
    }
    g_return_val_if_fail(g_strchomp(srt_str) != nullptr, FALSE);
    srt_parse_calibrate_time(srt_str, strlen(srt_str));

    /* make sure we have exactly three digits after comma */
    gchar *p = strchr(srt_str, ',');
    g_return_val_if_fail(p != nullptr, FALSE);
    ++p;
    gsize len = static_cast<gsize>(strlen(p));
    if (len > MSEC_CONTAIN_BITS) {
        p[MSEC_CONTAIN_BITS] = '\0';
    } else {
        while (len < MSEC_CONTAIN_BITS) {
            g_warn_if_fail(g_strlcat(&p[len], "0", 2) != 0); // 2 shows sizeof(&p[len])
            ++len;
        }
    }

    GST_LOG("srt parsing timestamp '%s'", srt_str);
    return parse_timestamp(srt_str, timestamp);
}

static gchar *parse_subsrt_time(GstSubtitleBaseParse *base, const gchar *line)
{
    GstSubtitleSrtParse *parse = GST_SUBTITLE_SRT_PARSE_CAST(base);
    GstClockTime ts_start;
    GstClockTime ts_end;
    const gchar *end_time = nullptr;

    /* looking for start_time --> end_time */
    if (line != nullptr) {
        end_time = static_cast<const gchar *>(strstr(line, " --> "));
    }
    if (end_time != nullptr) {
        if (srt_parse_time(line, &ts_start) && srt_parse_time(end_time + strlen(" --> "), &ts_end) &&
            (base->state.start_time <= ts_end)) {
            parse->state = SUBTEXT_STATE;
            base->state.start_time = ts_start;
            base->state.duration = ts_end - ts_start;
        } else {
            parse->state = SUBNUM_STATE;
        }
    } else {
        GST_DEBUG("error parsing subrip time line '%s'", line);
        parse->state = SUBNUM_STATE;
    }

    return nullptr;
}

static gchar *parse_subsrt_subtext(GstSubtitleBaseParse *base, const gchar *line)
{
    GstSubtitleSrtParse *parse = GST_SUBTITLE_SRT_PARSE_CAST(base);
    gchar *ret = nullptr;
    gchar *line_end = nullptr;
    g_return_val_if_fail(line != nullptr && parse->buf != nullptr, nullptr);

    line_end = const_cast<gchar *>(strchr(line, '\n'));
    if (line_end != nullptr) {
        if ((line[0] == '\r') && (line[1] == '\n')) {
            *(line_end - 1) = '\0';
        } else if (line[0] == '\n') {
            *(line_end) = '\0';
        }

        if ((strlen(line) > 1) && (line_end != line) && (*(line_end - 1) == '\r')) {
            *(line_end - 1) = '\n';
            *line_end = '\0';
        }
    }

    GST_DEBUG_OBJECT(parse, "parse->buf->len = %" G_GSIZE_FORMAT ", strlen(line) = %u",
        parse->buf->len, (uint32_t)strlen(line));

    g_warn_if_fail(g_string_append(parse->buf, line) != nullptr);

    if (strlen(line) == 0) {
        if (strlen(parse->buf->str)) {
            ret = g_strdup(parse->buf->str);
            g_return_val_if_fail(ret != nullptr, nullptr);
            GST_DEBUG_OBJECT(parse, "g_strdup success");
        } else {
            ret = static_cast<gchar *>(g_malloc0(2)); // assign 2 bytes storage
            g_return_val_if_fail(ret != nullptr, nullptr);
            GST_DEBUG_OBJECT(parse, "g_malloc0 success");
            ret[0] = 0;
        }

        g_warn_if_fail(g_string_truncate(parse->buf, 0) != nullptr);
        parse->state = SUBNUM_STATE;

        srt_trailing_newlines(ret, (gsize)strlen(ret));
        srt_fix_up_markup(&ret);
        return ret;
    }

    return nullptr;
}

static gchar *parse_subsrt(GstSubtitleBaseParse *base, const gchar *line)
{
    g_return_val_if_fail((line != nullptr) && (base != nullptr), nullptr);

    int subnum;
    GstSubtitleSrtParse *parse = GST_SUBTITLE_SRT_PARSE_CAST(base);

    switch (parse->state) {
        case SUBNUM_STATE: {
            /* looking for a single integer */
            if (sscanf_s(line, "%d", &subnum) == 1) {
                parse->state = TIME_STATE;
            }
            return nullptr;
        }
        case TIME_STATE: {
            return parse_subsrt_time(base, line);
        }
        case SUBTEXT_STATE: {
            return parse_subsrt_subtext(base, line);
        }
        default: {
            g_return_val_if_reached(nullptr);
        }
    }
}

static gsize read_frame_from_external(GstSubtitleBaseParse *base, GstSubtitleFrame *frame)
{
    GstSubtitleSrtParse *parse = GST_SUBTITLE_SRT_PARSE_CAST(base);
    gchar *subtitle = nullptr;
    gchar *line = nullptr;
    g_return_val_if_fail(parse->buf != nullptr, 0);

    gsize num = gst_subtitle_read_line(base, &line);
    if (num == 0) {
        g_return_val_if_fail(parse->buf->str != nullptr, 0);
        if (base->recv_eos && (parse->state == SUBTEXT_STATE) && (strlen(parse->buf->str) != 0)) {
            subtitle = g_strdup(parse->buf->str);
            g_return_val_if_fail(subtitle != nullptr, 0);
            g_warn_if_fail(g_string_truncate(parse->buf, 0) != nullptr);
            parse->state = SUBNUM_STATE;

            srt_trailing_newlines(subtitle, (gsize)strlen(subtitle));
            srt_fix_up_markup(&subtitle);
            if (subtitle != nullptr) {
                frame->data = (guint8 *)subtitle;
                frame->len = strlen(subtitle);
                return frame->len;
            } else {
                frame->data = nullptr;
                frame->len = 0;
            }
        }
        return 0;
    }

    subtitle = parse_subsrt(base, line);
    if (subtitle != nullptr) {
        frame->data = (guint8 *)subtitle;
        frame->len = strlen(subtitle);
    } else {
        frame->data = nullptr;
        frame->len = 0;
    }
    g_free(line);

    return num;
}

static gsize gst_subtitle_srt_parse_read_frame(GstSubtitleBaseParse *base, GstSubtitleFrame *frame)
{
    g_return_val_if_fail((base != nullptr) && (frame != nullptr), 0);

    gsize num = 0;

    if (!base->from_internal) {
        num = read_frame_from_external(base, frame);
    }

    return num;
}

static GstCaps *gst_subtitle_srt_parse_get_src_caps(const GstSubtitleBaseParse *base, gint stream_id)
{
    GstCaps *caps = nullptr;
    const gchar *language = "Unknown";
    (void)stream_id;

    caps = gst_caps_new_simple("text/x-raw", "format", G_TYPE_STRING, "pango-markup", nullptr);
    g_return_val_if_fail(caps != nullptr && base != nullptr, nullptr);

    if (base->language != nullptr) {
        language = base->language;
    }
    gst_caps_set_simple(caps, "language", G_TYPE_STRING, language, "type", G_TYPE_STRING, "SrtSubtitle", nullptr);

    return caps;
}

static void gst_subtitle_srt_parse_seek(GstSubtitleBaseParse *base, const GstEvent *event)
{
    GstSubtitleSrtParse *parse = GST_SUBTITLE_SRT_PARSE_CAST(base);
    g_return_if_fail((parse != nullptr) && (event != nullptr));

    GST_INFO_OBJECT(parse, "srt seek handling %s event", GST_EVENT_TYPE_NAME(event));
    parse->state = SUBNUM_STATE;
    base->state.start_time = 0;

    if (parse->buf != nullptr) {
        g_warn_if_fail(g_string_truncate(parse->buf, 0) != nullptr);
    }

    GST_INFO_OBJECT(parse, "srt seek out");
}

static GRegex *srt_parse_data_format_autodetect_regex_once(void)
{
    GError *gerr = nullptr;
    GRegex *result = g_regex_new("^[\\s\\n]{0,}[\\n]{0,1} {0,3}[ 0-9]{1,4}\\s{0,}(\x0d){0,1}\x0a"
        " {0,1}[0-9]{1,2}: {0,1}[0-9]{1,2}: {0,1}[0-9]{1,2}[,.] {0,2}[0-9]{1,3}"
        " +-{2}> +[0-9]{1,2}: {0,1}[0-9]{1,2}: {0,1}[0-9]{1,2}[,.] {0,2}[0-9]{1,3}",
        (GRegexCompileFlags)(G_REGEX_RAW | G_REGEX_OPTIMIZE), (GRegexMatchFlags)0, &gerr);
    if ((result == nullptr) && (gerr != nullptr)) {
        GST_WARNING("compilation of srt regex failed: %s", gerr->message);
    }
    if (gerr != nullptr) {
        g_error_free(gerr);
    }
    return result;
}

static GstCaps *srt_format_detect(const gchar *match_str)
{
    g_return_val_if_fail(match_str != nullptr, nullptr);

    GstCaps *caps = nullptr;
    GRegex *subrip_grx = nullptr;

    subrip_grx = srt_parse_data_format_autodetect_regex_once();
    if (g_regex_match(subrip_grx, match_str, (GRegexMatchFlags)0, nullptr)) {
        caps = SRT_CAPS;
    }
    g_regex_unref(subrip_grx);
    subrip_grx = nullptr;

    if (caps == nullptr) {
        GST_DEBUG("srt caps is nullptr");
    }

    return caps;
}

static void gst_subtitle_srt_parse_type_find(GstTypeFind *tf, gpointer priv)
{
    g_return_if_fail(tf != nullptr);

    GST_INFO("srt parse type find in");
    gst_subtitle_typefind(tf, priv, srt_format_detect);
}

gboolean gst_subtitle_srt_parse_register(GstPlugin *plugin)
{
    GST_INFO_OBJECT(plugin, "srt parse register in");
    g_return_val_if_fail(gst_type_find_register(plugin, "srt_typefind",
        GST_RANK_PRIMARY + 1, gst_subtitle_srt_parse_type_find, "srt,txt", SRT_CAPS, nullptr, nullptr), FALSE);
    g_return_val_if_fail(gst_element_register(plugin, "srt",
        GST_RANK_PRIMARY + 1, GST_TYPE_SUBTITLE_SRT_PARSE), FALSE);
    GST_INFO_OBJECT(plugin, "srt parse register out");
    return TRUE;
}
