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

using namespace OHOS::Media;
#define POINTER_MASK 0x00FFFFFF
#define FAKE_POINTER(addr) (POINTER_MASK & reinterpret_cast<uintptr_t>(addr))

struct _GstSubSinkPrivate {
    GstCaps *caps;
    GMutex mutex;
    gboolean started;
    GstSubSinkCallbacks callbacks;
    gpointer userdata;
    GDestroyNotify notify;
    std::unique_ptr<TaskQueue> timer_queue;
};


static GstStaticPadTemplate g_sinktemplate = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static void gst_sub_sink_dispose(GObject *obj);
static void gst_sub_sink_finalize(GObject *obj);
static void gst_sub_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_sub_sink_handle_buffer(GstBaseSink *basesink, GstBuffer *buffer, gboolean cancel, guint64 delayUs = 0ULL);
static GstStateChangeReturn gst_sub_sink_change_state(GstElement *element, GstStateChange transition);
static GstFlowReturn gst_sub_sink_render (GstBaseSink * basesink, GstBuffer * buffer);
static gboolean gst_sub_sink_event(GstBaseSink *basesink, GstEvent *event);

#define gst_sub_sink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstSubSink, gst_sub_sink,
                        GST_TYPE_APP_SINK, G_ADD_PRIVATE(GstSubSink));

GST_DEBUG_CATEGORY_STATIC(gst_sub_sink_debug_category);
#define GST_CAT_DEFAULT gst_sub_sink_debug_category

static void gst_sub_sink_class_init(GstSubSinkClass *kclass)
{
    g_return_if_fail(kclass != nullptr);

    GObjectClass *gobject_class = G_OBJECT_CLASS(kclass);
    GstElementClass *element_class = GST_ELEMENT_CLASS(kclass);
    GstBaseSinkClass *base_sink_class = GST_BASE_SINK_CLASS(kclass);
    gst_element_class_add_static_pad_template(element_class, &g_sinktemplate);

    gst_element_class_set_static_metadata(element_class,
        "SubSink", "Sink/Subtitle", " Sub sink", "OpenHarmony");

    gobject_class->dispose = gst_sub_sink_dispose;
    gobject_class->finalize = gst_sub_sink_finalize;
    gobject_class->set_property = gst_sub_sink_set_property;
    element_class->change_state = gst_sub_sink_change_state;

    base_sink_class->render = gst_sub_sink_render;
    base_sink_class->event = gst_sub_sink_event;
    GST_DEBUG_CATEGORY_INIT(gst_sub_sink_debug_category, "subsink", 0, "subsink class");
}

static void gst_sub_sink_init(GstSubSink *sub_sink)
{
    g_return_if_fail(sub_sink != nullptr);

    sub_sink->is_flushing = FALSE;

    auto priv = reinterpret_cast<GstSubSinkPrivate *>(gst_sub_sink_get_instance_private(sub_sink));
    g_return_if_fail(priv != nullptr);
    sub_sink->priv = priv;
    g_mutex_init(&priv->mutex);

    priv->started = FALSE;
    priv->caps = nullptr;
    priv->callbacks.new_sample = nullptr;
    priv->userdata = nullptr;
    priv->notify = nullptr;
    sub_sink->priv->timer_queue = std::make_unique<TaskQueue>("GstSubSink");
    (void)sub_sink->priv->timer_queue->Start();
}

void gst_sub_sink_set_callback(GstSubSink *sub_sink, GstSubSinkCallbacks *callbacks,
                               gpointer user_data, GDestroyNotify notify)
{
    g_return_if_fail(GST_IS_SUB_SINK(sub_sink));
    g_return_if_fail(callbacks != nullptr);
    GstSubSinkPrivate *priv = sub_sink->priv;
    g_return_if_fail(priv != nullptr);

    GST_OBJECT_LOCK(sub_sink);
    GDestroyNotify old_notify = priv->notify;
    if (old_notify) {
        gpointer old_data = priv->userdata;
        priv->userdata = nullptr;
        priv->notify = nullptr;

        GST_OBJECT_UNLOCK(sub_sink);
        old_notify(old_data);
        GST_OBJECT_LOCK(sub_sink);
    }

    priv->callbacks = *callbacks;
    priv->userdata = user_data;
    priv->notify = notify;
    GST_OBJECT_UNLOCK(sub_sink);
}

static void gst_sub_sink_get_gst_buffer_info(GstBuffer *buffer, guint64 &gstPts, guint32 &duration)
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

static void gst_sub_sink_handle_buffer(GstBaseSink *basesink, GstBuffer *buffer, gboolean cancel, guint64 delayUs)
{
    GstSubSink *sub_sink = GST_SUB_SINK_CAST(basesink);
    GstSubSinkPrivate *priv = sub_sink->priv;
    g_return_if_fail(priv != nullptr);

    auto handler = std::make_shared<TaskHandler<void>>([=]() {
        if (priv->callbacks.new_sample != nullptr) {
            (void)priv->callbacks.new_sample(buffer, priv->userdata);
        }
    });
    priv->timer_queue->EnqueueTask(handler, cancel, delayUs);
}

static void gst_sub_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr);
    g_return_if_fail(value != nullptr);
    g_return_if_fail(pspec != nullptr);

    GstSubSink *sub_sink = GST_SUB_SINK_CAST(object);
    GstSubSinkPrivate *priv = sub_sink->priv;
    g_return_if_fail(priv != nullptr);
    switch (prop_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static GstStateChangeReturn gst_sub_sink_change_state(GstElement *element, GstStateChange transition)
{
    g_return_val_if_fail(element != nullptr, GST_STATE_CHANGE_FAILURE);
    GstSubSink *sub_sink = GST_SUB_SINK(element);
    GstBaseSink *basesink = GST_BASE_SINK(element);
    GstSubSinkPrivate *priv = sub_sink->priv;
    g_return_val_if_fail(priv != nullptr, GST_STATE_CHANGE_FAILURE);
    switch (transition) {
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            gst_sub_sink_handle_buffer(basesink, nullptr, true);
            priv->timer_queue->Stop();
            GST_INFO_OBJECT(sub_sink, "sub sink has been stopped");
            break;
        default:
            break;
    }
    return GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
}

static GstFlowReturn gst_sub_sink_render(GstBaseSink * basesink, GstBuffer * buffer)
{
    GstSubSink *sub_sink = GST_SUB_SINK_CAST(basesink);
    g_return_val_if_fail(sub_sink != nullptr, GST_FLOW_ERROR);
    GstSubSinkPrivate *priv = sub_sink->priv;
    g_return_val_if_fail(priv != nullptr, GST_FLOW_ERROR);
    GstSubSinkClass *sub_sink_class = GST_SUB_SINK_GET_CLASS(sub_sink);
    g_return_val_if_fail(sub_sink_class != nullptr, GST_FLOW_ERROR);

    g_mutex_lock(&priv->mutex);
    if (!priv->started) {
        GST_INFO_OBJECT(sub_sink, "we are not started");
        g_mutex_unlock(&priv->mutex);
        return GST_FLOW_FLUSHING;
    }
    g_mutex_unlock(&priv->mutex);

    if (sub_sink->is_flushing) {
        GST_INFO_OBJECT(sub_sink, "we are flushing");
        return GST_FLOW_FLUSHING;
    }

    GST_INFO_OBJECT(sub_sink, "app render buffer 0x%06" PRIXPTR "", FAKE_POINTER(buffer));

    guint64 pts = 0;
    guint32 duration = 0;
    gst_sub_sink_get_gst_buffer_info(buffer, pts, duration);
    
    g_return_val_if_fail(priv != nullptr, GST_FLOW_ERROR);
    gint64 delay_us = pts - GST_TIME_AS_MSECONDS(gst_util_get_timestamp());
    gst_sub_sink_handle_buffer(basesink, buffer, FALSE, delay_us);
    gst_sub_sink_handle_buffer(basesink, nullptr, FALSE, pts + duration);
    return GST_FLOW_OK;
}

static gboolean gst_sub_sink_event(GstBaseSink *basesink, GstEvent *event)
{
    GstSubSink *sub_sink = GST_SUB_SINK_CAST(basesink);
    g_return_val_if_fail(sub_sink != nullptr, FALSE);
    GstSubSinkPrivate *priv = sub_sink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);
    g_return_val_if_fail(event != nullptr, FALSE);

    switch (event->type) {
        case GST_EVENT_EOS: {
            GST_INFO_OBJECT(sub_sink, "receiving EOS");
            break;
        }
        case GST_EVENT_FLUSH_START: {
            gst_sub_sink_handle_buffer(basesink, nullptr, TRUE, 0ULL);
            sub_sink->is_flushing = TRUE;
            break;
        }
        case GST_EVENT_FLUSH_STOP: {
            sub_sink->is_flushing = FALSE;
            break;
        }
        default:
            break;
    }
    return GST_BASE_SINK_CLASS(parent_class)->event(basesink, event);
}

static void gst_sub_sink_dispose(GObject *obj)
{
    g_return_if_fail(obj != nullptr);
    G_OBJECT_CLASS(parent_class)->dispose(obj);
}

static void gst_sub_sink_finalize(GObject *obj)
{
    g_return_if_fail(obj != nullptr);
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static gboolean plugin_init(GstPlugin *plugin)
{
    g_return_val_if_fail(plugin != nullptr, FALSE);
    gboolean ret = gst_element_register(plugin, "subsink", GST_RANK_PRIMARY, GST_TYPE_SUB_SINK);
    return ret;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    _sub_sink,
    "GStreamer Subtitle Sink",
    plugin_init,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)