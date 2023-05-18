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
#include "gst_subtitle_meta.h"

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


static GstStaticPadTemplate g_sinktemplate = GST_STATIC_PAD_TEMPLATE("subsink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static void gst_sub_sink_dispose(GObject *obj);
static void gst_sub_sink_finalize(GObject *obj);
static void gst_sub_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_sub_sink_handle_buffer(GstSubSink *sub_sink, GstBuffer *buffer, gboolean cancel, guint64 delayUs);
static GstStateChangeReturn gst_sub_sink_change_state(GstElement *element, GstStateChange transition);
static GstFlowReturn gst_sub_sink_new_sample(GstAppSink *appsink, gpointer user_data);
static GstFlowReturn gst_sub_sink_new_preroll(GstAppSink *appsink, gpointer user_data);
static gboolean gst_sub_sink_start(GstBaseSink * basesink);
static gboolean gst_sub_sink_stop(GstBaseSink * basesink);
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
    GstBaseSinkClass *basesink_class = GST_BASE_SINK_CLASS(kclass);
    gst_element_class_add_static_pad_template(element_class, &g_sinktemplate);

    gst_element_class_set_static_metadata(element_class,
        "SubSink", "Sink/Subtitle", " Sub sink", "OpenHarmony");

    gobject_class->dispose = gst_sub_sink_dispose;
    gobject_class->finalize = gst_sub_sink_finalize;
    gobject_class->set_property = gst_sub_sink_set_property;
    element_class->change_state = gst_sub_sink_change_state;
    basesink_class->event = gst_sub_sink_event;
    basesink_class->stop = gst_sub_sink_stop;
    basesink_class->start = gst_sub_sink_start;
    kclass->handle_buffer = gst_sub_sink_handle_buffer;
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
    priv->timer_queue = std::make_unique<TaskQueue>("GstSubSinkTask");
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

    GstAppSinkCallbacks appsink_callback;
    appsink_callback.new_sample = gst_sub_sink_new_sample;
    appsink_callback.new_preroll = gst_sub_sink_new_preroll;
    gst_app_sink_set_callbacks(reinterpret_cast<GstAppSink *>(sub_sink),
        &appsink_callback, user_data, notify);
    GST_OBJECT_UNLOCK(sub_sink);
}

static void gst_sub_sink_handle_buffer(GstSubSink *sub_sink, GstBuffer *buffer, gboolean cancel, guint64 delayUs)
{
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
    GstSubSinkPrivate *priv = sub_sink->priv;
    g_return_val_if_fail(priv != nullptr, GST_STATE_CHANGE_FAILURE);
    switch (transition) {
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            gst_sub_sink_handle_buffer(sub_sink, nullptr, TRUE, 0ULL);
            GST_INFO_OBJECT(sub_sink, "sub sink stop");
            break;
        default:
            break;
    }
    return GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
}

GstFlowReturn gst_sub_sink_new_preroll(GstAppSink *appsink, gpointer user_data)
{
    (void)appsink;
    (void)user_data;
    GST_DEBUG_OBJECT(appsink, "new preroll");
    return GST_FLOW_OK;
}

GstFlowReturn gst_sub_sink_new_sample(GstAppSink *appsink, gpointer user_data)
{
    GstSubSink *sub_sink = GST_SUB_SINK(appsink);
    g_return_val_if_fail(sub_sink != nullptr, GST_FLOW_ERROR);
    GstSubSinkPrivate *priv = sub_sink->priv;
    g_return_val_if_fail(priv != nullptr, GST_FLOW_ERROR);
    GstSubSinkClass *sub_sink_class = GST_SUB_SINK_GET_CLASS(sub_sink);
    g_return_val_if_fail(sub_sink_class != nullptr, GST_FLOW_ERROR);

    g_mutex_lock(&priv->mutex);
    if (!priv->started) {
        GST_WARNING_OBJECT(sub_sink, "we are not started");
        g_mutex_unlock(&priv->mutex);
        return GST_FLOW_FLUSHING;
    }
    g_mutex_unlock(&priv->mutex);

    if (sub_sink->is_flushing) {
        GST_DEBUG_OBJECT(sub_sink, "we are flushing");
        return GST_FLOW_FLUSHING;
    }
    if (sub_sink_class->subtitle_display_callback != nullptr) {
        return sub_sink_class->subtitle_display_callback(appsink, user_data);
    }
    return GST_FLOW_ERROR;
}

static gboolean gst_sub_sink_start(GstBaseSink * basesink)
{
    GstSubSink *sub_sink = GST_SUB_SINK_CAST (basesink);
    GstSubSinkPrivate *priv = sub_sink->priv;

    GST_BASE_SINK_CLASS(parent_class)->start(basesink);
    g_mutex_lock (&priv->mutex);
    GST_DEBUG_OBJECT (sub_sink, "starting");
    sub_sink->is_flushing = FALSE;
    priv->started = TRUE;
    priv->timer_queue->Start();
    g_mutex_unlock (&priv->mutex);

    return TRUE;
}

static gboolean gst_sub_sink_stop(GstBaseSink * basesink)
{
    GstSubSink *sub_sink = GST_SUB_SINK_CAST (basesink);
    GstSubSinkPrivate *priv = sub_sink->priv;

    g_mutex_lock (&priv->mutex);
    GST_DEBUG_OBJECT (sub_sink, "stopping");
    sub_sink->is_flushing = TRUE;
    priv->started = FALSE;
    priv->timer_queue->Stop();
    g_mutex_unlock (&priv->mutex);
    GST_BASE_SINK_CLASS(parent_class)->stop(basesink);
    return TRUE;
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
            GST_INFO_OBJECT(sub_sink, "received EOS");
            break;
        }
        case GST_EVENT_FLUSH_START: {
            GST_DEBUG_OBJECT(sub_sink, "flush start");
            gst_sub_sink_handle_buffer(sub_sink, nullptr, TRUE, 0ULL);
            sub_sink->is_flushing = TRUE;
            break;
        }
        case GST_EVENT_FLUSH_STOP: {
            GST_DEBUG_OBJECT(sub_sink, "flush stop");
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
    GstSubSink *subsink = GST_SUB_SINK_CAST(obj);
    GstSubSinkPrivate *priv = subsink->priv;
    g_mutex_lock (&priv->mutex);
    if (priv->timer_queue != nullptr) {
        priv->timer_queue = nullptr;
    }
    g_mutex_unlock (&priv->mutex);
    G_OBJECT_CLASS(parent_class)->dispose(obj);
}

static void gst_sub_sink_finalize(GObject *obj)
{
    g_return_if_fail(obj != nullptr);
    GstSubSink *subsink = GST_SUB_SINK_CAST(obj);
    GstSubSinkPrivate *priv = subsink->priv;
    if (priv != nullptr) {
        if (priv->timer_queue != nullptr) {
            priv->timer_queue = nullptr;
        }
        g_mutex_clear(&priv->mutex);
    }
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}