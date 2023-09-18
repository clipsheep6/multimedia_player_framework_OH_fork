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

#include "gst_subtitle_common.h"
#include "securec.h"
#include "gst_subtitle_base_parse_wrap.h"

namespace {
    constexpr gsize MAX_BUFFER_SIZE = 100000000;
    constexpr guint TYPEFIND_SIZE = 1025;
    constexpr guint TYPEFIND_MIN_SIZE = 9;
    constexpr guint BOM_OF_UTF_8 = 3;
    constexpr guint FIRST_INDEX_OF_UTF_8 = 0;
    constexpr guint SECOND_INDEX_OF_UTF_8 = 1;
    constexpr guint THIRD_INDEX_OF_UTF_8 = 2;
}

gchar *gst_subtitle_str_dup(const gchar *str, gboolean ndup, gsize len)
{
    g_return_val_if_fail(str != nullptr, nullptr);

    gsize dup_len = ndup ? len : (gsize)strlen(str);
    g_return_val_if_fail(dup_len <= MAX_BUFFER_SIZE, nullptr);

    return g_strndup(str, dup_len);
}

void gst_subtitle_free_frame(GstSubtitleBaseParse *base, GstSubtitleDecodedFrame *decoded_frame)
{
    g_return_if_fail((base != nullptr) && (decoded_frame != nullptr));

    if (decoded_frame->data != nullptr) {
        g_free(decoded_frame->data);
        decoded_frame->data = nullptr;
    }
    g_return_if_fail(memset_s(decoded_frame,
        sizeof(GstSubtitleDecodedFrame), 0, sizeof(GstSubtitleDecodedFrame)) == EOK);
}

static gchar *detect_encoding_and_convert_str(gchar **encoding, const gchar *str, guint len)
{
    g_return_val_if_fail((str != nullptr) && (len > 0), nullptr);

    gchar *ret = nullptr;

    if ((len >= BOM_OF_UTF_8) && ((guint8)str[FIRST_INDEX_OF_UTF_8] == 0xEF) &&
        ((guint8)str[SECOND_INDEX_OF_UTF_8] == 0xBB) && ((guint8)str[THIRD_INDEX_OF_UTF_8] == 0xBF)) {
        GST_INFO("utf-8 detected!");
        *encoding = g_strdup("UTF-8");
        g_return_val_if_fail(encoding != nullptr, nullptr);

        str += BOM_OF_UTF_8;
        len -= BOM_OF_UTF_8;
    } else {
        GST_INFO("not utf-8 detected!");
        *encoding = g_strdup("NOT-UTF-8");
        g_return_val_if_fail(encoding != nullptr, nullptr);
    }

    ret = static_cast<gchar *>(g_malloc0(len + 1));
    g_return_val_if_fail(ret != nullptr, nullptr);

    if (memcpy_s(ret, len, str, len) != EOK) {
        g_free(ret);
        ret = nullptr;
    }

    if (ret != nullptr) {
        ret[len] = '\0';
    }

    return ret;
}

static void caps_detect_handle(const gchar *encoding, GstTypeFind *tf,
    const GstSubtitleFormatDetect detect_caps_pfn, gchar *converted_str)
{
    GstCaps *caps = nullptr;

    g_return_if_fail(detect_caps_pfn != nullptr);
    caps = detect_caps_pfn(converted_str);

    if (caps != nullptr) {
        GST_DEBUG("subtitle encoding: %s", encoding);
        gst_type_find_suggest(tf, GST_TYPE_FIND_MAXIMUM, caps);
        gst_caps_unref(caps);
    }
}

static void probe_type_and_detect_caps(const gchar *str, guint tf_len,
    GstTypeFind *tf, const GstSubtitleFormatDetect detect_caps_pfn)
{
    g_return_if_fail((str != nullptr) && (tf != nullptr) && (detect_caps_pfn != nullptr) &&
        (tf_len >= TYPEFIND_MIN_SIZE));

    gchar *encoding = nullptr;
    gchar *converted_str = detect_encoding_and_convert_str(&encoding, str, tf_len - 1);
    if (converted_str == nullptr) {
        GST_DEBUG("Encoding detected but conversion failed");
        if (encoding != nullptr) {
            GST_DEBUG("Encoding is %s", encoding);
            g_free(encoding);
        }
        return;
    }

    /* call subclass caps detection function */
    caps_detect_handle(encoding, tf, detect_caps_pfn, converted_str);

    g_free(converted_str);
    g_free(encoding);
}

void gst_subtitle_typefind(GstTypeFind *tf, const gpointer priv,
    const GstSubtitleFormatDetect detect_caps_pfn)
{
    (void)priv;

    g_return_if_fail((tf != nullptr) && (detect_caps_pfn != nullptr));

    /* video or audio, no need to check characterset, the tf will mask sub */
    g_return_if_fail(!gst_type_find_is_mask_sub(tf));

    /* extract detected data */
    guint tf_len = (guint)gst_type_find_get_length(tf);
    tf_len = (tf_len >= TYPEFIND_SIZE) ? TYPEFIND_SIZE : tf_len;
    g_return_if_fail(tf_len >= TYPEFIND_MIN_SIZE);

    const guint8 *data = gst_type_find_peek(tf, (gint64)0, tf_len);
    g_return_if_fail(data != nullptr);

    gchar *str = static_cast<gchar *>(g_malloc0(tf_len));
    g_return_if_fail(str != nullptr);

    if (memcpy_s(str, tf_len, data, tf_len - 1) != EOK) {
        GST_ERROR("memcpy_s failed");
        g_free(str);
        return;
    }
    str[tf_len - 1] = '\0';

    probe_type_and_detect_caps(str, tf_len, tf, detect_caps_pfn);
    g_free(str);
}

/* read a line of text data from the external buffer */
gsize gst_subtitle_read_line(GstSubtitleBaseParse *base, gchar **out_line)
{
    g_return_val_if_fail((base != nullptr) && (out_line != nullptr), 0);

    gchar *str = nullptr;
    gsize consumed = 0;
    *out_line = nullptr;
    GstSubtitleBufferContext *buf_ctx = &base->buffer_ctx;

    g_return_val_if_fail(buf_ctx->text != nullptr, consumed);
    GST_DEBUG_OBJECT(base, "read line from the external buffer");

    while (TRUE) {
        g_return_val_if_fail((buf_ctx->text != nullptr) && (buf_ctx->text->str != nullptr), consumed);
        str = buf_ctx->text->str;

        if (str[0] == '\n') {
            buf_ctx->text = g_string_erase(buf_ctx->text, 0, 1);
            continue;
        }
        const char *line_end = strchr(str, '\n');
        if (line_end == nullptr) { // end of line not found, return for more data
            *out_line = nullptr;
            break;
        }
        gsize line_len = static_cast<gsize>(line_end - str);
        gchar *line = gst_subtitle_str_dup(str, TRUE, line_len + 1);
        g_return_val_if_fail(line != nullptr, consumed);

        buf_ctx->text = g_string_erase(buf_ctx->text, 0, (gssize)(line_len + 1));
        *out_line = line;
        consumed = line_len + 1;
        break;
    }

    return consumed;
}

/* encode the decoded subtitle frame @decoded_frame into gstbuffer and push it downstream */
GstFlowReturn gst_subtitle_push_buffer(GstSubtitleBaseParse *self,
    const GstSubtitleDecodedFrame *decoded_frame)
{
    GstFlowReturn ret = GST_FLOW_NOT_LINKED;

    g_return_val_if_fail((self != nullptr) && (decoded_frame != nullptr), ret);

    GstSubtitleStream *stream = gst_subtitle_get_stream_by_id(self, decoded_frame->stream_index);
    g_return_val_if_fail(stream != nullptr, GST_FLOW_NOT_LINKED);
    g_return_val_if_fail(handle_text_subtitle(self, decoded_frame, stream, &ret), GST_FLOW_ERROR);
    if (!self->from_internal) {
        ret = GST_FLOW_OK;
    }

    return ret;
}
