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
#include "gst_mux_bin.h"
#include <fcntl.h>
#include <unistd.h>
#include "gstappsrc.h"
#include "gstbaseparse.h"

enum
{
  PROP_0,
  PROP_PATH,
  PROP_MUX,
  PROP_H264PARSE,
  PROP_MPEG4PARSE,
  PROP_AACPARSE,
};

#define gst_mux_bin_parent_class parent_class
G_DEFINE_TYPE(GstMuxBin, gst_mux_bin, GST_TYPE_PIPELINE);

static void gst_mux_bin_finalize(GObject *object);
static void gst_mux_bin_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *param_spec);
static void gst_mux_bin_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *param_spec);
static GstStateChangeReturn gst_mux_bin_change_state(GstElement *element, GstStateChange transition);

void add_track(GstMuxBin* mux_bin, TrackType type, const char *name)
{
    switch (type) {
        case VIDEO:
            mux_bin->videoTrack_ = g_strdup(name);
            break;
        case AUDIO:
            mux_bin->audioTrack_ = g_slist_append(mux_bin->audioTrack_, g_strdup(name));
            break;
        default:
            break;
    }
}

static void gst_mux_bin_class_init(GstMuxBinClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass *gstelement_class = GST_ELEMENT_CLASS(klass);
    GstBinClass *gstbin_class = GST_BIN_CLASS(klass);

    g_return_if_fail(gobject_class != nullptr && gstelement_class != nullptr && gstbin_class != nullptr);

    gobject_class->finalize = gst_mux_bin_finalize;
    gobject_class->set_property = gst_mux_bin_set_property;
    gobject_class->get_property = gst_mux_bin_get_property;

    g_object_class_install_property(gobject_class, PROP_PATH,
        g_param_spec_string("path", "Path", "Path of the output",
            nullptr, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_MUX,
        g_param_spec_string("mux", "Mux", "type of the mux",
            nullptr, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_H264PARSE,
        g_param_spec_boolean("h264parse", "h264Parse", "whether to need h264paese",
            false, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    
    g_object_class_install_property(gobject_class, PROP_MPEG4PARSE,
        g_param_spec_boolean("mpeg4parse", "mpeg4Parse", "whether to need mpeg4paese",
            false, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_AACPARSE,
        g_param_spec_boolean("aacparse", "aacParse", "whether to need aacpaese",
            false, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    gst_element_class_set_static_metadata(gstelement_class,
        "Mux Bin", "Bin/Mux",
        "Auto construct mux pipeline", "OpenHarmony");

    gstelement_class->change_state = gst_mux_bin_change_state;
}

static void gst_mux_bin_init(GstMuxBin *mux_bin)
{
    g_return_if_fail(mux_bin != nullptr);
    GST_INFO_OBJECT(mux_bin, "gst_mux_bin_init");
    mux_bin->audioSrcList_ = nullptr;
    mux_bin->h264parse_ = nullptr;
    mux_bin->mpeg4parse_ = nullptr;
    mux_bin->aacparse_ = nullptr;
    mux_bin->videoSrc_ = nullptr;
    mux_bin->splitMuxSink_ = nullptr;
    mux_bin->path_ = nullptr;
    mux_bin->mux_ = nullptr;
    mux_bin->h264parseFlag_ = false;
    mux_bin->mpeg4parseFlag_ = false;
    mux_bin->aacparseFlag_ = false;
    mux_bin->videoTrack_ = nullptr;
    mux_bin->audioTrack_ = nullptr;
    mux_bin->outFd_ = -1;
}

static void gst_mux_bin_finalize(GObject *object)
{
    GstMuxBin *mux_bin = GST_MUX_BIN(object);
    GST_INFO_OBJECT(mux_bin, "gst_mux_bin_finalize");
    g_return_if_fail(mux_bin != nullptr);
    if (mux_bin->outFd_ > 0) {
        (void)::close(mux_bin->outFd_);
        mux_bin->outFd_ = -1;
    }
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_mux_bin_set_property(GObject *object, guint prop_id,
    const GValue *value, GParamSpec *param_spec)
{
    (void)param_spec;
    GstMuxBin *mux_bin = GST_MUX_BIN(object);
    g_return_if_fail(mux_bin != nullptr);
    switch (prop_id) {
        case PROP_PATH:
            mux_bin->path_ = g_strdup(g_value_get_string(value));
            break;
        case PROP_MUX:
            mux_bin->mux_ = g_strdup(g_value_get_string(value));
            break;
        case PROP_H264PARSE:
            mux_bin->h264parseFlag_ = g_value_get_boolean(value);
            break;
        case PROP_MPEG4PARSE:
            mux_bin->mpeg4parseFlag_ = g_value_get_boolean(value);
            break;
        case PROP_AACPARSE:
            mux_bin->aacparseFlag_ = g_value_get_boolean(value);
            break;
        default:
            break;
    }
}

static void gst_mux_bin_get_property(GObject *object, guint prop_id,
    GValue *value, GParamSpec *param_spec)
{
    (void)param_spec;
    GstMuxBin *mux_bin = GST_MUX_BIN(object);
    g_return_if_fail(mux_bin != nullptr);
    switch (prop_id) {
        case PROP_PATH:
            g_value_set_string(value, mux_bin->path_);
            break;
        case PROP_MUX:
            g_value_set_string(value, mux_bin->mux_);
            break;
        case PROP_H264PARSE:
            g_value_set_boolean(value, mux_bin->h264parseFlag_);
            break;
        case PROP_MPEG4PARSE:
            g_value_set_boolean(value, mux_bin->mpeg4parseFlag_);
            break;
        case PROP_AACPARSE:
            g_value_set_boolean(value, mux_bin->aacparseFlag_);
            break;
        default:
            break;
    }
}

static GstStateChangeReturn cerate_splitmuxsink(GstMuxBin *mux_bin)
{
    g_return_val_if_fail(mux_bin->path_ != nullptr, GST_STATE_CHANGE_FAILURE);
    g_return_val_if_fail(mux_bin->mux_ != nullptr, GST_STATE_CHANGE_FAILURE);

    mux_bin->splitMuxSink_ = gst_element_factory_make("splitmuxsink", "splitmuxsink");
    g_return_val_if_fail(mux_bin->splitMuxSink_ != nullptr, GST_STATE_CHANGE_FAILURE);

    GstElement *fdsink = gst_element_factory_make("fdsink", "fdsink");
    g_return_val_if_fail(fdsink != nullptr, GST_STATE_CHANGE_FAILURE);
    mux_bin->outFd_ = open(mux_bin->path_, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (mux_bin->outFd_ < 0) {
        GST_ERROR_OBJECT(mux_bin, "Open file failed! filePath");
        return GST_STATE_CHANGE_FAILURE;
    }

    g_object_set(fdsink, "fd", mux_bin->outFd_, nullptr);
    g_object_set(mux_bin->splitMuxSink_, "sink", fdsink, nullptr);

    GstElement *qtmux = gst_element_factory_make(mux_bin->mux_, mux_bin->mux_);
    g_return_val_if_fail(qtmux != nullptr, GST_STATE_CHANGE_FAILURE);
    g_object_set(qtmux, "streamable", true, nullptr);
    g_object_set(mux_bin->splitMuxSink_, "muxer", qtmux, nullptr);
    // g_object_set(mux_bin->splitMuxSink_, "muxer", qtmux, "use-robust-muxing", true, nullptr);

    return GST_STATE_CHANGE_SUCCESS;
}

static GstStateChangeReturn create_h264parse(GstMuxBin *mux_bin)
{
    mux_bin->h264parse_ = gst_element_factory_make("h264parse", "h264parse");
    g_return_val_if_fail(mux_bin->h264parse_ != nullptr, GST_STATE_CHANGE_FAILURE);

    return GST_STATE_CHANGE_SUCCESS;
}

static GstStateChangeReturn create_mpeg4parse(GstMuxBin *mux_bin)
{
    mux_bin->mpeg4parse_ = gst_element_factory_make("mpeg4videoparse", "mpeg4parse");
    g_return_val_if_fail(mux_bin->mpeg4parse_ != nullptr, GST_STATE_CHANGE_FAILURE);
    g_object_set(mux_bin->mpeg4parse_, "config-interval", -1, "drop", false, nullptr);

    return GST_STATE_CHANGE_SUCCESS;
}

static GstStateChangeReturn create_aacparse(GstMuxBin *mux_bin)
{
    mux_bin->aacparse_ = gst_element_factory_make("aacparse", "aacparse");
    g_return_val_if_fail(mux_bin->aacparse_ != nullptr, GST_STATE_CHANGE_FAILURE);

    return GST_STATE_CHANGE_SUCCESS;
}

static GstStateChangeReturn create_video_src(GstMuxBin *mux_bin)
{
    g_return_val_if_fail(mux_bin->videoTrack_ != nullptr, GST_STATE_CHANGE_FAILURE);

    mux_bin->videoSrc_ = gst_element_factory_make("appsrc", mux_bin->videoTrack_);
    g_return_val_if_fail(mux_bin->videoSrc_ != nullptr, GST_STATE_CHANGE_FAILURE);
    g_object_set(mux_bin->videoSrc_, "is-live", true, "format", GST_FORMAT_TIME, nullptr);
    // g_object_set(mux_bin->videoSrc_, "format", GST_FORMAT_TIME, nullptr);

    return GST_STATE_CHANGE_SUCCESS;
}

static GstStateChangeReturn create_audio_src(GstMuxBin *mux_bin)
{
    GSList *iter = mux_bin->audioTrack_;
    while(iter != nullptr) {
        GstElement *appSrc = gst_element_factory_make("appsrc", (gchar *)(iter->data));
        g_return_val_if_fail(appSrc != nullptr, GST_STATE_CHANGE_FAILURE);
        g_object_set(appSrc, "is-live", true, "format", GST_FORMAT_TIME, nullptr);
        // g_object_set(appSrc, "format", GST_FORMAT_TIME, nullptr);
        mux_bin->audioSrcList_ = g_slist_append(mux_bin->audioSrcList_, appSrc);
        iter = iter->next;
    }

    return GST_STATE_CHANGE_SUCCESS;
}

static GstStateChangeReturn create_element(GstMuxBin *mux_bin)
{
    g_return_val_if_fail(mux_bin != nullptr, GST_STATE_CHANGE_FAILURE);

    if (cerate_splitmuxsink(mux_bin) != GST_STATE_CHANGE_SUCCESS) {
        return GST_STATE_CHANGE_FAILURE;
    }

    // if (create_h264parse(mux_bin) != GST_STATE_CHANGE_SUCCESS) {
    //     return GST_STATE_CHANGE_FAILURE;
    // }

    if (mux_bin->videoTrack_ != nullptr) {
        if (create_video_src(mux_bin) != GST_STATE_CHANGE_SUCCESS) {
            return GST_STATE_CHANGE_FAILURE;
        }
    }

    if (create_audio_src(mux_bin) != GST_STATE_CHANGE_SUCCESS) {
        return GST_STATE_CHANGE_FAILURE;
    }

    return GST_STATE_CHANGE_SUCCESS;
}

static GstStateChangeReturn add_element_to_bin(GstMuxBin *mux_bin)
{
    // g_return_val_if_fail(mux_bin != nullptr && mux_bin->videoSrc_ != nullptr &&
    //     mux_bin->audioSrcList_ != nullptr && mux_bin->splitMuxSink_ != nullptr, GST_STATE_CHANGE_FAILURE);
    // gst_bin_add(GST_BIN(mux_bin), mux_bin->h264parse_);
    if (mux_bin->videoTrack_ != nullptr) {
        gst_bin_add(GST_BIN(mux_bin), mux_bin->videoSrc_);
    }
    GSList *iter = mux_bin->audioSrcList_;
    while (iter != nullptr) {
        gst_bin_add(GST_BIN(mux_bin), GST_ELEMENT_CAST(iter->data));
        iter = iter->next;
    }
    gst_bin_add(GST_BIN(mux_bin), mux_bin->splitMuxSink_);

    return GST_STATE_CHANGE_SUCCESS;
}

static GstStateChangeReturn connect_element(GstMuxBin *mux_bin)
{
    // g_return_val_if_fail(mux_bin != nullptr && mux_bin->videoSrc_ != nullptr &&
    //     mux_bin->audioSrcList_ != nullptr && mux_bin->splitMuxSink_ != nullptr, GST_STATE_CHANGE_FAILURE);
    if (mux_bin->videoTrack_ != nullptr) {
        GstPad *video_src_pad = gst_element_get_static_pad(mux_bin->videoSrc_, "src");
        GstPad *split_mux_sink_sink_pad = gst_element_get_request_pad(mux_bin->splitMuxSink_, "video");
        if (mux_bin->h264parseFlag_ == true) {
            GstPad *parse_sink_pad = gst_element_get_static_pad(mux_bin->h264parse_, "sink");
            if (gst_pad_link(video_src_pad, parse_sink_pad) != GST_PAD_LINK_OK) {
                return GST_STATE_CHANGE_FAILURE;
            }

            GstPad *parse_src_pad = gst_element_get_static_pad(mux_bin->h264parse_, "src");
            if (gst_pad_link(parse_src_pad, split_mux_sink_sink_pad) != GST_PAD_LINK_OK) {
                return GST_STATE_CHANGE_FAILURE;
            }
        } else if (mux_bin->mpeg4parseFlag_ == true) {
            GstPad *parse_sink_pad = gst_element_get_static_pad(mux_bin->mpeg4parse_, "sink");
            if (gst_pad_link(video_src_pad, parse_sink_pad) != GST_PAD_LINK_OK) {
                return GST_STATE_CHANGE_FAILURE;
            }

            GstPad *parse_src_pad = gst_element_get_static_pad(mux_bin->mpeg4parse_, "src");
            if (gst_pad_link(parse_src_pad, split_mux_sink_sink_pad) != GST_PAD_LINK_OK) {
                return GST_STATE_CHANGE_FAILURE;
            }
        } else {
            if (gst_pad_link(video_src_pad, split_mux_sink_sink_pad) != GST_PAD_LINK_OK) {
                return GST_STATE_CHANGE_FAILURE;
            }
        }
    }
    // GstPad* video_src_pad = gst_element_get_static_pad(mux_bin->videoSrc_, "src");
    // GstPad* parse_sink_pad = gst_element_get_static_pad(mux_bin->h264parse_, "sink");
    // if (gst_pad_link(video_src_pad, parse_sink_pad) != GST_PAD_LINK_OK) {
    //     return GST_STATE_CHANGE_FAILURE;
    // }

    // GstPad* parse_src_pad = gst_element_get_static_pad(mux_bin->h264parse_, "src");
    // GstPad* split_mux_sink_sink_pad = gst_element_get_request_pad(mux_bin->splitMuxSink_, "video");
    // if (gst_pad_link(parse_src_pad, split_mux_sink_sink_pad) != GST_PAD_LINK_OK) {
    //     return GST_STATE_CHANGE_FAILURE;
    // }

    GSList *iter = mux_bin->audioSrcList_;
    while (iter != nullptr) {
        GstPad *audio_src_pad = gst_element_get_static_pad(GST_ELEMENT_CAST(iter->data), "src");
        GstPad *split_mux_sink_sink_pad = gst_element_get_request_pad(mux_bin->splitMuxSink_, "audio_%u");
        if (mux_bin->aacparseFlag_ == true) {
            GstPad *parse_sink_pad = gst_element_get_static_pad(mux_bin->aacparse_, "sink");
            if (gst_pad_link(audio_src_pad, parse_sink_pad) != GST_PAD_LINK_OK) {
                return GST_STATE_CHANGE_FAILURE;
            }

            GstPad *parse_src_pad = gst_element_get_static_pad(mux_bin->aacparse_, "src");
            if (gst_pad_link(parse_src_pad, split_mux_sink_sink_pad) != GST_PAD_LINK_OK) {
                return GST_STATE_CHANGE_FAILURE;
            }
        } else {
            if (gst_pad_link(audio_src_pad, split_mux_sink_sink_pad) != GST_PAD_LINK_OK) {
                return GST_STATE_CHANGE_FAILURE;
            }
        }
        iter = iter->next;
    }
    return GST_STATE_CHANGE_SUCCESS;
}

static GstStateChangeReturn gst_mux_bin_change_state(GstElement *element, GstStateChange transition)
{
    GstMuxBin *mux_bin = GST_MUX_BIN(element);
    GST_INFO_OBJECT(mux_bin, "gst_mux_bin_change_state");
    g_return_val_if_fail(mux_bin != nullptr, GST_STATE_CHANGE_FAILURE);

    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            if (create_element(mux_bin) != GST_STATE_CHANGE_SUCCESS) {
                GST_ERROR_OBJECT(mux_bin, "Failed to create element");
                return GST_STATE_CHANGE_FAILURE;
            }
            if (add_element_to_bin(mux_bin) != GST_STATE_CHANGE_SUCCESS) {
                GST_ERROR_OBJECT(mux_bin, "Failed to add element to bin");
                return GST_STATE_CHANGE_FAILURE;
            }
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            if (mux_bin->h264parseFlag_ == true) {
                if (create_h264parse(mux_bin) != GST_STATE_CHANGE_SUCCESS) {
                    GST_ERROR_OBJECT(mux_bin, "Failed to create h264parse");
                    return GST_STATE_CHANGE_FAILURE;
                }
                gst_bin_add(GST_BIN(mux_bin), mux_bin->h264parse_);
            }
            if (mux_bin->mpeg4parseFlag_ == true) {
                if (create_mpeg4parse(mux_bin) != GST_STATE_CHANGE_SUCCESS) {
                    GST_ERROR_OBJECT(mux_bin, "Failed to create mpeg4parse");
                    return GST_STATE_CHANGE_FAILURE;
                }
                gst_bin_add(GST_BIN(mux_bin), mux_bin->mpeg4parse_);
            }
            if (mux_bin->aacparseFlag_ == true) {
                if (create_aacparse(mux_bin) != GST_STATE_CHANGE_SUCCESS) {
                    GST_ERROR_OBJECT(mux_bin, "Failed to create aacparse");
                    return GST_STATE_CHANGE_FAILURE;
                }
                gst_bin_add(GST_BIN(mux_bin), mux_bin->aacparse_);
            }
            if (connect_element(mux_bin) != GST_STATE_CHANGE_SUCCESS) {
                GST_ERROR_OBJECT(mux_bin, "Failed to connect element");
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
    return gst_element_register(plugin, "muxbin", GST_RANK_PRIMARY, GST_TYPE_MUX_BIN);
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    _media_mux_bin,
    "GStreamer Mux Bin",
    plugin_init,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)