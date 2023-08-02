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

#include "gst_video_display_sink.h"
#include <string>
#include "param_wrapper.h"

using namespace OHOS;
namespace {
    constexpr guint64 DEFAULT_MAX_WAIT_CLOCK_TIME = 200000000; // ns, 200ms
    constexpr gint64 DEFAULT_AUDIO_RUNNING_TIME_DIFF_THD = 20000000; // ns, 20ms
    constexpr gint64 DEFAULT_EXTRA_RENDER_FRAME_DIFF = 5000000; // ns, 5ms
    constexpr gint DEFAULT_DROP_BEHIND_VIDEO_BUF_FREQUENCY = 5; // drop 1 buffer every 5 buffers at most
    constexpr gint64 DEFAULT_VIDEO_BEHIND_AUDIO_THD = 90000000; // 90ms, level B
}

enum {
    PROP_0,
    PROP_AUDIO_SINK,
    PROP_ENABLE_KPI_AVSYNC_LOG,
};

struct _GstVideoDisplaySinkPrivate {
    GstElement *audio_sink;
    gboolean enable_kpi_avsync_log;
    gboolean enable_drop;
    gboolean close_avsync;
    GMutex mutex;
    guint64 render_time_diff_threshold;
    guint buffer_count;
    guint64 total_video_buffer_num;
    guint64 dropped_video_buffer_num;
    guint64 last_video_render_pts;
    guint bandwidth;
    gboolean need_report_bandwidth;
    gboolean start_first_render;
    guint audio_delay_time;
    guint video_delay_time;
};

#define gst_video_display_sink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstVideoDisplaySink, gst_video_display_sink,
                        GST_TYPE_SURFACE_MEM_SINK, G_ADD_PRIVATE(GstVideoDisplaySink));

GST_DEBUG_CATEGORY_STATIC(gst_video_display_sink_debug_category);
#define GST_CAT_DEFAULT gst_video_display_sink_debug_category

static void gst_video_display_sink_dispose(GObject *obj);
static void gst_video_display_sink_finalize(GObject *obj);
static void gst_video_display_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static GstFlowReturn gst_video_display_sink_do_app_render(GstSurfaceMemSink *surface_sink,
    GstBuffer *buffer, bool is_preroll);
static GstClockTime gst_video_display_sink_update_reach_time(GstBaseSink *base_sink, GstClockTime reach_time,
    gboolean *need_drop_this_buffer);
static gboolean gst_video_display_sink_event(GstBaseSink *base_sink, GstEvent *event);
static GstStateChangeReturn gst_video_display_sink_change_state(GstElement *element, GstStateChange transition);
static void gst_video_display_sink_enable_drop_from_sys_param(GstVideoDisplaySink *video_display_sink);
static void gst_close_avsync_from_sys_param(GstVideoDisplaySink *video_display_sink);

static void gst_video_display_sink_class_init(GstVideoDisplaySinkClass *klass)
{
    g_return_if_fail(klass != nullptr);

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstSurfaceMemSinkClass *surface_sink_class = GST_SURFACE_MEM_SINK_CLASS(klass);
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    GstBaseSinkClass *base_sink_class = GST_BASE_SINK_CLASS(klass);

    gst_element_class_set_static_metadata(element_class,
        "VideoDisplaySink", "Sink/Video", " Video Display sink", "OpenHarmony");

    gobject_class->dispose = gst_video_display_sink_dispose;
    gobject_class->finalize = gst_video_display_sink_finalize;
    gobject_class->set_property = gst_video_display_sink_set_property;
    element_class->change_state = gst_video_display_sink_change_state;

    g_signal_new("bandwidth-change", G_TYPE_FROM_CLASS(klass),
        static_cast<GSignalFlags>(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), 0, NULL,
        NULL, NULL, G_TYPE_NONE, 1, G_TYPE_UINT);  // 1 parameters

    g_object_class_install_property(gobject_class, PROP_AUDIO_SINK,
        g_param_spec_pointer("audio-sink", "audio sink", "audio sink",
            (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_ENABLE_KPI_AVSYNC_LOG,
        g_param_spec_boolean("enable-kpi-avsync-log", "Enable KPI AV sync log", "Enable KPI AV sync log", FALSE,
            (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    surface_sink_class->do_app_render = gst_video_display_sink_do_app_render;
    base_sink_class->update_reach_time = gst_video_display_sink_update_reach_time;
    base_sink_class->event = gst_video_display_sink_event;
    GST_DEBUG_CATEGORY_INIT(gst_video_display_sink_debug_category, "videodisplaysink", 0, "videodisplaysink class");
}

static void gst_video_display_sink_init(GstVideoDisplaySink *sink)
{
    g_return_if_fail(sink != nullptr);

    auto priv = reinterpret_cast<GstVideoDisplaySinkPrivate *>(gst_video_display_sink_get_instance_private(sink));
    g_return_if_fail(priv != nullptr);

    sink->priv = priv;
    priv->audio_sink = nullptr;
    priv->enable_kpi_avsync_log = FALSE;
    priv->close_avsync = FALSE;
    g_mutex_init(&priv->mutex);
    priv->render_time_diff_threshold = DEFAULT_MAX_WAIT_CLOCK_TIME;
    priv->buffer_count = 1;
    priv->bandwidth = 0;
    priv->need_report_bandwidth = FALSE;
    priv->total_video_buffer_num = 0;
    priv->dropped_video_buffer_num = 0;
    priv->last_video_render_pts = 0;
    priv->start_first_render = FALSE;
    priv->audio_delay_time = 0;
    priv->video_delay_time = 0;
    gst_video_display_sink_enable_drop_from_sys_param(sink);
    gst_close_avsync_from_sys_param(sink);
}

static void gst_video_display_sink_dispose(GObject *obj)
{
    g_return_if_fail(obj != nullptr);
    GstVideoDisplaySink *video_display_sink = GST_VIDEO_DISPLAY_SINK_CAST(obj);
    GstVideoDisplaySinkPrivate *priv = video_display_sink->priv;

    g_mutex_lock(&priv->mutex);
    if (priv->audio_sink != nullptr) {
        gst_object_unref(priv->audio_sink);
        priv->audio_sink = nullptr;
    }
    g_mutex_unlock(&priv->mutex);
    G_OBJECT_CLASS(parent_class)->dispose(obj);
}

static void gst_video_display_sink_finalize(GObject *obj)
{
    g_return_if_fail(obj != nullptr);
    GstVideoDisplaySink *video_display_sink = GST_VIDEO_DISPLAY_SINK_CAST(obj);
    GstVideoDisplaySinkPrivate *priv = video_display_sink->priv;
    if (priv != nullptr) {
        g_mutex_clear(&priv->mutex);
    }

    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void gst_video_display_sink_set_audio_sink(GstVideoDisplaySink *video_display_sink, gpointer audio_sink)
{
    g_return_if_fail(audio_sink != nullptr);

    GstVideoDisplaySinkPrivate *priv = video_display_sink->priv;
    g_mutex_lock(&priv->mutex);
    if (priv->audio_sink != nullptr) {
        GST_INFO_OBJECT(video_display_sink, "has audio sink: %s, unref it", GST_ELEMENT_NAME(priv->audio_sink));
        gst_object_unref(priv->audio_sink);
    }
    priv->audio_sink = GST_ELEMENT_CAST(gst_object_ref(audio_sink));
    GST_INFO_OBJECT(video_display_sink, "get audio sink: %s", GST_ELEMENT_NAME(priv->audio_sink));
    g_mutex_unlock(&priv->mutex);
}

static void gst_video_display_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(object != nullptr);
    g_return_if_fail(value != nullptr);
    g_return_if_fail(pspec != nullptr);

    GstVideoDisplaySink *video_display_sink = GST_VIDEO_DISPLAY_SINK_CAST(object);
    GstVideoDisplaySinkPrivate *priv = video_display_sink->priv;
    g_return_if_fail(priv != nullptr);

    switch (prop_id) {
        case PROP_AUDIO_SINK:
            gst_video_display_sink_set_audio_sink(video_display_sink, g_value_get_pointer(value));
            break;
        case PROP_ENABLE_KPI_AVSYNC_LOG:
            g_mutex_lock(&priv->mutex);
            priv->enable_kpi_avsync_log = g_value_get_boolean(value);
            g_mutex_unlock(&priv->mutex);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static gboolean gst_video_display_sink_event(GstBaseSink *base_sink, GstEvent *event)
{
    g_return_val_if_fail(event != nullptr, FALSE);
    GstVideoDisplaySink *video_display_sink = GST_VIDEO_DISPLAY_SINK_CAST(base_sink);
    g_return_val_if_fail(video_display_sink != nullptr, FALSE);
    GstVideoDisplaySinkPrivate *priv = video_display_sink->priv;

    switch (event->type) {
        case GST_EVENT_FLUSH_START: {
            if (priv != nullptr) {
                g_mutex_lock(&priv->mutex);
                priv->start_first_render = TRUE;
                g_mutex_unlock(&priv->mutex);
            }
            break;
        }
        case GST_EVENT_FLUSH_STOP: {
            if (priv != nullptr) {
                g_mutex_lock(&priv->mutex);
                priv->buffer_count = 1;
                priv->last_video_render_pts = 0;
                g_mutex_unlock(&priv->mutex);
            }
            break;
        }
        case GST_EVENT_TAG: {
            GstTagList *tagList;
            gst_event_parse_tag(event, &tagList);
            guint bandwidth;
            gst_tag_list_get_uint(tagList, "bandwidth", &bandwidth);
            if (priv != nullptr && priv->bandwidth != bandwidth && bandwidth != 0) {
                GST_DEBUG_OBJECT(video_display_sink, "bandwidth is %u", bandwidth);
                priv->bandwidth = bandwidth;
                priv->need_report_bandwidth = TRUE;
            }
            break;
        }
        default:
            break;
    }
    return GST_BASE_SINK_CLASS(parent_class)->event(base_sink, event);
}

static GstStateChangeReturn gst_video_display_sink_change_state(GstElement *element, GstStateChange transition)
{
    g_return_val_if_fail(element != nullptr, GST_STATE_CHANGE_FAILURE);
    GstVideoDisplaySink *video_display_sink = GST_VIDEO_DISPLAY_SINK(element);
    GstVideoDisplaySinkPrivate *priv = video_display_sink->priv;

    switch (transition) {
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            if (priv != nullptr) {
                g_mutex_lock(&priv->mutex);
                if (priv->total_video_buffer_num != 0) {
                    GST_DEBUG_OBJECT(video_display_sink, "total video buffer num:%" G_GUINT64_FORMAT
                        ", dropped video buffer num:%" G_GUINT64_FORMAT ", drop rate:%f",
                        priv->total_video_buffer_num, priv->dropped_video_buffer_num,
                        (gfloat)priv->dropped_video_buffer_num / priv->total_video_buffer_num);
                }
                priv->start_first_render = TRUE;
                priv->buffer_count = 1;
                priv->total_video_buffer_num = 0;
                priv->dropped_video_buffer_num = 0;
                priv->last_video_render_pts = 0;
                g_mutex_unlock(&priv->mutex);
            }
            break;
        default:
            break;
    }
    return GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
}

static void gst_video_display_sink_enable_drop_from_sys_param(GstVideoDisplaySink *video_display_sink)
{
    std::string drop_enable;
    video_display_sink->priv->enable_drop = TRUE;
    int32_t res = OHOS::system::GetStringParameter("sys.media.drop.video.buffer.enable", drop_enable, "");
    if (res != 0 || drop_enable.empty()) {
        GST_DEBUG_OBJECT(video_display_sink, "sys.media.drop.video.buffer.enable");
        return;
    }
    GST_DEBUG_OBJECT(video_display_sink, "sys.media.drop.video.buffer.enable=%s", drop_enable.c_str());

    if (drop_enable == "false") {
        video_display_sink->priv->enable_drop = FALSE;
    }
}

static void gst_close_avsync_from_sys_param(GstVideoDisplaySink *video_display_sink)
{
    std::string avsync_close;
    video_display_sink->priv->close_avsync = FALSE;
    int32_t res = OHOS::system::GetStringParameter("sys.media.close.avsync", avsync_close, "");
    if (res != 0 || avsync_close.empty()) {
        GST_DEBUG_OBJECT(video_display_sink, "sys.media.close.avsync");
        return;
    }
    GST_DEBUG_OBJECT(video_display_sink, "sys.media.close.avsync=%s", avsync_close.c_str());

    if (avsync_close == "true") {
        video_display_sink->priv->close_avsync = TRUE;
    }
}

static void kpi_log_avsync_diff(GstVideoDisplaySink *video_display_sink, guint64 last_render_pts)
{
    GstVideoDisplaySinkPrivate *priv = video_display_sink->priv;
    guint64 audio_last_render_pts = 0;

    // get av sync diff time
    g_mutex_lock(&priv->mutex);
    if (priv->enable_kpi_avsync_log && priv->audio_sink != nullptr) {
    g_object_get(priv->audio_sink, "last-render-pts", &audio_last_render_pts, nullptr);
    GST_WARNING_OBJECT(video_display_sink, "KPI-TRACE: audio_last_render_pts=%" G_GUINT64_FORMAT
        ", video_last_render_pts=%" G_GUINT64_FORMAT ", diff=%" G_GINT64_FORMAT " ms",
        audio_last_render_pts, last_render_pts,
        ((gint64)audio_last_render_pts - (gint64)last_render_pts) / GST_MSECOND);
    }
    g_mutex_unlock(&priv->mutex);
}

static void gst_video_display_sink_get_render_time_diff_thd(GstVideoDisplaySink *video_display_sink,
    GstClockTime duration)
{
    if (!GST_CLOCK_TIME_IS_VALID(duration)) {
        return;
    }
    GstVideoDisplaySinkPrivate *priv = video_display_sink->priv;
    if (priv == nullptr) {
        return;
    }

    guint64 render_time_diff_thd = duration + DEFAULT_EXTRA_RENDER_FRAME_DIFF;
    if (render_time_diff_thd > DEFAULT_MAX_WAIT_CLOCK_TIME) {
        // Low framerate does not enter smoothing logic to prevent video render too fast.
        priv->render_time_diff_threshold = G_MAXUINT64;
        GST_DEBUG_OBJECT(video_display_sink, "render_time_diff_thd is greater than DEFAULT_MAX_WAIT_CLOCK_TIME");
    } else if (render_time_diff_thd != priv->render_time_diff_threshold) {
        priv->render_time_diff_threshold = render_time_diff_thd;
        GST_INFO_OBJECT(video_display_sink,
            "get new render_time_diff_threshold=%" G_GUINT64_FORMAT, render_time_diff_thd);
    }
}

static GstFlowReturn gst_video_display_sink_do_app_render(GstSurfaceMemSink *surface_sink,
    GstBuffer *buffer, bool is_preroll)
{
    (void)is_preroll;
    g_return_val_if_fail(surface_sink != nullptr && buffer != nullptr, GST_FLOW_ERROR);
    GstVideoDisplaySink *video_display_sink = GST_VIDEO_DISPLAY_SINK_CAST(surface_sink);

    kpi_log_avsync_diff(video_display_sink, GST_BUFFER_PTS(buffer));

    /* The value of GST_BUFFER_DURATION(buffer) is average duration, which has no reference
        value in the variable frame rate stream, because the actual duration of each frame varies greatly.
        It is difficult to obtain the duration of the current frame, so using the duration of the previous
        frame does not affect perception */
    GstClockTime last_duration = GST_BUFFER_PTS(buffer) - video_display_sink->priv->last_video_render_pts;
    if (GST_BUFFER_PTS(buffer) <= video_display_sink->priv->last_video_render_pts ||
        video_display_sink->priv->last_video_render_pts == 0) {
        last_duration = GST_BUFFER_DURATION(buffer);
    }

    GST_DEBUG_OBJECT(video_display_sink, "avg duration %" G_GUINT64_FORMAT ", last_duration %" G_GUINT64_FORMAT
        ", pts %" G_GUINT64_FORMAT, GST_BUFFER_DURATION(buffer), last_duration, GST_BUFFER_PTS(buffer));
    video_display_sink->priv->last_video_render_pts = GST_BUFFER_PTS(buffer);
    gst_video_display_sink_get_render_time_diff_thd(video_display_sink, last_duration);
    if (video_display_sink->priv->need_report_bandwidth) {
        g_signal_emit_by_name(video_display_sink, "bandwidth-change", video_display_sink->priv->bandwidth);
        video_display_sink->priv->need_report_bandwidth = FALSE;
    }
    return GST_FLOW_OK;
}

static GstClockTime gst_video_get_current_running_time(GstBaseSink *base_sink)
{
    GstClockTime base_time = gst_element_get_base_time(GST_ELEMENT(base_sink)); // get base time
    GstClockTime cur_clock_time = gst_clock_get_time(GST_ELEMENT_CLOCK(base_sink)); // get current clock time
    if (!GST_CLOCK_TIME_IS_VALID(base_time) || !GST_CLOCK_TIME_IS_VALID(cur_clock_time)) {
        return GST_CLOCK_TIME_NONE;
    }
    if (cur_clock_time < base_time) {
        return GST_CLOCK_TIME_NONE;
    }
    return cur_clock_time - base_time;
}

static void gst_video_display_sink_adjust_reach_time_handle(GstVideoDisplaySink *video_display_sink,
    GstClockTime &reach_time, gint64 video_running_time_diff, gint64 audio_running_time_diff,
    gboolean *need_drop_this_buffer)
{
    GstVideoDisplaySinkPrivate *priv = video_display_sink->priv;

    if (video_running_time_diff - audio_running_time_diff > priv->video_delay_time + DEFAULT_VIDEO_BEHIND_AUDIO_THD) {
        if (priv->enable_drop == TRUE) {
            if (priv->buffer_count % DEFAULT_DROP_BEHIND_VIDEO_BUF_FREQUENCY == 0) {
                GST_WARNING_OBJECT(video_display_sink, "drop this video buffer, num:%" G_GUINT64_FORMAT,
                    priv->total_video_buffer_num);
                *need_drop_this_buffer = TRUE;
                priv->dropped_video_buffer_num++;
            } else {
                priv->buffer_count++;
                return;
            }
        }
    } else if (video_running_time_diff < audio_running_time_diff &&
               (audio_running_time_diff - video_running_time_diff) > DEFAULT_AUDIO_RUNNING_TIME_DIFF_THD) {
        GST_INFO_OBJECT(video_display_sink, "audio is too late, adjust video reach_time, video_running_time_diff=%"
            G_GUINT64_FORMAT "audio_running_time_diff=%" G_GINT64_FORMAT ", old reach_time=%"
            G_GUINT64_FORMAT ", new reach_time=%" G_GUINT64_FORMAT,
            video_running_time_diff, audio_running_time_diff, reach_time,
            audio_running_time_diff - video_running_time_diff);
        // The deviation between sound and image exceeds 5ms
        reach_time += (audio_running_time_diff - video_running_time_diff);
    }

    if (priv->enable_drop == TRUE) {
        priv->buffer_count = 1;
    }
}

static GstClockTime gst_video_display_sink_adjust_reach_time_by_jitter(GstBaseSink *base_sink,
    GstVideoDisplaySink *video_display_sink, GstClockTime reach_time, gboolean *need_drop_this_buffer)
{
    GstVideoDisplaySinkPrivate *priv = video_display_sink->priv;
    if (priv == nullptr) {
        return reach_time;
    }

    g_mutex_lock(&priv->mutex);
    if (priv->audio_sink != nullptr) {
        gint64 audio_running_time_diff = 0;
        g_object_get(priv->audio_sink, "last-running-time-diff", &audio_running_time_diff, nullptr);
        GstClockTime cur_running_time = gst_video_get_current_running_time(base_sink);
        g_return_val_if_fail(GST_CLOCK_TIME_IS_VALID(cur_running_time), reach_time);
        gint64 video_running_time_diff = cur_running_time - reach_time;

        GST_LOG_OBJECT(video_display_sink, "videosink buffer num:%" G_GUINT64_FORMAT ", video_running_time_diff:%"
            G_GINT64_FORMAT " = cur_running_time:%" G_GUINT64_FORMAT " - reach_time:%" G_GUINT64_FORMAT
            ", audio_running_time_diff:%" G_GINT64_FORMAT ", buffer_count:%u", priv->total_video_buffer_num,
            video_running_time_diff, cur_running_time, reach_time, audio_running_time_diff,
            priv->buffer_count);

        gst_video_display_sink_adjust_reach_time_handle(video_display_sink, reach_time,
            video_running_time_diff, audio_running_time_diff, need_drop_this_buffer);
    }
    g_mutex_unlock(&priv->mutex);
    return reach_time;
}

static GstClockTime gst_video_display_sink_update_reach_time(GstBaseSink *base_sink, GstClockTime reach_time,
    gboolean *need_drop_this_buffer)
{
    g_return_val_if_fail(base_sink != nullptr, reach_time);
    g_return_val_if_fail(GST_CLOCK_TIME_IS_VALID(reach_time), reach_time);
    GstVideoDisplaySink *video_display_sink = GST_VIDEO_DISPLAY_SINK_CAST(base_sink);
    GstVideoDisplaySinkPrivate *priv = video_display_sink->priv;
    if (priv == nullptr || priv->render_time_diff_threshold == G_MAXUINT64 || priv->close_avsync == TRUE) {
        return reach_time;
    }
    g_return_val_if_fail(priv != nullptr && priv->audio_sink != nullptr, reach_time);
    g_return_val_if_fail(priv->render_time_diff_threshold != G_MAXUINT64, reach_time);
    g_return_val_if_fail(priv->close_avsync != TRUE, reach_time);

    priv->total_video_buffer_num++;

    // 1st: update reach_time by audio running time jitter
    GstClockTime new_reach_time = gst_video_display_sink_adjust_reach_time_by_jitter(base_sink, video_display_sink,
        reach_time, need_drop_this_buffer);
    g_return_val_if_fail(GST_CLOCK_TIME_IS_VALID(new_reach_time), reach_time);
    guint dynamic_delay = 0;
    if (new_reach_time > reach_time) {
        dynamic_delay = new_reach_time - reach_time;
    }

    // 2ed: update reach_time if the running_time_diff exceeded the threshold
    guint static_delay = 0;
    g_object_get(priv->audio_sink, "audio-delay-time", &static_delay, nullptr);
    priv->audio_delay_time = static_delay + dynamic_delay;
    GST_INFO_OBJECT(video_display_sink, "audio_dellay_time:%d", priv->audio_delay_time);

    if (priv->start_first_render) {
        priv->video_delay_time = 0;
        priv->start_first_render = FALSE;
    }

    // 3th smotth transition
    if (priv->video_delay_time < priv->audio_delay_time &&
        priv->audio_delay_time - priv->video_delay_time > DEFAULT_EXTRA_RENDER_FRAME_DIFF) {
        priv->video_delay_time += DEFAULT_EXTRA_RENDER_FRAME_DIFF;
    }

    if (priv->video_delay_time > priv->audio_delay_time &&
        priv->video_delay_time - priv->audio_delay_time > DEFAULT_EXTRA_RENDER_FRAME_DIFF) {
        priv->video_delay_time -= DEFAULT_EXTRA_RENDER_FRAME_DIFF;
    }

    new_reach_time = reach_time + priv->video_delay_time;
    if (new_reach_time != reach_time) {
        GST_INFO_OBJECT(video_display_sink,
            " old reach_time:%" G_GUINT64_FORMAT
            " new reach_time:%" G_GUINT64_FORMAT
            " video delay time:%u", reach_time, new_reach_time, priv->video_delay_time);
    }
    return new_reach_time;
}
