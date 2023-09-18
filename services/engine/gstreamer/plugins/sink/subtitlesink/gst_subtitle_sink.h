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

#ifndef GST_SUBTITLE_SINK_H
#define GST_SUBTITLE_SINK_H

#include <memory>
#include <gst/base/gstbasesink.h>
#include <gst/app/gstappsink.h>
#include "task_queue.h"

G_BEGIN_DECLS

#define GST_TYPE_SUBTITLE_SINK (gst_subtitle_sink_get_type())
#define GST_SUBTITLE_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SUBTITLE_SINK, GstSubtitleSink))
#define GST_SUBTITLE_SINK_CLASS(kclass) \
    (G_TYPE_CHECK_CLASS_CAST((kclass), GST_TYPE_SUBTITLE_SINK, GstSubtitleSinkClass))
#define GST_IS_SUBTITLE_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SUBTITLE_SINK))
#define GST_IS_SUBTITLE_SINK_CLASS(kclass) \
    (G_TYPE_CHECK_CLASS_TYPE((kclass), GST_TYPE_SUBTITLE_SINK))
#define GST_SUBTITLE_SINK_CAST(obj) ((GstSubtitleSink*)(obj))
#define GST_SUBTITLE_SINK_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_SUBTITLE_SINK, GstSubtitleSinkClass))

#ifndef GST_API_EXPORT
#define GST_API_EXPORT __attribute__((visibility("default")))
#endif

using GstSubtitleSink = struct _GstSubtitleSink;
using GstSubtitleSinkClass = struct _GstSubtitleSinkClass;
using GstSubtitleSinkPrivate = struct _GstSubtitleSinkPrivate;

using GstSubtitleSinkCallbacks = struct {
    GstFlowReturn (*new_sample)(GstBuffer *sample, gpointer user_data);
};

struct _GstSubtitleSink {
    GstAppSink appsink;
    gboolean stop_render;
    gboolean have_first_segment;
    gboolean audio_segment_updated;
    gboolean segment_updated;
    gboolean is_changing_track;
    gboolean enable_display;
    gboolean need_send_empty_buffer;
    gboolean have_first_filter;
    GstBuffer *preroll_buffer;
    guint64 track_changing_position;
    guint64 init_position;
    gdouble rate;
    GCond segment_cond;
    GMutex segment_mutex;
    GstSegment segment;
    GstSeekFlags seek_flags;
    /* private */
    GstSubtitleSinkPrivate *priv;
};

struct _GstSubtitleSinkClass {
    GstAppSinkClass parent_class;
};

GST_API_EXPORT GType gst_subtitle_sink_get_type(void);

/**
 * @brief call this interface to set the notifiers for new_sample.
 *
 * @param subtitle_sink the sink element instance
 * @param callbacks callbacks, refer to {@GstSubtitleSinkCallbacks}
 * @param user_data will be passed to callbacks
 * @param notify the function to be used to destroy the user_data when the subtitle_sink is disposed
 * @return void.
 */
GST_API_EXPORT void gst_subtitle_sink_set_callback(GstSubtitleSink *subtitle_sink,
    GstSubtitleSinkCallbacks *callbacks, gpointer user_data, GDestroyNotify notify);
#ifdef G_DEFINE_AUTOPTR_CLEANUP_FUNC
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GstSubtitleSink, gst_object_unref)
#endif

G_END_DECLS
#endif // GST_SUBTITLE_SINK_H
