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

#include "gst_subtitle_sink.h"
#include <cinttypes>
#include <gst/gst.h>
#include "scope_guard.h"

using namespace OHOS;
using namespace OHOS::Media;
#define POINTER_MASK 0x00FFFFFF
#define FAKE_POINTER(addr) (POINTER_MASK & reinterpret_cast<uintptr_t>(addr))

enum {
    PROP_0,
    PROP_AUDIO_SINK,
    PROP_SEGMENT_UPDATED,
    PROP_CHANGE_TRACK,
    RPOP_ENABLE_DISPLAY,
};

struct _GstSubtitleSinkPrivate {
    GstElement *audio_sink;
    GMutex mutex;
    guint64 time_rendered;
    guint64 text_frame_duration;
    GstSubtitleSinkCallbacks callbacks;
    gpointer userdata;
    std::unique_ptr<TaskQueue> timer_queue;
};

static GstStaticPadTemplate g_sinktemplate = GST_STATIC_PAD_TEMPLATE("subtitlesink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static void gst_subtitle_sink_finalize(GObject *obj);
static void gst_subtitle_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_subtitle_sink_handle_buffer(GstSubtitleSink *subtitle_sink,
    GstBuffer *buffer, gboolean cancel, guint64 delayUs = 0ULL);
static void gst_subtitle_sink_cancel_not_executed_task(GstSubtitleSink *subtitle_sink);
static void gst_subtitle_sink_get_gst_buffer_info(GstBuffer *buffer, guint64 &pts, guint64 &duration);
static GstStateChangeReturn gst_subtitle_sink_change_state(GstElement *element, GstStateChange transition);
static gboolean gst_subtitle_sink_send_event(GstElement *element, GstEvent *event);
static GstFlowReturn gst_subtitle_sink_render(GstAppSink *appsink);
static GstFlowReturn gst_subtitle_sink_new_sample(GstAppSink *appsink, gpointer user_data);
static GstFlowReturn gst_subtitle_sink_new_preroll(GstAppSink *appsink, gpointer user_data);
static gboolean gst_subtitle_sink_start(GstBaseSink *basesink);
static gboolean gst_subtitle_sink_stop(GstBaseSink *basesink);
static gboolean gst_subtitle_sink_event(GstBaseSink *basesink, GstEvent *event);
static GstClockTime gst_subtitle_sink_update_reach_time(GstBaseSink *basesink, GstClockTime reach_time,
    gboolean *need_drop_this_buffer);
static gboolean gst_subtitle_sink_need_drop_buffer(GstBaseSink *basesink,
    GstSegment *segment, guint64 pts, guint64 pts_end);

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
        "SubtitleSink", "Sink/Subtitle", " Subtitle sink", "OpenHarmony");

    gobject_class->finalize = gst_subtitle_sink_finalize;
    gobject_class->set_property = gst_subtitle_sink_set_property;
    element_class->change_state = gst_subtitle_sink_change_state;
    element_class->send_event = gst_subtitle_sink_send_event;
    basesink_class->event = gst_subtitle_sink_event;
    basesink_class->stop = gst_subtitle_sink_stop;
    basesink_class->start = gst_subtitle_sink_start;
    basesink_class->update_reach_time = gst_subtitle_sink_update_reach_time;
    basesink_class->need_drop_buffer = gst_subtitle_sink_need_drop_buffer;

    g_object_class_install_property(gobject_class, PROP_AUDIO_SINK,
        g_param_spec_pointer("audio-sink", "audio sink", "audio sink",
            (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_SEGMENT_UPDATED,
        g_param_spec_boolean("segment-updated", "audio segment updated", "audio segment updated",
            FALSE, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_CHANGE_TRACK,
        g_param_spec_boolean("change-track", "change track", "select track change",
            FALSE, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, RPOP_ENABLE_DISPLAY,
        g_param_spec_boolean("enable-display", "enable subtitle display", "enable subtitle display",
            TRUE, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    GST_DEBUG_CATEGORY_INIT(gst_subtitle_sink_debug_category, "subtitlesink", 0, "subtitlesink class");
}

static void gst_subtitle_sink_init(GstSubtitleSink *subtitle_sink)
{
    g_return_if_fail(subtitle_sink != nullptr);

    subtitle_sink->stop_render = FALSE;
    subtitle_sink->have_first_segment = FALSE;
    subtitle_sink->audio_segment_updated = FALSE;
    subtitle_sink->preroll_buffer = nullptr;
    subtitle_sink->rate = 1.0f;
    subtitle_sink->segment_updated = FALSE;
    subtitle_sink->is_changing_track = FALSE;
    subtitle_sink->track_changing_position = 0;
    subtitle_sink->enable_display = TRUE;
    subtitle_sink->need_send_empty_buffer = FALSE;
    gst_segment_init (&subtitle_sink->segment, GST_FORMAT_UNDEFINED);

    auto priv = reinterpret_cast<GstSubtitleSinkPrivate *>(gst_subtitle_sink_get_instance_private(subtitle_sink));
    g_return_if_fail(priv != nullptr);
    subtitle_sink->priv = priv;
    g_mutex_init(&priv->mutex);

    priv->callbacks.new_sample = nullptr;
    priv->userdata = nullptr;
    priv->audio_sink = nullptr;
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

static void gst_subtitle_sink_set_audio_sink(GstSubtitleSink *subtitle_sink, gpointer audio_sink)
{
    g_return_if_fail(audio_sink != nullptr);

    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;
    g_mutex_lock(&priv->mutex);
    priv->audio_sink = GST_ELEMENT_CAST(gst_object_ref(audio_sink));
    GST_INFO_OBJECT(subtitle_sink, "get audio sink: %s", GST_ELEMENT_NAME(priv->audio_sink));
    g_mutex_unlock(&priv->mutex);
}

static gboolean gst_subtitle_sink_need_drop_buffer(GstBaseSink *basesink,
    GstSegment *segment, guint64 pts, guint64 pts_end)
{
    auto subtitle_sink = GST_SUBTITLE_SINK(basesink);
    g_return_val_if_fail(subtitle_sink != nullptr, FALSE);
    if (!subtitle_sink->enable_display) {
        GST_LOG_OBJECT(subtitle_sink, "subtitle display disabled, drop this buffer");
        return TRUE;
    }

    auto temp_segment = *segment;
    if (subtitle_sink->is_changing_track) {
        temp_segment.start = subtitle_sink->track_changing_position;
    }
    if (subtitle_sink->have_first_filter) {
        temp_segment.start = subtitle_sink->init_position;
    }
    guint64 start = temp_segment.start;
    if (pts <= start && start < pts_end) {
        GST_DEBUG_OBJECT(subtitle_sink, "no need drop, segment start is intersects with buffer time range, pts"
        " = %" GST_TIME_FORMAT ", pts end = %" GST_TIME_FORMAT " segment start = %"
        GST_TIME_FORMAT, GST_TIME_ARGS(pts), GST_TIME_ARGS(pts_end), GST_TIME_ARGS(start));
        return FALSE;
    }
    return G_LIKELY(!gst_segment_clip (&temp_segment, GST_FORMAT_TIME, pts, pts_end, NULL, NULL));
}

static void gst_subtitle_sink_handle_buffer(GstSubtitleSink *subtitle_sink,
    GstBuffer *buffer, gboolean cancel, guint64 delayUs)
{
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;
    g_return_if_fail(priv != nullptr);

    if (!subtitle_sink->enable_display) {
        if (subtitle_sink->need_send_empty_buffer) {
            subtitle_sink->need_send_empty_buffer = FALSE;
        } else {
            return;
        }
    }

    auto handler = std::make_shared<TaskHandler<void>>([=]() {
        (void)priv->callbacks.new_sample(buffer, priv->userdata);
        gst_buffer_unref(buffer);
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

static void gst_subtitle_sink_segment_updated(GstSubtitleSink *subtitle_sink)
{
    subtitle_sink->audio_segment_updated = TRUE;
    if (G_LIKELY(subtitle_sink->have_first_segment && !subtitle_sink->segment_updated)) {
        auto audio_base = GST_BASE_SINK(subtitle_sink->priv->audio_sink);
        GST_OBJECT_LOCK(audio_base);
        subtitle_sink->segment_updated = TRUE;
        gst_segment_copy_into(&audio_base->segment, &subtitle_sink->segment);
        GST_OBJECT_UNLOCK(audio_base);
        g_mutex_lock(&subtitle_sink->segment_mutex);
        g_cond_signal(&subtitle_sink->segment_cond);
        g_mutex_unlock(&subtitle_sink->segment_mutex);
    }
}

static void gst_subtitle_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr);
    g_return_if_fail(value != nullptr);
    g_return_if_fail(pspec != nullptr);
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK_CAST(object);
    switch (prop_id) {
        case PROP_AUDIO_SINK: {
            gst_subtitle_sink_set_audio_sink(subtitle_sink, g_value_get_pointer(value));
            break;
        }
        case PROP_SEGMENT_UPDATED: {
            gst_subtitle_sink_segment_updated(subtitle_sink);
            break;
        }
        case PROP_CHANGE_TRACK: {
            GST_OBJECT_LOCK(subtitle_sink);
            subtitle_sink->is_changing_track = g_value_get_boolean(value);
            GST_OBJECT_UNLOCK(subtitle_sink);
            break;
        }
        case RPOP_ENABLE_DISPLAY: {
            GST_BASE_SINK_PREROLL_LOCK(subtitle_sink);
            subtitle_sink->enable_display = g_value_get_boolean(value);
            if (!subtitle_sink->enable_display) {
                GST_DEBUG_OBJECT(subtitle_sink, "disable display, send an empty buffer");
                subtitle_sink->need_send_empty_buffer = true;
                gst_subtitle_sink_handle_buffer(subtitle_sink, nullptr, TRUE);
            }
            GST_BASE_SINK_PREROLL_UNLOCK(subtitle_sink);
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static GstStateChangeReturn gst_subtitle_sink_change_state(GstElement *element, GstStateChange transition)
{
    g_return_val_if_fail(element != nullptr, GST_STATE_CHANGE_FAILURE);
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK(element);
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;
    g_return_val_if_fail(priv != nullptr, GST_STATE_CHANGE_FAILURE);
    switch (transition) {
        case GST_STATE_CHANGE_READY_TO_PAUSED: {
            subtitle_sink->stop_render = TRUE;
            break;
        }
        case GST_STATE_CHANGE_PAUSED_TO_PLAYING: {
            g_mutex_lock(&priv->mutex);
            gint64 left_duration = priv->text_frame_duration - priv->time_rendered;
            left_duration = left_duration > 0 ? left_duration : 0;
            priv->time_rendered = gst_util_get_timestamp();
            g_mutex_unlock(&priv->mutex);
            if (subtitle_sink->preroll_buffer != nullptr) {
                GST_DEBUG_OBJECT(subtitle_sink, "text left duration is %" GST_TIME_FORMAT,
                    GST_TIME_ARGS(left_duration));
                gst_subtitle_sink_handle_buffer(subtitle_sink, nullptr, FALSE, GST_TIME_AS_USECONDS(left_duration));
            }
            subtitle_sink->stop_render = FALSE;
            break;
        }
        case GST_STATE_CHANGE_PLAYING_TO_PAUSED: {
            g_mutex_lock(&priv->mutex);
            subtitle_sink->stop_render = TRUE;
            priv->time_rendered = gst_util_get_timestamp() - priv->time_rendered;
            g_mutex_unlock(&priv->mutex);
            gst_subtitle_sink_cancel_not_executed_task(subtitle_sink);
            break;
        }
        case GST_STATE_CHANGE_PAUSED_TO_READY: {
            subtitle_sink->stop_render = FALSE;
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
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK(element);
    GstFormat seek_format;
    GstSeekType start_type;
    GstSeekType stop_type;
    gint64 start;
    gint64 stop;

    GST_DEBUG_OBJECT(subtitle_sink, "handling event name %s", GST_EVENT_TYPE_NAME(event));
    switch (GST_EVENT_TYPE(event)) {
        case GST_EVENT_SEEK: {
            gst_event_parse_seek(event, &subtitle_sink->rate, &seek_format,
                &subtitle_sink->seek_flags, &start_type, &start, &stop_type, &stop);
            GST_DEBUG_OBJECT(subtitle_sink, "parse seek rate: %f", subtitle_sink->rate);
            break;
        }
        default:
            break;
    }
    return GST_ELEMENT_CLASS(parent_class)->send_event(element, event);
}

static void gst_subtitle_sink_get_gst_buffer_info(GstBuffer *buffer, guint64 &pts, guint64 &duration)
{
    pts = GST_BUFFER_PTS_IS_VALID(buffer) ? GST_BUFFER_PTS(buffer) : GST_CLOCK_TIME_NONE;
    duration = GST_BUFFER_DURATION_IS_VALID(buffer) ? GST_BUFFER_DURATION(buffer) : GST_CLOCK_TIME_NONE;
}

static GstFlowReturn gst_subtitle_sink_new_preroll(GstAppSink *appsink, gpointer user_data)
{
    (void)user_data;
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK_CAST(appsink);
    if (subtitle_sink->stop_render) {
        GST_WARNING_OBJECT(subtitle_sink, "prepared buffer or playing to paused buffer, do not render");
        return GST_FLOW_OK;
    }
    GstSample *sample = gst_app_sink_pull_preroll(appsink);
    GstBuffer *buffer = gst_buffer_ref(gst_sample_get_buffer(sample));
    ON_SCOPE_EXIT(0) {
        gst_sample_unref(sample);
    };
    g_return_val_if_fail(buffer != nullptr, GST_FLOW_ERROR);

    if (subtitle_sink->preroll_buffer == buffer) {
        gst_buffer_unref(buffer);
        GST_DEBUG_OBJECT(subtitle_sink, "preroll buffer has been rendererd, no need render again");
        return GST_FLOW_OK;
    }

    GST_INFO_OBJECT(subtitle_sink, "app render preroll buffer 0x%06" PRIXPTR "", FAKE_POINTER(buffer));
    guint64 pts = 0;
    guint64 duration = 0;
    gst_subtitle_sink_get_gst_buffer_info(buffer, pts, duration);
    if (!GST_CLOCK_TIME_IS_VALID(pts) || !GST_CLOCK_TIME_IS_VALID(duration)) {
        gst_buffer_unref(buffer);
        GST_ERROR_OBJECT(subtitle_sink, "pts or duration invalid");
        return GST_FLOW_OK;
    }
    guint64 pts_end = pts + duration;
    auto time = GST_BASE_SINK(subtitle_sink)->segment.time;
    if (pts > time) {
        GST_DEBUG_OBJECT(subtitle_sink, "pts = %" GST_TIME_FORMAT ", pts end = %"
            GST_TIME_FORMAT " segment time = %" GST_TIME_FORMAT ", not yet render time",
            GST_TIME_ARGS(pts), GST_TIME_ARGS(pts_end), GST_TIME_ARGS(time));
        gst_buffer_unref(buffer);
        return GST_FLOW_OK;
    }
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;
    g_mutex_lock(&priv->mutex);
    duration = std::min(duration, pts_end - subtitle_sink->segment.start);
    priv->text_frame_duration = duration / subtitle_sink->rate;
    priv->time_rendered = 0ULL;
    g_mutex_unlock(&priv->mutex);
    GST_DEBUG_OBJECT(subtitle_sink, "preroll buffer pts is %" GST_TIME_FORMAT ", duration is %" GST_TIME_FORMAT,
        GST_TIME_ARGS(pts), GST_TIME_ARGS(priv->text_frame_duration));
    subtitle_sink->preroll_buffer = buffer;
    gst_subtitle_sink_handle_buffer(subtitle_sink, buffer, TRUE, 0ULL);
    return GST_FLOW_OK;
}

static GstClockTime gst_subtitle_sink_update_reach_time(GstBaseSink *basesink, GstClockTime reach_time,
    gboolean *need_drop_this_buffer)
{
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK(basesink);
    if (!subtitle_sink->enable_display) {
        *need_drop_this_buffer = TRUE;
        GST_DEBUG_OBJECT(subtitle_sink, "subtitle display disabled, drop this buffer");
    }
    return reach_time;
}

static GstFlowReturn gst_subtitle_sink_render(GstAppSink *appsink)
{
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK_CAST(appsink);
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;

    GstSample *sample = gst_app_sink_pull_sample(appsink);
    GstBuffer *buffer = gst_buffer_ref(gst_sample_get_buffer(sample));
    ON_SCOPE_EXIT(0) {
        gst_sample_unref(sample);
    };
    g_return_val_if_fail(buffer != nullptr, GST_FLOW_ERROR);

    if (subtitle_sink->preroll_buffer == buffer) {
        subtitle_sink->preroll_buffer = nullptr;
        GST_DEBUG_OBJECT(subtitle_sink, "preroll buffer, no need render again");
        gst_buffer_unref(buffer);
        return GST_FLOW_OK;
    }

    GST_INFO_OBJECT(subtitle_sink, "app render buffer 0x%06" PRIXPTR "", FAKE_POINTER(buffer));

    guint64 pts = 0;
    guint64 duration = 0;
    gst_subtitle_sink_get_gst_buffer_info(buffer, pts, duration);
    if (!GST_CLOCK_TIME_IS_VALID(pts) || !GST_CLOCK_TIME_IS_VALID(duration)) {
        GST_ERROR_OBJECT(subtitle_sink, "pts or duration invalid");
        gst_buffer_unref(buffer);
        return GST_FLOW_ERROR;
    }

    g_mutex_lock(&priv->mutex);
    guint64 start = subtitle_sink->segment.start;
    if (subtitle_sink->have_first_filter) {
        start = subtitle_sink->init_position;
    }
    if (subtitle_sink->is_changing_track) {
        start = subtitle_sink->track_changing_position;
    }
    duration = std::min(duration, pts + duration - start);
    priv->text_frame_duration = duration / subtitle_sink->rate;
    priv->time_rendered = gst_util_get_timestamp();
    GST_DEBUG_OBJECT(subtitle_sink, "buffer pts is %" GST_TIME_FORMAT ", duration is %" GST_TIME_FORMAT,
        GST_TIME_ARGS(pts), GST_TIME_ARGS(priv->text_frame_duration));
    g_mutex_unlock(&priv->mutex);

    gst_subtitle_sink_handle_buffer(subtitle_sink, buffer, TRUE, 0ULL);
    gst_subtitle_sink_handle_buffer(subtitle_sink, nullptr, FALSE, GST_TIME_AS_USECONDS(priv->text_frame_duration));
    return GST_FLOW_OK;
}

static GstFlowReturn gst_subtitle_sink_new_sample(GstAppSink *appsink, gpointer user_data)
{
    (void)user_data;
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK(appsink);
    g_return_val_if_fail(subtitle_sink != nullptr, GST_FLOW_ERROR);
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;
    g_return_val_if_fail(priv != nullptr, GST_FLOW_ERROR);
    return gst_subtitle_sink_render(appsink);
}

static gboolean gst_subtitle_sink_start(GstBaseSink *basesink)
{
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK_CAST (basesink);
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;

    GST_BASE_SINK_CLASS(parent_class)->start(basesink);
    g_mutex_lock (&priv->mutex);
    GST_DEBUG_OBJECT (subtitle_sink, "started");
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
    subtitle_sink->have_first_segment = FALSE;
    subtitle_sink->preroll_buffer = nullptr;
    subtitle_sink->stop_render = FALSE;
    subtitle_sink->segment_updated = FALSE;
    subtitle_sink->is_changing_track = FALSE;
    subtitle_sink->track_changing_position = 0;
    subtitle_sink->need_send_empty_buffer = FALSE;
    priv->timer_queue->Stop();
    g_mutex_unlock (&priv->mutex);
    GST_BASE_SINK_CLASS(parent_class)->stop(basesink);
    return TRUE;
}

static void gst_subtitle_sink_handle_speed(GstBaseSink *basesink)
{
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK_CAST(basesink);
    std::swap(subtitle_sink->segment.rate, subtitle_sink->segment.applied_rate);
}

static void gst_subtitle_sink_handle_audio_segment(GstBaseSink *basesink, const GstSegment *new_segment)
{
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK_CAST(basesink);
    auto audio_base = GST_BASE_SINK(subtitle_sink->priv->audio_sink);
    if (!subtitle_sink->audio_segment_updated) {
        g_mutex_lock(&subtitle_sink->segment_mutex);
        gint64 end_time = g_get_monotonic_time() + G_TIME_SPAN_SECOND / 2;
        g_cond_wait_until(&subtitle_sink->segment_cond, &subtitle_sink->segment_mutex, end_time);
        g_mutex_unlock(&subtitle_sink->segment_mutex);
    }
    if (!subtitle_sink->segment_updated) {
        GST_OBJECT_LOCK(audio_base);
        gst_segment_copy_into(&audio_base->segment, &subtitle_sink->segment);
        GST_OBJECT_UNLOCK(audio_base);
    }
    subtitle_sink->segment.stop = new_segment->stop;
    subtitle_sink->segment.duration = new_segment->duration;
}

static GstEvent* gst_subtitle_sink_handle_segment_event(GstBaseSink *basesink, GstEvent *event)
{
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK_CAST(basesink);
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;
    subtitle_sink->segment_updated = FALSE;
    auto audio_base = GST_BASE_SINK(priv->audio_sink);
    guint32 seqnum = gst_event_get_seqnum (event);
    GstSegment new_segment;
    gst_event_copy_segment (event, &new_segment);
    GST_DEBUG_OBJECT (basesink, "received upstream segment %u", seqnum);
    if (!subtitle_sink->have_first_segment) {
        subtitle_sink->have_first_segment = TRUE;
        subtitle_sink->have_first_filter = TRUE;
        GST_WARNING_OBJECT(subtitle_sink, "recv first segment event");
        new_segment.rate = audio_base->segment.applied_rate;
        subtitle_sink->init_position = new_segment.start;
        gst_segment_copy_into(&new_segment, &subtitle_sink->segment);
        subtitle_sink->segment.start = audio_base->segment.time;
    } else if (!subtitle_sink->is_changing_track) {
        subtitle_sink->have_first_filter = FALSE;
        gst_subtitle_sink_handle_audio_segment(basesink, &new_segment);
        gst_subtitle_sink_handle_speed(basesink);
        GST_DEBUG_OBJECT (basesink, "segment updated");
    }
    if (subtitle_sink->is_changing_track) {
        subtitle_sink->track_changing_position = new_segment.start;
    }
    subtitle_sink->audio_segment_updated = FALSE;
    subtitle_sink->segment_updated = TRUE;
    subtitle_sink->rate = subtitle_sink->segment.rate;
    auto new_event = gst_event_new_segment(&subtitle_sink->segment);
    if (new_event != nullptr) {
        gst_event_unref(event);
        event = new_event;
    }
    return event;
}

static gboolean gst_subtitle_sink_handle_flush_start_event(GstBaseSink *basesink, GstEvent *event)
{
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK_CAST(basesink);
    g_return_val_if_fail(subtitle_sink != nullptr, FALSE);
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;
    GST_DEBUG_OBJECT(subtitle_sink, "subtitle flush start");
    gst_subtitle_sink_handle_buffer(subtitle_sink, nullptr, TRUE);
    subtitle_sink->stop_render = FALSE;
    subtitle_sink->audio_segment_updated = FALSE;
    priv->time_rendered = 0;
    return GST_BASE_SINK_CLASS(parent_class)->event(basesink, event);
}

static gboolean gst_subtitle_sink_handle_flush_stop_event(GstBaseSink *basesink, GstEvent *event)
{
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK_CAST(basesink);
    g_return_val_if_fail(subtitle_sink != nullptr, FALSE);
    GST_DEBUG_OBJECT(subtitle_sink, "subtitle flush stop");
    return GST_BASE_SINK_CLASS(parent_class)->event(basesink, event);
}

static gboolean gst_subtitle_sink_event(GstBaseSink *basesink, GstEvent *event)
{
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK_CAST(basesink);
    g_return_val_if_fail(subtitle_sink != nullptr, FALSE);
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;
    g_return_val_if_fail(priv != nullptr, FALSE);
    g_return_val_if_fail(event != nullptr, FALSE);
    switch (event->type) {
        case GST_EVENT_SEGMENT: {
            if (priv->audio_sink == nullptr) {
                break;
            }
            GST_OBJECT_LOCK (basesink);
            event = gst_subtitle_sink_handle_segment_event(basesink, event);
            GST_OBJECT_UNLOCK(basesink);
            break;
        }
        case GST_EVENT_EOS: {
            GST_DEBUG_OBJECT(subtitle_sink, "received EOS");
            break;
        }
        case GST_EVENT_FLUSH_START: {
            return gst_subtitle_sink_handle_flush_start_event(basesink, event);
        }
        case GST_EVENT_FLUSH_STOP: {
            return gst_subtitle_sink_handle_flush_stop_event(basesink, event);
        }
        default:
            break;
    }
    return GST_BASE_SINK_CLASS(parent_class)->event(basesink, event);
}

static void gst_subtitle_sink_finalize(GObject *obj)
{
    g_return_if_fail(obj != nullptr);
    GstSubtitleSink *subtitle_sink = GST_SUBTITLE_SINK_CAST(obj);
    GstSubtitleSinkPrivate *priv = subtitle_sink->priv;
    subtitle_sink->preroll_buffer = nullptr;
    if (priv->audio_sink != nullptr) {
        gst_object_unref(priv->audio_sink);
        priv->audio_sink = nullptr;
    }
    priv->timer_queue = nullptr;
    g_mutex_clear(&priv->mutex);
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}