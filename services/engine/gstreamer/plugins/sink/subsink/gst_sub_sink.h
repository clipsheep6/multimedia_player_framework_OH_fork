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

#ifndef GST_SUB_SINK_H
#define GST_SUB_SINK_H

#include <memory>
#include <gst/base/gstbasesink.h>
#include <gst/app/gstappsink.h>
#include "task_queue.h"

G_BEGIN_DECLS

#define GST_TYPE_SUB_SINK (gst_sub_sink_get_type())
#define GST_SUB_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SUB_SINK, GstSubSink))
#define GST_SUB_SINK_CLASS(kclass) \
    (G_TYPE_CHECK_CLASS_CAST((kclass), GST_TYPE_SUB_SINK, GstSubSinkClass))
#define GST_IS_SUB_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SUB_SINK))
#define GST_IS_SUB_SINK_CLASS(kclass) \
    (G_TYPE_CHECK_CLASS_TYPE((kclass), GST_TYPE_SUB_SINK))
#define GST_SUB_SINK_CAST(obj) ((GstSubSink*)(obj))
#define GST_SUB_SINK_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_SUB_SINK, GstSubSinkClass))

#ifndef GST_API_EXPORT
#define GST_API_EXPORT __attribute__((visibility("default")))
#endif

typedef struct _GstSubSink GstSubSink;
typedef struct _GstSubSinkClass GstSubSinkClass;
typedef struct _GstSubSinkPrivate GstSubSinkPrivate;

typedef struct {
    GstFlowReturn (*new_sample)(GstBuffer *sample, gpointer user_data);
} GstSubSinkCallbacks;

struct _GstSubSink {
    GstAppSink appsink;
    gboolean is_flushing;
    /* private */
    GstSubSinkPrivate *priv;
};

struct _GstSubSinkClass {
    GstAppSinkClass parent_class;
    void (*handle_buffer)(GstSubSink *sub_sink, GstBuffer *buffer, gboolean cancel, guint64 delayUs);
    void (*cancel_not_executed_task)(GstSubSink *subsink);
    GstFlowReturn (*subtitle_display_callback)(GstAppSink *appsink, gpointer user_data);
};

GST_API_EXPORT GType gst_sub_sink_get_type(void);

/**
 * @brief call this interface to set the notifiers for new_sample.
 *
 * @param sub_sink the sink element instance
 * @param callbacks callbacks, refer to {@GstSubSinkCallbacks}
 * @param user_data will be passed to callbacks
 * @param notify the function to be used to destroy the user_data when the sub_sink is disposed
 * @return void.
 */
GST_API_EXPORT void gst_sub_sink_set_callback(GstSubSink *sub_sink,
                                              GstSubSinkCallbacks *callbacks,
                                              gpointer user_data,
                                              GDestroyNotify notify);
#ifdef G_DEFINE_AUTOPTR_CLEANUP_FUNC
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GstSubSink, gst_object_unref)
#endif

G_END_DECLS
#endif // GST_SUB_SINK_H
