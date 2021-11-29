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

#include "config.h"
#include "gst_codec_bin.h"
#include <gst/gst.h>

enum {
    PROP_0,
    PROP_TYPE,
    PROP_USE_SOFTWARE,
    PROP_CODER_NAME,
    PROP_STREAM_INPUT,
    PROP_SRC,
    PROP_SINK,
    PROP_SRC_CONVERT,
    PROP_SINK_CONVERT
};

#define gst_codec_bin_parent_class parent_class
G_DEFINE_TYPE(GstCodecBin, gst_codec_bin, GST_TYPE_BIN);

static void gst_codec_bin_finalize(GObject *object);
static void gst_codec_bin_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *param_spec);
static void gst_codec_bin_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *param_spec);
static GstStateChangeReturn gst_codec_bin_change_state(GstElement *element, GstStateChange transition);

#define GST_TYPE_CODEC_BIN_TYPE (gst_codec_bin_type_get_type())
static GType gst_codec_bin_type_get_type(void)
{
    static GType codec_bin_type = 0;
    static const GEnumValue bin_types[] = {
        {CODEC_BIN_TYPE_VIDEO_DECODER, "video decoder", "video decoder"},
        {CODEC_BIN_TYPE_VIDEO_ENCODER, "video encoder", "video encoder"},
        {CODEC_BIN_TYPE_AUDIO_DECODER, "audio decoder", "audio decoder"},
        {CODEC_BIN_TYPE_AUDIO_ENCODER, "audio encoder", "audio encoder"},
        {CODEC_BIN_TYPE_UNKNOWN, "unknown", "unknown"},
        {0, nullptr, nullptr}
    };
    if (!codec_bin_type) {
        codec_bin_type = g_enum_register_static("CodecBinType", bin_types);
    }
    return codec_bin_type;
}

static void gst_codec_bin_class_init(GstCodecBinClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass *gstelement_class = GST_ELEMENT_CLASS(klass);
    GstBinClass *gstbin_class = GST_BIN_CLASS(klass);

    g_return_if_fail(gobject_class != nullptr && gstelement_class != nullptr && gstbin_class != nullptr);

    gobject_class->finalize = gst_codec_bin_finalize;
    gobject_class->set_property = gst_codec_bin_set_property;
    gobject_class->get_property = gst_codec_bin_get_property;

    g_object_class_install_property(gobject_class, PROP_TYPE,
        g_param_spec_enum("type", "CodecBin type", "Type of CodecBin",
            GST_TYPE_CODEC_BIN_TYPE, CODEC_BIN_TYPE_UNKNOWN,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_USE_SOFTWARE,
        g_param_spec_boolean("use-software", "Use software plugin", "Use software plugin",
            TRUE, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_CODER_NAME,
        g_param_spec_string("coder-name", "Name of coder", "Name of the coder plugin",
            nullptr, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_STREAM_INPUT,
        g_param_spec_boolean("stream-input", "Use streaming input", "Use streaming input",
            FALSE, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_SRC,
        g_param_spec_pointer("src", "Src plugin-in", "Src plugin-in",
            (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_SINK,
        g_param_spec_pointer("sink", "Sink plugin-in", "Sink plugin-in",
            (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_SRC_CONVERT,
        g_param_spec_boolean("src-convert", "Need input converter", "Need converter for input data",
            FALSE, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_SINK_CONVERT,
        g_param_spec_boolean("sink-convert", "Need output converter", "Need converter for output data",
            FALSE, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    gst_element_class_set_static_metadata(gstelement_class,
        "Codec Bin", "Bin/Decoder&Encoder",
        "Auto construct codec pipeline", "OpenHarmony");

    gstelement_class->change_state = gst_codec_bin_change_state;
}

static void gst_codec_bin_init(GstCodecBin *codec_bin)
{
    g_return_if_fail(codec_bin != nullptr);
    GST_INFO_OBJECT(codec_bin, "gst_codec_bin_init");
    codec_bin->src = nullptr;
    codec_bin->src_parser = nullptr;
    codec_bin->src_convert = nullptr;
    codec_bin->coder = nullptr;
    codec_bin->sink_parser = nullptr;
    codec_bin->sink_convert = nullptr;
    codec_bin->sink = nullptr;
    codec_bin->type = CODEC_BIN_TYPE_UNKNOWN;
    codec_bin->use_software = FALSE;
    codec_bin->coder_name = nullptr;
    codec_bin->stream_input = FALSE;
    codec_bin->need_src_convert = FALSE;
    codec_bin->need_sink_convert = FALSE;
}

static void gst_codec_bin_finalize(GObject *object)
{
    GstCodecBin *codec_bin = GST_CODEC_BIN(object);
    GST_INFO_OBJECT(codec_bin, "gst_codec_bin_finalize");
    g_return_if_fail(codec_bin != nullptr);
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_codec_bin_set_property(GObject *object, guint prop_id,
    const GValue *value, GParamSpec *param_spec)
{
    (void)param_spec;
    GstCodecBin *codec_bin = GST_CODEC_BIN(object);
    g_return_if_fail(codec_bin != nullptr);
    switch (prop_id) {
        case PROP_TYPE:
            codec_bin->type = static_cast<CodecBinType>(g_value_get_enum(value));
            break;
        case PROP_USE_SOFTWARE:
            codec_bin->use_software = g_value_get_boolean(value);
            break;
        case PROP_CODER_NAME:
            codec_bin->coder_name = g_strdup(g_value_get_string(value));
            break;
        case PROP_STREAM_INPUT:
            codec_bin->stream_input = g_value_get_boolean(value);
            break;
        case PROP_SRC:
            codec_bin->src = static_cast<GstElement *>(g_value_get_pointer(value));
            GST_INFO_OBJECT(codec_bin, "Set src element");
            break;
        case PROP_SINK:
            codec_bin->sink = static_cast<GstElement *>(g_value_get_pointer(value));
            GST_INFO_OBJECT(codec_bin, "Set sink element");
            break;
        case PROP_SRC_CONVERT:
            codec_bin->need_src_convert = g_value_get_boolean(value);
            break;
        case PROP_SINK_CONVERT:
            codec_bin->need_sink_convert = g_value_get_boolean(value);
            break;
        default:
            break;
    }
}

static void gst_codec_bin_get_property(GObject *object, guint prop_id,
    GValue *value, GParamSpec *param_spec)
{
    (void)param_spec;
    GstCodecBin *codec_bin = GST_CODEC_BIN(object);
    g_return_if_fail(codec_bin != nullptr);
    switch (prop_id) {
        case PROP_TYPE:
            g_value_set_enum(value, codec_bin->type);
            break;
        case PROP_USE_SOFTWARE:
            g_value_set_boolean(value, codec_bin->use_software);
            break;
        case PROP_CODER_NAME:
            g_value_set_string(value, codec_bin->coder_name);
            break;
        case PROP_STREAM_INPUT:
            g_value_set_boolean(value, codec_bin->stream_input);
            break;
        case PROP_SRC_CONVERT:
            g_value_set_boolean(value, codec_bin->need_src_convert);
            break;
        case PROP_SINK_CONVERT:
            g_value_set_boolean(value, codec_bin->need_sink_convert);
            break;
        default:
            break;
    }
}

static GstStateChangeReturn create_private_coder(GstCodecBin *codec_bin)
{
    GST_INFO_OBJECT(codec_bin, "create_private_coder");
    g_return_val_if_fail(codec_bin != nullptr, GST_STATE_CHANGE_FAILURE);
    GstStateChangeReturn ret = GST_STATE_CHANGE_FAILURE;
    switch(codec_bin->type) {
        case CODEC_BIN_TYPE_VIDEO_DECODER:
            codec_bin->coder = gst_element_factory_make("gsthdivdec", "codec_coder");
            if (codec_bin->coder != nullptr) {
                gst_bin_add(GST_BIN_CAST(codec_bin), codec_bin->coder);
                g_object_set(codec_bin->coder, "name", codec_bin->coder_name, nullptr);
                ret = gst_element_set_state(codec_bin->coder, GST_STATE_READY);
            }
            break;
        case CODEC_BIN_TYPE_VIDEO_ENCODER:
            codec_bin->coder = gst_element_factory_make("gsthdivenc", "codec_coder");
            if (codec_bin->coder != nullptr) {
                gst_bin_add((GST_BIN_CAST(codec_bin)), codec_bin->coder);
                g_object_set(codec_bin->coder, "name", codec_bin->coder_name, nullptr);
                ret = gst_element_set_state(codec_bin->coder, GST_STATE_READY);
            }
            break;
        case CODEC_BIN_TYPE_AUDIO_DECODER:
            GST_ERROR_OBJECT(codec_bin, "Unsupport");
            break;
        case CODEC_BIN_TYPE_AUDIO_ENCODER:
            GST_ERROR_OBJECT(codec_bin, "Unsupport");
            break;
        default:
            break;
    }
    return ret;
}

static GstStateChangeReturn create_software_coder(GstCodecBin *codec_bin)
{
    GST_INFO_OBJECT(codec_bin, "create_software_coder");
    g_return_val_if_fail(codec_bin != nullptr, GST_STATE_CHANGE_FAILURE);
    codec_bin->coder = gst_element_factory_make(codec_bin->coder_name, "dec");
    if (codec_bin->coder != nullptr) {
        gst_bin_add(GST_BIN_CAST(codec_bin), codec_bin->coder);
        return gst_element_set_state(codec_bin->coder, GST_STATE_READY);
    }
    GST_ERROR_OBJECT(codec_bin, "Failed to create_software_coder");
    return GST_STATE_CHANGE_FAILURE;
}

static GstStateChangeReturn create_coder(GstCodecBin *codec_bin)
{
    g_return_val_if_fail(codec_bin != nullptr, GST_STATE_CHANGE_FAILURE);
    GST_INFO_OBJECT(codec_bin, "create_coder enter");
    g_return_val_if_fail(codec_bin->coder_name != nullptr, GST_STATE_CHANGE_FAILURE);
    g_return_val_if_fail(codec_bin->type != CODEC_BIN_TYPE_UNKNOWN, GST_STATE_CHANGE_FAILURE);

    if (codec_bin->use_software == TRUE) {
        return create_software_coder(codec_bin);
    }
    return create_private_coder(codec_bin);
}

static GstStateChangeReturn connect_element(GstCodecBin *codec_bin)
{
    GST_INFO_OBJECT(codec_bin, "Begin to connect_element");
    g_return_val_if_fail(codec_bin != nullptr && codec_bin->src != nullptr &&
        codec_bin->coder != nullptr && codec_bin->sink != nullptr, GST_STATE_CHANGE_FAILURE);

    GstPad *src_src_pad = gst_element_get_static_pad(codec_bin->src, "src");
    GstPad *coder_sink_pad = gst_element_get_static_pad(codec_bin->coder, "sink");
    GstPad *coder_src_pad = gst_element_get_static_pad(codec_bin->coder, "src");
    GstPad *sink_sink_pad = gst_element_get_static_pad(codec_bin->sink, "sink");
    g_return_val_if_fail(src_src_pad != nullptr && coder_sink_pad != nullptr &&
        coder_src_pad != nullptr && sink_sink_pad != nullptr, GST_STATE_CHANGE_FAILURE);

    if (codec_bin->src_convert != nullptr) {
        GstPad *src_convert_sink_pad = gst_element_get_static_pad(codec_bin->src_convert, "sink");
        GstPad *src_convert_src_pad = gst_element_get_static_pad(codec_bin->src_convert, "src");
        if (gst_pad_link(src_src_pad, src_convert_sink_pad) != GST_PAD_LINK_OK) {
            GST_ERROR_OBJECT(codec_bin, "Failed to link src->src_convert");
            return GST_STATE_CHANGE_FAILURE;
        }
        if (gst_pad_link(src_convert_src_pad, coder_sink_pad) != GST_PAD_LINK_OK) {
            GST_ERROR_OBJECT(codec_bin, "Failed to link src_convert->coder");
            return GST_STATE_CHANGE_FAILURE;
        }
    } else {
        if (gst_pad_link(src_src_pad, coder_sink_pad) != GST_PAD_LINK_OK) {
            GST_ERROR_OBJECT(codec_bin, "Failed to link src->coder");
            return GST_STATE_CHANGE_FAILURE;
        }
    }

    if (codec_bin->sink_convert != nullptr) {
        GstPad *sink_convert_sink_pad = gst_element_get_static_pad(codec_bin->sink_convert, "sink");
        GstPad *sink_convert_src_pad = gst_element_get_static_pad(codec_bin->sink_convert, "src");
        if (gst_pad_link(coder_src_pad, sink_convert_sink_pad) != GST_PAD_LINK_OK) {
            GST_ERROR_OBJECT(codec_bin, "Failed to link coder->sink_convert");
            return GST_STATE_CHANGE_FAILURE;
        }
        if (gst_pad_link(sink_convert_src_pad, sink_sink_pad) != GST_PAD_LINK_OK) {
            GST_ERROR_OBJECT(codec_bin, "Failed to link sink_covert->sink");
            return GST_STATE_CHANGE_FAILURE;
        }
    } else {
        if (gst_pad_link(coder_src_pad, sink_sink_pad) != GST_PAD_LINK_OK) {
            GST_ERROR_OBJECT(codec_bin, "Failed to link coder->sink");
            return GST_STATE_CHANGE_FAILURE;
        }
    }
    GST_INFO_OBJECT(codec_bin, "connect_element success");
    return GST_STATE_CHANGE_SUCCESS;
}

static GstStateChangeReturn add_src_convert(GstCodecBin *codec_bin)
{
    g_return_val_if_fail(codec_bin != nullptr, GST_STATE_CHANGE_FAILURE);
    gboolean isVideo = (codec_bin->type == CODEC_BIN_TYPE_VIDEO_DECODER ||
        codec_bin->type == CODEC_BIN_TYPE_VIDEO_ENCODER) ? TRUE : FALSE;
    if (isVideo) {
        codec_bin->src_convert = gst_element_factory_make("videoconvert", "src_convert");
    } else {
        codec_bin->src_convert = gst_element_factory_make("audioconvert", "src_convert");
    }
    if (codec_bin->src_convert != nullptr) {
        gst_bin_add(GST_BIN_CAST(codec_bin), codec_bin->src_convert);
        return gst_element_set_state(codec_bin->src_convert, GST_STATE_READY);
    }
    return GST_STATE_CHANGE_FAILURE;
}

static GstStateChangeReturn add_sink_convert(GstCodecBin *codec_bin)
{
    g_return_val_if_fail(codec_bin != nullptr, GST_STATE_CHANGE_FAILURE);
    gboolean isVideo = (codec_bin->type == CODEC_BIN_TYPE_VIDEO_DECODER ||
        codec_bin->type == CODEC_BIN_TYPE_VIDEO_ENCODER) ? TRUE : FALSE;
    if (isVideo) {
        codec_bin->sink_convert = gst_element_factory_make("videoconvert", "sink_convert");
    } else {
        codec_bin->sink_convert = gst_element_factory_make("audioconvert", "sink_convert");
    }
    if (codec_bin->sink_convert != nullptr) {
        gst_bin_add(GST_BIN_CAST(codec_bin), codec_bin->sink_convert);
        return gst_element_set_state(codec_bin->sink_convert, GST_STATE_READY);
    }
    return GST_STATE_CHANGE_FAILURE;
}

static GstStateChangeReturn add_convert_if_necessary(GstCodecBin *codec_bin)
{
    g_return_val_if_fail(codec_bin != nullptr, GST_STATE_CHANGE_FAILURE);
    if (codec_bin->need_src_convert && add_src_convert(codec_bin) == GST_STATE_CHANGE_FAILURE) {
        GST_ERROR_OBJECT(codec_bin, "Failed to add_src_convert");
        return GST_STATE_CHANGE_FAILURE;
    }
    if (codec_bin->need_sink_convert && add_sink_convert(codec_bin) == GST_STATE_CHANGE_FAILURE) {
        GST_ERROR_OBJECT(codec_bin, "Failed to add_sink_convert");
        return GST_STATE_CHANGE_FAILURE;
    }
    return GST_STATE_CHANGE_SUCCESS;
}

static GstStateChangeReturn add_element_to_bin(GstCodecBin *codec_bin)
{
    GST_INFO_OBJECT(codec_bin, "begin to add_element_to_bin");
    g_return_val_if_fail(codec_bin != nullptr && codec_bin->src != nullptr &&
        codec_bin->sink != nullptr, GST_STATE_CHANGE_FAILURE);

    GST_INFO_OBJECT(codec_bin, "begin to add src element");
    gst_bin_add(GST_BIN_CAST(codec_bin), codec_bin->src);
    if (gst_element_set_state(codec_bin->src, GST_STATE_READY) == GST_STATE_CHANGE_FAILURE) {
        return GST_STATE_CHANGE_FAILURE;
    }

    gst_bin_add(GST_BIN_CAST(codec_bin), codec_bin->src);
    if (gst_element_set_state(codec_bin->src, GST_STATE_READY) == GST_STATE_CHANGE_FAILURE) {
        return GST_STATE_CHANGE_FAILURE;
    }

    GST_INFO_OBJECT(codec_bin, "begin to add parser element");
    codec_bin->src_parser = gst_element_factory_make("h264parse", "src_parse");
    g_return_val_if_fail(codec_bin->src_parser != nullptr, GST_STATE_CHANGE_FAILURE);
    gst_bin_add(GST_BIN_CAST(codec_bin), codec_bin->src_parser);
    if (gst_element_set_state(codec_bin->src_parser, GST_STATE_READY) == GST_STATE_CHANGE_FAILURE) {
        return GST_STATE_CHANGE_FAILURE;
    }

    if (add_convert_if_necessary(codec_bin) != GST_STATE_CHANGE_SUCCESS) {
        GST_ERROR_OBJECT(codec_bin, "Failed to add convert");
        return GST_STATE_CHANGE_FAILURE;
    }

    GST_INFO_OBJECT(codec_bin, "begin to add sink element");
    gst_bin_add(GST_BIN_CAST(codec_bin), codec_bin->sink);
    if (gst_element_set_state(codec_bin->sink, GST_STATE_READY) == GST_STATE_CHANGE_FAILURE) {
        return GST_STATE_CHANGE_FAILURE;
    }
    GST_INFO_OBJECT(codec_bin, "add_element_to_bin success");
    return GST_STATE_CHANGE_SUCCESS;
}

static GstStateChangeReturn gst_codec_bin_change_state(GstElement *element, GstStateChange transition)
{
    GstCodecBin *codec_bin = GST_CODEC_BIN(element);
    g_return_val_if_fail(codec_bin != nullptr && codec_bin->type != CODEC_BIN_TYPE_UNKNOWN, GST_STATE_CHANGE_FAILURE);

    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            if (create_coder(codec_bin) == GST_STATE_CHANGE_FAILURE) {
                GST_ERROR_OBJECT(codec_bin, "Failed to create_coder");
                return GST_STATE_CHANGE_FAILURE;
            }
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            if (add_element_to_bin(codec_bin) == GST_STATE_CHANGE_FAILURE) {
                GST_ERROR_OBJECT(codec_bin, "Failed to add_element_to_bin");
                return GST_STATE_CHANGE_FAILURE;
            }
            if (connect_element(codec_bin) == GST_STATE_CHANGE_FAILURE) {
                GST_ERROR_OBJECT(codec_bin, "Failed to connect_element");
                return GST_STATE_CHANGE_FAILURE;
            }
            break;
        default:
            break;
    }
    return GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
}

static gboolean plugin_init(GstPlugin *plugin)
{
    g_return_val_if_fail(plugin != nullptr, FALSE);
    return gst_element_register(plugin, "codecbin", GST_RANK_PRIMARY, GST_TYPE_CODEC_BIN);
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    _codec_bin,
    "GStreamer Codec Bin",
    plugin_init,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
