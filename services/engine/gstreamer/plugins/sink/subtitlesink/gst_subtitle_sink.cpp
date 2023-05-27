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

#include <gst/gst.h>
#include <cinttypes>
#include "config.h"
#include "gst_subtitle_sink.h"

using namespace OHOS::Media;
#define POINTER_MASK 0x00FFFFFF
#define FAKE_POINTER(addr) (POINTER_MASK & reinterpret_cast<uintptr_t>(addr))
#define USECOND_TO_MSECOND(us) ((us) * 1000)
#define MSECOND_TO_SECOND(ms) ((ms) * 1000)

struct _GstSubtitleSinkPrivate {
    GMutex mutex;
    gboolean started;
    guint64 running_time;
    guint64 text_frame_duration;
    GstSubtitleSinkCallbacks callbacks;
    gpointer userdata;
    std::unique_ptr<TaskQueue> timer_queue;
};


static GstStaticPadTemplate g_sinktemplate = GST_STATIC_PAD_TEMPLATE("subtitlesink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static void gst_subtitle_sink_dispose(GObject *obj);
static void gst_subtitle_sink_finalize(GObject *obj);
static void gst_subtitle_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_subtitle_sink_handle_buffer(GstSubtitleSink *subtitle_sink,
    GstBuffer *buffer, gboolean cancel, guint64 delayUs = 0ULL);
static void gst_subtitle_sink_cancel_not_executed_task(GstSubtitleSink *subtitle_sink);
static void gst_subtitle_sink_get_gst_buffer_info(GstBuffer *buffer, guint64 &gstPts, guint32 &duration);
static GstStateChangeReturn gst_subtitle_sink_change_state(GstElement *element, GstStateChange transition);
static gboolean gst_subtitle_sink_send_event(GstElement *element, GstEvent *event);
static GstFlowReturn gst_subtitle_sink_render(GstAppSink *appsink);
static GstFlowReturn gst_subtitle_sink_new_sample(GstAppSink *appsink, gpointer user_data);
static GstFlowReturn gst_subtitle_sink_new_preroll(GstAppSink *appsink, gpointer user_data);
static gboolean gst_subtitle_sink_start(GstBaseSink *basesink);
static gboolean gst_subtitle_sink_stop(GstBaseSink *basesink);
static gboolean gst_subtitle_sink_event(GstBaseSink *basesink, GstEvent *event);

#define gst_subtitle_sink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstSubtitleSink, gst_subtitle_sink,
                        GST_TYPE_APP_SINK, G_ADD_PRIVATE(GstSubtitleSink));

GST_DEBUG_CATEGORY_STATIC(gst_subtitle_sink_debug_category);
#define GST_CAT_DEFAULT gst_subtitle_sink_debug_category

static void gst_subtitle_sink_class_init(GstSubtitleSinkClass *kclass)
{
    g_return_if_fail(kclass != nullptr);

    GObjectClass *gobject_class = G_OBJECT_CLASS(kclass);
    GstElementClass *element_class = GST_ELEMENT_CLASS(kclass);
    GstBaseSinkClass *basesink_class = GST_BASE_SINK_CLASS(kclass);
    gst_element_class_add_static_pad_template(element_class, &g_sinktemplate);

    gst_element_class_set_static_metadata(element_class,
        "SubSink", "Sink/Subtitle", " Sub sink", "OpenHarmony");

    gobject_class->dispose = gst_subtitle_sink_dispose;
    gobject_class->finalize = gst_subtitle_sink_finalize;
    gobject_class->set_property = gst_subtitle_sink_set_property;
    element_class->change_state = gst_subtitle_sink_change_state;
    element_class->send_event = gst_subtitle_sink_send_event;
    basesink_class->event = gst_subtitle_sink_event;
    basesink_class->stop = gst_subtitle_sink_stop;
    basesink_class->start = gst_subtitle_sink_start;
    GST_DEBUG_CATEGORY_INIT(gst_subtitle_sink_debug_category, "subtitle_sink", 0, "subtitle_sink class");
}

static void gst_subtitle_sink_init(GstSubtitleSink *subtitle_sink)
{
    g_return_if_fail(subtitle_sink != nullptr);

    subtitle_sink->is_flushing = FALSE;
    subtitle_sink->preroll_buffer = nullptr;
    subtitle_sink->rate = 1.0f;

    auto priv = reinterpret_cast<GstSubtitleSinkPrivate *>(gst_subtitle_sink_get_instance_private(subtitle_sink));
    g_return_if_fail(priv != nullptr);
    subtitle_sink->priv = priv;
    g_mutex_init(&priv->mutex);

    priv->started = FALSE;
    priv->callbacks.new_sample = nullptr;
    priv->userdata = nullptr;
    priv->timer_queue = std::make_unique<TaskQueue>("GstSubtitleSinkTask");
}

void gst_subtitle_sink_set_callback(GstSubtitleSink *subtitle_sink, GstSubtitleSinkCallbacks *callbacks,
    gpointer user_data, GDestroyNotify notify)
{
    g_return_if_fail(GST_IS_SUBTITLE_SINK(subtitle_sink));
    g_return_if_fail(callbacks != nullptr);
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;
    g_return_if_fail(priv != nullptr);

    GST_OBJECT_LOCK(subtitle_sink);
    priv->callbacks = *callbacks;
    priv->userdata = user_data;

    GstAppSinkCallbacks appsink_callback;
    appsink_callback.new_sample = gst_subtitle_sink_new_sample;
    appsink_callback.new_preroll = gst_subtitle_sink_new_preroll;
    gst_app_sink_set_callbacks(reinterpret_cast<GstAppSink *>(subtitle_sink),
        &appsink_callback, user_data, notify);
    GST_OBJECT_UNLOCK(subtitle_sink);
}

static void gst_subtitle_sink_handle_buffer(GstSubtitleSink *subtitle_sink,
    GstBuffer *buffer, gboolean cancel, guint64 delayUs)
{
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;
    g_return_if_fail(priv != nullptr);

    auto handler = std::make_shared<TaskHandler<void>>([=]() {
        if (priv->callbacks.new_sample != nullptr) {
            (void)priv->callbacks.new_sample(buffer, priv->userdata);
        }
    });
    priv->timer_queue->EnqueueTask(handler, cancel, delayUs);
}

static void gst_subtitle_sink_cancel_not_executed_task(GstSubtitleSink *subtitle_sink)
{
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;
    g_return_if_fail(priv != nullptr);
    auto handler = std::make_shared<TaskHandler<void>>([]() {});
    priv->timer_queue->EnqueueTask(handler, true);
}

static void gst_subtitle_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr);
    g_return_if_fail(value != nullptr);
    g_return_if_fail(pspec != nullptr);
    (void)prop_id;
}

static GstStateChangeReturn gst_subtitle_sink_change_state(GstElement *element, GstStateChange transition)
{
    g_return_val_if_fail(element != nullptr, GST_STATE_CHANGE_FAILURE);
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK(element);
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;
    g_return_val_if_fail(priv != nullptr, GST_STATE_CHANGE_FAILURE);
    switch (transition) {
        case GST_STATE_CHANGE_PAUSED_TO_PLAYING: {
            g_mutex_lock(&priv->mutex);
            guint64 left_duration = 0;
            left_duration = priv->text_frame_duration - priv->running_time;
            GST_DEBUG_OBJECT(subtitle_sink, "text left duration is %" GST_TIME_FORMAT,
                GST_TIME_ARGS(MSECOND_TO_SECOND(left_duration)));
            g_mutex_unlock(&priv->mutex);
            gst_subtitle_sink_handle_buffer(subtitle_sink, nullptr, FALSE, left_duration);
            break;
        }
        case GST_STATE_CHANGE_PLAYING_TO_PAUSED: {
            g_mutex_lock(&priv->mutex);
            priv->running_time = GST_TIME_AS_MSECONDS(gst_util_get_timestamp()) - priv->running_time;
            g_mutex_unlock(&priv->mutex);
            gst_subtitle_sink_cancel_not_executed_task(subtitle_sink);
            break;
        }
        case GST_STATE_CHANGE_PAUSED_TO_READY: {
            gst_subtitle_sink_handle_buffer(subtitle_sink, nullptr, TRUE);
            GST_INFO_OBJECT(subtitle_sink, "subtitle sink stop");
            break;
        }
        default:
            break;
    }
    return GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
}

static gboolean gst_subtitle_sink_send_event(GstElement *element, GstEvent *event)
{
    g_return_val_if_fail(element != nullptr && event != nullptr, FALSE);
    GstBaseSink *basesink = GST_BASE_SINK(element);
    GstSubtitleSink *self = GST_SUBTITLE_SINK(element);
    GstFormat seek_format;
    GstSeekFlags flags;
    GstSeekType start_type, stop_type;
    gint64 start, stop;

    GST_DEBUG_OBJECT(basesink, "handling event name %s", GST_EVENT_TYPE_NAME(event));
    switch (GST_EVENT_TYPE(event)) {
        case GST_EVENT_SEEK: {
            gst_event_parse_seek(event, &subtitle_sink->rate, &seek_format, &flags, &start_type, &start, &stop_type, &stop);
            GST_DEBUG_OBJECT(basesink, "parse seek rate: %f", subtitle_sink->rate);
            break;
        }
        default:
            break;
    }
    return GST_ELEMENT_CLASS(parent_class)->send_event(element, event);
}

static void gst_subtitle_sink_get_gst_buffer_info(GstBuffer *buffer, guint64 &gstPts, guint32 &duration)
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

static GstFlowReturn gst_subtitle_sink_new_preroll(GstAppSink *appsink, gpointer user_data)
{
    (void)user_data;
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK_CAST(appsink);
    GstSample *sample = gst_app_sink_pull_preroll(appsink);
    GstBuffer *buffer = gst_sample_get_buffer(sample);

    GST_INFO_OBJECT(subtitle_sink, "app render preroll buffer 0x%06" PRIXPTR "", FAKE_POINTER(buffer));
    g_return_val_if_fail(buffer != nullptr, GST_FLOW_ERROR);
    guint64 pts = 0;
    guint32 duration = 0;
    gst_subtitle_sink_get_gst_buffer_info(buffer, pts, duration);
    if (!GST_CLOCK_TIME_IS_VALID(pts) || !GST_CLOCK_TIME_IS_VALID(duration)) {
        GST_ERROR_OBJECT(subtitle_sink, "pts or duration invalid");
        return GST_FLOW_ERROR;
    }
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;
    g_mutex_lock(&priv->mutex);
    priv->text_frame_duration = USECOND_TO_MSECOND(duration / subtitle_sink->rate);
    priv->running_time = 0ULL;
    g_mutex_unlock(&priv->mutex);
    GST_DEBUG_OBJECT(subtitle_sink, "preroll buffer text duration is %" GST_TIME_FORMAT,
        GST_TIME_ARGS(MSECOND_TO_SECOND(priv->text_frame_duration)));

    subtitle_sink->preroll_buffer = buffer;
    gst_subtitle_sink_handle_buffer(subtitle_sink, buffer, TRUE, 0ULL);
    if (sample != nullptr) {
        gst_sample_unref(sample);
    }
    return GST_FLOW_OK;
}

static GstFlowReturn gst_subtitle_sink_render(GstAppSink *appsink)
{
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK_CAST(appsink);
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    GstBuffer *buffer = gst_sample_get_buffer(sample);

    if (subtitle_sink->preroll_buffer == buffer) {
        subtitle_sink->preroll_buffer = nullptr;
        GST_DEBUG_OBJECT(subtitle_sink, "preroll buffer, no need render again");
        return GST_FLOW_OK;
    }

    GST_INFO_OBJECT(subtitle_sink, "app render buffer 0x%06" PRIXPTR "", FAKE_POINTER(buffer));

    g_return_val_if_fail(buffer != nullptr, GST_FLOW_ERROR);
    guint64 pts = 0;
    guint32 duration = 0;
    gst_subtitle_sink_get_gst_buffer_info(buffer, pts, duration);
    if (!GST_CLOCK_TIME_IS_VALID(pts) || !GST_CLOCK_TIME_IS_VALID(duration)) {
        GST_ERROR_OBJECT(subtitle_sink, "pts or duration invalid");
        return GST_FLOW_ERROR;
    }

    g_mutex_lock(&priv->mutex);
    priv->text_frame_duration = USECOND_TO_MSECOND(duration / subtitle_sink->rate);
    priv->running_time = GST_TIME_AS_MSECONDS(gst_util_get_timestamp());

    GST_DEBUG_OBJECT(subtitle_sink, "text duration is %" GST_TIME_FORMAT,
        GST_TIME_ARGS(MSECOND_TO_SECOND(priv->text_frame_duration)));
    g_mutex_unlock(&priv->mutex);

    gst_subtitle_sink_handle_buffer(subtitle_sink, buffer, TRUE, 0ULL);
    gst_subtitle_sink_handle_buffer(subtitle_sink, nullptr, FALSE, USECOND_TO_MSECOND(priv->text_frame_duration));
    if (sample != nullptr) {
        gst_sample_unref(sample);
    }
    return GST_FLOW_OK;
}

static GstFlowReturn gst_subtitle_sink_new_sample(GstAppSink *appsink, gpointer user_data)
{
    (void)user_data;
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK(appsink);
    g_return_val_if_fail(subtitle_sink != nullptr, GST_FLOW_ERROR);
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;
    g_return_val_if_fail(priv != nullptr, GST_FLOW_ERROR);

    g_mutex_lock(&priv->mutex);
    if (!priv->started) {
        GST_WARNING_OBJECT(subtitle_sink, "we are not started");
        g_mutex_unlock(&priv->mutex);
        return GST_FLOW_FLUSHING;
    }
    g_mutex_unlock(&priv->mutex);

    if (subtitle_sink->is_flushing) {
        GST_DEBUG_OBJECT(subtitle_sink, "we are flushing");
        return GST_FLOW_FLUSHING;
    }
    return gst_subtitle_sink_render(appsink);
}

static gboolean gst_subtitle_sink_start(GstBaseSink *basesink)
{
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK_CAST (basesink);
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;

    GST_BASE_SINK_CLASS(parent_class)->start(basesink);
    g_mutex_lock (&priv->mutex);
    GST_DEBUG_OBJECT (subtitle_sink, "starting");
    subtitle_sink->is_flushing = FALSE;
    priv->started = TRUE;
    priv->timer_queue->Start();
    g_mutex_unlock (&priv->mutex);

    return TRUE;
}

static gboolean gst_subtitle_sink_stop(GstBaseSink *basesink)
{
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK_CAST (basesink);
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;

    g_mutex_lock (&priv->mutex);
    GST_DEBUG_OBJECT (subtitle_sink, "stopping");
    subtitle_sink->is_flushing = TRUE;
    priv->started = FALSE;
    priv->timer_queue->Stop();
    g_mutex_unlock (&priv->mutex);
    GST_BASE_SINK_CLASS(parent_class)->stop(basesink);
    return TRUE;
}

static gboolean gst_subtitle_sink_event(GstBaseSink *basesink, GstEvent *event)
{
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK_CAST(basesink);
    g_return_val_if_fail(subtitle_sink != nullptr, FALSE);
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);
    g_return_val_if_fail(event != nullptr, FALSE);

    switch (event->type) {
        case GST_EVENT_EOS: {
            GST_INFO_OBJECT(subtitle_sink, "received EOS");
            break;
        }
        case GST_EVENT_FLUSH_START: {
            GST_DEBUG_OBJECT(subtitle_sink, "subtitle flush start");
            gst_subtitle_sink_handle_buffer(subtitle_sink, nullptr, TRUE);
            subtitle_sink->is_flushing = TRUE;
            break;
        }
        case GST_EVENT_FLUSH_STOP: {
            GST_DEBUG_OBJECT(subtitle_sink, "subtitle flush stop");
            subtitle_sink->is_flushing = FALSE;
            break;
        }
        default:
            break;
    }
    return GST_BASE_SINK_CLASS(parent_class)->event(basesink, event);
}

static void gst_subtitle_sink_dispose(GObject *obj)
{
    g_return_if_fail(obj != nullptr);
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK_CAST(obj);
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;
    g_mutex_lock (&priv->mutex);
    if (priv->timer_queue != nullptr) {
        priv->timer_queue = nullptr;
    }
    g_mutex_unlock (&priv->mutex);
    G_OBJECT_CLASS(parent_class)->dispose(obj);
}

static void gst_subtitle_sink_finalize(GObject *obj)
{
    g_return_if_fail(obj != nullptr);
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK_CAST(obj);
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;
    if (priv != nullptr) {
        if (priv->timer_queue != nullptr) {
            priv->timer_queue = nullptr;
        }
        g_mutex_clear(&priv->mutex);
    }
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}