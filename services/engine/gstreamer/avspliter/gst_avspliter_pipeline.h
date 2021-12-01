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

#ifndef GST_AVSPLITER_PIPELINE_H
#define GST_AVSPLITER_PIPELINE_H

#include <gst/gst.h>

typedef struct _GstAVSpliterPipeline GstAVSpliterPipeline;
typedef struct _GstAVSpliterPipelineClass GstAVSpliterPipelineClass;
typedef struct _GstAVSpliterStream GstAVSpliterStream;

#define GST_TYPE_AVSPLITER_PIPELINE (gst_element_get_type ())
#define GST_IS_AVSPLITER_PIPELINE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_AVSPLITER_PIPELINE))
#define GST_IS_AVSPLITER_PIPELINE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_AVSPLITER_PIPELINE))
#define GST_AVSPLITER_PIPELINE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_AVSPLITER_PIPELINE, GstAVSpliterPipelineClass))
#define GST_AVSPLITER_PIPELINE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_AVSPLITER_PIPELINE, GstAVSpliterPipeline))
#define GST_AVSPLITER_PIPELINE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_AVSPLITER_PIPELINE, GstAVSpliterPipelineClass))
#define GST_AVSPLITER_PIPELINE_CAST(obj) ((GstAVSpliterPipeline *)(obj))

#define GST_AVSPLITER_STREAM(obj) ((GstAVSpliterStream *)(obj))

struct _GstAVSpliterStream {
    GstPad *avsBinPad; // not ref
    GstElement *shmemSink; // not ref
    gboolean inBin;
    GstPad *sinkpad;
    gulong sinkPadProbeId;
};

struct _GstAVSpliterPipeline {
    GstPipeline parent;
    gchar *uri;
    GstElement *urisourcebin; // not ref
    gulong urisrcbinPadAddedId;
    GstElement *avsBin; // not ref
    gulong avsbinPadAddedId;
    gulong avsbinPadRemovedId;
    gulong avsbinNoMorePads;
    GList *streams;
    gboolean asyncPending;
};

struct _GstAVSpliterPipelineClass {
    GstPipelineClass parent_class;
};

#endif
