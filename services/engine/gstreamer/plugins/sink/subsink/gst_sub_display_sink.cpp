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

#include "config.h"
#include <gst/gst.h>
#include <cinttypes>
#include "gst_sub_sink.h"
#include "gst_sub_display_sink.h"
#include "gst_subtitle_meta.h"

using namespace OHOS::Media;
#define POINTER_MASK 0x00FFFFFF
#define FAKE_POINTER(addr) (POINTER_MASK & reinterpret_cast<uintptr_t>(addr))

static GstStaticPadTemplate g_sinktemplate = GST_STATIC_PAD_TEMPLATE("subdisplaysink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static void gst_sub_display_sink_dispose(GObject *obj);
static void gst_sub_display_sink_finalize(GObject *obj);
static void gst_sub_display_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static GstStateChangeReturn gst_sub_display_sink_change_state(GstElement *element, GstStateChange transition);
static GstFlowReturn gst_sub_display_sink_render (GstAppSink *appsink, gpointer user_data);
static gboolean gst_sub_display_sink_event(GstBaseSink *basesink, GstEvent *event);

#define gst_sub_display_sink_parent_class parent_class
G_DEFINE_TYPE(GstSubDisplaySink, gst_sub_display_sink, GST_TYPE_SUB_SINK);

GST_DEBUG_CATEGORY_STATIC(gst_sub_display_sink_debug_category);
#define GST_CAT_DEFAULT gst_sub_display_sink_debug_category

static void gst_sub_display_sink_class_init(GstSubDisplaySinkClass *kclass)
{
    g_return_if_fail(kclass != nullptr);

    GObjectClass *gobject_class = G_OBJECT_CLASS(kclass);
    GstElementClass *element_class = GST_ELEMENT_CLASS(kclass);
    GstBaseSinkClass *base_sink_class = GST_BASE_SINK_CLASS(kclass);
    GstSubSinkClass *subsink_class = GST_SUB_SINK_CLASS(kclass);
    gst_element_class_add_static_pad_template(element_class, &g_sinktemplate);

    gst_element_class_set_static_metadata(element_class,
        "SubDisplaySink", "Sink/Subtitle", " Sub Display Sink", "OpenHarmony");

    gobject_class->dispose = gst_sub_display_sink_dispose;
    gobject_class->finalize = gst_sub_display_sink_finalize;
    gobject_class->set_property = gst_sub_display_sink_set_property;
    element_class->change_state = gst_sub_display_sink_change_state;
    base_sink_class->event = gst_sub_display_sink_event;
    subsink_class->subtitle_display_callback = gst_sub_display_sink_render;
    GST_DEBUG_CATEGORY_INIT(gst_sub_display_sink_debug_category, "subdisplaysink", 0, "subdisplaysink class");
}

static void gst_sub_display_sink_init(GstSubDisplaySink *sub_display_sink)
{
    g_return_if_fail(sub_display_sink != nullptr);
}

static void gst_sub_display_sink_get_gst_buffer_info(GstBuffer *buffer, guint64 &gstPts, guint32 &duration)
{
    if (GST_BUFFER_PTS_IS_VALID(buffer)) {
        gstPts = GST_TIME_AS_MSECONDS(GST_BUFFER_PTS(buffer));
    } else {
        gstPts = -1;
    }
    if (GST_BUFFER_DURATION_IS_VALID(buffer)) {
        duration = GST_TIME_AS_MSECONDS(GST_BUFFER_DURATION(buffer));
    } else {
        duration = -1;
    }
}

static void gst_sub_display_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr);
    g_return_if_fail(value != nullptr);
    g_return_if_fail(pspec != nullptr);
    switch (prop_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static GstStateChangeReturn gst_sub_display_sink_change_state(GstElement *element, GstStateChange transition)
{
    g_return_val_if_fail(element != nullptr, GST_STATE_CHANGE_FAILURE);
    GstSubDisplaySink *sub_display_sink = GST_SUB_DISPLAY_SINK(element);
    switch (transition) {
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            GST_INFO_OBJECT(sub_display_sink, "sub displaysink has been stopped");
            break;
        default:
            break;
    }
    return GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
}

static GstFlowReturn gst_sub_display_sink_render(GstAppSink *appsink, gpointer user_data)
{
    (void)user_data;
    GstSubDisplaySink *sub_display_sink = GST_SUB_DISPLAY_SINK_CAST(appsink);
    GstSubSink *subsink = GST_SUB_SINK_CAST(appsink);
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    GstBuffer *buffer = gst_sample_get_buffer(sample);

    GST_INFO_OBJECT(sub_display_sink, "app render buffer 0x%06" PRIXPTR "", FAKE_POINTER(buffer));

    g_return_val_if_fail(buffer != nullptr, GST_FLOW_ERROR);
    guint64 pts = 0;
    guint32 duration = 0;
    gst_sub_display_sink_get_gst_buffer_info(buffer, pts, duration);
    if (!GST_CLOCK_TIME_IS_VALID(pts) || !GST_CLOCK_TIME_IS_VALID(duration)) {
        GST_ERROR_OBJECT(sub_display_sink, "pts or duration invalid");
        return GST_FLOW_ERROR;
    }
    GstSubSinkClass *subsink_class = GST_SUB_SINK_GET_CLASS(subsink);
    if (subsink_class->handle_buffer != nullptr) {
        subsink_class->handle_buffer(subsink, buffer, TRUE);
        subsink_class->handle_buffer(subsink, nullptr, FALSE, duration);
    }
    return GST_FLOW_OK;
}

static gboolean gst_sub_display_sink_event(GstBaseSink *basesink, GstEvent *event)
{
    GstSubDisplaySink *sub_display_sink = GST_SUB_DISPLAY_SINK_CAST(basesink);
    g_return_val_if_fail(sub_display_sink != nullptr, FALSE);
    g_return_val_if_fail(event != nullptr, FALSE);

    switch (event->type) {
        case GST_EVENT_EOS: {
            GST_INFO_OBJECT(sub_display_sink, "receiving EOS");
            break;
        }
        default:
            break;
    }
    return GST_BASE_SINK_CLASS(parent_class)->event(basesink, event);
}

static void gst_sub_display_sink_dispose(GObject *obj)
{
    g_return_if_fail(obj != nullptr);
    G_OBJECT_CLASS(parent_class)->dispose(obj);
}

static void gst_sub_display_sink_finalize(GObject *obj)
{
    g_return_if_fail(obj != nullptr);
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}