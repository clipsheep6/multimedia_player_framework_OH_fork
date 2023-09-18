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

#include "gst_subtitle_display_sink.h"
#include <cinttypes>
#include <gst/gst.h>
#include "gst_subtitle_sink.h"

using namespace OHOS::Media;

static GstStaticPadTemplate g_sinktemplate = GST_STATIC_PAD_TEMPLATE("subtitledisplaysink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static void gst_subtitle_display_sink_dispose(GObject *obj);
static void gst_subtitle_display_sink_finalize(GObject *obj);
static void gst_subtitle_display_sink_set_property(GObject *object,
    guint prop_id, const GValue *value, GParamSpec *pspec);
static GstStateChangeReturn gst_subtitle_display_sink_change_state(GstElement *element, GstStateChange transition);
static gboolean gst_subtitle_display_sink_event(GstBaseSink *basesink, GstEvent *event);

#define gst_subtitle_display_sink_parent_class parent_class
G_DEFINE_TYPE(GstSubtitleDisplaySink, gst_subtitle_display_sink, GST_TYPE_SUBTITLE_SINK);

GST_DEBUG_CATEGORY_STATIC(gst_subtitle_display_sink_debug_category);
#define GST_CAT_DEFAULT gst_subtitle_display_sink_debug_category

static void gst_subtitle_display_sink_class_init(GstSubtitleDisplaySinkClass *kclass)
{
    g_return_if_fail(kclass != nullptr);

    GObjectClass *gobject_class = G_OBJECT_CLASS(kclass);
    GstElementClass *element_class = GST_ELEMENT_CLASS(kclass);
    GstBaseSinkClass *base_sink_class = GST_BASE_SINK_CLASS(kclass);
    gst_element_class_add_static_pad_template(element_class, &g_sinktemplate);
    gst_element_class_set_static_metadata(element_class,
        "SutitlebDisplaySink", "Sink/Subtitle", " Subtitle Display Sink", "OpenHarmony");

    gobject_class->dispose = gst_subtitle_display_sink_dispose;
    gobject_class->finalize = gst_subtitle_display_sink_finalize;
    gobject_class->set_property = gst_subtitle_display_sink_set_property;
    element_class->change_state = gst_subtitle_display_sink_change_state;
    base_sink_class->event = gst_subtitle_display_sink_event;

    GST_DEBUG_CATEGORY_INIT(gst_subtitle_display_sink_debug_category,
        "subtitledisplaysink", 0, "subtitledisplaysink class");
}

static void gst_subtitle_display_sink_init(GstSubtitleDisplaySink *subtitle_display_sink)
{
    g_return_if_fail(subtitle_display_sink != nullptr);
}

static void gst_subtitle_display_sink_set_property(GObject *object,
    guint prop_id, const GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr);
    g_return_if_fail(value != nullptr);
    g_return_if_fail(pspec != nullptr);
    (void)prop_id;
}

static GstStateChangeReturn gst_subtitle_display_sink_change_state(GstElement *element, GstStateChange transition)
{
    g_return_val_if_fail(element != nullptr, GST_STATE_CHANGE_FAILURE);
    GstSubtitleDisplaySink *subtitle_display_sink = GST_SUBTITLE_DISPLAY_SINK_CAST(element);
    switch (transition) {
        case GST_STATE_CHANGE_PAUSED_TO_READY: {
            GST_INFO_OBJECT(subtitle_display_sink, "subtitle display sink stop");
            break;
        }
        default:
            break;
    }
    return GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
}

static gboolean gst_subtitle_display_sink_event(GstBaseSink *basesink, GstEvent *event)
{
    GstSubtitleDisplaySink *subtitle_display_sink = GST_SUBTITLE_DISPLAY_SINK_CAST(basesink);
    g_return_val_if_fail(subtitle_display_sink != nullptr, FALSE);
    g_return_val_if_fail(event != nullptr, FALSE);

    switch (event->type) {
        case GST_EVENT_EOS: {
            GST_INFO_OBJECT(subtitle_display_sink, "receiving EOS");
            break;
        }
        default:
            break;
    }
    return GST_BASE_SINK_CLASS(parent_class)->event(basesink, event);
}

static void gst_subtitle_display_sink_dispose(GObject *obj)
{
    g_return_if_fail(obj != nullptr);
    G_OBJECT_CLASS(parent_class)->dispose(obj);
}

static void gst_subtitle_display_sink_finalize(GObject *obj)
{
    g_return_if_fail(obj != nullptr);
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}