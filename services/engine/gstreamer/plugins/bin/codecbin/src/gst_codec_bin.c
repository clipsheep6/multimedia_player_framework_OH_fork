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

enum
{
  PROP_0,
  PROP_TYPE,
  PROP_USE_SOFTWARE,
  PROP_PLUGIN_NAME,
  PROP_STREAM_INPUT,
  PROP_SRC,
  PROP_SINK
};

static void gst_codec_bin_finalize(GObject *object);
static void gst_codec_bin_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *param_spec);
static void gst_codec_bin_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *param_spec);
static GstStateChangeReturn gst_codec_bin_change_state(GstElement *element, GstStateChange transition);

#define GST_TYPE_CODEC_BIN_TYPE (gst_codec_bin_type())
static GType gst_codec_bin_type(void)
{
    static GType codec_bin_type = 0;
    static const GEnumValue bin_types[] = {
        {CODEC_BIN_TYPE_UNKNOWN, "unknown", "unknown"},
        {CODEC_BIN_TYPE_VIDEO_DECODER, "video decoder", "video decoder"},
        {CODEC_BIN_TYPE_VIDEO_ENCODER, "video encoder", "video encoder"},
        {CODEC_BIN_TYPE_AUDIO_DECODER, "audio decoder", "audio decoder"},
        {CODEC_BIN_TYPE_AUDIO_ENCODER, "audio encoder", "audio encoder"},
        {0, NULL, NULL}
    };
    if (!codec_bin_type) {
        codec_bin_type = g_enum_register_static("CodecBinType", bin_types);
    }
    return codec_bin_type;
}

static void gst_codec_bin_class_init(GstAudioCaptureSrcClass *class)
{
    GObjectClass *gobject_class = (GObjectClass *)class;
    GstElementClass *gstelement_class = (GstElementClass *)class;
    GstBinClass *gstbin_class = (GstBinClass *)class;

    g_return_if_fail(gobject_class != NULL && gstelement_class != NULL && gstbin_class != NULL);

    gobject_class->finalize = gst_codec_bin_finalize;
    gobject_class->set_property = gst_codec_bin_set_property;
    gobject_class->get_property = gst_codec_bin_get_property;

    g_object_class_install_property(gobject_class, PROP_TYPE,
        g_param_spec_enum("type", "Plug-in type", "Type of plug-in",
            GST_TYPE_CODEC_BIN_TYPE, CODEC_BIN_TYPE_UNKNOWN,
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(gobject_class, PROP_USE_SOFTWARE,
        g_param_spec_boolean("use-software", "Use software plugin", "Use software plugin",
            TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(gobject_class, PROP_PLUGIN_NAME,
        g_param_spec_string("name", "Name", "Name of the decoder/encoder plugin",
            NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(gobject_class, PROP_STREAM_INPUT,
        g_param_spec_boolean("stream-input", "Use streaming input", "Use streaming input",
            FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(gobject_class, PROP_SRC,
        g_param_spec_pointer("src", "Src plugin-in", "Src plugin-in",
            G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(gobject_class, PROP_SINK,
        g_param_spec_pointer("sink", "Sink plugin-in", "Sink plugin-in",
            G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

    gst_element_class_set_static_metadata(gstelement_class,
        "Codec Bin", "Bin/Decoder&Encoder",
        "Auto construct codec pipeline", "OpenHarmony");

    gstelement_class->change_state = gst_codec_bin_change_state;
}

static void gst_codec_bin_init(GstCodecBin *codec_bin)
{
    g_return_if_fail(codec_bin != NULL);
    codec_bin->src = NULL;
    codec_bin->parser = NULL;
    codec_bin->coder = NULL;
    codec_bin->sink = NULL;

    codec_bin->type = CODEC_BIN_UNKNOWN;
    codec_bin->use_software = FALSE;
    codec_bin->plugin_name = NULL;
    codec_bin->stream_input = FALSE;
}

static void gst_codec_bin_finalize(GObject *object)
{
    GstCodecBin *codec_bin = GST_CODEC_BIN(object);
    g_return_if_fail(codec_bin != NULL);
}

static void gst_codec_bin_set_property(GObject *object, guint prop_id,
    const GValue *value, GParamSpec *param_spec)
{
    (void)param_spec;
    GstCodecBin *codec_bin = GST_CODEC_BIN(object);
    g_return_if_fail(codec_bin != NULL);
    switch (prop_id) {
        case PROP_TYPE:
            codec_bin->type = (GstCodecBinType)g_value_get_enum(value);
            break;
        case PROP_USE_SOFTWARE:
            codec_bin->use_software = g_value_get_boolean(value);
            break;
        case PROP_PLUGIN_NAME:
            codec_bin->plugin_name = g_value_get_string(value);
            break;
        case PROP_STREAM_INPUT:
            codec_bin->stream_input = g_value_get_boolean(value);
            break;
        case PROP_SRC:
            codec_bin->src = g_value_get_pointer(value);
            break;
        case PROP_SINK:
            codec_bin->sink = g_value_get_pointer(value);
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
    g_return_if_fail(codec_bin != NULL);
    switch (prop_id) {
        case PROP_TYPE:
            g_value_set_enum(value, codec_bin->type);
            break;
        case PROP_USE_SOFTWARE:
            g_value_set_boolean(value, codec_bin->use_software);
            break;
        case PROP_PLUGIN_NAME:
            g_value_set_string(value, codec_bin->plugin_name);
            break;
        case PROP_STREAM_INPUT:
            g_value_set_boolean(value, codec_bin->stream_input);
            break;
        default:
            break;
    }
}

static GstStateChangeReturn create_plugin(GstCodecBin *codec_bin)
{
    g_return_val_if_fail(codec_bin != NULL, GST_STATE_CHANGE_FAILURE);
    g_return_val_if_fail(codec_bin->plugin_name != NULL, GST_STATE_CHANGE_FAILURE);
    g_return_val_if_fail(codec_bin->type != CODEC_BIN_TYPE_UNKNOWN, GST_STATE_CHANGE_FAILURE);

    if (codec_bin->use_software == TRUE) {
        codec_bin->coder = gst_element_factory_make(codec_bin->plugin_name, "codec_plugin");
        if (codec_bin->coder != NULL) {
            gst_bin_add(codec_bin, codec_bin->coder);
            if (gst_element_set_state(codec_bin->coder, GST_STATE_READY) != GST_STATE_CHANGE_FAILURE) {
                return GST_STATE_CHANGE_SUCCESS;
            }
        }
        return GST_STATE_CHANGE_FAILURE
    }

    GstStateChangeReturn ret = GST_STATE_CHANGE_FAILURE;
    switch(codec_bin->type) {
        case CODEC_BIN_TYPE_VIDEO_DECODER:
            codec_bin->coder = gst_element_factory_make("gsthdivdec", "codec_plugin");
            if (codec_bin->coder != NULL) {
                gst_bin_add(codec_bin, codec_bin->coder);
                g_object_set(codec_bin->coder, "name", codec_bin->plugin_name, nullptr);
                ret = gst_element_set_state(codec_bin->coder, GST_STATE_READY);
            }
            break;
        case CODEC_BIN_TYPE_VIDEO_ENCODER:
            codec_bin->coder = gst_element_factory_make("gsthdivenc", "codec_plugin");
            if (codec_bin->coder != NULL) {
                gst_bin_add(codec_bin, codec_bin->coder);
                g_object_set(codec_bin->coder, "name", codec_bin->plugin_name, nullptr);
                ret = gst_element_set_state(codec_bin->coder, GST_STATE_READY);
            }
            break;
        case CODEC_BIN_TYPE_AUDIO_DECODER:
            // Unsupported
            break;
        case CODEC_BIN_TYPE_AUDIO_ENCODER:
            // Unsupported
            break;
        default:
            break;
    }
    return ret;
}

static GstStateChangeReturn connect_element(GstCodecBin *codec_bin)
{
    g_return_val_if_fail(codec_bin != NULL && codec_bin->src != NULL &&
        codec_bin->coder != NULL && codec_bin->sink != NULL, GST_STATE_CHANGE_FAILURE);

    GstPad *src_src = gst_element_get_static_pad(codec_bin->src, "src");
    GstPad *plugin_sink = gst_element_get_static_pad(codec_bin->coder, "sink");
    GstPad *plugin_src = gst_element_get_static_pad(codec_bin->coder, "src");
    GstPad *sink_sink = gst_element_get_static_pad(codec_bin->sink, "sink");
    g_return_val_if_fail(src_src != NULL && plugin_sink != NULL && plugin_src != NULL && sink_sink != NULL,
        GST_STATE_CHANGE_FAILURE);

    if (gst_pad_link(src_src, plugin_sink) != GST_PAD_LINK_OK) {
        return GST_STATE_CHANGE_FAILURE;
    }
    if (gst_pad_link(plugin_src, sink_sink) != GST_PAD_LINK_OK) {
        return GST_STATE_CHANGE_FAILURE;
    }
    return GST_STATE_CHANGE_SUCCESS;
}

static GstStateChangeReturn add_element_to_bin(GstCodecBin *codec_bin)
{
    g_return_val_if_fail(codec_bin != NULL && codec_bin->src != NULL &&
        codec_bin->sink != NULL, GST_STATE_CHANGE_FAILURE);

    gst_bin_add(codec_bin, codec_bin->src);
    gst_bin_add(codec_bin, codec_bin->sink);

    if (gst_element_set_state(codec_bin->src, GST_STATE_READY) != GST_STATE_CHANGE_SUCCESS) {
        return GST_STATE_CHANGE_FAILURE;
    }

    if (gst_element_set_state(codec_bin->sink, GST_STATE_READY) != GST_STATE_CHANGE_SUCCESS) {
        return GST_STATE_CHANGE_FAILURE;
    }
    return GST_STATE_CHANGE_SUCCESS;
}

static GstStateChangeReturn gst_audio_capture_src_change_state(GstElement *element, GstStateChange transition)
{
    GstCodecBin *codec_bin = GST_CODEC_BIN(element);
    g_return_val_if_fail(codec_bin != NULL, GST_STATE_CHANGE_FAILURE);

    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            if (create_plugin(codec_bin) != GST_STATE_CHANGE_SUCCESS) {
                return GST_STATE_CHANGE_FAILURE;
            }
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            if (add_element_to_bin() != GST_STATE_CHANGE_SUCCESS) {
                return GST_STATE_CHANGE_FAILURE;
            }
            if (connect_element(codec_bin) != GST_STATE_CHANGE_SUCCESS) {
                return GST_STATE_CHANGE_FAILURE;
            }
            break;
        case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
            break;
        case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
            break;
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            break;
        case GST_STATE_CHANGE_READY_TO_NULL:
            break;
        default:
            break;
    }
    return GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
}

static gboolean plugin_init(GstPlugin *plugin)
{
    g_return_val_if_fail(plugin != NULL, FALSE);
    return gst_element_register(plugin, "codecbin", GST_RANK_PRIMARY, GST_TYPE_CODEC_BIN);
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    _codec_bin,
    "GStreamer Codec Bin",
    plugin_init,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
