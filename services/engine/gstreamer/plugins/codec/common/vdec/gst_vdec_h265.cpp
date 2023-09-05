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

#include "gst_vdec_h265.h"
#include "securec.h"
#include "media_dfx.h"
#include "media_log.h"

G_DEFINE_TYPE(GstVdecH265, gst_vdec_h265, GST_TYPE_VDEC_BASE);

static constexpr gint HVCC_EXT_NALU_LENGTH_START_POS = 21;
static constexpr gint ANEEXB_HEAD = 4;
static gboolean gst_vdec_h265_parser(GstVdecBase *base, ParseMeta &meta);

static void gst_vdec_h265_class_init(GstVdecH265Class *klass)
{
    GST_DEBUG_OBJECT(klass, "Init h265 class");
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    GstVdecBaseClass *base_class = GST_VDEC_BASE_CLASS(klass);
    base_class->parser = gst_vdec_h265_parser;

    gst_element_class_set_static_metadata(element_class,
        "Hardware Driver Interface H.265 Video Decoder",
        "Codec/Decoder/Video/Hardware",
        "Decode H.265 video streams",
        "OpenHarmony");
    const gchar *sink_caps_string = "video/x-h265";
    GstCaps *sink_caps = gst_caps_from_string(sink_caps_string);

    if (sink_caps != nullptr) {
        GstPadTemplate *sink_templ = gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS, sink_caps);
        gst_element_class_add_pad_template(element_class, sink_templ);
        gst_caps_unref(sink_caps);
    }
}

static gboolean gst_vdec_h265_parser_codec_data(GstVdecH265 *self, GstMapInfo &src_info,
    GstMapInfo &dts_info, guint &copy_len)
{
    // The 21 byte is 2 bits for framerate 3 bits for tempLayers 1 bit for tmpIdNest 2 bits for nalu len minus one
    gint src_offset = HVCC_EXT_NALU_LENGTH_START_POS;
    self->hvcc_nal_len = (src_info.data[src_offset] & 3) + 1;
    // Next byte is 1 byte for array nums.
    src_offset++;
    gint hvcc_array_nums = src_info.data[src_offset];
    // Next byte is 1 byte type.
    src_offset++;
    gint dts_offset = 0;
    for (gint i = 0; i < hvcc_array_nums; ++i) {
        // Next byte is 2 bytes for type nums like two vps.
        src_offset++;
        gint type_num = (src_info.data[src_offset] << 8) + src_info.data[src_offset + 1];
        // Next 2 bytes is 2 bytes for type len like one vps len.
        src_offset += 2;
        for (gint j = 0; j < type_num; ++j) {
            dts_info.data[dts_offset] = 0;
            dts_info.data[dts_offset + 1] = 0;
            dts_info.data[dts_offset + 2] = 0;
            dts_info.data[dts_offset + 3] = 1;

            gint len = (src_info.data[src_offset] << 8) + src_info.data[src_offset + 1];
            // Next 2 bytes is type data.
            src_offset += 2;
            GST_DEBUG_OBJECT(self, "src_offset %d len %d", src_offset, len);
            auto ret = memcpy_s(dts_info.data + dts_offset + ANEEXB_HEAD, dts_info.size - dts_offset - ANEEXB_HEAD,
                src_info.data + src_offset, len);
            g_return_val_if_fail(ret == EOK, FALSE);
            dts_offset += (ANEEXB_HEAD + len);
            src_offset += len;
        }
    }
    g_return_val_if_fail(dts_offset >= 0, FALSE);
    copy_len = static_cast<guint>(dts_offset);

    return TRUE;
}

static gboolean gst_vdec_h265_parser_nalu(GstVdecH265 *self, const GstMapInfo &src_info,
    GstMapInfo &dts_info, guint &copy_len)
{
    gsize read_size = 0;
    gsize dts_info_size = dts_info.size;
    guint8 *p_src_data = src_info.data;
    guint8 *p_dts_data = dts_info.data;
    copy_len = 0;

    while (read_size < src_info.size) {
        /* ANEEXB_HEAD */
        p_dts_data[0] = 0;
        p_dts_data[1] = 0;
        p_dts_data[2] = 0;
        p_dts_data[3] = 1;

        guint byte_count = 0;
        guint hvcc_nal_size = static_cast<guint>(self->hvcc_nal_len);
        for (guint i = 0; i < hvcc_nal_size; ++i) {
            // two hexadecimal code in 1 byte(8 bit)
            byte_count = (byte_count << 8) + p_src_data[i];
        }
        read_size += hvcc_nal_size + byte_count;
        copy_len += ANEEXB_HEAD;
        auto ret = memcpy_s(p_dts_data + ANEEXB_HEAD, dts_info_size - copy_len,
            p_src_data + hvcc_nal_size, byte_count);
        g_return_val_if_fail(ret == EOK, FALSE);
        p_src_data += byte_count + hvcc_nal_size;
        p_dts_data += byte_count + ANEEXB_HEAD;
        copy_len += byte_count;
    }
    return TRUE;
}

static gboolean gst_vdec_h265_copy_info(const GstMapInfo &src_info, const GstMapInfo &dts_info, guint &copy_len)
{
    auto ret = memcpy_s(dts_info.data, dts_info.size, src_info.data, src_info.size);
    g_return_val_if_fail(ret == EOK, FALSE);
    copy_len = src_info.size;
    return TRUE;
}

static gboolean gst_vdec_h265_parser(GstVdecBase *base, ParseMeta &meta)
{
    GstVdecH265 *self = GST_VDEC_H265(base);
    g_return_val_if_fail(self != nullptr, FALSE);
    gboolean ret = TRUE;
    if (meta.is_codec_data) {
        ret = gst_vdec_h265_parser_codec_data(self, meta.src_info, meta.dts_info, meta.copy_len);
        self->is_hvcc = true;
    } else if (self->is_hvcc) {
        ret = gst_vdec_h265_parser_nalu(self, meta.src_info, meta.dts_info, meta.copy_len);
    } else {
        ret = gst_vdec_h265_copy_info(meta.src_info, meta.dts_info, meta.copy_len);
    }
    return ret;
}

static void gst_vdec_h265_init(GstVdecH265 *self)
{
    GstVdecBase *base = GST_VDEC_BASE(self);
    base->compress_format = OHOS::Media::GstCompressionFormat::GST_HEVC;
    self->is_hvcc = false;
}