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

#ifndef GST_AVSPLITER_H
#define GST_AVSPLITER_H

#include <gst/gst.h>
#include "gst/base/gstqueuearray.h"

typedef struct _GstAVSpliter GstAVSpliter;
typedef struct _GstAVSpliterClass GstAVSpliterClass;
typedef struct _GstAVSpliterStream GstAVSpliterStream;
typedef struct _GstAVSpliterMediaInfo GstAVSpliterMediaInfo;
typedef struct _GstAVSpliterMediaInfoClass GstAVSpliterMediaInfoClass;

#define GST_TYPE_AVSPLITER (gst_element_get_type ())
#define GST_IS_AVSPLITER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_AVSPLITER))
#define GST_IS_AVSPLITER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_AVSPLITER))
#define GST_AVSPLITER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_AVSPLITER, GstAVSpliterClass))
#define GST_AVSPLITER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_AVSPLITER, GstAVSpliter))
#define GST_AVSPLITER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_AVSPLITER, GstAVSpliterClass))
#define GST_AVSPLITER_CAST(obj) ((GstAVSpliter *)(obj))

#define GST_AVSPLITER_STREAM(obj) ((GstAVSpliterStream *)(obj))

enum TrackPullReturn {
    TRACK_PULL_UNSELECTED,
    TRACK_PULL_ENDTIME,
    TRACK_PULL_EOS,
    TRACK_PULL_CACHE_EMPTY,
    TRACK_PULL_SUCCESS,
};

struct _GstAVSpliterStream {
    guint id;
    GstPad *avsBinPad; // not ref
    GstElement *shmemSink; // not ref
    gboolean inBin;
    GstPad *sinkpad;
    gulong sinkPadProbeId;
    GstStream *info;

    // cache sample info
    guint cacheLimit;
    GstQueueArray *sampleQueue;
    guint cacheSize;
    gboolean eos;
    GstClockTime lastPos;
    gboolean selected;
    TrackPullReturn lastPullRet;
};

struct _GstAVSpliter {
    GstPipeline parent;
    gchar *uri;
    GstElement *urisourcebin; // not ref
    gulong urisrcbinPadAddedId;
    GstElement *avsBin; // not ref
    gulong avsbinPadAddedId;
    gulong avsbinPadRemovedId;
    gulong avsbinNoMorePadsId;
    gulong avsbinHaveTypeId;
    gboolean noMorePads;
    gboolean asyncPending;
    GArray *streams;
    guint defaultTrackId;
    gboolean hasVideo;
    gboolean hasAudio;
    GMutex sampleLock;
    GCond sampleCond;
    GstAVSpliterMediaInfo *mediaInfo;
};

struct _GstAVSpliterClass {
    GstPipelineClass parent_class;
};

GST_EXPORT GType gst_avspliter_get_type(void);

GST_EXPORT GstAVSpliterMediaInfo *gst_avspliter_get_media_info(GstAVSpliter *avspliter);

GST_EXPORT gboolean gst_avspliter_select_track(GstAVSpliter *avspliter, guint trackIdx, gboolean select);

GST_EXPORT GList *gst_avspliter_pull_samples(GstAVSpliter *avspliter,
    GstClockTime starttime, GstClockTime endtime, guint bufcnt);

GST_EXPORT gboolean gst_avspliter_track_is_eos(GstAVSpliter *avspliter, guint trackIdx);

GST_EXPORT gboolean gst_avspliter_seek(GstAVSpliter *avspliter, GstClockTime pos, GstSeekFlags flags);

#define GST_TYPE_AVSPLITER_MEDIA_INFO (gst_avspliter_media_info_get_type())
#define GST_AVSPLITER_MEDIA_INFO(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_AVSPLITER_MEDIA_INFO, GstAVSpliterMediaInfo))
#define GST_AVSPLITER_MEDIA_INFO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_AVSPLITER_MEDIA_INFO, GstAVSpliterMediaInfoClass))
#define GST_IS_AVSPLITER_MEDIA_INFO(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_AVSPLITER_MEDIA_INFO))
#define GST_IS_AVSPLITER_MEDIA_INFO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_AVSPLITER_MEDIA_INFO))

struct _GstAVSpliterMediaInfo {
    GObject parent;
    GstCaps *containerCaps;
    GstTagList *globalTags;
    GList *streams; // GstStream
};

struct _GstAVSpliterMediaInfoClass {
    GObjectClass parent_class;
};

GST_EXPORT GType gst_avspliter_media_info_get_type(void);

#endif
