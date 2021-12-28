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

#ifndef GST_BYTRACE_TRACER_H
#define GST_BYTRACE_TRACER_H

#include <gst/gst.h>
#include <gst/gsttracer.h>

G_BEGIN_DECLS

typedef struct _GstBytraceTracer GstBytraceTracer;
typedef struct _GstBytraceTracerClass GstBytraceTracerClass;

#define GST_TYPE_BYTRACE_TRACER (gst_bytrace_tracer_get_type())
#define GST_BYTRACE_TRACER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_BYTRACE_TRACER, GstBytraceTracer))
#define GST_BYTRACE_TRACER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_BYTRACE_TRACER, GstBytraceTracerClass))
#define GST_IS_BYTRACE_TRACER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_BYTRACE_TRACER))
#define GST_IS_BYTRACE_TRACER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_BYTRACE_TRACER))
#define GST_BYTRACE_TRACER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_BYTRACE_TRACER, GstBytraceTracerClass))
#define GST_BYTRACE_TRACER_CAST(obj) ((GstBytraceTracer *)(obj))

struct _GstBytraceTracer {
  GstObject        parent;
};

struct _GstBytraceTracerClass {
  GstObjectClass parent_class;
};

GType gst_bytrace_tracer_get_type (void);

#ifdef G_DEFINE_AUTOPTR_CLEANUP_FUNC
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GstBytraceTracer, gst_object_unref)
#endif

G_END_DECLS

#endif
